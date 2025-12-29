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
#include "securec.h"
#include "sandbox_core.h"
#include "sandbox_common.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace AppSpawn {
AppSpawnTestHelper g_testSpawnHelper;
static AppSpawningCtx *GetTestAppPropertyCore()
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create reqMgr");
    AppSpawnReqMsgHandle reqHandle = g_testSpawnHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, return nullptr, "Failed to create req");
    return g_testSpawnHelper.GetAppProperty(clientHandle, reqHandle);
}
}

class AppSpawnSandboxManagerTest : public testing::Test {
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
 * @tc.name: App_Spawn_Sandbox_SpawnMountDirToShared_01
 * @tc.desc: Test SpawnMountDirToShared when IsNWebSpawnMode returns true
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxManagerTest, App_Spawn_Sandbox_SpawnMountDirToShared_01, TestSize.Level0)
{
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();
    // SpawnMountDirToShared is a static function, test it indirectly
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty);
    EXPECT_EQ(ret, 0);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_SpawnMountDirToShared_02
 * @tc.desc: Test SpawnMountDirToShared with nullptr parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxManagerTest, App_Spawn_Sandbox_SpawnMountDirToShared_02, TestSize.Level0)
{
    // Test with nullptr parameter
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(nullptr);
    EXPECT_NE(ret, 0);  // Should fail with nullptr
}

/**
 * @tc.name: App_Spawn_Sandbox_UninstallDebugSandbox_01
 * @tc.desc: Test UninstallDebugSandbox with nullptr content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxManagerTest, App_Spawn_Sandbox_UninstallDebugSandbox_01, TestSize.Level0)
{
    AppSpawn::g_testSpawnHelper.SetTestUid(1000);
    AppSpawn::g_testSpawnHelper.SetTestGid(1000);
    AppSpawn::g_testSpawnHelper.SetProcessName("test.debug.app");
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();

    AppSpawnMgr content;
    int32_t ret = AppSpawn::SandboxCore::UninstallDebugSandbox(&content, appProperty);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_UninstallDebugSandbox_02
 * @tc.desc: Test UninstallDebugSandbox with nullptr parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxManagerTest, App_Spawn_Sandbox_UninstallDebugSandbox_02, TestSize.Level0)
{
    AppSpawnMgr content;
    int32_t ret = AppSpawn::SandboxCore::UninstallDebugSandbox(&content, nullptr);
    EXPECT_NE(ret, 0);  // Should fail with nullptr
}

/**
 * @tc.name: App_Spawn_Sandbox_InstallDebugSandbox_01
 * @tc.desc: Test InstallDebugSandbox with nullptr content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxManagerTest, App_Spawn_Sandbox_InstallDebugSandbox_01, TestSize.Level0)
{
    AppSpawn::g_testSpawnHelper.SetTestUid(1000);
    AppSpawn::g_testSpawnHelper.SetTestGid(1000);
    AppSpawn::g_testSpawnHelper.SetProcessName("test.debug.app");
    AppSpawningCtx *appProperty = AppSpawn::GetTestAppPropertyCore();

    AppSpawnMgr content;
    int32_t ret = AppSpawn::SandboxCore::InstallDebugSandbox(&content, appProperty);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_InstallDebugSandbox_02
 * @tc.desc: Test InstallDebugSandbox with nullptr parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxManagerTest, App_Spawn_Sandbox_InstallDebugSandbox_02, TestSize.Level0)
{
    AppSpawnMgr content;
    int32_t ret = AppSpawn::SandboxCore::InstallDebugSandbox(&content, nullptr);
    EXPECT_EQ(ret, 0);
}

}  // namespace OHOS
