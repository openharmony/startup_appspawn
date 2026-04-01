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

/**
 * @file dec_api_test.cpp
 * @brief DecApiTest - 测试对外暴露的 dec API
 *
 * 此测试用例模拟外部用户使用 appspawn_dec_util so 库的场景。
 * 不依赖内部源文件，只通过公开头文件和 so 库进行测试。
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "dec_api.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class DecApiTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    }
};

/**
 * @tc.name: App_Spawn_DecApi_GetIgnoreCaseDirs_001
 * @tc.desc: Test GetIgnoreCaseDirs API - verify basic functionality
 * @tc.type: FUNC
 * @tc.level: Level0
 */
HWTEST_F(DecApiTest, App_Spawn_DecApi_GetIgnoreCaseDirs_001, TestSize.Level0)
{
    // 调用对外暴露的 API
    std::vector<std::pair<std::string, int>> dirs = GetIgnoreCaseDirs();

    // 验证返回值不为空（根据系统配置可能有不同结果）
    // 这里只验证 API 可以正常调用，不依赖具体配置
    GTEST_LOG_(INFO) << "GetIgnoreCaseDirs returned " << dirs.size() << " entries";

    // 验证每个条目的格式
    for (const auto& entry : dirs) {
        // path 不应为空
        EXPECT_FALSE(entry.first.empty()) << "Path should not be empty";
    }
}

/**
 * @tc.name: App_Spawn_DecApi_GetIgnoreCaseDirs_002
 * @tc.desc: Test GetIgnoreCaseDirs API - verify return type and structure
 * @tc.type: FUNC
 * @tc.level: Level1
 */
HWTEST_F(DecApiTest, App_Spawn_DecApi_GetIgnoreCaseDirs_002, TestSize.Level1)
{
    // 多次调用验证稳定性
    auto dirs1 = GetIgnoreCaseDirs();
    auto dirs2 = GetIgnoreCaseDirs();

    // 两次调用应该返回相同数量的条目
    EXPECT_EQ(dirs1.size(), dirs2.size()) << "Multiple calls should return consistent results";

    // 验证内容一致性
    for (size_t i = 0; i < dirs1.size() && i < dirs2.size(); i++) {
        GTEST_LOG_(INFO) << "Entry " << i << ": path=" << dirs1[i].first
                         << ", mode1=" << dirs1[i].second
                         << ", mode2=" << dirs2[i].second;
        EXPECT_EQ(dirs1[i].first, dirs2[i].first) << "Path should be consistent";
        EXPECT_EQ(dirs1[i].second, dirs2[i].second) << "Mode should be consistent";
    }
}

} // namespace OHOS
