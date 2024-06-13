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

static AppSpawnReqMsgHandle CreateMsg(AppSpawnClientHandle handle, const char *bundleName)
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

        ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);  // 12345678
        APPSPAWN_CHECK(ret == 0, break, "Failed to add access token %{public}s", APPSPAWN_SERVER_NAME);

        static const char *permissions[] = {
            "ohos.permission.READ_IMAGEVIDEO",
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

HWTEST(AppSpawnClientTest, AppSpawn_Client_test001, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = CreateClient(APPSPAWN_SERVER_NAME);
    ASSERT_EQ(clientHandle != NULL, 1);
    AppSpawnReqMsgHandle reqHandle = CreateMsg(clientHandle, "ohos.samples.clock");
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
