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
#include <dirent.h>
#include "hnp_base.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class HnpFileTest : public testing::Test {
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

static void RemoveTestDir(const char *dirPath)
{
    DIR *dir = opendir(dirPath);
    if (dir == nullptr) {
        return;
    }
    
    struct dirent *entry;
    char filePath[PATH_MAX];
    while ((entry = readdir(dir)) != nullptr) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        (void)snprintf_s(filePath, PATH_MAX, PATH_MAX, "%s/%s", dirPath, entry->d_name);
        struct stat statbuf;
        if (lstat(filePath, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                RemoveTestDir(filePath);
            } else {
                (void)remove(filePath);
            }
        }
    }
    closedir(dir);
    (void)rmdir(dirPath);
}

/**
 * @tc.name: GetFileSizeByHandleTest_001
 * @tc.desc: Test GetFileSizeByHandle with valid file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, GetFileSizeByHandleTest_001, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/test_file_size_001.txt";
    const char *content = "hello world";
    
    EXPECT_EQ(CreateTestFile(testFile, content), 0);
    
    FILE *fp = fopen(testFile, "rb");
    EXPECT_NE(fp, nullptr);
    
    if (fp != nullptr) {
        int size = 0;
        int ret = GetFileSizeByHandle(fp, &size);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(size, static_cast<int>(strlen(content)));
        (void)fclose(fp);
    }
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: GetFileSizeByHandleTest_002
 * @tc.desc: Test GetFileSizeByHandle with empty file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, GetFileSizeByHandleTest_002, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/test_file_size_002.txt";
    const char *content = "";
    
    EXPECT_EQ(CreateTestFile(testFile, content), 0);
    
    FILE *fp = fopen(testFile, "rb");
    EXPECT_NE(fp, nullptr);
    
    if (fp != nullptr) {
        int size = 0;
        int ret = GetFileSizeByHandle(fp, &size);
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(size, 0);
        (void)fclose(fp);
    }
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: GetFileSizeByHandleTest_003
 * @tc.desc: Test GetFileSizeByHandle with large file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, GetFileSizeByHandleTest_003, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/test_file_size_003.txt";
    const int fileSize = 1024;
    char *content = (char *)malloc(fileSize);
    EXPECT_NE(content, nullptr);
    
    if (content != nullptr) {
        (void)memset_s(content, fileSize, 'a', fileSize);
        EXPECT_EQ(CreateTestFile(testFile, std::string(content, fileSize).c_str()), 0);
        
        FILE *fp = fopen(testFile, "rb");
        EXPECT_NE(fp, nullptr);
        
        if (fp != nullptr) {
            int size = 0;
            int ret = GetFileSizeByHandle(fp, &size);
            EXPECT_EQ(ret, 0);
            EXPECT_EQ(size, fileSize);
            (void)fclose(fp);
        }
        
        RemoveTestFile(testFile);
        free(content);
    }
}

/**
 * @tc.name: ReadFileToStreamTest_001
 * @tc.desc: Test ReadFileToStream with non-existent file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, ReadFileToStreamTest_001, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/non_existent_file_001.txt";
    char *stream = nullptr;
    int streamLen = 0;
    
    (void)remove(testFile);
    
    int ret = ReadFileToStream(testFile, &stream, &streamLen);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_FILE_OPEN_FAILED);
    EXPECT_EQ(stream, nullptr);
    EXPECT_EQ(streamLen, 0);
}

/**
 * @tc.name: ReadFileToStreamTest_002
 * @tc.desc: Test ReadFileToStream with valid file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, ReadFileToStreamTest_002, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/test_read_stream_002.txt";
    const char *content = "hello world test";
    
    EXPECT_EQ(CreateTestFile(testFile, content), 0);
    
    char *stream = nullptr;
    int streamLen = 0;
    
    int ret = ReadFileToStream(testFile, &stream, &streamLen);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(stream, nullptr);
    EXPECT_EQ(streamLen, static_cast<int>(strlen(content)));
    
    if (stream != nullptr) {
        EXPECT_EQ(memcmp(stream, content, streamLen), 0);
        free(stream);
    }
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: ReadFileToStreamTest_003
 * @tc.desc: Test ReadFileToStream with empty file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, ReadFileToStreamTest_003, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/test_read_stream_003.txt";
    const char *content = "";
    
    EXPECT_EQ(CreateTestFile(testFile, content), 0);
    
    char *stream = nullptr;
    int streamLen = 0;
    
    int ret = ReadFileToStream(testFile, &stream, &streamLen);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_GET_FILE_LEN_NULL);
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: ReadFileToStreamTest_004
 * @tc.desc: Test ReadFileToStream with binary file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, ReadFileToStreamTest_004, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/test_read_stream_004.bin";
    unsigned char binaryData[] = {0x00, 0x01, 0x02, 0x7F, 0x80, 0xFF, 0xAB, 0xCD};
    size_t dataSize = sizeof(binaryData);
    
    FILE *fp = fopen(testFile, "wb");
    EXPECT_NE(fp, nullptr);
    if (fp != nullptr) {
        size_t written = fwrite(binaryData, 1, dataSize, fp);
        EXPECT_EQ(written, dataSize);
        (void)fclose(fp);
    }
    
    char *stream = nullptr;
    int streamLen = 0;
    
    int ret = ReadFileToStream(testFile, &stream, &streamLen);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(stream, nullptr);
    EXPECT_EQ(streamLen, static_cast<int>(dataSize));
    
    if (stream != nullptr) {
        EXPECT_EQ(memcmp(stream, binaryData, dataSize), 0);
        free(stream);
    }
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: GetRealPathTest_001
 * @tc.desc: Test GetRealPath with null parameters
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, GetRealPathTest_001, TestSize.Level0)
{
    char realPath[MAX_FILE_PATH_LEN] = {0};
    
    int ret = GetRealPath(nullptr, realPath);
    EXPECT_EQ(ret, HNP_ERRNO_PARAM_INVALID);
}

/**
 * @tc.name: GetRealPathTest_002
 * @tc.desc: Test GetRealPath with null realPath
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, GetRealPathTest_002, TestSize.Level0)
{
    char srcPath[] = "/data/local/tmp/test_realpath_002.txt";
    
    int ret = GetRealPath(srcPath, nullptr);
    EXPECT_EQ(ret, HNP_ERRNO_PARAM_INVALID);
}

/**
 * @tc.name: GetRealPathTest_003
 * @tc.desc: Test GetRealPath with non-existent file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, GetRealPathTest_003, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/non_existent_realpath_003.txt";
    char realPath[MAX_FILE_PATH_LEN] = {0};
    
    (void)remove(testFile);
    
    int ret = GetRealPath(const_cast<char *>(testFile), realPath);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_REALPATHL_FAILED);
}

/**
 * @tc.name: GetRealPathTest_004
 * @tc.desc: Test GetRealPath with valid file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, GetRealPathTest_004, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/test_realpath_004.txt";
    const char *content = "test content";
    
    EXPECT_EQ(CreateTestFile(testFile, content), 0);
    
    char realPath[MAX_FILE_PATH_LEN] = {0};
    int ret = GetRealPath(const_cast<char *>(testFile), realPath);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(realPath[0], '\0');
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: GetRealPathTest_005
 * @tc.desc: Test GetRealPath with relative path
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, GetRealPathTest_005, TestSize.Level0)
{
    const char *testFile = "/data/local/tmp/test_realpath_005.txt";
    const char *content = "test content";
    
    EXPECT_EQ(CreateTestFile(testFile, content), 0);
    
    char realPath[MAX_FILE_PATH_LEN] = {0};
    int ret = GetRealPath(const_cast<char *>(testFile), realPath);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(realPath[0], '\0');
    EXPECT_NE(strstr(realPath, "test_realpath_005.txt"), nullptr);
    
    RemoveTestFile(testFile);
}

/**
 * @tc.name: HnpCreateFolderTest_001
 * @tc.desc: Test HnpCreateFolder with null parameter
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpCreateFolderTest_001, TestSize.Level0)
{
    int ret = HnpCreateFolder(nullptr);
    EXPECT_EQ(ret, HNP_ERRNO_BASE_PARAMS_INVALID);
}

/**
 * @tc.name: HnpCreateFolderTest_002
 * @tc.desc: Test HnpCreateFolder with valid path
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpCreateFolderTest_002, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_create_002";
    
    RemoveTestDir(testDir);
    
    int ret = HnpCreateFolder(testDir);
    EXPECT_EQ(ret, 0);
    
    struct stat buffer;
    EXPECT_EQ(stat(testDir, &buffer), 0);
    
    RemoveTestDir(testDir);
}

/**
 * @tc.name: HnpCreateFolderTest_003
 * @tc.desc: Test HnpCreateFolder with nested path
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpCreateFolderTest_003, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_create_003/subdir1/subdir2";
    
    RemoveTestDir("/data/local/tmp/test_hnp_create_003");
    
    int ret = HnpCreateFolder(testDir);
    EXPECT_EQ(ret, 0);
    
    struct stat buffer;
    EXPECT_EQ(stat(testDir, &buffer), 0);
    
    RemoveTestDir("/data/local/tmp/test_hnp_create_003");
}

/**
 * @tc.name: HnpCreateFolderTest_004
 * @tc.desc: Test HnpCreateFolder with already existing directory
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpCreateFolderTest_004, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_create_004";
    
    RemoveTestDir(testDir);
    
    int ret = HnpCreateFolder(testDir);
    EXPECT_EQ(ret, 0);
    
    ret = HnpCreateFolder(testDir);
    EXPECT_EQ(ret, 0);
    
    RemoveTestDir(testDir);
}

/**
 * @tc.name: HnpDeleteFolderTest_001
 * @tc.desc: Test HnpDeleteFolder with non-existent path
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpDeleteFolderTest_001, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_delete_nonexistent_001";
    
    RemoveTestDir(testDir);
    
    int ret = HnpDeleteFolder(testDir);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: HnpDeleteFolderTest_002
 * @tc.desc: Test HnpDeleteFolder with empty directory
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpDeleteFolderTest_002, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_delete_002";
    
    RemoveTestDir(testDir);
    EXPECT_EQ(mkdir(testDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    
    int ret = HnpDeleteFolder(testDir);
    EXPECT_EQ(ret, 0);
    
    struct stat buffer;
    EXPECT_NE(stat(testDir, &buffer), 0);
}

/**
 * @tc.name: HnpDeleteFolderTest_003
 * @tc.desc: Test HnpDeleteFolder with directory containing files
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpDeleteFolderTest_003, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_delete_003";
    const char *testFile = "/data/local/tmp/test_hnp_delete_003/test_file.txt";
    
    RemoveTestDir(testDir);
    EXPECT_EQ(mkdir(testDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(CreateTestFile(testFile, "test content"), 0);
    
    int ret = HnpDeleteFolder(testDir);
    EXPECT_EQ(ret, 0);
    
    struct stat buffer;
    EXPECT_NE(stat(testDir, &buffer), 0);
}

/**
 * @tc.name: HnpDeleteFolderTest_004
 * @tc.desc: Test HnpDeleteFolder with nested directories
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpDeleteFolderTest_004, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_delete_004";
    const char *subDir = "/data/local/tmp/test_hnp_delete_004/subdir";
    const char *testFile1 = "/data/local/tmp/test_hnp_delete_004/test_file1.txt";
    const char *testFile2 = "/data/local/tmp/test_hnp_delete_004/subdir/test_file2.txt";
    
    RemoveTestDir(testDir);
    EXPECT_EQ(mkdir(testDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir(subDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(CreateTestFile(testFile1, "test content 1"), 0);
    EXPECT_EQ(CreateTestFile(testFile2, "test content 2"), 0);
    
    int ret = HnpDeleteFolder(testDir);
    EXPECT_EQ(ret, 0);
    
    struct stat buffer;
    EXPECT_NE(stat(testDir, &buffer), 0);
}

/**
 * @tc.name: HnpPathFileCountTest_001
 * @tc.desc: Test HnpPathFileCount with non-existent directory
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpPathFileCountTest_001, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_count_nonexistent_001";
    
    RemoveTestDir(testDir);
    
    int count = HnpPathFileCount(testDir);
    EXPECT_EQ(count, -1);
}

/**
 * @tc.name: HnpPathFileCountTest_002
 * @tc.desc: Test HnpPathFileCount with empty directory
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpPathFileCountTest_002, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_count_002";
    
    RemoveTestDir(testDir);
    EXPECT_EQ(mkdir(testDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    
    int count = HnpPathFileCount(testDir);
    EXPECT_EQ(count, 0);
    
    RemoveTestDir(testDir);
}

/**
 * @tc.name: HnpPathFileCountTest_003
 * @tc.desc: Test HnpPathFileCount with directory containing one file
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpPathFileCountTest_003, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_count_003";
    const char *testFile = "/data/local/tmp/test_hnp_count_003/test_file.txt";
    
    RemoveTestDir(testDir);
    EXPECT_EQ(mkdir(testDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(CreateTestFile(testFile, "test content"), 0);
    
    int count = HnpPathFileCount(testDir);
    EXPECT_EQ(count, 1);
    
    RemoveTestDir(testDir);
}

/**
 * @tc.name: HnpPathFileCountTest_004
 * @tc.desc: Test HnpPathFileCount with directory containing multiple files
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpPathFileCountTest_004, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_count_004";
    const char *testFile1 = "/data/local/tmp/test_hnp_count_004/test_file1.txt";
    const char *testFile2 = "/data/local/tmp/test_hnp_count_004/test_file2.txt";
    const char *testFile3 = "/data/local/tmp/test_hnp_count_004/test_file3.txt";
    
    RemoveTestDir(testDir);
    EXPECT_EQ(mkdir(testDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(CreateTestFile(testFile1, "test content 1"), 0);
    EXPECT_EQ(CreateTestFile(testFile2, "test content 2"), 0);
    EXPECT_EQ(CreateTestFile(testFile3, "test content 3"), 0);
    
    int count = HnpPathFileCount(testDir);
    EXPECT_EQ(count, 3);
    
    RemoveTestDir(testDir);
}

/**
 * @tc.name: HnpPathFileCountTest_005
 * @tc.desc: Test HnpPathFileCount with directory containing subdirectories
 * @tc.type: FUNC
 */
HWTEST_F(HnpFileTest, HnpPathFileCountTest_005, TestSize.Level0)
{
    const char *testDir = "/data/local/tmp/test_hnp_count_005";
    const char *subDir = "/data/local/tmp/test_hnp_count_005/subdir";
    const char *testFile1 = "/data/local/tmp/test_hnp_count_005/test_file1.txt";
    const char *testFile2 = "/data/local/tmp/test_hnp_count_005/subdir/test_file2.txt";
    
    RemoveTestDir(testDir);
    EXPECT_EQ(mkdir(testDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(mkdir(subDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0);
    EXPECT_EQ(CreateTestFile(testFile1, "test content 1"), 0);
    EXPECT_EQ(CreateTestFile(testFile2, "test content 2"), 0);
    
    int count = HnpPathFileCount(testDir);
    EXPECT_EQ(count, 2);
    
    RemoveTestDir(testDir);
}
}