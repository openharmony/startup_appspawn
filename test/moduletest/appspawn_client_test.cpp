/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "securec.h"
#include "appspawn_server.h"

#include <gtest/gtest.h>

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace AppSpawn {
class AppSpawnClientTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

static AppSpawnReqMsgHandle CreateMsg(AppSpawnClientHandle handle, const char *bundleName, RunMode mode)
{
    AppSpawnReqMsgHandle reqHandle = 0;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, bundleName, &reqHandle);
    APPSPAWN_CHECK(ret == 0, return 0, "Failed to create req %{public}s", bundleName);
    do {
        ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, bundleName);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create req %{public}s", bundleName);

        AppDacInfo dacInfo = {};
        dacInfo.uid = 20010029;              // 20010029 test data
        dacInfo.gid = 20010029;              // 20010029 test data
        dacInfo.gidCount = 2;                // 2 count
        dacInfo.gidTable[0] = 20010029;      // 20010029 test data
        dacInfo.gidTable[1] = 20010029 + 1;  // 20010029 test data
        (void)strcpy_s(dacInfo.userName, sizeof(dacInfo.userName), "test-app-name");
        ret = AppSpawnReqMsgSetAppDacInfo(reqHandle, &dacInfo);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add dac %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgSetAppFlag(reqHandle, static_cast<AppFlagsIndex>(10));  // 10 test
        if (mode == MODE_FOR_NATIVE_SPAWN) {
            AppSpawnReqMsgSetAppFlag(reqHandle, static_cast<AppFlagsIndex>(23)); // 23 APP_FLAGS_ISOLATED_SANDBOX_TYPE
            AppSpawnReqMsgSetAppFlag(reqHandle, static_cast<AppFlagsIndex>(26)); // 26 APP_FLAGS_ISOLATED_NETWORK
        }

        const char *apl = "normal";
        ret = AppSpawnReqMsgSetAppDomainInfo(reqHandle, 1, apl);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add domain %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);  // 12345678
        APPSPAWN_CHECK(ret == 0, break, "Failed to add access token %{public}s", APPSPAWN_SERVER_NAME);

        static const char *permissions[] = {
            "ohos.permission.MANAGE_PRIVATE_PHOTOS",
            "ohos.permission.FILE_CROSS_APP",
            "ohos.permission.ACTIVATE_THEME_PACKAGE",
            "ohos.permission.GET_WALLPAPER",
        };
        size_t count = sizeof(permissions) / sizeof(permissions[0]);
        for (size_t i = 0; i < count; i++) {
            ret = AppSpawnReqMsgAddPermission(reqHandle, permissions[i]);
            APPSPAWN_CHECK(ret == 0, break, "Failed to create req %{public}s", bundleName);
        }
        return reqHandle;
    } while (0);
    AppSpawnReqMsgFree(reqHandle);
    return INVALID_REQ_HANDLE;
}

static AppSpawnClientHandle CreateClient(const char *serviceName)
{
    AppSpawnClientHandle clientHandle = NULL;
    int ret = AppSpawnClientInit(serviceName, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create client %{public}s", serviceName);
    return clientHandle;
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_test001, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = CreateClient(APPSPAWN_SERVER_NAME);
    ASSERT_EQ(clientHandle != NULL, 1);
    AppSpawnReqMsgHandle reqHandle = CreateMsg(clientHandle, "ohos.samples.clock", MODE_FOR_APP_SPAWN);
    ASSERT_EQ(reqHandle != INVALID_REQ_HANDLE, 1);

    AppSpawnResult result = {};
    int ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    if (ret == 0 && result.pid > 0) {
        kill(result.pid, SIGKILL);
    }
    AppSpawnClientDestroy(clientHandle);
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_test002, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = CreateClient(NATIVESPAWN_SERVER_NAME);
    ASSERT_EQ(clientHandle != NULL, 1);
    AppSpawnReqMsgHandle reqHandle = CreateMsg(clientHandle, "ohos.samples.clock", MODE_FOR_NATIVE_SPAWN);
    ASSERT_EQ(reqHandle != INVALID_REQ_HANDLE, 1);

    AppSpawnResult result = {};
    int ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    if (ret == 0 && result.pid > 0) {
        kill(result.pid, SIGKILL);
    }
    AppSpawnClientDestroy(clientHandle);
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_test003, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = CreateClient(APPSPAWN_SERVER_NAME);
    ASSERT_EQ(clientHandle != NULL, 1);

    AppSpawnReqMsgHandle reqHandle = 0;
    int ret = AppSpawnReqMsgCreate(MSG_UNINSTALL_DEBUG_HAP, "test.uninstall", &reqHandle);
    ASSERT_EQ(ret, 0);

    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 100, "test.uninstall");
    ASSERT_EQ(ret, 0);

    AppSpawnResult result = {};
    ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    if (ret == 0 && result.pid > 0) {
        kill(result.pid, SIGKILL);
    }
    AppSpawnClientDestroy(clientHandle);
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_test004, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = CreateClient(APPSPAWN_SERVER_NAME);
    ASSERT_EQ(clientHandle != NULL, 1);

    AppSpawnReqMsgHandle reqHandle = 0;
    int ret = AppSpawnReqMsgCreate(MSG_UNINSTALL_DEBUG_HAP, "test.uninstall", &reqHandle);
    ASSERT_EQ(ret, 0);

    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_USERID, "100");
    ASSERT_EQ(ret, 0);

    AppSpawnResult result = {};
    ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    if (ret == 0 && result.pid > 0) {
        kill(result.pid, SIGKILL);
    }
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: AppSpawn_Client_test005
 * @tc.desc: 模拟hybridspawn的客户端向hybridspawn服务端发送应用孵化请求
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnClientTest, AppSpawn_Client_test005, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = CreateClient(HYBRIDSPAWN_SERVER_NAME);
    ASSERT_EQ(clientHandle != NULL, 1);
    AppSpawnReqMsgHandle reqHandle = CreateMsg(clientHandle, "ohos.samples.clock", MODE_FOR_HYBRID_SPAWN);
    ASSERT_EQ(reqHandle != INVALID_REQ_HANDLE, 1);

    AppSpawnResult result = {};
    int ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    if (ret == 0 && result.pid > 0) {
        kill(result.pid, SIGKILL);
    }
    AppSpawnClientDestroy(clientHandle);
}

}  // namespace AppSpawn
}  // namespace OHOS
