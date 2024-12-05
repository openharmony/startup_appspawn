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

#include <gtest/gtest.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>

#include "appspawn_utils.h"
#include "parameter.h"
#include "hnp_base.h"
#include "hnp_pack.h"
#include "hnp_installer.h"
#include "hnp_api.h"
#include "securec.h"
#include "hnp_stub.h"

using namespace testing;
using namespace testing::ext;

#define HNP_BASE_PATH "/data/app/el1/bundle/10000"
#define PARAM_BUFFER_SIZE 10

#ifdef __cplusplus
    extern "C" {
#endif


#ifdef __cplusplus
    }
#endif

namespace OHOS {
class HnpInstallerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HnpInstallerTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "Hnp_Installer_TEST SetUpTestCase";
}

void HnpInstallerTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "Hnp_Installer_TEST TearDownTestCase";
}

void HnpInstallerTest::SetUp()
{
    GTEST_LOG_(INFO) << "Hnp_Installer_TEST SetUp";
}

void HnpInstallerTest::TearDown()
{
    GTEST_LOG_(INFO) << "Hnp_Installer_TEST TearDown";
}

void HnpPackWithoutBin(char *name, bool isPublic, bool isFirst)
{
    char arg6[MAX_FILE_PATH_LEN];

    if (isPublic) {
        EXPECT_EQ(strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/public"), EOK);
    } else {
        EXPECT_EQ(strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/private"), EOK);
    }

    if (isFirst) {
        EXPECT_EQ(mkdir("./hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("./hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_out/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_out/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    }

    char arg1[] = "hnpcli";
    char arg2[] = "pack";
    char arg3[] = "-i";
    char arg4[] = "./hnp_sample";
    char arg5[] = "-o";
    char arg7[] = "-n";
    char arg9[] = "-v";
    char arg10[] = "1.1";
    char *argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, name, arg9, arg10};
    int argc = sizeof(argv) / sizeof(argv[0]);

    EXPECT_EQ(HnpCmdPack(argc, argv), 0);
}

void HnpPackWithoutBinDelete(void)
{
    EXPECT_EQ(HnpDeleteFolder("hnp_sample"), 0);
    EXPECT_EQ(HnpDeleteFolder("hnp_out"), 0);
}

void HnpPackWithBin(char *name, char *version, bool isPublic, bool isFirst, mode_t mode)
{
    char arg6[MAX_FILE_PATH_LEN];

    if (strcmp(version, "1.1") == 0) {
        if (isPublic) {
            EXPECT_EQ(strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/public"), EOK);
        } else {
            EXPECT_EQ(strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/private"), EOK);
        }
    } else {
        if (isPublic) {
            EXPECT_GT(sprintf_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out_%s/public", version), 0);
        } else {
            EXPECT_GT(sprintf_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out_%s/private", version), 0);
        }
    }

    if (isFirst) {
        EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_sample/bin", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        FILE *fp = fopen("hnp_sample/bin/out", "wb");
        EXPECT_NE(fp, nullptr);
        (void)fclose(fp);
        EXPECT_EQ(chmod("hnp_sample/bin/out", mode), 0);
        EXPECT_EQ(mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_out/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_out/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    }

    char arg1[] = "hnpcli";
    char arg2[] = "pack";
    char arg3[] = "-i";
    char arg4[] = "./hnp_sample";
    char arg5[] = "-o";
    char arg7[] = "-n";
    char arg9[] = "-v";
    char *argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, name, arg9, version};
    int argc = sizeof(argv) / sizeof(argv[0]);

    EXPECT_EQ(HnpCmdPack(argc, argv), 0);
}

void HnpPackWithBinDelete(void)
{
    EXPECT_EQ(HnpDeleteFolder("hnp_sample"), 0);
    EXPECT_EQ(HnpDeleteFolder("hnp_out"), 0);
}

void HnpPackWithCfg(bool isPublic, bool isFirst)
{
    FILE *fp;
    int whitelen;
    char arg6[MAX_FILE_PATH_LEN];

    if (isPublic) {
        EXPECT_EQ(strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/public"), EOK);
    } else {
        EXPECT_EQ(strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/private"), EOK);
    }

    if (isFirst) {
        EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_sample/bin", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_out/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_out/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        fp = fopen("hnp_sample/bin/out", "wb");
        EXPECT_NE(fp, NULL);
        (void)fclose(fp);
        fp = fopen("hnp_sample/bin/out2", "wb");
        EXPECT_NE(fp, NULL);
        (void)fclose(fp);
        fp = fopen("hnp_sample/hnp.json", "w");
        EXPECT_NE(fp, nullptr);
        (void)fclose(fp);
    }

    char arg1[] = "hnp";
    char arg2[] = "pack";
    char arg3[] = "-i";
    char arg4[] = "./hnp_sample";
    char arg5[] = "-o";
    char *argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
    int argc = sizeof(argv) / sizeof(argv[0]);
    char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":"
        "{\"links\":[{\"source\":\"bin/out\",\"target\":\"outt\"},{\"source\":\"bin/out2\","
        "\"target\":\"out2\"}]}}";
    fp = fopen("hnp_sample/hnp.json", "w");
    whitelen = fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp);
    (void)fclose(fp);
    EXPECT_EQ(whitelen, strlen(cfg) + 1);

    EXPECT_EQ(HnpCmdPack(argc, argv), 0);
}

void HnpPackWithCfgDelete(void)
{
    EXPECT_EQ(HnpDeleteFolder("hnp_sample"), 0);
    EXPECT_EQ(HnpDeleteFolder("hnp_out"), 0);
}

void HnpInstall(char *package)
{
    char arg1[] = "hnp";
    char arg2[] = "install";
    char arg3[] = "-u";
    char arg4[] = "10000";
    char arg5[] = "-p";
    char arg7[] = "-f";
    char arg8[] = "-i";
    char arg9[] = "./hnp_out";
    char arg10[] = "-s";
    char arg11[] = "./hnp_out";
    char arg12[] = "-a";
    char arg13[] = "system64";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, package, arg7, arg8, arg9, arg10, arg11, arg12, arg13};
    int argc = sizeof(argv) / sizeof(argv[0]);

    EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
}

void HnpUnInstall(char *package)
{
    char arg1[] = "hnp";
    char arg2[] = "uninstall";
    char arg3[] = "-u";
    char arg4[] = "10000";
    char arg5[] = "-p";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, package};
    int argc = sizeof(argv) / sizeof(argv[0]);

    EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);
}

/**
* @tc.name: Hnp_Install_001
* @tc.desc:  Verify set Arg if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_001 start";

    // clear resource before test
    HnpDeleteFolder("hnp_sample");
    HnpDeleteFolder("hnp_out");
    HnpDeleteFolder(HNP_BASE_PATH);
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1.1"), true, true,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpPackWithBin(const_cast<char *>("sample_private"), const_cast<char *>("1.1"), false, false,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);

    char arg1[] = "hnp";
    char arg2[] = "install";
    char arg3[] = "-u";

    { // param num not enough
        char* argv[] = {arg1, arg2};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_OPERATOR_ARGV_MISS);
    }
    { // param uid is invalid
        char arg4[] = "asd1231";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-s";
        char arg8[] = "./hnp_out";
        char arg9[] = "-a";
        char arg10[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_INSTALLER_ARGV_UID_INVALID);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Installer_001 end";
}

/**
* @tc.name: Hnp_Install_002
* @tc.desc:  Verify install path get if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_002 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1.1"), true, true,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpPackWithBin(const_cast<char *>("sample_private"), const_cast<char *>("1.1"), false, false,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { // dir exist but force is false
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-s";
        char arg10[] = "./hnp_out";
        char arg11[] = "-a";
        char arg12[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_INSTALLER_PATH_IS_EXIST);
    }
    { //ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char arg10[] = "-s";
        char arg11[] = "./hnp_out";
        char arg12[] = "-a";
        char arg13[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/sample/bin/out", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Installer_002 end";
}

/**
* @tc.name: Hnp_Install_003
* @tc.desc:  Verify scr path bin not exist HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_003 start";

    char arg1[] = "hnp";
    char arg2[] = "install";
    char arg3[] = "-u";
    char arg4[] = "10000";
    char arg5[] = "-p";
    char arg6[] = "sample";
    char arg7[] = "-i";
    char arg8[] = "./hnp_out";
    char arg9[] = "-s";
    char arg10[] = "./hnp_out";
    char arg11[] = "-a";
    char arg12[] = "system64";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12};
    int argc = sizeof(argv) / sizeof(argv[0]);

    { // scr path bin not exist
        EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        HnpPackWithoutBin(const_cast<char *>("sample_public"), true, true);
        HnpPackWithoutBin(const_cast<char *>("sample_private"), false, false);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), -1);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/sample/bin/out", F_OK), -1);
        HnpPackWithoutBinDelete();
        HnpDeleteFolder(HNP_BASE_PATH);
        remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);
    }
    { //ok
        EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1.1"), true, true,
            S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
        HnpPackWithBin(const_cast<char *>("sample_private"), const_cast<char *>("1.1"), false, false,
            S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/sample/bin/out", F_OK), 0);
        HnpPackWithBinDelete();
    }
    HnpDeleteFolder(HNP_BASE_PATH);
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Installer_003 end";
}

/**
* @tc.name: Hnp_Install_004
* @tc.desc:  Verify src path file is not hnp cli generate if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_004 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1.1"), true, true,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpPackWithBin(const_cast<char *>("sample_private"), const_cast<char *>("1.1"), false, false,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);

    char arg1[] = "hnp";
    char arg2[] = "install";
    char arg3[] = "-u";
    char arg4[] = "10000";
    char arg5[] = "-p";
    char arg6[] = "sample";
    char arg7[] = "-i";
    char arg8[] = "./hnp_out";
    char arg9[] = "-f";
    char arg10[] = "-s";
    char arg11[] = "./hnp_out";
    char arg12[] = "-a";
    char arg13[] = "system64";

    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13};
    int argc = sizeof(argv) / sizeof(argv[0]);

    { //src path file is not hnp
        FILE *fp = fopen("./hnp_out/public/example.zip", "wb");
        int data[15] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        EXPECT_NE(fp, NULL);
        fwrite(data, sizeof(int), 15, fp);
        (void)fclose(fp);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        remove("./hnp_out/public/example.zip");
    }
    { //ok
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/sample/bin/out", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Installer_004 end";
}

/**
* @tc.name: Hnp_Install_005
* @tc.desc:  Verify more than 2 link if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_005 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { //public ok
        HnpPackWithCfg(true, true);
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char arg10[] = "-s";
        char arg11[] = "./hnp_out";
        char arg12[] = "-a";
        char arg13[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
    }
    { //ok
        HnpPackWithCfg(false, false);
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char arg10[] = "-s";
        char arg11[] = "./hnp_out";
        char arg12[] = "-a";
        char arg13[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/sample/bin/outt", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/sample/bin/out2", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Installer_005 end";
}

/**
* @tc.name: Hnp_Install_006
* @tc.desc:  Verify private HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_006 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { //ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char arg10[] = "-s";
        char arg11[] = "./hnp_out";
        char arg12[] = "-a";
        char arg13[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,  arg10, arg11, arg12, arg13};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_006 end";
}

/**
* @tc.name: Hnp_Install_007
* @tc.desc:  Verify set Arg if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_007 start";

    // clear resource before test
    remove("hnp_out/sample.hnp");
    remove("hnp_sample/bin/out");
    rmdir("hnp_sample/bin");
    rmdir("hnp_sample");
    rmdir("hnp_out");
    HnpDeleteFolder(HNP_BASE_PATH);

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1.1"), true, true,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpPackWithBin(const_cast<char *>("sample_private"), const_cast<char *>("1.1"), false, false,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { // src dir path is invalid
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_in";
        char arg9[] = "-s";
        char arg10[] = "./hnp_out";
        char arg11[] = "-a";
        char arg12[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_BASE_PARAMS_INVALID);
    }
    { // dst dir path is invalid
        char arg3[] = "-u";
        char arg4[] = "10001";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out/sample.hnp";
        char arg9[] = "-s";
        char arg10[] = "./hnp_out";
        char arg11[] = "-a";
        char arg12[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_007 end";
}

/**
* @tc.name: Hnp_Install_008
* @tc.desc:  Verify file permission.
* @tc.type: FUNC
* @tc.require:issueI9RYCK
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_008, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_08 start";

    // clear resource before test
    remove("hnp_out/sample.hnp");
    remove("hnp_sample/bin/out");
    rmdir("hnp_sample/bin");
    rmdir("hnp_sample");
    rmdir("hnp_out");
    HnpDeleteFolder(HNP_BASE_PATH);

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1.1"), true, true,
        S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { //ok
        struct stat st = {0};

        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char arg10[] = "-s";
        char arg11[] = "./hnp_out";
        char arg12[] = "-a";
        char arg13[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,  arg10, arg11, arg12, arg13};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(stat(HNP_BASE_PATH"/hnppublic/bin/out", &st), 0);
        EXPECT_EQ(st.st_mode & 0777, 0744);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_008 end";
}

/**
* @tc.name: Hnp_Install_009
* @tc.desc:  Verify file permission.
* @tc.type: FUNC
* @tc.require:issueI9RYCK
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_009, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_009 start";

    // clear resource before test
    remove("hnp_out/sample.hnp");
    remove("hnp_sample/bin/out");
    rmdir("hnp_sample/bin");
    rmdir("hnp_sample");
    rmdir("hnp_out");
    HnpDeleteFolder(HNP_BASE_PATH);

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1.1"), true, true,
        S_IXUSR | S_IXGRP | S_IXOTH);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { //ok
        struct stat st = {0};

        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char arg10[] = "-s";
        char arg11[] = "./hnp_out";
        char arg12[] = "-a";
        char arg13[] = "system64";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,  arg10, arg11, arg12, arg13};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(stat(HNP_BASE_PATH"/hnppublic/bin/out", &st), 0);
        EXPECT_EQ(st.st_mode & 0777, 0755);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_009 end";
}

static void HnpVersionPathCreate(void)
{
    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_1", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_1/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_1/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_2", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_2/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_2/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_3", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_3/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_3/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_4", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_4/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir("hnp_out_4/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1"), true, true,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("2"), true, false,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("3"), true, false,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("4"), true, false,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
}

static void HnpVersionPathDelete(void)
{
    EXPECT_EQ(HnpDeleteFolder("hnp_out_1"), 0);
    EXPECT_EQ(HnpDeleteFolder("hnp_out_2"), 0);
    EXPECT_EQ(HnpDeleteFolder("hnp_out_3"), 0);
    EXPECT_EQ(HnpDeleteFolder("hnp_out_4"), 0);
}

static void HnpVersionV1Install()
{
    char arg1[] = "hnp";
    char arg2[] = "install";
    char arg3[] = "-u";
    char arg4[] = "10000";
    char arg5[] = "-p";
    char arg7[] = "-i";
    char arg9[] = "-f";
    char arg10[] = "-s";
    char arg11[] = "./hnp_out_1";
    char arg12[] = "-a";
    char arg13[] = "system64";

    char arg6[] = "sample1";
    char arg8[] = "./hnp_out_1";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,  arg10, arg11, arg12, arg13};
    int argc = sizeof(argv) / sizeof(argv[0]);
    // install v1
    EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
}

static void HnpVersionV2Install()
{
    char arg1[] = "hnp";
    char arg2[] = "install";
    char arg3[] = "-u";
    char arg4[] = "10000";
    char arg5[] = "-p";
    char arg7[] = "-i";
    char arg9[] = "-f";
    char arg10[] = "-s";
    char arg11[] = "./hnp_out_2";
    char arg12[] = "-a";
    char arg13[] = "system64";

    char arg6[] = "sample2";
    char arg8[] = "./hnp_out_2";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,  arg10, arg11, arg12, arg13};
    int argc = sizeof(argv) / sizeof(argv[0]);
    // install v1
    EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
}

static void HnpVersionV3Install()
{
    char arg1[] = "hnp";
    char arg2[] = "install";
    char arg3[] = "-u";
    char arg4[] = "10000";
    char arg5[] = "-p";
    char arg7[] = "-i";
    char arg9[] = "-f";
    char arg10[] = "-s";
    char arg11[] = "./hnp_out_3";
    char arg12[] = "-a";
    char arg13[] = "system64";

    char arg6[] = "sample3";
    char arg8[] = "./hnp_out_3";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,  arg10, arg11, arg12, arg13};
    int argc = sizeof(argv) / sizeof(argv[0]);
    // install v1
    EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
}

static void HnpVersionV4Install()
{
    char arg1[] = "hnp";
    char arg2[] = "install";
    char arg3[] = "-u";
    char arg4[] = "10000";
    char arg5[] = "-p";
    char arg7[] = "-i";
    char arg9[] = "-f";
    char arg10[] = "-s";
    char arg11[] = "./hnp_out_4";
    char arg12[] = "-a";
    char arg13[] = "system64";

    char arg6[] = "sample4";
    char arg8[] = "./hnp_out_4";
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,  arg10, arg11, arg12, arg13};
    int argc = sizeof(argv) / sizeof(argv[0]);
    // install v1
    EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
}

static void HnpVersionV1Uninstall()
{
    char uarg1[] = "hnp";
    char uarg2[] = "uninstall";
    char uarg3[] = "-u";
    char uarg4[] = "10000";
    char uarg5[] = "-p";

    // uninstall v1
    char uarg6[] = "sample1";
    char* uargv[] = {uarg1, uarg2, uarg3, uarg4, uarg5, uarg6};
    int uargc = sizeof(uargv) / sizeof(uargv[0]);
    EXPECT_EQ(HnpCmdUnInstall(uargc, uargv), 0);
}

static void HnpVersionV2Uninstall()
{
    char uarg1[] = "hnp";
    char uarg2[] = "uninstall";
    char uarg3[] = "-u";
    char uarg4[] = "10000";
    char uarg5[] = "-p";

    // uninstall v2
    char uarg6[] = "sample2";
    char* uargv[] = {uarg1, uarg2, uarg3, uarg4, uarg5, uarg6};
    int uargc = sizeof(uargv) / sizeof(uargv[0]);
    EXPECT_EQ(HnpCmdUnInstall(uargc, uargv), 0);
}

static void HnpVersionV3Uninstall()
{
    char uarg1[] = "hnp";
    char uarg2[] = "uninstall";
    char uarg3[] = "-u";
    char uarg4[] = "10000";
    char uarg5[] = "-p";

    // uninstall v3
    char uarg6[] = "sample3";
    char* uargv[] = {uarg1, uarg2, uarg3, uarg4, uarg5, uarg6};
    int uargc = sizeof(uargv) / sizeof(uargv[0]);
    EXPECT_EQ(HnpCmdUnInstall(uargc, uargv), 0);
}

static void HnpVersionV4Uninstall()
{
    char uarg1[] = "hnp";
    char uarg2[] = "uninstall";
    char uarg3[] = "-u";
    char uarg4[] = "10000";
    char uarg5[] = "-p";

    // uninstall v4
    char uarg6[] = "sample4";
    char* uargv[] = {uarg1, uarg2, uarg3, uarg4, uarg5, uarg6};
    int uargc = sizeof(uargv) / sizeof(uargv[0]);
    EXPECT_EQ(HnpCmdUnInstall(uargc, uargv), 0);
}

static bool HnpSymlinkCheck(const char *sourcePath, const char *targetPath)
{
    int ret = false;
    struct stat stat;
    char link[MAX_FILE_PATH_LEN];

    if (lstat(sourcePath, &stat) < 0) {
        return ret;
    }

    if (!S_ISLNK(stat.st_mode)) {
        return ret;
    }

    int len = readlink(sourcePath, link, sizeof(link) - 1);
    if (len < 0) {
        return ret;
    }

    link[len] = '\0';

    GTEST_LOG_(INFO) << "sourcelink is" << link << ";targetlink is" << targetPath;
    return strcmp(link, targetPath) == 0 ? true : false;
}

/**
* @tc.name: Hnp_Install_010
* @tc.desc:  Verify version rule HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueIAGPEW
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_010, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_010 start";

    HnpVersionPathCreate();

    // install v1 v2 v3
    HnpVersionV1Install();
    HnpVersionV2Install();
    HnpVersionV3Install();
    // check v1 v2 v3 exist
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_1", F_OK), 0);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_2", F_OK), 0);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_3", F_OK), 0);
    EXPECT_EQ(HnpSymlinkCheck(HNP_BASE_PATH"/hnppublic/bin/out", "../sample_public.org/sample_public_3/bin/out"), true);

    // uninstall v3
    HnpVersionV3Uninstall();
    // check v1 v2 v3 exist
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_1", F_OK), 0);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_2", F_OK), 0);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_3", F_OK), 0);
    EXPECT_EQ(HnpSymlinkCheck(HNP_BASE_PATH"/hnppublic/bin/out", "../sample_public.org/sample_public_3/bin/out"), true);

    HnpVersionV4Install();
    // check v1 v2 v4 exist v3 is uninstall
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_1", F_OK), 0);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_2", F_OK), 0);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_3", F_OK), -1);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_4", F_OK), 0);
    EXPECT_EQ(HnpSymlinkCheck(HNP_BASE_PATH"/hnppublic/bin/out", "../sample_public.org/sample_public_4/bin/out"), true);

    // uninstall v1
    HnpVersionV1Uninstall();
    // check v2 v4 exist v1 v3 is uninstall
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_1", F_OK), -1);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_2", F_OK), 0);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_3", F_OK), -1);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_4", F_OK), 0);
    EXPECT_EQ(HnpSymlinkCheck(HNP_BASE_PATH"/hnppublic/bin/out", "../sample_public.org/sample_public_4/bin/out"), true);

    // uninstall v4
    HnpVersionV4Uninstall();
    // check v2 v4 exist v1 v3 is uninstall
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_1", F_OK), -1);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_2", F_OK), 0);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_3", F_OK), -1);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_4", F_OK), 0);
    EXPECT_EQ(HnpSymlinkCheck(HNP_BASE_PATH"/hnppublic/bin/out", "../sample_public.org/sample_public_4/bin/out"), true);

    // uninstall v2
    HnpVersionV2Uninstall();
    // all version is uninstall
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_1", F_OK), -1);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_2", F_OK), -1);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_3", F_OK), -1);
    EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/sample_public.org/sample_public_4", F_OK), -1);

    HnpVersionPathDelete();
    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_010 end";
}

static bool IsHnpInstallEnable()
{
    char buffer[PARAM_BUFFER_SIZE] = {0};
    int ret = GetParameter("const.startup.hnp.install.enable", "false", buffer, PARAM_BUFFER_SIZE);
    if (ret <= 0) {
        return false;
    }

    if (strcmp(buffer, "true") == 0) {
        return true;
    }

    return false;
}

/**
* @tc.name: Hnp_Install_API_001
* @tc.desc:  Verify set Arg if NativeInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_API_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_API_001 start";

    int ret;
    bool installEnable = IsHnpInstallEnable();

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);
    struct HapInfo hapInfo;
    (void)memset_s(&hapInfo, sizeof(HapInfo), 0, sizeof(HapInfo));
    EXPECT_EQ(sprintf_s(hapInfo.packageName, sizeof(hapInfo.packageName), "%s", "sample") > 0, true);
    EXPECT_EQ(sprintf_s(hapInfo.abi, sizeof(hapInfo.abi), "%s", "system64") > 0, true);
    EXPECT_EQ(sprintf_s(hapInfo.hapPath, sizeof(hapInfo.hapPath), "%s", "./hnp_out") > 0, true);

    if (!installEnable) {
        GTEST_LOG_(INFO) << "hnp install enable false";
    }

    if (IsDeveloperModeOpen() && installEnable) {
        { //param is invalid
            ret = NativeInstallHnp(NULL, "./hnp_out", &hapInfo, 1);
            EXPECT_EQ(ret, HNP_API_ERRNO_PARAM_INVALID);
        }
        { //ok
            EXPECT_EQ(sprintf_s(hapInfo.hapPath, sizeof(hapInfo.hapPath), "%s", "test") > 0, true);
            ret = NativeInstallHnp("10000", "./hnp_out", &hapInfo, 1);
            EXPECT_EQ(ret, 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_API_001 end";
}

/**
* @tc.name: Hnp_Install_API_002
* @tc.desc:  Verify set Arg if NativeInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_API_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_API_002 start";

    int ret;
    bool installEnable = IsHnpInstallEnable();

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);

    if (!installEnable) {
        GTEST_LOG_(INFO) << "hnp install enable false";
    }

    if (IsDeveloperModeOpen() && installEnable) {
        { //st dir path is invalid
            struct HapInfo hapInfo;
            (void)memset_s(&hapInfo, sizeof(HapInfo), 0, sizeof(HapInfo));
            EXPECT_EQ(sprintf_s(hapInfo.packageName, sizeof(hapInfo.packageName), "%s", "sample") > 0, true);
            EXPECT_EQ(sprintf_s(hapInfo.abi, sizeof(hapInfo.abi), "%s", "system64") > 0, true);
            EXPECT_EQ(sprintf_s(hapInfo.hapPath, sizeof(hapInfo.hapPath), "%s", "test") > 0, true);
            ret = NativeInstallHnp("10001", "./hnp_out/", &hapInfo, 1);
            EXPECT_EQ(ret, HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_API_002 end";
}

/**
* @tc.name: Hnp_Install_API_003
* @tc.desc:  Verify more than 1 hnp package if NativeInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_API_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_API_003 start";

    int ret;
    bool installEnable = IsHnpInstallEnable();

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);

    if (IsDeveloperModeOpen()) {
        { //ok
            struct HapInfo hapInfo;
            (void)memset_s(&hapInfo, sizeof(HapInfo), 0, sizeof(HapInfo));
            EXPECT_EQ(sprintf_s(hapInfo.packageName, sizeof(hapInfo.packageName), "%s", "sample") > 0, true);
            EXPECT_EQ(sprintf_s(hapInfo.hapPath, sizeof(hapInfo.hapPath), "%s", "test") > 0, true);
            EXPECT_EQ(sprintf_s(hapInfo.abi, sizeof(hapInfo.abi), "%s", "system64") > 0, true);
            ret = NativeInstallHnp("10000", "./hnp_out/", &hapInfo, 1);
            if (installEnable) {
                EXPECT_EQ(ret, 0);
                EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
                EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
            } else {
                GTEST_LOG_(INFO) << "hnp install enable false";
                EXPECT_EQ(ret, HNP_API_ERRNO_HNP_INSTALL_DISABLED);
            }
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_API_003 end";
}

/**
* @tc.name: Hnp_Install_API_004
* @tc.desc:  Verify develop mode NativeInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9JCQ1
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_Install_API_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_API_004 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);

    if (!IsHnpInstallEnable()) {
        GTEST_LOG_(INFO) << "hnp install enable false";
    } else {
        { //ok
            struct HapInfo hapInfo;
            (void)memset_s(&hapInfo, sizeof(HapInfo), 0, sizeof(HapInfo));
            EXPECT_EQ(sprintf_s(hapInfo.packageName, sizeof(hapInfo.packageName), "%s", "sample") > 0, true);
            EXPECT_EQ(sprintf_s(hapInfo.hapPath, sizeof(hapInfo.hapPath), "%s", "test") > 0, true);
            EXPECT_EQ(sprintf_s(hapInfo.abi, sizeof(hapInfo.abi), "%s", "system64") > 0, true);
            ret = NativeInstallHnp("10000", "./hnp_out/", &hapInfo, 1);
            if (IsDeveloperModeOpen()) {
                GTEST_LOG_(INFO) << "this is developer mode";
                EXPECT_EQ(ret, 0);
                EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
                EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
            } else {
                GTEST_LOG_(INFO) << "this is not developer mode";
                EXPECT_EQ(ret, HNP_API_NOT_IN_DEVELOPER_MODE);
                EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), -1);
                EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), -1);
            }
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Install_API_004 end";
}

/**
* @tc.name: Hnp_RelPath_API_001
* @tc.desc:  Verify HnpRelPath succeed.
* @tc.type: FUNC
* @tc.require:issueIANH44
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_RelPath_API_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_RelPath_API_001 start";

    const char *fromPath = "test";
    const char *toPath = "test2";
    char relPath[MAX_FILE_PATH_LEN]{};

    HnpRelPath(fromPath, toPath, relPath);
    EXPECT_EQ(strcmp(relPath, "test2"), 0);

    const char *fromPath2 = "/aaa/bbb/ccc/ddd";
    const char *toPath2 = "/aaa/bbb/ccc/eeefff";
    char relPath2[MAX_FILE_PATH_LEN]{};

    HnpRelPath(fromPath2, toPath2, relPath2);
    EXPECT_EQ(strcmp(relPath2, "eeefff"), 0);

    const char *fromPath3 = "/aaa/bbb/bin/bbb/aaa";
    const char *toPath3 = "/aaa/bbb/bisheng/aaa";
    char relPath3[MAX_FILE_PATH_LEN]{};

    HnpRelPath(fromPath3, toPath3, relPath3);
    EXPECT_EQ(strcmp(relPath3, "../../bisheng/aaa"), 0);

    const char *fromPath4 = "/aaa/bbb/cccddd/aaa/bbb";
    const char *toPath4 = "/aaa/bbb/ccc/eeefff";
    char relPath4[MAX_FILE_PATH_LEN]{};

    HnpRelPath(fromPath4, toPath4, relPath4);
    EXPECT_EQ(strcmp(relPath4, "../../ccc/eeefff"), 0);
}

/**
* @tc.name: Hnp_UnInstall_001
* @tc.desc:  Verify set Arg if HnpCmdUnInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_UnInstall_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_001 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin(const_cast<char *>("sample"), const_cast<char *>("1.1"), true, true,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpInstall(const_cast<char *>("sample"));

    char arg1[] = "hnp";
    char arg2[] = "uninstall";

    { // param num not enough
        char* argv[] = {arg1, arg2};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), HNP_ERRNO_OPERATOR_ARGV_MISS);
    }
    { // param uid is invalid
        char arg3[] = "-u";
        char arg4[] = "asd1231";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), HNP_ERRNO_INSTALLER_ARGV_UID_INVALID);
    }
    { // ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_UnInstall_001 end";
}

/**
* @tc.name: Hnp_UnInstall_002
* @tc.desc:  Verify cfg pack HnpCmdUnInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_UnInstall_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_002 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithCfg(true, true);
    HnpInstall(const_cast<char *>("sample"));

    char arg1[] = "hnp";
    char arg2[] = "uninstall";
    { // param uninstall path is invalid
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "wechat";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    }
    { // ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_UnInstall_002 end";
}

/**
* @tc.name: Hnp_UnInstall_003
* @tc.desc:  Verify set Arg if HnpCmdUnInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_UnInstall_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_003 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin(const_cast<char *>("sample_public"), const_cast<char *>("1.1"), true, true,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpPackWithBin(const_cast<char *>("sample_private"), const_cast<char *>("1.1"), false, false,
        S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    HnpInstall(const_cast<char *>("sample"));

    char arg1[] = "hnp";
    char arg2[] = "uninstall";

    { // param software name is invalid
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "wechat";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    }
    { // ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "sample";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_UnInstall_003 end";
}

/**
* @tc.name: Hnp_UnInstall_API_001
* @tc.desc:  Verify param invalid API NativeUnInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_UnInstall_API_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_001 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithCfg(true, true);
    HnpInstall(const_cast<char *>("sample"));

    if (IsDeveloperModeOpen()) {
        { // param is invalid
            ret = NativeUnInstallHnp(NULL, "sample");
            EXPECT_EQ(ret, HNP_API_ERRNO_PARAM_INVALID);
        }
        { // ok
            ret = NativeUnInstallHnp("10000", "sample");
            EXPECT_EQ(ret, 0);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_001 end";
}

/**
* @tc.name: Hnp_UnInstall_API_002
* @tc.desc:  Verify path invalid API NativeUnInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_UnInstall_API_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_002 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithCfg(true, true);
    HnpInstall(const_cast<char *>("sample"));

    if (IsDeveloperModeOpen()) {
        { // param uninstall path is invalid
            ret = NativeUnInstallHnp("10001", "wechat");
            EXPECT_EQ(ret, HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_002 end";
}

/**
* @tc.name: Hnp_UnInstall_API_003
* @tc.desc:  Verify develop mode NativeUnInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9JCQ1
* @tc.author:
*/
HWTEST_F(HnpInstallerTest, Hnp_UnInstall_API_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_003 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithCfg(true, true);
    HnpInstall(const_cast<char *>("sample"));

    {
        ret = NativeUnInstallHnp("10000", "sample");
        if (IsDeveloperModeOpen()) {
            GTEST_LOG_(INFO) << "this is developer mode";
            EXPECT_EQ(ret, 0);
        } else {
            GTEST_LOG_(INFO) << "this is not developer mode";
            EXPECT_EQ(ret, HNP_API_NOT_IN_DEVELOPER_MODE);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();
    remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);

    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_003 end";
}

}