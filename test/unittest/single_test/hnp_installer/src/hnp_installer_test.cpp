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
#include "hnp_installer_test.h"
#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <cerrno>
#include "hnp_installer.h"


using namespace testing;
using namespace testing::ext;


#ifdef __cplusplus
    extern "C" {
#endif

#ifdef __cplusplus
    }
#endif

namespace OHOS {
class HnpInstallerSingleTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HnpInstallerSingleTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "Hnp_Installer_TEST SetUpTestCase";
}

void HnpInstallerSingleTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "Hnp_Installer_TEST TearDownTestCase";
}

void HnpInstallerSingleTest::SetUp()
{
    GTEST_LOG_(INFO) << "Hnp_Installer_TEST SetUp";
}

void HnpInstallerSingleTest::TearDown()
{
    GTEST_LOG_(INFO) << "Hnp_Installer_TEST TearDown";
}

static void MakeDirRecursive(const std::string &path, mode_t mode)
{
    size_t size = path.size();
    if (size == 0) {
        return;
    }

    size_t index = 0;
    do {
        size_t pathIndex = path.find_first_of('/', index);
        index = pathIndex == std::string::npos ? size : pathIndex + 1;
        std::string dir = path.substr(0, index);
        if (access(dir.c_str(), F_OK) < 0) {
            int ret = mkdir(dir.c_str(), mode);
            if (ret != 0) {
                return;
            }
        }
    } while (index < size);
}

static void CreateTestFile(const char *fileName, const char *data)
{
    FILE *tmpFile = fopen(fileName, "wr");
    if (tmpFile != nullptr) {
        fprintf(tmpFile, "%s", data);
        (void)fflush(tmpFile);
        fclose(tmpFile);
    }
}

HWTEST_F(HnpInstallerSingleTest, hnp_clearSoftLink_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "hnp_clearSoftLink_001 start";

    const std::string binDir = HNP_DEFAULT_INSTALL_ROOT_PATH"/100/hnppublic/bin";
    const std::string fileDir = HNP_DEFAULT_INSTALL_ROOT_PATH"/100/hnppublic/test.org/bin";
    
    MakeDirRecursive(binDir, 0711);
    MakeDirRecursive(fileDir, 0711);

    const std::string lnkFile = HNP_DEFAULT_INSTALL_ROOT_PATH"/100/hnppublic/bin/test";
    const std::string sourceFile = "../test.org/bin/test";
    
    CreateTestFile(HNP_DEFAULT_INSTALL_ROOT_PATH"/100/hnppublic/test.org/bin/test", "test");
    symlink(sourceFile.c_str(), lnkFile.c_str());

    ClearSoftLink(100);
    int ret = access(lnkFile.c_str(), F_OK);
    EXPECT_EQ(ret, 0);

    remove(HNP_DEFAULT_INSTALL_ROOT_PATH"/100/hnppublic/test.org/bin/test");
    ClearSoftLink(100);

    ret = access(lnkFile.c_str(), F_OK);
    EXPECT_NE(ret, 0);
}

}