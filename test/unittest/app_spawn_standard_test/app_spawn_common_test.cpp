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
#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_permission.h"
#include "appspawn_hisysevent.h"
#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static AppSpawnTestHelper g_testHelper;
class AppSpawnCommonTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_001, TestSize.Level0)
{
    int ret = SetSelinuxConNweb(nullptr, nullptr);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_002, TestSize.Level0)
{
    int ret = SetAppAccessToken(nullptr, nullptr);
    EXPECT_NE(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_003, TestSize.Level0)
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
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_DEVELOPER_MODE;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_004, TestSize.Level0)
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
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_BEGETCTL_BOOT;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, APPSPAWN_NATIVE_NOT_SUPPORT);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_005, TestSize.Level0)
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
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE,
            reinterpret_cast<uint8_t *>(const_cast<char *>("render")), 7); // 7 is value len
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetSeccompFilter(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_006, TestSize.Level0)
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
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE,
            reinterpret_cast<uint8_t *>(const_cast<char *>("xxx")), 7); // 7 is value len
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ISOLATED_SANDBOX);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetSeccompFilter(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_007, TestSize.Level0)
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
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE,
            reinterpret_cast<uint8_t *>(const_cast<char *>("xxx")), 7); // 7 is value len
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_IGNORE_SANDBOX);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetSeccompFilter(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_008, TestSize.Level0)
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
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE,
            reinterpret_cast<uint8_t *>(const_cast<char *>("xxx")), 7); // 7 is value len
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_IGNORE_SANDBOX);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetSeccompFilter(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_009, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 0, 0);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        ret = SetInternetPermission(property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_010, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 0, 1);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        ret = SetInternetPermission(property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_011, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 1, 0);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        ret = SetInternetPermission(property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_012, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 1, 1);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        ret = SetInternetPermission(property);
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
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_021, TestSize.Level0)
{
    int ret = SetInternetPermission(nullptr);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_022, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        const char *appEnv = "{\"test\"}";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, "AppEnv",
            reinterpret_cast<uint8_t *>(const_cast<char *>(appEnv)), strlen(appEnv) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add ext tlv %{public}s", appEnv);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        reqHandle = nullptr;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        SetEnvInfo(nullptr, property);
        // spawn
        property = nullptr;
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_023, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        const char *appEnv = "%";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, "AppEnv",
            reinterpret_cast<uint8_t *>(const_cast<char *>(appEnv)), strlen(appEnv) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add ext tlv %{public}s", appEnv);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        reqHandle = nullptr;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        SetEnvInfo(nullptr, property);
        // spawn
        property = nullptr;
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_024, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        reqHandle = nullptr;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        SetEnvInfo(nullptr, property);
        // spawn
        property = nullptr;
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_025, TestSize.Level0)
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
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_BEGETCTL_BOOT;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        SetAppSpawnMsgFlag(property->message, APP_FLAGS_ISOLATED_SANDBOX, 16);
        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, APPSPAWN_NATIVE_NOT_SUPPORT);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_026, TestSize.Level0)
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
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_BEGETCTL_BOOT;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        SetAppSpawnMsgFlag(property->message, APP_FLAGS_DLP_MANAGER, 1);
        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, APPSPAWN_NATIVE_NOT_SUPPORT);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Common_027, TestSize.Level0)
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
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        property->client.flags |= APP_BEGETCTL_BOOT;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        SetAppSpawnMsgFlag(property->message, APP_FLAGS_DEBUGGABLE, 1);
        ret = SetSelinuxCon(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, APPSPAWN_NATIVE_NOT_SUPPORT);
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
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":0,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},{\"ohos.permission.string\":\"test\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";

    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    uint32_t permissionCount = 0;
    int ret = AddPermissionToEncaps(encapsJson, NULL, &permissionCount);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    cJSON_Delete(encapsJson);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_002, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":4,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},{\"ohos.permission.string\":\"test\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";

    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    uint32_t permissionCount = 0;
    int ret = AddPermissionToEncaps(encapsJson, NULL, &permissionCount);
    EXPECT_EQ(ret, APPSPAWN_ERROR_UTILS_ADD_JSON_FAIL);

    cJSON_Delete(encapsJson);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_003, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":5,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},{\"ohos.permission.string\":\"test\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";

    cJSON *encapsOut = cJSON_CreateObject();
    EXPECT_NE(encapsOut, nullptr);
    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    uint32_t permissionCount = 0;

    cJSON *permissions = cJSON_GetObjectItemCaseSensitive(encapsJson, "permissions");
    EXPECT_NE(permissions, nullptr);
    cJSON *emptyItem = cJSON_CreateObject();
    cJSON_AddItemToArray(permissions, emptyItem);

    int ret = AddPermissionToEncaps(encapsJson, encapsOut, &permissionCount);
    EXPECT_EQ(ret, APPSPAWN_ERROR_UTILS_DECODE_JSON_FAIL);

    cJSON_Delete(encapsJson);
    cJSON_Delete(encapsOut);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_004, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":4,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},{\"ohos.permission.string\":\"test\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";

    cJSON *encapsOut = cJSON_CreateObject();
    EXPECT_NE(encapsOut, nullptr);
    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    uint32_t permissionCount = 0;

    cJSON *permissions = cJSON_GetObjectItemCaseSensitive(encapsJson, "permissions");
    EXPECT_NE(permissions, nullptr);
    cJSON *permission = cJSON_GetArrayItem(permissions, 0);
    EXPECT_NE(permission, nullptr);
    permission->child = nullptr;
    int ret = AddPermissionToEncaps(encapsJson, encapsOut, &permissionCount);
    EXPECT_EQ(ret, APPSPAWN_ERROR_UTILS_DECODE_JSON_FAIL);

    cJSON_Delete(encapsJson);
    cJSON_Delete(encapsOut);
}

HWTEST_F(AppSpawnCommonTest, App_Spawn_Encaps_005, TestSize.Level0)
{
    const char encapsJsonStr[] = "{\"name\":\"Permissions\",\"ohos.encaps.count\":4,\"permissions\":"
        "[{\"ohos.permission.bool\":true},{\"ohos.permission.int\":3225},{\"ohos.permission.string\":\"test\"},"
        "{\"ohos.permission.array\":[1,2,3,4,5]}]}";

    cJSON *encapsOut = cJSON_CreateObject();
    EXPECT_NE(encapsOut, nullptr);
    cJSON *encapsJson = cJSON_Parse(encapsJsonStr);
    uint32_t permissionCount = 0;

    int ret = AddPermissionToEncaps(encapsJson, encapsOut, &permissionCount);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(permissionCount, 4);

    cJSON_Delete(encapsJson);
    cJSON_Delete(encapsOut);
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

HWTEST_F(AppSpawnCommonTest, App_Spawn_SetIsolateDir, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    int ret = -1;
    do {
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        // set custom sandbox flag
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_CUSTOM_SANDBOX);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetIsolateDir(property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
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
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
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

}  // namespace OHOS
