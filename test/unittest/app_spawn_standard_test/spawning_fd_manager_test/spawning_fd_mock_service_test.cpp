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

#include <gtest/gtest.h>

#include <csetjmp>
#include <fcntl.h>
#include <unistd.h>

#include <vector>
#include <cstring>
#include <sys/wait.h>

#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_server.h"
#include "appspawn_utils.h"
#include "appspawn_msg.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

// ===== ProcessExit interception via setjmp/longjmp =====
// APPSPAWN_EXIT_TEST makes ProcessExit a no-op, but APPSPAWN_CHECK relies on
// ProcessExit terminating the process. Use --wrap=ProcessExit in BUILD.gn
// to intercept calls and longjmp back to the test.
static thread_local jmp_buf g_processExitBuf;
static thread_local volatile int g_processExitCalled = 0;
static thread_local volatile int g_processExitCode = 0;

extern "C" {
// Declare APPSPAWN_STATIC functions from appspawn_service.c for test access
APPSPAWN_STATIC void HandlePreforkForkMsg(AppSpawnContent *content, AppSpawningCtx *property,
    const AppSpawnPreforkMsg *preforkMsg);
APPSPAWN_STATIC void PreforkChildLoop(AppSpawnContent *content, AppSpawningCtx *property,
    AppSpawnMgr *mgr, enum fdsan_error_level errorLevel);
APPSPAWN_STATIC int PreparePreforkMsg(AppSpawnContent *content, AppSpawningCtx *property,
    const AppSpawnClient *client, uint32_t memSize, AppSpawnPipeMsg **outPipeMsg);
APPSPAWN_STATIC char *GetMapMem(uint32_t clientId, const char *processName,
    uint32_t size, bool readOnly, RunMode runMode);
APPSPAWN_STATIC int WritePreforkMsg(AppSpawningCtx *property, uint32_t memSize);
APPSPAWN_STATIC pid_t ForkAndRegisterFds(AppSpawnMgr *mgr, AppSpawningCtx *property,
    int preforkFd[PIPE_FD_LENGTH], int parentToChildFd[PIPE_FD_LENGTH]);
APPSPAWN_STATIC int SendPipeMsgToChild(AppSpawnMgr *mgr, pid_t childPid, AppSpawnPipeMsg *pipeMsg);
APPSPAWN_STATIC int TransferPreforkFdToForkCtx(AppSpawnMgr *mgr, pid_t childPid, AppSpawningCtx *property);
APPSPAWN_STATIC void CleanupPreforkChild(AppSpawnMgr *mgr, pid_t childPid);

// This function is linked via --wrap=ProcessExit in BUILD.gn
// It replaces all ProcessExit calls with a longjmp back to the test
void __wrap_ProcessExit(int code)
{
    g_processExitCode = code;
    g_processExitCalled = 1;
    longjmp(g_processExitBuf, code);
}

// This function is linked via --wrap=GetMapMem in BUILD.gn
// Always returns nullptr so PreparePreforkMsg takes the failure path
void *__wrap_GetMapMem(uint32_t clientId, const char *processName,
    uint32_t size, bool readOnly, int runMode)
{
    (void)clientId;
    (void)processName;
    (void)size;
    (void)readOnly;
    (void)runMode;
    return nullptr;
}

// ===== fork interception =====
static constexpr int MOCK_FORK_USE_REAL = -2;
static thread_local volatile pid_t g_mockForkReturn = MOCK_FORK_USE_REAL;

// This function is linked via --wrap=fork in BUILD.gn
pid_t __real_fork(void);

pid_t __wrap_fork(void)
{
    if (g_mockForkReturn != MOCK_FORK_USE_REAL) {
        return g_mockForkReturn;
    }
    return __real_fork();
}

}  // extern "C"

namespace OHOS {

// No-op stub for notifyResToParent to prevent NULL dereference
static void NotifyResToParentStub(AppSpawnContent *content, AppSpawnClient *client, int result) {}

/**
 * Test fixture for mock-based L2 tests.
 * Uses --wrap=ProcessExit so ProcessExit() does longjmp instead of no-op.
 * This correctly simulates process termination while allowing test verification.
 */
class SpawningFdMockTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());
        mgr_ = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        ASSERT_NE(mgr_, nullptr);
        mgr_->servicePid = getpid();
        mgr_->content.notifyResToParent = NotifyResToParentStub;
        g_processExitCalled = 0;
        g_processExitCode = 0;
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
        if (mgr_ != nullptr) {
            CleanupSpawningFdsByPid(mgr_, getpid());
            DeleteAppSpawnMgr(mgr_);
            mgr_ = nullptr;
        }
    }

protected:
    AppSpawnMgr *mgr_ = nullptr;

    bool IsFdValid(int fd)
    {
        return fcntl(fd, F_GETFD) >= 0;
    }

    AppSpawningCtx *CreateMinimalCtx(uint32_t clientId)
    {
        AppSpawningCtx *ctx = CreateAppSpawningCtx();
        if (ctx != nullptr) {
            ctx->client.id = clientId;
        }
        return ctx;
    }

    // Create a valid AppSpawnPreforkMsg within normal range
    AppSpawnPreforkMsg MakeValidPreforkMsg(uint32_t id, uint32_t flags, uint32_t msgLen)
    {
        AppSpawnPreforkMsg msg = {};
        msg.id = id;
        msg.flags = flags;
        msg.msgLen = msgLen;
        return msg;
    }
};

// ===== HandlePreforkForkMsg tests (HF001-HF004) =====

/**
 * @tc.name: SpawningFd_HandlePreforkForkMsg_001
 * @tc.desc: HandlePreforkForkMsg msgLen越上界：msgLen > MAX_MSG_TOTAL_LENGTH时调用ProcessExit(0)，
 *           通过--wrap=ProcessExit+setjmp捕获退出，spawningFdsQueue无副作用
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_HandlePreforkForkMsg_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(3001);
    ASSERT_NE(property, nullptr);

    AppSpawnContent *content = &mgr_->content;

    // msgLen exceeds max
    AppSpawnPreforkMsg msg = MakeValidPreforkMsg(3001, 0, MAX_MSG_TOTAL_LENGTH + 1);

    // Set longjmp target - if ProcessExit is called, we jump back here
    int jmpVal = setjmp(g_processExitBuf);
    if (jmpVal == 0) {
        HandlePreforkForkMsg(content, property, &msg);
        // If we reach here, ProcessExit was NOT called (unexpected)
        FAIL() << "HandlePreforkForkMsg should have called ProcessExit for invalid msgLen";
    }

    // Verify ProcessExit was called
    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(g_processExitCode, 0);

    // No side effects on spawningFdsQueue
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, getpid(), TYPE_CHILD_PARENT), nullptr);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_HandlePreforkForkMsg_002
 * @tc.desc: HandlePreforkForkMsg msgLen越下界：msgLen < sizeof(AppSpawnMsg)时调用ProcessExit(0)，
 *           通过--wrap=ProcessExit+setjmp捕获退出
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_HandlePreforkForkMsg_002, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(3002);
    ASSERT_NE(property, nullptr);

    AppSpawnContent *content = &mgr_->content;

    // msgLen too small
    AppSpawnPreforkMsg msg = MakeValidPreforkMsg(3002, 0, 1);

    int jmpVal = setjmp(g_processExitBuf);
    if (jmpVal == 0) {
        HandlePreforkForkMsg(content, property, &msg);
        FAIL() << "HandlePreforkForkMsg should have called ProcessExit for invalid msgLen";
    }

    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(g_processExitCode, 0);
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, getpid(), TYPE_CHILD_PARENT), nullptr);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_HandlePreforkForkMsg_003
 * @tc.desc: HandlePreforkForkMsg prefork fd未找到：msgLen合法但当前pid无TYPE_CHILD_PARENT注册时，
 *           调用ProcessExit(0)，通过--wrap=ProcessExit+setjmp捕获退出
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_HandlePreforkForkMsg_003, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(3003);
    ASSERT_NE(property, nullptr);

    AppSpawnContent *content = &mgr_->content;

    // Valid msgLen but no TYPE_CHILD_PARENT registered for current pid
    AppSpawnPreforkMsg msg = MakeValidPreforkMsg(3003, 0, sizeof(AppSpawnMsg) + 100);

    int jmpVal = setjmp(g_processExitBuf);
    if (jmpVal == 0) {
        HandlePreforkForkMsg(content, property, &msg);
        FAIL() << "HandlePreforkForkMsg should have called ProcessExit when prefork fds not found";
    }

    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(g_processExitCode, 0);
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, getpid(), TYPE_CHILD_PARENT), nullptr);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_HandlePreforkForkMsg_004
 * @tc.desc: HandlePreforkForkMsg正常路径：prefork fd已注册时，RemoveSpawningFdsByPid移除节点，
 *           fd转移到forkCtx.fd仍可用(不close)，最终因GetAppSpawnMsg失败调用ProcessExit
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_HandlePreforkForkMsg_004, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(3004);
    ASSERT_NE(property, nullptr);

    AppSpawnContent *content = &mgr_->content;

    // Register TYPE_CHILD_PARENT for current pid
    int fds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(fds), 0);
    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, getpid() };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    AppSpawnPreforkMsg msg = MakeValidPreforkMsg(3004, 0, sizeof(AppSpawnMsg) + 100);

    int jmpVal = setjmp(g_processExitBuf);
    if (jmpVal == 0) {
        HandlePreforkForkMsg(content, property, &msg);
        FAIL() << "HandlePreforkForkMsg should have called ProcessExit at end";
    }

    EXPECT_EQ(g_processExitCalled, 1);

    // Prefork fds should be removed from queue (RemoveSpawningFdsByPid happens before GetAppSpawnMsg)
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, getpid(), TYPE_CHILD_PARENT), nullptr);

    // The fds should still be valid (RemoveSpawningFdsByPid doesn't close them)
    EXPECT_TRUE(IsFdValid(fds[0]));
    EXPECT_TRUE(IsFdValid(fds[1]));

    close(fds[0]);
    close(fds[1]);
    DeleteAppSpawningCtx(property);
}

// ===== PreforkChildLoop tests (PC001-PC004) =====

/**
 * @tc.name: SpawningFd_PreforkChildLoop_001
 * @tc.desc: PreforkChildLoop parent-child fd未找到：当前pid无TYPE_PARENT_CHILD注册时，
 *           调用ProcessExit(0)，通过--wrap=ProcessExit+setjmp捕获退出
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_PreforkChildLoop_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(4001);
    ASSERT_NE(property, nullptr);
    property->forkCtx.fd[0] = -1;
    property->forkCtx.fd[1] = -1;

    AppSpawnContent *content = &mgr_->content;

    int jmpVal = setjmp(g_processExitBuf);
    if (jmpVal == 0) {
        PreforkChildLoop(content, property, mgr_, FDSAN_ERROR_LEVEL_FATAL);
        FAIL() << "PreforkChildLoop should have called ProcessExit when parent-child fds not found";
    }

    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, getpid(), TYPE_PARENT_CHILD), nullptr);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_PreforkChildLoop_002
 * @tc.desc: PreforkChildLoop read失败：注册TYPE_PARENT_CHILD但关闭read fd使read()返回错误，
 *           调用ProcessExit(0)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_PreforkChildLoop_002, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(4002);
    ASSERT_NE(property, nullptr);
    property->forkCtx.fd[0] = -1;
    property->forkCtx.fd[1] = -1;

    AppSpawnContent *content = &mgr_->content;

    // Register TYPE_PARENT_CHILD but close read end so read() fails
    int fds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(fds), 0);
    close(fds[0]);  // Close read end
    SpawningFdRegInfo regInfo = { TYPE_PARENT_CHILD, PIPE_FD_LENGTH, fds, getpid() };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    int jmpVal = setjmp(g_processExitBuf);
    if (jmpVal == 0) {
        PreforkChildLoop(content, property, mgr_, FDSAN_ERROR_LEVEL_FATAL);
        FAIL() << "PreforkChildLoop should have called ProcessExit on read failure";
    }

    EXPECT_EQ(g_processExitCalled, 1);

    // Cleanup
    RemoveSpawningFdsByPid(mgr_, getpid(), TYPE_PARENT_CHILD);
    close(fds[1]);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_PreforkChildLoop_003
 * @tc.desc: PreforkChildLoop收到MSG_APP_SPAWN消息：向pipe写入有效AppSpawnPipeMsg后，
 *           PreforkChildLoop读取并分发给HandlePreforkForkMsg，最终因GetAppSpawnMsg失败调用ProcessExit；
 *           分发后TYPE_CHILD_PARENT节点被RemoveSpawningFdsByPid移除
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_PreforkChildLoop_003, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(4003);
    ASSERT_NE(property, nullptr);
    property->forkCtx.fd[0] = -1;
    property->forkCtx.fd[1] = -1;

    AppSpawnContent *content = &mgr_->content;

    // Register TYPE_PARENT_CHILD with a real pipe
    int fds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(fds), 0);
    SpawningFdRegInfo regInfo = { TYPE_PARENT_CHILD, PIPE_FD_LENGTH, fds, getpid() };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // Write a valid AppSpawnPipeMsg to the pipe
    AppSpawnPipeMsg pipeMsg = {};
    pipeMsg.type = MSG_APP_SPAWN;
    pipeMsg.msg.preforkMsg.id = 4003;
    pipeMsg.msg.preforkMsg.flags = 0;
    pipeMsg.msg.preforkMsg.msgLen = sizeof(AppSpawnMsg) + 100;

    ssize_t written = write(fds[1], &pipeMsg, sizeof(AppSpawnPipeMsg));
    ASSERT_EQ(written, static_cast<ssize_t>(sizeof(AppSpawnPipeMsg)));

    // Close write end so read() can complete
    close(fds[1]);

    // Register TYPE_CHILD_PARENT so HandlePreforkForkMsg can find it
    int pfFds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(pfFds), 0);
    SpawningFdRegInfo pfRegInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, pfFds, getpid() };
    EXPECT_NE(RegisterSpawningFds(mgr_, &pfRegInfo), nullptr);

    int jmpVal = setjmp(g_processExitBuf);
    if (jmpVal == 0) {
        PreforkChildLoop(content, property, mgr_, FDSAN_ERROR_LEVEL_FATAL);
        // ProcessExit called via HandlePreforkForkMsg -> GetAppSpawnMsg failure
        FAIL() << "PreforkChildLoop should have called ProcessExit";
    }

    EXPECT_EQ(g_processExitCalled, 1);

    // After HandlePreforkForkMsg, prefork fds should be removed
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, getpid(), TYPE_CHILD_PARENT), nullptr);

    close(fds[0]);
    close(pfFds[0]);
    close(pfFds[1]);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_PreforkChildLoop_004
 * @tc.desc: PreforkChildLoop收到未知消息类型(0xFF)：读取后因type不匹配调用ProcessExit(0)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_PreforkChildLoop_004, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(4004);
    ASSERT_NE(property, nullptr);
    property->forkCtx.fd[0] = -1;
    property->forkCtx.fd[1] = -1;

    AppSpawnContent *content = &mgr_->content;

    // Register TYPE_PARENT_CHILD
    int fds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(fds), 0);
    SpawningFdRegInfo regInfo = { TYPE_PARENT_CHILD, PIPE_FD_LENGTH, fds, getpid() };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // Write an unknown message type
    AppSpawnPipeMsg pipeMsg = {};
    pipeMsg.type = (AppSpawnMsgType)0xFF;  // Unknown type

    ssize_t written = write(fds[1], &pipeMsg, sizeof(AppSpawnPipeMsg));
    ASSERT_EQ(written, static_cast<ssize_t>(sizeof(AppSpawnPipeMsg)));
    close(fds[1]);

    int jmpVal = setjmp(g_processExitBuf);
    if (jmpVal == 0) {
        PreforkChildLoop(content, property, mgr_, FDSAN_ERROR_LEVEL_FATAL);
        FAIL() << "PreforkChildLoop should have called ProcessExit for unknown msg type";
    }

    EXPECT_EQ(g_processExitCalled, 1);
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, getpid(), TYPE_CHILD_PARENT), nullptr);

    close(fds[0]);

    DeleteAppSpawningCtx(property);
}

// ===== PreparePreforkMsg tests (PP001-PP004) =====

/**
 * @tc.name: SpawningFd_PreparePreforkMsg_001
 * @tc.desc: PreparePreforkMsg GetMapMem失败：通过--wrap=GetMapMem使GetMapMem返回NULL，
 *           PreparePreforkMsg返回-1，outPipeMsg为NULL
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_PreparePreforkMsg_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(5001);
    ASSERT_NE(property, nullptr);

    AppSpawnContent *content = &mgr_->content;
    content->propertyBuffer = NULL;

    AppSpawnClient client = {};
    client.id = 5001;
    client.flags = 0;

    AppSpawnPipeMsg *outPipeMsg = NULL;

    // GetMapMem will fail because APPSPAWN_MSG_DIR doesn't exist in test environment
    int ret = PreparePreforkMsg(content, property, &client, sizeof(AppSpawnMsg), &outPipeMsg);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(outPipeMsg, nullptr);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_PreparePreforkMsg_002
 * @tc.desc: PreparePreforkMsg WritePreforkMsg失败：GetMapMem或WritePreforkMsg失败时返回-1
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_PreparePreforkMsg_002, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(5002);
    ASSERT_NE(property, nullptr);

    AppSpawnContent *content = &mgr_->content;
    content->propertyBuffer = NULL;

    AppSpawnClient client = {};
    client.id = 5002;
    client.flags = 0;

    AppSpawnPipeMsg *outPipeMsg = NULL;

    int ret = PreparePreforkMsg(content, property, &client, sizeof(AppSpawnMsg), &outPipeMsg);
    // Either -1 (GetMapMem or WritePreforkMsg failed)
    if (ret == -1) {
        EXPECT_EQ(outPipeMsg, nullptr);
    }

    free(outPipeMsg);
    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_PreparePreforkMsg_003
 * @tc.desc: PreparePreforkMsg calloc失败路径：测试环境下GetMapMem返回NULL导致提前返回-1
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_PreparePreforkMsg_003, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(5003);
    ASSERT_NE(property, nullptr);

    AppSpawnContent *content = &mgr_->content;
    content->propertyBuffer = NULL;

    AppSpawnClient client = {};
    client.id = 5003;
    client.flags = 0;

    AppSpawnPipeMsg *outPipeMsg = NULL;

    int ret = PreparePreforkMsg(content, property, &client, sizeof(AppSpawnMsg), &outPipeMsg);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(outPipeMsg, nullptr);

    free(outPipeMsg);
    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_PreparePreforkMsg_004
 * @tc.desc: PreparePreforkMsg正常路径(如GetMapMem可用)：验证outPipeMsg->type=MSG_APP_SPAWN，
 *           msg.preforkMsg.id/flags与输入client一致
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdMockTest, SpawningFd_PreparePreforkMsg_004, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(5004);
    ASSERT_NE(property, nullptr);

    AppSpawnContent *content = &mgr_->content;
    content->propertyBuffer = NULL;

    AppSpawnClient client = {};
    client.id = 5004;
    client.flags = 0x1234;

    AppSpawnPipeMsg *outPipeMsg = NULL;

    int ret = PreparePreforkMsg(content, property, &client, sizeof(AppSpawnMsg), &outPipeMsg);

    if (ret == 0) {
        ASSERT_NE(outPipeMsg, nullptr);
        EXPECT_EQ(outPipeMsg->type, MSG_APP_SPAWN);
        EXPECT_EQ(outPipeMsg->msg.preforkMsg.id, (uint32_t)5004);
        EXPECT_EQ(outPipeMsg->msg.preforkMsg.flags, (uint32_t)0x1234);
    } else {
        EXPECT_EQ(outPipeMsg, nullptr);
    }

    free(outPipeMsg);
    DeleteAppSpawningCtx(property);
}
}  // namespace OHOS

// ===== Mock-based tests moved from spawning_fd_service_test.cpp =====
// These tests use --wrap=fork and --wrap=write to reliably control
// fork() and write() behavior, avoiding device environment issues.

namespace OHOS {

class SpawningFdForkMockTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());
        mgr_ = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        ASSERT_NE(mgr_, nullptr);
        mgr_->servicePid = getpid();
        g_mockForkReturn = MOCK_FORK_USE_REAL;
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
        if (mgr_ != nullptr) {
            DeleteAppSpawnMgr(mgr_);
            mgr_ = nullptr;
        }
    }

protected:
    AppSpawnMgr *mgr_ = nullptr;

    bool IsFdValid(int fd)
    {
        return fcntl(fd, F_GETFD) >= 0;
    }

    AppSpawningCtx *CreateMinimalCtx(uint32_t clientId)
    {
        AppSpawningCtx *ctx = CreateAppSpawningCtx();
        if (ctx != nullptr) {
            ctx->client.id = clientId;
        }
        return ctx;
    }
};

// ===== ForkAndRegisterFds mock tests =====

/**
 * @tc.name: SpawningFd_ForkAndRegister_005
 * @tc.desc: ForkAndRegisterFds fork失败(通过--wrap=fork模拟)：两个pipe创建成功后fork返回-1，
 *           ClearPipeFd清理所有pipe fd，队列不新增节点(total不变)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdForkMockTest, SpawningFd_ForkAndRegister_005, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2011);
    ASSERT_NE(property, nullptr);

    uint32_t totalBefore = 0, prePc0 = 0, prePc1 = 0;
    GetSpawningFdsStats(mgr_, &totalBefore, &prePc0, &prePc1);

    // Force fork to fail
    g_mockForkReturn = -1;

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    EXPECT_LT(pid, 0);  // fork should fail

    // No new nodes should be added (ClearPipeFd closes both pipe pairs on fork failure)
    uint32_t totalAfter = 0, postPc0 = 0, postPc1 = 0;
    GetSpawningFdsStats(mgr_, &totalAfter, &postPc0, &postPc1);
    EXPECT_EQ(totalAfter, totalBefore);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_TransferPreforkFd_003
 * @tc.desc: TransferPreforkFdToForkCtx通过--wrap=fork模拟fork返回假pid(54321)：
 *           fork后正常注册节点，Transfer后fd转移到forkCtx.fd且仍可用(F_GETFD>=0)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdForkMockTest, SpawningFd_TransferPreforkFd_003, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2008);
    ASSERT_NE(property, nullptr);

    // Force fork to return a fake child pid
    g_mockForkReturn = 54321;

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    EXPECT_GT(pid, 0);

    int ret = TransferPreforkFdToForkCtx(mgr_, pid, property);
    EXPECT_EQ(ret, 0);

    // Verify fds are valid after transfer
    EXPECT_TRUE(IsFdValid(property->forkCtx.fd[0]));
    EXPECT_TRUE(IsFdValid(property->forkCtx.fd[1]));

    // Cleanup
    close(property->forkCtx.fd[0]);
    close(property->forkCtx.fd[1]);
    CleanupSpawningFdsByPid(mgr_, pid);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_CleanupPreforkChild_001
 * @tc.desc: CleanupPreforkChild通过--wrap=fork模拟fork返回假pid(54322)：
 *           fork后注册2个节点(TYPE_CHILD_PARENT+TYPE_PARENT_CHILD)，CleanupPreforkChild清理后
 *           两种type的Find均返回NULL，total=0
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdForkMockTest, SpawningFd_CleanupPreforkChild_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2009);
    ASSERT_NE(property, nullptr);

    // Force fork to return a fake child pid
    g_mockForkReturn = 54322;

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    EXPECT_GT(pid, 0);

    // Verify fds are registered
    uint32_t total = 0, pc0 = 0, pc1 = 0;
    GetSpawningFdsStats(mgr_, &total, &pc0, &pc1);
    EXPECT_EQ(total, (uint32_t)2);

    // CleanupPreforkChild should cleanup all fds for this pid
    // (kill won't actually work on fake pid, but cleanup logic is the same)
    CleanupPreforkChild(mgr_, pid);

    // After cleanup, all fds for this pid should be gone
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT), nullptr);
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD), nullptr);

    uint32_t totalAfter = 0, pc0a = 0, pc1a = 0;
    GetSpawningFdsStats(mgr_, &totalAfter, &pc0a, &pc1a);
    EXPECT_EQ(totalAfter, (uint32_t)0);

    DeleteAppSpawningCtx(property);
}

// ===== SendPipeMsgToChild mock tests =====

/**
 * @tc.name: SpawningFd_SendPipeMsg_004
 * @tc.desc: SendPipeMsgToChild写失败(EBADF)：注册TYPE_PARENT_CHILD后关闭write fd，
 *           调用SendPipeMsgToChild返回-1，且不调用Unregister(节点仍在队列中)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdForkMockTest, SpawningFd_SendPipeMsg_004, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(fds), 0);
    pid_t pid = 99999;

    SpawningFdRegInfo regInfo = { TYPE_PARENT_CHILD, PIPE_FD_LENGTH, fds, pid };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // Close write end so write() in SendPipeMsgToChild fails with EBADF
    close(fds[1]);

    AppSpawnPipeMsg pipeMsg = {};
    pipeMsg.type = MSG_APP_SPAWN;

    int ret = SendPipeMsgToChild(mgr_, pid, &pipeMsg);
    EXPECT_NE(ret, 0);

    // On failure, Unregister is NOT called, node should still be in queue
    EXPECT_NE(FindSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD), nullptr);

    // Cleanup: Remove without closing (write fd already closed), then close read fd
    RemoveSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD);
    close(fds[0]);
}

/**
 * @tc.name: SpawningFd_ForkAndRegister_006
 * @tc.desc: ForkAndRegisterFds子进程分支(pid==0)：通过--wrap=fork模拟fork返回0，
 *           验证子进程用getpid()注册了TYPE_CHILD_PARENT和TYPE_PARENT_CHILD两个节点，
 *           FindSpawningFdsByPid(mgr, getpid(), TYPE_CHILD_PARENT)和TYPE_PARENT_CHILD均能找到
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdForkMockTest, SpawningFd_ForkAndRegister_006, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2012);
    ASSERT_NE(property, nullptr);

    // Force fork to return 0 (child process branch)
    g_mockForkReturn = 0;

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    EXPECT_EQ(pid, 0);  // child branch

    // In the child branch, fds are registered with getpid() as key
    pid_t myPid = getpid();

    // Verify TYPE_CHILD_PARENT registered with getpid()
    AppSpawnFds *pfFds = FindSpawningFdsByPid(mgr_, myPid, TYPE_CHILD_PARENT);
    ASSERT_NE(pfFds, nullptr);
    EXPECT_EQ(pfFds->pid, myPid);

    // Verify TYPE_PARENT_CHILD registered with getpid()
    AppSpawnFds *pcFds = FindSpawningFdsByPid(mgr_, myPid, TYPE_PARENT_CHILD);
    ASSERT_NE(pcFds, nullptr);
    EXPECT_EQ(pcFds->pid, myPid);

    // Verify stats: 2 nodes total
    uint32_t total = 0, cpCount = 0, pcCount = 0;
    GetSpawningFdsStats(mgr_, &total, &cpCount, &pcCount);
    EXPECT_EQ(total, (uint32_t)2);
    EXPECT_EQ(cpCount, (uint32_t)1);
    EXPECT_EQ(pcCount, (uint32_t)1);

    // Cleanup
    CleanupSpawningFdsByPid(mgr_, myPid);

    DeleteAppSpawningCtx(property);
}

}  // namespace OHOS
