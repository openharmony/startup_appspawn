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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unistd.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cJSON.h>

#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "appspawn_adapter.h"
#include "appspawn_encaps.h"
#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_permission.h"
#include "hisysevent_adapter.h"
#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"
#include "securec.h"

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
void SetHapDomainInfo(const AppSpawnMgr *content, const AppSpawningCtx *property,
    AppSpawnMsgDomainInfo *msgDomainInfo, HapDomainInfo *hapDomainInfo);
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static AppSpawnTestHelper g_testHelper;
class AppSpawnCommonTest : public testing::Test {
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

/**
 * @brief 非开发模式下appspawn不支持为MSG_SPAWN_NATIVE_PROCESS设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_01, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_SPAWN_NATIVE_PROCESS);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, APPSPAWN_NATIVE_NOT_SUPPORT);
}

/**
 * @brief 开发模式下appspawn不为MSG_SPAWN_NATIVE_PROCESS设置selinux标签，直接返回
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_02, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_SPAWN_NATIVE_PROCESS);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        property->client.flags |= APP_DEVELOPER_MODE;

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为不携带任何flag的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_03, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带APP_FLAGS_DEBUGGABLE的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_04, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_DEBUGGABLE), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带APP_FLAGS_DLP_MANAGER_FULL_CONTROL的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_05, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DLP_MANAGER_FULL_CONTROL);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_DLP_MANAGER_FULL_CONTROL), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带APP_FLAGS_DLP_MANAGER_READ_ONLY的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_06, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create reqMsg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DLP_MANAGER_READ_ONLY);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_DLP_MANAGER_READ_ONLY), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带APP_FLAGS_EXTENSION_SANDBOX和APP_FLAGS_ISOLATED_SANDBOX的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_07, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_EXTENSION_SANDBOX);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SANDBOX);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_EXTENSION_SANDBOX), 0);
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带APP_FLAGS_EXTENSION_SANDBOX但不携带APP_FLAGS_ISOLATED_SANDBOX的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_08, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_EXTENSION_SANDBOX);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_EXTENSION_SANDBOX), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带APP_FLAGS_CUSTOM_SANDBOX的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_09, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_CUSTOM_SANDBOX);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_CUSTOM_SANDBOX), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带APP_FLAGS_ISOLATED_SELINUX_LABEL的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSelinuxCon_10, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_EXTENSION_TYPE, "test");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_EXTENSION_TYPE %{public}s", "test");
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SELINUX_LABEL);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SELINUX_LABEL), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief nwebspawn为render进程设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, Nweb_Spawn_SetSelinuxCon_01, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NWEBSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE, "render");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_PROCESS_TYPE %{public}s", "render");

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief nwebspawn为gpu进程设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, Nweb_Spawn_SetSelinuxCon_02, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NWEBSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE, "gpu");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_PROCESS_TYPE %{public}s", "gpu");

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief nativespawn为isolated模式下hostId为null的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, Native_Spawn_SetSelinuxCon_01, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(NATIVESPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NATIVESPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SANDBOX_TYPE);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX_TYPE), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief nativespawn为isolated模式下hostId不为null的应用设置selinux标签
 *
 */
HWTEST_F(AppSpawnCommonTest, Native_Spawn_SetSelinuxCon_02, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(NATIVESPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NATIVESPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PARENT_UID, "100");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_PARENT_UID %{public}s", "100");
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SANDBOX_TYPE);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX_TYPE), 0);

        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_002, TestSize.Level0)
{
    int ret = SetAppAccessToken(nullptr, nullptr);
    EXPECT_NE(ret, 0);
}

/**
 * @brief appspawn为不携带任何flag的应用设置Seccomp Filter
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSeccompFilter_01, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetSeccompFilter(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带多个flag的应用设置Seccomp Filter
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSeccompFilter_02, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SANDBOX);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ATOMIC_SERVICE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ALLOW_IOURING);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX), 0);
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_ATOMIC_SERVICE), 0);
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_ALLOW_IOURING), 0);

        ret = SetSeccompFilter(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief nwebspawn为render进程设置Seccomp Filter，直接返回
 *
 */
HWTEST_F(AppSpawnCommonTest, Nweb_Spawn_SetSeccompFilter_01, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NWEBSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE, "render");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_PROCESS_TYPE %{public}s", "render");

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetSeccompFilter(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为应用设置Internet Permission [0, 0]
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetInternetPermission_01, TestSize.Level0)
{
    int ret = -1;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 0, 0);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add internet permission info");

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetInternetPermission(property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为应用设置Internet Permission [0, 1]
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetInternetPermission_02, TestSize.Level0)
{
    int ret = -1;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 0, 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add internet permission info");

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetInternetPermission(property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为应用设置Internet Permission [1, 0]
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetInternetPermission_03, TestSize.Level0)
{
    int ret = -1;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 1, 0);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add internet permission info");

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetInternetPermission(property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为应用设置Internet Permission [1, 1]
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetInternetPermission_04, TestSize.Level0)
{
    int ret = -1;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 1, 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add internet permission info");

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetInternetPermission(property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief property为NULL时，appspawn为应用设置Internet Permission
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetInternetPermission_05, TestSize.Level0)
{
    int ret = SetInternetPermission(nullptr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为应用设置env环境变量
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_SetEnvInfo_01, TestSize.Level0)
{
    int ret = -1;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        const char *appEnv = "{\"name1\": \"value1\", \"name2\": \"value2\"}";
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_APP_ENV, appEnv);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_APP_ENV %{public}s", appEnv);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SetEnvInfo(nullptr, property);
        EXPECT_EQ(strcmp(getenv("name1"), "value1"), 0);
        EXPECT_EQ(strcmp(getenv("name2"), "value2"), 0);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_013, TestSize.Level0)
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
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetFdEnv(nullptr, nullptr);
        ASSERT_EQ(ret, -1);
        ret = SetFdEnv(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, -1);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_014, TestSize.Level0)
{
    NsInitFunc();
    EXPECT_EQ(GetNsPidFd(-1), -1);
    GetPidByName("///////");
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_015, TestSize.Level0)
{
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    mgr->content.sandboxNsFlags = 0;
    PreLoadEnablePidNs(mgr);
    DeleteAppSpawnMgr(mgr);
    ASSERT_NE(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_016, TestSize.Level0)
{
    HOOK_MGR *hookMgr = GetAppSpawnHookMgr();
    EXPECT_EQ(hookMgr != nullptr, 1);
    DeleteAppSpawnHookMgr();
    AppSpawnHiSysEventWrite();

    AppSpawnHisyseventInfo *info = (AppSpawnHisyseventInfo *)calloc(1, sizeof(AppSpawnHisyseventInfo));
    EXPECT_EQ(info != nullptr, 1);
    ReportSpawnStatisticDuration(nullptr, (void *)info);
    ReportMountFull(getpid(), 0, 0, 0);
    free(info);
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_TRUE(mgr != nullptr);
    ReportMountFullHisysevent(APPSPAWN_SANDBOX_MOUNT_FULL);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_028, TestSize.Level0)
{
    ASSERT_EQ(GetAppSpawnNamespace(nullptr), nullptr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_029, TestSize.Level0)
{
    DeleteAppSpawnNamespace(nullptr);
    FreeAppSpawnNamespace(nullptr);
    AppSpawnMgr *mgr = nullptr;
    mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    AppSpawnNamespace *appSpawnNamespace = GetAppSpawnNamespace(mgr);
    DeleteAppSpawnNamespace(appSpawnNamespace);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(appSpawnNamespace == nullptr, 1);

}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_030, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    mgr->content.sandboxNsFlags = 1;
    ret = PreForkSetPidNamespace(mgr, nullptr);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_031, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    mgr->content.sandboxNsFlags = 0;
    ret = PostForkSetPidNamespace(mgr, nullptr);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
    AppSpawnClientInit(nullptr, nullptr);
}

#ifdef WITH_SELINUX
HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_032, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    HapDomainInfo hapDomainInfo;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE,
            reinterpret_cast<uint8_t *>(const_cast<char *>("render")), 7); // 7 is value len
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PARENT_UID, "123");
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_DEVELOPER_MODE;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        AppSpawnMsgDomainInfo *msgDomainInfo =
            reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(property, TLV_DOMAIN_INFO));
        APPSPAWN_CHECK(msgDomainInfo != NULL, break,
            "No domain info in req form %{public}s", GetProcessName(property));
        SetHapDomainInfo(mgr, property, msgDomainInfo, &hapDomainInfo);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(hapDomainInfo.uid, 123);
    EXPECT_EQ(hapDomainInfo.hapFlags & SELINUX_HAP_ISOLATED_RENDER, SELINUX_HAP_ISOLATED_RENDER);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_033, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    HapDomainInfo hapDomainInfo;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE,
            reinterpret_cast<uint8_t *>(const_cast<char *>("gpu")), 4); // 4 is value len
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PARENT_UID, "123");
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_DEVELOPER_MODE;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        AppSpawnMsgDomainInfo *msgDomainInfo =
            reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(property, TLV_DOMAIN_INFO));
        APPSPAWN_CHECK(msgDomainInfo != NULL, break,
            "No domain info in req form %{public}s", GetProcessName(property));
        SetHapDomainInfo(mgr, property, msgDomainInfo, &hapDomainInfo);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(hapDomainInfo.uid, 123);
    EXPECT_EQ(hapDomainInfo.hapFlags & SELINUX_HAP_ISOLATED_GPU, SELINUX_HAP_ISOLATED_GPU);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_034, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    HapDomainInfo hapDomainInfo;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PARENT_UID, "123");
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SANDBOX_TYPE);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_DEVELOPER_MODE;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        AppSpawnMsgDomainInfo *msgDomainInfo =
            reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(property, TLV_DOMAIN_INFO));
        APPSPAWN_CHECK(msgDomainInfo != NULL, break,
            "No domain info in req form %{public}s", GetProcessName(property));
        SetHapDomainInfo(mgr, property, msgDomainInfo, &hapDomainInfo);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(hapDomainInfo.uid, 123);
    EXPECT_EQ(hapDomainInfo.hapFlags & SELINUX_HAP_ISOLATED_RENDER, SELINUX_HAP_ISOLATED_RENDER);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_035, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    HapDomainInfo hapDomainInfo;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SANDBOX_TYPE);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_DEVELOPER_MODE;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        AppSpawnMsgDomainInfo *msgDomainInfo =
            reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(property, TLV_DOMAIN_INFO));
        APPSPAWN_CHECK(msgDomainInfo != NULL, break,
            "No domain info in req form %{public}s", GetProcessName(property));
        SetHapDomainInfo(mgr, property, msgDomainInfo, &hapDomainInfo);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(hapDomainInfo.uid, 0);
    EXPECT_EQ(hapDomainInfo.hapFlags & SELINUX_HAP_ISOLATED_RENDER, SELINUX_HAP_ISOLATED_RENDER);
    ASSERT_EQ(ret, 0);
}
#endif // WITH_SELINUX

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_GetAppSpawnNamespace, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnNamespace *appSpawnNamespace = nullptr;
    mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    appSpawnNamespace = CreateAppSpawnNamespace();
    appSpawnNamespace->nsSelfPidFd = GetNsPidFd(getpid());
    OH_ListInit(&appSpawnNamespace->extData.node);
    OH_ListAddTail(&mgr->extData, &appSpawnNamespace->extData.node);
    AppSpawnNamespace *appSpawnNamespace1 = nullptr;
    appSpawnNamespace1 = GetAppSpawnNamespace(mgr);
    EXPECT_EQ(appSpawnNamespace1 != nullptr, 1);
    uint32_t dataId = EXT_DATA_NAMESPACE;
    AppSpawnExtDataCompareDataId(&mgr->extData, (void *)&dataId);
    DeleteAppSpawnNamespace(appSpawnNamespace);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_GetPidByName, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnNamespace *appSpawnNamespace = nullptr;
    mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    appSpawnNamespace = CreateAppSpawnNamespace();
    OH_ListInit(&appSpawnNamespace->extData.node);
    OH_ListAddTail(&mgr->extData, &appSpawnNamespace->extData.node);
    appSpawnNamespace->nsInitPidFd = GetNsPidFd(getpid());
    pid_t pid = GetPidByName("appspawn");
    EXPECT_EQ(pid > 0, 1);
    DeleteAppSpawnNamespace(appSpawnNamespace);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_PreLoadEnablePidNs, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnNamespace *appSpawnNamespace = nullptr;
    mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    mgr->content.sandboxNsFlags = CLONE_NEWPID;
    ret = PreLoadEnablePidNs(mgr);
    EXPECT_EQ(ret, 0);
    AppSpawnNamespace *appSpawnNamespace1 = nullptr;
    appSpawnNamespace1 = GetAppSpawnNamespace(mgr);
    EXPECT_EQ(appSpawnNamespace1 != nullptr, 1);
    DeleteAppSpawnNamespace(appSpawnNamespace);
    DeleteAppSpawnMgr(mgr);
    SetPidNamespace(0, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_PreForkSetPidNamespace, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnNamespace *appSpawnNamespace = nullptr;
    mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    mgr->content.sandboxNsFlags = CLONE_NEWPID;
    ret = PreLoadEnablePidNs(mgr);
    EXPECT_EQ(ret, 0);
    PreForkSetPidNamespace(mgr, nullptr);
    PostForkSetPidNamespace(mgr, nullptr);
    mgr->content.sandboxNsFlags = 0;
    PreForkSetPidNamespace(mgr, nullptr);
    PostForkSetPidNamespace(mgr, nullptr);
    DeleteAppSpawnNamespace(appSpawnNamespace);
    DeleteAppSpawnMgr(mgr);
    SetPidNamespace(0, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_001, TestSize.Level0)
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
        const char *permissions = "{\"name\":\"Permissions\",\"ohos.encaps.count\":6,\"permissions\":"
            "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},{\"ohos.permission.string\":\"nihaoma\"},"
            "{\"ohos.permission.strarray\":[\"abc\",\"def\"]},{\"ohos.permission.intarray\":[1,2,3,4,5]},"
            "{\"ohos.permission.boolarray\":[true,false,true]}]}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_JIT_PERMISSIONS,
            reinterpret_cast<uint8_t *>(const_cast<char *>(permissions)), strlen(permissions) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add permissions");
        const char *maxChildProcess = "512";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX,
            reinterpret_cast<uint8_t *>(const_cast<char *>(maxChildProcess)), strlen(maxChildProcess) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add maxChildProcess");
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        ret = SpawnSetEncapsPermissions(NULL, property);
        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

        ret = SpawnSetEncapsPermissions(mgr, NULL);
        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

        ret = SpawnSetEncapsPermissions(mgr, property);
    } while (0);

    EXPECT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_002, TestSize.Level0)
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
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SpawnSetEncapsPermissions(mgr, property);
    } while (0);

    EXPECT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_003, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    UserEncaps encapsInfo = {0};
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        const char *permissions = "{\"name\":\"Permissions\",\"ohos.encaps.count\":0,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},"
        "{\"ohos.permission.string\":\"{\\\"key\\\":\\\"value\\\"}\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_JIT_PERMISSIONS,
            reinterpret_cast<uint8_t *>(const_cast<char *>(permissions)), strlen(permissions) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add permissions");
        const char *maxChildProcess = "512";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX,
            reinterpret_cast<uint8_t *>(const_cast<char *>(maxChildProcess)), strlen(maxChildProcess) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add maxChildProcess");
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SpawnSetPermissions(property, &encapsInfo);
    } while (0);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(encapsInfo.encapsCount, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    FreeEncapsInfo(&encapsInfo);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_004, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    UserEncaps encapsInfo = {0};
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        const char *permissions = "{\"name\":\"Permissions\",\"ohos.encaps.count\":2,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225}]}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_JIT_PERMISSIONS,
            reinterpret_cast<uint8_t *>(const_cast<char *>(permissions)), strlen(permissions) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add permissions");
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SpawnSetPermissions(property, &encapsInfo);
    } while (0);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(encapsInfo.encapsCount, 2);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    FreeEncapsInfo(&encapsInfo);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_005, TestSize.Level0)
{
    UserEncaps encapsInfo = {0};
    int ret = AddMembersToEncapsInfo(NULL, &encapsInfo, 0);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_006, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":5,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},"
        "{\"ohos.permission.string\":\"{\\\"key\\\":\\\"value\\\"}\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";

    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    EXPECT_NE(encapsJson, nullptr);
    int count = 0;
    cJSON *permissions = GetEncapsPermissions(encapsJson, &count);
    EXPECT_EQ(permissions, nullptr);
    EXPECT_EQ(count, 0);

    cJSON_Delete(encapsJson);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_007, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":4,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},"
        "{\"ohos.permission.string\":\"{\\\"key\\\":\\\"value\\\"}\"},"
        "{\"ohos.permission.array\":[1,\"{\\\"key\\\":\\\"value\\\"}\",3,4,5]}]}";

    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    EXPECT_NE(encapsJson, nullptr);
    int count = 0;
    cJSON *permissions = GetEncapsPermissions(encapsJson, &count);
    EXPECT_NE(permissions, nullptr);
    EXPECT_EQ(count, 4);

    UserEncaps encapsInfo = {0};
    encapsInfo.encap = (UserEncap *)calloc(count, sizeof(UserEncap));
    EXPECT_NE(encapsInfo.encap, nullptr);
    int ret = AddMembersToEncapsInfo(permissions, &encapsInfo, count);
    EXPECT_EQ(ret, APPSPAWN_ERROR_UTILS_ADD_JSON_FAIL);
    EXPECT_EQ(encapsInfo.encapsCount, 3);

    cJSON_Delete(encapsJson);
    FreeEncapsInfo(&encapsInfo);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_008, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.bool\":true}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    // permissionItem->string is NULL
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionItem);
    EXPECT_EQ(ret, APPSPAWN_SYSTEM_ERROR);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_009, TestSize.Level0)
{
    // len = 512 + "\0"
    const char permissionItemStr[] = "{\"ohos.permission.string\":\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\"}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_010, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.strarray\":[\"abc\",\"def\",1]}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_011, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.boolarray\":[true,false,\"abc\"]}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_012, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.intarray\":[1,2,3,4,false]}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_013, TestSize.Level0)
{
    // array len = 510 + "\0" * 3
    const char permissionItemStr[] = "{\"ohos.permission.strarray\":[\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "\",\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAA\",\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\"]}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_014, TestSize.Level0)
{
    // array len = 509 + "\0" * 3
    const char permissionItemStr[] = "{\"ohos.permission.strarray\":[\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "\",\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAA\",\"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\"]}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(encap.valueLen, 512);
    EXPECT_EQ(encap.type, ENCAPS_CHAR_ARRAY);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_015, TestSize.Level0)
{
    // key len = 64 + "\0"
    const char permissionItemStr[] = "{\"ohos.permission.strarrayabcdefghijklmnopqrstuvwxyzaaaaaaaaaaaaaa\":"
        "\"{\\\"key\\\":\\\"value\\\"}\"}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_SYSTEM_ERROR);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_016, TestSize.Level0)
{
    // key len = 63 + "\0"
    const char permissionItemStr[] = "{\"ohos.permission.strarrayabcdefghijklmnopqrstuvwxyzaaaaaaaaaaaaa\":"
        "\"{\\\"key\\\":\\\"value\\\"}\"}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_017, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.int\":128}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(encap.type, ENCAPS_INT);
    EXPECT_EQ(encap.value.intValue, 128);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_018, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.bool\":true}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(encap.type, ENCAPS_BOOL);
    EXPECT_EQ(encap.value.intValue, 1);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_019, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":6,\"permissions\":100}";

    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    EXPECT_NE(encapsJson, nullptr);
    int count = 0;
    cJSON *permissions = GetEncapsPermissions(encapsJson, &count);
    EXPECT_EQ(permissions, nullptr);
    EXPECT_EQ(count, 0);

    cJSON_Delete(encapsJson);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_020, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":\"6\",\"permissions\":"
    "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},{\"ohos.permission.string\":\"nihaoma\"},"
    "{\"ohos.permission.strarray\":[\"abc\",\"def\"]},{\"ohos.permission.intarray\":[1,2,3,4,5]},"
    "{\"ohos.permission.boolarray\":[true,false,true]}]}";

    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    EXPECT_NE(encapsJson, nullptr);
    int count = 0;
    cJSON *permissions = GetEncapsPermissions(encapsJson, &count);
    EXPECT_EQ(permissions, nullptr);
    EXPECT_EQ(count, 0);

    cJSON_Delete(encapsJson);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_021, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.intarray\":[]}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_022, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.strarray\":[\"abc\",\"\",\"def\"]}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_023, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.str\":\"\"}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_024, TestSize.Level0)
{
    const char permissionItemStr[] = "[1,2,3,4,5]";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionIntArrayToValue(permissionChild, &encap, 6);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_025, TestSize.Level0)
{
    const char permissionItemStr[] = "[true,false,true]";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionBoolArrayToValue(permissionChild, &encap, 4);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_026, TestSize.Level0)
{
    const char permissionItemStr[] = "[\"abc\",\"def\"]";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *item = cJSON_CreateString("");
    EXPECT_NE(item, nullptr);
    free(item->valuestring);
    item->valuestring = nullptr;
    EXPECT_EQ(cJSON_AddItemToArray(permissionItem, item), 1);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionStrArrayToValue(permissionChild, &encap);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_027, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"ohos.permission.intarray\":[{\"ohos.permission.bool\":true},2,3,4,5]}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionArrayToValue(permissionChild, &encap);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_028, TestSize.Level0)
{
    const char permissionItemStr[] = "{\"permissions\":{\"ohos.permission.bool\":true}}";

    cJSON *permissionItem = cJSON_Parse(permissionItemStr);
    EXPECT_NE(permissionItem, nullptr);
    cJSON *permissionChild = permissionItem->child;
    EXPECT_NE(permissionChild, nullptr);
    UserEncap encap;
    (void)memset_s(&encap, sizeof(encap), 0, sizeof(encap));
    int ret = AddPermissionItemToEncapsInfo(&encap, permissionChild);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(permissionItem);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_029, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":6,\"permissions\":[]}";

    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    EXPECT_NE(encapsJson, nullptr);
    int count = 0;
    cJSON *permissions = GetEncapsPermissions(encapsJson, &count);
    EXPECT_EQ(permissions, nullptr);
    EXPECT_EQ(count, 0);

    cJSON_Delete(encapsJson);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_030, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    UserEncaps encapsInfo = {0};
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        const char *permissions = "{\"name\":\"Permissions\",\"ohos.encaps.count\":4,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},"
        "{\"ohos.permission.string\":\"{\\\"key\\\":\\\"value\\\"}\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_JIT_PERMISSIONS,
            reinterpret_cast<uint8_t *>(const_cast<char *>(permissions)), strlen(permissions) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add permissions");
        const char *maxChildProcess = "512";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX,
            reinterpret_cast<uint8_t *>(const_cast<char *>(maxChildProcess)), strlen(maxChildProcess) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add maxChildProcess");
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SpawnSetPermissions(property, &encapsInfo);
    } while (0);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(encapsInfo.encapsCount, 4);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    FreeEncapsInfo(&encapsInfo);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_031, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    UserEncaps encapsInfo = {0};
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        const char *permissions = "{\"name\":\"Permissions\",\"ohos.encaps.count\":4,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},"
        "{\"ohos.permission.string\":\"{\\\"key\\\":\\\"value\\\"}\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_JIT_PERMISSIONS,
            reinterpret_cast<uint8_t *>(const_cast<char *>(permissions)), strlen(permissions) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add permissions");
        const char *maxChildProcess = "0";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX,
            reinterpret_cast<uint8_t *>(const_cast<char *>(maxChildProcess)), strlen(maxChildProcess) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add maxChildProcess");
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SpawnSetPermissions(property, &encapsInfo);
    } while (0);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(encapsInfo.encapsCount, 4);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    FreeEncapsInfo(&encapsInfo);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_032, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_HYBRID_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(HYBRIDSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", HYBRIDSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s",
                       HYBRIDSPAWN_SERVER_NAME);
        const char *permissions = "{\"name\":\"Permissions\",\"ohos.encaps.count\":6,\"permissions\":"
            "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},{\"ohos.permission.string\":\"nihaoma\"},"
            "{\"ohos.permission.strarray\":[\"abc\",\"def\"]},{\"ohos.permission.intarray\":[1,2,3,4,5]},"
            "{\"ohos.permission.boolarray\":[true,false,true]}]}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_JIT_PERMISSIONS,
            reinterpret_cast<uint8_t *>(const_cast<char *>(permissions)), strlen(permissions) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add permissions");
        const char *maxChildProcess = "512";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX,
            reinterpret_cast<uint8_t *>(const_cast<char *>(maxChildProcess)), strlen(maxChildProcess) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add maxChildProcess");
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        ret = SpawnSetEncapsPermissions(mgr, property);
    } while (0);

    EXPECT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_033, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_HYBRID_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(HYBRIDSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", HYBRIDSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", HYBRIDSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SpawnSetEncapsPermissions(mgr, property);
    } while (0);

    EXPECT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_034, TestSize.Level0)
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
        const char *permissions = "{\"name\":\"Permissions\",\"ohos.encaps.count\":6,\"permissions\":"
            "[{\"ohos.permission.kernel.ALLOW_DEBUG\":true},{\"ohos.permission.kernel.ALLOW_DEBUG\":3225}, \
             {\"ohos.permission.kernel.ALLOW_DEBUG\":\"nihaoma\"},"
            "{\"ohos.permission.kernel.ALLOW_DEBUG\":[\"abc\",\"def\"]}, \
             {\"ohos.permission.kernel.ALLOW_DEBUG\":[1,2,3,4,5]},"
            "{\"ohos.permission.kernel.ALLOW_DEBUG\":[true,false,true]}]}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_JIT_PERMISSIONS,
            reinterpret_cast<uint8_t *>(const_cast<char *>(permissions)), strlen(permissions) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add permissions");
        const char *maxChildProcess = "512";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX,
            reinterpret_cast<uint8_t *>(const_cast<char *>(maxChildProcess)), strlen(maxChildProcess) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add maxChildProcess");
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        ret = SpawnSetEncapsPermissions(mgr, property);
    } while (0);

    EXPECT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief appspawn为未携带APP_FLAGS_CUSTOM_SANDBOX的非弱沙箱应用应用，不传递标识给内核进行设置
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_035, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SpawnSetEncapsPermissions(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn为携带了APP_FLAGS_CUSTOM_SANDBOX的弱沙箱应用应用，传递标识给内核进行设置
 *
 */
HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_036, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_CUSTOM_SANDBOX);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_CUSTOM_SANDBOX), 0);

        ret = SpawnSetEncapsPermissions(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_037, TestSize.Level0)
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
        const char *permissions = "{\"name\":\"Permissions\",\"ohos.encaps.count\":6,\"permissions\":"
            "[{\"ohos.permission.kernel.ALLOW_DEBUG\":true},{\"ohos.permission.kernel.ALLOW_DEBUG\":3225}, \
             {\"ohos.permission.kernel.ALLOW_DEBUG\":\"nihaoma\"},"
            "{\"ohos.permission.kernel.ALLOW_DEBUG\":[\"abc\",\"def\"]}, \
             {\"ohos.permission.kernel.ALLOW_DEBUG\":[1,2,3,4,5]},"
            "{\"ohos.permission.kernel.ALLOW_DEBUG\":[true,false,true]}]}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_JIT_PERMISSIONS,
            reinterpret_cast<uint8_t *>(const_cast<char *>(permissions)), strlen(permissions) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add permissions");
        const char *maxChildProcess = "512";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX,
            reinterpret_cast<uint8_t *>(const_cast<char *>(maxChildProcess)), strlen(maxChildProcess) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add maxChildProcess");
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SANDBOX);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        ret = SpawnSetEncapsPermissions(mgr, property);
    } while (0);

    EXPECT_EQ(ret, 0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetFdEnv, TestSize.Level0)
{
    int ret = SetFdEnv(nullptr, nullptr);
    EXPECT_EQ(ret, -1);

    AppSpawningCtx property;
    ret = SetFdEnv(nullptr, &property);
    EXPECT_EQ(ret, -1); // message == null

    property.message = (AppSpawnMsgNode *)malloc(sizeof(AppSpawnMsgNode) + sizeof(AppSpawnMsgNode) + APP_LEN_PROC_NAME);
    ASSERT_EQ(property.message != nullptr, 1);
    AppSpawnConnection *connection = (AppSpawnConnection *)malloc(sizeof(AppSpawnConnection));
    ASSERT_EQ(connection != nullptr, 1);
    uint8_t *buffer = (uint8_t*)malloc(sizeof(uint8_t) * 10);
    ASSERT_EQ(buffer != nullptr, 1);

    property.message->buffer = nullptr;
    property.message->connection = nullptr;
    ret = SetFdEnv(nullptr, &property);
    EXPECT_EQ(ret, -1); // message != null, message->buffer == null, message->connection == null

    property.message->buffer = nullptr;
    property.message->connection = connection;
    ret = SetFdEnv(nullptr, &property); // message != null, message->connection != null, message->buffer == null
    EXPECT_EQ(ret, -1);

    property.message->buffer = buffer;
    property.message->connection = nullptr;
    ret = SetFdEnv(nullptr, &property); // message != null, message->connection == null, message->buffer != null
    EXPECT_EQ(ret, -1);

    free(buffer);
    free(connection);
    free(property.message);
}

#ifdef APPSPAWN_HITRACE_OPTION
HWTEST_F(AppSpawnCommonTest, App_Spawn_FilterAppSpawnTrace, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    FilterAppSpawnTrace(nullptr, property);
    EXPECT_EQ(property, nullptr);

    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        FilterAppSpawnTrace(nullptr, property);
    } while (0);

    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}
#endif

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetCapabilities, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
        EXPECT_EQ(mgr != nullptr, 1);
        // create msg
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetCapabilities(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetSchedPriority_01, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_NE(mgr, nullptr);
    int ret = SetSchedPriority(mgr, nullptr);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);

    mgr = CreateAppSpawnMgr(MODE_FOR_HYBRID_SPAWN);
    EXPECT_NE(mgr, nullptr);
    ret = SetSchedPriority(mgr, nullptr);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);

    mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_NE(mgr, nullptr);
    ret = SetSchedPriority(mgr, nullptr);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetUidGid_001, TestSize.Level0)
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
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = false;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetUidGid_002, TestSize.Level0)
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
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = true;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetUidGid_003, TestSize.Level0)
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
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = false;
        property->client.flags = APP_DEVELOPER_MODE;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetUidGid_004, TestSize.Level0)
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
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = true;
        property->client.flags = APP_DEVELOPER_MODE;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetUidGid_005, TestSize.Level0)
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
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = false;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetUidGid_006, TestSize.Level0)
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
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = true;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetUidGid_007, TestSize.Level0)
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
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);

        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = false;
        property->client.flags = APP_DEVELOPER_MODE;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetUidGid_008, TestSize.Level0)
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
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = true;
        property->client.flags = APP_DEVELOPER_MODE;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}
}  // namespace OHOS
