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
#include <cstdbool>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "appspawn_manager.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_server.h"
#include "appspawn_utils.h"
#include "cJSON.h"
#include "json_utils.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

#define MAX_BUFF 20
#define TEST_STR_LEN 30

extern "C" {
    void DumpSandboxMountNode(const SandboxMountNode *sandboxNode, uint32_t index);
}

namespace OHOS {
AppSpawnTestHelper g_testHelper;
class AppSpawnDebugSandboxTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase()
    {
        StubNode *stub = GetStubNode(STUB_MOUNT);
        if (stub) {
            stub->flags &= ~STUB_NEED_CHECK;
        }
    }
    void SetUp() {}
    void TearDown() {}
};

static AppSpawningCtx *AppSpawnDebugSandboxTestCreateAppSpawningCtx(int base)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create reqMgr");
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, return nullptr, "Failed to create req");
    if (base == 0) {
        return g_testHelper.GetAppProperty(clientHandle, reqHandle);
    } else if (base == 1) {
        // save provision type to req
        const char *provision = "debug";
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PROVISION_TYPE, provision);
        APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to add provision %{public}s", APPSPAWN_SERVER_NAME);
        return g_testHelper.GetAppProperty(clientHandle, reqHandle);
    }
    return nullptr;
}

static SandboxContext *AppSpawnDebugSandboxTestGetSandboxContext(const AppSpawningCtx *property, int newbspawn)
{
    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppProperty(property, TLV_MSG_FLAGS);
    APPSPAWN_CHECK(msgFlags != nullptr, return nullptr, "No msg flags in msg %{public}s", GetProcessName(property));

    SandboxContext *context = GetSandboxContext();
    APPSPAWN_CHECK(context != nullptr, return nullptr, "Failed to get context");

    context->nwebspawn = newbspawn;
    context->bundleName = GetBundleName(property);
    context->bundleHasWps = strstr(context->bundleName, "wps") != nullptr;
    context->dlpBundle = strcmp(GetProcessName(property), "com.ohos.dlpmanager") == 0;
    context->appFullMountEnable = 0;

    context->sandboxSwitch = 1;
    context->sandboxShared = false;
    context->message = property->message;
    context->rootPath = nullptr;
    context->rootPath = nullptr;
    return context;
}

/**
 * @tc.name: InstallDebugSandbox_ShouldReturnInvalidArg_WhenPropertyIsNull
 * @tc.desc: 测试当 property 为NULL 时, 函数应返回 APPSPAWN_ARG_INVAILD.
 * @tc.number: InstallDebugSandboxTest_001
 */
HWTEST_F(AppSpawnDebugSandboxTest, ATC_InstallDebugSandbox_ShouldReturnInvalidArg_WhenPropertyIsNull, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *property = nullptr;
    int ret = InstallDebugSandbox(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    // delete
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: InstallDebugSandbox_ShouldReturnInvalidArg_WhenContentIsNull
 * @tc.desc: 测试当 content 为NULL 时, 函数应返回 APPSPAWN_ARG_INVAILD.
 * @tc.number: InstallDebugSandboxTest_002
 */
HWTEST_F(AppSpawnDebugSandboxTest, ATC_InstallDebugSandbox_ShouldReturnInvalidArg_WhenContentIsNull, TestSize.Level0)
{
    AppSpawnMgr *mgr = nullptr;
    AppSpawningCtx *property = AppSpawnDebugSandboxTestCreateAppSpawningCtx(0);
    int ret = InstallDebugSandbox(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    // delete
    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: InstallDebugSandbox_ShouldReturnZero_WhenNotDeveloperMode
 * @tc.desc: 测试当不在开发者模式时,函数应返回 0.
 * @tc.number: InstallDebugSandboxTest_003
 */
HWTEST_F(AppSpawnDebugSandboxTest, ATC_InstallDebugSandbox_ShouldReturnZero_WhenNotDeveloperMode, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *property = AppSpawnDebugSandboxTestCreateAppSpawningCtx(0);
    int ret = InstallDebugSandbox(mgr, property);
    EXPECT_EQ(ret, 0);
    // delete
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: InstallDebugSandbox_ShouldReturnZero_WhenProvisionTypeInvalid
 * @tc.desc: 测试当 provisionType 无效时,函数应返回 0.
 * @tc.number: InstallDebugSandboxTest_004
 */
HWTEST_F(AppSpawnDebugSandboxTest, ATC_InstallDebugSandbox_ShouldReturnZero_WhenProvisionTypeInvalid, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *property = AppSpawnDebugSandboxTestCreateAppSpawningCtx(0);
    property->client.flags = APP_DEVELOPER_MODE;
    int ret = InstallDebugSandbox(mgr, property);
    EXPECT_EQ(ret, 0);
    // delete
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: InstallDebugSandbox_ShouldReturnSandboxInvalid_WhenSandboxConfigNotFound
 * @tc.desc: 测试当找不到沙箱配置时,函数3应返回 APPSPAWN_SANDBOX_INVALID.
 * @tc.number: InstallDebugSandboxTest_005
 */
HWTEST_F(AppSpawnDebugSandboxTest,
         ATC_InstallDebugSandbox_ShouldReturnSandboxInvalid_WhenSandboxConfigNotFound, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *property = AppSpawnDebugSandboxTestCreateAppSpawningCtx(1);
    property->client.flags = APP_DEVELOPER_MODE;
    int ret = InstallDebugSandbox(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_SANDBOX_INVALID);
    // delete
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: InstallDebugSandbox_ShouldReturnSystemError_WhenSandboxContextIsNull
 * @tc.desc: 测试当沙箱上下文获取失败时,函数应返回 APPSPAWN_SYSTEM_ERROR
 * @tc.number: InstallDebugSandboxTest_006
 */
HWTEST_F(AppSpawnDebugSandboxTest,
         ATC_InstallDebugSandbox_ShouldReturnSystemError_WhenSandboxContextIsNull, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *property = AppSpawnDebugSandboxTestCreateAppSpawningCtx(1);
    property->client.flags = APP_DEVELOPER_MODE;
    int ret = PreLoadDebugSandboxCfg(mgr);
    EXPECT_EQ(ret, 0);
    ret = InstallDebugSandbox(mgr, property);
    EXPECT_EQ(ret, 0);
    // delete
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}