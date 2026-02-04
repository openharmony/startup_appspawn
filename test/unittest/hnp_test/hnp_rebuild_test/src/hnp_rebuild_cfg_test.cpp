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
#include "hnp_rebuild_cfg_test.h"

#include <gtest/gtest.h>
#include "hnp_installer.h"
#include "hnp_base.h"
#include "cJSON.h"
#include "securec.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <cerrno>

#define TEST_UID_INDEX_1 (10000)
#define TEST_UID_INDEX_2 (10001)

#define TEST_HNP_NAME1 "testhnp1"
#define TEST_HNP_NAME2 "testhnp2"

#define TEST_HAP_NAME1 "com.example.testhap1"
#define TEST_HAP_NAME2 "com.example.testhap2"

#define TEST_HNP_INSTALLVERSION1 "1.1"
#define TEST_HNP_INSTALLVERSION2 "1.2"

#define TEST_HNP_INDEX_1 (1)
#define TEST_HNP_INDEX_2 (2)

#ifndef HNP_TEST_DIR_MODE
#define HNP_TEST_DIR_MODE (0771)

#endif

using namespace testing;
using namespace testing::ext;

#ifdef __cplusplus
    extern "C" {
#endif

int HnpHapJsonWrite(cJSON *json, const char* fileName)
{
    if (fileName == nullptr || json == nullptr) {
        return -1;
    }

    FILE *fp = fopen(fileName, "wb");
    if (fp == nullptr) {
        printf("invalid fileName");
        return -1;
    }

    char *jsonStr = cJSON_Print(json);
    if (jsonStr == nullptr) {
        printf("get json str unsuccess!");
        (void)fclose(fp);
        return -1;
    }

    size_t jsonStrSize = strlen(jsonStr);
    size_t writeLen = fwrite(jsonStr, sizeof(char), jsonStrSize, fp);
    (void)fclose(fp);
    free(jsonStr);

    if (writeLen != jsonStrSize) {
        printf("package info write file:%s unsuccess!", fileName);
        return -1;
    }
    return 0;
}

static int AddHapToRoot(cJSON *root, const char *hapName, cJSON *hnpArray)
{
    if (root == nullptr || hapName == nullptr || hnpArray == nullptr) {
        printf("invalid AddHapToRoot param \r\n");
        return -1;
    }

    if (!cJSON_IsArray(root)|| !cJSON_IsArray(hnpArray)) {
        printf("root or hnp is not array\r\n");
        return -1;
    }

    if (cJSON_GetArraySize(hnpArray) == 0) {
        printf("empty hnpArray\r\n");
        return -1;
    }

    cJSON *hapItem = cJSON_CreateObject();
    if (hapItem == nullptr) {
        printf("cJSON_CreateObject hapItem failed\r\n");
        return -1;
    }

    cJSON_AddItemToObject(hapItem, "hnp", hnpArray);
    cJSON_AddItemToObject(hapItem, "hap", cJSON_CreateString(hapName));
    cJSON_AddItemToArray(root, hapItem);
    return 0;
}

static int AddHnpToHnpArray(cJSON *hnpArray, int index)
{
    if (hnpArray == nullptr) {
        printf("invalid AddHnpToHnpArray param \r\n");
        return -1;
    }

    if (!cJSON_IsArray(hnpArray)) {
        printf("hnpArray is not array\r\n");
        return -1;
    }

    if (index & TEST_HNP_INDEX_1) {
        cJSON *hnpArrayItem = cJSON_CreateObject();
        cJSON_AddItemToObject(hnpArrayItem, "name", cJSON_CreateString(TEST_HNP_NAME1));
        cJSON_AddItemToObject(hnpArrayItem, "install_version", cJSON_CreateString(TEST_HNP_INSTALLVERSION1));
        cJSON_AddItemToObject(hnpArrayItem, "current_version", cJSON_CreateString(TEST_HNP_INSTALLVERSION1));
        cJSON_AddItemToArray(hnpArray, hnpArrayItem);
    }

    if (index & TEST_HNP_INDEX_2) {
        cJSON *hnpArrayItem = cJSON_CreateObject();
        cJSON_AddItemToObject(hnpArrayItem, "name", cJSON_CreateString(TEST_HNP_NAME2));
        cJSON_AddItemToObject(hnpArrayItem, "install_version", cJSON_CreateString(TEST_HNP_INSTALLVERSION2));
        cJSON_AddItemToObject(hnpArrayItem, "current_version", cJSON_CreateString(TEST_HNP_INSTALLVERSION2));
        cJSON_AddItemToArray(hnpArray, hnpArrayItem);
    }
    return 0;
}

#ifdef __cplusplus
    }
#endif

namespace OHOS {
class HnpRebuildCfgSingleTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void HnpRebuildCfgSingleTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "HnpRebuildCfgSingleTest SetUpTestCase";
}

void HnpRebuildCfgSingleTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "HnpRebuildCfgSingleTest TearDownTestCase";
}

void HnpRebuildCfgSingleTest::SetUp()
{
    GTEST_LOG_(INFO) << "HnpRebuildCfgSingleTest SetUp";
}

void HnpRebuildCfgSingleTest::TearDown()
{
    GTEST_LOG_(INFO) << "HnpRebuildCfgSingleTest TearDown";
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

void RemoveUidCfg(int uid)
{
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    if (ret > 0) {
        remove(newCfgPath);
    }
}

static void CreateTestFile(const char *fileName, const char *data)
{
    FILE *tmpFile = fopen(fileName, "wr");
    if (tmpFile != nullptr) {
        if (data != nullptr) {
            fprintf(tmpFile, "%s", data);
            (void)fflush(tmpFile);
        }
        fclose(tmpFile);
    }
}

static int CreateTestDir(const char *hnpName, const char *version, const int uid)
{
    char hnpVersionPath[PATH_MAX] = {0};
    int ret = snprintf_s(hnpVersionPath, PATH_MAX, PATH_MAX - 1,
        HNP_PUBLIC_BASE_PATH, uid, hnpName, hnpName, version);
    if (ret <= 0) {
        return -1;
    }
    MakeDirRecursive(hnpVersionPath, HNP_TEST_DIR_MODE);
    return 0;
}

static int RemoveTestDir(const char *hnpName, const char *version, const int uid)
{
    char hnpVersionPath[PATH_MAX] = {0};
    int ret = snprintf_s(hnpVersionPath, PATH_MAX, PATH_MAX - 1,
        HNP_PUBLIC_BASE_PATH, uid, hnpName, hnpName, version);
    if (ret <= 0) {
        return -1;
    }

    if (access(hnpVersionPath, F_OK) == 0) {
        remove(hnpVersionPath);
    }
    return 0;
}

// exist new cfg
HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "hnp_rebuild_cfg_001 start";

    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);

    // clear old cfg
    ret = remove(newCfgPath);

    // build old cfg
    cJSON *emptyJson = cJSON_CreateArray();
    ret = HnpHapJsonWrite(emptyJson, newCfgPath);
    EXPECT_EQ(ret, 0);

    // do rebuild
    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(emptyJson);

    // clear test cfg
    ret = remove(newCfgPath);
    EXPECT_EQ(ret, 0);

    GTEST_LOG_(INFO) << "hnp_rebuild_cfg_001 end";
}

// not exist new cfg and new cfg
HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "hnp_rebuild_cfg_002 start";

    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);

    // clear old cfg
    ret = remove(newCfgPath);

    // do rebuild
    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    remove(newCfgPath);
    GTEST_LOG_(INFO) << "hnp_rebuild_cfg_002 end";
}

// exist empty old cfg
HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_003, TestSize.Level0)
{
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);

    // clear old cfg
    ret = remove(newCfgPath);

    // build old cfg
    cJSON *emptyJson = cJSON_CreateArray();
    ret = HnpHapJsonWrite(emptyJson, HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);

    // do rebuild
    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(emptyJson);

    ret = access(newCfgPath, F_OK) == 0;
    EXPECT_TRUE(ret);

    // clear test cfg
    ret = remove(newCfgPath);
    remove(HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);
}

// exist config but hnp source path not exist
HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_004, TestSize.Level0)
{
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);

    // clear new cfg
    ret = remove(newCfgPath);

    // build old cfg
    cJSON *json = cJSON_CreateArray();
    cJSON *hnpArray = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray, TEST_HNP_INDEX_1);
    AddHapToRoot(json, TEST_HAP_NAME1, hnpArray);

    ret = HnpHapJsonWrite(json, HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);

    // do rebuild
    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(json);

    ret = access(newCfgPath, F_OK) == 0;
    EXPECT_TRUE(ret);

    char *infoStream;
    int size;
    ret = ReadFileToStream(newCfgPath, &infoStream, &size);
    EXPECT_EQ(ret, 0);
    json = cJSON_Parse(infoStream);
    free(infoStream);
    EXPECT_NE(json, nullptr);
    
    EXPECT_TRUE(cJSON_IsArray(json));
    EXPECT_EQ(cJSON_GetArraySize(json), 0);
    cJSON_Delete(json);

    // clear test cfg
    ret = remove(newCfgPath);
    EXPECT_EQ(ret, 0);

    ret = remove(HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_005, TestSize.Level0)
{
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);
    ret = remove(newCfgPath);

    // build old cfg
    cJSON *json = cJSON_CreateArray();
    cJSON *hnpArray = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray, TEST_HNP_INDEX_1);
    AddHapToRoot(json, TEST_HAP_NAME1, hnpArray);
    ret = HnpHapJsonWrite(json, HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);
    ret = CreateTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_1);
    // do rebuild
    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    char *oldStr = cJSON_Print(json);
    cJSON_Delete(json);

    ret = access(newCfgPath, F_OK) == 0;
    EXPECT_TRUE(ret);

    char *infoStream;
    int size;
    ret = ReadFileToStream(newCfgPath, &infoStream, &size);
    EXPECT_EQ(ret, 0);

    json = cJSON_Parse(infoStream);
    free(infoStream);
    EXPECT_NE(json, nullptr);
    
    EXPECT_TRUE(cJSON_IsArray(json));
    EXPECT_EQ(cJSON_GetArraySize(json), 1);
    char *newStr = cJSON_Print(json);
    EXPECT_TRUE(strcmp(oldStr, newStr) == 0);
    free(newStr);
    free(oldStr);
    cJSON_Delete(json);

    // clear test cfg
    remove(newCfgPath);
    remove(HNP_OLD_CFG_PATH);
    ret = RemoveTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_006, TestSize.Level0)
{
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);

    // clear new cfg
    ret = remove(newCfgPath);

    // build old cfg
    cJSON *json = cJSON_CreateArray();
    cJSON *hnpArray = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray, TEST_HNP_INDEX_1 | TEST_HNP_INDEX_2);
    AddHapToRoot(json, TEST_HAP_NAME1, hnpArray);

    ret = HnpHapJsonWrite(json, HNP_OLD_CFG_PATH);
    cJSON_Delete(json);
    EXPECT_EQ(ret, 0);

    ret = CreateTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_1);
    EXPECT_TRUE(ret == 0);

    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);

    ret = access(newCfgPath, F_OK) == 0;
    EXPECT_TRUE(ret);

    char *infoStream;
    int size;
    ret = ReadFileToStream(newCfgPath, &infoStream, &size);
    EXPECT_EQ(ret, 0);

    json = cJSON_Parse(infoStream);
    free(infoStream);
    EXPECT_NE(json, nullptr);
    EXPECT_TRUE(cJSON_IsArray(json));
    EXPECT_EQ(cJSON_GetArraySize(json), 1);
    cJSON_Delete(json);
    ret = remove(newCfgPath);
    EXPECT_EQ(ret, 0);
    ret = RemoveTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_1);
    ret = remove(HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "hnp_rebuild_cfg_005 end";
}

HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "hnp_rebuild_cfg_006 start";
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);

    ret = remove(newCfgPath);

    cJSON *json = cJSON_CreateArray();
    cJSON *hnpArray = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray, TEST_HNP_INDEX_1 | TEST_HNP_INDEX_2);
    AddHapToRoot(json, TEST_HAP_NAME1, hnpArray);
    ret = HnpHapJsonWrite(json, HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);

    ret = CreateTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    ret = CreateTestDir(TEST_HNP_NAME2, TEST_HNP_INSTALLVERSION2, TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);

    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(json);
    ret = access(newCfgPath, F_OK) == 0;
    EXPECT_TRUE(ret);
    char *infoStream;
    int size;
    ret = ReadFileToStream(newCfgPath, &infoStream, &size);
    EXPECT_EQ(ret, 0);

    json = cJSON_Parse(infoStream);
    free(infoStream);
    EXPECT_NE(json, nullptr);
    EXPECT_TRUE(cJSON_IsArray(json));
    EXPECT_EQ(cJSON_GetArraySize(json), 1);
    cJSON *hapItem = cJSON_GetArrayItem(json, 0);
    EXPECT_TRUE(cJSON_GetArraySize(hapItem) == 2);
    cJSON_Delete(json);

    remove(newCfgPath);
    RemoveTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_1);
    RemoveTestDir(TEST_HNP_NAME2, TEST_HNP_INSTALLVERSION2, TEST_UID_INDEX_1);
    remove(HNP_OLD_CFG_PATH);
    GTEST_LOG_(INFO) << "hnp_rebuild_cfg_006 end";
}

HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_008, TestSize.Level0)
{
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);
    ret = remove(newCfgPath);

    cJSON *json = cJSON_CreateArray();
    cJSON *hnpArray = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray, TEST_HNP_INDEX_1);
    AddHapToRoot(json, TEST_HAP_NAME1, hnpArray);
    cJSON *hnpArray2 = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray2, TEST_HNP_INDEX_2);
    AddHapToRoot(json, TEST_HAP_NAME2, hnpArray2);
    ret = HnpHapJsonWrite(json, HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);

    ret = CreateTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);

    // do rebuild
    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(json);
    ret = access(newCfgPath, F_OK) == 0;
    EXPECT_TRUE(ret);

    char *infoStream;
    int size;
    ret = ReadFileToStream(newCfgPath, &infoStream, &size);
    EXPECT_EQ(ret, 0);

    json = cJSON_Parse(infoStream);
    free(infoStream);
    EXPECT_NE(json, nullptr);
    EXPECT_TRUE(cJSON_IsArray(json));
    EXPECT_EQ(cJSON_GetArraySize(json), 1);
    cJSON_Delete(json);

    // clear test cfg
    remove(newCfgPath);
    RemoveTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_1);
    remove(HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_009, TestSize.Level0)
{
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);
    ret = remove(newCfgPath);

    cJSON *json = cJSON_CreateArray();
    cJSON *hnpArray = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray, TEST_HNP_INDEX_1);
    AddHapToRoot(json, TEST_HAP_NAME1, hnpArray);
    cJSON *hnpArray2 = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray2, TEST_HNP_INDEX_2);
    AddHapToRoot(json, TEST_HAP_NAME2, hnpArray2);
    ret = HnpHapJsonWrite(json, HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);

    ret = CreateTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_2);
    EXPECT_EQ(ret, 0);

    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_2);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(json);
    ret = access(newCfgPath, F_OK) == 0;
    EXPECT_TRUE(ret);

    char *infoStream;
    int size;
    ret = ReadFileToStream(newCfgPath, &infoStream, &size);
    EXPECT_EQ(ret, 0);

    json = cJSON_Parse(infoStream);
    free(infoStream);
    EXPECT_NE(json, nullptr);
    EXPECT_TRUE(cJSON_IsArray(json));
    EXPECT_EQ(cJSON_GetArraySize(json), 0);
    cJSON_Delete(json);
    remove(newCfgPath);
    RemoveUidCfg(TEST_UID_INDEX_2);
    RemoveTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_2);
    ret = remove(HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_010, TestSize.Level0)
{
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_2);
    EXPECT_NE(ret, 0);
    ret = remove(newCfgPath);

    cJSON *json = cJSON_CreateArray();
    cJSON *hnpArray = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray, TEST_HNP_INDEX_1);
    AddHapToRoot(json, TEST_HAP_NAME1, hnpArray);
    cJSON *hnpArray2 = cJSON_CreateArray();
    AddHnpToHnpArray(hnpArray2, TEST_HNP_INDEX_2);
    AddHapToRoot(json, TEST_HAP_NAME2, hnpArray2);
    ret = HnpHapJsonWrite(json, HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);

    ret = CreateTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_2);
    EXPECT_EQ(ret, 0);

    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    EXPECT_EQ(ret, 0);
    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_2);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(json);
    ret = access(newCfgPath, F_OK) == 0;
    EXPECT_TRUE(ret);

    char *infoStream;
    int size;
    ret = ReadFileToStream(newCfgPath, &infoStream, &size);
    EXPECT_EQ(ret, 0);

    json = cJSON_Parse(infoStream);
    free(infoStream);
    EXPECT_NE(json, nullptr);
    EXPECT_TRUE(cJSON_IsArray(json));
    printf("currentJson is %s \r\n", cJSON_Print(json));
    EXPECT_EQ(cJSON_GetArraySize(json), 1);
    cJSON_Delete(json);
    remove(newCfgPath);
    RemoveUidCfg(TEST_UID_INDEX_2);
    RemoveTestDir(TEST_HNP_NAME1, TEST_HNP_INSTALLVERSION1, TEST_UID_INDEX_2);
    ret = remove(HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(HnpRebuildCfgSingleTest, HnpRebuildCfgTest_011, TestSize.Level0)
{
    MakeDirRecursive(APPSPAWN_BASE_DIR"/data/service/el1/startup", HNP_TEST_DIR_MODE);
    char newCfgPath[PATH_MAX] = {0};
    int ret = snprintf_s(newCfgPath, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, TEST_UID_INDEX_1);
    EXPECT_NE(ret, 0);
    ret = remove(newCfgPath);
    CreateTestFile(HNP_OLD_CFG_PATH, nullptr);

    ret = RebuildHnpInfoCfg(TEST_UID_INDEX_1);
    remove(newCfgPath);
    remove(HNP_OLD_CFG_PATH);
    EXPECT_EQ(ret, 0);
}
}