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
#include <string>

#include "appspawn_utils.h"
#include "dec_api.h"
#include "dec_config.h"
#include "sandbox_dec.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class DecUtilTest : public testing::Test {
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
* @tc.name: App_Spawn_DecUtil_GetDecPathMap_Success
* @tc.desc: Verify get decPaths map succeed
*
*/
HWTEST_F(DecUtilTest, App_Spawn_DecUtil_GetDecPathMap_Success, TestSize.Level0)
{
    std::map<std::string, std::vector<std::string>> decMap = GetDecPathMap();

    for (const auto& pair : decMap) {
        const std::string& key = pair.first;
        const std::vector<std::string>& values = pair.second;
        APPSPAWN_LOGI("permission %{public}s", key.c_str());
        for (const std::string& value : values) {
            APPSPAWN_LOGI("decPath %{public}s", value.c_str());
        }
    }

    EXPECT_GT(decMap.size(), 0);
}

/**
* @tc.name: App_Spawn_DecUtil_GetIgnoreCaseDirs_Success
* @tc.desc: Verify GetIgnoreCaseDirs returns correct configuration
*
*/
HWTEST_F(DecUtilTest, App_Spawn_DecUtil_GetIgnoreCaseDirs_Success, TestSize.Level0)
{
    std::vector<std::pair<std::string, int>> dirs = GetIgnoreCaseDirs();

    // Build expected list from DEC_IGNORE_CASE_LIST macro
    static const DecIgnoreCaseInfo expected[] = { DEC_IGNORE_CASE_LIST };
    size_t expectedCount = sizeof(expected) / sizeof(expected[0]);

    EXPECT_EQ(dirs.size(), expectedCount);

    // Verify each entry has valid path and mode
    for (const auto& [path, mode] : dirs) {
        APPSPAWN_LOGI("ignore case dir: %{public}s, mode: %{public}d", path.c_str(), mode);

        // Path should not be empty
        EXPECT_FALSE(path.empty());

        // Mode should be valid (DEC_MODE_IGNORE_CASE or DEC_MODE_NOT_IGNORE_CASE)
        EXPECT_TRUE(mode == DEC_MODE_IGNORE_CASE || mode == DEC_MODE_NOT_IGNORE_CASE);
    }
}

/**
* @tc.name: App_Spawn_DecUtil_GetIgnoreCaseDirs_ConfigMatch
* @tc.desc: Verify GetIgnoreCaseDirs returns expected configuration values
*
*/
HWTEST_F(DecUtilTest, App_Spawn_DecUtil_GetIgnoreCaseDirs_ConfigMatch, TestSize.Level0)
{
    std::vector<std::pair<std::string, int>> dirs = GetIgnoreCaseDirs();

    // Build expected list from DEC_IGNORE_CASE_LIST macro
    static const DecIgnoreCaseInfo expected[] = { DEC_IGNORE_CASE_LIST };
    size_t expectedCount = sizeof(expected) / sizeof(expected[0]);

    EXPECT_EQ(dirs.size(), expectedCount);

    for (size_t i = 0; i < dirs.size() && i < expectedCount; i++) {
        EXPECT_EQ(dirs[i].first, expected[i].path);
        EXPECT_EQ(dirs[i].second, expected[i].mode);
    }
}

/**
* @tc.name: App_Spawn_DecUtil_GetIgnoreCaseDirs_MultipleCalls
* @tc.desc: Verify GetIgnoreCaseDirs returns consistent results across multiple calls
*
*/
HWTEST_F(DecUtilTest, App_Spawn_DecUtil_GetIgnoreCaseDirs_MultipleCalls, TestSize.Level0)
{
    std::vector<std::pair<std::string, int>> dirs1 = GetIgnoreCaseDirs();
    std::vector<std::pair<std::string, int>> dirs2 = GetIgnoreCaseDirs();

    // Results should be consistent
    EXPECT_EQ(dirs1.size(), dirs2.size());

    for (size_t i = 0; i < dirs1.size() && i < dirs2.size(); i++) {
        EXPECT_EQ(dirs1[i].first, dirs2[i].first);
        EXPECT_EQ(dirs1[i].second, dirs2[i].second);
    }
}

/**
* @tc.name: App_Spawn_DecUtil_Macro_MAX_POLICY_NUM_001
* @tc.desc: Verify MAX_POLICY_NUM macro is 64 as required
* @tc.type: FUNC
*/
HWTEST_F(DecUtilTest, App_Spawn_DecUtil_Macro_MAX_POLICY_NUM_001, TestSize.Level0)
{
    // TC001: Verify MAX_POLICY_NUM is 64 to ensure policy capacity meets requirement
    EXPECT_EQ(MAX_POLICY_NUM, 64);
}

/**
* @tc.name: App_Spawn_DecUtil_Macro_KERNEL_BATCH_SIZE_001
* @tc.desc: Verify KERNEL_BATCH_SIZE macro is 8 as required
* @tc.type: FUNC
*/
HWTEST_F(DecUtilTest, App_Spawn_DecUtil_Macro_KERNEL_BATCH_SIZE_001, TestSize.Level0)
{
    // TC002: Verify KERNEL_BATCH_SIZE is 8 for batch delivery mechanism
    EXPECT_EQ(KERNEL_BATCH_SIZE, 8);
}
}
