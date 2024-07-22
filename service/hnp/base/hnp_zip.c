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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include <dirent.h>
#ifdef _WIN32
#include <windows.h>

#endif

#include "zlib.h"
#include "contrib/minizip/zip.h"
#include "contrib/minizip/unzip.h"

#include "securec.h"

#include "hnp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZIP_EXTERNAL_FA_OFFSET 16

// zipOpenNewFileInZip3只识别带‘/’的路径，需要将路径中‘\’转换成‘/’
static void TransPath(const char *input, char *output)
{
    int len = strlen(input);
    for (int i = 0; i < len; i++) {
        if (input[i] == '\\') {
            output[i] = '/';
        } else {
            output[i] = input[i];
        }
    }
    output[len] = '\0';
}

#ifdef _WIN32
// 转换char路径字符串为wchar_t宽字符串,支持路径字符串长度超过260
static bool TransWidePath(const char *inPath, wchar_t *outPath)
{
    wchar_t tmpPath[MAX_FILE_PATH_LEN] = {0};
    MultiByteToWideChar(CP_ACP, 0, inPath, -1, tmpPath, MAX_FILE_PATH_LEN);
    if (swprintf_s(outPath, MAX_FILE_PATH_LEN, L"\\\\?\\%ls", tmpPath) < 0) {
        HNP_LOGE("swprintf unsuccess.");
        return false;
    }
    return true;
}
#endif

// 向zip压缩包中添加文件
static int ZipAddFile(const char* file, int offset, zipFile zf)
{
    int err;
    char buf[1024];
    char transPath[MAX_FILE_PATH_LEN];
    int len;
    FILE *f;
    zip_fileinfo fileInfo = {0};

#ifdef _WIN32
    struct _stat buffer = {0};
    // 使用wchar_t支持处理字符串长度超过260的路径字符串
    wchar_t wideFullPath[MAX_FILE_PATH_LEN] = {0};
    if (!TransWidePath(file, wideFullPath)) {
        return HNP_ERRNO_BASE_STAT_FAILED;
    }
    if (_wstat(wideFullPath, &buffer) != 0) {
        HNP_LOGE("get filefile[%{public}s] stat fail.", file);
        return HNP_ERRNO_BASE_STAT_FAILED;
    }
    buffer.st_mode |= S_IXOTH;
#else
    struct stat buffer = {0};
    if (stat(file, &buffer) != 0) {
        HNP_LOGE("get filefile[%{public}s] stat fail.", file);
        return HNP_ERRNO_BASE_STAT_FAILED;
    }
#endif
    fileInfo.external_fa = (buffer.st_mode & 0xFFFF) << ZIP_EXTERNAL_FA_OFFSET;
    TransPath(file, transPath);
    err = zipOpenNewFileInZip3(zf, transPath + offset, &fileInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED,
        Z_BEST_COMPRESSION, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0);
    if (err != ZIP_OK) {
        HNP_LOGE("open new file[%{public}s] in zip unsuccess ", file);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }
#ifdef _WIN32
    f = _wfopen(wideFullPath, L"rb");
#else
    f = fopen(file, "rb");
#endif
    if (f == NULL) {
        HNP_LOGE("open file[%{public}s] unsuccess ", file);
        return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
    }

    while ((len = fread(buf, 1, sizeof(buf), f)) > 0) {
        zipWriteInFileInZip(zf, buf, len);
    }
    (void)fclose(f);
    zipCloseFileInZip(zf);
    return 0;
}

// 判断是否为目录
static int IsDirPath(struct dirent *entry, char *fullPath, int *isDir)
{
#ifdef _WIN32
    // 使用wchar_t支持处理字符串长度超过260的路径字符串
    wchar_t wideFullPath[MAX_FILE_PATH_LEN] = {0};
    if (!TransWidePath(fullPath, wideFullPath)) {
        return HNP_ERRNO_GET_FILE_ATTR_FAILED;
    }
    DWORD fileAttr = GetFileAttributesW(wideFullPath);
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        DWORD err = GetLastError();
        HNP_LOGE("get file[%{public}s] attr unsuccess, errno[%{public}lu].", fullPath, err);
        return HNP_ERRNO_GET_FILE_ATTR_FAILED;
    }
    *isDir = (int)(fileAttr & FILE_ATTRIBUTE_DIRECTORY);
#else
    *isDir = (int)(entry->d_type == DT_DIR);
#endif

    return 0;
}

static int ZipAddDir(const char *sourcePath, int offset, zipFile zf);

static int ZipHandleDir(char *fullPath, int offset, zipFile zf)
{
    int ret;
    char transPath[MAX_FILE_PATH_LEN];
    TransPath(fullPath, transPath);
    if (zipOpenNewFileInZip3(zf, transPath + offset, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED,
                             Z_BEST_COMPRESSION, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
                             NULL, 0) != ZIP_OK) {
        HNP_LOGE("open new file[%{public}s] in zip unsuccess ", fullPath);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }
    zipCloseFileInZip(zf);
    ret = ZipAddDir(fullPath, offset, zf);
    if (ret != 0) {
        HNP_LOGE("zip add dir[%{public}s] unsuccess ", fullPath);
        return ret;
    }
    return 0;
}

// sourcePath--文件夹路径  zf--压缩文件句柄
static int ZipAddDir(const char *sourcePath, int offset, zipFile zf)
{
    struct dirent *entry;
    char fullPath[MAX_FILE_PATH_LEN];
    int isDir;

    DIR *dir = opendir(sourcePath);
    if (dir == NULL) {
        HNP_LOGE("open dir=%{public}s unsuccess ", sourcePath);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (sprintf_s(fullPath, MAX_FILE_PATH_LEN, "%s%s", sourcePath, entry->d_name) < 0) {
            HNP_LOGE("sprintf unsuccess.");
            closedir(dir);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        int ret = IsDirPath(entry, fullPath, &isDir);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
        if (isDir) {
            int endPos = strlen(fullPath);
            fullPath[endPos] = DIR_SPLIT_SYMBOL;
            fullPath[endPos + 1] = '\0';
            ret = ZipHandleDir(fullPath, offset, zf);
            if (ret != 0) {
                closedir(dir);
                return ret;
            }
        } else if ((ret = ZipAddFile(fullPath, offset, zf)) != 0) {
            HNP_LOGE("zip add file[%{public}s] unsuccess ", fullPath);
            closedir(dir);
            return ret;
        }
    }
    closedir(dir);

    return 0;
}

static int ZipDir(const char *sourcePath, int offset, const char *zipPath)
{
    int ret;
    char transPath[MAX_FILE_PATH_LEN];

    zipFile zf = zipOpen(zipPath, APPEND_STATUS_CREATE);
    if (zf == NULL) {
        HNP_LOGE("open zip=%{public}s unsuccess ", zipPath);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }

    TransPath(sourcePath, transPath);

    // 将外层文件夹信息保存到zip文件中
    ret = zipOpenNewFileInZip3(zf, transPath + offset, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION,
        0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0);
    if (ret != ZIP_OK) {
        HNP_LOGE("open new file[%{public}s] in zip unsuccess ", sourcePath + offset);
        zipClose(zf, NULL);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }
    zipCloseFileInZip(zf);
    ret = ZipAddDir(sourcePath, offset, zf);

    zipClose(zf, NULL);

    return ret;
}

int HnpZip(const char *inputDir, const char *outputFile)
{
    int ret;
    char *strPtr;
    int offset;
    char sourcePath[MAX_FILE_PATH_LEN];

    HNP_LOGI("HnpZip dir=%{public}s, output=%{public}s ", inputDir, outputFile);

    // zip压缩文件内只保存相对路径，不保存绝对路径信息，偏移到压缩文件夹位置
    strPtr = strrchr(inputDir, DIR_SPLIT_SYMBOL);
    if (strPtr == NULL) {
        offset = 0;
    } else {
        offset = strPtr - inputDir + 1;
    }

    // zip函数根据后缀是否'/'区分目录还是文件
    ret = sprintf_s(sourcePath, MAX_FILE_PATH_LEN, "%s%c", inputDir, DIR_SPLIT_SYMBOL);
    if (ret < 0) {
        HNP_LOGE("sprintf unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    ret = ZipDir(sourcePath, offset, outputFile);

    return ret;
}

int HnpAddFileToZip(char *zipfile, char *filename, char *buff, int size)
{
    zipFile zf;
    int ret;
    char transPath[MAX_FILE_PATH_LEN];

    zf = zipOpen(zipfile, APPEND_STATUS_ADDINZIP);
    if (zf == NULL) {
        HNP_LOGE("open zip=%{public}s unsuccess ", zipfile);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }

    TransPath(filename, transPath);

    // 将外层文件夹信息保存到zip文件中
    ret = zipOpenNewFileInZip3(zf, transPath, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION,
        0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0);
    if (ret != ZIP_OK) {
        HNP_LOGE("open new file[%{public}s] in zip unsuccess ", filename);
        zipClose(zf, NULL);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }
    zipWriteInFileInZip(zf, buff, size);
    zipCloseFileInZip(zf);
    zipClose(zf, NULL);

    return 0;
}

static int HnpUnZipForFile(const char *filePath, unzFile zipFile, unz_file_info fileInfo)
{
#ifdef _WIN32
    return 0;
#else
    int ret;
    mode_t mode = (fileInfo.external_fa >> ZIP_EXTERNAL_FA_OFFSET) & 0xFFFF;

    /* 如果解压缩的是目录 */
    if (filePath[strlen(filePath) - 1] == '/') {
        mkdir(filePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    } else {
        FILE *outFile = fopen(filePath, "wb");
        if (outFile == NULL) {
            HNP_LOGE("unzip open file:%{public}s unsuccess!", filePath);
            return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
        }
        unzOpenCurrentFile(zipFile);
        int readSize = 0;
        do {
            char buffer[BUFFER_SIZE];
            readSize = unzReadCurrentFile(zipFile, buffer, sizeof(buffer));
            if (readSize < 0) {
                HNP_LOGE("unzip read zip:%{public}s file unsuccess", (char *)zipFile);
                fclose(outFile);
                unzCloseCurrentFile(zipFile);
                return HNP_ERRNO_BASE_UNZIP_READ_FAILED;
            }

            fwrite(buffer, readSize, sizeof(char), outFile);
        } while (readSize > 0);

        fclose(outFile);
        unzCloseCurrentFile(zipFile);
        /* 如果其他人有可执行权限，那么将解压后的权限设置成755，否则为744 */
        if ((mode & S_IXOTH) != 0) {
            ret = chmod(filePath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        } else {
            ret = chmod(filePath, S_IRWXU | S_IRGRP | S_IROTH);
        }
        if (ret != 0) {
            HNP_LOGE("hnp install chmod unsuccess, src:%{public}s, errno:%{public}d", filePath, errno);
            return HNP_ERRNO_BASE_CHMOD_FAILED;
        }
    }
    return 0;
#endif
}

static bool HnpELFFileCheck(const char *path)
{
    FILE *fp;
    char buff[HNP_ELF_FILE_CHECK_HEAD_LEN];

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return false;
    }

    int ret = fread(buff, sizeof(char), HNP_ELF_FILE_CHECK_HEAD_LEN, fp);
    if (ret != HNP_ELF_FILE_CHECK_HEAD_LEN) {
        (void)fclose(fp);
        return false;
    }

    if (buff[HNP_INDEX_0] == 0x7F && buff[HNP_INDEX_1] == 'E' && buff[HNP_INDEX_2] == 'L' && buff[HNP_INDEX_3] == 'F') {
        (void)fclose(fp);
        return true;
    }

    (void)fclose(fp);
    return false;
}

static int HnpInstallAddSignMap(const char* hnpSignKeyPrefix, const char *key, const char *value,
    HnpSignMapInfo *hnpSignMapInfos, int *count)
{
    int ret;
    int sum = *count;

    if (HnpELFFileCheck(value) == false) {
        return 0;
    }

    ret = sprintf_s(hnpSignMapInfos[sum].key, MAX_FILE_PATH_LEN, "%s!/%s", hnpSignKeyPrefix, key);
    if (ret < 0) {
        HNP_LOGE("add sign map sprintf unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    ret = strcpy_s(hnpSignMapInfos[sum].value, MAX_FILE_PATH_LEN, value);
    if (ret != EOK) {
        HNP_LOGE("add sign map strcpy[%{public}s] unsuccess.", value);
        return HNP_ERRNO_BASE_COPY_FAILED;
    }

    *count  = sum + 1;
    return 0;
}

int HnpFileCountGet(const char *path, int *count)
{
    int sum = 0;

    unzFile zipFile = unzOpen(path);
    if (zipFile == NULL) {
        HNP_LOGE("unzip open hnp:%{public}s unsuccess!", path);
        return HNP_ERRNO_BASE_UNZIP_OPEN_FAILED;
    }

    int ret = unzGoToFirstFile(zipFile);
    while (ret == UNZ_OK) {
        sum++;
        ret = unzGetCurrentFileInfo(zipFile, NULL, NULL, 0, NULL, 0, NULL, 0);
        if (ret != UNZ_OK) {
            HNP_LOGE("unzip get zip:%{public}s info unsuccess!", path);
            unzClose(zipFile);
            return HNP_ERRNO_BASE_UNZIP_GET_INFO_FAILED;
        }

        ret = unzGoToNextFile(zipFile);
    }

    unzClose(zipFile);
    *count += sum;
    return 0;
}

int HnpUnZip(const char *inputFile, const char *outputDir, const char *hnpSignKeyPrefix,
    HnpSignMapInfo *hnpSignMapInfos, int *count)
{
    unzFile zipFile;
    int result;
    char fileName[MAX_FILE_PATH_LEN];
    unz_file_info fileInfo;
    char filePath[MAX_FILE_PATH_LEN];

    HNP_LOGI("HnpUnZip zip=%{public}s, output=%{public}s", inputFile, outputDir);

    zipFile = unzOpen(inputFile);
    if (zipFile == NULL) {
        HNP_LOGE("unzip open hnp:%{public}s unsuccess!", inputFile);
        return HNP_ERRNO_BASE_UNZIP_OPEN_FAILED;
    }

    result = unzGoToFirstFile(zipFile);
    while (result == UNZ_OK) {
        result = unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), NULL, 0, NULL, 0);
        if (result != UNZ_OK) {
            HNP_LOGE("unzip get zip:%{public}s info unsuccess!", inputFile);
            unzClose(zipFile);
            return HNP_ERRNO_BASE_UNZIP_GET_INFO_FAILED;
        }
        char *slash = strchr(fileName, '/');
        if (slash != NULL) {
            slash++;
        } else {
            slash = fileName;
        }

        result = sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", outputDir, slash);
        if (result < 0) {
            HNP_LOGE("sprintf unsuccess.");
            unzClose(zipFile);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        result = HnpUnZipForFile(filePath, zipFile, fileInfo);
        if (result != 0) {
            HNP_LOGE("unzip for file:%{public}s unsuccess", filePath);
            unzClose(zipFile);
            return result;
        }
        result = HnpInstallAddSignMap(hnpSignKeyPrefix, fileName, filePath, hnpSignMapInfos, count);
        if (result != 0) {
            unzClose(zipFile);
            return result;
        }
        result = unzGoToNextFile(zipFile);
    }

    unzClose(zipFile);
    return 0;
}

int HnpCfgGetFromZip(const char *inputFile, HnpCfgInfo *hnpCfg)
{
    char fileName[MAX_FILE_PATH_LEN];
    unz_file_info fileInfo;
    char *cfgStream = NULL;

    unzFile zipFile = unzOpen(inputFile);
    if (zipFile == NULL) {
        HNP_LOGE("unzip open hnp:%{public}s unsuccess!", inputFile);
        return HNP_ERRNO_BASE_UNZIP_OPEN_FAILED;
    }

    int ret = unzGoToFirstFile(zipFile);
    while (ret == UNZ_OK) {
        ret = unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), NULL, 0, NULL, 0);
        if (ret != UNZ_OK) {
            HNP_LOGE("unzip get zip:%{public}s info unsuccess!", inputFile);
            unzClose(zipFile);
            return HNP_ERRNO_BASE_UNZIP_GET_INFO_FAILED;
        }
        char *fileNameTmp = strrchr(fileName, DIR_SPLIT_SYMBOL);
        if (fileNameTmp == NULL) {
            fileNameTmp = fileName;
        } else {
            fileNameTmp++;
        }
        if (strcmp(fileNameTmp, HNP_CFG_FILE_NAME) != 0) {
            ret = unzGoToNextFile(zipFile);
            continue;
        }

        unzOpenCurrentFile(zipFile);
        cfgStream = malloc(fileInfo.uncompressed_size);
        if (cfgStream == NULL) {
            HNP_LOGE("malloc unsuccess. size=%{public}lu, errno=%{public}d", fileInfo.uncompressed_size, errno);
            unzClose(zipFile);
            return HNP_ERRNO_NOMEM;
        }
        uLong readSize = unzReadCurrentFile(zipFile, cfgStream, fileInfo.uncompressed_size);
        if (readSize != fileInfo.uncompressed_size) {
            free(cfgStream);
            unzClose(zipFile);
            HNP_LOGE("unzip read zip:%{public}s info size[%{public}lu]=>[%{public}lu] error!", inputFile,
                fileInfo.uncompressed_size, readSize);
            return HNP_ERRNO_BASE_FILE_READ_FAILED;
        }
        break;
    }
    unzClose(zipFile);
    ret = HnpCfgGetFromSteam(cfgStream, hnpCfg);
    free(cfgStream);
    return ret;
}

#ifdef __cplusplus
}
#endif
