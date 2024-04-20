/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "appspawn_client.h"
#include "appspawn_manager.h"
#include "appspawn_mount_permission.h"
#include "appspawn_utils.h"
#include "json_utils.h"
#include "parameter.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

#define TEST_PID 100
namespace OHOS {
static AppSpawnTestHelper g_testHelper;
class AppSpawnClientTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief 测试服务端报文不完整，关闭连接。客户端超时
 * 消息测试时，不能在中间执行assert，必须执行stop后执行assert检查，否则会导致消息异常
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Communication_001, TestSize.Level0)
{
    OHOS::AppSpawnTestServer testServer("appspawn -mode appspawn", false);
    testServer.Start([](TestConnection *connection, const uint8_t *buffer, uint32_t buffLen) {
        LE_CloseStreamTask(LE_GetDefaultLoop(), connection->stream);
    });
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    } while (0);
    testServer.Stop();
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, APPSPAWN_TIMEOUT);
}

/**
 * @brief 测试收到报文后，消息错误，直接回复
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Communication_002, TestSize.Level0)
{
    OHOS::AppSpawnTestServer testServer("appspawn -mode appspawn", false);
    testServer.Start(
        [](TestConnection *connection, const uint8_t *buffer, uint32_t buffLen) {
            connection->SendResponse(
                reinterpret_cast<AppSpawnMsg *>(const_cast<uint8_t *>(buffer)), APPSPAWN_MSG_INVALID, 0);
        },
        3000);  // 3000 3s
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnResult result = {};
        (void)AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        ret = result.result;
    } while (0);
    testServer.Stop();
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, APPSPAWN_MSG_INVALID);
}

/**
 * @brief 测试多线程报文发送
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Communication_003, TestSize.Level0)
{
    OHOS::AppSpawnTestServer testServer("appspawn -mode appspawn");
    testServer.Start(nullptr);
    AppSpawnClientHandle clientHandle = nullptr;
    auto sendMsg = [&clientHandle]() {
        int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        ASSERT_EQ(ret, 0);
        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(result.result, 0);
        ASSERT_NE(result.pid, 0);
        if (result.pid > 0) {
            kill(result.pid, SIGKILL);
        }
    };

    std::thread thread1(sendMsg);
    std::thread thread2(sendMsg);
    std::thread thread3(sendMsg);
    thread1.join();
    thread3.join();
    thread2.join();

    testServer.Stop();
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试收到报文后，不回复，消息超时
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Communication_004, TestSize.Level0)
{
    OHOS::AppSpawnTestServer testServer("appspawn -mode appspawn", false);
    testServer.Start(
        [](TestConnection *connection, const uint8_t *buffer, uint32_t buffLen) {
        },
        3000);  // 3000 3s
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnResult result = {};
        (void)AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        ret = result.result;
    } while (0);
    testServer.Stop();
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, APPSPAWN_TIMEOUT);
}

/**
 * @brief 测试消息构建及解析，msg flags
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.example.myapplication", &reqHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        // flags test
        const uint32_t testFlags[] = {10, 20, 31, 32, 34};
        uint32_t max = sizeof(testFlags) / sizeof(testFlags[0]);
        for (size_t i = 0; i < max; i++) {
            ret = AppSpawnReqMsgSetAppFlag(reqHandle, static_cast<AppFlagsIndex>(testFlags[i]));
            ASSERT_EQ(ret, 0);
        }
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, MAX_FLAGS_INDEX);
        ASSERT_NE(ret, 0);

        ret = APPSPAWN_ARG_INVALID;
        AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
        APPSPAWN_CHECK(reqNode != nullptr, break, "Invalid reqNode");
        APPSPAWN_CHECK(reqNode->msgFlags != nullptr, break, "Invalid reqNode");
        uint32_t maxUnit = (MAX_FLAGS_INDEX % 32) == 0 ? MAX_FLAGS_INDEX / 32 : MAX_FLAGS_INDEX / 32 + 1;  // 32 bits
        APPSPAWN_CHECK(reqNode->msgFlags->count == maxUnit,
            break, "Invalid reqNode %{public}d", reqNode->msgFlags->count);

        for (size_t i = 0; i < max; i++) {
            uint32_t index = testFlags[i] / 32;      // 32 bits
            uint32_t bits = 1 << testFlags[i] % 32;  // 32 bits
            APPSPAWN_LOGV("AppSpawnClientTest index %{public}u bits 0x%{public}x", index, bits);
            uint32_t result = (reqNode->msgFlags->flags[index] & bits) == bits;
            ASSERT_EQ(result == 1, 1);
        }
        ret = 0;
    } while (0);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试消息构建及解析，dac 测试
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_002, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        void *tlvValue = GetAppProperty(property, TLV_DAC_INFO);
        AppDacInfo *info = static_cast<AppDacInfo *>(tlvValue);
        APPSPAWN_CHECK(info != nullptr, break, "Can not find dac info in msg");
        APPSPAWN_CHECK(info->uid == g_testHelper.GetTestUid(), break, "Invalid uid %{public}d", info->uid);
        APPSPAWN_CHECK(info->gid == g_testHelper.GetTestGid(), break, "Invalid gid %{public}d", info->gid);
        APPSPAWN_CHECK(info->gidCount == 2, break, "Invalid gidCount %{public}d", info->gidCount);  // 2 default
        APPSPAWN_CHECK(info->gidTable[1] == g_testHelper.GetTestGidGroup() + 1,
            break, "Invalid uid %{public}d", info->gidTable[1]);
        ret = 0;
    } while (0);
    ASSERT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试消息构建及解析，bundle name 测试
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_003, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        std::string processName = "test222222222222222234512345678test222222222222222234512345678"
            "test222222222222222234512345678test222222222222222234512345678"
            "test222222222222222234512345678test222222222222222234512345678"
            "test222222222222222234512345678test2222222222222222345123456781234567";
        g_testHelper.SetProcessName(processName.c_str());
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        void *tlvValue = GetAppProperty(property, TLV_BUNDLE_INFO);
        AppSpawnMsgBundleInfo *info = static_cast<AppSpawnMsgBundleInfo *>(tlvValue);
        APPSPAWN_CHECK(info != nullptr, break, "Can not find dac info in msg");
        APPSPAWN_CHECK(info->bundleIndex == g_testHelper.GetTestBundleIndex(),
            break, "Invalid bundleIndex %{public}d", info->bundleIndex);
        APPSPAWN_LOGV("info->bundleName %{public}s", info->bundleName);
        APPSPAWN_CHECK(strcmp(info->bundleName, processName.c_str()) == 0,
            break, "Invalid bundleName %{public}s", info->bundleName);
        ret = 0;
    } while (0);
    ASSERT_EQ(ret, 0);
    g_testHelper.SetDefaultTestData();
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试消息构建及解析，render cmd
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_004, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        // save render cmd to req
        const char *renderCmd = "test 222222222222222222222222222222222222222222222222 \
            222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222 \
            333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333334456789";
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD, renderCmd);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add render cmd %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        DumpAppSpawnMsg(property->message);
        uint32_t len = 0;
        char *renderCmdMsg = reinterpret_cast<char *>(GetAppPropertyExt(property, MSG_EXT_NAME_RENDER_CMD, &len));
        APPSPAWN_CHECK(renderCmdMsg != nullptr, break, "Can not find render cmd in msg");
        APPSPAWN_LOGV("info->bundleName %{public}s", renderCmdMsg);
        APPSPAWN_CHECK(strcmp(renderCmdMsg, renderCmd) == 0,
            break, "Invalid renderCmd %{public}s", renderCmd);
        ret = 0;
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试消息构建及解析，ownerId cmd
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_005, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        // save owner to req
        const char *ownerId = "test 2222222222222222222222222222222222222222222222223451234567";
        ret = AppSpawnReqMsgSetAppOwnerId(reqHandle, ownerId);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add owner %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        void *tlvValue = GetAppProperty(property, TLV_OWNER_INFO);
        AppSpawnMsgOwnerId *owner = static_cast<AppSpawnMsgOwnerId *>(tlvValue);
        APPSPAWN_CHECK(owner != nullptr, break, "Can not find owner cmd in msg");
        APPSPAWN_LOGV("owner->ownerId %{public}s", owner->ownerId);
        APPSPAWN_CHECK(strcmp(owner->ownerId, ownerId) == 0, break, "Invalid ownerId %{public}s", ownerId);
        ret = 0;
    } while (0);
    ASSERT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试消息构建及解析，internet permission
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_006, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 101, 102);  // 101 102 test
        APPSPAWN_CHECK(ret == 0, break, "Failed to add owner %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        void *tlvValue = GetAppProperty(property, TLV_INTERNET_INFO);
        AppSpawnMsgInternetInfo *interInfo = static_cast<AppSpawnMsgInternetInfo *>(tlvValue);
        APPSPAWN_CHECK(interInfo != nullptr, break, "Can not find interInfo in msg");
        APPSPAWN_CHECK(102 == interInfo->setAllowInternet, // 102 test
            break, "Invalid setAllowInternet %{public}d", interInfo->setAllowInternet);
        APPSPAWN_CHECK(101 == interInfo->allowInternet, // 101 test
            break, "Invalid allowInternet %{public}d", interInfo->allowInternet);
        ret = 0;
    } while (0);
    ASSERT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试消息构建及解析，domain info
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_007, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    const char *apl = "test222222222222222234512345678";
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetAppDomainInfo(reqHandle, 1, apl);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add domain %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        void *tlvValue = GetAppProperty(property, TLV_DOMAIN_INFO);
        AppSpawnMsgDomainInfo *domainInfo = static_cast<AppSpawnMsgDomainInfo *>(tlvValue);
        APPSPAWN_CHECK(domainInfo != nullptr, break, "Can not find owner cmd in msg");
        APPSPAWN_CHECK(1 == domainInfo->hapFlags, break, "Invalid hapFlags %{public}d", domainInfo->hapFlags);
        APPSPAWN_LOGV("Test apl: %{public}s", domainInfo->apl);
        APPSPAWN_CHECK(strcmp(domainInfo->apl, apl) == 0, break, "Invalid apl %{public}s", domainInfo->apl);
        ret = 0;
    } while (0);
    ASSERT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试消息构建及解析，access token
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_008, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);  // 12345678
        APPSPAWN_CHECK(ret == 0, break, "Failed to add access token %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        void *tlvValue = GetAppProperty(property, TLV_ACCESS_TOKEN_INFO);
        AppSpawnMsgAccessToken *tokenInfo = static_cast<AppSpawnMsgAccessToken *>(tlvValue);
        APPSPAWN_CHECK(tokenInfo != nullptr, break, "Can not find owner cmd in msg");
        APPSPAWN_CHECK(12345678 == tokenInfo->accessTokenIdEx, break, "Invalid accessTokenIdEx");
        ret = 0;
    } while (0);
    ASSERT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试消息构建及解析，测试扩展tlv
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_009, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    const char *tlvName = "tlv-name-2";
    const uint32_t testDataLen = 10;  // 10
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<char> testData(testDataLen, '1');
        testData.push_back('1');
        testData.push_back('2');
        testData.push_back('3');
        testData.push_back('4');
        testData.push_back('5');
        testData.push_back('6');
        testData.push_back('7');
        testData.push_back('8');
        testData.push_back('9');
        testData.push_back('\0');
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, tlvName,
            reinterpret_cast<uint8_t *>(const_cast<char *>(testData.data())), testData.size());
        APPSPAWN_CHECK(ret == 0, break, "Failed to ext tlv %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);  // 12345678
        APPSPAWN_CHECK(ret == 0, break, "Failed to add access token %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        uint32_t tlvLen = 0;
        uint8_t *tlvValue = reinterpret_cast<uint8_t *>(GetAppPropertyExt(property, tlvName, &tlvLen));
        APPSPAWN_CHECK(tlvValue != nullptr, break, "Can not find tlv in msg");
        APPSPAWN_CHECK(tlvLen == testData.size(), break, "Invalid tlv len %{public}u", tlvLen);
        APPSPAWN_CHECK(strncmp(reinterpret_cast<char *>(tlvValue), testData.data(), testData.size()) == 0,
            break, "Invalid ext tlv %{public}s ", reinterpret_cast<char *>(tlvValue + testDataLen));
        ret = 0;
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试消息构建及解析，测试扩展tlv-超长
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_010, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    const char *tlvName = "tlv-name-3";
    const uint32_t testDataLen = 7416;  // 7416
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<char> testData(testDataLen, '1');
        testData.push_back('1');
        testData.push_back('2');
        testData.push_back('3');
        testData.push_back('4');
        testData.push_back('5');
        testData.push_back('6');
        testData.push_back('7');
        testData.push_back('8');
        testData.push_back('9');
        testData.push_back('\0');
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, tlvName,
            reinterpret_cast<uint8_t *>(const_cast<char *>(testData.data())), testData.size());
        APPSPAWN_CHECK(ret == 0, break, "Failed to ext tlv %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);  // 12345678
        APPSPAWN_CHECK(ret == 0, break, "Failed to add access token %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        uint32_t tlvLen = 0;
        uint8_t *tlvValue = reinterpret_cast<uint8_t *>(GetAppPropertyExt(property, tlvName, &tlvLen));
        APPSPAWN_CHECK(tlvValue != nullptr, break, "Can not find tlv in msg");
        APPSPAWN_CHECK(tlvLen == testData.size(), break, "Invalid tlv len %{public}u", tlvLen);
        APPSPAWN_CHECK(strncmp(reinterpret_cast<char *>(tlvValue), testData.data(), testData.size()) == 0,
            break, "Invalid ext tlv %{public}s ", reinterpret_cast<char *>(tlvValue + testDataLen));
        ret = 0;
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试消息构建及解析，测试扩展tlv-最大
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_011, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    const char *tlvName = "tlv-name-4";
    const uint32_t testDataLen = EXTRAINFO_TOTAL_LENGTH_MAX - 10;  // EXTRAINFO_TOTAL_LENGTH_MAX - 10 最大扩展
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<char> testData(testDataLen, '1');
        testData.push_back('1');
        testData.push_back('2');
        testData.push_back('3');
        testData.push_back('4');
        testData.push_back('5');
        testData.push_back('6');
        testData.push_back('7');
        testData.push_back('8');
        testData.push_back('9');
        testData.push_back('\0');
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, tlvName,
            reinterpret_cast<uint8_t *>(const_cast<char *>(testData.data())), testData.size());
        APPSPAWN_CHECK(ret == 0, break, "Failed to ext tlv %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);  // 12345678
        APPSPAWN_CHECK(ret == 0, break, "Failed to add access token %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        uint32_t tlvLen = 0;
        uint8_t *tlvValue = reinterpret_cast<uint8_t *>(GetAppPropertyExt(property, tlvName, &tlvLen));
        APPSPAWN_CHECK(tlvValue != nullptr, break, "Can not find tlv in msg");
        APPSPAWN_CHECK(tlvLen == testData.size(), break, "Invalid tlv len %{public}u", tlvLen);
        APPSPAWN_CHECK(strncmp(reinterpret_cast<char *>(tlvValue), testData.data(), testData.size()) == 0,
            break, "Invalid ext tlv %{public}s ", reinterpret_cast<char *>(tlvValue + testDataLen));
        ret = 0;
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试消息构建及解析，permission flags
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_012, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.example.myapplication", &reqHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        // flags test
        int max = GetMaxPermissionIndex();
        for (int i = 0; i < max; i++) {
            ret = AppSpawnReqMsgAddPermission(reqHandle, GetPermissionByIndex(i));
            ASSERT_EQ(ret, 0);
        }
        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        ret = 0;
        for (size_t i = 0; i < max; i++) {
            if (!CheckAppPermissionFlagSet(property, (uint32_t)i)) {
                APPSPAWN_LOGE("Invalid permission not set %{public}d %{public}s", i, GetPermissionByIndex(i));
                ret = APPSPAWN_ARG_INVALID;
                break;
            }
        }
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试消息构建及解析，测试terminate msg
 *
 */
HWTEST(AppSpawnClientTest, App_Client_Msg_013, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnTerminateMsgCreate(100, &reqHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);  // 12345678
        ASSERT_NE(ret, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        ASSERT_NE(result.result, 0);
    } while (0);
    AppSpawnClientDestroy(clientHandle);
}
}  // namespace OHOS
