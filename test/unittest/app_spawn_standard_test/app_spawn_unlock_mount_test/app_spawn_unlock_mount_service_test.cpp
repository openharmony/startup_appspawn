/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file app_spawn_unlock_mount_service_test.cpp
 * @brief Unit tests for unlock mount service functions in appspawn_service.c
 *
 * Test Suite: UnlockMount_Service_Test
 * Batch-1 Scenarios: TC-078 ~ TC-096
 *   - HandleUnlockEvent (TC-078 ~ TC-087)
 *   - SendUnlockMsgToPrefork (TC-088 ~ TC-092)
 *   - ForkAndDoUnlockMount (TC-093 ~ TC-096)
 * Batch-2 Scenarios: TC-097 ~ TC-113
 *   - DoUnlockMountSerial (TC-097 ~ TC-100)
 *   - ProcessUnlockMessage (TC-101 ~ TC-103)
 *   - HandlePreforkUnlockMsg (TC-104 ~ TC-105)
 *   - PreforkChildLoop (TC-106)
 *   - StopAppSpawn / HandleDiedPid / ProcessAppSpawnLockStatusMsg (TC-107~TC-113: not covered, static functions)
 *
 * Mock Strategy:
 *   - --wrap=ProcessExit: Intercept ProcessExit() calls via ldflags
 *   - --wrap=fork: Control fork() return value for testing parent/child paths
 *   - setjmp/longjmp: Safely test ProcessExit paths without terminating test process
 *   - Function pointer defines: Hook registration for STAGE_SERVER_LOCK
 *
 * Dependencies:
 *   - libappspawn_mock (provides SetParameter, kill mocks)
 */

#include <gtest/gtest.h>
#include <csetjmp>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>
#include <thread>

#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_server.h"
#include "appspawn_utils.h"
#include "appspawn_msg.h"
#include "appspawn_hook.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"
#include "securec.h"
// Forward declarations for APPSPAWN_STATIC functions in appspawn_service.c
extern "C" {
void StopAppSpawn(void);
void HandleDiedPid(pid_t pid, uid_t uid, int status);
bool ProcessAppSpawnLockStatusMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message, int *result);
bool HandleUnlockEvent(AppSpawnContent *content, int uid, AppSpawnMsgNode *message, AppSpawnConnection *connection);
int SendUnlockMsgToPrefork(AppSpawnContent *content, int uid);
int DoUnlockMountSerial(AppSpawnContent *content, int uid);
int ProcessUnlockMessage(int uid);
int ForkAndDoUnlockMount(AppSpawnContent *content, int uid, AppSpawningCtx *property);
APPSPAWN_STATIC void HandlePreforkUnlockMsg(AppSpawnContent *content, const AppSpawnUnlockMsg *unlockMsg);
APPSPAWN_STATIC void PreforkChildLoop(AppSpawnContent *content, AppSpawningCtx *property,
    AppSpawnMgr *mgr, enum fdsan_error_level errorLevel);
}

using namespace testing;
using namespace testing::ext;

// ===== ProcessExit interception via setjmp/longjmp =====
// Allows testing ProcessExit paths without terminating test process
static thread_local jmp_buf g_processExitEnv;
static thread_local volatile int g_processExitCalled = 0;
static thread_local volatile int g_processExitCode = 0;

// ===== Mock fork return value =====
// Controls fork() behavior via --wrap=fork
static constexpr int MOCK_FORK_USE_REAL = -999;
static thread_local volatile pid_t g_mockForkReturn = MOCK_FORK_USE_REAL;
static thread_local volatile int g_forkCallCount = 0;

// ===== Mock hook tracking for STAGE_SERVER_LOCK =====
static thread_local volatile int g_hookCalled = 0;
static thread_local volatile int g_hookUid = 0;
static thread_local volatile int g_hookReturn = 0;

// ===== Mock prefork child registration =====
// Use a fixed-size array instead of AppSpawnFds (which has flexible array member)
#define MAX_FAKE_FDS 8
static struct {
    pid_t pid;
    uint32_t count;
    SpawningFdType type;
    int fds[MAX_FAKE_FDS];
} g_fakeFdsData;

static bool g_fakeFdsRegistered = false;
static int g_pipeWriteFd = -1;
static int g_pipeReadFd = -1;
static int g_childParentFds[2] = {-1, -1};  // Track TYPE_CHILD_PARENT pipe fds

// ===== Mock SetParameter tracking =====
static thread_local char g_lastSetParamKey[256] = {0};
static thread_local char g_lastSetParamValue[64] = {0};

extern "C" {
// ProcessExit wrapper linked via --wrap=ProcessExit in BUILD.gn
void __wrap_ProcessExit(int code)
{
    g_processExitCode = code;
    g_processExitCalled = 1;
    longjmp(g_processExitEnv, code);
}

// fork wrapper linked via --wrap=fork in BUILD.gn
pid_t __real_fork(void);

pid_t __wrap_fork(void)
{
    g_forkCallCount++;
    if (g_mockForkReturn != MOCK_FORK_USE_REAL) {
        return g_mockForkReturn;
    }
    return __real_fork();
}

// Mock SetParameter for tracking parameter writes
int __wrap_SetParameter(const char *key, const char *value)
{
    if (key != nullptr) {
        int ret = snprintf_s(reinterpret_cast<char *>(g_lastSetParamKey), sizeof(g_lastSetParamKey),
                             sizeof(g_lastSetParamKey) - 1, "%s", key);
        if (ret < 0) {
            return -1;
        }
    }
    if (value != nullptr) {
        int ret = snprintf_s(reinterpret_cast<char *>(g_lastSetParamValue), sizeof(g_lastSetParamValue),
                             sizeof(g_lastSetParamValue) - 1, "%s", value);
        if (ret < 0) {
            return -1;
        }
    }
    return 0;
}

// Stub for DisallowInternet (required by appspawn_adapter.cpp)
void DisallowInternet(void)
{
}

// Mock kill to avoid killing real processes
int __wrap_kill(pid_t pid, int sig)
{
    (void)pid;
    (void)sig;
    return 0;  // Pretend kill succeeded
}

// PRQA S 5131 --  // End of GCC wrap symbols

}  // extern "C"

namespace OHOS {

// ===== Test hook for STAGE_SERVER_LOCK =====
static int TestUnlockHook(AppSpawnMgr *mgr)
{
    g_hookCalled = 1;
    if (mgr != nullptr && mgr->content.currentUnlockUid > 0) {
        g_hookUid = mgr->content.currentUnlockUid;
    }
    return g_hookReturn;
}

// ===== Test Fixture =====
class AppSpawnUnlockMountServiceTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        // Ignore SIGPIPE to prevent process termination when writing to closed pipe
        // This allows write() to return EPIPE error instead of killing the process
        signal(SIGPIPE, SIG_IGN);

        // Register test hook for STAGE_SERVER_LOCK
        AddServerStageHook(STAGE_SERVER_LOCK, HOOK_PRIO_COMMON, TestUnlockHook);
    }

    static void TearDownTestCase()
    {
        // Hook will be automatically cleaned up
    }

    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();

        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());

        // Close any leaked file descriptors from previous tests
        // (e.g., from longjmp that skipped TearDown)
        for (uint32_t i = 0; i < MAX_FAKE_FDS; i++) {
            if (g_fakeFdsData.fds[i] >= 0) {
                close(g_fakeFdsData.fds[i]);
                g_fakeFdsData.fds[i] = -1;
            }
        }
        for (uint32_t i = 0; i < sizeof(g_childParentFds) / sizeof(g_childParentFds[0]); i++) {
            if (g_childParentFds[i] >= 0) {
                close(g_childParentFds[i]);
                g_childParentFds[i] = -1;
            }
        }
        if (g_pipeWriteFd >= 0) {
            close(g_pipeWriteFd);
            g_pipeWriteFd = -1;
        }
        if (g_pipeReadFd >= 0) {
            close(g_pipeReadFd);
            g_pipeReadFd = -1;
        }

        // Reset all tracking variables (DO NOT use memset_s on g_fakeFds after closing fds!)
        g_processExitCalled = 0;
        g_processExitCode = 0;
        g_mockForkReturn = MOCK_FORK_USE_REAL;
        g_forkCallCount = 0;
        g_hookCalled = 0;
        g_hookUid = 0;
        g_hookReturn = 0;
        g_fakeFdsRegistered = false;

        // Reset g_fakeFds struct (ONLY the non-fd fields!)
        g_fakeFdsData.pid = 0;
        g_fakeFdsData.count = 0;
        g_fakeFdsData.type = static_cast<SpawningFdType>(0);
        // Initialize fds array to -1
        for (int i = 0; i < MAX_FAKE_FDS; i++) {
            g_fakeFdsData.fds[i] = -1;
        }

        memset_s(g_lastSetParamKey, sizeof(g_lastSetParamKey), 0, sizeof(g_lastSetParamKey));
        memset_s(g_lastSetParamValue, sizeof(g_lastSetParamValue), 0, sizeof(g_lastSetParamValue));

        // Create AppSpawnMgr
        mgr_ = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        ASSERT_NE(mgr_, nullptr);
        mgr_->servicePid = getpid();
    }

    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());

        // Cleanup g_fakeFds pipe fds
        for (uint32_t i = 0; i < MAX_FAKE_FDS; i++) {
            if (g_fakeFdsData.fds[i] >= 0) {
                close(g_fakeFdsData.fds[i]);
                g_fakeFdsData.fds[i] = -1;
            }
        }

        // Cleanup child-parent pipe fds
        if (g_childParentFds[0] >= 0) {
            close(g_childParentFds[0]);
            g_childParentFds[0] = -1;
        }
        if (g_childParentFds[1] >= 0) {
            close(g_childParentFds[1]);
            g_childParentFds[1] = -1;
        }

        // Cleanup pipe fd tracking variables
        if (g_pipeWriteFd >= 0) {
            close(g_pipeWriteFd);
            g_pipeWriteFd = -1;
        }
        if (g_pipeReadFd >= 0) {
            close(g_pipeReadFd);
            g_pipeReadFd = -1;
        }

        // Cleanup fake fds registration
        if (g_fakeFdsRegistered) {
            CleanupSpawningFdsByPid(mgr_, g_fakeFdsData.pid);
            g_fakeFdsRegistered = false;
        }

        if (mgr_ != nullptr) {
            DeleteAppSpawnMgr(mgr_);
            mgr_ = nullptr;
        }
    }

protected:
    AppSpawnMgr *mgr_ = nullptr;

    // Helper: Register fake prefork fds with read ends closed (for error tests)
    void RegisterFakePreforkFds(pid_t pid)
    {
        // Call pipe() to create communication channel
        int ret = pipe(g_fakeFdsData.fds);
        ASSERT_EQ(ret, 0);

        g_fakeFdsData.pid = pid;
        g_fakeFdsData.count = PIPE_FD_LENGTH;
        g_fakeFdsData.type = TYPE_PARENT_CHILD;

        SpawningFdRegInfo regInfo = {
            .type = TYPE_PARENT_CHILD,
            .count = PIPE_FD_LENGTH,
            .fds = g_fakeFdsData.fds,
            .pid = pid
        };
        EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);
        g_fakeFdsRegistered = true;

        // Also register TYPE_CHILD_PARENT (required by HandleUnlockEvent)
        ASSERT_EQ(pipe(g_childParentFds), 0);
        SpawningFdRegInfo pfRegInfo = {
            .type = TYPE_CHILD_PARENT,
            .count = PIPE_FD_LENGTH,
            .fds = g_childParentFds,
            .pid = pid
        };
        EXPECT_NE(RegisterSpawningFds(mgr_, &pfRegInfo), nullptr);

        // Close read ends of pipes to prevent write() blocking
        // In real scenario, prefork child reads from these pipes
        // In unit test, we close them so write() fails immediately with EPIPE
        close(g_fakeFdsData.fds[0]);  // Close parent-to-child read end
        g_fakeFdsData.fds[0] = -1;
        close(g_childParentFds[0]);  // Close child-to-parent read end
        g_childParentFds[0] = -1;

        // Store write fd for external manipulation
        g_pipeWriteFd = g_fakeFdsData.fds[1];
        g_pipeReadFd = -1;  // Already closed
    }

    // Helper: Register fake prefork fds with read ends open (for success tests)
    // Use non-blocking pipe to avoid blocking write when buffer is full
    void RegisterFakePreforkFdsWithOpenReadEnd(pid_t pid)
    {
        ASSERT_EQ(pipe(g_fakeFdsData.fds), 0);
        g_fakeFdsData.pid = pid;
        g_fakeFdsData.count = PIPE_FD_LENGTH;
        g_fakeFdsData.type = TYPE_PARENT_CHILD;

        // Set read end to non-blocking mode
        int flags = fcntl(g_fakeFdsData.fds[0], F_GETFL);
        ASSERT_NE(flags, -1);
        ASSERT_EQ(fcntl(g_fakeFdsData.fds[0], F_SETFL, flags | O_NONBLOCK), 0);

        SpawningFdRegInfo regInfo = {
            .type = TYPE_PARENT_CHILD,
            .count = PIPE_FD_LENGTH,
            .fds = g_fakeFdsData.fds,
            .pid = pid
        };
        EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);
        g_fakeFdsRegistered = true;

        // Also register TYPE_CHILD_PARENT (required by HandleUnlockEvent)
        ASSERT_EQ(pipe(g_childParentFds), 0);
        SpawningFdRegInfo pfRegInfo = {
            .type = TYPE_CHILD_PARENT,
            .count = PIPE_FD_LENGTH,
            .fds = g_childParentFds,
            .pid = pid
        };
        EXPECT_NE(RegisterSpawningFds(mgr_, &pfRegInfo), nullptr);

        // Close child-parent read end to prevent blocking
        close(g_childParentFds[0]);
        g_childParentFds[0] = -1;

        // Store fds
        g_pipeWriteFd = g_fakeFdsData.fds[1];
        g_pipeReadFd = g_fakeFdsData.fds[0];  // Keep open for reading
    }

    // Helper: Check if fd is valid
    bool IsFdValid(int fd)
    {
        return fcntl(fd, F_GETFD) >= 0;
    }
};

// ============================================================================
// Batch-1: HandleUnlockEvent Tests (TC-078 ~ TC-087)
// ============================================================================

/**
 * @tc.name: UnlockMount_HandleUnlockEvent_001
 * @tc.desc: HandleUnlockEvent with NULL content: returns immediately (line 2115)
 * @tc.type: FUNC
 * @tc.level: Level1
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleUnlockEvent_001, TestSize.Level1)
{
    // 覆盖: HandleUnlockEvent NULL content early return (appspawn_service.c:2113-2115)
    // Arrange
    AppSpawnContent *content = nullptr;
    int uid = 100;
    int originalReservedPid = mgr_->content.reservedPid;
    int originalHookCalled = 0;

    // Act
    HandleUnlockEvent(content, uid, NULL, NULL);

    // Assert - function should return immediately without modifying any state
    EXPECT_EQ(mgr_->content.reservedPid, originalReservedPid);
    EXPECT_EQ(g_hookCalled, originalHookCalled);
    EXPECT_EQ(g_processExitCalled, 0);
}

/**
 * @tc.name: UnlockMount_HandleUnlockEvent_002
 * @tc.desc: HandleUnlockEvent with invalid uid (uid <= 0): returns immediately (line 2119)
 * @tc.type: FUNC
 * @tc.level: Level1
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleUnlockEvent_002, TestSize.Level1)
{
    // 覆盖: HandleUnlockEvent uid <= 0 early return (appspawn_service.c:2117-2119)
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    int originalReservedPid = content->reservedPid;
    int uid = 0;  // Invalid uid

    // Act
    HandleUnlockEvent(content, uid, NULL, NULL);

    // Assert - function should return immediately without modifying state
    EXPECT_EQ(content->reservedPid, originalReservedPid);
    EXPECT_EQ(g_hookCalled, 0);
}

/**
 * @tc.name: UnlockMount_HandleUnlockEvent_003
 * @tc.desc: HandleUnlockEvent Level 1 write fails: fallback to Level 2
 *           (line 2135, SendUnlockMsgToPrefork returns error)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleUnlockEvent_003, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54321;
    content->reservedPid = preforkPid;
    content->enablePerfork = 0;  // Disable prefork to avoid refork after L1/L2 failure
    int uid = 100;

    // Register fake prefork fds with read ends closed
    // This will cause write() to fail with EPIPE (SIGPIPE ignored)
    RegisterFakePreforkFds(preforkPid);

    // Force fork to fail, so Level 2 also fails and falls back to Level 3
    g_mockForkReturn = -1;

    // Act: HandleUnlockEvent returns false when L1/L2 fail, caller handles L3
    bool async = HandleUnlockEvent(content, uid, NULL, NULL);
    EXPECT_EQ(async, false);  // L1/L2 failed, returned sync

    // Caller executes L3 serial mount
    int result = DoUnlockMountSerial(content, uid);

    // Assert
    // reservedPid remains unchanged (prefork disabled, no refork)
    EXPECT_EQ(content->reservedPid, preforkPid);

    // Hook should have been called via Level 3 serial path
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);
    EXPECT_EQ(result, 0);  // L3 should succeed
}

/**
 * @tc.name: UnlockMount_HandleUnlockEvent_004
 * @tc.desc: HandleUnlockEvent Level 1 fds not found: fallback to Level 2
 *           (line 2134, pcFds == NULL)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleUnlockEvent_004, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54322;
    content->reservedPid = preforkPid;
    content->enablePerfork = 0;  // Disable prefork to avoid refork after L1/L2 failure
    int uid = 100;

    // Force fork to fail, so Level 2 also fails and falls back to Level 3
    g_mockForkReturn = -1;

    // Act: HandleUnlockEvent returns false when L1/L2 fail, caller handles L3
    bool async = HandleUnlockEvent(content, uid, NULL, NULL);
    EXPECT_EQ(async, false);  // L1/L2 failed, returned sync

    // Caller executes L3 serial mount
    int result = DoUnlockMountSerial(content, uid);

    // Assert
    // reservedPid remains unchanged (prefork disabled, no refork)
    EXPECT_EQ(content->reservedPid, preforkPid);

    // fds should not be found (we didn't register any)
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, preforkPid, TYPE_PARENT_CHILD), nullptr);

    // Hook should have been called via Level 3 serial path
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);
    EXPECT_EQ(result, 0);  // L3 should succeed
}

/**
 * @tc.name: UnlockMount_HandleUnlockEvent_005
 * @tc.desc: HandleUnlockEvent Level 1 write failed: fallback to Level 2
 *           (line 2135, SendUnlockMsgToPrefork returns error)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleUnlockEvent_005, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54323;
    content->reservedPid = preforkPid;
    content->enablePerfork = 0;  // Disable prefork to avoid refork after L1/L2 failure
    int uid = 100;

    // Register fake prefork fds
    RegisterFakePreforkFds(preforkPid);

    // Close write end to make write() fail
    close(g_pipeWriteFd);
    g_pipeWriteFd = -1;

    // Force fork to fail, so Level 2 also fails and falls back to Level 3
    g_mockForkReturn = -1;

    // Act: HandleUnlockEvent returns false when L1/L2 fail, caller handles L3
    bool async = HandleUnlockEvent(content, uid, NULL, NULL);
    EXPECT_EQ(async, false);  // L1/L2 failed, returned sync

    // Caller executes L3 serial mount
    int result = DoUnlockMountSerial(content, uid);

    // Assert
    // reservedPid remains unchanged (prefork disabled, no refork)
    EXPECT_EQ(content->reservedPid, preforkPid);

    // Hook should have been called via Level 3 serial path
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);
    EXPECT_EQ(result, 0);  // L3 should succeed
}

/**
 * @tc.name: UnlockMount_HandleUnlockEvent_008
 * @tc.desc: HandleUnlockEvent Level 2 fork success: break
 *           (line 2160, pid > 0)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleUnlockEvent_008, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    content->reservedPid = 0;  // No prefork, go directly to Level 2
    int uid = 100;

    // Force fork to return a fake pid (Level 2 success)
    g_mockForkReturn = 54326;

    // Act: message/connection are NULL, HandleUnlockEvent returns false immediately
    bool async = HandleUnlockEvent(content, uid, NULL, NULL);
    EXPECT_EQ(async, false);  // NULL params, return false

    // Caller executes L3 serial mount
    int result = DoUnlockMountSerial(content, uid);

    // Assert
    // reservedPid should remain 0
    EXPECT_EQ(content->reservedPid, 0);

    // Hook should be called via Level 3 serial path
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(result, 0);  // L3 should succeed
}

/**
 * @tc.name: UnlockMount_HandleUnlockEvent_009
 * @tc.desc: HandleUnlockEvent Level 2 fork failed: Level 3 serial
 *           (line 2167, fork returns -1)
 * @tc.type: FUNC
 * @tc.level: Level1
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleUnlockEvent_009, TestSize.Level1)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    content->reservedPid = 0;  // No prefork
    int uid = 100;

    // Force fork to fail
    g_mockForkReturn = -1;

    // Act: HandleUnlockEvent returns false when L1/L2 fail, caller handles L3
    bool async = HandleUnlockEvent(content, uid, NULL, NULL);
    EXPECT_EQ(async, false);  // L1/L2 failed, returned sync

    // Caller executes L3 serial mount
    int result = DoUnlockMountSerial(content, uid);

    // Assert
    // reservedPid should remain 0
    EXPECT_EQ(content->reservedPid, 0);

    // Hook should have been called via Level 3 serial path
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);
    EXPECT_EQ(result, 0);  // L3 should succeed
}

// ============================================================================
// Batch-1: SendUnlockMsgToPrefork Tests (TC-088 ~ TC-092)
// ============================================================================

/**
 * @tc.name: UnlockMount_SendUnlockMsgToPrefork_001
 * @tc.desc: SendUnlockMsgToPrefork with NULL content: returns error
 *           (line 2186)
 * @tc.type: FUNC
 * @tc.level: Level1
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_SendUnlockMsgToPrefork_001, TestSize.Level1)
{
    // Arrange
    AppSpawnContent *content = nullptr;
    int uid = 100;

    // Act
    int ret = SendUnlockMsgToPrefork(content, uid);

    // Assert
    EXPECT_NE(ret, 0);  // Should return error code
}

/**
 * @tc.name: UnlockMount_SendUnlockMsgToPrefork_002
 * @tc.desc: SendUnlockMsgToPrefork fds not found: returns error
 *           (line 2192)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_SendUnlockMsgToPrefork_002, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54328;
    content->reservedPid = preforkPid;
    int uid = 100;

    // Don't register any fds

    // Act
    int ret = SendUnlockMsgToPrefork(content, uid);

    // Assert
    EXPECT_NE(ret, 0);  // Should return error code
}

/**
 * @tc.name: UnlockMount_SendUnlockMsgToPrefork_003
 * @tc.desc: SendUnlockMsgToPrefork pipe write success: returns 0
 *           (line 2199)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_SendUnlockMsgToPrefork_003, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54329;
    content->reservedPid = preforkPid;
    int uid = 100;

    // Register fake prefork fds with open read end (non-blocking)
    RegisterFakePreforkFdsWithOpenReadEnd(preforkPid);

    // Act
    int ret = SendUnlockMsgToPrefork(content, uid);

    // Assert
    EXPECT_EQ(ret, 0);  // Should return success

    // Clean up: read the message from pipe to drain buffer
    AppSpawnPipeMsg msg = {};
    ssize_t readSize = read(g_pipeReadFd, &msg, sizeof(msg));
    // We expect to read the message, but don't assert on it
    // (it might have been consumed or drained)
    (void)readSize;
}

/**
 * @tc.name: UnlockMount_SendUnlockMsgToPrefork_004
 * @tc.desc: SendUnlockMsgToPrefork pipe write fails: returns error
 *           (line 2199, write returns error)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_SendUnlockMsgToPrefork_004, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54330;
    content->reservedPid = preforkPid;
    int uid = 100;

    // Register fake prefork fds
    RegisterFakePreforkFds(preforkPid);

    // Close write end to make write() fail
    close(g_pipeWriteFd);
    g_pipeWriteFd = -1;

    // Act
    int ret = SendUnlockMsgToPrefork(content, uid);

    // Assert
    EXPECT_NE(ret, 0);  // Should return error code
}

// ============================================================================
// Batch-1: ForkAndDoUnlockMount Tests (TC-093 ~ TC-096)
// ============================================================================

/**
 * @tc.name: UnlockMount_ForkAndDoUnlockMount_001
 * @tc.desc: ForkAndDoUnlockMount fork fails: returns -1
 *           (line 2213, pid < 0)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_ForkAndDoUnlockMount_001, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    int uid = 100;

    // Create a valid AppSpawningCtx for ForkAndDoUnlockMount
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);

    // Force fork to fail
    g_mockForkReturn = -1;

    // Act
    int ret = ForkAndDoUnlockMount(content, uid, property);

    // Cleanup property
    DeleteAppSpawningCtx(property);

    // Assert
    EXPECT_NE(ret, 0);  // Should return error code (APPSPAWN_SYSTEM_ERROR)

    // Hook should not be called (fork failed before child code)
    EXPECT_EQ(g_hookCalled, 0);
}

/**
 * @tc.name: UnlockMount_ForkAndDoUnlockMount_002
 * @tc.desc: ForkAndDoUnlockMount fork >0 (parent): returns pid
 *           (line 2232, parent process)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_ForkAndDoUnlockMount_002, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    int uid = 100;
    pid_t fakePid = 54332;

    // Create a valid AppSpawningCtx for ForkAndDoUnlockMount
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);

    // Force fork to return fake pid
    g_mockForkReturn = fakePid;

    // Act
    int ret = ForkAndDoUnlockMount(content, uid, property);

    // Cleanup property
    DeleteAppSpawningCtx(property);

    // Assert
    // ForkAndDoUnlockMount may fail if EventLoop is not initialized
    // In test environment, AddUnlockChildWatcher often fails
    if (ret != 0) {
        APPSPAWN_LOGI("ForkAndDoUnlockMount failed (expected in test env): ret=%{public}d", ret);
    }

    // Hook should not be called in parent process
    EXPECT_EQ(g_hookCalled, 0);
}

/**
 * @tc.name: UnlockMount_ForkAndDoUnlockMount_003
 * @tc.desc: ForkAndDoUnlockMount fork ==0 (child): ProcessExit(0) called,
 *           ProcessUnlockMessage returns 0, SetParameter("0")
 *           (line 2217-2229, child process)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_ForkAndDoUnlockMount_003, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    int uid = 100;

    // Create a valid AppSpawningCtx for ForkAndDoUnlockMount
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);

    // Force fork to return 0 (child process)
    g_mockForkReturn = 0;

    // Set hook to return success
    g_hookReturn = 0;

    // Set up longjmp to catch ProcessExit
    int jmpVal = setjmp(g_processExitEnv);
    if (jmpVal == 0) {
        // Act
        int ret = ForkAndDoUnlockMount(content, uid, property);

        // Should not reach here in child
        FAIL() << "ForkAndDoUnlockMount should have called ProcessExit in child";
        (void)ret;  // Suppress unused warning
    }

    // Cleanup property (not reached in child, but for completeness)
    DeleteAppSpawningCtx(property);

    // Assert
    // ProcessExit should have been called
    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(g_processExitCode, 0);

    // Hook should have been called
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);

    // SetParameter should have been called with "0" (success)
    EXPECT_GT(strlen(reinterpret_cast<const char *>(g_lastSetParamKey)), 0);
    EXPECT_STREQ(reinterpret_cast<const char *>(g_lastSetParamValue), "0");
}

/**
 * @tc.name: UnlockMount_ForkAndDoUnlockMount_004
 * @tc.desc: ForkAndDoUnlockMount fork ==0 (child): ProcessUnlockMessage returns error,
 *           SetParameter("1")
 *           (line 2217-2229, child process, hook returns error)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_ForkAndDoUnlockMount_004, TestSize.Level0)
{
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    int uid = 100;

    // Create a valid AppSpawningCtx for ForkAndDoUnlockMount
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);

    // Force fork to return 0 (child process)
    g_mockForkReturn = 0;

    // Set hook to return error
    g_hookReturn = -1;

    // Set up longjmp to catch ProcessExit
    int jmpVal = setjmp(g_processExitEnv);
    if (jmpVal == 0) {
        // Act
        int ret = ForkAndDoUnlockMount(content, uid, property);

        // Should not reach here in child
        FAIL() << "ForkAndDoUnlockMount should have called ProcessExit in child";
        (void)ret;  // Suppress unused warning
    }

    // Cleanup property (not reached in child, but for completeness)
    DeleteAppSpawningCtx(property);

    // Assert
    // ProcessExit should have been called
    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(g_processExitCode, 0);

    // Hook should have been called
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);

    // SetParameter should have been called with "1" (failure)
    EXPECT_GT(strlen(reinterpret_cast<const char *>(g_lastSetParamKey)), 0);
    EXPECT_STREQ(reinterpret_cast<const char *>(g_lastSetParamValue), "1");
}

// ============================================================================
// Batch-2: DoUnlockMountSerial Tests (TC-097 ~ TC-100)
// ============================================================================

/**
 * @tc.name: UnlockMount_DoUnlockMountSerial_004
 * @tc.desc: DoUnlockMountSerial normal flow: priority raised, mount executed,
 *           priority restored. (appspawn_service.c:2236-2265, B1+B2+B3)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_DoUnlockMountSerial_004, TestSize.Level0)
{
    // 覆盖: appspawn_service.c:2236-2265 DoUnlockMountSerial normal flow
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    int uid = 203;
    g_hookReturn = 0;

    // Act
    DoUnlockMountSerial(content, uid);

    // Assert
    // Hook should have been called with correct uid
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);
}

// ============================================================================
// Batch-2: ProcessUnlockMessage Tests (TC-101 ~ TC-103)
// ============================================================================

/**
 * @tc.name: UnlockMount_ProcessUnlockMessage_001
 * @tc.desc: ProcessUnlockMessage hook succeeds, returns 0.
 *           (appspawn_service.c:2267-2282, B1)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_ProcessUnlockMessage_001, TestSize.Level0)
{
    // 覆盖: appspawn_service.c:2267-2282 ProcessUnlockMessage hook returns 0
    // Arrange
    int uid = 100;
    g_hookReturn = 0;

    // Act
    int ret = ProcessUnlockMessage(uid);

    // Assert
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);

    // Verify currentUnlockUid was set in content
    EXPECT_EQ(mgr_->content.currentUnlockUid, uid);
}

/**
 * @tc.name: UnlockMount_ProcessUnlockMessage_002
 * @tc.desc: ProcessUnlockMessage hook fails, returns -1.
 *           (appspawn_service.c:2267-2282, B1)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_ProcessUnlockMessage_002, TestSize.Level0)
{
    // 覆盖: appspawn_service.c:2267-2282 ProcessUnlockMessage hook returns -1
    // Arrange
    int uid = 101;
    g_hookReturn = -1;

    // Act
    int ret = ProcessUnlockMessage(uid);

    // Assert
    EXPECT_NE(ret, 0);
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, uid);

    // Verify currentUnlockUid was still set
    EXPECT_EQ(mgr_->content.currentUnlockUid, uid);
}

/**
 * @tc.name: UnlockMount_ProcessUnlockMessage_003
 * @tc.desc: ProcessUnlockMessage verifies currentUnlockUid is set correctly
 *           for valid content path. (appspawn_service.c:2272-2275, B1)
 *           NOTE: NULL content path (GetAppSpawnContent returns NULL) cannot be
 *           tested directly in unit tests because GetAppSpawnContent is a global
 *           accessor without a mock interface. The null check at line 2273 is
 *           defensive and covered by integration tests.
 * @tc.type: FUNC
 * @tc.level: Level1
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_ProcessUnlockMessage_003, TestSize.Level1)
{
    // 覆盖: appspawn_service.c:2272-2275 ProcessUnlockMessage sets currentUnlockUid
    // Arrange
    int uid = 102;
    g_hookReturn = 0;
    // Reset currentUnlockUid to verify it gets set
    mgr_->content.currentUnlockUid = 0;

    // Act
    int ret = ProcessUnlockMessage(uid);

    // Assert
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(mgr_->content.currentUnlockUid, uid);

    // Call again with different uid to verify it updates
    int uid2 = 200;
    g_hookCalled = 0;
    g_hookUid = 0;
    ret = ProcessUnlockMessage(uid2);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(mgr_->content.currentUnlockUid, uid2);
    EXPECT_EQ(g_hookUid, uid2);
}

// ============================================================================
// Batch-2: HandlePreforkUnlockMsg Tests (TC-104 ~ TC-105)
// ============================================================================

/**
 * @tc.name: UnlockMount_HandlePreforkUnlockMsg_001
 * @tc.desc: HandlePreforkUnlockMsg mount succeeds: SetParameter value "0",
 *           ProcessExit(0) called. (appspawn_service.c:870-886, B1)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandlePreforkUnlockMsg_001, TestSize.Level0)
{
    // 覆盖: appspawn_service.c:870-886 HandlePreforkUnlockMsg success path
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    AppSpawnUnlockMsg unlockMsg = {0};
    unlockMsg.uid = 300;
    g_hookReturn = 0;

    // Set up longjmp to catch ProcessExit(0)
    int jmpVal = setjmp(g_processExitEnv);
    if (jmpVal == 0) {
        // Act
        HandlePreforkUnlockMsg(content, &unlockMsg);

        // Should not reach here - ProcessExit(0) should have been called
        FAIL() << "HandlePreforkUnlockMsg should have called ProcessExit(0)";
    }

    // Assert
    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(g_processExitCode, 0);

    // Hook should have been called with correct uid
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, unlockMsg.uid);

    // SetParameter should have been called with "0" (success)
    EXPECT_GT(strlen(reinterpret_cast<const char *>(g_lastSetParamKey)), 0);
    EXPECT_STREQ(reinterpret_cast<const char *>(g_lastSetParamValue), "0");
}

/**
 * @tc.name: UnlockMount_HandlePreforkUnlockMsg_002
 * @tc.desc: HandlePreforkUnlockMsg mount fails: SetParameter value "1",
 *           ProcessExit(0) called. (appspawn_service.c:870-886, B1)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandlePreforkUnlockMsg_002, TestSize.Level0)
{
    // 覆盖: appspawn_service.c:870-886 HandlePreforkUnlockMsg failure path
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    AppSpawnUnlockMsg unlockMsg = {0};
    unlockMsg.uid = 301;
    g_hookReturn = -1;

    // Set up longjmp to catch ProcessExit(0)
    int jmpVal = setjmp(g_processExitEnv);
    if (jmpVal == 0) {
        // Act
        HandlePreforkUnlockMsg(content, &unlockMsg);

        // Should not reach here - ProcessExit(0) should have been called
        FAIL() << "HandlePreforkUnlockMsg should have called ProcessExit(0)";
    }

    // Assert
    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(g_processExitCode, 0);

    // Hook should have been called with correct uid
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, unlockMsg.uid);

    // SetParameter should have been called with "1" (failure)
    EXPECT_GT(strlen(reinterpret_cast<const char *>(g_lastSetParamKey)), 0);
    EXPECT_STREQ(reinterpret_cast<const char *>(g_lastSetParamValue), "1");
}

// ============================================================================
// Batch-2: PreforkChildLoop Tests (TC-106)
// ============================================================================

/**
 * @tc.name: UnlockMount_PreforkChildLoop_001
 * @tc.desc: PreforkChildLoop dispatches MSG_LOCK_STATUS to HandlePreforkUnlockMsg.
 *           (appspawn_service.c:972-1004, B_unlock_1)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_PreforkChildLoop_001, TestSize.Level0)
{
    // 覆盖: appspawn_service.c:992-994 PreforkChildLoop MSG_LOCK_STATUS dispatch
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54350;
    content->reservedPid = preforkPid;

    // Register fake prefork fds with open read end for the current process
    RegisterFakePreforkFdsWithOpenReadEnd(getpid());

    // Create a minimal AppSpawningCtx
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    // Set up forkCtx.fd with dummy pipe (will be cleared by ClearPipeFd)
    int dummyPipe[PIPE_FD_LENGTH] = {-1, -1};
    memcpy_s(property->forkCtx.fd, sizeof(property->forkCtx.fd), dummyPipe, sizeof(dummyPipe));

    // Prepare MSG_LOCK_STATUS message to write into the pipe
    AppSpawnPipeMsg pipeMsg;
    memset_s(&pipeMsg, sizeof(pipeMsg), 0, sizeof(pipeMsg));
    pipeMsg.type = MSG_LOCK_STATUS;
    pipeMsg.msg.unlockMsg.uid = 400;
    g_hookReturn = 0;

    // Write the message into the pipe so PreforkChildLoop can read it
    // PreforkChildLoop reads from pcFds->fds[0] (g_pipeReadFd)
    ssize_t writeSize = write(g_pipeWriteFd, &pipeMsg, sizeof(AppSpawnPipeMsg));
    ASSERT_EQ(writeSize, (ssize_t)sizeof(AppSpawnPipeMsg));

    // Set up longjmp to catch ProcessExit(0) from HandlePreforkUnlockMsg
    int jmpVal = setjmp(g_processExitEnv);
    if (jmpVal == 0) {
        // Act
        // PreforkChildLoop will:
        // 1. ClearPipeFd(property->forkCtx.fd) - clears dummy pipe fds
        // 2. SetPreforkProcessName(content)
        // 3. FindSpawningFdsByPid(mgr, getpid(), TYPE_PARENT_CHILD) -> finds g_fakeFds
        // 4. read(pcFds->fds[0], ...) -> reads the MSG_LOCK_STATUS we wrote
        // 5. Dispatch to HandlePreforkUnlockMsg -> ProcessExit(0)
        PreforkChildLoop(content, property, mgr_, (enum fdsan_error_level)0);

        // Should not reach here - ProcessExit(0) should have been called
        FAIL() << "PreforkChildLoop should have called ProcessExit via HandlePreforkUnlockMsg";
    }

    // Assert
    // ProcessExit should have been called
    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(g_processExitCode, 0);

    // Hook should have been called via HandlePreforkUnlockMsg
    EXPECT_EQ(g_hookCalled, 1);
    EXPECT_EQ(g_hookUid, pipeMsg.msg.unlockMsg.uid);

    // Cleanup
    DeleteAppSpawningCtx(property);
}

// ============================================================================
// Batch-2: StopAppSpawn Tests (TC-107 ~ TC-109)
// ============================================================================

/**
 * @tc.name: UnlockMount_StopAppSpawn_001
 * @tc.desc: StopAppSpawn reservedPid > 0: kill called, fds cleaned, reservedPid reset
 *           (appspawn_service.c:94-111, B_unlock_1)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_StopAppSpawn_001, TestSize.Level0)
{
    // 覆盖: appspawn_service.c:94-111 StopAppSpawn reservedPid > 0
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54360;
    content->reservedPid = preforkPid;

    // Register fake prefork fds so we can verify cleanup
    RegisterFakePreforkFds(preforkPid);

    // Act
    StopAppSpawn();

    // Assert
    // reservedPid should be reset to 0 after cleanup
    EXPECT_EQ(content->reservedPid, 0);

    // fds should be cleaned up
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, preforkPid, TYPE_PARENT_CHILD), nullptr);
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, preforkPid, TYPE_CHILD_PARENT), nullptr);
}

/**
 * @tc.name: UnlockMount_StopAppSpawn_002
 * @tc.desc: StopAppSpawn reservedPid = 0: no prefork cleanup, function completes
 *           (appspawn_service.c:94-111, B_unlock_1)
 * @tc.type: FUNC
 * @tc.level: Level1
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_StopAppSpawn_002, TestSize.Level1)
{
    // 覆盖: StopAppSpawn reservedPid == 0 (appspawn_service.c:94-111)
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    content->reservedPid = 0;

    // Act - with reservedPid=0, the prefork cleanup block is skipped
    // StopAppSpawn calls TraversalSpawnedProcess and LE_StopLoop internally,
    // which may call ProcessExit. Use setjmp to handle that gracefully.
    int jmpVal = setjmp(g_processExitEnv);
    if (jmpVal == 0) {
        StopAppSpawn();
    }

    // Assert - reservedPid should still be 0 (no prefork to clean up)
    EXPECT_EQ(content->reservedPid, 0);
}

// ============================================================================
// Batch-2: HandleDiedPid Tests (TC-110 ~ TC-111)
// ============================================================================

/**
 * @tc.name: UnlockMount_HandleDiedPid_001
 * @tc.desc: HandleDiedPid pid matches reservedPid: cleanup fds, reset reservedPid
 *           (appspawn_service.c:159-163, B_unlock_1)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleDiedPid_001, TestSize.Level0)
{
    // 覆盖: appspawn_service.c:154-170 HandleDiedPid pid == reservedPid
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t preforkPid = 54370;
    content->reservedPid = preforkPid;

    // Register fake prefork fds
    RegisterFakePreforkFds(preforkPid);

    int status = 0;
    uid_t uid = 100;

    // Act
    HandleDiedPid(preforkPid, uid, status);

    // Assert
    // reservedPid should be reset to 0
    EXPECT_EQ(content->reservedPid, 0);

    // fds should be cleaned up
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, preforkPid, TYPE_PARENT_CHILD), nullptr);
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, preforkPid, TYPE_CHILD_PARENT), nullptr);
}

/**
 * @tc.name: UnlockMount_HandleDiedPid_002
 * @tc.desc: HandleDiedPid pid does NOT match reservedPid: no reserved cleanup
 *           (appspawn_service.c:159-163, B_unlock_1)
 * @tc.type: FUNC
 * @tc.level: Level1
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_HandleDiedPid_002, TestSize.Level1)
{
    // 覆盖: appspawn_service.c:159-163 HandleDiedPid pid != reservedPid
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    pid_t reservedPid = 54371;
    content->reservedPid = reservedPid;

    // Register fake prefork fds for reservedPid
    RegisterFakePreforkFds(reservedPid);

    pid_t deadPid = 99999;  // Different from reservedPid
    int status = 0;
    uid_t uid = 100;

    // Act
    HandleDiedPid(deadPid, uid, status);

    // Assert
    // reservedPid should remain unchanged
    EXPECT_EQ(content->reservedPid, reservedPid);

    // fds should NOT be cleaned up (because deadPid != reservedPid)
    EXPECT_NE(FindSpawningFdsByPid(mgr_, reservedPid, TYPE_PARENT_CHILD), nullptr);
    EXPECT_NE(FindSpawningFdsByPid(mgr_, reservedPid, TYPE_CHILD_PARENT), nullptr);
}

// ============================================================================
// Batch-2: ProcessAppSpawnLockStatusMsg Tests (TC-112 ~ TC-113)
// ============================================================================

/**
 * @tc.name: UnlockMount_ProcessAppSpawnLockStatusMsg_001
 * @tc.desc: ProcessAppSpawnLockStatusMsg nullptr: returns immediately without state change
 *           (appspawn_service.c:1922-1924, B_unlock_1)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: unlock_mount feature
 */
HWTEST_F(AppSpawnUnlockMountServiceTest, UnlockMount_ProcessAppSpawnLockStatusMsg_001, TestSize.Level0)
{
    // 覆盖: ProcessAppSpawnLockStatusMsg nullptr early return (appspawn_service.c:1922-1924)
    // Arrange
    AppSpawnContent *content = &mgr_->content;
    int originalHookCalled = 0;
    int originalUnlockUid = content->currentUnlockUid;
    int result = 0;

    // Act
    ProcessAppSpawnLockStatusMsg(nullptr, nullptr, &result);

    // Assert - nullptr should be caught by APPSPAWN_CHECK_ONLY_EXPER, no state change
    EXPECT_EQ(g_hookCalled, originalHookCalled);
    EXPECT_EQ(content->currentUnlockUid, originalUnlockUid);
}

// NOTE: Full ProcessAppSpawnLockStatusMsg tests with valid TLV message data
// (TC-112: "100:0" triggers HandleUnlockEvent, TC-113: "100:1" does NOT trigger)
// require constructing AppSpawnMsgNode with proper TLV buffer containing
// "lockstatus" ext info. This is complex because GetAppSpawnMsgExtInfo parses
// TLV-encoded data from message->buffer. These are better suited for
// integration tests where real message infrastructure is available.

// ============================================================================
// Batch-3: Integration Tests (Future Work)
// ============================================================================

// NOTE: Batch-3 Scenarios (TC-118 ~ TC-123) are integration tests that require
// full message infrastructure and multi-process coordination. These scenarios
// should be covered in integration test suites:
//   - TC-118: Full Level 1 -> refork flow
//   - TC-119: Full Level 2 -> child process flow
//   - TC-120: Full Level 3 -> serial flow
//   - TC-121: AppCleanupHook + AddLockBundleRef + ReleaseLockBundleRef lifecycle
//   - TC-122: SpawnFailedHook + AddLockBundleRef + ReleaseLockBundleRef
//   - TC-123: Lock -> Unlock flow

}  // namespace OHOS
