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

#include "appspawn_utils.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "cJSON.h"
#include "appspawn_silk.h"

extern struct SilkConfig g_silkConfig;
APPSPAWN_STATIC bool ParseSilkConfig(const cJSON *root, struct SilkConfig *config);
#ifdef __cplusplus
}
#endif

using namespace testing;
using namespace testing::ext;

class ParseSilkConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @brief 模拟测试，正常解析json文件并且正常使能silk
 *
 */
HWTEST_F(ParseSilkConfigTest, Parse_Silk_Config_001, TestSize.Level0)
{
    const char *silkJsonStr = "{\"enabled_app_list\":[\"com.taobao.taobao\",\"com.tencent.mm\"]}";
    cJSON *root =  cJSON_Parse(silkJsonStr);
    const char* testEnableSilkProcessName = "com.taobao.taobao";
    bool ret = ParseSilkConfig(root, &g_silkConfig);
    cJSON_Delete(root);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(g_silkConfig.configCursor, 2);
    ret = LoadSilkLibrary(testEnableSilkProcessName);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(g_silkConfig.configItems, NULL);
    ASSERT_EQ(g_silkConfig.configCursor, 0);
}

/**
 * @brief 模拟测试，processName为NULL的情况
 *
 */
HWTEST_F(ParseSilkConfigTest, Parse_Silk_Config_002, TestSize.Level0)
{
    const char *silkJsonStr = "{\"enabled_app_list\":[\"com.taobao.taobao\",\"com.tencent.mm\"]}";
    cJSON *root =  cJSON_Parse(silkJsonStr);
    const char* testEnableSilkProcessName = NULL;
    bool ret = ParseSilkConfig(root, &g_silkConfig);
    cJSON_Delete(root);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(g_silkConfig.configCursor, 2);
    ret = LoadSilkLibrary(testEnableSilkProcessName);
    ASSERT_EQ(ret, false);
    ASSERT_EQ(g_silkConfig.configItems, NULL);
    ASSERT_EQ(g_silkConfig.configCursor, 0);
}
