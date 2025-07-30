/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include "cJSON.h"
#define private public
#include "sandbox_core.h"
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include "sandbox_core.h"
#include "lib_func_mock.h"

// mock sandbox_core.cpp func
#include "lib_func_define.h"
#include "sandbox_core.cpp"
#include "lib_func_undef.h"

#undef private

using namespace std;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;

class DebugSandboxTest : public ::testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase();
    void SetUp() override {};
    void TearDown() override {};
    static inline shared_ptr<LibraryFuncMock> funcMock = nullptr;
};

void DebugSandboxTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << "SetUpTestCase enter";
    funcMock = make_shared<LibraryFuncMock>();
    LibraryFuncMock::libraryFunc_ = funcMock;
}

void DebugSandboxTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "TearDownTestCase enter";
    LibraryFuncMock::libraryFunc_ = nullptr;
    funcMock = nullptr;
}

/**
 * @tc.name  : DoUninstallDebugSandbox_ShouldRemoveSandbox_WhenConfigIsValid
 * @tc.number: DoUninstallDebugSandboxTest_001
 * @tc.desc  : 测试当配置有效时,DoUninstallDebugSandbox 函数能够正确卸载沙箱
 */
HWTEST_F(DebugSandboxTest, ATC_DoUninstallDebugSandbox_ShouldRemoveSandbox_WhenConfigIsValid, TestSize.Level0)
{
    const char testDebugMountPathsJson[] = R"({
            "mount-paths": [
                {
                    "src-path": "/data/app/el1/<currentUserId>/base/<variablePackageName>",
                    "sandbox-path": "/data/storage/el1/base",
                    "sandbox-flags": [ "bind", "rec" ],
                    "mount-shared-flag": "true",
                    "check-action-status": "false"
                },
                {
                    "src-path": "/data/app/el1/<currentUserId>/database/<variablePackageName>",
                    "sandbox-path": "/data/storage/el1/database",
                    "sandbox-flags": [ "bind", "rec" ],
                    "mount-shared-flag": "true",
                    "check-action-status": "false"
                }
            ]
        })";

    cJSON *config = cJSON_Parse(testDebugMountPathsJson);
    ASSERT_NE(config, nullptr);
    std::vector<std::string> bundleList = {"com.example.app"};

    // 模拟 access 返回 0,表示路径存在
    EXPECT_CALL(*funcMock, access(_, _)).WillOnce(Return(0)).WillOnce(Return(0));

    // 模拟 umount2 返回 0,表示卸载成功
    EXPECT_CALL(*funcMock, umount2(_, _)).WillOnce(Return(0)).WillOnce(Return(0));

    // 模拟 rmdir 返回 0,表示删除目录成功
    EXPECT_CALL(*funcMock, rmdir(_)).WillOnce(Return(0)).WillOnce(Return(0));

    SandboxCore::DoUninstallDebugSandbox(bundleList, config);

    cJSON_Delete(config);
}

/**
 * @tc.name  : DoUninstallDebugSandbox_ShouldNotRemoveSandbox_WhenConfigIsInvalid
 * @tc.number: DoUninstallDebugSandboxTest_002
 * @tc.desc  : 测试当配置无效时,DoUninstallDebugSandbox 函数不会卸载沙箱
 */
HWTEST_F(DebugSandboxTest, ATC_DoUninstallDebugSandbox_ShouldNotRemoveSandbox_WhenConfigIsInvalid, TestSize.Level0)
{
    std::vector<std::string> bundleList = {"com.example.app"};
    cJSON *config = nullptr;

    SandboxCore::DoUninstallDebugSandbox(bundleList, config);
}

/**
 * @tc.name  : DoUninstallDebugSandbox_ShouldNotRemoveSandbox_WhenMountPointIsNotArray
 * @tc.number: DoUninstallDebugSandboxTest_003
 * @tc.desc  : 测试当 mountPoints 不是数组时,DoUninstallDebugSandbox 函数不会卸载沙箱
 */
HWTEST_F(DebugSandboxTest, ATC_DoUninstallDebugSandbox_ShouldNotRemoveSandbox_WhenMountPointIsNotArray, TestSize.Level0)
{
    const char testDebugMountPathsJson[] = R"({
            "mount-paths": {
                "src-path": "/data/app/el1/<currentUserId>/base/<variablePackageName>",
                "sandbox-path": "/data/storage/el1/base",
                "sandbox-flags": [ "bind", "rec" ],
                "mount-shared-flag": "true",
                "check-action-status": "false"
            }
        })";

    cJSON *config = cJSON_Parse(testDebugMountPathsJson);
    ASSERT_NE(config, nullptr);
    std::vector<std::string> bundleList = {"com.example.app"};

    SandboxCore::DoUninstallDebugSandbox(bundleList, config);

    cJSON_Delete(config);
}

/**
 * @tc.name  : UninstallDebugSandbox_ShouldReturnInvalidArg_WhenPropertyIsNull
 * @tc.number: UninstallDebugSandboxTest_001
 * @tc.desc  : 测试当传入的 property 为 nullptr 时,函数应返回 APPSPAWN_ARG_INVALID
 */
HWTEST_F(DebugSandboxTest, ATC_UninstallDebugSandbox_ShouldReturnInvalidArg_WhenPropertyIsNull, TestSize.Level0)
{
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr content;

    int32_t result = SandboxCore::UninstallDebugSandbox(&content, property);
    EXPECT_EQ(result, APPSPAWN_ARG_INVALID);
}

/**
 * @tc.name  : UninstallDebugSandbox_ShouldReturnInvalidArg_WhenContentIsNull
 * @tc.number: UninstallDebugSandboxTest_002
 * @tc.desc  : 测试当传入的 content 为 nullptr 时,函数应返回 APPSPAWN_ARG_INVALID
 */
HWTEST_F(DebugSandboxTest, ATC_UninstallDebugSandbox_ShouldReturnInvalidArg_WhenContentIsNull, TestSize.Level0)
{
    AppSpawningCtx property;
    AppSpawnMgr *content = nullptr;

    int32_t result = SandboxCore::UninstallDebugSandbox(content, &property);
    EXPECT_EQ(result, APPSPAWN_ARG_INVALID);
}