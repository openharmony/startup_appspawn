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
#include "hnp_base.h"
#include "hnp_pack.h"
#include "hnp_installer.h"
#include "hnp_api.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;

#define HNP_BASE_PATH "/data/app/el1/bundle/10000"

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

void HnpPackWithBin(char *name, bool isPublic, bool isFirst)
{
    char arg6[MAX_FILE_PATH_LEN];

    if (isPublic) {
        EXPECT_EQ(strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/public"), EOK);
    } else {
        EXPECT_EQ(strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/private"), EOK);
    }

    if (isFirst) {
        EXPECT_EQ(mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        EXPECT_EQ(mkdir("hnp_sample/bin", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        FILE *fp = fopen("hnp_sample/bin/out", "wb");
        EXPECT_NE(fp, nullptr);
        (void)fclose(fp);
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
    char arg10[] = "1.1";
    char *argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, name, arg9, arg10};
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
    char* argv[] = {arg1, arg2, arg3, arg4, arg5, package, arg7, arg8, arg9};
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
}

/**
* @tc.name: Hnp_Install_001
* @tc.desc:  Verify set Arg if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_001 start";

    // clear resource before test
    HnpDeleteFolder("hnp_sample");
    HnpDeleteFolder("hnp_out");
    HnpDeleteFolder(HNP_BASE_PATH);

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin((char *)"sample_public", true, true);
    HnpPackWithBin((char *)"sample_private", false, false);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { // param num not enough
        char* argv[] = {arg1, arg2};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_OPERATOR_ARGV_MISS);
    }
    { // param uid is invalid
        char arg3[] = "-u";
        char arg4[] = "asd1231";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_INSTALLER_ARGV_UID_INVALID);
    }
    { // ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-i";
        char arg6[] = "./hnp_out";
        char arg7[] = "-p";
        char arg8[] = "huawei";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();

    GTEST_LOG_(INFO) << "Hnp_Installer_001 end";
}

/**
* @tc.name: Hnp_Install_002
* @tc.desc:  Verify install path get if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_002 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin((char *)"sample_public", true, true);
    HnpPackWithBin((char *)"sample_private", false, false);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { // dir exist but force is false
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_INSTALLER_PATH_IS_EXIST);
    }
    { //ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();

    GTEST_LOG_(INFO) << "Hnp_Installer_002 end";
}

/**
* @tc.name: Hnp_Install_003
* @tc.desc:  Verify scr path bin not exist HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_003 start";

    char arg1[] = "hnp";
    char arg2[] = "install";

    { // scr path bin not exist
        EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        HnpPackWithoutBin((char *)"sample_public", true, true);
        HnpPackWithoutBin((char *)"sample_private", false, false);
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), -1);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out", F_OK), -1);
        HnpPackWithoutBinDelete();
        HnpDeleteFolder(HNP_BASE_PATH);
    }
    { //ok
        EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
        HnpPackWithBin((char *)"sample_public", true, true);
        HnpPackWithBin((char *)"sample_private", false, false);
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out", F_OK), 0);
        HnpPackWithBinDelete();
    }
    HnpDeleteFolder(HNP_BASE_PATH);

    GTEST_LOG_(INFO) << "Hnp_Installer_003 end";
}

/**
* @tc.name: Hnp_Install_004
* @tc.desc:  Verify src path file is not hnp cli generate if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Installer_004 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin((char *)"sample_public", true, true);
    HnpPackWithBin((char *)"sample_private", false, false);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { //src path file is not hnp
        FILE *fp = fopen("./hnp_out/public/example.zip", "wb");
        int data[15] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
        EXPECT_NE(fp, NULL);
        fwrite(data, sizeof(int), 15, fp);
        (void)fclose(fp);
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        remove("./hnp_out/public/example.zip");
    }
    { //ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();

    GTEST_LOG_(INFO) << "Hnp_Installer_004 end";
}

/**
* @tc.name: Hnp_Install_005
* @tc.desc:  Verify more than 2 link if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_005, TestSize.Level0)
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
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};
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
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/outt", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out2", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_Installer_005 end";
}

/**
* @tc.name: Hnp_Install_006
* @tc.desc:  Verify private HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_006, TestSize.Level0)
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
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char arg9[] = "-f";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};
        int argc = sizeof(argv) / sizeof(argv[0]);
        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_Install_006 end";
}

/**
* @tc.name: Hnp_Install_007
* @tc.desc:  Verify set Arg if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_007, TestSize.Level0)
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
    HnpPackWithBin((char *)"sample_public", true, true);
    HnpPackWithBin((char *)"sample_private", false, false);

    char arg1[] = "hnp";
    char arg2[] = "install";

    { // src dir path is invalid
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_in";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_BASE_DIR_OPEN_FAILED);
    }
    { // dst dir path is invalid
        char arg3[] = "-u";
        char arg4[] = "10001";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out/sample.hnp";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED);
    }
    { // ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char arg7[] = "-i";
        char arg8[] = "./hnp_out";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdInstall(argc, argv), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out", F_OK), 0);
        EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out", F_OK), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();

    GTEST_LOG_(INFO) << "Hnp_Install_007 end";
}

/**
* @tc.name: Hnp_Install_API_001
* @tc.desc:  Verify set Arg if NativeInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_API_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_API_001 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);

    if (IsDeveloperModeOpen()) {
        { //param is invalid
            ret = NativeInstallHnp(NULL, "./hnp_out/", "huawei", 1);
            EXPECT_EQ(ret, HNP_API_ERRNO_PARAM_INVALID);
        }
        { //ok
            ret = NativeInstallHnp("10000", "./hnp_out/", "huawei", 1);
            EXPECT_EQ(ret, 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out2", F_OK), 0);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_Install_API_001 end";
}

/**
* @tc.name: Hnp_Install_API_002
* @tc.desc:  Verify set Arg if NativeInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_API_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_API_002 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);

    if (IsDeveloperModeOpen()) {
        { //st dir path is invalid
            ret = NativeInstallHnp("10001", "./hnp_out/", "huawei", 1);
            EXPECT_NE(ret, 0);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_Install_API_002 end";
}

/**
* @tc.name: Hnp_Install_API_003
* @tc.desc:  Verify more than 1 hnp package if NativeInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_API_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_API_003 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);

    if (IsDeveloperModeOpen()) {
        { //ok
            ret = NativeInstallHnp("10000", "./hnp_out/", "huawei", 1);
            EXPECT_EQ(ret, 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out2", F_OK), 0);
        }
        { //ok
            ret = NativeInstallHnp("10000", "./hnp_out/", "baidu", 1);
            EXPECT_EQ(ret, 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/baidu/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/baidu/bin/out2", F_OK), 0);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_Install_API_003 end";
}

/**
* @tc.name: Hnp_Install_API_004
* @tc.desc:  Verify develop mode NativeInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9JCQ1
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_Install_API_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_Install_API_004 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);

    HnpPackWithCfg(true, true);

    { //ok
        ret = NativeInstallHnp("10000", "./hnp_out/", "huawei", 1);
        if (IsDeveloperModeOpen()) {
            GTEST_LOG_(INFO) << "this is developer mode";
            EXPECT_EQ(ret, 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/outt", F_OK), 0);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out2", F_OK), 0);
        } else {
            GTEST_LOG_(INFO) << "this is not developer mode";
            EXPECT_EQ(ret, HNP_API_NOT_IN_DEVELOPER_MODE);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/outt", F_OK), -1);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnppublic/bin/out2", F_OK), -1);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/outt", F_OK), -1);
            EXPECT_EQ(access(HNP_BASE_PATH"/hnp/huawei/bin/out2", F_OK), -1);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_Install_API_004 end";
}

/**
* @tc.name: Hnp_UnInstall_001
* @tc.desc:  Verify set Arg if HnpCmdUnInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_UnInstall_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_001 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin((char *)"sample_public", true, true);
    HnpInstall((char *)"huawei");

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
        char arg6[] = "huawei";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), HNP_ERRNO_INSTALLER_ARGV_UID_INVALID);
    }
    { // ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();

    GTEST_LOG_(INFO) << "Hnp_UnInstall_001 end";
}

/**
* @tc.name: Hnp_UnInstall_002
* @tc.desc:  Verify cfg pack HnpCmdUnInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_UnInstall_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_002 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithCfg(true, true);

    char arg1[] = "hnp";
    char arg2[] = "uninstall";
    { // param uninstall path is invalid
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "wechat";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST);
    }
    { // ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_UnInstall_002 end";
}

/**
* @tc.name: Hnp_UnInstall_003
* @tc.desc:  Verify set Arg if HnpCmdUnInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_UnInstall_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_003 start";

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithBin((char *)"sample_public", true, true);
    HnpInstall((char *)"huawei");

    char arg1[] = "hnp";
    char arg2[] = "uninstall";

    { // param software name is invalid
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "wechat";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST);
    }
    { // ok
        char arg3[] = "-u";
        char arg4[] = "10000";
        char arg5[] = "-p";
        char arg6[] = "huawei";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        EXPECT_EQ(HnpCmdUnInstall(argc, argv), 0);
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithBinDelete();

    GTEST_LOG_(INFO) << "Hnp_UnInstall_003 end";
}

/**
* @tc.name: Hnp_UnInstall_API_001
* @tc.desc:  Verify param invalid API NativeUnInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_UnInstall_API_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_001 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithCfg(true, true);

    if (IsDeveloperModeOpen()) {
        { // param is invalid
            ret = NativeUnInstallHnp(NULL, "huawei");
            EXPECT_EQ(ret, HNP_API_ERRNO_PARAM_INVALID);
        }
        { // ok
            ret = NativeUnInstallHnp("10000", "huawei");
            EXPECT_EQ(ret, 0);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_001 end";
}

/**
* @tc.name: Hnp_UnInstall_API_002
* @tc.desc:  Verify path invalid API NativeUnInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9DQSE
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_UnInstall_API_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_002 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithCfg(true, true);

    if (IsDeveloperModeOpen()) {
        { // param uninstall path is invalid
            ret = NativeUnInstallHnp("10000", "wechat");
            EXPECT_NE(ret, 0);
        }
    }

    HnpDeleteFolder(HNP_BASE_PATH);
    HnpPackWithCfgDelete();

    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_002 end";
}

/**
* @tc.name: Hnp_UnInstall_API_003
* @tc.desc:  Verify develop mode NativeUnInstallHnp succeed.
* @tc.type: FUNC
* @tc.require:issueI9JCQ1
* @tc.author:
*/
HWTEST(HnpInstallerTest, Hnp_UnInstall_API_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_003 start";

    int ret;

    EXPECT_EQ(mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    HnpPackWithCfg(true, true);

    {
        ret = NativeUnInstallHnp("10000", "huawei");
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

    GTEST_LOG_(INFO) << "Hnp_UnInstall_API_003 end";
}

}