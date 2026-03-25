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
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>
#include <map>

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
#include "appspawn_hook.h"
#include "sandbox_lock_bundle_test.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;

namespace OHOS {

AppSpawnTestHelper g_testHelperLock;

class AppSpawnLockBundleTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    static constexpr const char *testBundle1 = "com.example.test1";
    static constexpr const char *testBundle2 = "com.example.test2";
    static constexpr const char *testLockPath1 = "/tmp/test_locked_1";
    static constexpr const char *testLockPath2 = "/tmp/test_locked_2";
    static constexpr const char *testLockPathClone = "/tmp/test_locked_clone";
    static constexpr const char *testLockPathIsolated = "/tmp/test_locked_isolated";
    static constexpr mode_t dirMode = 0755;  // NOLINT(cppcoreguidelines-avoid-magic-numbers)
};

void AppSpawnLockBundleTest::SetUpTestCase()
{
    // Create test directories
    mkdir(testLockPath1, dirMode);
    mkdir(testLockPath2, dirMode);
    mkdir(testLockPathClone, dirMode);
    mkdir(testLockPathIsolated, dirMode);
}

void AppSpawnLockBundleTest::TearDownTestCase()
{
    // Clean up test directories
    rmdir(testLockPath1);
    rmdir(testLockPath2);
    rmdir(testLockPathClone);
    rmdir(testLockPathIsolated);
}

void AppSpawnLockBundleTest::SetUp()
{
    const TestInfo *info = UnitTest::GetInstance()->current_test_info();
    GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
    // Clear the map before each test
    g_lockBundleMap.clear();
}

void AppSpawnLockBundleTest::TearDown()
{
    const TestInfo *info = UnitTest::GetInstance()->current_test_info();
    GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    // Clear the map after each test
    g_lockBundleMap.clear();
}

// ==================== AddLockBundleRef Tests ====================

/**
 * @tc.name: AddLockBundleRef_001
 * @tc.desc: Test AddLockBundleRef creates new entry for new lockPath
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, AddLockBundleRef_001, TestSize.Level1)
{
    int ret = AddLockBundleRef(testLockPath1);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_lockBundleMap.size(), 1u);

    auto it = g_lockBundleMap.find(testLockPath1);
    EXPECT_NE(it, g_lockBundleMap.end());
    EXPECT_EQ(it->second.refCount, 1u);
    EXPECT_EQ(it->second.lockPath, testLockPath1);
}

/**
 * @tc.name: AddLockBundleRef_002
 * @tc.desc: Test AddLockBundleRef increments ref count for existing lockPath
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, AddLockBundleRef_002, TestSize.Level1)
{
    AddLockBundleRef(testLockPath1);
    int ret = AddLockBundleRef(testLockPath1);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_lockBundleMap.size(), 1u);
    EXPECT_EQ(g_lockBundleMap[testLockPath1].refCount, 2u);
}

/**
 * @tc.name: AddLockBundleRef_003
 * @tc.desc: Test AddLockBundleRef creates separate entries for different lockPaths
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, AddLockBundleRef_003, TestSize.Level1)
{
    AddLockBundleRef(testLockPath1);
    AddLockBundleRef(testLockPathClone);

    // Different lockPaths should have separate entries
    EXPECT_EQ(g_lockBundleMap.size(), 2u);
    EXPECT_EQ(g_lockBundleMap[testLockPath1].refCount, 1u);
    EXPECT_EQ(g_lockBundleMap[testLockPathClone].refCount, 1u);
}

/**
 * @tc.name: AddLockBundleRef_004
 * @tc.desc: Test AddLockBundleRef with empty lockPath
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, AddLockBundleRef_004, TestSize.Level1)
{
    int ret = AddLockBundleRef("");
    // Should return error for empty lockPath
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(g_lockBundleMap.size(), 0u);
}

// ==================== ReleaseLockBundleRef Tests ====================

/**
 * @tc.name: ReleaseLockBundleRef_001
 * @tc.desc: Test ReleaseLockBundleRef decrements ref count
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, ReleaseLockBundleRef_001, TestSize.Level1)
{
    AddLockBundleRef(testLockPath1);
    AddLockBundleRef(testLockPath1);

    ReleaseLockBundleRef(testLockPath1);

    // Ref count should be 1, entry still exists
    auto it = g_lockBundleMap.find(testLockPath1);
    EXPECT_NE(it, g_lockBundleMap.end());
    EXPECT_EQ(it->second.refCount, 1u);
}

/**
 * @tc.name: ReleaseLockBundleRef_002
 * @tc.desc: Test ReleaseLockBundleRef removes entry when ref count reaches 0
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, ReleaseLockBundleRef_002, TestSize.Level1)
{
    AddLockBundleRef(testLockPath1);

    ReleaseLockBundleRef(testLockPath1);

    // Entry should be removed when ref count reaches 0
    EXPECT_EQ(g_lockBundleMap.find(testLockPath1), g_lockBundleMap.end());
    EXPECT_EQ(g_lockBundleMap.size(), 0u);
}

/**
 * @tc.name: ReleaseLockBundleRef_003
 * @tc.desc: Test ReleaseLockBundleRef for non-existent lockPath
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, ReleaseLockBundleRef_003, TestSize.Level1)
{
    // Should not crash for non-existent lockPath
    ReleaseLockBundleRef(testLockPath1);
    EXPECT_EQ(g_lockBundleMap.size(), 0u);
}

/**
 * @tc.name: ReleaseLockBundleRef_004
 * @tc.desc: Test ReleaseLockBundleRef with empty lockPath
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, ReleaseLockBundleRef_004, TestSize.Level1)
{
    AddLockBundleRef(testLockPath1);
    // Should handle gracefully
    ReleaseLockBundleRef("");
    // Map should be unchanged
    EXPECT_EQ(g_lockBundleMap.size(), 1u);
}

// ==================== Fine-grained Management Tests ====================

/**
 * @tc.name: FineGrainedManagement_001
 * @tc.desc: Test clone and non-clone have separate entries
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, FineGrainedManagement_001, TestSize.Level1)
{
    // Non-clone process starts
    AddLockBundleRef(testLockPath1);
    // Clone process starts (different lockPath)
    AddLockBundleRef(testLockPathClone);

    // Should have two separate entries
    EXPECT_EQ(g_lockBundleMap.size(), 2u);
    EXPECT_EQ(g_lockBundleMap[testLockPath1].refCount, 1u);
    EXPECT_EQ(g_lockBundleMap[testLockPathClone].refCount, 1u);

    // Clone process exits
    ReleaseLockBundleRef(testLockPathClone);
    // Non-clone entry should still exist
    EXPECT_EQ(g_lockBundleMap.size(), 1u);
    EXPECT_NE(g_lockBundleMap.find(testLockPath1), g_lockBundleMap.end());
    // Clone entry should be removed
    EXPECT_EQ(g_lockBundleMap.find(testLockPathClone), g_lockBundleMap.end());

    // Non-clone process exits
    ReleaseLockBundleRef(testLockPath1);
    EXPECT_EQ(g_lockBundleMap.size(), 0u);
}

/**
 * @tc.name: FineGrainedManagement_002
 * @tc.desc: Test isolated and non-isolated have separate entries
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, FineGrainedManagement_002, TestSize.Level1)
{
    // Non-isolated process starts
    AddLockBundleRef(testLockPath1);
    // Isolated process starts (different lockPath)
    AddLockBundleRef(testLockPathIsolated);

    // Should have two separate entries
    EXPECT_EQ(g_lockBundleMap.size(), 2u);

    // Both exit
    ReleaseLockBundleRef(testLockPath1);
    ReleaseLockBundleRef(testLockPathIsolated);

    EXPECT_EQ(g_lockBundleMap.size(), 0u);
}

/**
 * @tc.name: FineGrainedManagement_003
 * @tc.desc: Test multiple instances of same lockPath
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, FineGrainedManagement_003, TestSize.Level1)
{
    // Same lockPath with multiple references
    AddLockBundleRef(testLockPath1);
    AddLockBundleRef(testLockPath1);
    AddLockBundleRef(testLockPath1);

    EXPECT_EQ(g_lockBundleMap.size(), 1u);
    EXPECT_EQ(g_lockBundleMap[testLockPath1].refCount, 3u);

    // Release one
    ReleaseLockBundleRef(testLockPath1);
    EXPECT_EQ(g_lockBundleMap[testLockPath1].refCount, 2u);

    // Release all
    ReleaseLockBundleRef(testLockPath1);
    ReleaseLockBundleRef(testLockPath1);
    EXPECT_EQ(g_lockBundleMap.size(), 0u);
}

// ==================== Multiple Bundle Tests ====================

/**
 * @tc.name: MixedBundlesScenario_001
 * @tc.desc: Test mixed bundles scenario
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, MixedBundlesScenario_001, TestSize.Level1)
{
    // Bundle 1 with 2 processes (same lockPath)
    AddLockBundleRef(testLockPath1);
    AddLockBundleRef(testLockPath1);

    // Bundle 2 with 1 process
    AddLockBundleRef(testLockPath2);

    EXPECT_EQ(g_lockBundleMap.size(), 2u);

    // One process of bundle 1 exits
    ReleaseLockBundleRef(testLockPath1);
    EXPECT_EQ(g_lockBundleMap.size(), 2u);
    EXPECT_EQ(g_lockBundleMap[testLockPath1].refCount, 1u);

    // Bundle 2 exits completely
    ReleaseLockBundleRef(testLockPath2);
    EXPECT_EQ(g_lockBundleMap.size(), 1u);
    EXPECT_EQ(g_lockBundleMap.find(testLockPath2), g_lockBundleMap.end());

    // Last process of bundle 1 exits
    ReleaseLockBundleRef(testLockPath1);
    EXPECT_EQ(g_lockBundleMap.size(), 0u);
}

// ==================== Key Format Tests ====================

/**
 * @tc.name: LockBundleKeyFormat_001
 * @tc.desc: Test that key is the complete lockPath
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, LockBundleKeyFormat_001, TestSize.Level1)
{
    // Different lockPaths have separate entries
    AddLockBundleRef(testLockPath1);
    AddLockBundleRef(testLockPathClone);
    AddLockBundleRef(testLockPathIsolated);

    // Should have three entries with lockPath as key
    EXPECT_EQ(g_lockBundleMap.size(), 3u);
    EXPECT_NE(g_lockBundleMap.find(testLockPath1), g_lockBundleMap.end());
    EXPECT_NE(g_lockBundleMap.find(testLockPathClone), g_lockBundleMap.end());
    EXPECT_NE(g_lockBundleMap.find(testLockPathIsolated), g_lockBundleMap.end());

    // The lockPath stored should match the key
    EXPECT_EQ(g_lockBundleMap[testLockPath1].lockPath, testLockPath1);
    EXPECT_EQ(g_lockBundleMap[testLockPathClone].lockPath, testLockPathClone);
    EXPECT_EQ(g_lockBundleMap[testLockPathIsolated].lockPath, testLockPathIsolated);
}

// ==================== Edge Case Tests ====================

/**
 * @tc.name: EdgeCase_001
 * @tc.desc: Test empty lock path
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, EdgeCase_001, TestSize.Level1)
{
    int ret = AddLockBundleRef("");
    // Should return error for empty lockPath
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name: EdgeCase_002
 * @tc.desc: Test release more times than add
 * @tc.type: FUNC
 * @tc.require: issueI:1876
 */
HWTEST_F(AppSpawnLockBundleTest, EdgeCase_002, TestSize.Level1)
{
    AddLockBundleRef(testLockPath1);
    ReleaseLockBundleRef(testLockPath1);
    // Should not crash when releasing non-existent entry
    ReleaseLockBundleRef(testLockPath1);
    EXPECT_EQ(g_lockBundleMap.size(), 0u);
}

}  // namespace OHOS
