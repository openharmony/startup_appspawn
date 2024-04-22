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

// 向zip压缩包中添加文件
static int ZipAddFile(const char* file, int offset, zipFile zf)
{
    int err;
    char buf[1024];
    int len;
    FILE *f;

    err = zipOpenNewFileInZip3(zf, file + offset, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION,
        0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0);
    if (err != ZIP_OK) {
        HNP_LOGE("open new file[%s] in zip unsuccess ", file);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }
    f = fopen(file, "rb");
    if (f == NULL) {
        HNP_LOGE("open file[%s] unsuccess ", file);
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
    DWORD fileAttr = GetFileAttributes(fullPath);
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        HNP_LOGE("get file[%s] attr unsuccess.", fullPath);
        return HNP_ERRNO_GET_FILE_ATTR_FAILED;
    }
    *isDir = (int)(fileAttr & FILE_ATTRIBUTE_DIRECTORY);
#else
    *isDir = (int)(entry->d_type == DT_DIR);
#endif

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
        HNP_LOGE("open dir=%s unsuccess ", sourcePath);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (sprintf_s(fullPath, MAX_FILE_PATH_LEN, "%s%c%s", sourcePath, DIR_SPLIT_SYMBOL, entry->d_name) < 0) {
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

            if (zipOpenNewFileInZip3(zf, fullPath + offset, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED,
                Z_BEST_COMPRESSION, 0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0) != ZIP_OK) {
                HNP_LOGE("open new file[%s] in zip unsuccess ", fullPath);
                closedir(dir);
                return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
            }
            zipCloseFileInZip(zf);
            if ((ret = ZipAddDir(fullPath, offset, zf)) != 0) {
                HNP_LOGE("zip add dir[%s] unsuccess ", fullPath);
                closedir(dir);
                return ret;
            }
        } else {
            if ((ret = ZipAddFile(fullPath, offset, zf)) != 0) {
                HNP_LOGE("zip add file[%s] unsuccess ", fullPath);
                closedir(dir);
                return ret;
            }
        }
    }
    closedir(dir);

    return 0;
}

static int ZipDir(const char *sourcePath, int offset, const char *zipPath)
{
    int ret;

    zipFile zf = zipOpen(zipPath, APPEND_STATUS_CREATE);
    if (zf == NULL) {
        HNP_LOGE("open zip=%s unsuccess ", zipPath);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }

    // 将外层文件夹信息保存到zip文件中
    ret = zipOpenNewFileInZip3(zf, sourcePath + offset, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION,
        0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0);
    if (ret != ZIP_OK) {
        HNP_LOGE("open new file[%s] in zip unsuccess ", sourcePath + offset);
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

    HNP_LOGI("HnpZip dir=%s, output=%s ", inputDir, outputFile);

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

    zf = zipOpen(zipfile, APPEND_STATUS_ADDINZIP);
    if (zf == NULL) {
        HNP_LOGE("open zip=%s unsuccess ", zipfile);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }

    // 将外层文件夹信息保存到zip文件中
    ret = zipOpenNewFileInZip3(zf, filename, NULL, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION,
        0, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0);
    if (ret != ZIP_OK) {
        HNP_LOGE("open new file[%s] in zip unsuccess ", filename);
        zipClose(zf, NULL);
        return HNP_ERRNO_BASE_CREATE_ZIP_FAILED;
    }
    zipWriteInFileInZip(zf, buff, size);
    zipCloseFileInZip(zf);
    zipClose(zf, NULL);

    return 0;
}

static int HnpUnZipForFile(const char *fileName, const char *outputDir, unzFile zipFile)
{
#ifdef _WIN32
    return 0;
#else
    int ret;
    char filePath[MAX_FILE_PATH_LEN];

    ret = sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", outputDir, fileName);
    if (ret < 0) {
        HNP_LOGE("sprintf unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 如果解压缩的是目录 */
    if (filePath[strlen(filePath) - 1] == '/') {
        mkdir(filePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    } else {
        FILE *outFile = fopen(filePath, "wb");
        if (outFile == NULL) {
            HNP_LOGE("unzip open file:%s unsuccess!", filePath);
            return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
        }
        unzOpenCurrentFile(zipFile);
        int readSize = 0;
        do {
            char buffer[BUFFER_SIZE];
            readSize = unzReadCurrentFile(zipFile, buffer, sizeof(buffer));
            if (readSize < 0) {
                HNP_LOGE("unzip read zip:%s file unsuccess", zipFile);
                fclose(outFile);
                unzCloseCurrentFile(zipFile);
                return HNP_ERRNO_BASE_UNZIP_READ_FAILED;
            }

            fwrite(buffer, readSize, sizeof(char), outFile);
        } while (readSize > 0);

        fclose(outFile);
        unzCloseCurrentFile(zipFile);
    }
    return 0;
#endif
}

int HnpUnZip(const char *inputFile, const char *outputDir)
{
    unzFile zipFile;
    int result;
    char fileName[MAX_FILE_PATH_LEN];
    unz_file_info fileInfo;

    HNP_LOGI("HnpUnZip zip=%s, output=%s", inputFile, outputDir);

    zipFile = unzOpen(inputFile);
    if (zipFile == NULL) {
        HNP_LOGE("unzip open hnp:%s unsuccess!", inputFile);
        return HNP_ERRNO_BASE_UNZIP_OPEN_FAILED;
    }

    result = unzGoToFirstFile(zipFile);
    while (result == UNZ_OK) {
        result = unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), NULL, 0, NULL, 0);
        if (result != UNZ_OK) {
            HNP_LOGE("unzip get zip:%s info unsuccess!", inputFile);
            unzClose(zipFile);
            return HNP_ERRNO_BASE_UNZIP_GET_INFO_FAILED;
        }
        char *slash = strchr(fileName, '/');
        if (slash != NULL) {
            slash++;
        } else {
            slash = fileName;
        }

        result = HnpUnZipForFile(slash, outputDir, zipFile);
        if (result != 0) {
            HNP_LOGE("unzip for file:%s unsuccess", slash);
            unzClose(zipFile);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        result = unzGoToNextFile(zipFile);
    }

    unzClose(zipFile);
    return 0;
}

int HnpCfgGetFromZip(const char *inputFile, HnpCfgInfo *hnpCfg)
{
    char fileName[MAX_FILE_PATH_LEN];
    char *fileNameTmp;
    unz_file_info fileInfo;
    char *cfgStream = NULL;

    unzFile zipFile = unzOpen(inputFile);
    if (zipFile == NULL) {
        HNP_LOGE("unzip open hnp:%s unsuccess!", inputFile);
        return HNP_ERRNO_BASE_UNZIP_OPEN_FAILED;
    }

    int ret = unzGoToFirstFile(zipFile);
    while (ret == UNZ_OK) {
        ret = unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName), NULL, 0, NULL, 0);
        if (ret != UNZ_OK) {
            HNP_LOGE("unzip get zip:%s info unsuccess!", inputFile);
            unzClose(zipFile);
            return HNP_ERRNO_BASE_UNZIP_GET_INFO_FAILED;
        }
        fileNameTmp = strrchr(fileName, DIR_SPLIT_SYMBOL);
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
            HNP_LOGE("malloc unsuccess. size=%d, errno=%d", fileInfo.uncompressed_size, errno);
            unzClose(zipFile);
            return HNP_ERRNO_NOMEM;
        }
        uLong readSize = unzReadCurrentFile(zipFile, cfgStream, fileInfo.uncompressed_size);
        if (readSize != fileInfo.uncompressed_size) {
            free(cfgStream);
            unzClose(zipFile);
            HNP_LOGE("unzip read zip:%s info size[%lu]=>[%lu] error!", inputFile, fileInfo.uncompressed_size, readSize);
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
