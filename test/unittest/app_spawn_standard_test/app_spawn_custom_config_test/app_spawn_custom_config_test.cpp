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
#include <climits>
#include <sys/ioctl.h>

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
APPSPAWN_STATIC int SetUidGid(const AppSpawnMgr *content, const AppSpawningCtx *property);
#ifdef __cplusplus
extern "C" {
#endif
int SetUserId(char *userIdStr);
int GetUserId(uint32_t *userId);
#ifdef __cplusplus
}
#endif

namespace OHOS {
static AppSpawnTestHelper g_testHelper;
class AppSpawnCustomConfigTest : public testing::Test {
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_LoadCustomSandboxEnv_00, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, SpawnSetCustomSandboxEnv_01, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_TrimWhitespace_01, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_TrimWhitespace_02, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_TrimWhitespace_03, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_ParseEnvLine_01, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_ParseEnvLine_02, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_ParseEnvLine_03, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_ParseEnvLine_04, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_LoadCustomSandboxEnv_01, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_LoadCustomSandboxEnv_02, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_LoadCustomSandboxEnv_03, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_LoadCustomSandboxEnv_04, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_LoadCustomSandboxEnv_05, TestSize.Level0)
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
HWTEST_F(AppSpawnCustomConfigTest, App_Spawn_LoadCustomSandboxEnv_06, TestSize.Level0)
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

/**
 * @tc.name: SetUidGidUserIdTest_001
 * @tc.desc: Test SetUidGid function when userIdStr is NULL
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUidGidUserIdTest_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUidGidUserIdTest_001 start";
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);
        // create msg
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_USERID, "105");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_USERID %{public}s", "105");
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = false;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUidGidUserIdTest_001 end";
}

/**
 * @tc.name: SetUidGidUserIdTest_002
 * @tc.desc: Test SetUidGid function when userIdStr is NULL
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUidGidUserIdTest_002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUidGidUserIdTest_002 start";
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        EXPECT_NE(mgr, nullptr);
        // create msg
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", NWEBSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        property->allowDumpable = false;
        ret = SetUidGid(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUidGidUserIdTest_002 end";
}

/**
 * @tc.name: SetUserIdTest_001
 * @tc.desc: Test SetUserId with valid userId string
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_001 start";
    // Test with valid userId string "100"
    char userIdStr[] = "100";
    int ret = SetUserId(userIdStr);
    // Expected to return 0 on success
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_001 end";
}

/**
 * @tc.name: SetUserIdTest_002
 * @tc.desc: Test SetUserId with null pointer
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_002 start";
    // Test with null pointer - should cause segmentation fault or return error
    // This test verifies null pointer handling
    char *nullUserIdStr = nullptr;
    int ret = SetUserId(nullUserIdStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_002 end";
}

/**
 * @tc.name: SetUserIdTest_003
 * @tc.desc: Test SetUserId with empty string
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_003 start";
    // Test with empty string - should return -1
    char emptyUserIdStr[] = "";
    int ret = SetUserId(emptyUserIdStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_003 end";
}

/**
 * @tc.name: SetUserIdTest_004
 * @tc.desc: Test SetUserId with invalid string (no digits)
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_004, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_004 start";
    // Test with string containing no valid digits
    char invalidUserIdStr[] = "abc";
    int ret = SetUserId(invalidUserIdStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_004 end";
}

/**
 * @tc.name: SetUserIdTest_005
 * @tc.desc: Test SetUserId with string containing extra characters
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_005, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_005 start";
    // Test with string containing extra characters after digits
    char invalidUserIdStr[] = "100abc";
    int ret = SetUserId(invalidUserIdStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_005 end";
}

/**
 * @tc.name: SetUserIdTest_006
 * @tc.desc: Test SetUserId with negative number
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_006, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_006 start";
    // Test with negative number - should return -1
    char negativeUserIdStr[] = "-100";
    int ret = SetUserId(negativeUserIdStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_006 end";
}

/**
 * @tc.name: SetUserIdTest_007
 * @tc.desc: Test SetUserId with zero
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_007, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_007 start";
    // Test with zero - should be valid
    char zeroUserIdStr[] = "0";
    int ret = SetUserId(zeroUserIdStr);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_007 end";
}

/**
 * @tc.name: SetUserIdTest_008
 * @tc.desc: Test SetUserId with maximum uint32_t value
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_008, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_008 start";
    // Test with maximum uint32_t value
    char maxUserIdStr[] = "4294967295";
    int ret = SetUserId(maxUserIdStr);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_008 end";
}

/**
 * @tc.name: SetUserIdTest_009
 * @tc.desc: Test SetUserId with value exceeding uint32_t range
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_009, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_009 start";
    // Test with value exceeding uint32_t range - should return -1
    char overflowUserIdStr[] = "4294967296";
    int ret = SetUserId(overflowUserIdStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_009 end";
}

/**
 * @tc.name: SetUserIdTest_010
 * @tc.desc: Test SetUserId with very large number causing long overflow
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_010, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_010 start";
    char largeOverflowStr[] = "999999999999999999999";
    int ret = SetUserId(largeOverflowStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_010 end";
}

/**
 * @tc.name: SetUserIdTest_011
 * @tc.desc: Test SetUserId with string containing only plus sign
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_011, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_011 start";
    char plusOnlyStr[] = "+";
    int ret = SetUserId(plusOnlyStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_011 end";
}

/**
 * @tc.name: SetUserIdTest_012
 * @tc.desc: Test SetUserId with string containing plus sign and digits
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_012, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_012 start";
    char plusDigitsStr[] = "+100";
    int ret = SetUserId(plusDigitsStr);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_012 end";
}

/**
 * @tc.name: SetUserIdTest_013
 * @tc.desc: Test SetUserId with hexadecimal string
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_013, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_013 start";
    char hexStr[] = "0x64";
    int ret = SetUserId(hexStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_013 end";
}

/**
 * @tc.name: SetUserIdTest_014
 * @tc.desc: Test SetUserId with string containing leading zeros
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_014, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_014 start";
    // Test with string containing leading zeros - should be valid
    char leadingZerosStr[] = "000100";
    int ret = SetUserId(leadingZerosStr);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_014 end";
}

/**
 * @tc.name: SetUserIdTest_015
 * @tc.desc: Test SetUserId with boundary value (UINT32_MAX - 1)
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_015, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_015 start";
    // Test with boundary value (UINT32_MAX - 1)
    char boundaryStr[] = "4294967294";
    int ret = SetUserId(boundaryStr);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_015 end";
}

/**
 * @tc.name: SetUserIdTest_016
 * @tc.desc: Test SetUserId with string containing tab character
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_016, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_016 start";
    // Test with string containing tab character - should return -1
    char tabStr[] = "100\t";
    int ret = SetUserId(tabStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_016 end";
}

/**
 * @tc.name: SetUserIdTest_017
 * @tc.desc: Test SetUserId with string containing newline character
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_017, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_017 start";
    // Test with string containing newline character - should return -1
    char newlineStr[] = "100\n";
    int ret = SetUserId(newlineStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_017 end";
}

/**
 * @tc.name: SetUserIdTest_018
 * @tc.desc: Test SetUserId with single digit
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_018, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_018 start";
    // Test with single digit - should be valid
    char singleDigitStr[] = "5";
    int ret = SetUserId(singleDigitStr);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_018 end";
}

/**
 * @tc.name: SetUserIdTest_019
 * @tc.desc: Test SetUserId with very long valid string
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_019, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_019 start";
    // Test with very long valid string (10 digits)
    char longValidStr[] = "1234567890";
    int ret = SetUserId(longValidStr);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_019 end";
}

/**
 * @tc.name: SetUserIdTest_020
 * @tc.desc: Test SetUserId with string containing decimal point
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_020, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_020 start";
    // Test with string containing decimal point - should return -1
    char decimalStr[] = "100.5";
    int ret = SetUserId(decimalStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_020 end";
}

/**
 * @tc.name: SetUserIdTest_021
 * @tc.desc: Test SetUserId with string containing comma
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_021, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_021 start";
    // Test with string containing comma - should return -1
    char commaStr[] = "1,000";
    int ret = SetUserId(commaStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_021 end";
}

/**
 * @tc.name: SetUserIdTest_022
 * @tc.desc: Test SetUserId with string containing space in middle
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_022, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_022 start";
    // Test with string containing space in middle - should return -1
    char spaceStr[] = "1 00";
    int ret = SetUserId(spaceStr);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "SetUserIdTest_022 end";
}

/**
 * @tc.name: SetUserIdTest_023
 * @tc.desc: Test SetUserId with minimum valid positive value
 * @tc.type: FUNC
 *
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_023, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_023 start";
    // Test with minimum valid positive value
    char minPositiveStr[] = "1";
    int ret = SetUserId(minPositiveStr);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "SetUserIdTest_023 end";
}

/**
 * @tc.name: SetUserIdTest_024
 * @tc.desc: Test SetUserId with ioctl failure
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCustomConfigTest, SetUserIdTest_024, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "SetUserIdTest_024 start";
    // Test with ioctl failure
    SetIoctlResult(-1);
    char userIdStr[] = "100";
    int ret = SetUserId(userIdStr);
    // Expected to return -1 when ioctl fails
    EXPECT_EQ(ret, -1);
    SetIoctlResult(0);
    GTEST_LOG_(INFO) << "SetUserIdTest_024 end";
}

/**
 * @tc.name: GetUserIdTest_001
 * @tc.desc: Test GetUserId with null pointer
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCustomConfigTest, GetUserIdTest_001, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetUserIdTest_001 start";
    // Test with null pointer - should return -1
    uint32_t *nullUserId = nullptr;
    int ret = GetUserId(nullUserId);
    EXPECT_EQ(ret, -1);
    GTEST_LOG_(INFO) << "GetUserIdTest_001 end";
}

/**
 * @tc.name: GetUserIdTest_002
 * @tc.desc: Test GetUserId with valid pointer
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCustomConfigTest, GetUserIdTest_002, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetUserIdTest_002 start";
    // Test with valid pointer
    uint32_t userId = 0;
    int ret = GetUserId(&userId);
    // Expected to return 0 on success
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "GetUserIdTest_002 end";
}

/**
 * @tc.name: GetUserIdTest_003
 * @tc.desc: Test GetUserId with ioctl failure
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCustomConfigTest, GetUserIdTest_003, TestSize.Level1)
{
    GTEST_LOG_(INFO) << "GetUserIdTest_003 start";
    // Test with ioctl failure
    SetIoctlResult(-1);
    uint32_t userId = 0;
    int ret = GetUserId(&userId);
    // Expected to return -1 when ioctl fails
    EXPECT_EQ(ret, -1);
    SetIoctlResult(0);
    GTEST_LOG_(INFO) << "GetUserIdTest_003 end";
}

}   // namespace OHOS
