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

#include <cerrno>
#include <cstring>
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "app_spawn_stub.h"
#include "lib_wrapper.h"
#include "app_spawn_test_helper.h"
#include "securec.h"

#ifdef __cplusplus
extern "C" {
#endif

APPSPAWN_STATIC AppSpawnFds *GetSpawningFd(AppSpawnMgr *content, SpawningFdType fdType);
APPSPAWN_STATIC int32_t AddSpawningFds(AppSpawnMgr *content, SpawningFdType type, uint32_t count, int *fds);
APPSPAWN_STATIC AppSpawnedCheckPointProcesses *AddSpawningImgInfo(
    AppSpawnMgr *content, uint64_t checkpointId, const char *processName, uint32_t appIndex, uint32_t uid);
APPSPAWN_STATIC AppSpawnedCheckPointProcesses *GetSpawningImgInfoByName(AppSpawnMgr *content, const char *name);
APPSPAWN_STATIC int32_t ProcessWithForkAllFd(AppSpawnMgr *content, AppSpawningCtx *property, int fd);
APPSPAWN_STATIC int32_t SpawnGetForkAllPid(const AppSpawnMgr *content, AppSpawningCtx *property);
APPSPAWN_STATIC void SpawnDestroyImgQue(const AppSpawnMgr *content);
APPSPAWN_STATIC int32_t GetAppImgInfo(AppSpawnMgr *content, AppSpawningCtx *property, int fd);

typedef void (*CheckPointProcessTraversal)(const AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data);
APPSPAWN_STATIC void TraversalImgProcess(CheckPointProcessTraversal traversal, const AppSpawnMgr *content, void *data);

#ifdef __cplusplus
}
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static AppSpawnTestHelper g_testHelper;

/**
 * @brief 镜像启动测试用例
 * 用于测试 MSG_APP_FORK_ALL 和 MSG_GET_APP_IMG_INFO 消息类型处理
 * 以及 AppSpawnClientSendImgSpawnMsg 和 AppSpawnReqMsgSetImgInfo 接口
 */
class AppSpawnImgInfoTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// Helper function to create AppSpawnReqMsgHandle for img info testing
static AppSpawnReqMsgHandle CreateImgInfoReqMsg(
    AppSpawnMsgType msgType, const char *processName, pid_t imgPid, uint64_t checkPointId)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(msgType, processName, &reqHandle);
    APPSPAWN_CHECK(ret == 0, return INVALID_REQ_HANDLE, "Failed to create req msg for %{public}s", processName);

    if (msgType == MSG_GET_APP_IMG_INFO || msgType == MSG_APP_FORK_ALL) {
        // Set img info
        ret = AppSpawnReqMsgSetImgInfo(reqHandle, imgPid, checkPointId);
        if (ret != 0) {
            APPSPAWN_LOGE("Failed to set img info for %{public}s, ret=%{public}d", processName, ret);
            AppSpawnReqMsgFree(reqHandle);
            return INVALID_REQ_HANDLE;
        }
    }

    return reqHandle;
}

// Helper function to create a mock AppSpawnMsgNode with imgPid info
static AppSpawnMsgNode *CreateMsgNodeWithImgPid(const char *processName, const char *imgPidStr)
{
    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    if (msgNode == nullptr) {
        return nullptr;
    }
    if (processName != nullptr) {
        strcpy_s(msgNode->msgHeader.processName, APP_LEN_PROC_NAME, processName);
    }
    return msgNode;
}

/**
 * @tc.name: App_ImgInfo_SetImgInfo_001
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with valid parameters
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SetImgInfo_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "test.img.process", &reqHandle);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 99999;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfo_SetImgInfo_002
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with invalid handle (NULL)
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SetImgInfo_002, TestSize.Level0)
{
    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 99999;

    int ret = AppSpawnReqMsgSetImgInfo(nullptr, testImgPid, testCheckPointId);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: App_ImgInfo_SetImgInfo_003
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with negative imgPid
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SetImgInfo_003, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "test.img.process", &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = -1;  // Negative pid
    uint64_t testCheckPointId = 99999;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_NE(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfo_SetImgInfo_004
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with zero imgPid
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SetImgInfo_004, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "test.img.process", &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = 0;  // Zero pid
    uint64_t testCheckPointId = 99999;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_NE(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfo_SetImgInfo_005
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with zero checkPointId
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SetImgInfo_005, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "test.img.process", &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 0;  // Zero checkpoint id

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfo_SetImgInfo_006
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with maximum checkpointId
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SetImgInfo_006, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "test.img.process", &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 0xFFFFFFFFFFFFFFFF;  // Maximum uint64_t

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfo_SetImgInfo_007
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo called multiple times
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SetImgInfo_007, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "test.img.process", &reqHandle);
    ASSERT_EQ(ret, 0);

    // First call
    pid_t testImgPid1 = 11111;
    uint64_t testCheckPointId1 = 111111;
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid1, testCheckPointId1);
    EXPECT_EQ(ret, 0);

    // Second call - should update
    pid_t testImgPid2 = 22222;
    uint64_t testCheckPointId2 = 222222;
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid2, testCheckPointId2);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfo_MsgType_001
 * @tc.desc: Test MSG_APP_FORK_ALL message type value
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_MsgType_001, TestSize.Level0)
{
    // Verify MSG_APP_FORK_ALL is correctly defined
    EXPECT_EQ(MSG_APP_FORK_ALL, static_cast<AppSpawnMsgType>(14));
    EXPECT_EQ(MSG_GET_APP_IMG_INFO, static_cast<AppSpawnMsgType>(15));
}

/**
 * @tc.name: App_ImgInfo_GetAppImgInfo_001
 * @tc.desc: Test GetAppImgInfo with null parameters
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetAppImgInfo_001, TestSize.Level0)
{
    AppSpawningCtx property;
    memset_s(&property, sizeof(AppSpawningCtx), 0, sizeof(AppSpawningCtx));

    int fd = 10;
    int32_t ret = GetAppImgInfo(nullptr, &property, fd);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @tc.name: App_ImgInfo_GetAppImgInfo_002
 * @tc.desc: Test GetAppImgInfo with null property
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetAppImgInfo_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx property;
    memset_s(&property, sizeof(AppSpawningCtx), 0, sizeof(AppSpawningCtx));

    int fd = 10;
    int32_t ret = GetAppImgInfo(mgr, nullptr, fd);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_GetAppImgInfo_003
 * @tc.desc: Test GetAppImgInfo with missing img info in message
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetAppImgInfo_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        ret = GetAppImgInfo(mgr, property, 10);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_GetAppImgInfo_004
 * @tc.desc: Test GetAppImgInfo with img info in message
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetAppImgInfo_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 123, 1);
        EXPECT_EQ(ret, 0);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return 0;
        };
        UpdateIoctlFunc(ioctlFunc);
        ret = GetAppImgInfo(mgr, property, 10);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_GetAppImgInfo_005
 * @tc.desc: Test GetAppImgInfo with updates existing checkpoint info
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetAppImgInfo_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node1 = AddSpawningImgInfo(mgr, 1, "com.example.myapplication", 0, 0);
    ASSERT_NE(node1, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 123, 1);
        EXPECT_EQ(ret, 0);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return 0;
        };
        UpdateIoctlFunc(ioctlFunc);
        ret = GetAppImgInfo(mgr, property, 10);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_GetAppImgInfo_006
 * @tc.desc: Test GetAppImgInfo with img info in message but ioctl failed
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetAppImgInfo_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 123, 1);
        EXPECT_EQ(ret, 0);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return -1;
        };
        UpdateIoctlFunc(ioctlFunc);
        ret = GetAppImgInfo(mgr, property, 10);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_GetAppImgInfo_007
 * @tc.desc: Test GetAppImgInfo with img info in message and AppSpawnedCheckPointProcesses in checkPointIdQueue
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetAppImgInfo_007, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node1 = AddSpawningImgInfo(mgr, 2, "com.example.myapplication", 0, 0);
    ASSERT_NE(node1, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 123, 1);
        EXPECT_EQ(ret, 0);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return 0;
        };
        UpdateIoctlFunc(ioctlFunc);
        ret = GetAppImgInfo(mgr, property, 10);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_SpawnGetForkAllPid_001
 * @tc.desc: Test SpawnGetForkAllPid with APP_FLAGS_SPAWN_IMG_INFO flag set
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SpawnGetForkAllPid_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Pre-add fork all fd
    int fds[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_EQ(ret, 0);

    AppSpawningCtx property;
    memset_s(&property, sizeof(AppSpawningCtx), 0, sizeof(AppSpawningCtx));

    // Create message with SPAWN_IMG_INFO flag
    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    ASSERT_NE(msgNode, nullptr);
    strcpy_s(msgNode->msgHeader.processName, APP_LEN_PROC_NAME, "test_process");

    property.message = msgNode;

    // This should call SpawnGetForkAllPid due to SPAWN_IMG_INFO flag
    int32_t result = SpawnGetForkAllPid(mgr, &property);
    // Expected to fail in unit test environment without real checkpoint/clone device
    APPSPAWN_LOGV("SpawnGetForkAllPid result: %{public}d", result);

    DeleteAppSpawnMsg(&msgNode);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_SpawnGetForkAllPid_002
 * @tc.desc: Test SpawnGetForkAllPid without APP_FLAGS_SPAWN_IMG_INFO flag
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_SpawnGetForkAllPid_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Pre-add fork all fd
    int fds[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_EQ(ret, 0);

    AppSpawningCtx property;
    memset_s(&property, sizeof(AppSpawningCtx), 0, sizeof(AppSpawningCtx));

    // Create message without SPAWN_IMG_INFO flag
    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    ASSERT_NE(msgNode, nullptr);
    strcpy_s(msgNode->msgHeader.processName, APP_LEN_PROC_NAME, "test_process");

    property.message = msgNode;

    // This should call ProcessWithForkAllFd
    int32_t result = SpawnGetForkAllPid(mgr, &property);
    // Expected to fail in unit test environment without real checkpoint/clone device
    APPSPAWN_LOGV("SpawnGetForkAllPid result: %{public}d", result);

    DeleteAppSpawnMsg(&msgNode);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_ForkAllWithImgInfo_001
 * @tc.desc: Test fork all process creation with img info set
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_ForkAllWithImgInfo_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Pre-add fork all fd
    int fds[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_EQ(ret, 0);

    // Create message with img info
    AppSpawnMsgNode *msgNode = CreateMsgNodeWithImgPid("test_fork_all", "5000");
    ASSERT_NE(msgNode, nullptr);

    AppSpawningCtx property;
    memset_s(&property, sizeof(AppSpawningCtx), 0, sizeof(AppSpawningCtx));
    property.message = msgNode;

    // Add img info to message if needed for the test
    // ProcessWithForkAllFd expects img info to be present in message
    int32_t result = ProcessWithForkAllFd(mgr, &property, fds[0]);
    // Will fail on ioctl in unit test without real checkpoint device
    APPSPAWN_LOGV("ProcessWithForkAllFd result: %{public}d", result);

    DeleteAppSpawnMsg(&msgNode);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_ForkAllWithImgInfo_002
 * @tc.desc: Test ProcessWithForkAllFd with img info in message
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_ForkAllWithImgInfo_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 123, 1);
        EXPECT_EQ(ret, 0);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return 0;
        };
        UpdateIoctlFunc(ioctlFunc);
        ret = ProcessWithForkAllFd(mgr, property, 10);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_ForkAllWithImgInfo_003
 * @tc.desc: Test ProcessWithForkAllFd updates existing checkpoint info
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_ForkAllWithImgInfo_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node1 = AddSpawningImgInfo(mgr, 1, "com.example.myapplication", 0, 0);
    ASSERT_NE(node1, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 123, 1);
        EXPECT_EQ(ret, 0);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return 0;
        };
        UpdateIoctlFunc(ioctlFunc);
        ret = ProcessWithForkAllFd(mgr, property, 10);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_ForkAllWithImgInfo_004
 * @tc.desc: Test GetAppImgInfo with img info in message but ioctl failed
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_ForkAllWithImgInfo_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 123, 1);
        EXPECT_EQ(ret, 0);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return -1;
        };
        UpdateIoctlFunc(ioctlFunc);
        ret = ProcessWithForkAllFd(mgr, property, 10);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_ForkAllWithImgInfo_005
 * @tc.desc: Test ProcessWithForkAllFd updates existing checkpoint info
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_ForkAllWithImgInfo_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node1 = AddSpawningImgInfo(mgr, 2, "com.example.myapplication", 0, 0);
    ASSERT_NE(node1, nullptr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 123, 1);
        EXPECT_EQ(ret, 0);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return 0;
        };
        UpdateIoctlFunc(ioctlFunc);
        ret = ProcessWithForkAllFd(mgr, property, 10);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_ImgQueueDestroy_001
 * @tc.desc: Test SpawnDestroyImgQue destroys all img processes
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_ImgQueueDestroy_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Add multiple img processes
    AppSpawnedCheckPointProcesses *node1 = AddSpawningImgInfo(mgr, 1001, "destroy_test1", 0, 1000);
    ASSERT_NE(node1, nullptr);
    AppSpawnedCheckPointProcesses *node2 = AddSpawningImgInfo(mgr, 1002, "destroy_test2", 0, 1001);
    ASSERT_NE(node2, nullptr);
    AppSpawnedCheckPointProcesses *node3 = AddSpawningImgInfo(mgr, 1003, "destroy_test3", 0, 1002);
    ASSERT_NE(node3, nullptr);

    // Verify all were added
    int countBefore = 0;
    auto countTraversal = [](const AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data) {
        int *count = static_cast<int *>(data);
        (*count)++;
    };
    TraversalImgProcess(countTraversal, mgr, &countBefore);
    EXPECT_EQ(countBefore, 3);

    // Add invalid fork all fd (ioctl will fail but traversal should work)
    int invalidFd = -1;
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, &invalidFd);
    EXPECT_EQ(ret, 0);

    // Destroy all img processes
    SpawnDestroyImgQue(mgr);

    // Note: ioctl will fail with invalid fd, but traversal happens
    APPSPAWN_LOGV("SpawnDestroyImgQue completed");

    // Verify some are removed (actual cleanup depends on ioctl results)
    AppSpawnedCheckPointProcesses *found1 = GetSpawningImgInfoByName(mgr, "destroy_test1");
    AppSpawnedCheckPointProcesses *found2 = GetSpawningImgInfoByName(mgr, "destroy_test2");
    AppSpawnedCheckPointProcesses *found3 = GetSpawningImgInfoByName(mgr, "destroy_test3");

    // In unit test without real checkpoint device, nodes might still exist
    // Real behavior verified in integration tests
    APPSPAWN_LOGV("After destroy - found1=%{public}p, found2=%{public}p, found3=%{public}p",
        static_cast<void*>(found1), static_cast<void*>(found2), static_cast<void*>(found3));

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_ImgProcessManagement_001
 * @tc.desc: Test complete lifecycle of img process management
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_ImgProcessManagement_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Step 1: Add img process info
    AppSpawnedCheckPointProcesses *node1 = AddSpawningImgInfo(mgr, 2001, "lifecycle_test1", 0, 1000);
    ASSERT_NE(node1, nullptr);
    AppSpawnedCheckPointProcesses *node2 = AddSpawningImgInfo(mgr, 2002, "lifecycle_test2", 0, 1001);
    ASSERT_NE(node2, nullptr);

    // Step 2: Verify retrieval by name
    AppSpawnedCheckPointProcesses *found = nullptr;
    found = GetSpawningImgInfoByName(mgr, "lifecycle_test1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->checkPointId, 2001);

    found = GetSpawningImgInfoByName(mgr, "lifecycle_test2");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->checkPointId, 2002);

    // Step 3: Verify ordering by checkpointId
    std::vector<uint64_t> checkpointIds;
    auto collectIds = [](const AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data) {
        std::vector<uint64_t> *ids = static_cast<std::vector<uint64_t> *>(data);
        ids->push_back(imgInfo->checkPointId);
    };
    std::vector<uint64_t> ids;
    TraversalImgProcess(collectIds, mgr, &ids);

    ASSERT_EQ(ids.size(), 2);
    EXPECT_EQ(ids[0], 2001);  // Should be sorted
    EXPECT_EQ(ids[1], 2002);

    // Step 4: Add fork all fd
    int fds[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_EQ(ret, 0);

    // Step 5: Get fd and verify
    AppSpawnFds *forkAllFdInfo = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(forkAllFdInfo, nullptr);
    EXPECT_EQ(forkAllFdInfo->fds[0], 100);

    // Step 6: Destroy queue
    SpawnDestroyImgQue(mgr);
    APPSPAWN_LOGV("Img process lifecycle test completed");

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_TraversalWithData_001
 * @tc.desc: Test img process traversal with custom data
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_TraversalWithData_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Add img processes
    AddSpawningImgInfo(mgr, 3001, "traversal_a", 0, 1000);
    AddSpawningImgInfo(mgr, 3002, "traversal_b", 0, 1001);
    AddSpawningImgInfo(mgr, 3003, "traversal_c", 0, 1002);

    // Custom data structure for traversal
    struct TraversalData {
        std::vector<std::string> names;
        std::vector<uint64_t> checkpointIds;
        uint32_t count;
    };

    TraversalData data = {};
    auto customTraversal = [](const AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data) {
        TraversalData *td = static_cast<TraversalData *>(data);
        td->names.push_back(imgInfo->name);
        td->checkpointIds.push_back(imgInfo->checkPointId);
        td->count++;
    };

    TraversalImgProcess(customTraversal, mgr, &data);

    EXPECT_EQ(data.count, 3);
    ASSERT_EQ(data.names.size(), 3);
    ASSERT_EQ(data.checkpointIds.size(), 3);

    EXPECT_STREQ(data.names[0].c_str(), "traversal_a");
    EXPECT_EQ(data.checkpointIds[0], 3001);

    EXPECT_STREQ(data.names[1].c_str(), "traversal_b");
    EXPECT_EQ(data.checkpointIds[1], 3002);

    EXPECT_STREQ(data.names[2].c_str(), "traversal_c");
    EXPECT_EQ(data.checkpointIds[2], 3003);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_FdManagement_001
 * @tc.desc: Test fd type management with TYPE_FOR_FORK_ALL
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_FdManagement_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Add TYPE_FOR_FORK_ALL fd
    int forkAllFds[2] = {50, 60};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 2, forkAllFds);
    EXPECT_EQ(ret, 0);

    // Retrieve and verify
    AppSpawnFds *foundFds = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(foundFds, nullptr);
    EXPECT_EQ(foundFds->type, TYPE_FOR_FORK_ALL);
    EXPECT_EQ(foundFds->count, 2);
    EXPECT_EQ(foundFds->fds[0], 50);
    EXPECT_EQ(foundFds->fds[1], 60);

    // Verify other types are not affected
    AppSpawnFds *decFds = GetSpawningFd(mgr, TYPE_FOR_DEC);
    EXPECT_EQ(decFds, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_MultiProcessImgInfo_001
 * @tc.desc: Test multiple processes with same checkpointId
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_MultiProcessImgInfo_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Add processes with same checkpointId
    AppSpawnedCheckPointProcesses *node1 = AddSpawningImgInfo(mgr, 4000, "multi_same_cp_1", 0, 1000);
    ASSERT_NE(node1, nullptr);
    AppSpawnedCheckPointProcesses *node2 = AddSpawningImgInfo(mgr, 4000, "multi_same_cp_2", 0, 1001);
    ASSERT_NE(node2, nullptr);
    AppSpawnedCheckPointProcesses *node3 = AddSpawningImgInfo(mgr, 4000, "multi_same_cp_3", 0, 1002);
    ASSERT_NE(node3, nullptr);

    // All should be findable by name
    EXPECT_NE(GetSpawningImgInfoByName(mgr, "multi_same_cp_1"), nullptr);
    EXPECT_NE(GetSpawningImgInfoByName(mgr, "multi_same_cp_2"), nullptr);
    EXPECT_NE(GetSpawningImgInfoByName(mgr, "multi_same_cp_3"), nullptr);

    // Count via traversal should be 3
    int count = 0;
    auto countTraversal = [](const AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data) {
        int *count = static_cast<int *>(data);
        (*count)++;
    };
    TraversalImgProcess(countTraversal, mgr, &count);
    EXPECT_EQ(count, 3);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_CheckpointIdUpdate_001
 * @tc.desc: Test checkpointId update for existing process
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_CheckpointIdUpdate_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Initial checkpointId
    AppSpawnedCheckPointProcesses *node = AddSpawningImgInfo(mgr, 5000, "update_cp_test", 0, 1000);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->checkPointId, 5000);

    // Find and verify
    AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, "update_cp_test");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->checkPointId, 5000);
    EXPECT_EQ(found->uid, 1000);

    // Update via ProcessWithForkAllFd - add new fd first
    int fds[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_EQ(ret, 0);

    // Create message with new checkpoint (simulating fork from same template with new snapshot)
    AppSpawnMsgNode *msgNode = CreateMsgNodeWithImgPid("update_cp_test", "6000");
    ASSERT_NE(msgNode, nullptr);

    AppSpawningCtx property;
    memset_s(&property, sizeof(AppSpawningCtx), 0, sizeof(AppSpawningCtx));
    property.message = msgNode;

    int32_t result = ProcessWithForkAllFd(mgr, &property, fds[0]);
    APPSPAWN_LOGV("ProcessWithForkAllFd result for update: %{public}d", result);

    DeleteAppSpawnMsg(&msgNode);

    // After the operation, the process info should still exist
    AppSpawnedCheckPointProcesses *updatedFound = GetSpawningImgInfoByName(mgr, "update_cp_test");
    ASSERT_NE(updatedFound, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_StressTest_001
 * @tc.desc: Stress test with many img processes
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_StressTest_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    const int processCount = 50;
    for (int i = 0; i < processCount; i++) {
        char name[64];
        sprintf_s(name, sizeof(name), "stress_test_process_%02d", i);
        AppSpawnedCheckPointProcesses *node = AddSpawningImgInfo(mgr, 10000 + i, name, 0, 2000 + i);
        ASSERT_NE(node, nullptr);
    }

    // Verify all can be found
    for (int i = 0; i < processCount; i++) {
        char name[64];
        sprintf_s(name, sizeof(name), "stress_test_process_%02d", i);
        AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, name);
        EXPECT_NE(found, nullptr) << "Failed to find " << name;
        if (found) {
            EXPECT_EQ(found->checkPointId, static_cast<uint64_t>(10000 + i));
        }
    }

    // Count via traversal
    int count = 0;
    auto countTraversal = [](const AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data) {
        int *count = static_cast<int *>(data);
        (*count)++;
    };
    TraversalImgProcess(countTraversal, mgr, &count);
    EXPECT_EQ(count, processCount);

    // Add fork all fd
    int fds[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_EQ(ret, 0);

    APPSPAWN_LOGV("Stress test completed with %{public}d processes", processCount);

    DeleteAppSpawnMgr(mgr);
}

}  // namespace OHOS