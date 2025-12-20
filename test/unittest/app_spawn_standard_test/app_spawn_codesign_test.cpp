/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cJSON.h>

#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "appspawn_adapter.h"
#include "appspawn.h"
#include "appspawn_hook.h"
#include "hisysevent_adapter.h"
#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static AppSpawnTestHelper g_testHelper;
class AppSpawnCodeSignTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
    }
};

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_001, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetXpmConfig(mgr, property);
        EXPECT_EQ(ret, 0);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_002, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        const char *appSignType = "none";
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_APP_SIGN_TYPE, appSignType);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add appSignType %{public}s", APPSPAWN_SERVER_NAME);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetXpmConfig(mgr, property);
        EXPECT_EQ(ret, 0);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_003, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        const char *appSignType = "enterpriseResign";
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_APP_SIGN_TYPE, appSignType);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add appSignType %{public}s", APPSPAWN_SERVER_NAME);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetXpmConfig(mgr, property);
        EXPECT_EQ(ret, 0);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_004, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        const char *provisionType = "debug";
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PROVISION_TYPE, provisionType);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add provisionType %{public}s", APPSPAWN_SERVER_NAME);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetXpmConfig(mgr, property);
        EXPECT_EQ(ret, 0);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_005, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_TEMP_JIT);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetXpmConfig(mgr, property);
        EXPECT_EQ(ret, 0);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}
}  // namespace OHOS
