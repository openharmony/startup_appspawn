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
#include <unistd.h>

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

    uint32_t flags = AppSpawn::SandboxCore::GetAppMsgFlags(appProperty);
    EXPECT_EQ(flags, 0);

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
    ASSERT_NE(appProperty, nullptr);

    uint32_t flags = AppSpawn::SandboxCore::GetAppMsgFlags(appProperty);
    EXPECT_EQ(flags, 0);

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
    ASSERT_NE(appProperty, nullptr);

    cJSON *appConfig = cJSON_Parse(appConfigStr);
    ASSERT_EQ(appConfig != nullptr, 1);

    bool ret = AppSpawn::SandboxCore::CheckMountFlag(appProperty, bundleName, appConfig);
    EXPECT_EQ(ret, false);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

    const std::string buffer = "{ \
            \"global\": { \
                \"sandbox-path\": \"/mnt/sandbox/<currentUserId>/sandbox-path\", \
                \"sandbox-ns-flags\": [ \"pid\", \"net\" ], \
                \"sandbox-flags\": 100, \
                \"top-sandbox-switch\": \"ON\" \
            } \
        }";

    cJSON *mntPoint = cJSON_Parse(buffer.c_str());
    ASSERT_EQ(mntPoint != nullptr, 1);
    cJSON *mntPoint1 = cJSON_GetObjectItemCaseSensitive(mntPoint, "global");

    std::string section = "app-base";
    std::string sandboxRoot = "/data/sandbox";
    std::string sandboxPath = AppSpawn::SandboxCore::GetSandboxPath(appProperty, mntPoint1, section, sandboxRoot);
    std::cout << "sandboxPath: " << sandboxPath << std::endl;
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
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/data/test/sandbox";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    int ret = AppSpawn::SandboxCore::DoSandboxRootFolderCreate(mgr, appProperty, sandboxPackagePath);
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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

    AppSpawn::SandboxCore::SetDecDenyWithDir(appProperty);
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

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
    ASSERT_NE(appProperty, nullptr);

    int ret = AppSpawn::SandboxCore::SetPermissionAppSandboxProperty(appProperty);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_01
 * @tc.desc: Test MountIPCGroup with appProperty nullptr
 *           Branch: appProperty == nullptr → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_01, TestSize.Level0)
{
    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    int ret = AppSpawn::SandboxCore::MountIPCGroup(nullptr, sandboxPackagePath);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: MountIPCGroup_02
 * @tc.desc: Test MountIPCGroup with empty sandboxPackagePath
 *           Branch: sandboxPackagePath == "" → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_02, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "";
    int ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_03
 * @tc.desc: Test MountIPCGroup with no IPCGroup ext info
 *           Branch: ipcGroupRoot == nullptr → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_03, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.test.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    int ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_04
 * @tc.desc: Test MountIPCGroup with empty IPCGroup array
 *           Branch: HandleArrayForeach iterates nothing → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_04, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo = R"([])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_05
 * @tc.desc: Test MountIPCGroup with array item missing ipcGroupId
 *           Branch: ProcessIPCGroupItem groupIdItem == nullptr → return -1, abort
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_05, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo = R"([{"ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // ProcessIPCGroupItem: groupIdItem == nullptr → return -1, abort
    EXPECT_EQ(ret, -1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_06
 * @tc.desc: Test MountIPCGroup with ipcGroupId as non-string type (number)
 *           Branch: ProcessIPCGroupItem !cJSON_IsString(groupIdItem) → return -1, abort
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_06, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo = R"([{"ipcGroupId":12345,"ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // ProcessIPCGroupItem: !cJSON_IsString → return -1, abort
    EXPECT_EQ(ret, -1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_07
 * @tc.desc: Test MountIPCGroup with empty string ipcGroupId
 *           Branch: ProcessIPCGroupItem strlen == 0 → return -1, abort
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_07, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo = R"([{"ipcGroupId":"","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // ProcessIPCGroupItem: strlen == 0 → return -1, abort
    EXPECT_EQ(ret, -1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_08
 * @tc.desc: Test MountIPCGroup with array item missing ipcGroupGid
 *           Branch: ProcessIPCGroupItem groupGidItem == nullptr → return -1, abort
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_08, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo = R"([{"ipcGroupId":"308"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // ProcessIPCGroupItem: groupGidItem == nullptr → return -1, abort
    EXPECT_EQ(ret, -1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_09
 * @tc.desc: Test MountIPCGroup with ipcGroupGid as non-string type (number)
 *           Branch: ProcessIPCGroupItem !cJSON_IsString(groupGidItem) → return -1, abort
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_09, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo = R"([{"ipcGroupId":"309","ipcGroupGid":3000}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // ProcessIPCGroupItem: !cJSON_IsString(groupGidItem) → return -1, abort
    EXPECT_EQ(ret, -1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_10
 * @tc.desc: Test MountIPCGroup with valid single-element IPCGroup array
 *           Branch: ProcessIPCGroupItem valid → EnsureDirWithMode + mount → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_10, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"123","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // Valid data: srcPath = /mnt/sandbox/shm/100/group/123
    EXPECT_EQ(ret, 0);

    rmdir("/mnt/sandbox/shm/100/group/123");
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_11
 * @tc.desc: Test MountIPCGroup with multi-element IPCGroup array
 *           Branch: HandleArrayForeach iterates all items, all valid → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_11, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"133","ipcGroupGid":"3000"},)"
        R"({"ipcGroupId":"456","ipcGroupGid":"4000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    EXPECT_EQ(ret, 0);

    rmdir("/mnt/sandbox/shm/100/group/133");
    rmdir("/mnt/sandbox/shm/100/group/456");
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_12
 * @tc.desc: Test MountIPCGroup with first item valid, second item invalid
 *           Branch: HandleArrayForeach aborts on second item → return -1
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_12, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"140","ipcGroupGid":"3000"},)"
        R"({"ipcGroupId":"abc","ipcGroupGid":"4000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // item 0 valid → return 0, item 1 non-numeric groupId → return -1 → abort
    EXPECT_EQ(ret, -1);

    rmdir("/mnt/sandbox/shm/100/group/140");
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_13
 * @tc.desc: Test MountIPCGroup with ipcGroupGid as non-numeric string
 *           Branch: ProcessIPCGroupItem atoi == 0 → groupGid == 0 → return -1, abort
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_13, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"312","ipcGroupGid":"not_a_number"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // ProcessIPCGroupItem: atoi == 0 → groupGid == 0 → return -1, abort
    EXPECT_EQ(ret, -1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_14
 * @tc.desc: Test MountIPCGroup with non-numeric ipcGroupId
 *           Branch: ProcessIPCGroupItem find_first_not_of != npos → return -1, abort
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_14, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"123abc","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // ProcessIPCGroupItem: non-numeric groupId → return -1, abort
    EXPECT_EQ(ret, -1);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_15
 * @tc.desc: Test MountIPCGroup with appProperty missing DAC info
 *           Branch: dacInfo == nullptr → cJSON_Delete + return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_15, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    // MSG_DUMP: valid msgType but not spawn type → CreateMsg skips AddDacInfo → no TLV_DAC_INFO
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_DUMP, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"316","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // dacInfo == nullptr → cJSON_Delete + return 0, HandleArrayForeach not reached
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_16
 * @tc.desc: Test MountIPCGroup with pre-existing src dir owned by non-root
 *           Branch: EnsureDirWithMode stat ok, uid mismatch → recreate → mount → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_16, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"3017","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    // Pre-create src dir with wrong owner (uid=1000, not root)
    std::string srcPath = "/mnt/sandbox/shm/100/group/3017";
    SandboxCommon::CreateDirRecursive(srcPath, SandboxCommonDef::FILE_MODE);
    chown(srcPath.c_str(), 1000, 1000);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // EnsureDirWithMode: stat ok, uid=1000 != 0 → recreate → mount → ret=0
    EXPECT_EQ(ret, 0);

    rmdir(srcPath.c_str());
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_17
 * @tc.desc: Test MountIPCGroup with pre-existing src dir, correct uid but wrong gid
 *           Branch: EnsureDirWithMode stat ok, gid mismatch → recreate → mount → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_17, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"3018","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    // Pre-create src dir as root → uid=0, gid=0 (gid != groupGid 3000)
    std::string srcPath = "/mnt/sandbox/shm/100/group/3018";
    SandboxCommon::CreateDirRecursive(srcPath, SandboxCommonDef::FILE_MODE);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // EnsureDirWithMode: stat ok, uid==0, gid=0 != 3000 → recreate → mount → ret=0
    EXPECT_EQ(ret, 0);

    rmdir(srcPath.c_str());
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_18
 * @tc.desc: Test MountIPCGroup with pre-existing src dir, correct owner but wrong mode
 *           Branch: EnsureDirWithMode stat ok, mode mismatch → recreate → mount → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_18, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"3019","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    // Pre-create src dir with correct owner but wrong mode
    std::string srcPath = "/mnt/sandbox/shm/100/group/3019";
    SandboxCommon::CreateDirRecursive(srcPath, SandboxCommonDef::FILE_MODE);
    chown(srcPath.c_str(), 0, 3000);
    chmod(srcPath.c_str(), 0755);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // EnsureDirWithMode: stat ok, uid==0, gid==3000, mode=0755 != 01771 → recreate → mount → ret=0
    EXPECT_EQ(ret, 0);

    rmdir(srcPath.c_str());
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_19
 * @tc.desc: Test MountIPCGroup with pre-existing src dir, all metadata correct
 *           Branch: EnsureDirWithMode stat ok, all match → skip creation → mount → return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_19, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"3020","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    // Pre-create src dir with fully correct metadata
    std::string srcPath = "/mnt/sandbox/shm/100/group/3020";
    SandboxCommon::CreateDirRecursive(srcPath, SandboxCommonDef::FILE_MODE);
    chown(srcPath.c_str(), 0, 3000);
    chmod(srcPath.c_str(), SandboxCommonDef::IPC_GROUP_SRC_PATH_MODE);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // EnsureDirWithMode: stat ok, all match → skip creation → mount → ret=0
    EXPECT_EQ(ret, 0);

    rmdir(srcPath.c_str());
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_20
 * @tc.desc: Test MountIPCGroup with mount failure (non-fatal)
 *           Branch: ProcessIPCGroupItem mount fails → log only, return 0 (continue)
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_20, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo =
        R"([{"ipcGroupId":"3021","ipcGroupGid":"3000"}])";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    // Configure MountStub to return failure:
    // originPath must MATCH the actual srcPath so the if-check passes,
    // then destinationPath mismatches → result=0 → errno=-EINVAL → mount fails
    StubNode *node = GetStubNode(STUB_MOUNT);
    MountTestArg mountArg = {
        "/mnt/sandbox/shm/100/group/3021",  // matches actual srcPath
        "/wrong_dest",  // mismatches actual destPath → failure
        "",             // fsType (non-NULL, printf-safe)
        0,              // mountFlags (mismatches MS_REC|MS_BIND)
        "",             // options (non-NULL, printf-safe)
        0               // mountSharedFlag
    };
    node->arg = &mountArg;
    node->flags |= STUB_NEED_CHECK;

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // mount fails → log only, return 0 (continue) → ret stays 0
    EXPECT_EQ(ret, 0);

    // Restore MountStub
    node->flags &= ~STUB_NEED_CHECK;
    node->arg = nullptr;

    // Cleanup
    rmdir("/mnt/sandbox/shm/100/group/3021");
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: MountIPCGroup_21
 * @tc.desc: Test MountIPCGroup with IPCGroup ext info not an array
 *           Branch: !cJSON_IsArray(ipcGroupRoot) → cJSON_Delete + return 0
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, MountIPCGroup_21, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(
        clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);

    const char *appGroupInfo = R"({})";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_IPC_GROUP, appGroupInfo);
    ASSERT_EQ(ret, 0);

    AppSpawningCtx *appProperty = g_testHelperCore.GetAppProperty(
        clientHandle, reqHandle);
    ASSERT_NE(appProperty, nullptr);

    std::string sandboxPackagePath = "/mnt/sandbox/100/com.test.app";
    ret = AppSpawn::SandboxCore::MountIPCGroup(appProperty, sandboxPackagePath);
    // !cJSON_IsArray → cJSON_Delete + return 0
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

// ==================== 普通权限 Debug 相关测试 ====================

/**
 * @tc.name: DoInstallDebugPermissionPoints_01
 * @tc.desc: Test debug install permission points with null json
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoInstallDebugPermissionPoints_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.debug.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    cJSON *debugJson = nullptr;
    int ret = AppSpawn::SandboxCore::DoInstallDebugPermissionPoints(appProperty, debugJson);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoInstallDebugPermissionPoints_02
 * @tc.desc: Test debug install permission points with valid config but app does NOT have
 *           the permission (unregistered name), should skip mount
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoInstallDebugPermissionPoints_02, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.debug.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    // Use an unregistered permission name so CheckAppPermissionFlagSet returns 0
    // (app does NOT have the permission), the mount is skipped.
    const char *configStr = R"({
        "permission": [
            {
                "ohos.permission.UNREGISTERED_TEST": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/debug/src",
                            "sandbox-path": "/data/test/debug/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    int ret = AppSpawn::SandboxCore::DoInstallDebugPermissionPoints(appProperty, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoInstallDebugPermissionPoints_03
 * @tc.desc: Test debug install permission points with valid config and app HAS the
 *           registered permission (FILE_ACCESS_MANAGER), should mount
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoInstallDebugPermissionPoints_03, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.debug.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    // Set the permission flag for a registered permission so that
    // CheckAppPermissionFlagSet returns non-zero (app has the permission).
    int index = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index));
    EXPECT_EQ(ret, 0);

    const char *configStr = R"({
        "permission": [
            {
                "ohos.permission.FILE_ACCESS_MANAGER": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/debug/src",
                            "sandbox-path": "/data/test/debug/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    ret = AppSpawn::SandboxCore::DoInstallDebugPermissionPoints(appProperty, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoInstallDebugPermissionPoints_04
 * @tc.desc: Test debug install permission points with empty permission child (no array items),
 *           permissionMountPaths is null, should skip
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoInstallDebugPermissionPoints_04, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.debug.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    // The permission child value is an empty object {}, so cJSON_GetArrayItem(child, 0)
    // returns null and the !permissionMountPaths branch is taken.
    // Set the permission flag so that CheckAppPermissionFlagSet returns non-zero,
    // allowing the code to reach the !permissionMountPaths check.
    int index = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index));
    EXPECT_EQ(ret, 0);

    const char *configStr = R"({
        "permission": [
            {
                "ohos.permission.FILE_ACCESS_MANAGER": {}
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    ret = AppSpawn::SandboxCore::DoInstallDebugPermissionPoints(appProperty, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoUninstallDebugPermissionPoints_01
 * @tc.desc: Test debug uninstall permission points with null json
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoUninstallDebugPermissionPoints_01, TestSize.Level0)
{
    std::vector<std::string> bundleList;
    cJSON *debugJson = nullptr;
    int ret = AppSpawn::SandboxCore::DoUninstallDebugPermissionPoints(bundleList, debugJson);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: DoUninstallDebugPermissionPoints_02
 * @tc.desc: Test debug uninstall permission points with valid config and mount-paths,
 *           should uninstall
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoUninstallDebugPermissionPoints_02, TestSize.Level0)
{
    std::vector<std::string> bundleList;
    bundleList.push_back("com.ohos.test.app");

    const char *configStr = R"({
        "permission": [
            {
                "ohos.permission.FILE_ACCESS_MANAGER": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/debug/src",
                            "sandbox-path": "/data/test/debug/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    int ret = AppSpawn::SandboxCore::DoUninstallDebugPermissionPoints(bundleList, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
}

/**
 * @tc.name: DoUninstallDebugPermissionPoints_03
 * @tc.desc: Test debug uninstall permission points with empty permission child (no array items),
 *           permissionMountPaths is null, should skip
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoUninstallDebugPermissionPoints_03, TestSize.Level0)
{
    std::vector<std::string> bundleList;
    bundleList.push_back("com.ohos.test.app");

    // The permission child value is an empty object {}, so cJSON_GetArrayItem(child, 0)
    // returns null and the !permissionMountPaths branch is taken.
    const char *configStr = R"({
        "permission": [
            {
                "ohos.permission.TEST_INVERTED": {}
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    int ret = AppSpawn::SandboxCore::DoUninstallDebugPermissionPoints(bundleList, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
}

// ==================== 反向权限文件相关测试 ====================

/**
 * @tc.name: DoSandboxFileInvertedPermissionBind_01
 * @tc.desc: Test inverted-permission file binding with null config
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxFileInvertedPermissionBind_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.inverted.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    cJSON *wholeConfig = nullptr;
    int ret = AppSpawn::SandboxCore::DoSandboxFileInvertedPermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFileInvertedPermissionBind_02
 * @tc.desc: Test inverted-permission file binding with valid config (app without permission, should mount)
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxFileInvertedPermissionBind_02, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.inverted.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.TEST_INVERTED": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/inverted/src",
                            "sandbox-path": "/data/test/inverted/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *wholeConfig = cJSON_Parse(configStr);
    ASSERT_NE(wholeConfig, nullptr);
    int ret = AppSpawn::SandboxCore::DoSandboxFileInvertedPermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFileInvertedPermissionBind_03
 * @tc.desc: Test inverted-permission with app having the permission (should skip mount)
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxFileInvertedPermissionBind_03, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.inverted.app");
    g_testHelperCore.SetTestApl("normal");
    std::vector<const char *> &permissions = g_testHelperCore.GetPermissions();
    permissions.push_back("ohos.permission.FILE_CROSS_APP");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.FILE_CROSS_APP": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/inverted/src",
                            "sandbox-path": "/data/test/inverted/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *wholeConfig = cJSON_Parse(configStr);
    ASSERT_NE(wholeConfig, nullptr);
    int ret = AppSpawn::SandboxCore::DoSandboxFileInvertedPermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFileInvertedPermissionBind_04
 * @tc.desc: Test inverted-permission with app having a registered permission (FILE_ACCESS_MANAGER),
 *           should skip mount and cover line 366 TRUE branch
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxFileInvertedPermissionBind_04, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.inverted.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    // Set the permission flag for a registered permission so that
    // CheckAppPermissionFlagSet returns non-zero (app has the permission).
    int index = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index));
    EXPECT_EQ(ret, 0);

    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.FILE_ACCESS_MANAGER": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/inverted/src",
                            "sandbox-path": "/data/test/inverted/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *wholeConfig = cJSON_Parse(configStr);
    ASSERT_NE(wholeConfig, nullptr);
    ret = AppSpawn::SandboxCore::DoSandboxFileInvertedPermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFileInvertedPermissionBind_05
 * @tc.desc: Test inverted-permission with empty permission child (no mount-paths array),
 *           permissionMountPaths is null, cover line 372 TRUE branch
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoSandboxFileInvertedPermissionBind_05, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.inverted.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    // The permission child value is an empty object {}, so cJSON_GetArrayItem(child, 0)
    // returns null and the !permissionMountPaths branch (line 372) is taken.
    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.TEST_INVERTED": {}
            }
        ]
    })";
    cJSON *wholeConfig = cJSON_Parse(configStr);
    ASSERT_NE(wholeConfig, nullptr);
    int ret = AppSpawn::SandboxCore::DoSandboxFileInvertedPermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: SetInvertedPermissionAppSandboxProperty__01
 * @tc.desc: Test inverted-permission app sandbox property setting with null config
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, SetInvertedPermissionAppSandboxProperty__01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.inverted.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    cJSON *config = nullptr;
    int ret = AppSpawn::SandboxCore::SetInvertedPermissionAppSandboxProperty_(appProperty, config);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: SetInvertedPermissionAppSandboxProperty_01
 * @tc.desc: Test inverted-permission app sandbox property setting
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, SetInvertedPermissionAppSandboxProperty_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.inverted.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    int ret = AppSpawn::SandboxCore::SetInvertedPermissionAppSandboxProperty(appProperty);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoInstallDebugInvertedPermissionPoints_01
 * @tc.desc: Test debug install inverted-permission points with null json
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoInstallDebugInvertedPermissionPoints_01, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.debug.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    cJSON *debugJson = nullptr;
    int ret = AppSpawn::SandboxCore::DoInstallDebugInvertedPermissionPoints(appProperty, debugJson);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoInstallDebugInvertedPermissionPoints_02
 * @tc.desc: Test debug install inverted-permission points with valid config but app does NOT
 *           have the permission (unregistered name), should mount (inverted logic)
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoInstallDebugInvertedPermissionPoints_02, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.debug.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    // Use an unregistered permission name so CheckAppPermissionFlagSet returns 0
    // (app does NOT have the permission), inverted-permission proceeds to mount.
    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.UNREGISTERED_TEST": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/debug/src",
                            "sandbox-path": "/data/test/debug/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    int ret = AppSpawn::SandboxCore::DoInstallDebugInvertedPermissionPoints(appProperty, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoInstallDebugInvertedPermissionPoints_03
 * @tc.desc: Test debug install inverted-permission points with valid config and app HAS the
 *           registered permission (FILE_ACCESS_MANAGER), should skip (inverted logic)
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoInstallDebugInvertedPermissionPoints_03, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.debug.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    // Set the permission flag for a registered permission so that
    // CheckAppPermissionFlagSet returns non-zero (app has the permission),
    // inverted-permission skips the mount.
    int index = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index));
    EXPECT_EQ(ret, 0);

    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.FILE_ACCESS_MANAGER": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/debug/src",
                            "sandbox-path": "/data/test/debug/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    ret = AppSpawn::SandboxCore::DoInstallDebugInvertedPermissionPoints(appProperty, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoInstallDebugInvertedPermissionPoints_04
 * @tc.desc: Test debug install inverted-permission points with empty permission child (no array
 *           items), permissionMountPaths is null, should skip
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoInstallDebugInvertedPermissionPoints_04, TestSize.Level0)
{
    g_testHelperCore.SetProcessName("com.ohos.debug.app");
    g_testHelperCore.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyCore();
    ASSERT_NE(appProperty, nullptr);

    // The permission child value is an empty object {}, so cJSON_GetArrayItem(child, 0)
    // returns null and the !permissionMountPaths branch is taken.
    // Set the permission flag so that CheckAppPermissionFlagSet returns non-zero,
    // allowing the code to reach the !permissionMountPaths check.
    int index = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index));
    EXPECT_EQ(ret, 0);

    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.FILE_ACCESS_MANAGER": {}
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    ret = AppSpawn::SandboxCore::DoInstallDebugInvertedPermissionPoints(appProperty, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoUninstallDebugInvertedPermissionPoints_01
 * @tc.desc: Test debug uninstall inverted-permission points with null json
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoUninstallDebugInvertedPermissionPoints_01, TestSize.Level0)
{
    std::vector<std::string> bundleList;
    cJSON *debugJson = nullptr;
    int ret = AppSpawn::SandboxCore::DoUninstallDebugInvertedPermissionPoints(bundleList, debugJson);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: DoUninstallDebugInvertedPermissionPoints_02
 * @tc.desc: Test debug uninstall inverted-permission points with valid config and mount-paths,
 *           should uninstall
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoUninstallDebugInvertedPermissionPoints_02, TestSize.Level0)
{
    std::vector<std::string> bundleList;
    bundleList.push_back("com.ohos.test.app");

    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.FILE_ACCESS_MANAGER": {
                    "mount-paths": [
                        {
                            "src-path": "/data/test/debug/src",
                            "sandbox-path": "/data/test/debug/dest"
                        }
                    ]
                }
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    int ret = AppSpawn::SandboxCore::DoUninstallDebugInvertedPermissionPoints(bundleList, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
}

/**
 * @tc.name: DoUninstallDebugInvertedPermissionPoints_03
 * @tc.desc: Test debug uninstall inverted-permission points with empty permission child (no
 *           array items), permissionMountPaths is null, should skip
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxCoreTest, DoUninstallDebugInvertedPermissionPoints_03, TestSize.Level0)
{
    std::vector<std::string> bundleList;
    bundleList.push_back("com.ohos.test.app");

    // The permission child value is an empty object {}, so cJSON_GetArrayItem(child, 0)
    // returns null and the !permissionMountPaths branch is taken.
    const char *configStr = R"({
        "inverted-permission": [
            {
                "ohos.permission.TEST_INVERTED": {}
            }
        ]
    })";
    cJSON *debugJson = cJSON_Parse(configStr);
    ASSERT_NE(debugJson, nullptr);
    int ret = AppSpawn::SandboxCore::DoUninstallDebugInvertedPermissionPoints(bundleList, debugJson);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(debugJson);
}

} // namespace OHOS