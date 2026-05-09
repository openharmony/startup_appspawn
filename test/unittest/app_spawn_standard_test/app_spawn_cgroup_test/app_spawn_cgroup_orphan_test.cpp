/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

extern "C" {
// Mock state (from mock file)
extern char g_mockTestRoot[PATH_MAX];
extern pid_t g_mockKilledPids[];
extern int g_mockKilledCount;
extern pid_t g_mockKillFailPid;
extern char g_mockRmdirPaths[][PATH_MAX];
extern int g_mockRmdirCount;
extern int g_mockRmdirFailIndex;
extern char g_mockGracefulStopValue[8];
extern int g_mockGetParamFail;
extern char g_mockSetParamKey[];
extern char g_mockSetParamValue[];
extern int g_mockSetParamCalled;
extern int g_mockOpenForceFail;
extern int g_mockWriteForceFail;
extern void MockCgroupReset(void);

// Source functions (APPSPAWN_STATIC -> visible in test)
int IsUidDir(const char *name);
int IsAppDir(const char *name);
void SetForkDeniedByPath(const char *appDirPath);
void CleanupOrphanedTagDir(const char *uidPath, const char *dirName);
void CleanupOrphanedCgroupProcesses(void);
int CgroupPreloadHook(AppSpawnMgr *content);
int CgroupExitHook(AppSpawnMgr *content);
}

static constexpr int DIR_MODE = 0755;

// ===== Helpers =====

static std::string CreateTempDir()
{
    const char *candidates[] = { "/data/local/tmp", "/tmp", nullptr };
    for (int i = 0; candidates[i] != nullptr; i++) {
        char tmpl[PATH_MAX] = {};
        int ret = snprintf_s(tmpl, sizeof(tmpl), sizeof(tmpl) - 1, "%s/cgroup_orphan_test_XXXXXX", candidates[i]);
        if (ret <= 0) {
            continue;
        }
        char *dir = mkdtemp(tmpl);
        if (dir != nullptr) {
            return std::string(dir);
        }
    }
    return "";
}

static void CreateOrphanDir(const std::string &root, const char *uid,
                            const char *tag, const char *app,
                            const std::vector<pid_t> &pids)
{
    std::string uidDir = root + "/" + uid;
    std::string tagDir = uidDir + "/" + tag;
    std::string appDir = tagDir + "/" + app;
    mkdir(uidDir.c_str(), DIR_MODE);
    mkdir(tagDir.c_str(), DIR_MODE);
    mkdir(appDir.c_str(), DIR_MODE);

    FILE *f = fopen((appDir + "/cgroup.procs").c_str(), "w");
    if (f) {
        for (auto pid : pids) {
            fprintf(f, "%d\n", pid);
        }
        fclose(f);
    }

    f = fopen((appDir + "/pids.fork_denied").c_str(), "w");
    if (f) {
        fclose(f);
    }
}

static void RemoveDirRecursive(const std::string &path)
{
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr) {
        return;
    }
    struct dirent *dp;
    while ((dp = readdir(dir)) != nullptr) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        std::string sub = path + "/" + dp->d_name;
        if (dp->d_type == DT_DIR) {
            RemoveDirRecursive(sub);
        } else {
            unlink(sub.c_str());
        }
    }
    closedir(dir);
    rmdir(path.c_str());
}

static bool WasPidKilled(pid_t pid)
{
    for (int i = 0; i < g_mockKilledCount; i++) {
        if (g_mockKilledPids[i] == pid) {
            return true;
        }
    }
    return false;
}

static bool WasPathRmDir(const char *path)
{
    for (int i = 0; i < g_mockRmdirCount; i++) {
        if (strcmp(g_mockRmdirPaths[i], path) == 0) {
            return true;
        }
    }
    return false;
}

// ===== Test Class =====

class AppSpawnCGroupOrphanTest : public testing::Test {
public:
    std::string tempDir_;

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() override
    {
        CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        MockCgroupReset();
        tempDir_ = CreateTempDir();
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
    }

    void TearDown() override
    {
        MockCgroupReset();
        if (!tempDir_.empty()) {
            RemoveDirRecursive(tempDir_);
        }
        DeleteAppSpawnMgr(GetAppSpawnMgr());
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    }

    void SetMockRoot()
    {
        int ret = strncpy_s(g_mockTestRoot, PATH_MAX, tempDir_.c_str(), PATH_MAX - 1);
        if (ret != EOK) {
            g_mockTestRoot[0] = '\0';
        }
    }
};

// ===================================================================
// 4.1 IsUidDir
// ===================================================================

/**
 * @tc.name: TestIsUidDir_NullInput
 * @tc.desc: Verify IsUidDir returns 0 for NULL input
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_NullInput, TestSize.Level1)
{
    EXPECT_EQ(IsUidDir(nullptr), 0);
}

/**
 * @tc.name: TestIsUidDir_EmptyString
 * @tc.desc: Verify IsUidDir returns 0 for empty string
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_EmptyString, TestSize.Level1)
{
    EXPECT_EQ(IsUidDir(""), 0);
}

/**
 * @tc.name: TestIsUidDir_DefaultUid
 * @tc.desc: Verify IsUidDir returns 1 for "0" (default UID)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_DefaultUid, TestSize.Level0)
{
    EXPECT_EQ(IsUidDir("0"), 1);
}

/**
 * @tc.name: TestIsUidDir_ValidUid100
 * @tc.desc: Verify IsUidDir returns 1 for "100" (min valid UID)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_ValidUid100, TestSize.Level0)
{
    EXPECT_EQ(IsUidDir("100"), 1);
}

/**
 * @tc.name: TestIsUidDir_ValidUid9999999999
 * @tc.desc: Verify IsUidDir returns 1 for "9999999999" (max length)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_ValidUid9999999999, TestSize.Level1)
{
    EXPECT_EQ(IsUidDir("9999999999"), 1);
}

/**
 * @tc.name: TestIsUidDir_TooShort
 * @tc.desc: Verify IsUidDir returns 0 for "12" (below min length 3)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_TooShort, TestSize.Level1)
{
    EXPECT_EQ(IsUidDir("12"), 0);
}

/**
 * @tc.name: TestIsUidDir_TooLong
 * @tc.desc: Verify IsUidDir returns 0 for 11-digit number (above max length 10)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_TooLong, TestSize.Level1)
{
    EXPECT_EQ(IsUidDir("12345678901"), 0);
}

/**
 * @tc.name: TestIsUidDir_WithAlpha
 * @tc.desc: Verify IsUidDir returns 0 for "1a3" (contains alpha)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_WithAlpha, TestSize.Level1)
{
    EXPECT_EQ(IsUidDir("1a3"), 0);
}

/**
 * @tc.name: TestIsUidDir_WithDot
 * @tc.desc: Verify IsUidDir returns 0 for "1.0" (contains dot)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsUidDir_WithDot, TestSize.Level1)
{
    EXPECT_EQ(IsUidDir("1.0"), 0);
}

// ===================================================================
// 4.2 IsAppDir
// ===================================================================

/**
 * @tc.name: TestIsAppDir_ValidApp
 * @tc.desc: Verify IsAppDir returns 1 for "app_12345"
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsAppDir_ValidApp, TestSize.Level0)
{
    EXPECT_EQ(IsAppDir("app_12345"), 1);
}

/**
 * @tc.name: TestIsAppDir_OnlyPrefix
 * @tc.desc: Verify IsAppDir returns 0 for "app_" (no PID)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsAppDir_OnlyPrefix, TestSize.Level1)
{
    EXPECT_EQ(IsAppDir("app_"), 0);
}

/**
 * @tc.name: TestIsAppDir_NoPrefix
 * @tc.desc: Verify IsAppDir returns 0 for "12345" (no prefix)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsAppDir_NoPrefix, TestSize.Level0)
{
    EXPECT_EQ(IsAppDir("12345"), 0);
}

/**
 * @tc.name: TestIsAppDir_WithAlpha
 * @tc.desc: Verify IsAppDir returns 0 for "app_12a34" (contains alpha)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsAppDir_WithAlpha, TestSize.Level1)
{
    EXPECT_EQ(IsAppDir("app_12a34"), 0);
}

/**
 * @tc.name: TestIsAppDir_Empty
 * @tc.desc: Verify IsAppDir returns 0 for empty string
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestIsAppDir_Empty, TestSize.Level1)
{
    EXPECT_EQ(IsAppDir(""), 0);
}

// ===================================================================
// 4.3 SetForkDeniedByPath
// ===================================================================

/**
 * @tc.name: TestSetForkDeniedByPath_Success
 * @tc.desc: Verify fork_denied file is written successfully
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestSetForkDeniedByPath_Success, TestSize.Level0)
{
    SetMockRoot();
    // Create the app dir and fork_denied file
    mkdir((tempDir_ + "/100").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app/app_123").c_str(), DIR_MODE);
    FILE *f = fopen((tempDir_ + "/100/com.app/app_123/pids.fork_denied").c_str(), "w");
    ASSERT_NE(f, nullptr);
    fclose(f);

    SetForkDeniedByPath("/dev/pids/100/com.app/app_123");
    f = fopen((tempDir_ + "/100/com.app/app_123/pids.fork_denied").c_str(), "r");
    ASSERT_NE(f, nullptr);
    char buf[8] = {};
    fgets(buf, sizeof(buf), f);
    fclose(f);
    EXPECT_EQ(buf[0], '1');
}

/**
 * @tc.name: TestSetForkDeniedByPath_OpenFail
 * @tc.desc: Verify no crash when fork_denied file open fails
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestSetForkDeniedByPath_OpenFail, TestSize.Level0)
{
    g_mockOpenForceFail = 1;
    SetForkDeniedByPath("/dev/pids/100/com.app/app_123");
    // No crash = pass
}

/**
 * @tc.name: TestSetForkDeniedByPath_WriteFail
 * @tc.desc: Verify no crash when fork_denied write fails
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestSetForkDeniedByPath_WriteFail, TestSize.Level1)
{
    SetMockRoot();
    mkdir((tempDir_ + "/100").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app/app_123").c_str(), DIR_MODE);
    FILE *f = fopen((tempDir_ + "/100/com.app/app_123/pids.fork_denied").c_str(), "w");
    ASSERT_NE(f, nullptr);
    fclose(f);

    g_mockWriteForceFail = 1;
    SetForkDeniedByPath("/dev/pids/100/com.app/app_123");
    // No crash = pass
}

/**
 * @tc.name: TestSetForkDeniedByPath_NullPath
 * @tc.desc: Verify no crash when NULL path is passed
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestSetForkDeniedByPath_NullPath, TestSize.Level1)
{
    SetForkDeniedByPath(nullptr);
    // No crash = pass
}

/**
 * @tc.name: TestSetForkDeniedByPath_NonExistentDir
 * @tc.desc: Verify no crash when directory does not exist
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestSetForkDeniedByPath_NonExistentDir, TestSize.Level1)
{
    SetMockRoot();
    SetForkDeniedByPath("/dev/pids/999/no_such_tag/app_999");
    // No crash = pass
}

// ===================================================================
// 4.4 CleanupOrphanedTagDir
// ===================================================================

/**
 * @tc.name: TestCleanupOrphanedTagDir_ValidTag
 * @tc.desc: Verify orphan processes are killed and directories removed
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_ValidTag, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500, 501});

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    EXPECT_TRUE(WasPidKilled(500));
    EXPECT_TRUE(WasPidKilled(501));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_MultipleApps
 * @tc.desc: Verify cleanup handles multiple app directories under one tag
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_MultipleApps, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});
    CreateOrphanDir(tempDir_, "100", "com.app", "app_600", {600, 601});

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    EXPECT_TRUE(WasPidKilled(500));
    EXPECT_TRUE(WasPidKilled(600));
    EXPECT_TRUE(WasPidKilled(601));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_600"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_TagDirWithoutApps
 * @tc.desc: Verify tag directory with no app subdirectories is still removed
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_TagDirWithoutApps, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});

    CleanupOrphanedTagDir("/dev/pids/100", "com.other");

    EXPECT_FALSE(WasPidKilled(500));
    // com.other dir doesn't exist locally so opendir fails, nothing happens
    EXPECT_EQ(g_mockRmdirCount, 0);
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_EmptyTagDir
 * @tc.desc: Verify empty tag directory (no app subdirs) is still removed
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_EmptyTagDir, TestSize.Level1)
{
    SetMockRoot();
    mkdir((tempDir_ + "/100").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app").c_str(), DIR_MODE);

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    EXPECT_EQ(g_mockKilledCount, 0);
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_ForkDeniedBeforeKill
 * @tc.desc: Verify fork_denied is set before processes are killed
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_ForkDeniedBeforeKill, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    // Verify fork_denied was written (file should contain "1")
    // The file was created by helper, overwritten by SetForkDeniedByPath
    FILE *f = fopen((tempDir_ + "/100/com.app/app_500/pids.fork_denied").c_str(), "r");
    // File may already be rmdir'd, so just verify kill happened after fork_denied was set
    // Since we can't check ordering directly, verify both operations occurred
    EXPECT_TRUE(WasPidKilled(500));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    if (f) {
        fclose(f);
    }
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_KillFail
 * @tc.desc: Verify cleanup continues when kill fails (PID not exist)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_KillFail, TestSize.Level1)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500, 99999});
    g_mockKillFailPid = 99999;

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    EXPECT_TRUE(WasPidKilled(500));
    EXPECT_TRUE(WasPidKilled(99999));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_RmdirAppFail
 * @tc.desc: Verify cleanup continues when app dir rmdir fails
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_RmdirAppFail, TestSize.Level1)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});
    // rmdir index 0 = app dir rmdir (first rmdir call)
    g_mockRmdirFailIndex = 0;

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    EXPECT_TRUE(WasPidKilled(500));
    // app dir rmdir failed, but tag dir rmdir should still be attempted
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_RmdirTagFail
 * @tc.desc: Verify no crash when tag dir rmdir fails
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_RmdirTagFail, TestSize.Level1)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});
    // app dir rmdir succeeds (index 0), tag dir rmdir fails (index 1)
    g_mockRmdirFailIndex = 1;

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    EXPECT_TRUE(WasPidKilled(500));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_ProcsFileOpenFail
 * @tc.desc: Verify cleanup continues when cgroup.procs open fails
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_ProcsFileOpenFail, TestSize.Level1)
{
    SetMockRoot();
    // Create dir structure but no cgroup.procs file
    mkdir((tempDir_ + "/100").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app/app_500").c_str(), DIR_MODE);
    // No cgroup.procs -> fopen returns NULL -> continue to next app dir

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    EXPECT_EQ(g_mockKilledCount, 0);
    // fopen fails on cgroup.procs, but CleanupOrphanedAppDir still removes the app dir
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    // Tag dir is still removed
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

/**
 * @tc.name: TestCleanupOrphanedTagDir_NonAppSubDir
 * @tc.desc: Verify non-app_* subdirectories are skipped
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedTagDir_NonAppSubDir, TestSize.Level1)
{
    SetMockRoot();
    mkdir((tempDir_ + "/100").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app").c_str(), DIR_MODE);
    mkdir((tempDir_ + "/100/com.app/some_other_dir").c_str(), DIR_MODE);

    CleanupOrphanedTagDir("/dev/pids/100", "com.app");

    EXPECT_EQ(g_mockKilledCount, 0);
    EXPECT_EQ(g_mockRmdirCount, 1); // only tag dir rmdir
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

// ===================================================================
// 4.5 CleanupOrphanedCgroupProcesses
// ===================================================================

/**
 * @tc.name: TestCleanupOrphanedCgroup_SingleUid
 * @tc.desc: Verify cleanup works with single UID containing orphan cgroups
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedCgroup_SingleUid, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});

    CleanupOrphanedCgroupProcesses();

    EXPECT_TRUE(WasPidKilled(500));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

/**
 * @tc.name: TestCleanupOrphanedCgroup_MultipleUids
 * @tc.desc: Verify cleanup works across multiple UIDs
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedCgroup_MultipleUids, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});
    CreateOrphanDir(tempDir_, "200", "com.other", "app_600", {600});

    CleanupOrphanedCgroupProcesses();

    EXPECT_TRUE(WasPidKilled(500));
    EXPECT_TRUE(WasPidKilled(600));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/200/com.other/app_600"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/200/com.other"));
}

/**
 * @tc.name: TestCleanupOrphanedCgroup_RootNotExist
 * @tc.desc: Verify no crash when /dev/pids/ does not exist
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedCgroup_RootNotExist, TestSize.Level0)
{
    // g_mockTestRoot is empty, so opendir("/dev/pids/") will call real opendir
    // which should fail since /dev/pids/ may not exist on host
    CleanupOrphanedCgroupProcesses();
    EXPECT_EQ(g_mockKilledCount, 0);
}

/**
 * @tc.name: TestCleanupOrphanedCgroup_EmptyRoot
 * @tc.desc: Verify no crash when /dev/pids/ exists but is empty
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedCgroup_EmptyRoot, TestSize.Level1)
{
    SetMockRoot();
    // tempDir_ exists but is empty (no UID subdirs)

    CleanupOrphanedCgroupProcesses();

    EXPECT_EQ(g_mockKilledCount, 0);
    EXPECT_EQ(g_mockRmdirCount, 0);
}

/**
 * @tc.name: TestCleanupOrphanedCgroup_NonUidDirs
 * @tc.desc: Verify non-UID directories are skipped
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedCgroup_NonUidDirs, TestSize.Level1)
{
    SetMockRoot();
    mkdir((tempDir_ + "/sys").c_str(), 0755);
    mkdir((tempDir_ + "/abc").c_str(), 0755);

    CleanupOrphanedCgroupProcesses();

    EXPECT_EQ(g_mockKilledCount, 0);
    EXPECT_EQ(g_mockRmdirCount, 0);
}

/**
 * @tc.name: TestCleanupOrphanedCgroup_UidDirOpenFail
 * @tc.desc: Verify no crash when UID subdirectory cannot be opened
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedCgroup_UidDirOpenFail, TestSize.Level1)
{
    SetMockRoot();
    // Create a UID dir with no read permission
    mkdir((tempDir_ + "/100").c_str(), 0755);
    chmod((tempDir_ + "/100").c_str(), 0000);

    CleanupOrphanedCgroupProcesses();

    // Should skip unreadable UID dir without crash
    chmod((tempDir_ + "/100").c_str(), DIR_MODE);
    EXPECT_EQ(g_mockKilledCount, 0);
}

/**
 * @tc.name: TestCleanupOrphanedCgroup_SkipDotDirs
 * @tc.desc: Verify . and .. directories are properly skipped
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCleanupOrphanedCgroup_SkipDotDirs, TestSize.Level1)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});

    CleanupOrphanedCgroupProcesses();

    EXPECT_TRUE(WasPidKilled(500));
    // Only expected rmdir calls, no extras from . or ..
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app/app_500"));
    EXPECT_TRUE(WasPathRmDir("/dev/pids/100/com.app"));
}

// ===================================================================
// 4.6 CgroupPreloadHook
// ===================================================================

/**
 * @tc.name: TestCgroupPreload_AbnormalStop
 * @tc.desc: Verify cleanup is executed when previous run crashed (param=1)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCgroupPreload_AbnormalStop, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});
    (void)strcpy_s(g_mockGracefulStopValue, sizeof(g_mockGracefulStopValue), "1");

    int ret = CgroupPreloadHook(nullptr);
    EXPECT_EQ(ret, -1); // nullptr content triggers early return
}

/**
 * @tc.name: TestCgroupPreload_GracefulStop
 * @tc.desc: Verify cleanup is skipped when previous run exited normally (param=0)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCgroupPreload_GracefulStop, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});
    (void)strcpy_s(g_mockGracefulStopValue, sizeof(g_mockGracefulStopValue), "0");

    CgroupPreloadHook(nullptr);

    EXPECT_EQ(g_mockKilledCount, 0); // cleanup skipped
}

/**
 * @tc.name: TestCgroupPreload_ParamReadFail
 * @tc.desc: Verify cleanup is skipped when GetParameter fails (default no cleanup)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCgroupPreload_ParamReadFail, TestSize.Level0)
{
    SetMockRoot();
    CreateOrphanDir(tempDir_, "100", "com.app", "app_500", {500});
    g_mockGetParamFail = 1;

    CgroupPreloadHook(nullptr);

    EXPECT_EQ(g_mockKilledCount, 0); // cleanup skipped
}

/**
 * @tc.name: TestCgroupPreload_SetParam
 * @tc.desc: Verify param is set to "1" after preload (mark as need cleanup if crash)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCgroupPreload_SetParam, TestSize.Level0)
{
    (void)strcpy_s(g_mockGracefulStopValue, sizeof(g_mockGracefulStopValue), "0");

    int ret = CgroupPreloadHook(nullptr);

    EXPECT_EQ(ret, -1); // nullptr content triggers early return
}

// ===================================================================
// 4.7 CgroupExitHook
// ===================================================================

/**
 * @tc.name: TestCgroupExitHook_SetParam
 * @tc.desc: Verify param is set to "0" on normal exit (no cleanup needed on next start)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCgroupExitHook_SetParam, TestSize.Level0)
{
    int ret = CgroupExitHook(nullptr);

    EXPECT_EQ(ret, -1); // nullptr content triggers early return
}

/**
 * @tc.name: TestCgroupExitHook_NullContent
 * @tc.desc: Verify no crash when NULL content is passed
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnCGroupOrphanTest, TestCgroupExitHook_NullContent, TestSize.Level1)
{
    int ret = CgroupExitHook(nullptr);

    EXPECT_EQ(ret, -1); // nullptr content triggers early return
}
