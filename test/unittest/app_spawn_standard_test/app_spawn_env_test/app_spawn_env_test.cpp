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

#include <cerrno>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <cJSON.h>

#include "appspawn.h"
#include "appspawn_adapter.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

APPSPAWN_STATIC std::string_view TrimWhitespaceView(std::string_view str);
APPSPAWN_STATIC int32_t ParseEnvLine(const std::string &line, std::string &envName, std::string &envValue);
APPSPAWN_STATIC int32_t LoadCustomSandboxEnv(const char* envFilePath);
APPSPAWN_STATIC int SpawnSetCustomSandboxEnv(AppSpawnMgr *content, AppSpawningCtx *property);

namespace OHOS {
static AppSpawnTestHelper g_testHelper;
class AppSpawnEnvTest : public testing::Test {
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
 * @brief App_Spawn_LoadCustomSandboxEnv_00
 *
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_LoadCustomSandboxEnv_00, TestSize.Level0)
{
    const std::string envFilePathFile = "/etc/environment";
    int32_t ret = LoadCustomSandboxEnv(envFilePathFile.c_str());
    EXPECT_TRUE(ret >= 0);
    APPSPAWN_LOGI("App_Spawn_LoadCustomSandboxEnv_00 ret: %{public}d", ret);
}

/**
 * @brief SpawnSetCustomSandboxEnv_01
 *
 */
HWTEST_F(AppSpawnEnvTest, SpawnSetCustomSandboxEnv_01, TestSize.Level0)
{
    int ret = -1;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != nullptr, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_CUSTOM_SANDBOX);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");

        ret = SpawnSetCustomSandboxEnv(nullptr, property);
        ASSERT_EQ(ret, 0);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief App_Spawn_TrimWhitespace_01
 *
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_TrimWhitespace_01, TestSize.Level0)
{
    // Test empty string
    EXPECT_EQ(TrimWhitespaceView(""), "");

    // Test string with only spaces
    EXPECT_EQ(TrimWhitespaceView("   "), "");

    // Test string with only tabs
    EXPECT_EQ(TrimWhitespaceView("\t\t\t"), "");

    // Test string with only newlines
    EXPECT_EQ(TrimWhitespaceView("\n\n\n"), "");

    // Test string with only carriage returns
    EXPECT_EQ(TrimWhitespaceView("\r\r\r"), "");

    // Test string with mixed whitespace characters
    EXPECT_EQ(TrimWhitespaceView(" \t\n\r \t\n\r"), "");
}

/**
 * @brief App_Spawn_TrimWhitespace_02
 *
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_TrimWhitespace_02, TestSize.Level0)
{
    // Test string with leading whitespace
    EXPECT_EQ(TrimWhitespaceView("   hello"), "hello");

    // Test string with trailing whitespace
    EXPECT_EQ(TrimWhitespaceView("hello   "), "hello");

    // Test string with both leading and trailing whitespace
    EXPECT_EQ(TrimWhitespaceView("   hello   "), "hello");

    // Test string with mixed leading/trailing whitespace
    EXPECT_EQ(TrimWhitespaceView(" \t\nhello\r \t\n"), "hello");

    // Test string without any whitespace
    EXPECT_EQ(TrimWhitespaceView("hello"), "hello");

    // Test string with internal whitespace (should not be trimmed)
    EXPECT_EQ(TrimWhitespaceView("   hello world   "), "hello world");

    // Test string with multiple internal whitespace
    EXPECT_EQ(TrimWhitespaceView("   hello   world   "), "hello   world");
}

/**
 * @brief App_Spawn_TrimWhitespace_03
 *
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_TrimWhitespace_03, TestSize.Level0)
{
    // Test string with special characters
    EXPECT_EQ(TrimWhitespaceView("   !@#$%^&*()   "), "!@#$%^&*()");

    // Test string with numbers
    EXPECT_EQ(TrimWhitespaceView("   12345   "), "12345");

    // Test string with alphanumeric characters
    EXPECT_EQ(TrimWhitespaceView("   abc123   "), "abc123");

    // Test string with Unicode characters
    EXPECT_EQ(TrimWhitespaceView("   你好世界   "), "你好世界");

    // Test string with single character and whitespace
    EXPECT_EQ(TrimWhitespaceView("   a   "), "a");

    // Test string with only one whitespace character
    EXPECT_EQ(TrimWhitespaceView(" "), "");
}

/**
 * @brief App_Spawn_ParseEnvLine_01
 *
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_ParseEnvLine_01, TestSize.Level0)
{
    std::string envName, envValue;
    int32_t ret;

    // Test empty line
    ret = ParseEnvLine("", envName, envValue);
    EXPECT_NE(ret, 0);
    EXPECT_TRUE(envName.empty());
    EXPECT_TRUE(envValue.empty());

    // Test comment line
    ret = ParseEnvLine("# This is a comment", envName, envValue);
    EXPECT_NE(ret, 0);
    EXPECT_TRUE(envName.empty());
    EXPECT_TRUE(envValue.empty());

    // Test line with only
    ret = ParseEnvLine("#", envName, envValue);
    EXPECT_NE(ret, 0);
    EXPECT_TRUE(envName.empty());
    EXPECT_TRUE(envValue.empty());

    // Test line without equals sign
    ret = ParseEnvLine("NO_EQUALS_SIGN", envName, envValue);
    EXPECT_NE(ret, 0);
    EXPECT_TRUE(envName.empty());
    EXPECT_TRUE(envValue.empty());

    // Test line with only equals sign
    ret = ParseEnvLine("=", envName, envValue);
    EXPECT_NE(ret, 0);
    EXPECT_TRUE(envName.empty());
    EXPECT_TRUE(envValue.empty());

    // Test line with leading whitespace before equals
    ret = ParseEnvLine("   =", envName, envValue);
    EXPECT_NE(ret, 0);
    EXPECT_TRUE(envName.empty());
    EXPECT_TRUE(envValue.empty());
}

/**
 * @brief App_Spawn_ParseEnvLine_02
 *
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_ParseEnvLine_02, TestSize.Level0)
{
    std::string envName, envValue;
    int32_t ret;

    // Test normal key=value
    ret = ParseEnvLine("KEY=value", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "value");

    // Test key with leading and trailing whitespace
    ret = ParseEnvLine("   KEY   = value", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "value");

    // Test value with leading and trailing whitespace
    ret = ParseEnvLine("KEY=   value   ", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "value");

    // Test both key and value with whitespace
    ret = ParseEnvLine("   KEY   =   value   ", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "value");

    // Test empty value
    ret = ParseEnvLine("KEY=", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "");

    // Test value with only whitespace
    ret = ParseEnvLine("KEY=   ", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "");
}

/**
 * @brief App_Spawn_ParseEnvLine_03
 *
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_ParseEnvLine_03, TestSize.Level0)
{
    std::string envName, envValue;
    int32_t ret;

    // Test double quoted value
    ret = ParseEnvLine("KEY=\"quoted value\"", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "quoted value");

    // Test single quoted value
    ret = ParseEnvLine("KEY='single quoted value'", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "single quoted value");

    // Test quoted value with leading/trailing whitespace
    ret = ParseEnvLine("KEY=   \"quoted\"   ", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "quoted");

    // Test quoted empty value
    ret = ParseEnvLine("KEY=\"\"", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "");

    // Test quoted value with spaces inside
    ret = ParseEnvLine("KEY=\"value with spaces\"", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "value with spaces");

    // Test quoted value with tabs and newlines
    ret = ParseEnvLine("KEY=\"value\twith\nnewlines\"", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "value\twith\nnewlines");
}

/**
 * @brief App_Spawn_ParseEnvLine_04
 *
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_ParseEnvLine_04, TestSize.Level0)
{
    std::string envName, envValue;
    int32_t ret;

    // Test mismatched quotes
    ret = ParseEnvLine("KEY=\"unmatched quote", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "\"unmatched quote");

    // Test single quote at start
    ret = ParseEnvLine("KEY='single quote start", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "'single quote start");

    // Test single quote at end
    ret = ParseEnvLine("KEY=single quote end'", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "single quote end'");

    // Test too short quoted value
    ret = ParseEnvLine("KEY=\"", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "\"");

    // Test only one character in quotes
    ret = ParseEnvLine("KEY=\"x", envName, envValue);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(envName, "KEY");
    EXPECT_EQ(envValue, "\"x");
}

/**
 * @brief App_Spawn_LoadCustomSandboxEnv_01
 * Test file open failure scenario
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_LoadCustomSandboxEnv_01, TestSize.Level0)
{
    // Use a non-existent file path to simulate file open failure
    const std::string nonExistentFile = "./non_existent_environment_file_12345";

    // Test with non-existent file
    int32_t ret = LoadCustomSandboxEnv(nonExistentFile.c_str());

    EXPECT_EQ(ret, 0);
}

/**
 * @brief App_Spawn_LoadCustomSandboxEnv_02
 * Test empty file scenario
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_LoadCustomSandboxEnv_02, TestSize.Level0)
{
    const std::string tempEnvFile = "./temp_environment_test_empty";

    // Create empty file
    std::ofstream outFile(tempEnvFile, std::ios::trunc);
    ASSERT_TRUE(outFile.is_open());
    outFile.close();

    // Use the actual LoadCustomSandboxEnv function with our temp file
    int32_t ret = LoadCustomSandboxEnv(tempEnvFile.c_str());
    EXPECT_EQ(ret, 0);

    // Clean up
    remove(tempEnvFile.c_str());
}

/**
 * @brief App_Spawn_LoadCustomSandboxEnv_CommentLinesTest
 * Test comment lines and empty lines scenario (App_Spawn_LoadCustomSandboxEnv_03)
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_LoadCustomSandboxEnv_03, TestSize.Level0)
{
    const std::string tempEnvFile = "./temp_environment_test_comments";

    // Create file with comments and empty lines
    std::ofstream outFile(tempEnvFile, std::ios::trunc);
    ASSERT_TRUE(outFile.is_open());
    outFile << "# This is a comment\n";
    outFile << "\n";
    outFile << "   # Another comment with leading spaces\n";
    outFile << "    \n";
    outFile.close();

    // Use the actual LoadCustomSandboxEnv function with our temp file
    int32_t ret = LoadCustomSandboxEnv(tempEnvFile.c_str());
    EXPECT_EQ(ret, 0);

    // Clean up
    remove(tempEnvFile.c_str());
}

/**
 * @brief App_Spawn_LoadCustomSandboxEnv_NormalKeysTest
 * Test normal key=value pairs scenario (App_Spawn_LoadCustomSandboxEnv_04)
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_LoadCustomSandboxEnv_04, TestSize.Level0)
{
    const std::string tempEnvFile = "./temp_environment_test_normal";

    // Create file with normal key=value pairs
    std::ofstream outFile(tempEnvFile, std::ios::trunc);
    ASSERT_TRUE(outFile.is_open());
    outFile << "KEY1=value1\n";
    outFile << "   KEY2   =   value2   \n";
    outFile << "KEY3=\"quoted value\"\n";
    outFile.close();

    // Use the actual LoadCustomSandboxEnv function with our temp file
    int32_t ret = LoadCustomSandboxEnv(tempEnvFile.c_str());
    EXPECT_EQ(ret, 0);

    // Clean up
    remove(tempEnvFile.c_str());
}

/**
 * @brief App_Spawn_LoadCustomSandboxEnv_EmptyKeyTest
 * Test empty key name scenario (App_Spawn_LoadCustomSandboxEnv_05)
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_LoadCustomSandboxEnv_05, TestSize.Level0)
{
    const std::string tempEnvFile = "./temp_environment_test_empty_key";

    // Create file with empty key name
    std::ofstream outFile(tempEnvFile, std::ios::trunc);
    ASSERT_TRUE(outFile.is_open());
    outFile << "=invalid_value\n";
    outFile.close();

    // Use the actual LoadCustomSandboxEnv function with our temp file
    int32_t ret = LoadCustomSandboxEnv(tempEnvFile.c_str());
    EXPECT_EQ(ret, 0);

    // Clean up
    remove(tempEnvFile.c_str());
}

/**
 * @brief App_Spawn_LoadCustomSandboxEnv_EmptyKeyTest
 * Test empty key name scenario (App_Spawn_LoadCustomSandboxEnv_06)
 */
HWTEST_F(AppSpawnEnvTest, App_Spawn_LoadCustomSandboxEnv_06, TestSize.Level0)
{
    const std::string tempEnvFile = "./temp_environment_test_empty_key";

    // Create file with empty key name
    std::ofstream outFile(tempEnvFile, std::ios::trunc);
    ASSERT_TRUE(outFile.is_open());
    outFile << "=invalid_value\n";
    outFile.close();

    chmod(tempEnvFile.c_str(), 0300);

    // Use the actual LoadCustomSandboxEnv function with our temp file
    int32_t ret = LoadCustomSandboxEnv(tempEnvFile.c_str());
    EXPECT_TRUE(ret == 0 || ret == APPSPAWN_ENV_FILE_READ_ERROR);

    // Clean up
    remove(tempEnvFile.c_str());
}
}   // namespace OHOS

