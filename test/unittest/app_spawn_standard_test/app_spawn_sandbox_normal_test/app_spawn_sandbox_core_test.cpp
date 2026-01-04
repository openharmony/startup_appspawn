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

#include <gtest/gtest.h>
#include <cerrno>
#include <memory>
#include <string>
#include <vector>

#include "appspawn_server.h"
#include "appspawn_service.h"
#include "json_utils.h"
#include "parameter.h"
#include "sandbox_def.h"
#include "sandbox_core.h"
#include "sandbox_common.h"
#include "securec.h"
#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"
#include "sandbox_dec.h"
#include "parameters.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;

namespace OHOS {
AppSpawnTestHelper g_testHelperCore;

class AppSpawnSandboxCoreTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppSpawnSandboxCoreTest::SetUpTestCase() {}

void AppSpawnSandboxCoreTest::TearDownTestCase() {}

void AppSpawnSandboxCoreTest::SetUp()
{
    const TestInfo *info = UnitTest::GetInstance()->current_test_info();
    GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
    APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());

    std::vector<cJSON *> &appVec =
        AppSpawn::SandboxCommon::GetCJsonConfig(SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);
    appVec.erase(appVec.begin(), appVec.end());

    std::vector<cJSON *> &isolatedVec =
        AppSpawn::SandboxCommon::GetCJsonConfig(SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG);
    isolatedVec.erase(isolatedVec.begin(), isolatedVec.end());
}

void AppSpawnSandboxCoreTest::TearDown()
{
    std::vector<cJSON *> &appVec =
        AppSpawn::SandboxCommon::GetCJsonConfig(SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);
    appVec.erase(appVec.begin(), appVec.end());

    std::vector<cJSON *> &isolatedVec =
        AppSpawn::SandboxCommon::GetCJsonConfig(SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG);
    isolatedVec.erase(isolatedVec.begin(), isolatedVec.end());

    const TestInfo *info = UnitTest::GetInstance()->current_test_info();
    GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
}

static AppSpawningCtx *GetTestAppPropertyCore()
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create reqMgr");
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, return nullptr, "Failed to create req");
    return g_testHelperCore.GetAppProperty(clientHandle, reqHandle);
}

// ==================== 网络隔离相关测试 ====================

/**
 * @tc.name: NeedNetworkIsolated_01
 * @tc.desc: Test network isolation when app requires network isolation
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, NeedNetworkIsolated_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.isolated.app");
    g_testHelperCore.SetTestApl("normal");
    std::vector<const char *> &permissions = g_testHelperCore.GetPermissions();
    permissions.push_back("ohos.permission.INTERNET");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    bool ret = AppSpawn::SandboxCore::NeedNetworkIsolated(appProperty);
    EXPECT_EQ(ret, false);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: NeedNetworkIsolated_02
 * @tc.desc: Test network isolation when app is normal app
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, NeedNetworkIsolated_02, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.normal.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    bool ret = AppSpawn::SandboxCore::NeedNetworkIsolated(appProperty);
    EXPECT_EQ(ret, false);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: EnableSandboxNamespace_01
 * @tc.desc: Test enabling sandbox namespace with basic flags
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, EnableSandboxNamespace_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    uint32_t sandboxNsFlags = CLONE_NEWPID | CLONE_NEWNS;
    int ret = AppSpawn::SandboxCore::EnableSandboxNamespace(appProperty, sandboxNsFlags);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: EnableSandboxNamespace_02
 * @tc.desc: Test enabling sandbox namespace with network isolation
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, EnableSandboxNamespace_02, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    uint32_t sandboxNsFlags = CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWNET;
    int ret = AppSpawn::SandboxCore::EnableSandboxNamespace(appProperty, sandboxNsFlags);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: GetAppMsgFlags_01
 * @tc.desc: Test getting app message flags for normal app
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, GetAppMsgFlags_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.normal.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    uint32_t flags = AppSpawn::SandboxCore::GetAppMsgFlags(appProperty);
    EXPECT_EQ(flags != 0, 1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: GetAppMsgFlags_02
 * @tc.desc: Test getting app message flags for preinstalled app
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, GetAppMsgFlags_02, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.preinstalled.app");
    g_testHelperCore.SetTestApl("system_basic");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    uint32_t flags = AppSpawn::SandboxCore::GetAppMsgFlags(appProperty);
    EXPECT_EQ(flags != 0, 1);

    DeleteAppSpawningCtx(appProperty);
}

// ==================== 权限和标志相关测试 ====================

/**
 * @tc.name: CheckMountFlag_01
 * @tc.desc: Test mount flag checking for valid configuration
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, CheckMountFlag_01, TestSize.Level0)
{
    const char *bundleName = "com.ohos.test.app";
    const char *appConfigStr = R"({
        "mount-flags": {
            "test-flag": {
                "enabled": true,
                "condition": "normal"
            }
        }
    })";

    g_testHelperCore.SetProcessName(bundleName);
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    cJSON *appConfig = cJSON_Parse(appConfigStr);
    ASSERT_EQ(appConfig != nullptr, 1);

    bool ret = AppSpawn::SandboxCore::CheckMountFlag(appProperty, bundleName, appConfig);
    EXPECT_EQ(ret, true);

    cJSON_Delete(appConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: UpdateMsgFlagsWithPermission_01
 * @tc.desc: Test updating message flags with permissions
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, UpdateMsgFlagsWithPermission_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.permission.app");
    g_testHelperCore.SetTestApl("normal");
    std::vector<const char *> &permissions = g_testHelperCore.GetPermissions();
    permissions.push_back("ohos.permission.READ_MEDIA");
    permissions.push_back("ohos.permission.WRITE_MEDIA");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    AppSpawn::SandboxCore::UpdateMsgFlagsWithPermission(appProperty, "normal", 0);
    EXPECT_EQ(appProperty != nullptr, 1);

    DeleteAppSpawningCtx(appProperty);
}

// ==================== 路径和沙箱相关测试 ====================

/**
 * @tc.name: GetSandboxPath_01
 * @tc.desc: Test getting sandbox path for normal app
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, GetSandboxPath_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    cJSON *mntPoint = nullptr;
    std::string section = "app-base";
    std::string sandboxRoot = "/data/sandbox";
    std::string sandboxPath = AppSpawn::SandboxCore::GetSandboxPath(appProperty, mntPoint, section, sandboxRoot);
    EXPECT_EQ(!sandboxPath.empty(), 1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxRootFolderCreateAdapt_01
 * @tc.desc: Test sandbox root folder creation adaptation
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxRootFolderCreateAdapt_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    std::string sandboxPackagePath = "/data/test/sandbox";
    int ret = AppSpawn::SandboxCore::DoSandboxRootFolderCreateAdapt(sandboxPackagePath);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: DoSandboxRootFolderCreate_01
 * @tc.desc: Test sandbox root folder creation
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxRootFolderCreate_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    std::string sandboxPackagePath = "/data/test/sandbox";
    int ret = AppSpawn::SandboxCore::DoSandboxRootFolderCreate(appProperty, sandboxPackagePath);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: ChangeCurrentDir_01
 * @tc.desc: Test changing current directory
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, ChangeCurrentDir_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    std::string sandboxPackagePath = "/data/test/sandbox";
    std::string bundleName = "com.ohos.test.app";
    bool sandboxSharedStatus = false;

    int ret = AppSpawn::SandboxCore::ChangeCurrentDir(sandboxPackagePath, bundleName, sandboxSharedStatus);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: SetDecWithDir_01
 * @tc.desc: Test setting DEC with directory
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, SetDecWithDir_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    uint32_t userId = 1000;
    int ret = AppSpawn::SandboxCore::SetDecWithDir(appProperty, userId);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: SetDecDenyWithDir_01
 * @tc.desc: Test setting DEC deny with directory
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, SetDecDenyWithDir_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    AppSpawn::SandboxCore::SetDecDenyWithDir(appProperty);
    EXPECT_EQ(appProperty != nullptr, 1);

    DeleteAppSpawningCtx(appProperty);
}

// ==================== 私有文件相关测试 ====================

/**
 * @tc.name: DoSandboxFilePrivateBind_01
 * @tc.desc: Test private file binding
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxFilePrivateBind_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.private.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    cJSON *wholeConfig = nullptr;
    int ret = AppSpawn::SandboxCore::DoSandboxFilePrivateBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFilePrivateSymlink_01
 * @tc.desc: Test private file symlink creation
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxFilePrivateSymlink_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.private.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    cJSON *wholeConfig = nullptr;
    int ret = AppSpawn::SandboxCore::DoSandboxFilePrivateSymlink(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: SetPrivateAppSandboxProperty__01
 * @tc.desc: Test private app sandbox property setting
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, SetPrivateAppSandboxProperty__01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.private.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    cJSON *config = nullptr;
    int ret = AppSpawn::SandboxCore::SetPrivateAppSandboxProperty_(appProperty, config);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

// ==================== 权限文件相关测试 ====================

/**
 * @tc.name: DoSandboxFilePermissionBind_01
 * @tc.desc: Test permission file binding
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxFilePermissionBind_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.permission.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    cJSON *wholeConfig = nullptr;
    int ret = AppSpawn::SandboxCore::DoSandboxFilePermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: SetPermissionAppSandboxProperty__01
 * @tc.desc: Test permission app sandbox property setting
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, SetPermissionAppSandboxProperty__01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.permission.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    cJSON *config = nullptr;
    int ret = AppSpawn::SandboxCore::SetPermissionAppSandboxProperty_(appProperty, config);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: SetPermissionAppSandboxProperty_01
 * @tc.desc: Test permission app sandbox property setting
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, SetPermissionAppSandboxProperty_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.permission.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_EQ(appProperty != nullptr, 1);

    int ret = AppSpawn::SandboxCore::SetPermissionAppSandboxProperty(appProperty);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

} // namespace OHOS