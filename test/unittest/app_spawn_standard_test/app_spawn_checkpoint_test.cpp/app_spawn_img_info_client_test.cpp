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

#include "appspawn.h"
#include "appspawn_utils.h"
#include "appspawn_client.h"
#include "securec.h"
#include "appspawn_server.h"

#include <gtest/gtest.h>

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace AppSpawn {

/**
 * @brief 镜像启动客户端接口测试
 * 测试 AppSpawnClientSendImgSpawnMsg 和 AppSpawnReqMsgSetImgInfo 接口
 */
class AppSpawnImgInfoClientTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// Helper function to create a message with img info for client testing
static AppSpawnReqMsgHandle CreateClientImgInfoMsg(
    AppSpawnClientHandle handle, const char *processName, AppSpawnMsgType msgType,
    pid_t imgPid, uint64_t checkPointId)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(msgType, processName, &reqHandle);
    APPSPAWN_CHECK(ret == 0, return INVALID_REQ_HANDLE, "Failed to create req %{public}s", processName);

    // Set bundle info
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, processName);
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to set bundle info for %{public}s, ret=%{public}d", processName, ret);
        AppSpawnReqMsgFree(reqHandle);
        return INVALID_REQ_HANDLE;
    }

    // Set img info
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, imgPid, checkPointId);
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to set img info for %{public}s, ret=%{public}d", processName, ret);
        AppSpawnReqMsgFree(reqHandle);
        return INVALID_REQ_HANDLE;
    }

    return reqHandle;
}

/**
 * @tc.name: App_ImgInfoClient_SendImgSpawnMsg_001
 * @tc.desc: Test AppSpawnClientSendImgSpawnMsg with MSG_GET_APP_IMG_INFO
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SendImgSpawnMsg_001, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(clientHandle, nullptr);

    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "com.test.imgapp", &reqHandle);
    ASSERT_EQ(ret, 0);

    // Set img info
    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 99999;
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    // Send img spawn message
    AppSpawnImgInfoResult result = {0};
    ret = AppSpawnClientSendImgSpawnMsg(clientHandle, reqHandle, &result);
    APPSPAWN_LOGV("AppSpawnClientSendImgSpawnMsg ret=%{public}d, pid=%{public}d, checkPointId=%{public}lu",
        ret, result.pid, result.checkPointId);

    // In unit test without running server, expect failure
    // Integration tests verify actual functionality
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: App_ImgInfoClient_SendImgSpawnMsg_002
 * @tc.desc: Test AppSpawnClientSendImgSpawnMsg with MSG_APP_FORK_ALL
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SendImgSpawnMsg_002, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(clientHandle, nullptr);

    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, "com.test.forkapp", &reqHandle);
    ASSERT_EQ(ret, 0);

    // Set img info
    pid_t testImgPid = 54321;
    uint64_t testCheckPointId = 111111;
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    // Send img spawn message
    AppSpawnImgInfoResult result = {0};
    ret = AppSpawnClientSendImgSpawnMsg(clientHandle, reqHandle, &result);
    APPSPAWN_LOGV("AppSpawnClientSendImgSpawnMsg MSG_APP_FORK_ALL ret=%{public}d", ret);

    // In unit test without running server, expect failure
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: App_ImgInfoClient_SendImgSpawnMsg_003
 * @tc.desc: Test AppSpawnClientSendImgSpawnMsg with invalid handle
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SendImgSpawnMsg_003, TestSize.Level0)
{
    AppSpawnImgInfoResult result = {0};
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "com.test.invalid", &reqHandle);
    ASSERT_EQ(ret, 0);

    // Test with null client handle
    ret = AppSpawnClientSendImgSpawnMsg(nullptr, reqHandle, &result);
    EXPECT_NE(ret, 0);
    printf("reqHandle: %p\n", reqHandle);
}

/**
 * @tc.name: App_ImgInfoClient_SendImgSpawnMsg_004
 * @tc.desc: Test AppSpawnClientSendImgSpawnMsg with invalid reqHandle
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SendImgSpawnMsg_004, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);

    AppSpawnImgInfoResult result = {0};

    // Test with null reqHandle
    ret = AppSpawnClientSendImgSpawnMsg(clientHandle, nullptr, &result);
    EXPECT_NE(ret, 0);

    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: App_ImgInfoClient_SendImgSpawnMsg_005
 * @tc.desc: Test AppSpawnClientSendImgSpawnMsg with INVALID_REQ_HANDLE
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SendImgSpawnMsg_005, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);

    AppSpawnImgInfoResult result = {0};

    // Test with INVALID_REQ_HANDLE
    ret = AppSpawnClientSendImgSpawnMsg(clientHandle, INVALID_REQ_HANDLE, &result);
    EXPECT_NE(ret, 0);

    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: App_ImgInfoClient_SetImgInfo_001
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo basic functionality
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SetImgInfo_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "com.test.imgset", &reqHandle);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 99999;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfoClient_SetImgInfo_002
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with zero imgPid
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SetImgInfo_002, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "com.test.zeropid", &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t zeroImgPid = 0;
    uint64_t testCheckPointId = 99999;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, zeroImgPid, testCheckPointId);
    EXPECT_NE(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfoClient_SetImgInfo_003
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with zero checkPointId
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SetImgInfo_003, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "com.test.zerocp", &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = 12345;
    uint64_t zeroCheckPointId = 0;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, zeroCheckPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfoClient_SetImgInfo_004
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with null handle
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SetImgInfo_004, TestSize.Level0)
{
    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 99999;

    int ret = AppSpawnReqMsgSetImgInfo(nullptr, testImgPid, testCheckPointId);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: App_ImgInfoClient_SetImgInfo_005
 * @tc.desc: Test AppSpawnReqMsgSetImgInfo with maximum values
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_SetImgInfo_005, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "com.test.maxvalues", &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = 99999;
    uint64_t maxCheckPointId = 0xFFFFFFFFFFFFFFFF;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, maxCheckPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfoClient_MsgWithImgInfo_001
 * @tc.desc: Test MSG_GET_APP_IMG_INFO message creation with img info
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_MsgWithImgInfo_001, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);

    // Create message with all necessary info
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "com.test.msgwithimg", &reqHandle);
    ASSERT_EQ(ret, 0);

    // Set bundle info
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, "com.test.msgwithimg");
    EXPECT_EQ(ret, 0);

    // Set img info
    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 88888;
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    // Set application flags (SPAWN_IMG_INFO)
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_SPAWN_IMG_INFO);
    EXPECT_EQ(ret, 0);

    // Free resources
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: App_ImgInfoClient_MsgWithImgInfo_002
 * @tc.desc: Test MSG_APP_FORK_ALL message creation with img info
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_MsgWithImgInfo_002, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);

    // Create message with all necessary info
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    ret = AppSpawnReqMsgCreate(MSG_APP_FORK_ALL, "com.test.forkwithimg", &reqHandle);
    ASSERT_EQ(ret, 0);

    // Set bundle info
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, "com.test.forkwithimg");
    EXPECT_EQ(ret, 0);

    // Set img info
    pid_t testImgPid = 54321;
    uint64_t testCheckPointId = 77777;
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    // Free resources
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: App_ImgInfoClient_DestroyTest_001
 * @tc.desc: Test client destroy after img spawn operations
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_DestroyTest_001, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);

    // Create and send message
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, "com.test.cleanup", &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = 99999;
    uint64_t testCheckPointId = 123456;
    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    // Destroy client without sending (clean up test)
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);

    // Verify no crash and resources cleaned
    SUCCEED();
}

/**
 * @tc.name: App_ImgInfoClient_MultipleMsgTest_001
 * @tc.desc: Test multiple img spawn messages from same client
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_MultipleMsgTest_001, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);

    const int msgCount = 5;
    AppSpawnReqMsgHandle handles[msgCount];

    for (int i = 0; i < msgCount; i++) {
        char bundleName[64];
        sprintf_s(bundleName, sizeof(bundleName), "com.test.multi.%d", i);

        ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, bundleName, &handles[i]);
        EXPECT_EQ(ret, 0);

        if (ret == 0) {
            pid_t testImgPid = 10000 + i;
            uint64_t testCheckPointId = 20000 + i;
            ret = AppSpawnReqMsgSetImgInfo(handles[i], testImgPid, testCheckPointId);
            EXPECT_EQ(ret, 0);
        }
    }

    // Clean up all handles
    for (int i = 0; i < msgCount; i++) {
        if (handles[i] != INVALID_REQ_HANDLE) {
            AppSpawnReqMsgFree(handles[i]);
        }
    }

    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: App_ImgInfoClient_MessageTypeCombinations_001
 * @tc.desc: Test different message type combinations with img spawn
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_MessageTypeCombinations_001, TestSize.Level0)
{
    // Test creating reqHandle for each message type that supports img info
    AppSpawnMsgType supportedTypes[] = {
        MSG_GET_APP_IMG_INFO,
        MSG_APP_FORK_ALL
    };
    int typeCount = sizeof(supportedTypes) / sizeof(supportedTypes[0]);

    for (int i = 0; i < typeCount; i++) {
        AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
        char bundleName[64];
        sprintf_s(bundleName, sizeof(bundleName), "com.test.type.%d", supportedTypes[i]);

        int ret = AppSpawnReqMsgCreate(supportedTypes[i], bundleName, &reqHandle);
        EXPECT_EQ(ret, 0) << "Failed to create msg type " << supportedTypes[i];

        if (ret == 0) {
            pid_t testImgPid = 20000 + i;
            uint64_t testCheckPointId = 30000 + i;
            ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
            EXPECT_EQ(ret, 0) << "Failed to set img info for msg type " << supportedTypes[i];

            AppSpawnReqMsgFree(reqHandle);
        }
    }
}

/**
 * @tc.name: App_ImgInfoClient_StringProcessName_001
 * @tc.desc: Test img spawn with long process names
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_StringProcessName_001, TestSize.Level0)
{
    // Test with long process name
    std::string longProcessName = "com.very.long.application.name.that.exceeds.standard.length";
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;

    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, longProcessName.c_str(), &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 99999;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @tc.name: App_ImgInfoClient_StringProcessName_002
 * @tc.desc: Test img spawn with special characters in process name
 * @tc.type: FUNC
 * @tc.require: AR000H5J8I
 */
HWTEST_F(AppSpawnImgInfoClientTest, App_ImgInfoClient_StringProcessName_002, TestSize.Level0)
{
    // Test with process name containing numbers and dots
    std::string processName = "com.test.app.version_2.0.1";
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;

    int ret = AppSpawnReqMsgCreate(MSG_GET_APP_IMG_INFO, processName.c_str(), &reqHandle);
    ASSERT_EQ(ret, 0);

    pid_t testImgPid = 12345;
    uint64_t testCheckPointId = 99999;

    ret = AppSpawnReqMsgSetImgInfo(reqHandle, testImgPid, testCheckPointId);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

}  // namespace AppSpawn
}  // namespace OHOS