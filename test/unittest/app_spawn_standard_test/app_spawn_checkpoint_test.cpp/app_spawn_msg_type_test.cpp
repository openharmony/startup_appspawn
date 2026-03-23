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

APPSPAWN_STATIC void ProcessGetAppImgInfoMsg(
    AppSpawnConnection *connection, AppSpawnMsgNode *message, AppSpawnImgInfoResult *result);
APPSPAWN_STATIC void ProcessForkAllReqMsg(
    AppSpawnConnection *connection, AppSpawnMsgNode *message, AppSpawnResult *result);

#ifdef __cplusplus
}
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static AppSpawnTestHelper g_testHelper;

class AppSpawnMsgTypeTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief Test creating message with MSG_APP_FORK_ALL type
 * @note 验证MSG_APP_FORK_ALL消息类型创建
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Type_Fork_All_Create_001, TestSize.Level0)
{
    const char *processName = "com.test.forkapp";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    // Create MSG_APP_FORK_ALL request
    int ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, processName, &reqHandle);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(reqHandle, nullptr);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating message with MSG_GET_APP_IMG_INFO type
 * @note 验证MSG_GET_APP_IMG_INFO消息类型创建
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Type_Get_App_Img_Info_Create_001, TestSize.Level0)
{
    const char *processName = "com.test.imgapp";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    // Create MSG_GET_APP_IMG_INFO request
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(reqHandle, nullptr);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating message with invalid process name (empty string)
 * @note 验证空进程名创建消息失败
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Type_Invalid_Name_001, TestSize.Level0)
{
    const char *emptyName = "";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, emptyName, &reqHandle);
    // Empty string should fail validation
    EXPECT_NE(ret, 0);
}

/**
 * @brief Test AppSpawnReqMsgSetImgInfo with valid parameters
 * @note 验证设置镜像进程信息成功
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_Img_Info_Valid_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    // Create a message first
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(reqHandle, nullptr);

    // Set img info
    pid_t imgPid = 12345;
    uint64_t checkPointId = 9876543210ULL;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, imgPid, checkPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test AppSpawnReqMsgSetImgInfo with null handle
 * @note 验证空句柄设置镜像进程信息失败
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_Img_Info_Null_Handle_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    pid_t imgPid = 12345;
    uint64_t checkPointId = 9876543210ULL;

    int ret = AppSpawnReqMsgSetImgInfo(reqHandle, imgPid, checkPointId);
    EXPECT_NE(ret, 0);
}

/**
 * @brief Test AppSpawnReqMsgSetImgInfo with zero imgPid
 * @note 验证imgPid为0时设置失败
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_Img_Info_Zero_Pid_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, 0, 1000);
    EXPECT_NE(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test AppSpawnReqMsgSetImgInfo with zero checkPointId
 * @note 验证checkPointId为0时设置成功
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_Img_Info_Zero_Checkpoint_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, 12345, 0);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test message creation with APP_FLAGS_SPAWN_IMG_INFO flag
 * @note 验证设置镜像启动标志的消息创建
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Flags_Spawn_Img_Info_001, TestSize.Level0)
{
    const char *processName = "com.test.imgspawn";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set APP_FLAGS_SPAWN_IMG_INFO flag
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_SPAWN_IMG_INFO);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating MSG_APP_FORK_ALL with APP_FLAGS_SPAWN_IMG_INFO flag
 * @note 验证FORK_ALL消息配合镜像标志创建
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Fork_All_With_Img_Flag_001, TestSize.Level0)
{
    const char *processName = "com.test.forkwithimg";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set img info
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, 54321, 12345678);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test multiple img info settings on same message
 * @note 验证同一消息多次设置镜像信息
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Multi_Set_Img_Info_001, TestSize.Level1)
{
    const char *processName = "com.test.multiset";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // First set
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, 11111, 1000);
    EXPECT_EQ(ret, 0);

    // Second set - should update previous values
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, 22222, 2000);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @ Test creating bundle info for fork all message
 * @note 验证FORK_ALL消息设置bundle信息
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Fork_All_Bundle_Info_001, TestSize.Level0)
{
    const char *processName = "com.test.forkbundle";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set bundle info
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 1, "com.test.bundle");
    EXPECT_EQ(ret, 0);

    // Set img info
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, 33333, 3000);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test message creation with special process name characters
 * @note 验证特殊字符进程名处理
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Special_Chars_In_Name_001, TestSize.Level1)
{
    // Process names with numbers and dots (typical bundle format)
    const char *validNames[] = {
        "com.example.app123",
        "com.test.app_version2",
        "app.simple.name"
    };

    for (size_t i = 0; i < sizeof(validNames) / sizeof(validNames[0]); i++) {
        AppSpawnReqMsgHandle reqHandle = nullptr;
        int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, validNames[i], &reqHandle);
        EXPECT_EQ(ret, 0) << "Failed to create msg for: " << validNames[i];
        if (ret == 0) {
            AppSpawnReqMsgFree(reqHandle);
        }
    }
}

/**
 * @brief Test AppSpawnReqMsgSetAppFlag for all image-related flags
 * @note 验证镜像相关标志设置
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Img_Related_Flags_001, TestSize.Level1)
{
    const char *processName = "com.test.flags";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set SPAWN_IMG_INFO flag
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_SPAWN_IMG_INFO);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating MSG_APP_FORK_ALL without img info
 * @note 验证FORK_ALL消息不设置img信息时行为
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Fork_All_Without_Img_Info_001, TestSize.Level1)
{
    const char *processName = "com.test.forknoimg";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Do NOT set img info - this is the test scenario

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test message with MSG_GET_APP_IMG_INFO and missing img info
 * @note 验证GET_APP_IMG消息缺少镜像信息时的行为
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Get_Img_Missing_Info_001, TestSize.Level1)
{
    const char *processName = "com.test.getimgmiss";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Do not call AppSpawnReqMsgSetImgInfo - missing imgPid/checkPointId

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test combination of MSG_APP_FORK_ALL with APP_FLAGS_COLD_BOOT
 * @note 验证FORK_ALL与冷启动标志组合
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Fork_All_With_Cold_Boot_001, TestSize.Level1)
{
    const char *processName = "com.test.forkcold";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set cold boot flag (should be allowed)
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_COLD_BOOT);
    EXPECT_EQ(ret, 0);

    // Set img info
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, 99999, 88888);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test message type routing logic verification
 * @note 验证不同消息类型处理路径差异
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Type_Routing_Diff_001, TestSize.Level1)
{
    const char *processName = "com.test.routing";
    AppSpawnReqMsgHandle reqHandleForkAll = nullptr;
    AppSpawnReqMsgHandle reqHandleGetImg = nullptr;
    AppSpawnReqMsgHandle reqHandleSpawn = nullptr;

    // Create different types
    int ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, processName, &reqHandleForkAll);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandleGetImg);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, processName, &reqHandleSpawn);
    EXPECT_EQ(ret, 0);

    // Verify msgType field is different
    if (reqHandleForkAll != nullptr && reqHandleGetImg != nullptr) {
        AppSpawnReqMsgNode *node1 = (AppSpawnReqMsgNode *)reqHandleForkAll;
        AppSpawnReqMsgNode *node2 = (AppSpawnReqMsgNode *)reqHandleGetImg;
        EXPECT_NE(node1->msg->msgType, node2->msg->msgType);
    }

    AppSpawnReqMsgFree(reqHandleForkAll);
    AppSpawnReqMsgFree(reqHandleGetImg);
    AppSpawnReqMsgFree(reqHandleSpawn);
}

/**
 * @brief Test AppSpawnClientSendMsg with MSG_APP_FORK_ALL type
 * @note 验证客户端使用SendMsg接口发送FORK_ALL消息
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Client_Send_Msg_Fork_All_001, TestSize.Level2)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    // May fail if server not running, that's acceptable for unit test
    // Just verify the API can be called without crash

    if (ret == 0 && clientHandle != nullptr) {
        const char *processName = "com.test.clientfork";
        ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, processName, &reqHandle);
        EXPECT_EQ(ret, 0);

        if (ret == 0 && reqHandle != nullptr) {
            AppSpawnResult result = {0};
            // This will fail without server, but that's expected
            ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
            // Result depends on whether server is running
        }
    }

    if (clientHandle != nullptr) {
        AppSpawnClientDestroy(clientHandle);
    }
}

/**
 * @brief Test AppSpawnClientSendImgSpawnMsg interface availability
 * @note 验证AppSpawnClientSendImgSpawnMsg接口可用性
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Client_Send_Img_Spawn_Msg_001, TestSize.Level2)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    if (ret != 0 || clientHandle == nullptr) {
        // Can't test without client init
        GTEST_SKIP() << "Client init failed, skipping integration test";
    }

    const char *processName = "com.test.clientimgspawn";
    ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    if (ret == 0 && reqHandle != nullptr) {
        // Set required img info
        ret = AppSpawnReqMsgSetImgInfo(reqHandle, 12345, 1000);
        EXPECT_EQ(ret, 0);

        AppSpawnImgInfoResult result = {0, 0, 0};
        // Call the new interface
        ret = AppSpawnClientSendImgSpawnMsg(clientHandle, reqHandle, &result);
        // Expected to fail without real server
    }

    if (clientHandle != nullptr) {
        AppSpawnClientDestroy(clientHandle);
    }
}

/**
 * @brief Test ProcessGetAppImgInfoMsg interface execute STAGE_PARENT_BOOT_FROM_IMG stage success.
 * @note 验证 ProcessGetAppImgInfoMsg 接口可用性
 */
HWTEST_F(AppSpawnMsgTypeTest, Process_Get_App_Img_Info_Msg_001, TestSize.Level2)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawnImgInfoResult result = {0};
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_GET_APP_IMG_INFO, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgNode *reqNode = static_cast<AppSpawnReqMsgNode *>(reqHandle);
        APPSPAWN_CHECK(reqNode != nullptr && reqNode->msg != nullptr, AppSpawnReqMsgFree(reqHandle);
            break, "Invalid reqNode");

        AppSpawnMsgNode *msgNode = g_testHelper.CreateAppSpawnMsg(reqNode->msg);
        APPSPAWN_CHECK(msgNode != nullptr, break, "Failed to alloc for msg");

        AppSpawnConnection connection = {
            .connectionId = 1,
            .stream = nullptr,
            .receiverCtx = {0}
        };

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return 0;
        };
        UpdateIoctlFunc(ioctlFunc);
        ProcessGetAppImgInfoMsg(&connection, msgNode, &result);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief Test ProcessGetAppImgInfoMsg interface execute STAGE_PARENT_BOOT_FROM_IMG stage failed
 * @note 验证 ProcessGetAppImgInfoMsg 接口可用性
 */
HWTEST_F(AppSpawnMsgTypeTest, Process_Get_App_Img_Info_Msg_002, TestSize.Level2)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawnImgInfoResult result = {0};
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_GET_APP_IMG_INFO, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgNode *reqNode = static_cast<AppSpawnReqMsgNode *>(reqHandle);
        APPSPAWN_CHECK(reqNode != nullptr && reqNode->msg != nullptr, AppSpawnReqMsgFree(reqHandle);
            break, "Invalid reqNode");

        AppSpawnMsgNode *msgNode = g_testHelper.CreateAppSpawnMsg(reqNode->msg);
        APPSPAWN_CHECK(msgNode != nullptr, break, "Failed to alloc for msg");

        AppSpawnConnection connection = {
            .connectionId = 1,
            .stream = nullptr,
            .receiverCtx = {0}
        };

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return -1;
        };
        UpdateIoctlFunc(ioctlFunc);
        ProcessGetAppImgInfoMsg(&connection, msgNode, &result);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief Test ProcessForkAllReqMsg interface execute STAGE_PARENT_BOOT_FROM_IMG stage success.
 * @note 验证 ProcessForkAllReqMsg 接口可用性
 */
HWTEST_F(AppSpawnMsgTypeTest, Process_Fork_All_Req_Msg_001, TestSize.Level2)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawnResult result = {0};
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_FORK_ALL, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgNode *reqNode = static_cast<AppSpawnReqMsgNode *>(reqHandle);
        APPSPAWN_CHECK(reqNode != nullptr && reqNode->msg != nullptr, AppSpawnReqMsgFree(reqHandle);
            break, "Invalid reqNode");

        AppSpawnMsgNode *msgNode = g_testHelper.CreateAppSpawnMsg(reqNode->msg);
        APPSPAWN_CHECK(msgNode != nullptr, break, "Failed to alloc for msg");

        AppSpawnConnection connection = {
            .connectionId = 1,
            .stream = nullptr,
            .receiverCtx = {0}
        };

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return 0;
        };
        UpdateIoctlFunc(ioctlFunc);
        ProcessForkAllReqMsg(&connection, msgNode, &result);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief Test ProcessForkAllReqMsg interface execute STAGE_PARENT_BOOT_FROM_IMG stage failed.
 * @note 验证 ProcessForkAllReqMsg 接口可用性
 */
HWTEST_F(AppSpawnMsgTypeTest, Process_Fork_All_Req_Msg_002, TestSize.Level2)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawnResult result = {0};
    AppSpawningCtx *property = nullptr;
    do {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_FORK_ALL, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgNode *reqNode = static_cast<AppSpawnReqMsgNode *>(reqHandle);
        APPSPAWN_CHECK(reqNode != nullptr && reqNode->msg != nullptr, AppSpawnReqMsgFree(reqHandle);
            break, "Invalid reqNode");

        AppSpawnMsgNode *msgNode = g_testHelper.CreateAppSpawnMsg(reqNode->msg);
        APPSPAWN_CHECK(msgNode != nullptr, break, "Failed to alloc for msg");

        AppSpawnConnection connection = {
            .connectionId = 1,
            .stream = nullptr,
            .receiverCtx = {0}
        };

        IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
            return -1;
        };
        UpdateIoctlFunc(ioctlFunc);
        ProcessForkAllReqMsg(&connection, msgNode, &result);
        UpdateIoctlFunc(nullptr);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}
}   // namespace OHOS