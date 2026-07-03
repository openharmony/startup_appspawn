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

#include <gtest/gtest.h>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include "cJSON.h"
#include "hnp_base.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class HnpJsonTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    }
};

static int CreateTestFile(const char *filePath, const char *content)
{
    char dirPath[PATH_MAX] = {0};
    const char *lastSlash = strrchr(filePath, '/');
    if (lastSlash != nullptr) {
        size_t dirLen = lastSlash - filePath;
        if (dirLen > 0 && dirLen < PATH_MAX) {
            (void)strncpy_s(dirPath, PATH_MAX, filePath, dirLen);
            dirPath[dirLen] = '\0';
            mkdir(dirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }
    }
    
    FILE *fp = fopen(filePath, "w");
    if (fp == nullptr) {
        return -1;
    }
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, fp);
    (void)fclose(fp);
    return (written == len) ? 0 : -1;
}

static void RemoveTestFile(const char *filePath)
{
    (void)remove(filePath);
}

static void EnsurePathExists(const char *filePath)
{
    char dirPath[PATH_MAX] = {0};
    const char *lastSlash = strrchr(filePath, '/');
    if (lastSlash != nullptr) {
        size_t dirLen = lastSlash - filePath;
        if (dirLen > 0 && dirLen < PATH_MAX) {
            (void)strncpy_s(dirPath, PATH_MAX, filePath, dirLen);
            dirPath[dirLen] = '\0';
            HnpCreateFolder(dirPath);
        }
    }
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_001
 * @tc.desc: Test HnpCfgGetFromSteam with null stream
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_001, TestSize.Level0)
{
    char *cfgStream = nullptr;
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED);
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_002
 * @tc.desc: Test HnpCfgGetFromSteam with invalid json content
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_002, TestSize.Level0)
{
    char cfgStream[] = "invalid json content";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARSE_JSON_FAILED);
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_003
 * @tc.desc: Test HnpCfgGetFromSteam with valid json without links
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_003, TestSize.Level0)
{
    char cfgStream[] = "{\"type\":\"hnp-config\",\"name\":\"test\",\"version\":\"1.0\",\"install\":{}}";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(hnpCfg.name, "test");
    EXPECT_STREQ(hnpCfg.version, "1.0");
    EXPECT_EQ(hnpCfg.linkNum, 0u);
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_004
 * @tc.desc: Test HnpCfgGetFromSteam with invalid type field
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_004, TestSize.Level0)
{
    char cfgStream[] = "{\"type\":\"invalid\",\"name\":\"test\",\"version\":\"1.0\",\"install\":{}}";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND);
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_005
 * @tc.desc: Test HnpCfgGetFromSteam without name field
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_005, TestSize.Level0)
{
    char cfgStream[] = "{\"type\":\"hnp-config\",\"version\":\"1.0\",\"install\":{}}";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND);
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_006
 * @tc.desc: Test HnpCfgGetFromSteam without version field
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_006, TestSize.Level0)
{
    char cfgStream[] = "{\"type\":\"hnp-config\",\"name\":\"test\",\"install\":{}}";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND);
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_007
 * @tc.desc: Test HnpCfgGetFromSteam without install field
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_007, TestSize.Level0)
{
    char cfgStream[] = "{\"type\":\"hnp-config\",\"name\":\"test\",\"version\":\"1.0\"}";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND);
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_008
 * @tc.desc: Test HnpCfgGetFromSteam with one link item
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_008, TestSize.Level0)
{
    char cfgStream[] = "{\"type\":\"hnp-config\",\"name\":\"test\",\"version\":\"1.0\",\"install\":"
        "{\"links\":[{\"source\":\"bin/test\",\"target\":\"test\"}]}}";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(hnpCfg.name, "test");
    EXPECT_STREQ(hnpCfg.version, "1.0");
    EXPECT_EQ(hnpCfg.linkNum, 1u);
    EXPECT_NE(hnpCfg.links, nullptr);
    if (hnpCfg.links != nullptr) {
        EXPECT_STREQ(hnpCfg.links[0].source, "bin/test");
        EXPECT_STREQ(hnpCfg.links[0].target, "test");
        free(hnpCfg.links);
    }
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_009
 * @tc.desc: Test HnpCfgGetFromSteam with two link items
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_009, TestSize.Level0)
{
    char cfgStream[] = "{\"type\":\"hnp-config\",\"name\":\"test\",\"version\":\"1.0\",\"install\":"
        "{\"links\":[{\"source\":\"bin/test1\"},{\"source\":\"bin/test2\",\"target\":\"test2\"}]}}";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(hnpCfg.linkNum, 2u);
    EXPECT_NE(hnpCfg.links, nullptr);
    if (hnpCfg.links != nullptr) {
        EXPECT_STREQ(hnpCfg.links[0].source, "bin/test1");
        EXPECT_EQ(hnpCfg.links[0].target[0], '\0');
        EXPECT_STREQ(hnpCfg.links[1].source, "bin/test2");
        EXPECT_STREQ(hnpCfg.links[1].target, "test2");
        free(hnpCfg.links);
    }
}

/**
 * @tc.name: HnpCfgGetFromSteamTest_010
 * @tc.desc: Test HnpCfgGetFromSteam with link missing source field
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCfgGetFromSteamTest_010, TestSize.Level0)
{
    char cfgStream[] = "{\"type\":\"hnp-config\",\"name\":\"test\",\"version\":\"1.0\",\"install\":"
        "{\"links\":[{\"target\":\"test\"}]}}";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = HnpCfgGetFromSteam(cfgStream, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND);
}

/**
 * @tc.name: ParseHnpCfgFileTest_001
 * @tc.desc: Test ParseHnpCfgFile with non-existent file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, ParseHnpCfgFileTest_001, TestSize.Level0)
{
    const char *testFile = "/tmp/test_hnp_cfg_001.json";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = ParseHnpCfgFile(testFile, &hnpCfg);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: ParseHnpCfgFileTest_002
 * @tc.desc: Test ParseHnpCfgFile with valid config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, ParseHnpCfgFileTest_002, TestSize.Level0)
{
    const char *testFile = "/tmp/test_hnp_cfg_002.json";
    const char *content = "{\"type\":\"hnp-config\",\"name\":\"test\",\"version\":\"1.0\",\"install\":{}}";
    
    EXPECT_EQ(CreateTestFile(testFile, content), 0);
    
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = ParseHnpCfgFile(testFile, &hnpCfg);
    EXPECT_EQ(ret, 0);
    EXPECT_STREQ(hnpCfg.name, "test");
    EXPECT_STREQ(hnpCfg.version, "1.0");
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: ParseHnpCfgFileTest_003
 * @tc.desc: Test ParseHnpCfgFile with invalid json file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, ParseHnpCfgFileTest_003, TestSize.Level0)
{
    const char *testFile = "/tmp/test_hnp_cfg_003.json";
    const char *content = "invalid json";
    
    EXPECT_EQ(CreateTestFile(testFile, content), 0);
    
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    
    int ret = ParseHnpCfgFile(testFile, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARSE_JSON_FAILED);
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: GetHnpJsonBuffTest_001
 * @tc.desc: Test GetHnpJsonBuff with valid hnp config
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, GetHnpJsonBuffTest_001, TestSize.Level0)
{
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    strcpy_s(hnpCfg.name, MAX_FILE_PATH_LEN, "test");
    strcpy_s(hnpCfg.version, HNP_VERSION_LEN, "1.0");
    
    char *buff = nullptr;
    int ret = GetHnpJsonBuff(&hnpCfg, &buff);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(buff, nullptr);
    
    if (buff != nullptr) {
        cJSON *json = cJSON_Parse(buff);
        EXPECT_NE(json, nullptr);
        if (json != nullptr) {
            cJSON *typeItem = cJSON_GetObjectItem(json, "type");
            EXPECT_NE(typeItem, nullptr);
            EXPECT_STREQ(typeItem->valuestring, "hnp-config");
            
            cJSON *nameItem = cJSON_GetObjectItem(json, "name");
            EXPECT_NE(nameItem, nullptr);
            EXPECT_STREQ(nameItem->valuestring, "test");
            
            cJSON *versionItem = cJSON_GetObjectItem(json, "version");
            EXPECT_NE(versionItem, nullptr);
            EXPECT_STREQ(versionItem->valuestring, "1.0");
            
            cJSON_Delete(json);
        }
        free(buff);
    }
}

/**
 * @tc.name: HnpInstallInfoJsonWriteTest_001
 * @tc.desc: Test HnpInstallInfoJsonWrite with null hapPackageName
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpInstallInfoJsonWriteTest_001, TestSize.Level0)
{
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    hnpCfg.uid = 10000;
    strcpy_s(hnpCfg.name, MAX_FILE_PATH_LEN, "test_hnp");
    strcpy_s(hnpCfg.version, HNP_VERSION_LEN, "1.0");
    hnpCfg.isInstall = true;
    
    int ret = HnpInstallInfoJsonWrite(nullptr, &hnpCfg);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARAMS_INVALID);
}

/**
 * @tc.name: HnpInstallInfoJsonWriteTest_002
 * @tc.desc: Test HnpInstallInfoJsonWrite with null hnpCfg
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpInstallInfoJsonWriteTest_002, TestSize.Level0)
{
    const char *hapPackageName = "com.test.hap";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    hnpCfg.uid = 10000;
    strcpy_s(hnpCfg.name, MAX_FILE_PATH_LEN, "test_hnp");
    strcpy_s(hnpCfg.version, HNP_VERSION_LEN, "1.0");
    hnpCfg.isInstall = true;
    
    int ret = HnpInstallInfoJsonWrite(hapPackageName, nullptr);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARAMS_INVALID);
}

/**
 * @tc.name: HnpInstallInfoJsonWriteTest_003
 * @tc.desc: Test HnpInstallInfoJsonWrite with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpInstallInfoJsonWriteTest_003, TestSize.Level0)
{
    const char *hapPackageName = "com.test.hap";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    hnpCfg.uid = 10000;
    strcpy_s(hnpCfg.name, MAX_FILE_PATH_LEN, "test_hnp");
    strcpy_s(hnpCfg.version, HNP_VERSION_LEN, "1.0");
    hnpCfg.isInstall = true;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, hnpCfg.uid);
    
    char dirPath[PATH_MAX] = {0};
    const char *lastSlash = strrchr(cfgPath, '/');
    if (lastSlash != nullptr) {
        size_t dirLen = lastSlash - cfgPath;
        (void)strncpy_s(dirPath, PATH_MAX, cfgPath, dirLen);
        dirPath[dirLen] = '\0';
        HnpCreateFolder(dirPath);
    }
    
    (void)remove(cfgPath);
    
    int ret = HnpInstallInfoJsonWrite(hapPackageName, &hnpCfg);
    EXPECT_EQ(ret, 0);
    if (ret == 0) {
        FILE *fp = fopen(cfgPath, "r");
        EXPECT_NE(fp, nullptr);
        if (fp != nullptr) {
            (void)fclose(fp);
        }
    }
    
    (void)remove(cfgPath);
}

/**
 * @tc.name: HnpPackageInfoGetTest_001
 * @tc.desc: Test HnpPackageInfoGet with non-existent config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpPackageInfoGetTest_001, TestSize.Level0)
{
    const char *packageName = "com.test.hap";
    HnpPackageInfo *packageInfo = nullptr;
    int count = 0;
    int uid = 10001;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    (void)remove(cfgPath);
    
    int ret = HnpPackageInfoGet(packageName, &packageInfo, &count, uid);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(count, 0);
    EXPECT_EQ(packageInfo, nullptr);
}

/**
 * @tc.name: HnpPackageInfoGetTest_002
 * @tc.desc: Test HnpPackageInfoGet with valid config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpPackageInfoGetTest_002, TestSize.Level0)
{
    const char *packageName = "com.test.hap";
    HnpPackageInfo *packageInfo = nullptr;
    int count = 0;
    int uid = 10002;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    
    char dirPath[PATH_MAX] = {0};
    const char *lastSlash = strrchr(cfgPath, '/');
    if (lastSlash != nullptr) {
        size_t dirLen = lastSlash - cfgPath;
        (void)strncpy_s(dirPath, PATH_MAX, cfgPath, dirLen);
        dirPath[dirLen] = '\0';
        HnpCreateFolder(dirPath);
    }
    
    const char *content = "[{\"hap\":\"com.test.hap\",\"hnp\":[{\"name\":\"test_hnp\","
        "\"current_version\":\"1.0\",\"install_version\":\"1.0\"}]}]";
    EXPECT_EQ(CreateTestFile(cfgPath, content), 0);
    
    int ret = HnpPackageInfoGet(packageName, &packageInfo, &count, uid);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(count, 1);
    
    if (packageInfo != nullptr) {
        EXPECT_STREQ(packageInfo[0].name, "test_hnp");
        EXPECT_STREQ(packageInfo[0].currentVersion, "1.0");
        EXPECT_STREQ(packageInfo[0].installVersion, "1.0");
        free(packageInfo);
    }
    
    (void)remove(cfgPath);
}

/**
 * @tc.name: HnpPackageInfoHnpDeleteTest_001
 * @tc.desc: Test HnpPackageInfoHnpDelete with non-existent config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpPackageInfoHnpDeleteTest_001, TestSize.Level0)
{
    const char *packageName = "com.test.hap";
    const char *hnpName = "test_hnp";
    const char *version = "1.0";
    int uid = 10003;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    (void)remove(cfgPath);
    
    int ret = HnpPackageInfoHnpDelete(packageName, hnpName, version, uid);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: HnpPackageInfoHnpDeleteTest_002
 * @tc.desc: Test HnpPackageInfoHnpDelete with valid config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpPackageInfoHnpDeleteTest_002, TestSize.Level0)
{
    const char *packageName = "com.test.hap";
    const char *hnpName = "test_hnp";
    const char *version = "1.0";
    int uid = 10004;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    
    EnsurePathExists(cfgPath);
    
    const char *content = "[{\"hap\":\"com.test.hap\",\"hnp\":[{\"name\":\"test_hnp\","
        "\"current_version\":\"1.0\",\"install_version\":\"1.0\"}]}]";
    EXPECT_EQ(CreateTestFile(cfgPath, content), 0);
    
    int ret = HnpPackageInfoHnpDelete(packageName, hnpName, version, uid);
    EXPECT_EQ(ret, 0);
    
    (void)remove(cfgPath);
}

/**
 * @tc.name: HnpPackageInfoDeleteTest_001
 * @tc.desc: Test HnpPackageInfoDelete with non-existent config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpPackageInfoDeleteTest_001, TestSize.Level0)
{
    const char *packageName = "com.test.hap";
    int uid = 10005;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    (void)remove(cfgPath);
    
    int ret = HnpPackageInfoDelete(packageName, uid);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: HnpPackageInfoDeleteTest_002
 * @tc.desc: Test HnpPackageInfoDelete with valid config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpPackageInfoDeleteTest_002, TestSize.Level0)
{
    const char *packageName = "com.test.hap";
    int uid = 10006;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    
    const char *content = "[{\"hap\":\"com.test.hap\",\"hnp\":[{\"name\":\"test_hnp\","
        "\"current_version\":\"1.0\",\"install_version\":\"1.0\"}]}]";
    EXPECT_EQ(CreateTestFile(cfgPath, content), 0);
    
    int ret = HnpPackageInfoDelete(packageName, uid);
    EXPECT_EQ(ret, 0);
    
    (void)remove(cfgPath);
}

/**
 * @tc.name: HnpCurrentVersionGetTest_001
 * @tc.desc: Test HnpCurrentVersionGet with non-existent config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCurrentVersionGetTest_001, TestSize.Level0)
{
    const char *hnpName = "test_hnp";
    int uid = 10007;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    (void)remove(cfgPath);
    
    char *version = HnpCurrentVersionGet(hnpName, uid);
    EXPECT_EQ(version, nullptr);
}

/**
 * @tc.name: HnpCurrentVersionGetTest_002
 * @tc.desc: Test HnpCurrentVersionGet with valid config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCurrentVersionGetTest_002, TestSize.Level0)
{
    const char *hnpName = "test_hnp";
    int uid = 10008;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    
    const char *content = "[{\"hap\":\"com.test.hap\",\"hnp\":[{\"name\":\"test_hnp\","
        "\"current_version\":\"1.0\",\"install_version\":\"1.0\"}]}]";
    EXPECT_EQ(CreateTestFile(cfgPath, content), 0);
    
    char *version = HnpCurrentVersionGet(hnpName, uid);
    EXPECT_NE(version, nullptr);
    if (version != nullptr) {
        EXPECT_STREQ(version, "1.0");
        free(version);
    }
    
    (void)remove(cfgPath);
}

/**
 * @tc.name: HnpCurrentVersionGetTest_003
 * @tc.desc: Test HnpCurrentVersionGet with null hnpName
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCurrentVersionGetTest_003, TestSize.Level0)
{
    char *version = HnpCurrentVersionGet(nullptr, 10009);
    EXPECT_EQ(version, nullptr);
}

/**
 * @tc.name: HnpCurrentVersionUninstallCheckTest_001
 * @tc.desc: Test HnpCurrentVersionUninstallCheck with non-existent config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCurrentVersionUninstallCheckTest_001, TestSize.Level0)
{
    const char *hnpName = "test_hnp";
    int uid = 10010;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    (void)remove(cfgPath);
    
    char *version = HnpCurrentVersionUninstallCheck(hnpName, uid);
    EXPECT_EQ(version, nullptr);
}

/**
 * @tc.name: HnpCurrentVersionUninstallCheckTest_002
 * @tc.desc: Test HnpCurrentVersionUninstallCheck with valid config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCurrentVersionUninstallCheckTest_002, TestSize.Level0)
{
    const char *hnpName = "test_hnp";
    int uid = 10011;
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    
    const char *content = "[{\"hap\":\"com.test.hap\",\"hnp\":[{\"name\":\"test_hnp\","
        "\"current_version\":\"1.0\",\"install_version\":\"1.0\"}]}]";
    EXPECT_EQ(CreateTestFile(cfgPath, content), 0);
    
    char *version = HnpCurrentVersionUninstallCheck(hnpName, uid);
    EXPECT_NE(version, nullptr);
    if (version != nullptr) {
        EXPECT_STREQ(version, "1.0");
        free(version);
    }
    
    (void)remove(cfgPath);
}

/**
 * @tc.name: HnpCurrentVersionUninstallCheckTest_003
 * @tc.desc: Test HnpCurrentVersionUninstallCheck with null hnpName
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, HnpCurrentVersionUninstallCheckTest_003, TestSize.Level0)
{
    char *version = HnpCurrentVersionUninstallCheck(nullptr, 10012);
    EXPECT_EQ(version, nullptr);
}

/**
 * @tc.name: CanRecoveryTest_001
 * @tc.desc: Test CanRecovery with non-existent config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, CanRecoveryTest_001, TestSize.Level0)
{
    const char *hnpPackageName = "com.test.hap";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    hnpCfg.uid = 10013;
    strcpy_s(hnpCfg.name, MAX_FILE_PATH_LEN, "test_hnp");
    strcpy_s(hnpCfg.version, HNP_VERSION_LEN, "1.0");
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, hnpCfg.uid);
    (void)remove(cfgPath);
    
    bool canRecovery = CanRecovery(hnpPackageName, &hnpCfg);
    EXPECT_EQ(canRecovery, true);
}

/**
 * @tc.name: CanRecoveryTest_002
 * @tc.desc: Test CanRecovery with same package config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, CanRecoveryTest_002, TestSize.Level0)
{
    const char *hnpPackageName = "com.test.hap";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    hnpCfg.uid = 10014;
    strcpy_s(hnpCfg.name, MAX_FILE_PATH_LEN, "test_hnp");
    strcpy_s(hnpCfg.version, HNP_VERSION_LEN, "1.0");
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, hnpCfg.uid);
    
    const char *content = "[{\"hap\":\"com.test.hap\",\"hnp\":[{\"name\":\"test_hnp\","
        "\"current_version\":\"1.0\",\"install_version\":\"1.0\"}]}]";
    EXPECT_EQ(CreateTestFile(cfgPath, content), 0);
    
    bool canRecovery = CanRecovery(hnpPackageName, &hnpCfg);
    EXPECT_EQ(canRecovery, true);
    
    (void)remove(cfgPath);
}

/**
 * @tc.name: CanRecoveryTest_003
 * @tc.desc: Test CanRecovery with different package config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, CanRecoveryTest_003, TestSize.Level0)
{
    const char *hnpPackageName = "com.test.hap";
    HnpCfgInfo hnpCfg;
    (void)memset_s(&hnpCfg, sizeof(HnpCfgInfo), 0, sizeof(HnpCfgInfo));
    hnpCfg.uid = 10015;
    strcpy_s(hnpCfg.name, MAX_FILE_PATH_LEN, "test_hnp");
    strcpy_s(hnpCfg.version, HNP_VERSION_LEN, "1.0");
    
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, hnpCfg.uid);
    
    const char *content = "[{\"hap\":\"com.other.hap\",\"hnp\":[{\"name\":\"test_hnp\","
        "\"current_version\":\"1.0\",\"install_version\":\"1.0\"}]}]";
    EXPECT_EQ(CreateTestFile(cfgPath, content), 0);
    
    bool canRecovery = CanRecovery(hnpPackageName, &hnpCfg);
    EXPECT_EQ(canRecovery, false);
    
    (void)remove(cfgPath);
}

/**
 * @tc.name: DoRebuildHnpInfoCfgTest_001
 * @tc.desc: Test DoRebuildHnpInfoCfg with empty config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, DoRebuildHnpInfoCfgTest_001, TestSize.Level0)
{
    int uid = 10016;
    
    EnsurePathExists(HNP_OLD_CFG_PATH);
    EXPECT_EQ(CreateTestFile(HNP_OLD_CFG_PATH, ""), 0);
    
    int ret = DoRebuildHnpInfoCfg(uid);
    EXPECT_EQ(ret, 0);
    
    (void)remove(HNP_OLD_CFG_PATH);
}

/**
 * @tc.name: DoRebuildHnpInfoCfgTest_002
 * @tc.desc: Test DoRebuildHnpInfoCfg with empty array config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, DoRebuildHnpInfoCfgTest_002, TestSize.Level0)
{
    int uid = 10017;
    
    EnsurePathExists(HNP_OLD_CFG_PATH);
    
    const char *content = "[]";
    EXPECT_EQ(CreateTestFile(HNP_OLD_CFG_PATH, content), 0);
    
    int ret = DoRebuildHnpInfoCfg(uid);
    EXPECT_EQ(ret, 0);
    
    (void)remove(HNP_OLD_CFG_PATH);
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    (void)remove(cfgPath);
}

/**
 * @tc.name: DoRebuildHnpInfoCfgTest_003
 * @tc.desc: Test DoRebuildHnpInfoCfg with valid config file
 * @tc.type: FUNC
 */
HWTEST_F(HnpJsonTest, DoRebuildHnpInfoCfgTest_003, TestSize.Level0)
{
    int uid = 10018;
    
    EnsurePathExists(HNP_OLD_CFG_PATH);
    
    const char *content = "[{\"hap\":\"com.test.hap\",\"hnp\":[{\"name\":\"test_hnp\","
        "\"current_version\":\"1.0\",\"install_version\":\"1.0\"}]}]";
    EXPECT_EQ(CreateTestFile(HNP_OLD_CFG_PATH, content), 0);
    
    int ret = DoRebuildHnpInfoCfg(uid);
    EXPECT_EQ(ret, 0);
    
    (void)remove(HNP_OLD_CFG_PATH);
    char cfgPath[PATH_MAX] = {0};
    (void)snprintf_s(cfgPath, PATH_MAX, PATH_MAX, HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    (void)remove(cfgPath);
}
}