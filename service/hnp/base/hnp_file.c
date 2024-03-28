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

#ifdef __cplusplus
}
#endif