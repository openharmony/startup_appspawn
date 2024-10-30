/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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


using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnUtilTest : public testing::Test {
public:
    const char* testDir = "testDir"; // 测试目录
    const char* nestedDir = "testDir/nestedDir"; // 嵌套目录
    std::vector<std::string> collectedStrings;

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        // 这里可以设置一些全局环境变量
        setenv("TEST_VAR", "test_value", 1);

        // 清除所有相关环境变量
        unsetenv("HNP_PRIVATE_HOME");
        unsetenv("HNP_PUBLIC_HOME");
        unsetenv("PATH");
        unsetenv("HOME");
        unsetenv("TMPDIR");
        unsetenv("SHELL");
        unsetenv("PWD");
    }
    void TearDown()
    {
        // 清理环境变量
        unsetenv("TEST_VAR");
    }
};

HWTEST_F(AppSpawnSandboxCoverageTest, ConvertEnvValue_01, TestSize.Level0)
{
    const char* srcEnv = "This is a test: ${TEST_VAR} end.";
    char dstEnv[100] = {0};

    int result = ConvertEnvValue(srcEnv, dstEnv, sizeof(dstEnv));
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dstEnv, "This is a test: test_value end.");
}

HWTEST_F(AppSpawnSandboxCoverageTest, ConvertEnvValue_02, TestSize.Level0)
{
    const char* srcEnv = "This will not resolve: ${NON_EXISTENT_VAR}.";
    char dstEnv[100] = {0};

    int result = ConvertEnvValue(srcEnv, dstEnv, sizeof(dstEnv));
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dstEnv, "This will not resolve: .");
}

HWTEST_F(AppSpawnSandboxCoverageTest, ConvertEnvValue_03, TestSize.Level0)
{
    const char* srcEnv = "Buffer size is too small: ${TEST_VAR}.";
    char dstEnv[10] = {0};  // Buffer too small

    int result = ConvertEnvValue(srcEnv, dstEnv, sizeof(dstEnv));
    EXPECT_EQ(result, -1);  // Expect failure due to buffer size
}

HWTEST_F(AppSpawnSandboxCoverageTest, ConvertEnvValue_04, TestSize.Level0)
{
    char dstEnv[100] = {0};

    int result = ConvertEnvValue(nullptr, dstEnv, sizeof(dstEnv));
    EXPECT_EQ(result, -1);  // Expect failure due to null source
}

HWTEST_F(AppSpawnSandboxCoverageTest, ConvertEnvValue_05, TestSize.Level0)
{
    const char* srcEnv = "This should not work: ${TEST_VAR}.";

    int result = ConvertEnvValue(srcEnv, nullptr, sizeof(dstEnv));
    EXPECT_EQ(result, -1);  // Expect failure due to null destination
}

HWTEST_F(AppSpawnSandboxCoverageTest, ConvertEnvValue_06, TestSize.Level0)
{
    const char* srcEnv = "";
    char dstEnv[100] = {0};

    int result = ConvertEnvValue(srcEnv, dstEnv, sizeof(dstEnv));
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dstEnv, "");  // Expect empty result
}
// 测试用例：检查环境变量是否正确设置
HWTEST_F(AppSpawnSandboxCoverageTest, InitCommonEnv_01, TestSize.Level0)
{
    InitCommonEnv();

    EXPECT_STREQ(getenv("HNP_PRIVATE_HOME"), "/data/app");
    EXPECT_STREQ(getenv("HNP_PUBLIC_HOME"), "/data/service/hnp");
    EXPECT_STREQ(getenv("HOME"), nullptr); // 期望为空，因为开发者模式关闭
    EXPECT_STREQ(getenv("TMPDIR"), nullptr); // 期望为空，因为开发者模式关闭
    EXPECT_STREQ(getenv("SHELL"), nullptr); // 期望为空，因为开发者模式关闭
    EXPECT_STREQ(getenv("PWD"), nullptr); // 期望为空，因为开发者模式关闭
}

// 测试用例：验证 PATH 环境变量的设置
HWTEST_F(AppSpawnSandboxCoverageTest, InitCommonEnv_02, TestSize.Level0)
{
    InitCommonEnv();

    const char* pathValue = getenv("PATH");
    ASSERT_NE(pathValue, nullptr); // 确保 PATH 不为空

    // 验证 PATH 中是否包含开发者模式下设置的变量
    EXPECT_NE(std::string(pathValue).find("/data/app/bin"), std::string::npos);
    EXPECT_NE(std::string(pathValue).find("/data/service/hnp/bin"), std::string::npos);
}

// 测试用例：验证开发者模式下未设置的环境变量
HWTEST_F(AppSpawnSandboxCoverageTest, InitCommonEnv_03, TestSize.Level0)
{
    InitCommonEnv();

    EXPECT_STREQ(getenv("HOME"), nullptr);
    EXPECT_STREQ(getenv("TMPDIR"), nullptr);
    EXPECT_STREQ(getenv("SHELL"), nullptr);
    EXPECT_STREQ(getenv("PWD"), nullptr);
}

// 测试用例：检查重复设置同一变量的情况
HWTEST_F(AppSpawnSandboxCoverageTest, InitCommonEnv_04, TestSize.Level0)
{
    setenv("HNP_PRIVATE_HOME", "/original/path", 1);
    InitCommonEnv();

    EXPECT_STREQ(getenv("HNP_PRIVATE_HOME"), "/data/app"); // 应被覆盖
}

// 测试用例：检查环境变量是否持久化
HWTEST_F(AppSpawnSandboxCoverageTest, InitCommonEnv_05, TestSize.Level0)
{
    InitCommonEnv();

    const char* originalPath = getenv("PATH");
    InitCommonEnv(); // 再次调用以检查是否持久化
    const char* newPath = getenv("PATH");

    EXPECT_STREQ(originalPath, newPath); // PATH 应该保持不变
}

// 测试用例：检查环境变量在不同调用中的一致性
HWTEST_F(AppSpawnSandboxCoverageTest, InitCommonEnv_06, TestSize.Level0)
{
    InitCommonEnv();
    const char* privateHome1 = getenv("HNP_PRIVATE_HOME");
    
    // 重置并再调用
    SetUp();
    InitCommonEnv();
    const char* privateHome2 = getenv("HNP_PRIVATE_HOME");

    EXPECT_STREQ(privateHome1, privateHome2); // 应保持一致
}

// 测试用例：起始时间和结束时间相同
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_01, TestSize.Level0)
{
    struct timespec startTime = {5, 500000000}; // 5.5 秒
    struct timespec endTime = {5, 500000000};   // 5.5 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 0);
}

// 测试用例：结束时间晚于起始时间
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_02, TestSize.Level0)
{
    struct timespec startTime = {1, 200000000}; // 1.2 秒
    struct timespec endTime = {1, 800000000};   // 1.8 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 600000); // 600 ms = 600000 us
}

// 测试用例：跨秒的时间差
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_03, TestSize.Level0)
{
    struct timespec startTime = {1, 999000000}; // 1.999 秒
    struct timespec endTime = {2, 500000000};   // 2.5 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 501000); // 501 ms = 501000 us
}

// 测试用例：结束时间早于起始时间
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_04, TestSize.Level0)
{
    struct timespec startTime = {3, 500000000}; // 3.5 秒
    struct timespec endTime = {3, 200000000};   // 3.2 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 300000); // 300 ms = 300000 us
}

// 测试用例：NULL 指针处理
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_05, TestSize.Level0)
{
    struct timespec endTime = {3, 200000000}; // 3.2 秒
    EXPECT_EQ(DiffTime(NULL, &endTime), 0);
}

HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_06, TestSize.Level0)
{
    struct timespec startTime = {3, 500000000}; // 3.5 秒
    EXPECT_EQ(DiffTime(&startTime, NULL), 0);
}

// 测试用例：两个 NULL 指针
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_07, TestSize.Level0)
{
    EXPECT_EQ(DiffTime(NULL, NULL), 0);
}

// 测试用例：结束时间早于起始时间（负值情况）
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_08, TestSize.Level0)
{
    struct timespec startTime = {2, 500000000}; // 2.5 秒
    struct timespec endTime = {2, 200000000};   // 2.2 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 300000); // 300 ms = 300000 us
}

// 测试用例：起始时间和结束时间的纳秒部分不同
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_09, TestSize.Level0)
{
    struct timespec startTime = {1, 500000000}; // 1.5 秒
    struct timespec endTime = {1, 999000000};   // 1.999 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 499000); // 499 ms = 499000 us
}

// 测试用例：大时间间隔
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_10, TestSize.Level0)
{
    struct timespec startTime = {1000, 0}; // 1000 秒
    struct timespec endTime = {2000, 0};   // 2000 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 1000000000); // 1000 秒 = 1000000000 us
}

// 测试用例：时间为零的情况
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_11, TestSize.Level0)
{
    struct timespec startTime = {0, 0}; // 0 秒
    struct timespec endTime = {0, 0};   // 0 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 0);
}

// 测试用例：非常大的时间值
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_12, TestSize.Level0)
{
    struct timespec startTime = {INT32_MAX, 999999999}; // 最大整型值
    struct timespec endTime = {INT32_MAX + 1, 0};      // 超过最大值

    EXPECT_EQ(DiffTime(&startTime, &endTime), 1); // 应返回 1 微秒
}

// 测试用例：相同的秒和纳秒但以不同的顺序
HWTEST_F(AppSpawnSandboxCoverageTest, DiffTime_13, TestSize.Level0)
{
    struct timespec startTime = {2, 500000000}; // 2.5 秒
    struct timespec endTime = {2, 500000000};   // 2.5 秒

    EXPECT_EQ(DiffTime(&startTime, &endTime), 0);
}

// 测试用例：创建嵌套目录
HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_01, TestSize.Level0)
{
    EXPECT_EQ(MakeDirRec(nestedDir, 0755, 1), 0);
    struct stat sb;
    EXPECT_EQ(stat("testDir", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
    EXPECT_EQ(stat("testDir/nestedDir", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
}

// 测试用例：已存在的目录
HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_02, TestSize.Level0)
{
    mkdir(testDir, 0755); // 先手动创建
    EXPECT_EQ(MakeDirRec(nestedDir, 0755, 1), 0);
    // 确保目录依然存在
    struct stat sb;
    EXPECT_EQ(stat("testDir", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
    EXPECT_EQ(stat("testDir/nestedDir", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
}

// 测试用例：无效路径
HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_03, TestSize.Level0)
{
    EXPECT_EQ(MakeDirRec(NULL, 0755, 1), -1);
    EXPECT_EQ(MakeDirRec("", 0755, 1), -1);
}

// 测试用例：仅创建最后路径
HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_04, TestSize.Level0)
{
    EXPECT_EQ(MakeDirRec(testDir, 0755, 1), 0);
    struct stat sb;
    EXPECT_EQ(stat("testDir", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
}

// 测试用例：未创建中间目录
HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_05, TestSize.Level0)
{
    EXPECT_EQ(MakeDirRec("testDir/nestedDir", 0755, 1), 0);
    // 应该无法创建中间目录
    struct stat sb;
    EXPECT_EQ(stat("testDir", &sb), -1); // 中间目录应不存在
    EXPECT_EQ(errno, ENOENT);
}

HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_06, TestSize.Level0)
{
    EXPECT_EQ(MakeDirRec("invalidModeDir", 1000, 1), -1);
    EXPECT_EQ(errno, EINVAL);
}

HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_07, TestSize.Level0)
{
    const char* multiDir = "testDir1/testDir2/testDir3";
    EXPECT_EQ(MakeDirRec(multiDir, 0755, 1), 0);
    struct stat sb;
    EXPECT_EQ(stat("testDir1/testDir2", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
    EXPECT_EQ(stat("testDir1/testDir2/testDir3", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
}

HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_08, TestSize.Level0)
{
    mkdir(testDir, 0755); // 创建中间目录
    EXPECT_EQ(MakeDirRec("testDir/partiallyExists/nestedDir", 0755, 1), 0);
    struct stat sb;
    EXPECT_EQ(stat("testDir/partiallyExists", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
    EXPECT_EQ(stat("testDir/partiallyExists/nestedDir", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
}

HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_09, TestSize.Level0)
{
    mkdir("noPermissionDir", 0400); // 创建只读目录
    EXPECT_EQ(MakeDirRec("noPermissionDir/nestedDir", 0755, 1), -1);
    EXPECT_EQ(errno, EACCES);
    rmdir("noPermissionDir"); // 清理
}

HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_10, TestSize.Level0)
{
    std::string longPath = "testDir/" + std::string(1000, 'a');
    EXPECT_EQ(MakeDirRec(longPath.c_str(), 0755, 1), -1);
    EXPECT_EQ(errno, ENAMETOOLONG);
}

HWTEST_F(AppSpawnSandboxCoverageTest, MakeDirRec_11, TestSize.Level0)
{
    const char* specialDir = "test Dir/@!#";
    EXPECT_EQ(MakeDirRec(specialDir, 0755, 1), 0);
    struct stat sb;
    EXPECT_EQ(stat("test Dir/@!#", &sb), 0);
    EXPECT_TRUE(S_ISDIR(sb.st_mode));
}

// 测试用例
HWTEST_F(AppSpawnSandboxCoverageTest, StringSplit_01, TestSize.Level0)
{
    const char* input = "one,two,three";
    const char* separator = ",";
    StringSplit(input, separator, &collectedStrings, CollectStrings);
    EXPECT_EQ(collectedStrings.size(), 3);
    EXPECT_EQ(collectedStrings[0], "one");
    EXPECT_EQ(collectedStrings[1], "two");
    EXPECT_EQ(collectedStrings[2], "three");
}

HWTEST_F(AppSpawnSandboxCoverageTest, StringSplit_02, TestSize.Level0)
{
    const char* input = "one,,three";
    const char* separator = ",";
    StringSplit(input, separator, &collectedStrings, CollectStrings);
    EXPECT_EQ(collectedStrings.size(), 3);
    EXPECT_EQ(collectedStrings[0], "one");
    EXPECT_EQ(collectedStrings[1], "");
    EXPECT_EQ(collectedStrings[2], "three");
}

HWTEST_F(AppSpawnSandboxCoverageTest, StringSplit_03, TestSize.Level0)
{
    const char* input = "  one, two , three  ";
    const char* separator = ",";
    StringSplit(input, separator, &collectedStrings, CollectStrings);
    EXPECT_EQ(collectedStrings.size(), 3);
    EXPECT_EQ(collectedStrings[0], "  one");
    EXPECT_EQ(collectedStrings[1], " two ");
    EXPECT_EQ(collectedStrings[2], " three  ");
}

HWTEST_F(AppSpawnSandboxCoverageTest, StringSplit_04, TestSize.Level0)
{
    const char* input = "onelinetext";
    const char* separator = ",";
    StringSplit(input, separator, &collectedStrings, CollectStrings);
    EXPECT_EQ(collectedStrings.size(), 1);
    EXPECT_EQ(collectedStrings[0], "onelinetext");
}

HWTEST_F(AppSpawnSandboxCoverageTest, StringSplit_05, TestSize.Level0)
{
    const char* input = "";
    const char* separator = ",";
    StringSplit(input, separator, &collectedStrings, CollectStrings);
    EXPECT_EQ(collectedStrings.size(), 0);
}

HWTEST_F(AppSpawnSandboxCoverageTest, StringSplit_06, TestSize.Level0)
{
    const char* separator = ",";
    int result = StringSplit(nullptr, separator, &collectedStrings, CollectStrings);
    EXPECT_EQ(result, APPSPAWN_ARG_INVALID);
}

HWTEST_F(AppSpawnSandboxCoverageTest, StringSplit_07, TestSize.Level0)
{
    const char* input = "one,two,three";
    int result = StringSplit(input, nullptr, &collectedStrings, CollectStrings);
    EXPECT_EQ(result, APPSPAWN_ARG_INVALID);
}

HWTEST_F(AppSpawnSandboxCoverageTest, StringSplit_08, TestSize.Level0)
{
    const char* input = "one,two,three";
    const char* separator = ",";
    int result = StringSplit(input, separator, &collectedStrings, nullptr);
    EXPECT_EQ(result, APPSPAWN_ARG_INVALID);
}

// 测试用例
HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_01, TestSize.Level0)
{
    const char* input = "hello world, hello";
    const char* search = "hello";
    char* result = GetLastStr(input, search);
    EXPECT_STREQ(result, "hello"); // 检查返回的字符串
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_02, TestSize.Level0)
{
    const char* input = "hello world";
    const char* search = "test";
    char* result = GetLastStr(input, search);
    EXPECT_EQ(result, nullptr); // 没有匹配，返回 nullptr
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_03, TestSize.Level0)
{
    const char* input = "";
    const char* search = "hello";
    char* result = GetLastStr(input, search);
    EXPECT_EQ(result, nullptr); // 空字符串，返回 nullptr
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_04, TestSize.Level0)
{
    const char* search = "hello";
    char* result = GetLastStr(nullptr, search);
    EXPECT_EQ(result, nullptr); // 输入为 nullptr，返回 nullptr
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_05, TestSize.Level0)
{
    const char* input = "hello world";
    char* result = GetLastStr(input, nullptr);
    EXPECT_EQ(result, nullptr); // 查找字符串为 nullptr，返回 nullptr
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_06, TestSize.Level0)
{
    const char* input = "hello hello";
    const char* search = "hello";
    char* result = GetLastStr(input, search);
    EXPECT_STREQ(result, "hello"); // 最后一次出现的字符串
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_07, TestSize.Level0)
{
    const char* input = "hello world hello";
    const char* search = "hello";
    char* result = GetLastStr(input, search);
    EXPECT_STREQ(result, "hello"); // 最后一次出现的字符串
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_08, TestSize.Level0)
{
    const char* input = "hello   world   hello";
    const char* search = "hello";
    char* result = GetLastStr(input, search);
    EXPECT_STREQ(result, "hello"); // 处理空格
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetLastStr_09, TestSize.Level0)
{
    const char* input = "abcabcabc";
    const char* search = "abc";
    char* result = GetLastStr(input, search);
    EXPECT_STREQ(result, "abcabcabc"); // 测试重叠匹配
}

// 测试用例
HWTEST_F(AppSpawnSandboxCoverageTest, CreateTestFile_01, TestSize.Level0)
{
    const char* fileName = "test_success.txt";
    const char* content = "Hello, World!";
    CreateTestFile(fileName, content);

    char* result = ReadFile(fileName);
    ASSERT_NE(result, nullptr); // 确保返回值不为 nullptr
    EXPECT_STREQ(result, content); // 检查读取内容是否正确

    free(result); // 释放内存
    RemoveTestFile(fileName); // 清理测试文件
}

HWTEST_F(AppSpawnSandboxCoverageTest, CreateTestFile_02, TestSize.Level0)
{
    const char* fileName = "nonexistent_file.txt";
    char* result = ReadFile(fileName);
    EXPECT_EQ(result, nullptr); // 应返回 nullptr
}

HWTEST_F(AppSpawnSandboxCoverageTest, CreateTestFile_03, TestSize.Level0)
{
    const char* fileName = "empty_file.txt";
    CreateTestFile(fileName, ""); // 创建空文件

    char* result = ReadFile(fileName);
    EXPECT_EQ(result, nullptr); // 应返回 nullptr

    RemoveTestFile(fileName); // 清理测试文件
}

HWTEST_F(AppSpawnSandboxCoverageTest, CreateTestFile_04, TestSize.Level0)
{
    const char* fileName = "large_file.txt";
    const size_t size = MAX_JSON_FILE_LEN + 1; // 假设定义了 MAX_JSON_FILE_LEN
    char* content = new char[size];
    int ret = memset_s(content, 'A', size - 1);
    if (ret != 0) {
        return;
    }
    content[size - 1] = '\0'; // 确保以 null 结尾

    CreateTestFile(fileName, content); // 创建超大文件

    char* result = ReadFile(fileName);
    EXPECT_EQ(result, nullptr); // 应返回 nullptr

    delete[] content; // 清理内存
    RemoveTestFile(fileName); // 清理测试文件
}

HWTEST_F(AppSpawnSandboxCoverageTest, CreateTestFile_05, TestSize.Level0)
{
    char* result = ReadFile(nullptr); // 传入 nullptr
    EXPECT_EQ(result, nullptr); // 应返回 nullptr
}

// 测试用例
HWTEST_F(AppSpawnSandboxCoverageTest, DumpCurrentDir_01, TestSize.Level0)
{
    int ret = DumpCurrentDir(NULL, 100, "test_dir");
    EXPECT_EQ(ret, /* 需要返回的错误码 */);
}

HWTEST_F(AppSpawnSandboxCoverageTest, DumpCurrentDir_02, TestSize.Level0)
{
    char buffer[100];
    DumpCurrentDir(buffer, 100, NULL);
    // 检查是否处理了错误，可能通过日志或返回值
}

HWTEST_F(AppSpawnSandboxCoverageTest, DumpCurrentDir_03, TestSize.Level0)
{
    char buffer[100];
    DumpCurrentDir(buffer, 0, "test_dir");
    // 检查是否处理了错误
}

HWTEST_F(AppSpawnSandboxCoverageTest, DumpCurrentDir_04, TestSize.Level0)
{
    char buffer[100];
    DumpCurrentDir(buffer, 100, "invalid_dir");
    // 检查是否处理了打开目录失败的情况
}

HWTEST_F(AppSpawnSandboxCoverageTest, DumpCurrentDir_05, TestSize.Level0)
{
    // 创建一个空目录
    mkdir("empty_dir", 0755);
    char buffer[100];
    DumpCurrentDir(buffer, 100, "empty_dir");
    // 检查没有输出
    rmdir("empty_dir");
}
<<<<<<< Updated upstream

HWTEST_F(AppSpawnSandboxCoverageTest, DumpCurrentDir_06, TestSize.Level0)
{
    // 创建一个只包含 . 和 .. 的目录
    mkdir("dot_dir", 0755);
    chdir("dot_dir");
    char buffer[100];
    DumpCurrentDir(buffer, 100, ".");
    // 检查没有输出
    chdir("..");
    rmdir("dot_dir");
}

HWTEST_F(AppSpawnSandboxCoverageTest, DumpCurrentDir_7, TestSize.Level0)
{
    char buffer[256];
    int ret = memset_s(buffer, 0, sizeof(buffer)); // 清空 buffer
    if (ret != 0) {
        return;
    }
    DumpCurrentDir(buffer, sizeof(buffer), "test_dir");
    
    // 检查 buffer 是否包含预期的输出
    EXPECT_NE(strstr(buffer, "Current path test_dir/test_file.txt"), nullptr);
    EXPECT_NE(strstr(buffer, "Current path test_dir/sub_dir"), nullptr);
}

=======
<<<<<<< Updated upstream
=======

// 测试用例
HWTEST_F(AppSpawnSandboxCoverageTest, IsDeveloperModeOpen_01, TestSize.Level0)
{
    mockValue = "true";
    mockReturnValue = 1;  // 模拟成功
    EXPECT_EQ(IsDeveloperModeOpen(), 1);  // 期待返回 1
}

HWTEST_F(AppSpawnSandboxCoverageTest, IsDeveloperModeOpen_02, TestSize.Level0)
{
    mockValue = "false";
    mockReturnValue = 1;  // 模拟成功
    EXPECT_EQ(IsDeveloperModeOpen(), 0);  // 期待返回 0
}

HWTEST_F(AppSpawnSandboxCoverageTest, IsDeveloperModeOpen_03, TestSize.Level0)
{
    mockValue = "true";
    mockReturnValue = -1;  // 模拟失败
    EXPECT_EQ(IsDeveloperModeOpen(), 0);  // 期待返回 0
}

HWTEST_F(AppSpawnSandboxCoverageTest, IsDeveloperModeOpen_04, TestSize.Level0)
{
    mockValue = "false";
    mockReturnValue = 0;  // 模拟未找到参数
    EXPECT_EQ(IsDeveloperModeOpen(), 0);  // 期待返回 0
}

HWTEST_F(AppSpawnSandboxCoverageTest, IsDeveloperModeOpen_05, TestSize.Level0)
{
    mockValue = nullptr;  // 模拟未设置值
    mockReturnValue = 0;  // 模拟未找到参数
    EXPECT_EQ(IsDeveloperModeOpen(), 0);  // 期待返回 0
}

// 测试用例
HWTEST_F(AppSpawnSandboxCoverageTest, GetSpawnTimeout_01, TestSize.Level0)
{
    mockValue = "30";  // 设置模拟返回值
    mockReturnValue = 1;  // 模拟成功返回
    EXPECT_EQ(GetSpawnTimeout(10), 30);  // 期待返回 30
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetSpawnTimeout_02, TestSize.Level0)
{
    mockValue = "0";  // 设置模拟返回值为 "0"
    mockReturnValue = 1;  // 模拟成功返回
    EXPECT_EQ(GetSpawnTimeout(10), 10);  // 期待返回默认值 10
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetSpawnTimeout_03, TestSize.Level0)
{
    mockValue = nullptr;  // 模拟未设置值
    mockReturnValue = 0;  // 模拟未找到参数
    EXPECT_EQ(GetSpawnTimeout(10), 10);  // 期待返回默认值 10
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetSpawnTimeout_04, TestSize.Level0)
{
    mockValue = "abc";  // 设置模拟返回值为无效的数字
    mockReturnValue = 1;  // 模拟成功返回
    EXPECT_EQ(GetSpawnTimeout(10), 10);  // 期待返回默认值 10
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetSpawnTimeout_05, TestSize.Level0)
{
    mockValue = "5";  // 设置模拟返回值小于默认值
    mockReturnValue = 1;  // 模拟成功返回
    EXPECT_EQ(GetSpawnTimeout(10), 10);  // 期待返回默认值 10
}

HWTEST_F(AppSpawnSandboxCoverageTest, GetSpawnTimeout_06, TestSize.Level0)
{
    mockValue = "20";  // 设置模拟返回值大于默认值
    mockReturnValue = 1;  // 模拟成功返回
    EXPECT_EQ(GetSpawnTimeout(10), 20);  // 期待返回 20
}


>>>>>>> Stashed changes
>>>>>>> Stashed changes
}  // namespace OHOS
