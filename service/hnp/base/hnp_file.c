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
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "hnp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

int GetFileSizeByHandle(FILE *file, int *size)
{
    int ret;
    int len;

    ret = fseek(file, 0, SEEK_END);
    if (ret != 0) {
        HNP_LOGE("fseek end unsuccess.");
        return HNP_ERRNO_BASE_FILE_SEEK_FAILED;
    }
    len = ftell(file);
    if (len < 0) {
        HNP_LOGE("ftell unsuccess. len=%d", len);
        return HNP_ERRNO_BASE_FILE_TELL_FAILED;
    }
    ret = fseek(file, 0, SEEK_SET);
    if (ret != 0) {
        HNP_LOGE("fseek set unsuccess. ");
        return HNP_ERRNO_BASE_FILE_SEEK_FAILED;
    }
    *size = len;
    return 0;
}

int ReadFileToStream(const char *filePath, char **stream, int *streamLen)
{
    int ret;
    FILE *file;
    int size = 0;
    char *streamTmp;

    file = fopen(filePath, "rb");
    if (file == NULL) {
        HNP_LOGE("open file[%s] unsuccess. ", filePath);
        return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
    }
    ret = GetFileSizeByHandle(file, &size);
    if (ret != 0) {
        HNP_LOGE("get file[%s] size unsuccess.", filePath);
        (void)fclose(file);
        return ret;
    }
    if (size == 0) {
        HNP_LOGE("get file[%s] size is null.", filePath);
        (void)fclose(file);
        return HNP_ERRNO_BASE_GET_FILE_LEN_NULL;
    }
    streamTmp = (char*)malloc(size);
    if (streamTmp == NULL) {
        HNP_LOGE("malloc unsuccess. size=%d", size);
        (void)fclose(file);
        return HNP_ERRNO_NOMEM;
    }
    ret = fread(streamTmp, sizeof(char), size, file);
    if (ret != size) {
        HNP_LOGE("fread unsuccess. ret=%d, size=%d", ret, size);
        (void)fclose(file);
        free(streamTmp);
        return HNP_ERRNO_BASE_FILE_READ_FAILED;
    }
    *stream = streamTmp;
    *streamLen = size;
    (void)fclose(file);
    return 0;
}

int ReadFileToStreamBySize(const char *filePath, char **stream, int readSize)
{
    int ret;
    FILE *file;
    char *streamTmp;

    file = fopen(filePath, "rb");
    if (file == NULL) {
        HNP_LOGE("open file[%s] unsuccess. ", filePath);
        return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
    }

    if (readSize <= 0) {
        (void)fclose(file);
        HNP_LOGE("read size(%d) is invalid.", readSize);
        return HNP_ERRNO_BASE_PARAMS_INVALID;
    }
    streamTmp = (char*)malloc(readSize);
    if (streamTmp == NULL) {
        HNP_LOGE("malloc unsuccess. size=%d", readSize);
        (void)fclose(file);
        return HNP_ERRNO_NOMEM;
    }
    ret = fread(streamTmp, sizeof(char), readSize, file);
    if (ret != readSize) {
        HNP_LOGE("fread unsuccess. ret=%d, size=%d", ret, readSize);
        (void)fclose(file);
        free(streamTmp);
        return HNP_ERRNO_BASE_FILE_READ_FAILED;
    }
    *stream = streamTmp;
    (void)fclose(file);
    return 0;
}

int HnpWriteInfoToFile(const char* filePath, char *buff, int len)
{
    FILE *fp = fopen(filePath, "w");
    if (fp == NULL) {
        HNP_LOGE("open file:%s unsuccess!", filePath);
        return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
    }
    int writeLen = fwrite(buff, sizeof(char), len, fp);
    if (writeLen != len) {
        HNP_LOGE("write file:%s unsuccess! len=%d, write=%d", filePath, len, writeLen);
        (void)fclose(fp);
        return HNP_ERRNO_BASE_FILE_WRITE_FAILED;
    }

    (void)fclose(fp);

    return 0;
}

int GetRealPath(char *srcPath, char *realPath)
{
    char dstTmpPath[PATH_MAX];

    if (srcPath == NULL || realPath == NULL) {
        return HNP_ERRNO_PARAM_INVALID;
    }
#ifdef _WIN32
    DWORD ret = GetFullPathName(srcPath, PATH_MAX, dstTmpPath, NULL);
#else
    char *ret = realpath(srcPath, dstTmpPath);
#endif
    if (ret == 0) {
        HNP_LOGE("realpath unsuccess. path=%s", srcPath);
        return HNP_ERRNO_BASE_REALPATHL_FAILED;
    }
    if (strlen(dstTmpPath) >= MAX_FILE_PATH_LEN) {
        HNP_LOGE("realpath over max path len. len=%d", strlen(dstTmpPath));
        return HNP_ERRNO_BASE_STRING_LEN_OVER_LIMIT;
    }
    if (strcpy_s(realPath, MAX_FILE_PATH_LEN, dstTmpPath) != EOK) {
        HNP_LOGE("strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }
    return 0;
}

int HnpDeleteFolder(const char *path)
{
    int ret = 0;
#ifdef _WIN32
    return ret;
#else
    DIR *dir = opendir(path);
    struct dirent *entry;
    struct stat statbuf;
    char filePath[MAX_FILE_PATH_LEN];

    if (dir == NULL) {
        HNP_LOGE("delete folder open dir=%s unsuccess ", path);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }

        ret = sprintf_s(filePath, MAX_FILE_PATH_LEN, "%s/%s", path, entry->d_name);
        if (ret < 0) {
            HNP_LOGE("delete folder sprintf path[%s], file[%s] unsuccess.", path, entry->d_name);
            closedir(dir);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        if (lstat(filePath, &statbuf) < 0) {
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            /* 如果是文件夹，递归删除 */
            ret = HnpDeleteFolder(filePath);
            if (ret != 0) {
                closedir(dir);
                return ret;
            }
        } else {
            ret = unlink(filePath);
            if (ret != 0) {
                closedir(dir);
                HNP_LOGE("delete file unsuccess.ret=%d, path=%s, errno=%d", ret, filePath, errno);
                return HNP_ERRNO_BASE_UNLINK_FAILED;
            }
        }
    }

    closedir(dir);
    ret = rmdir(path);
    if (ret != 0) {
        HNP_LOGE("rmdir path unsuccess.ret=%d, path=%s, errno=%d", ret, path, errno);
    }
    return ret;
#endif
}

int HnpCreateFolder(const char* path)
{
    int ret = 0;
#ifdef _WIN32
    return ret;
#else
    char *p = NULL;
    struct stat buffer;

    if (path == NULL) {
        HNP_LOGE("delete folder param path is null");
        return HNP_ERRNO_BASE_PARAMS_INVALID;
    }

    /* 如果目录存在，则返回 */
    if (stat(path, &buffer) == 0) {
        return 0;
    }

    p = strdup(path);
    if (p == NULL) {
        HNP_LOGE("delete folder strdup unsuccess, path=%s, errno=%d", path, errno);
        return HNP_ERRNO_BASE_STRDUP_FAILED;
    }

    for (char *q = p + 1; *q; q++) {
        if (*q != '/') {
            continue;
        }
        *q = '\0';
        if (stat(path, &buffer) != 0) {
            ret = mkdir(p, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if ((ret != 0) && (errno != EEXIST)) {
                HNP_LOGE("delete folder unsuccess, ret=%d, p=%s, errno:%d", ret, p, errno);
            }
        }
        *q = '/';
    }

    if (path[strlen(path) - 1] != '/') {
        ret = mkdir(p, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            HNP_LOGE("delete folder unsuccess, ret=%d, path=%s, errno:%d", ret, path, errno);
        }
    }
    free(p);

    if (errno == EEXIST) {
        ret = 0;
    }
    return ret;
#endif
}

#ifdef __cplusplus
}
#endif