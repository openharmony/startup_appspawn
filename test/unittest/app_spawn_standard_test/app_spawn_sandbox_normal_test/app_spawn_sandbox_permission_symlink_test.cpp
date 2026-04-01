/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <sys/stat.h>
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
AppSpawnTestHelper g_testHelperPermSymlink;

// ==================== JSON 配置常量 ====================

// 多权限配置（FILE_ACCESS_MANAGER + SANDBOX_ACCESS_MANAGER）
static const char *CONFIG_MULTI_PERMISSION = R"({
    "permission": [{
        "ohos.permission.FILE_ACCESS_MANAGER": [{
            "sandbox-switch": "ON",
            "gids": [1006, 1008],
            "mount-paths": [],
            "symbol-links": [{
                "target-name": "/system/bin",
                "link-name": "/bin",
                "check-action-status": "false"
            }]
        }],
        "ohos.permission.SANDBOX_ACCESS_MANAGER": [{
            "sandbox-switch": "ON",
            "gids": [1015],
            "mount-paths": [],
            "symbol-links": [{
                "target-name": "/system/lib",
                "link-name": "/lib",
                "check-action-status": "false"
            }]
        }]
    }]
})";

// 多权限配置（FILE_ACCESS_MANAGER + MANAGE_PRIVATE_PHOTOS）
static const char *CONFIG_MULTI_PERMISSION2 = R"({
    "permission": [{
        "ohos.permission.FILE_ACCESS_MANAGER": [{
            "sandbox-switch": "ON",
            "gids": [1006, 1008],
            "mount-paths": [],
            "symbol-links": [{
                "target-name": "/system/bin",
                "link-name": "/bin",
                "check-action-status": "false"
            }]
        }],
        "ohos.permission.MANAGE_PRIVATE_PHOTOS": [{
            "sandbox-switch": "ON",
            "gids": [1015],
            "mount-paths": [],
            "symbol-links": [{
                "target-name": "/system/lib",
                "link-name": "/lib",
                "check-action-status": "false"
            }]
        }]
    }]
})";

class AppSpawnSandboxPermissionSymlinkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppSpawnSandboxPermissionSymlinkTest::SetUpTestCase()
{
    // Load permissions for testing permission-based features
    LoadPermission(CLIENT_FOR_APPSPAWN);
}

void AppSpawnSandboxPermissionSymlinkTest::TearDownTestCase()
{
    // Clean up permissions
    DeletePermission(CLIENT_FOR_APPSPAWN);
}

void AppSpawnSandboxPermissionSymlinkTest::SetUp()
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

void AppSpawnSandboxPermissionSymlinkTest::TearDown()
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

static AppSpawningCtx *GetTestAppPropertyPermSymlink()
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create reqMgr");
    AppSpawnReqMsgHandle reqHandle = g_testHelperPermSymlink.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, return nullptr, "Failed to create req");
    return g_testHelperPermSymlink.GetAppProperty(clientHandle, reqHandle);
}

// ==================== Symbol-Links Permission 测试用例 ====================

/**
 * @tc.name: DoSandboxFilePermissionSymlink_01
 * @tc.desc: 测试有权限时 symbol-links 创建成功
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxPermissionSymlinkTest, DoSandboxFilePermissionSymlink_01, TestSize.Level0)
{
    g_testHelperPermSymlink.SetProcessName("com.ohos.symlink.test");
    g_testHelperPermSymlink.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyPermSymlink();
    ASSERT_NE(appProperty, nullptr);

    // 设置权限
    int index = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index));
    EXPECT_EQ(ret, 0);

    // 计算沙箱根路径
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(
        GetAppProperty(appProperty, TLV_DAC_INFO));
    ASSERT_NE(dacInfo, nullptr);
    const char *bundleName = GetBundleName(appProperty);
    ASSERT_NE(bundleName, nullptr);
    std::string sandboxRoot = "/mnt/sandbox/" + std::to_string(dacInfo->uid / UID_BASE) + "/" + bundleName;

    // 创建沙箱根目录
    SandboxCommon::CreateDirRecursive(sandboxRoot, 0755);
    // 清理可能残留的符号链接
    unlink((sandboxRoot + "/bin").c_str());
    unlink((sandboxRoot + "/lib").c_str());

    const char *configStr = R"({
        "permission": [{
            "ohos.permission.FILE_ACCESS_MANAGER": [{
                "sandbox-switch": "ON",
                "gids": [1006, 1008],
                "mount-paths": [],
                "symbol-links": [{
                    "target-name": "/system/bin",
                    "link-name": "/bin",
                    "check-action-status": "false"
                }, {
                    "target-name": "/system/lib",
                    "link-name": "/lib",
                    "check-action-status": "false"
                }]
            }]
        }]
    })";

    cJSON *wholeConfig = cJSON_Parse(configStr);
    ASSERT_NE(wholeConfig, nullptr);

    // 执行测试
    ret = AppSpawn::SandboxCore::DoSandboxFilePermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    // 验证符号链接是否创建成功
    struct stat st = {};
    bool binExists = (lstat((sandboxRoot + "/bin").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    bool libExists = (lstat((sandboxRoot + "/lib").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    EXPECT_TRUE(binExists) << "symlink /bin should exist";
    EXPECT_TRUE(libExists) << "symlink /lib should exist";

    // 清理测试环境
    unlink((sandboxRoot + "/bin").c_str());
    unlink((sandboxRoot + "/lib").c_str());

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFilePermissionSymlink_02
 * @tc.desc: 测试配置中有多个权限，但应用只有一个权限时，只为有权限的创建符号链接
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxPermissionSymlinkTest, DoSandboxFilePermissionSymlink_02, TestSize.Level0)
{
    g_testHelperPermSymlink.SetProcessName("com.ohos.symlink.partial");
    g_testHelperPermSymlink.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyPermSymlink();
    ASSERT_NE(appProperty, nullptr);

    // 只设置一个权限：FILE_ACCESS_MANAGER
    int index1 = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index1, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index1));
    EXPECT_EQ(ret, 0);

    // 计算沙箱根路径
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(
        GetAppProperty(appProperty, TLV_DAC_INFO));
    ASSERT_NE(dacInfo, nullptr);
    const char *bundleName = GetBundleName(appProperty);
    ASSERT_NE(bundleName, nullptr);
    std::string sandboxRoot = "/mnt/sandbox/" + std::to_string(dacInfo->uid / UID_BASE) + "/" + bundleName;

    // 创建沙箱根目录
    SandboxCommon::CreateDirRecursive(sandboxRoot, 0755);
    // 清理可能残留的符号链接
    unlink((sandboxRoot + "/bin").c_str());
    unlink((sandboxRoot + "/lib").c_str());

    // 使用全局配置常量
    cJSON *wholeConfig = cJSON_Parse(CONFIG_MULTI_PERMISSION);
    ASSERT_NE(wholeConfig, nullptr);

    // 执行测试
    ret = AppSpawn::SandboxCore::DoSandboxFilePermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    // 验证：只有 FILE_ACCESS_MANAGER 对应的符号链接被创建
    struct stat st = {};
    bool binExists = (lstat((sandboxRoot + "/bin").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    bool libExists = (lstat((sandboxRoot + "/lib").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    EXPECT_TRUE(binExists) << "symlink /bin should exist (has permission)";
    EXPECT_FALSE(libExists) << "symlink /lib should NOT exist (no permission)";

    // 清理测试环境
    unlink((sandboxRoot + "/bin").c_str());

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFilePermissionSymlink_03
 * @tc.desc: 测试应用没有权限时，不创建符号链接
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxPermissionSymlinkTest, DoSandboxFilePermissionSymlink_03, TestSize.Level0)
{
    g_testHelperPermSymlink.SetProcessName("com.ohos.symlink.noperm");
    g_testHelperPermSymlink.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyPermSymlink();
    ASSERT_NE(appProperty, nullptr);

    // 不设置任何权限

    // 计算沙箱根路径
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(
        GetAppProperty(appProperty, TLV_DAC_INFO));
    ASSERT_NE(dacInfo, nullptr);
    const char *bundleName = GetBundleName(appProperty);
    ASSERT_NE(bundleName, nullptr);
    std::string sandboxRoot = "/mnt/sandbox/" + std::to_string(dacInfo->uid / UID_BASE) + "/" + bundleName;

    // 创建沙箱根目录
    SandboxCommon::CreateDirRecursive(sandboxRoot, 0755);
    // 清理可能残留的符号链接
    unlink((sandboxRoot + "/bin").c_str());
    unlink((sandboxRoot + "/lib").c_str());

    const char *configStr = R"({
        "permission": [{
            "ohos.permission.SANDBOX_ACCESS_MANAGER": [{
                "sandbox-switch": "ON",
                "gids": [1006, 1008],
                "mount-paths": [],
                "symbol-links": [{
                    "target-name": "/system/bin",
                    "link-name": "/bin",
                    "check-action-status": "false"
                }, {
                    "target-name": "/system/lib",
                    "link-name": "/lib",
                    "check-action-status": "false"
                }]
            }]
        }]
    })";

    cJSON *wholeConfig = cJSON_Parse(configStr);
    ASSERT_NE(wholeConfig, nullptr);

    // 执行测试
    int ret = AppSpawn::SandboxCore::DoSandboxFilePermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    // 验证：没有权限时，符号链接不应该被创建
    struct stat st = {};
    bool binExists = (lstat((sandboxRoot + "/bin").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    bool libExists = (lstat((sandboxRoot + "/lib").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    EXPECT_FALSE(binExists) << "symlink /bin should NOT exist (no permission)";
    EXPECT_FALSE(libExists) << "symlink /lib should NOT exist (no permission)";

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFilePermissionSymlink_04
 * @tc.desc: 测试配置为空（没有symbol-links）时不创建符号链接
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxPermissionSymlinkTest, DoSandboxFilePermissionSymlink_04, TestSize.Level0)
{
    g_testHelperPermSymlink.SetProcessName("com.ohos.symlink.empty");
    g_testHelperPermSymlink.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyPermSymlink();
    ASSERT_NE(appProperty, nullptr);

    // 设置权限
    int index = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index));
    EXPECT_EQ(ret, 0);

    // 计算沙箱根路径
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(
        GetAppProperty(appProperty, TLV_DAC_INFO));
    ASSERT_NE(dacInfo, nullptr);
    const char *bundleName = GetBundleName(appProperty);
    ASSERT_NE(bundleName, nullptr);
    std::string sandboxRoot = "/mnt/sandbox/" + std::to_string(dacInfo->uid / UID_BASE) + "/" + bundleName;

    // 创建沙箱根目录
    SandboxCommon::CreateDirRecursive(sandboxRoot, 0755);
    // 清理可能残留的符号链接
    unlink((sandboxRoot + "/bin").c_str());
    unlink((sandboxRoot + "/lib").c_str());

    // 配置中没有 symbol-links
    const char *configStr = R"({
        "permission": [{
            "ohos.permission.FILE_ACCESS_MANAGER": [{
                "sandbox-switch": "ON",
                "gids": [1006, 1008],
                "mount-paths": []
            }]
        }]
    })";

    cJSON *wholeConfig = cJSON_Parse(configStr);
    ASSERT_NE(wholeConfig, nullptr);

    // 执行测试
    ret = AppSpawn::SandboxCore::DoSandboxFilePermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    // 验证：没有配置符号链接时，不应该创建
    struct stat st = {};
    bool binExists = (lstat((sandboxRoot + "/bin").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    bool libExists = (lstat((sandboxRoot + "/lib").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    EXPECT_FALSE(binExists) << "symlink /bin should NOT exist (not configured)";
    EXPECT_FALSE(libExists) << "symlink /lib should NOT exist (not configured)";

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: DoSandboxFilePermissionSymlink_05
 * @tc.desc: 测试有多个权限时，为所有有权限的创建符号链接
 * @tc.type: FUNC
 * @tc.require: issueI5NTX6
 */
HWTEST_F(AppSpawnSandboxPermissionSymlinkTest, DoSandboxFilePermissionSymlink_05, TestSize.Level0)
{
    g_testHelperPermSymlink.SetProcessName("com.ohos.symlink.multi");
    g_testHelperPermSymlink.SetTestApl("normal");

    AppSpawningCtx *appProperty = GetTestAppPropertyPermSymlink();
    ASSERT_NE(appProperty, nullptr);

    // 设置两个权限
    int index1 = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    ASSERT_GE(index1, 0);
    int ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index1));
    EXPECT_EQ(ret, 0);

    int index2 = GetPermissionIndex(nullptr, "ohos.permission.MANAGE_PRIVATE_PHOTOS");
    ASSERT_GE(index2, 0);
    ret = SetAppPermissionFlags(appProperty, static_cast<uint32_t>(index2));
    EXPECT_EQ(ret, 0);

    // 计算沙箱根路径
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(
        GetAppProperty(appProperty, TLV_DAC_INFO));
    ASSERT_NE(dacInfo, nullptr);
    const char *bundleName = GetBundleName(appProperty);
    ASSERT_NE(bundleName, nullptr);
    std::string sandboxRoot = "/mnt/sandbox/" + std::to_string(dacInfo->uid / UID_BASE) + "/" + bundleName;

    // 创建沙箱根目录
    SandboxCommon::CreateDirRecursive(sandboxRoot, 0755);

    // 使用全局配置常量
    cJSON *wholeConfig = cJSON_Parse(CONFIG_MULTI_PERMISSION2);
    ASSERT_NE(wholeConfig, nullptr);

    // 执行测试
    ret = AppSpawn::SandboxCore::DoSandboxFilePermissionBind(appProperty, wholeConfig);
    EXPECT_EQ(ret, 0);

    // 验证：两个符号链接都被创建
    struct stat st = {};
    bool binExists = (lstat((sandboxRoot + "/bin").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    bool libExists = (lstat((sandboxRoot + "/lib").c_str(), &st) == 0 && S_ISLNK(st.st_mode));
    EXPECT_TRUE(binExists) << "symlink /bin should exist";
    EXPECT_TRUE(libExists) << "symlink /lib should exist";

    // 清理测试环境
    unlink((sandboxRoot + "/bin").c_str());
    unlink((sandboxRoot + "/lib").c_str());

    cJSON_Delete(wholeConfig);
    DeleteAppSpawningCtx(appProperty);
}

} // namespace OHOS
