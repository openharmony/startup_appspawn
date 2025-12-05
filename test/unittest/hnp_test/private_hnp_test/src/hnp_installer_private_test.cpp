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

#include "hnp_installer_private_test.h"
#include "hnp_test_api.h"
#include "hnp_test_installer.h"
#include "hnp_test_zip.h"

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
#include "lib_wrapper.h"
#include "elf.h"
using namespace testing;
using namespace testing::ext;


#ifdef __cplusplus
    extern "C" {
#endif

static void CreateTestFileWrite32(const char *fileName)
{
    FILE *tmpFile = fopen(fileName, "wb");
    if (tmpFile != nullptr) {
        Elf32_Ehdr ehdr = {};
        ehdr.e_ident[EI_MAG0] = '\177';
        ehdr.e_ident[EI_MAG1] = 'E';
        ehdr.e_ident[EI_MAG2] = 'L';
        ehdr.e_ident[EI_MAG3] = 'F';
        ehdr.e_ident[EI_CLASS] = ELFCLASS32;
        fwrite(&ehdr, sizeof(Elf32_Ehdr), 1, tmpFile);
        (void)fflush(tmpFile);
        fclose(tmpFile);
    }
}

static void CreateTestFileWrite64(const char *fileName)
{
    FILE *tmpFile = fopen(fileName, "wb");
    if (tmpFile != nullptr) {
        Elf64_Ehdr ehdr = {};
        ehdr.e_ident[EI_MAG0] = '\177';
        ehdr.e_ident[EI_MAG1] = 'E';
        ehdr.e_ident[EI_MAG2] = 'L';
        ehdr.e_ident[EI_MAG3] = 'F';
        ehdr.e_ident[EI_CLASS] = ELFCLASS64;
        fwrite(&ehdr, sizeof(Elf64_Ehdr), 1, tmpFile);
        (void)fflush(tmpFile);
        fclose(tmpFile);
    }
}

static void CreateTestFileWriteOther(const char *fileName)
{
    FILE *tmpFile = fopen(fileName, "wb");
    if (tmpFile != nullptr) {
        Elf64_Ehdr ehdr = {};
        ehdr.e_ident[EI_MAG0] = '\177';
        ehdr.e_ident[EI_MAG1] = 'E';
        ehdr.e_ident[EI_MAG2] = 'L';
        ehdr.e_ident[EI_MAG3] = 'F';
        ehdr.e_ident[EI_CLASS] = ELFCLASSNONE;
        fwrite(&ehdr, sizeof(Elf64_Ehdr), 1, tmpFile);
        (void)fflush(tmpFile);
        fclose(tmpFile);
    }
}

#ifdef __cplusplus
    }
#endif

namespace OHOS {
class HnpPrivatTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HnpPrivatTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "HnpPrivatTest SetUpTestCase";
}

void HnpPrivatTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "HnpPrivatTest TearDownTestCase";
}

void HnpPrivatTest::SetUp()
{
    GTEST_LOG_(INFO) << "HnpPrivatTest SetUp";
}

void HnpPrivatTest::TearDown()
{
    GTEST_LOG_(INFO) << "HnpPrivatTest TearDown";
}

HWTEST_F(HnpPrivatTest, HnpGetRet_001, TestSize.Level0)
{
    ReadFunc readFunc = [](int fd, void *buf, size_t count) -> ssize_t {
        return 0;
    };
    UpdateReadFunc(readFunc);
    int ret = 0;
    EXPECT_NE(HnpCmdApiReturnGet(0, &ret), 0);
    UpdateReadFunc(NULL);
}

HWTEST_F(HnpPrivatTest, HnpELFFileCheckTest_001, TestSize.Level0)
{
    char testFile[PATH_MAX] = {0};
    size_t bytes = snprintf_s(testFile, PATH_MAX, PATH_MAX - 1, APPSPAWN_BASE_DIR "/hnptestelf32");
    EXPECT_NE(bytes, 0);

    CreateTestFileWrite32(testFile);

    HnpSignMapInfo temp = {};
    bool ret = HnpELFFileCheck(testFile, &temp);
    EXPECT_TRUE(ret);
    remove(testFile);
}

HWTEST_F(HnpPrivatTest, HnpELFFileCheckTest_002, TestSize.Level0)
{
    char testFile[PATH_MAX] = {0};
    size_t bytes = snprintf_s(testFile, PATH_MAX, PATH_MAX - 1, APPSPAWN_BASE_DIR "/hnptestelf64");
    EXPECT_NE(bytes, 0);

    CreateTestFileWrite64(testFile);

    HnpSignMapInfo temp = {};
    bool ret = HnpELFFileCheck(testFile, &temp);
    EXPECT_TRUE(ret);
    remove(testFile);
}

HWTEST_F(HnpPrivatTest, HnpELFFileCheckTest_003, TestSize.Level0)
{
    char testFile[PATH_MAX] = {0};
    size_t bytes = snprintf_s(testFile, PATH_MAX, PATH_MAX - 1, APPSPAWN_BASE_DIR "/hnptestelfother");
    EXPECT_NE(bytes, 0);

    CreateTestFileWriteOther(testFile);

    HnpSignMapInfo temp = {};
    bool ret = HnpELFFileCheck(testFile, &temp);
    EXPECT_TRUE(ret);
    remove(testFile);
}

HWTEST_F(HnpPrivatTest, HapInstallInfoDestory_001, TestSize.Level0)
{
    HapInstallInfo *installInfo = (HapInstallInfo *)calloc(1, sizeof(HapInstallInfo));
    installInfo->signHnpSize = 1;
    char** hnps = (char**)calloc(1, sizeof(char*));
    hnps[0] = strdup("test");
    installInfo->signHnpPaths = hnps;
    HapInstallInfoDestory(&installInfo);
    EXPECT_TRUE(installInfo == NULL);
}

HWTEST_F(HnpPrivatTest, HapInstallInfoDestory_002, TestSize.Level0)
{
    HapInstallInfo *installInfo = (HapInstallInfo *)calloc(1, sizeof(HapInstallInfo));
    installInfo->signHnpSize = 0;
    installInfo->signHnpPaths = NULL;
    HapInstallInfoDestory(&installInfo);
    EXPECT_TRUE(installInfo == NULL);
}

}