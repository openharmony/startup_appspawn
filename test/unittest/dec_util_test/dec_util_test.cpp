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
}
