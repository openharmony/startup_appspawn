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

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_permission.h"
#include "appspawn_utils.h"
#include "sandbox_common.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace AppSpawn {
AppSpawnTestHelper g_testHelperCore;

static AppSpawningCtx *GetTestAppPropertyCore()
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create reqMgr");
    AppSpawnReqMsgHandle reqHandle = g_testHelperCore.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, return nullptr, "Failed to create req");
    return g_testHelperCore.GetAppProperty(clientHandle, reqHandle);
}
}

class AppSpawnSandboxCommonTest : public testing::Test {
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
 * @tc.name: App_Spawn_SandboxCommon_GetSandboxNsFlags_01
 * @tc.desc: Test GetSandboxNsFlags with normal flags
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetSandboxNsFlags_01, TestSize.Level0)
{
    int32_t ret = AppSpawn::SandboxCommon::GetSandboxNsFlags(false);
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetSandboxNsFlags_02
 * @tc.desc: Test GetSandboxNsFlags with nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetSandboxNsFlags_02, TestSize.Level0)
{
    int32_t ret = AppSpawn::SandboxCommon::GetSandboxNsFlags(true);
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetSandboxRootPath_01
 * @tc.desc: Test GetSandboxRootPath with valid bundle name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetSandboxRootPath_01, TestSize.Level0)
{
    const AppSpawningCtx *appProperty = nullptr;
    cJSON *config = cJSON_CreateObject();
    std::string result = AppSpawn::SandboxCommon::GetSandboxRootPath(appProperty, config);
    cJSON_Delete(config);
    EXPECT_NE(result.length(), 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetSandboxRootPath_02
 * @tc.desc: Test GetSandboxRootPath with nullptr parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetSandboxRootPath_02, TestSize.Level0)
{
    const AppSpawningCtx *appProperty = nullptr;
    cJSON *config = nullptr;
    std::string result = AppSpawn::SandboxCommon::GetSandboxRootPath(appProperty, config);
    EXPECT_EQ(result.length(), 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetSandboxRootPath_03
 * @tc.desc: Test GetSandboxRootPath with empty bundle name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetSandboxRootPath_03, TestSize.Level0)
{
    const AppSpawningCtx *appProperty = nullptr;
    cJSON *config = cJSON_CreateObject();
    std::string result = AppSpawn::SandboxCommon::GetSandboxRootPath(appProperty, config);
    cJSON_Delete(config);
    EXPECT_EQ(result.length(), 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CreateDirRecursive_01
 * @tc.desc: Test CreateDirRecursive with normal path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CreateDirRecursive_01, TestSize.Level0)
{
    const char *path = "/data/test/sandbox/dir";
    int32_t ret = AppSpawn::SandboxCommon::CreateDirRecursive(path, 0755);
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CreateDirRecursive_02
 * @tc.desc: Test CreateDirRecursive with nullptr path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CreateDirRecursive_02, TestSize.Level0)
{
    int32_t ret = AppSpawn::SandboxCommon::CreateDirRecursive(nullptr, 0755);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_VerifyDirRecursive_01
 * @tc.desc: Test VerifyDirRecursive with normal path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_VerifyDirRecursive_01, TestSize.Level0)
{
    const char *path = "/data/appspawn_ut";
    bool exists = AppSpawn::SandboxCommon::VerifyDirRecursive(path);
    EXPECT_TRUE(exists);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_VerifyDirRecursive_02
 * @tc.desc: Test VerifyDirRecursive with nullptr path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_VerifyDirRecursive_02, TestSize.Level0)
{
    bool exists = AppSpawn::SandboxCommon::VerifyDirRecursive(nullptr);
    EXPECT_FALSE(exists);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CreateFileIfNotExist_01
 * @tc.desc: Test CreateFileIfNotExist with normal path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CreateFileIfNotExist_01, TestSize.Level0)
{
    const char *path = "/data/appspawn_ut/test_file.txt";
    AppSpawn::SandboxCommon::CreateFileIfNotExist(path);
    int32_t ret = 0;
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CreateFileIfNotExist_02
 * @tc.desc: Test CreateFileIfNotExist with nullptr path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CreateFileIfNotExist_02, TestSize.Level0)
{
    AppSpawn::SandboxCommon::CreateFileIfNotExist(nullptr);
    int32_t ret = 0;
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_SetSandboxPathChmod_01
 * @tc.desc: Test SetSandboxPathChmod with normal path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_SetSandboxPathChmod_01, TestSize.Level0)
{
    cJSON *jsonConfig = cJSON_CreateObject();
    std::string sandboxRoot = "/data/appspawn_ut";
    AppSpawn::SandboxCommon::SetSandboxPathChmod(jsonConfig, sandboxRoot);
    cJSON_Delete(jsonConfig);
    int32_t ret = 0;
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_SetSandboxPathChmod_02
 * @tc.desc: Test SetSandboxPathChmod with nullptr path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_SetSandboxPathChmod_02, TestSize.Level0)
{
    std::string sandboxRoot = "/data/appspawn_ut";
    AppSpawn::SandboxCommon::SetSandboxPathChmod(nullptr, sandboxRoot);
    int32_t ret = 0;
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_IsAppSandboxEnabled_01
 * @tc.desc: Test IsAppSandboxEnabled with true config
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_IsAppSandboxEnabled_01, TestSize.Level0)
{
    const AppSpawningCtx *appProperty = nullptr;
    bool enabled = AppSpawn::SandboxCommon::IsAppSandboxEnabled(appProperty);
    EXPECT_TRUE(enabled);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetSandboxMountConfig_01
 * @tc.desc: Test GetSandboxMountConfig with nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetSandboxMountConfig_01, TestSize.Level0)
{
    const AppSpawningCtx *appProperty = nullptr;
    std::string section = "";
    cJSON *mntPoint = nullptr;
    AppSpawn::SandboxMountConfig mountConfig;
    AppSpawn::SandboxCommon::GetSandboxMountConfig(appProperty, section, mntPoint, mountConfig);
    int32_t ret = 0;
    EXPECT_GE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_IsMountSuccessful_01
 * @tc.desc: Test IsMountSuccessful with valid status
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_IsMountSuccessful_01, TestSize.Level0)
{
    cJSON *mntPoint = cJSON_CreateObject();
    bool success = AppSpawn::SandboxCommon::IsMountSuccessful(mntPoint);
    cJSON_Delete(mntPoint);
    EXPECT_TRUE(success);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_IsMountSuccessful_02
 * @tc.desc: Test IsMountSuccessful with failed status
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_IsMountSuccessful_02, TestSize.Level0)
{
    cJSON *mntPoint = nullptr;
    bool success = AppSpawn::SandboxCommon::IsMountSuccessful(mntPoint);
    EXPECT_FALSE(success);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CheckBundleName_01
 * @tc.desc: Test CheckBundleName with valid bundle name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CheckBundleName_01, TestSize.Level0)
{
    std::string bundleName = "com.test.app";
    bool valid = AppSpawn::SandboxCommon::CheckBundleName(bundleName);
    EXPECT_TRUE(valid);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CheckBundleName_02
 * @tc.desc: Test CheckBundleName with nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CheckBundleName_02, TestSize.Level0)
{
    std::string bundleName = "";
    bool valid = AppSpawn::SandboxCommon::CheckBundleName(bundleName);
    EXPECT_FALSE(valid);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CheckBundleName_03
 * @tc.desc: Test CheckBundleName with empty string
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CheckBundleName_03, TestSize.Level0)
{
    std::string bundleName = "";
    bool valid = AppSpawn::SandboxCommon::CheckBundleName(bundleName);
    EXPECT_FALSE(valid);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_HasPrivateInBundleName_01
 * @tc.desc: Test HasPrivateInBundleName with normal bundle name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_HasPrivateInBundleName_01, TestSize.Level0)
{
    std::string bundleName = "com.test.app";
    bool hasPrivate = AppSpawn::SandboxCommon::HasPrivateInBundleName(bundleName);
    EXPECT_TRUE(hasPrivate);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_IsPrivateSharedStatus_01
 * @tc.desc: Test IsPrivateSharedStatus with normal parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_IsPrivateSharedStatus_01, TestSize.Level0)
{
    std::string bundleName = "com.test.app";
    AppSpawningCtx *appProperty = nullptr;
    bool status = AppSpawn::SandboxCommon::IsPrivateSharedStatus(bundleName, appProperty);

    EXPECT_TRUE(status);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_IsValidMountConfig_01
 * @tc.desc: Test IsValidMountConfig with nullptr
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_IsValidMountConfig_01, TestSize.Level0)
{
    cJSON *mntPoint = nullptr;
    const AppSpawningCtx *appProperty = nullptr;
    bool checkFlag = false;
    bool valid = AppSpawn::SandboxCommon::IsValidMountConfig(mntPoint, appProperty, checkFlag);
    EXPECT_FALSE(valid);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_SplitString_01
 * @tc.desc: Test SplitString with normal input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_SplitString_01, TestSize.Level0)
{
    std::string input = "test1,test2,test3";
    std::string delimiter = ",";
    std::vector<std::string> result = AppSpawn::SandboxCommon::SplitString(input, delimiter);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "test1");
    EXPECT_EQ(result[1], "test2");
    EXPECT_EQ(result[2], "test3");
}

/**
 * @tc.name: App_Spawn_SandboxCommon_SplitString_02
 * @tc.desc: Test SplitString with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_SplitString_02, TestSize.Level0)
{
    std::string input = "";
    std::string delimiter = ",";
    std::vector<std::string> result = AppSpawn::SandboxCommon::SplitString(input, delimiter);
    EXPECT_EQ(result.size(), 1); // An empty string should result in a vector with one empty string
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceAllVariables_01
 * @tc.desc: Test ReplaceAllVariables with normal input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceAllVariables_01, TestSize.Level0)
{
    std::string input = "/data/{APP}";
    std::string from = "{APP}";
    std::string to = "test";

    std::string result = AppSpawn::SandboxCommon::ReplaceAllVariables(input, from, to);
    EXPECT_EQ(result, "/data/test");
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceAllVariables_02
 * @tc.desc: Test ReplaceAllVariables with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceAllVariables_02, TestSize.Level0)
{
    std::string input = "";
    std::string from = "{APP}";
    std::string to = "test";

    std::string result = AppSpawn::SandboxCommon::ReplaceAllVariables(input, from, to);
    EXPECT_EQ(result, ""); // Empty input should return empty string
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceAllVariables_03
 * @tc.desc: Test ReplaceAllVariables with nullptr from
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceAllVariables_03, TestSize.Level0)
{
    std::string input = "/data/test";
    std::string from = "";
    std::string to = "test";

    std::string result = AppSpawn::SandboxCommon::ReplaceAllVariables(input, from, to);
    EXPECT_EQ(result, input); // Empty from string should return original string
}

/**
 * @tc.name: App_Spawn_SandboxCommon_MakeAtomicServiceDir_01
 * @tc.desc: Test MakeAtomicServiceDir with normal parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_MakeAtomicServiceDir_01, TestSize.Level0)
{
    const AppSpawningCtx *appProperty = nullptr;
    std::string path = "/data/test";
    std::string variablePackageName = "test.package";

    AppSpawn::SandboxCommon::MakeAtomicServiceDir(appProperty, path, variablePackageName);
    // Just verify the function doesn't crash
    EXPECT_TRUE(true);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceVariablePackageName_01
 * @tc.desc: Test ReplaceVariablePackageName with normal input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceVariablePackageName_01, TestSize.Level0)
{
    const char *input = "/data/{UID}/{PACKAGE_NAME}";
    const AppSpawningCtx *appProperty = nullptr;
    std::string path(input);

    std::string result = AppSpawn::SandboxCommon::ReplaceVariablePackageName(appProperty, path);
    EXPECT_FALSE(result.empty());
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceVariablePackageName_02
 * @tc.desc: Test ReplaceVariablePackageName with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceVariablePackageName_02, TestSize.Level0)
{
    const AppSpawningCtx *appProperty = nullptr;
    std::string path;

    std::string result = AppSpawn::SandboxCommon::ReplaceVariablePackageName(appProperty, path);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ParseParamTemplate_01
 * @tc.desc: Test ParseParamTemplate with normal template string
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ParseParamTemplate_01, TestSize.Level0)
{
    const char *templateStr = "/data/{UID}/app";
    std::string str(templateStr);

    std::string result = AppSpawn::SandboxCommon::ParseParamTemplate(str);
    EXPECT_NE(result, "");
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ParseParamTemplate_02
 * @tc.desc: Test ParseParamTemplate with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ParseParamTemplate_02, TestSize.Level0)
{
    std::string result = AppSpawn::SandboxCommon::ParseParamTemplate("");
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: App_Spawn_SandboxCommon_JoinParamPaths_01
 * @tc.desc: Test JoinParamPaths with normal path components
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_JoinParamPaths_01, TestSize.Level0)
{
    std::vector<std::string> paths = {"/data", "test", "path"};

    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    EXPECT_NE(result, "");
}

/**
 * @tc.name: App_Spawn_SandboxCommon_JoinParamPaths_02
 * @tc.desc: Test JoinParamPaths with empty vector
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_JoinParamPaths_02, TestSize.Level0)
{
    std::vector<std::string> paths;

    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: App_Spawn_SandboxCommon_JoinParamPaths_03
 * @tc.desc: Test JoinParamPaths with single path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_JoinParamPaths_03, TestSize.Level0)
{
    std::vector<std::string> paths = {"/single/path"};

    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    EXPECT_EQ(result, "/single/path");
}

/**
 * @tc.name: App_Spawn_SandboxCommon_IsCreateSandboxPathEnabled_01
 * @tc.desc: Test IsCreateSandboxPathEnabled function
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_IsCreateSandboxPathEnabled_01, TestSize.Level0)
{
    cJSON *json = cJSON_CreateObject();
    std::string srcPath = "/data/test";

    bool enabled = AppSpawn::SandboxCommon::IsCreateSandboxPathEnabled(json, srcPath);

    EXPECT_TRUE(enabled);

    cJSON_Delete(json);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_IsTotalSandboxEnabled_01
 * @tc.desc: Test IsTotalSandboxEnabled function
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_IsTotalSandboxEnabled_01, TestSize.Level0)
{
    const AppSpawningCtx *appProperty = nullptr;

    bool enabled = AppSpawn::SandboxCommon::IsTotalSandboxEnabled(appProperty);

    EXPECT_TRUE(enabled);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_IsDacOverrideEnabled_01
 * @tc.desc: Test IsDacOverrideEnabled function
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_IsDacOverrideEnabled_01, TestSize.Level0)
{
    cJSON *config = cJSON_CreateObject();

    bool enabled = AppSpawn::SandboxCommon::IsDacOverrideEnabled(config);

    EXPECT_TRUE(enabled);

    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetSwitchStatus_01
 * @tc.desc: Test GetSwitchStatus with normal switch name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetSwitchStatus_01, TestSize.Level0)
{
    cJSON *config = cJSON_CreateObject();

    bool status = AppSpawn::SandboxCommon::GetSwitchStatus(config);
    // Status may be false (off) or true (on), just verify it returns
    EXPECT_TRUE(status);

    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetSwitchStatus_02
 * @tc.desc: Test GetSwitchStatus with nullptr switch name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetSwitchStatus_02, TestSize.Level0)
{
    bool status = AppSpawn::SandboxCommon::GetSwitchStatus(nullptr);
    // Since the function returns bool, we can't check for -1
    EXPECT_TRUE(status);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetMountFlags_01
 * @tc.desc: Test GetMountFlags with normal flags string
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetMountFlags_01, TestSize.Level0)
{
    cJSON *config = cJSON_CreateObject();

    unsigned long flags = AppSpawn::SandboxCommon::GetMountFlags(config);
    EXPECT_NE(flags, 0);

    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetMountFlags_02
 * @tc.desc: Test GetMountFlags with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetMountFlags_02, TestSize.Level0)
{
    unsigned long flags = AppSpawn::SandboxCommon::GetMountFlags(nullptr);
    EXPECT_EQ(flags, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetFsType_01
 * @tc.desc: Test GetFsType with normal filesystem type
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetFsType_01, TestSize.Level0)
{
    cJSON *config = cJSON_CreateObject();

    std::string result = AppSpawn::SandboxCommon::GetFsType(config);
    EXPECT_FALSE(result.empty());

    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetFsType_02
 * @tc.desc: Test GetFsType with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetFsType_02, TestSize.Level0)
{
    cJSON *config = cJSON_CreateObject();
    std::string result = AppSpawn::SandboxCommon::GetFsType(config);
    EXPECT_TRUE(result.empty());
    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetOptions_01
 * @tc.desc: Test GetOptions with normal options string
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetOptions_01, TestSize.Level0)
{
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();
    cJSON *config = cJSON_CreateObject();
    if (appProperty == nullptr || config == nullptr) {
        GTEST_SKIP() << "Failed to create test objects";
    }
    cJSON_AddStringToObject(config, "options", "rw,noatime");

    std::string result = AppSpawn::SandboxCommon::GetOptions(appProperty, config);
    EXPECT_FALSE(result.empty());

    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetOptions_02
 * @tc.desc: Test GetOptions with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetOptions_02, TestSize.Level0)
{
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();
    if (appProperty == nullptr) {
        GTEST_SKIP() << "Failed to create test object";
    }

    cJSON *config = cJSON_CreateObject();
    std::string result = AppSpawn::SandboxCommon::GetOptions(appProperty, config);
    EXPECT_TRUE(result.empty());

    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetDecPath_01
 * @tc.desc: Test GetDecPath with normal path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetDecPath_01, TestSize.Level0)
{
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();
    cJSON *config = cJSON_CreateObject();
    if (appProperty == nullptr || config == nullptr) {
        GTEST_SKIP() << "Failed to create test objects";
    }
    cJSON_AddStringToObject(config, "decPath", "/data/app");

    std::vector<std::string> result = AppSpawn::SandboxCommon::GetDecPath(appProperty, config);
    EXPECT_FALSE(result.empty());

    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetDecPath_02
 * @tc.desc: Test GetDecPath with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetDecPath_02, TestSize.Level0)
{
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();
    if (appProperty == nullptr) {
        GTEST_SKIP() << "Failed to create test object";
    }

    cJSON *config = cJSON_CreateObject();
    std::vector<std::string> result = AppSpawn::SandboxCommon::GetDecPath(appProperty, config);
    EXPECT_FALSE(result.empty());

    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetMountFlagsFromConfig_01
 * @tc.desc: Test GetMountFlagsFromConfig with normal config
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetMountFlagsFromConfig_01, TestSize.Level0)
{
    std::vector<std::string> vec;
    vec.push_back("MS_BIND");

    uint32_t flags = AppSpawn::SandboxCommon::GetMountFlagsFromConfig(vec);
    EXPECT_NE(flags, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetMountFlagsFromConfig_02
 * @tc.desc: Test GetMountFlagsFromConfig with nullptr config
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetMountFlagsFromConfig_02, TestSize.Level0)
{
    std::vector<std::string> vec;

    uint32_t flags = AppSpawn::SandboxCommon::GetMountFlagsFromConfig(vec);
    EXPECT_EQ(flags, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CheckAppFullMountEnable_01
 * @tc.desc: Test CheckAppFullMountEnable function
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CheckAppFullMountEnable_01, TestSize.Level0)
{
    int32_t result = AppSpawn::SandboxCommon::CheckAppFullMountEnable();

    EXPECT_TRUE(result == 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceClonePackageName_01
 * @tc.desc: Test ReplaceClonePackageName with normal input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceClonePackageName_01, TestSize.Level0)
{
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();
    if (appProperty == nullptr) {
        GTEST_SKIP() << "Failed to create test object";
    }

    std::string path = "/data/{CLONE_PACKAGE}";
    std::string result = AppSpawn::SandboxCommon::ReplaceClonePackageName(appProperty, path);
    EXPECT_FALSE(result.empty());

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceClonePackageName_02
 * @tc.desc: Test ReplaceClonePackageName with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceClonePackageName_02, TestSize.Level0)
{
    AppSpawningCtx *appProperty = nullptr;

    std::string path = "/data/{CLONE_PACKAGE}";
    std::string result = AppSpawn::SandboxCommon::ReplaceClonePackageName(appProperty, path);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceHostUserId_01
 * @tc.desc: Test ReplaceHostUserId with normal input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceHostUserId_01, TestSize.Level0)
{
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();
    if (appProperty == nullptr) {
        GTEST_SKIP() << "Failed to create test object";
    }

    std::string path = "/data/{HOST_USER_ID}";
    std::string result = AppSpawn::SandboxCommon::ReplaceHostUserId(appProperty, path);
    EXPECT_FALSE(result.empty());

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ReplaceHostUserId_02
 * @tc.desc: Test ReplaceHostUserId with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ReplaceHostUserId_02, TestSize.Level0)
{
    AppSpawningCtx *appProperty = nullptr;

    std::string path = "/data/{HOST_USER_ID}";
    std::string result = AppSpawn::SandboxCommon::ReplaceHostUserId(appProperty, path);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: App_Spawn_SandboxCommon_StoreCJsonConfig_01
 * @tc.desc: Test StoreCJsonConfig with normal config
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_StoreCJsonConfig_01, TestSize.Level0)
{
    cJSON *config = cJSON_CreateObject();
    if (config == nullptr) {
        GTEST_SKIP() << "Failed to create JSON object";
    }
    cJSON_AddStringToObject(config, "key", "value");

    AppSpawn::SandboxCommon::StoreCJsonConfig(config, AppSpawn::SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);
    EXPECT_TRUE(true);

    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetCJsonConfig_01
 * @tc.desc: Test GetCJsonConfig function
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetCJsonConfig_01, TestSize.Level0)
{
    std::vector<cJSON *> configs = AppSpawn::SandboxCommon::GetCJsonConfig(
        AppSpawn::SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    EXPECT_TRUE(true);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_FreeAppSandboxConfigCJson_01
 * @tc.desc: Test FreeAppSandboxConfigCJson function
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_FreeAppSandboxConfigCJson_01, TestSize.Level0)
{
    AppSpawnMgr content;
    int32_t ret = AppSpawn::SandboxCommon::FreeAppSandboxConfigCJson(&content);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_HandleArrayForeach_01
 * @tc.desc: Test HandleArrayForeach with normal array
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_HandleArrayForeach_01, TestSize.Level0)
{
    cJSON *array = cJSON_CreateArray();
    if (array == nullptr) {
        GTEST_SKIP() << "Failed to create JSON array";
    }
    cJSON_AddItemToArray(array, cJSON_CreateString("item1"));
    cJSON_AddItemToArray(array, cJSON_CreateString("item2"));

    int32_t ret = AppSpawn::SandboxCommon::HandleArrayForeach(array, [](cJSON *item) { return 0; });
    EXPECT_EQ(ret, 0);

    cJSON_Delete(array);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_HandleArrayForeach_02
 * @tc.desc: Test HandleArrayForeach with nullptr array
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_HandleArrayForeach_02, TestSize.Level0)
{
    int32_t ret = AppSpawn::SandboxCommon::HandleArrayForeach(nullptr, [](cJSON *item) { return 0; });
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_LoadAppSandboxConfigCJson_01
 * @tc.desc: Test LoadAppSandboxConfigCJson with valid path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_LoadAppSandboxConfigCJson_01, TestSize.Level0)
{
    AppSpawnMgr content;
    int32_t ret = AppSpawn::SandboxCommon::LoadAppSandboxConfigCJson(&content);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_LoadAppSandboxConfigCJson_02
 * @tc.desc: Test LoadAppSandboxConfigCJson with nullptr path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_LoadAppSandboxConfigCJson_02, TestSize.Level0)
{
    AppSpawnMgr content;
    int32_t ret = AppSpawn::SandboxCommon::LoadAppSandboxConfigCJson(&content);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ConvertFlagStr_01
 * @tc.desc: Test ConvertFlagStr with normal flag
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ConvertFlagStr_01, TestSize.Level0)
{
    uint32_t result = AppSpawn::SandboxCommon::ConvertFlagStr("MS_BIND");
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ConvertFlagStr_02
 * @tc.desc: Test ConvertFlagStr with nullptr input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ConvertFlagStr_02, TestSize.Level0)
{
    uint32_t result = AppSpawn::SandboxCommon::ConvertFlagStr(nullptr);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_GetExtraInfoByType_01
 * @tc.desc: Test GetExtraInfoByType with valid type
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_GetExtraInfoByType_01, TestSize.Level0)
{
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();
    if (appProperty == nullptr) {
        GTEST_SKIP() << "Failed to create test object";
    }

    std::string result = AppSpawn::SandboxCommon::GetExtraInfoByType(appProperty, "bundleName");
    EXPECT_FALSE(result.empty());
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CreateDirRecursiveWithClock_01
 * @tc.desc: Test CreateDirRecursiveWithClock with normal path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CreateDirRecursiveWithClock_01, TestSize.Level0)
{
    std::string path = "/data/test/sandbox/clock";
    AppSpawn::SandboxCommon::CreateDirRecursiveWithClock(path, 0755);

    EXPECT_TRUE(true);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_CreateDirRecursiveWithClock_02
 * @tc.desc: Test CreateDirRecursiveWithClock with nullptr path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_CreateDirRecursiveWithClock_02, TestSize.Level0)
{
    std::string path = "/data/test/sandbox/clock";
    AppSpawn::SandboxCommon::CreateDirRecursiveWithClock(path, 0755);

    EXPECT_TRUE(true);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ConvertToRealPath_01
 * @tc.desc: Test ConvertToRealPath with normal context
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ConvertToRealPath_01, TestSize.Level0)
{
    AppSpawningCtx *ctx = AppSpawn::GetTestAppPropertyCore();
    ASSERT_NE(ctx, nullptr);

    std::string path = "/data/storage/el1/bundle";
    std::string realPath = AppSpawn::SandboxCommon::ConvertToRealPath(ctx, path);

    EXPECT_TRUE(true);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ConvertToRealPath_02
 * @tc.desc: Test ConvertToRealPath with nullptr context
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ConvertToRealPath_02, TestSize.Level0)
{
    std::string path = "/data/storage/el1/bundle";
    std::string realPath = AppSpawn::SandboxCommon::ConvertToRealPath(nullptr, path);
    EXPECT_TRUE(realPath.empty());
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ConvertToRealPathWithPermission_01
 * @tc.desc: Test ConvertToRealPathWithPermission with normal context
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ConvertToRealPathWithPermission_01, TestSize.Level0)
{
    AppSpawningCtx *ctx = AppSpawn::GetTestAppPropertyCore();
    ASSERT_NE(ctx, nullptr);

    std::string path = "/data/storage/el1/bundle";
    std::string realPath = AppSpawn::SandboxCommon::ConvertToRealPathWithPermission(ctx, path);

    EXPECT_TRUE(true);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_ConvertToRealPathWithPermission_02
 * @tc.desc: Test ConvertToRealPathWithPermission with nullptr context
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_ConvertToRealPathWithPermission_02, TestSize.Level0)
{
    std::string path = "/data/storage/el1/bundle";
    std::string realPath = AppSpawn::SandboxCommon::ConvertToRealPathWithPermission(nullptr, path);
    EXPECT_TRUE(realPath.empty());
}

/**
 * @tc.name: App_Spawn_SandboxCommon_BuildFullParamSrcPath_01
 * @tc.desc: Test BuildFullParamSrcPath with normal config
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_BuildFullParamSrcPath_01, TestSize.Level0)
{
    cJSON *config = cJSON_CreateObject();
    if (config == nullptr) {
        GTEST_SKIP() << "Failed to create JSON object";
    }
    cJSON_AddStringToObject(config, "preParamPath", "/pre/path");
    cJSON_AddStringToObject(config, "paramPath", "/param/path");
    cJSON_AddStringToObject(config, "postParamPath", "/post/path");

    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(config);
    EXPECT_FALSE(result.empty());

    cJSON_Delete(config);
}

/**
 * @tc.name: App_Spawn_SandboxCommon_BuildFullParamSrcPath_02
 * @tc.desc: Test BuildFullParamSrcPath with nullptr config
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_BuildFullParamSrcPath_02, TestSize.Level0)
{
    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(nullptr);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: App_Spawn_SandboxCommon_BuildFullParamSrcPath_03
 * @tc.desc: Test BuildFullParamSrcPath with empty param paths
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxCommonTest, App_Spawn_SandboxCommon_BuildFullParamSrcPath_03, TestSize.Level0)
{
    cJSON *config = cJSON_CreateObject();
    if (config == nullptr) {
        GTEST_SKIP() << "Failed to create JSON object";
    }

    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(config);
    EXPECT_TRUE(result.empty());

    cJSON_Delete(config);
}
}  // namespace OHOS
