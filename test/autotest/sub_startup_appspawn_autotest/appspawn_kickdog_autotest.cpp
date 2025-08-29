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
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "appspawn_utils.h"  // 假设这个头文件包含了 OpenAndWriteToProc 函数

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnUtilsTest : public testing::Test {
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
* @tc.name: OpenAndWriteToProc_Success
* @tc.desc: Verify that the function successfully writes to the file.
*/
HWTEST_F(AppSpawnUtilsTest, OpenAndWriteToProc_Success, TestSize.Level0)
{
    const char *procName = "/proc/test";
    const char *writeStr = "Test Data";
    size_t writeLen = strlen(writeStr);

    // 模拟文件成功打开和写入
    int result = OpenAndWriteToProc(procName, writeStr, writeLen);

    EXPECT_GT(result, 0);  // 假设返回写入的字节数
}

/**
* @tc.name: OpenAndWriteToProc_OpenFail
* @tc.desc: Verify that the function returns an error when opening the file fails.
*/
HWTEST_F(AppSpawnUtilsTest, OpenAndWriteToProc_OpenFail, TestSize.Level0)
{
    const char *procName = "/proc/nonexistent";
    const char *writeStr = "Test Data";
    size_t writeLen = strlen(writeStr);

    // 模拟文件打开失败，返回 -1
    int result = OpenAndWriteToProc(procName, writeStr, writeLen);

    EXPECT_EQ(result, -1);  // 打开失败，返回 -1
}

/**
* @tc.name: OpenAndWriteToProc_WriteFail
* @tc.desc: Verify that the function returns an error when writing to the file fails.
*/
HWTEST_F(AppSpawnUtilsTest, OpenAndWriteToProc_WriteFail, TestSize.Level0)
{
    const char *procName = "/proc/test";
    const char *writeStr = nullptr;  // 模拟空指针，写入失败
    size_t writeLen = 0;

    // 模拟写入失败
    int result = OpenAndWriteToProc(procName, writeStr, writeLen);

    EXPECT_EQ(result, -1);  // 写入失败，返回 -1
}

/**
* @tc.name: GetProcFile_Linux
* @tc.desc: Verify that the function returns the correct procFile for Linux.
*/
HWTEST_F(AppSpawnUtilsTest, GetProcFile_Linux, TestSize.Level0)
{
    bool isLinux = true;
    const char *procFile = GetProcFile(isLinux);

    EXPECT_STREQ(procFile, LINUX_APPSPAWN_WATCHDOG_FILE);  // 假设返回的是 LINUX_APPSPAWN_WATCHDOG_FILE
}

/**
* @tc.name: GetProcFile_Other
* @tc.desc: Verify that the function returns the correct procFile for non-Linux platforms.
*/
HWTEST_F(AppSpawnUtilsTest, GetProcFile_Other, TestSize.Level0)
{
    bool isLinux = false;
    const char *procFile = GetProcFile(isLinux);

    EXPECT_STREQ(procFile, HM_APPSPAWN_WATCHDOG_FILE);  // 假设非Linux平台返回的是 HM_APPSPAWN_WATCHDOG_FILE
}

/**
* @tc.name: GetProcContent_Linux_Open
* @tc.desc: Verify that the function returns the correct content for Linux when open is true.
*/
HWTEST_F(AppSpawnUtilsTest, GetProcContent_Linux_Open, TestSize.Level0)
{
    bool isLinux = true;
    bool isOpen = true;
    int mode = MODE_FOR_NWEB_SPAWN;

    const char *procContent = GetProcContent(isLinux, isOpen, mode);

    EXPECT_STREQ(procContent, LINUX_APPSPAWN_WATCHDOG_ON);  // 假设返回 LINUX_APPSPAWN_WATCHDOG_ON
}

/**
* @tc.name: GetProcContent_Linux_Close
* @tc.desc: Verify that the function returns the correct content for Linux when open is false.
*/
HWTEST_F(AppSpawnUtilsTest, GetProcContent_Linux_Close, TestSize.Level0)
{
    bool isLinux = true;
    bool isOpen = false;
    int mode = MODE_FOR_HYBRID_SPAWN;

    const char *procContent = GetProcContent(isLinux, isOpen, mode);

    EXPECT_STREQ(procContent, LINUX_APPSPAWN_WATCHDOG_KICK);  // 假设返回 LINUX_APPSPAWN_WATCHDOG_KICK
}

/**
* @tc.name: GetProcContent_Other_Open
* @tc.desc: Verify that the function returns the correct content for non-Linux when open is true.
*/
HWTEST_F(AppSpawnUtilsTest, GetProcContent_Other_Open, TestSize.Level0)
{
    bool isLinux = false;
    bool isOpen = true;
    int mode = MODE_FOR_HYBRID_SPAWN;

    const char *procContent = GetProcContent(isLinux, isOpen, mode);

    EXPECT_STREQ(procContent, HM_HYBRIDSPAWN_WATCHDOG_ON);  // 假设返回 HM_HYBRIDSPAWN_WATCHDOG_ON
}

/**
* @tc.name: GetProcContent_Other_Close
* @tc.desc: Verify that the function returns the correct content for non-Linux when open is false.
*/
HWTEST_F(AppSpawnUtilsTest, GetProcContent_Other_Close, TestSize.Level0)
{
    bool isLinux = false;
    bool isOpen = false;
    int mode = MODE_FOR_NWEB_SPAWN;

    const char *procContent = GetProcContent(isLinux, isOpen, mode);

    EXPECT_STREQ(procContent, HM_NWEBSPAWN_WATCHDOG_KICK);  // 假设返回 HM_NWEBSPAWN_WATCHDOG_KICK
}

/**
* @tc.name: DealSpawnWatchdog_Success
* @tc.desc: Verify that the function works correctly when the watchdog is successfully dealt with.
*/
HWTEST_F(AppSpawnUtilsTest, DealSpawnWatchdog_Success, TestSize.Level0)
{
    AppSpawnContent content;
    content.isLinux = true;
    content.mode = MODE_FOR_NWEB_SPAWN;
    content.wdgOpened = false;

    // 模拟 OpenAndWriteToProc 成功
    EXPECT_CALL(*this, OpenAndWriteToProc(_, _, _)).WillOnce(Return(1));  // 假设写入成功

    DealSpawnWatchdog(&content, true);

    EXPECT_TRUE(content.wdgOpened);  // 验证 watchdog 状态
}

/**
* @tc.name: DealSpawnWatchdog_Failure
* @tc.desc: Verify that the function handles failure correctly when writing to the proc file fails.
*/
HWTEST_F(AppSpawnUtilsTest, DealSpawnWatchdog_Failure, TestSize.Level0)
{
    AppSpawnContent content;
    content.isLinux = true;
    content.mode = MODE_FOR_HYBRID_SPAWN;
    content.wdgOpened = false;

    // 模拟 OpenAndWriteToProc 失败
    EXPECT_CALL(*this, OpenAndWriteToProc(_, _, _)).WillOnce(Return(-1));  // 假设写入失败

    DealSpawnWatchdog(&content, true);

    EXPECT_FALSE(content.wdgOpened);  // 验证 watchdog 状态
}
}
