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
 * @file app_spawn_checkpoint_process_test.cpp
 * @brief Checkpoint process creation functions test
 *
 * Test coverage:
 * 1. DoCheckpointProcess function tests
 * 2. CreateImageProcess function tests
 * 3. CreateWorkerProcess function tests
 * 4. SpawnPrepareCheckpointFd function tests
 * 5. CreateImageProcessHook function tests
 * 6. CreateWorkerProcessHook function tests
 * 7. SetWorkerProcesssForkDenied function tests (via DoCheckpointProcess)
 */

#include <cerrno>
#include <cstring>
#include <gtest/gtest.h>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/ioctl.h>
#include <vector>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn_service.h"
#include "lib_wrapper.h"
#include "checkpoint_test_helper.h"
#include "securec.h"

// IoctlCheckPointArgs struct definition (defined in appspawn_checkpoint.c, duplicated for testing)
struct IoctlCheckPointArgs {
    pid_t inputPid;
    char name[APP_CHECKPOINT_NAME_LEN];
    uint64_t checkPointId;
    pid_t resultPid;
};

#define CHECKPOINT_IOCTL_CHECKPOINT_ALL    _IOR(0xE0, 0x0, int)  // Create image process
#define CHECKPOINT_IOCTL_RESTORE_ALL       _IOR(0XE0, 0x1, int)  // Create worker process
#define CHECKPOINT_IOCTL_KILL_ALL          _IOR(0xE0, 0x4, int)  // Kill image process

#ifdef __cplusplus
extern "C" {
#endif

// Declare APPSPAWN_STATIC functions from appspawn_checkpoint.c
APPSPAWN_STATIC int32_t DoCheckpointProcess(
    AppSpawnMgr *content, AppSpawningCtx *property, int fd, unsigned long ioctlCmd, bool needForkDenied);
APPSPAWN_STATIC int32_t CreateImageProcess(AppSpawnMgr *content, AppSpawningCtx *property, int fd);
APPSPAWN_STATIC int32_t CreateWorkerProcess(AppSpawnMgr *content, AppSpawningCtx *property, int fd);
APPSPAWN_STATIC int32_t SpawnPrepareCheckpointFd(AppSpawnMgr *content, AppSpawningCtx *property);
APPSPAWN_STATIC int32_t CreateImageProcessHook(AppSpawnMgr *content, AppSpawningCtx *property);
APPSPAWN_STATIC int32_t CreateWorkerProcessHook(AppSpawnMgr *content, AppSpawningCtx *property);

APPSPAWN_STATIC AppSpawnFds *GetSpawningFd(AppSpawnMgr *content, SpawningFdType fdType);
APPSPAWN_STATIC int32_t AddSpawningFds(AppSpawnMgr *content, SpawningFdType type, uint32_t count, int *fds);
APPSPAWN_STATIC AppSpawnedCheckPointProcesses *AddSpawningCheckPointInfo(
    AppSpawnMgr *content, uint64_t checkpointId, const char *processName, uint32_t appIndex, uint32_t uid);
APPSPAWN_STATIC AppSpawnedCheckPointProcesses *GetSpawningImgInfoByName(AppSpawnMgr *content, const char *name);

#ifdef __cplusplus
}
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {

// Track whether open was called (used by ForkDenied_ImageProcessNotCalled_007)
static bool g_openCalledForForkDenied = false;

class AppSpawnCheckpointProcessTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}

    /**
     * @brief Helper to create a full AppSpawningCtx with checkpoint message
     */
    AppSpawningCtx *CreateCheckpointProperty(uint32_t msgType, const char *processName,
                                             pid_t imagePid, uint64_t checkPointId)
    {
        CheckpointTestHelper testHelper;
        std::vector<uint8_t> buffer(1024 * 2);
        uint32_t msgLen = 0;
        CheckpointMsgParams params = {msgType, processName, imagePid, checkPointId, nullptr};
        int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
        if (ret != 0) {
            return nullptr;
        }

        AppSpawnMsgNode *msgNode = nullptr;
        uint32_t msgRecvLen = 0;
        uint32_t reminder = 0;
        ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
        if (ret != 0 || msgNode == nullptr) {
            return nullptr;
        }

        ret = DecodeAppSpawnMsg(msgNode);
        if (ret != 0) {
            DeleteAppSpawnMsg(&msgNode);
            return nullptr;
        }

        AppSpawningCtx *property = CreateAppSpawningCtx();
        if (property == nullptr) {
            DeleteAppSpawnMsg(&msgNode);
            return nullptr;
        }
        property->message = msgNode;
        return property;
    }

    /**
     * @brief Helper to create a property without checkpoint TLV
     */
    AppSpawningCtx *CreatePropertyWithoutCheckpoint(uint32_t msgType, const char *processName)
    {
        AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
        if (msgNode == nullptr) {
            return nullptr;
        }
        strcpy_s(msgNode->msgHeader.processName, APP_LEN_PROC_NAME, processName);
        msgNode->msgHeader.msgType = msgType;
        msgNode->msgHeader.magic = APPSPAWN_MSG_MAGIC;

        AppSpawningCtx *property = CreateAppSpawningCtx();
        if (property == nullptr) {
            DeleteAppSpawnMsg(&msgNode);
            return nullptr;
        }
        property->message = msgNode;
        return property;
    }

    void CleanupProperty(AppSpawningCtx *property)
    {
        if (property != nullptr) {
            AppSpawnMsgNode *msg = property->message;
            property->message = nullptr;
            DeleteAppSpawnMsg(&msg);
            DeleteAppSpawningCtx(property);
        }
    }
};

// ============================================================================
// DoCheckpointProcess function tests
// ============================================================================

/**
 * @tc.name: DoCheckpointProcess_001
 * @tc.desc: Test DoCheckpointProcess with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.doproc", 12345, 1001);
    ASSERT_NE(property, nullptr);

    int32_t ret = DoCheckpointProcess(nullptr, property, 10, 0, false);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
}

/**
 * @tc.name: DoCheckpointProcess_002
 * @tc.desc: Test DoCheckpointProcess with null property
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int32_t ret = DoCheckpointProcess(mgr, nullptr, 10, 0, false);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_003
 * @tc.desc: Test DoCheckpointProcess with null message (no checkpoint info)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = nullptr;

    int32_t ret = DoCheckpointProcess(mgr, property, 10, 0, false);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_004
 * @tc.desc: Test DoCheckpointProcess with missing checkpoint TLV info
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreatePropertyWithoutCheckpoint(MSG_SPAWN_IMAGE_PROCESS, "com.test.nocheckpoint");
    ASSERT_NE(property, nullptr);

    int32_t ret = DoCheckpointProcess(mgr, property, 10, 0, false);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_005
 * @tc.desc: Test DoCheckpointProcess with ioctl failure
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.iofail", 12345, 1001);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        errno = EIO;
        return -1;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    EXPECT_EQ(ret, EIO);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_006
 * @tc.desc: Test DoCheckpointProcess success path for image process (needForkDenied=false)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.imgsuccess", 12345, 1001);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 54321;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(property->pid, 54321);
    EXPECT_EQ(property->checkPointId, 1001);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_007
 * @tc.desc: Test DoCheckpointProcess success path for worker process (needForkDenied=true)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_007, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.workersuccess", 12345, 1002);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 54322;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_RESTORE_ALL, true);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(property->pid, 54322);
    EXPECT_EQ(property->checkPointId, 1002);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_008
 * @tc.desc: Test DoCheckpointProcess updates existing checkpoint info when checkPointId differs
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_008, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Pre-add checkpoint info with different checkPointId
    AppSpawnedCheckPointProcesses *existing = AddSpawningCheckPointInfo(
        mgr, 9999, "com.test.updatecp", 0, 1000);
    ASSERT_NE(existing, nullptr);
    EXPECT_EQ(existing->checkPointId, 9999);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.updatecp", 12345, 2000);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 54323;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    EXPECT_EQ(ret, 0);
    // The existing node should have been updated with the new checkPointId
    EXPECT_EQ(existing->checkPointId, 2000);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_009
 * @tc.desc: Test DoCheckpointProcess adds new checkpoint info when none exists
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_009, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // No pre-existing checkpoint info for this process name
    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.newcp", 12345, 3000);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 54324;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    EXPECT_EQ(ret, 0);

    // Verify the new checkpoint info was added
    AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, "com.test.newcp");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->checkPointId, 3000);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_010
 * @tc.desc: Test DoCheckpointProcess with same checkPointId (no update path)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_010, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Pre-add with same checkPointId
    AppSpawnedCheckPointProcesses *existing = AddSpawningCheckPointInfo(
        mgr, 4000, "com.test.samecp", 0, 1000);
    ASSERT_NE(existing, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.samecp", 12345, 4000);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 54325;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    EXPECT_EQ(ret, 0);
    // checkPointId should remain the same since they match
    EXPECT_EQ(existing->checkPointId, 4000);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_011
 * @tc.desc: Test DoCheckpointProcess with ENOMEM ioctl error
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_011, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.enomem", 12345, 1001);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        errno = ENOMEM;
        return -1;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    EXPECT_EQ(ret, ENOMEM);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_012
 * @tc.desc: Test DoCheckpointProcess with non-empty imgName (use imgName directly, not fallback)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_012, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.customimgname", 12345, 1001,
                                  "CustomCheckpointImgName"};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            EXPECT_STREQ(argsPtr->name, "CustomCheckpointImgName");
            argsPtr->resultPid = 54330;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(property->pid, 54330);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_013
 * @tc.desc: Test DoCheckpointProcess with empty imgName and no bundle info (GetBundleName returns NULL)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_013, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create message with only checkpoint TLV (no bundle info)
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;

    auto addCheckpointOnly = [&testHelper](uint8_t *buf, uint32_t bufLen,
        uint32_t &realLen, uint32_t &count) -> int {
        CheckpointMsgParams cp = {MSG_SPAWN_IMAGE_PROCESS, "com.test.nobundle", 12345, 1001, nullptr};
        return testHelper.CheckpointTestAddCheckpointTlv(buf, bufLen, realLen, count, cp);
    };

    int ret = testHelper.CheckpointTestCreateSendMsg(
        buffer, MSG_SPAWN_IMAGE_PROCESS, msgLen, {addCheckpointOnly});
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;

    // Verify no bundle info present
    AppSpawnMsgBundleInfo *bundleInfo = (AppSpawnMsgBundleInfo *)GetAppSpawnMsgInfo(
        property->message, TLV_BUNDLE_INFO);
    EXPECT_EQ(bundleInfo, nullptr);

    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    // imgName empty + GetBundleName returns NULL → name == NULL → APPSPAWN_SYSTEM_ERROR
    EXPECT_EQ(result, APPSPAWN_SYSTEM_ERROR);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: DoCheckpointProcess_014
 * @tc.desc: Test DoCheckpointProcess with imgName having no null terminator (strcpy_s fails)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, DoCheckpointProcess_014, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create a normal checkpoint message that passes decode validation
    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.strcpyfail", 12345, 1001);
    ASSERT_NE(property, nullptr);

    // Tamper with decoded checkpoint info: fill imgName with non-null bytes (no null terminator)
    // This makes strcpy_s unable to find null within CHECKPOINT_NAME_LEN bytes → returns error
    AppSpawnCheckpointInfo *cpInfo = (AppSpawnCheckpointInfo *)GetAppSpawnMsgInfo(
        property->message, TLV_CHECK_POINT_INFO);
    ASSERT_NE(cpInfo, nullptr);
    memset_s(cpInfo->imgName, APP_CHECKPOINT_NAME_LEN, 'B', APP_CHECKPOINT_NAME_LEN);

    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    // imgName has no null terminator → name[0] != '\0' → use imgName → strcpy_s fails
    EXPECT_EQ(result, APPSPAWN_SYSTEM_ERROR);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// CreateImageProcess function tests
// ============================================================================

/**
 * @tc.name: CreateImageProcess_001
 * @tc.desc: Test CreateImageProcess success path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcess_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.createimg", 12345, 5001);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 60001;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = CreateImageProcess(mgr, property, 10);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(property->pid, 60001);
    EXPECT_EQ(property->checkPointId, 5001);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateImageProcess_002
 * @tc.desc: Test CreateImageProcess with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcess_002, TestSize.Level0)
{
    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.createimg2", 12345, 5002);
    ASSERT_NE(property, nullptr);

    int32_t ret = CreateImageProcess(nullptr, property, 10);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
}

/**
 * @tc.name: CreateImageProcess_003
 * @tc.desc: Test CreateImageProcess with ioctl failure
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcess_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.createimg3", 12345, 5003);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        errno = EINVAL;
        return -1;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = CreateImageProcess(mgr, property, 10);
    EXPECT_EQ(ret, EINVAL);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// CreateWorkerProcess function tests
// ============================================================================

/**
 * @tc.name: CreateWorkerProcess_001
 * @tc.desc: Test CreateWorkerProcess success path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcess_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.createworker", 12345, 6001);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 70001;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = CreateWorkerProcess(mgr, property, 10);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(property->pid, 70001);
    EXPECT_EQ(property->checkPointId, 6001);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateWorkerProcess_002
 * @tc.desc: Test CreateWorkerProcess with null property
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcess_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int32_t ret = CreateWorkerProcess(mgr, nullptr, 10);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateWorkerProcess_003
 * @tc.desc: Test CreateWorkerProcess with ioctl failure
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcess_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.createworker3", 12345, 6003);
    ASSERT_NE(property, nullptr);

    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        errno = EPERM;
        return -1;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t ret = CreateWorkerProcess(mgr, property, 10);
    EXPECT_EQ(ret, EPERM);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// SpawnPrepareCheckpointFd function tests
// ============================================================================

/**
 * @tc.name: SpawnPrepareCheckpointFd_001
 * @tc.desc: Test SpawnPrepareCheckpointFd with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, SpawnPrepareCheckpointFd_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.preparenull", 12345, 1001);
    ASSERT_NE(property, nullptr);

    int32_t ret = SpawnPrepareCheckpointFd(nullptr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
}

/**
 * @tc.name: SpawnPrepareCheckpointFd_002
 * @tc.desc: Test SpawnPrepareCheckpointFd with null property
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, SpawnPrepareCheckpointFd_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int32_t ret = SpawnPrepareCheckpointFd(mgr, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: SpawnPrepareCheckpointFd_003
 * @tc.desc: Test SpawnPrepareCheckpointFd with wrong mode
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, SpawnPrepareCheckpointFd_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.preparemode", 12345, 1001);
    ASSERT_NE(property, nullptr);

    int32_t ret = SpawnPrepareCheckpointFd(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: SpawnPrepareCheckpointFd_004
 * @tc.desc: Test SpawnPrepareCheckpointFd without checkpoint info (not checkpoint request)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, SpawnPrepareCheckpointFd_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreatePropertyWithoutCheckpoint(MSG_SPAWN_IMAGE_PROCESS, "com.test.preparenochk");
    ASSERT_NE(property, nullptr);

    int32_t ret = SpawnPrepareCheckpointFd(mgr, property);
    EXPECT_EQ(ret, 0);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: SpawnPrepareCheckpointFd_005
 * @tc.desc: Test SpawnPrepareCheckpointFd when fd already cached
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, SpawnPrepareCheckpointFd_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Pre-add a cached fd
    int fd = 100;
    int addRet = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, &fd);
    EXPECT_EQ(addRet, 0);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.preparecached", 12345, 1001);
    ASSERT_NE(property, nullptr);

    int32_t ret = SpawnPrepareCheckpointFd(mgr, property);
    EXPECT_EQ(ret, 0);

    // Verify fd is still the same
    AppSpawnFds *fdNode = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(fdNode, nullptr);
    EXPECT_EQ(fdNode->fds[0], 100);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: SpawnPrepareCheckpointFd_006
 * @tc.desc: Test SpawnPrepareCheckpointFd with open failure
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, SpawnPrepareCheckpointFd_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.prepareopenfail", 12345, 1001);
    ASSERT_NE(property, nullptr);

    // Mock open to fail
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        errno = ENOENT;
        return -1;
    };
    UpdateOpenFunc(openFunc);

    int32_t ret = SpawnPrepareCheckpointFd(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_SYSTEM_ERROR);

    UpdateOpenFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: SpawnPrepareCheckpointFd_007
 * @tc.desc: Test SpawnPrepareCheckpointFd success path (open succeeds)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, SpawnPrepareCheckpointFd_007, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.preparesuccess", 12345, 1001);
    ASSERT_NE(property, nullptr);

    // Mock open to return a fake fd
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        return 42;  // Return fake fd
    };
    UpdateOpenFunc(openFunc);

    int32_t ret = SpawnPrepareCheckpointFd(mgr, property);
    EXPECT_EQ(ret, 0);

    // Verify fd was cached
    AppSpawnFds *fdNode = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(fdNode, nullptr);
    EXPECT_EQ(fdNode->fds[0], 42);

    UpdateOpenFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// CreateImageProcessHook function tests
// ============================================================================

/**
 * @tc.name: CreateImageProcessHook_001
 * @tc.desc: Test CreateImageProcessHook with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcessHook_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.imghooknull", 12345, 1001);
    ASSERT_NE(property, nullptr);

    int32_t ret = CreateImageProcessHook(nullptr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
}

/**
 * @tc.name: CreateImageProcessHook_002
 * @tc.desc: Test CreateImageProcessHook with null property
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcessHook_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int32_t ret = CreateImageProcessHook(mgr, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateImageProcessHook_003
 * @tc.desc: Test CreateImageProcessHook with wrong mode
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcessHook_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.imghookmode", 12345, 1001);
    ASSERT_NE(property, nullptr);

    int32_t ret = CreateImageProcessHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateImageProcessHook_004
 * @tc.desc: Test CreateImageProcessHook when APP_FLAGS_SPAWN_IMAGE_PROCESS is not set (not image process)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcessHook_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create property without APP_FLAGS_SPAWN_IMAGE_PROCESS flag
    AppSpawningCtx *property = CreatePropertyWithoutCheckpoint(MSG_SPAWN_IMAGE_PROCESS, "com.test.imghooknoflag");
    ASSERT_NE(property, nullptr);
    SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_SPAWN_IMAGE_PROCESS);

    int32_t ret = CreateImageProcessHook(mgr, property);
    EXPECT_EQ(ret, 0);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateImageProcessHook_005
 * @tc.desc: Test CreateImageProcessHook when checkpoint fd not prepared
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcessHook_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create a checkpoint property with APP_FLAGS_SPAWN_IMAGE_PROCESS set
    // Using CheckpointTestHelper which adds msg flags TLV with bit 41 set
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.imghooknofd", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;
    SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_SPAWN_IMAGE_PROCESS);

    // Do NOT add fd to spawningFdsQueue - should return APPSPAWN_SYSTEM_ERROR
    int32_t hookRet = CreateImageProcessHook(mgr, property);
    EXPECT_EQ(hookRet, APPSPAWN_SYSTEM_ERROR);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateImageProcessHook_006
 * @tc.desc: Test CreateImageProcessHook success path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateImageProcessHook_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create checkpoint property
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.imghooksuccess", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;
    SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_SPAWN_IMAGE_PROCESS);

    // Add checkpoint fd
    int fd = 42;
    int addRet = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, &fd);
    EXPECT_EQ(addRet, 0);

    // Mock ioctl to return success
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 80001;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t hookRet = CreateImageProcessHook(mgr, property);
    EXPECT_EQ(hookRet, 0);
    EXPECT_EQ(property->pid, 80001);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// CreateWorkerProcessHook function tests
// ============================================================================

/**
 * @tc.name: CreateWorkerProcessHook_001
 * @tc.desc: Test CreateWorkerProcessHook with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcessHook_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.workerhooknull", 12345, 1001);
    ASSERT_NE(property, nullptr);

    int32_t ret = CreateWorkerProcessHook(nullptr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
}

/**
 * @tc.name: CreateWorkerProcessHook_002
 * @tc.desc: Test CreateWorkerProcessHook with null property
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcessHook_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int32_t ret = CreateWorkerProcessHook(mgr, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateWorkerProcessHook_003
 * @tc.desc: Test CreateWorkerProcessHook with wrong mode
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcessHook_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.workerhookmode", 12345, 1001);
    ASSERT_NE(property, nullptr);

    int32_t ret = CreateWorkerProcessHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateWorkerProcessHook_004
 * @tc.desc: Test CreateWorkerProcessHook when APP_FLAGS_SPAWN_IMAGE_PROCESS is set (image process, skip)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcessHook_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create a checkpoint property with image process flags set
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.workerhookimg", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;
    SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_SPAWN_IMAGE_PROCESS);

    // Since APP_FLAGS_SPAWN_IMAGE_PROCESS is set, CreateWorkerProcessHook should return 0 and skip
    int32_t hookRet = CreateWorkerProcessHook(mgr, property);
    EXPECT_EQ(hookRet, 0);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateWorkerProcessHook_005
 * @tc.desc: Test CreateWorkerProcessHook without checkpoint info (not checkpoint request, return 0)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcessHook_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create property without checkpoint TLV
    AppSpawningCtx *property = CreatePropertyWithoutCheckpoint(MSG_SPAWN_IMAGE_PROCESS, "com.test.workerhooknochk");
    ASSERT_NE(property, nullptr);

    int32_t ret = CreateWorkerProcessHook(mgr, property);
    EXPECT_EQ(ret, 0);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateWorkerProcessHook_006
 * @tc.desc: Test CreateWorkerProcessHook when checkpoint fd not prepared
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcessHook_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create a worker process checkpoint property without image flags set
    // We need to construct a message that has checkpoint info but NOT APP_FLAGS_SPAWN_IMAGE_PROCESS
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    // Use MSG_SPAWN_WORKER_PROCESS to create a message, but we'll manually adjust flags
    CheckpointMsgParams params = {MSG_SPAWN_WORKER_PROCESS, "com.test.workerhooknofd", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;

    // Do NOT add fd to spawningFdsQueue - should return APPSPAWN_SYSTEM_ERROR
    int32_t hookRet = CreateWorkerProcessHook(mgr, property);
    EXPECT_EQ(hookRet, APPSPAWN_SYSTEM_ERROR);

    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CreateWorkerProcessHook_007
 * @tc.desc: Test CreateWorkerProcessHook success path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CreateWorkerProcessHook_007, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create a worker process checkpoint property
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_WORKER_PROCESS, "com.test.workerhooksuccess", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;

    // Add checkpoint fd
    int fd = 42;
    int addRet = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, &fd);
    EXPECT_EQ(addRet, 0);

    // Mock ioctl to return success
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 90001;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t hookRet = CreateWorkerProcessHook(mgr, property);
    EXPECT_EQ(hookRet, 0);
    EXPECT_EQ(property->pid, 90001);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// End-to-end hook chain test
// ============================================================================

/**
 * @tc.name: CheckpointProcess_E2E_001
 * @tc.desc: Test end-to-end: prepare fd -> create image process hook -> verify
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CheckpointProcess_E2E_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.e2eimage", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;
    SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_SPAWN_IMAGE_PROCESS);

    // Step 1: Mock open and prepare checkpoint fd
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        return 99;
    };
    UpdateOpenFunc(openFunc);
    int32_t prepareRet = SpawnPrepareCheckpointFd(mgr, property);
    EXPECT_EQ(prepareRet, 0);
    UpdateOpenFunc(nullptr);

    // Step 2: Create image process via hook
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 111111;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t hookRet = CreateImageProcessHook(mgr, property);
    EXPECT_EQ(hookRet, 0);
    EXPECT_EQ(property->pid, 111111);
    EXPECT_EQ(property->checkPointId, 1001);

    // Verify checkpoint info was added
    AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, "com.test.e2eimage");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->checkPointId, 1001);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: CheckpointProcess_E2E_002
 * @tc.desc: Test end-to-end: prepare fd -> create worker process hook -> verify
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, CheckpointProcess_E2E_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_WORKER_PROCESS, "com.test.e2eworker", 12345, 2001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;

    // Step 1: Prepare checkpoint fd
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        return 88;
    };
    UpdateOpenFunc(openFunc);
    int32_t prepareRet = SpawnPrepareCheckpointFd(mgr, property);
    EXPECT_EQ(prepareRet, 0);
    UpdateOpenFunc(nullptr);

    // Step 2: Create worker process via hook
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 222222;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int32_t hookRet = CreateWorkerProcessHook(mgr, property);
    EXPECT_EQ(hookRet, 0);
    EXPECT_EQ(property->pid, 222222);
    EXPECT_EQ(property->checkPointId, 2001);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// SetWorkerProcesssForkDenied function tests (tested via DoCheckpointProcess)
// ============================================================================

/**
 * @tc.name: ForkDenied_NoDacInfo_001
 * @tc.desc: Test SetWorkerProcesssForkDenied when DAC info is absent (appInfo == NULL)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, ForkDenied_NoDacInfo_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Build a message with only checkpoint TLV (no base TLVs, no DAC info)
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;

    auto addCheckpointOnly = [&testHelper](uint8_t *buf, uint32_t bufLen,
        uint32_t &realLen, uint32_t &count) -> int {
        CheckpointMsgParams cp = {MSG_SPAWN_WORKER_PROCESS, "com.test.forkdenied_nodac", 12345, 7001, nullptr};
        return testHelper.CheckpointTestAddCheckpointTlv(buf, bufLen, realLen, count, cp);
    };

    auto addBundleNameOnly = [&testHelper](uint8_t *buf, uint32_t bufLen,
        uint32_t &realLen, uint32_t &count) -> int {
        CheckpointMsgParams cp = {MSG_SPAWN_WORKER_PROCESS, "com.test.forkdenied_nodac", 12345, 7001, nullptr};
        return testHelper.CheckpointTestAddBundleTlv(buf, bufLen, realLen, count, cp);
    };

    int ret = testHelper.CheckpointTestCreateSendMsg(
        buffer, MSG_SPAWN_WORKER_PROCESS, msgLen, {addCheckpointOnly, addBundleNameOnly});
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = msgNode;

    // Verify no DAC info in the message
    AppDacInfo *dacInfo = (AppDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    EXPECT_EQ(dacInfo, nullptr);

    // Mock ioctl to succeed
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 55555;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    // DoCheckpointProcess with needForkDenied=true
    // SetWorkerProcesssForkDenied should return early (no DAC info),
    // but DoCheckpointProcess should still succeed
    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_RESTORE_ALL, true);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(property->pid, 55555);

    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: ForkDenied_NullProperty_002
 * @tc.desc: Test SetWorkerProcesssForkDenied when property->message is NULL (GetAppProperty returns NULL)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, ForkDenied_NullProperty_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = nullptr;

    // DoCheckpointProcess returns early for null message, SetWorkerProcesssForkDenied never reached
    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_RESTORE_ALL, true);
    EXPECT_EQ(result, APPSPAWN_ARG_INVALID);

    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: ForkDenied_OpenFail_003
 * @tc.desc: Test SetWorkerProcesssForkDenied when open() for fork_denied path fails
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, ForkDenied_OpenFail_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create property with full TLVs (includes DAC info)
    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.forkdenied_openfail", 12345, 7002);
    ASSERT_NE(property, nullptr);

    // Verify DAC info exists
    AppDacInfo *dacInfo = (AppDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    ASSERT_NE(dacInfo, nullptr);

    // Mock ioctl to succeed
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 55556;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    // Mock open to fail for the fork_denied path
    // The first open call (for checkpoint device) should NOT be intercepted here
    // because DoCheckpointProcess is called with fd already provided.
    // The open inside SetWorkerProcesssForkDenied opens /dev/pids/... path
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        // Fail the fork_denied open
        errno = ENOENT;
        return -1;
    };
    UpdateOpenFunc(openFunc);

    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_RESTORE_ALL, true);
    // DoCheckpointProcess should still succeed even if SetWorkerProcesssForkDenied's open fails
    EXPECT_EQ(result, 0);
    EXPECT_EQ(property->pid, 55556);

    UpdateOpenFunc(nullptr);
    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: ForkDenied_OpenSuccess_WriteSuccess_004
 * @tc.desc: Test SetWorkerProcesssForkDenied when open and write both succeed
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, ForkDenied_OpenSuccess_WriteSuccess_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.forkdenied_wrok", 12345, 7003);
    ASSERT_NE(property, nullptr);

    // Verify DAC info exists
    AppDacInfo *dacInfo = (AppDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    ASSERT_NE(dacInfo, nullptr);

    // Mock ioctl to succeed
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 55557;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    // Mock open to succeed
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        return 200;  // Return fake fd
    };
    UpdateOpenFunc(openFunc);

    // Mock write to succeed
    WriteFunc writeFunc = [](int fd, const void *buf, size_t count) -> ssize_t {
        return static_cast<ssize_t>(count);  // Return bytes written
    };
    UpdateWriteFunc(writeFunc);

    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_RESTORE_ALL, true);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(property->pid, 55557);
    EXPECT_EQ(property->checkPointId, 7003);

    UpdateWriteFunc(nullptr);
    UpdateOpenFunc(nullptr);
    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: ForkDenied_OpenSuccess_WriteFail_005
 * @tc.desc: Test SetWorkerProcesssForkDenied when open succeeds but write fails
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, ForkDenied_OpenSuccess_WriteFail_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.forkdenied_wfail", 12345, 7004);
    ASSERT_NE(property, nullptr);

    // Verify DAC info exists
    AppDacInfo *dacInfo = (AppDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    ASSERT_NE(dacInfo, nullptr);

    // Mock ioctl to succeed
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 55558;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    // Mock open to succeed
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        return 201;  // Return fake fd
    };
    UpdateOpenFunc(openFunc);

    // Mock write to fail
    WriteFunc writeFunc = [](int fd, const void *buf, size_t count) -> ssize_t {
        errno = EIO;
        return -1;
    };
    UpdateWriteFunc(writeFunc);

    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_RESTORE_ALL, true);
    // DoCheckpointProcess should still succeed even if write fails in SetWorkerProcesssForkDenied
    EXPECT_EQ(result, 0);
    EXPECT_EQ(property->pid, 55558);

    UpdateWriteFunc(nullptr);
    UpdateOpenFunc(nullptr);
    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: ForkDenied_WithDacInfo_UidCalc_006
 * @tc.desc: Test SetWorkerProcesssForkDenied with specific uid to verify userId calculation
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, ForkDenied_WithDacInfo_UidCalc_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_WORKER_PROCESS, "com.test.forkdenied_uidcalc", 12345, 7005);
    ASSERT_NE(property, nullptr);

    // Verify DAC info and uid value (20010029 / 200000 = 100)
    AppDacInfo *dacInfo = (AppDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    ASSERT_NE(dacInfo, nullptr);
    EXPECT_EQ(dacInfo->uid, 20010029u);
    int expectedUserId = static_cast<int>(dacInfo->uid / UID_BASE);
    EXPECT_EQ(expectedUserId, 100);

    // Mock ioctl to succeed and set a known pid
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 55559;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    // Mock open to succeed
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        return 202;
    };
    UpdateOpenFunc(openFunc);

    // Mock write to succeed
    WriteFunc writeFunc = [](int fd, const void *buf, size_t count) -> ssize_t {
        return static_cast<ssize_t>(count);
    };
    UpdateWriteFunc(writeFunc);

    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_RESTORE_ALL, true);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(property->pid, 55559);

    UpdateWriteFunc(nullptr);
    UpdateOpenFunc(nullptr);
    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: ForkDenied_ImageProcessNotCalled_007
 * @tc.desc: Test that SetWorkerProcesssForkDenied is NOT called for image process (needForkDenied=false)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCheckpointProcessTest, ForkDenied_ImageProcessNotCalled_007, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateCheckpointProperty(
        MSG_SPAWN_IMAGE_PROCESS, "com.test.forkdenied_notcalled", 12345, 7006);
    ASSERT_NE(property, nullptr);

    // Mock ioctl to succeed
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 55560;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    // Mock open - should NOT be called since needForkDenied=false
    g_openCalledForForkDenied = false;
    OpenFunc openFunc = [](const char *pathname, int flags, mode_t mode) -> int {
        g_openCalledForForkDenied = true;
        return 200;
    };
    UpdateOpenFunc(openFunc);

    // Call with needForkDenied=false (image process)
    int32_t result = DoCheckpointProcess(mgr, property, 10, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(property->pid, 55560);
    // open should not be called for image process path
    EXPECT_FALSE(g_openCalledForForkDenied);

    UpdateOpenFunc(nullptr);
    UpdateIoctlFunc(nullptr);
    CleanupProperty(property);
    DeleteAppSpawnMgr(mgr);
}

}  // namespace OHOS
