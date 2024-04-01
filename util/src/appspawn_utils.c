/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "appspawn_utils.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_hook.h"
#include "cJSON.h"
#include "config_policy_utils.h"
#include "parameter.h"
#include "securec.h"

int MakeDirRec(const char *path, mode_t mode, int lastPath)
{
    if (path == NULL || *path == '\0') {
        return -1;
    }
    APPSPAWN_CHECK(path != NULL && *path != '\0', return -1, "Invalid path to create");
    char buffer[PATH_MAX] = {0};
    const char slash = '/';
    const char *p = path;
    char *curPos = strchr(path, slash);
    while (curPos != NULL) {
        int len = curPos - p;
        p = curPos + 1;
        if (len == 0) {
            curPos = strchr(p, slash);
            continue;
        }
        int ret = memcpy_s(buffer, PATH_MAX, path, p - path - 1);
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to copy path");
        ret = mkdir(buffer, mode);
        if (ret == -1 && errno != EEXIST) {
            return errno;
        }
        curPos = strchr(p, slash);
    }
    if (lastPath) {
        if (mkdir(path, mode) == -1 && errno != EEXIST) {
            return errno;
        }
    }
    return 0;
}

static void CheckDirRecursive(const char *path)
{
    char buffer[PATH_MAX] = {0};
    const char slash = '/';
    const char *p = path;
    char *curPos = strchr(path, slash);
    while (curPos != NULL) {
        int len = curPos - p;
        p = curPos + 1;
        if (len == 0) {
            curPos = strchr(p, slash);
            continue;
        }
        int ret = memcpy_s(buffer, PATH_MAX, path, p - path - 1);
        APPSPAWN_CHECK(ret == 0, return, "Failed to copy path");
        ret = access(buffer, F_OK);
        APPSPAWN_CHECK(ret == 0, return, "Dir not exit %{public}s errno: %{public}d", buffer, errno);
        curPos = strchr(p, slash);
    }
    int ret = access(path, F_OK);
    APPSPAWN_CHECK(ret == 0, return, "Dir not exit %{public}s errno: %{public}d", buffer, errno);
    return;
}

int SandboxMountPath(const MountArg *arg)
{
    APPSPAWN_CHECK(arg != NULL && arg->originPath != NULL && arg->destinationPath != NULL,
        return APPSPAWN_ARG_INVALID, "Invalid arg ");
    int ret = mount(arg->originPath, arg->destinationPath, arg->fsType, arg->mountFlags, arg->options);
    if (ret != 0) {
        if (arg->originPath != NULL && strstr(arg->originPath, "/data/app/el2/") != NULL) {
            CheckDirRecursive(arg->originPath);
        }
        APPSPAWN_LOGW("errno is: %{public}d, bind mount %{public}s => %{public}s",
            errno, arg->originPath, arg->destinationPath);
        return errno;
    }
    ret = mount(NULL, arg->destinationPath, NULL, arg->mountSharedFlag, NULL);
    if (ret != 0) {
        APPSPAWN_LOGW("errno is: %{public}d, bind mount %{public}s => %{public}s",
            errno, arg->originPath, arg->destinationPath);
        return errno;
    }
    return 0;
}

static void TrimTail(char *buffer, uint32_t maxLen)
{
    int32_t index = maxLen - 1;
    while (index > 0) {
        if (isspace(buffer[index])) {
            buffer[index] = '\0';
            index--;
            continue;
        }
        break;
    }
}

int32_t StringSplit(const char *str, const char *separator, void *context, SplitStringHandle handle)
{
    APPSPAWN_CHECK(str != NULL && handle != NULL && separator != NULL, return APPSPAWN_ARG_INVALID, "Invalid arg ");

    int ret = 0;
    char *tmp = (char *)str;
    char buffer[PATH_MAX] = {0};
    uint32_t len = strlen(separator);
    uint32_t index = 0;
    while ((*tmp != '\0') && (index < (uint32_t)sizeof(buffer))) {
        if (index == 0 && isspace(*tmp)) {
            tmp++;
            continue;
        }
        if (strncmp(tmp, separator, len) != 0) {
            buffer[index++] = *tmp;
            tmp++;
            continue;
        }
        tmp += len;
        buffer[index] = '\0';
        TrimTail(buffer, index);
        index = 0;

        int result = handle(buffer, context);
        if (result != 0) {
            ret = result;
        }
    }
    if (index > 0) {
        buffer[index] = '\0';
        TrimTail(buffer, index);
        index = 0;
        int result = handle(buffer, context);
        if (result != 0) {
            ret = result;
        }
    }
    return ret;
}

char *GetLastStr(const char *str, const char *dst)
{
    char *end = (char *)str + strlen(str);
    size_t len = strlen(dst);
    while (end != str) {
        if (isspace(*end)) { // clear space
            *end = '\0';
            end--;
            continue;
        }
        if (strncmp(end, dst, len) == 0) {
            return end;
        }
        end--;
    }
    return NULL;
}

static char *ReadFile(const char *fileName)
{
    char *buffer = NULL;
    FILE *fd = NULL;
    do {
        struct stat fileStat;
        if (stat(fileName, &fileStat) != 0 ||
            fileStat.st_size <= 0 || fileStat.st_size > MAX_JSON_FILE_LEN) {
            return NULL;
        }
        APPSPAWN_LOGI("LoadAppSandboxConfig %{public}s size %{public}u", fileName, (uint32_t)fileStat.st_size);
        fd = fopen(fileName, "r");
        APPSPAWN_CHECK(fd != NULL, break, "Failed to open file  %{public}s", fileName);

        buffer = (char*)malloc((size_t)(fileStat.st_size + 1));
        APPSPAWN_CHECK(buffer != NULL, break, "Failed to alloc mem %{public}s", fileName);

        int ret = fread(buffer, fileStat.st_size, 1, fd);
        APPSPAWN_CHECK(ret == 1, break, "Failed to read %{public}s to buffer", fileName);
        buffer[fileStat.st_size] = '\0';
        (void)fclose(fd);
        return buffer;
    } while (0);

    if (fd != NULL) {
        (void)fclose(fd);
        fd = NULL;
    }
    if (buffer != NULL) {
        free(buffer);
    }
    return NULL;
}

cJSON *GetJsonObjFromFile(const char *jsonPath)
{
    APPSPAWN_CHECK_ONLY_EXPER(jsonPath != NULL && *jsonPath != '\0', NULL);
    char *buffer = ReadFile(jsonPath);
    APPSPAWN_CHECK_ONLY_EXPER(buffer != NULL, NULL);
    return cJSON_Parse(buffer);
}

int ParseJsonConfig(const char *basePath, const char *fileName, ParseConfig parseConfig, ParseJsonContext *context)
{
    // load sandbox config
    char path[PATH_MAX] = {};
    CfgFiles *files = GetCfgFiles(basePath);
    if (files == NULL) {
        return APPSPAWN_SANDBOX_NONE;
    }
    int ret = 0;
    for (int i = 0; i < MAX_CFG_POLICY_DIRS_CNT; ++i) {
        if (files->paths[i] == NULL) {
            continue;
        }
        int len = snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s%s", files->paths[i], fileName);
        APPSPAWN_CHECK(len > 0 && (size_t)len < sizeof(path), ret = APPSPAWN_SANDBOX_INVALID;
            continue, "Failed to format sandbox config file name %{public}s %{public}s", files->paths[i], fileName);
        cJSON *root = GetJsonObjFromFile(path);
        APPSPAWN_CHECK(root != NULL, ret = APPSPAWN_SANDBOX_INVALID;
            continue, "Failed to load app data sandbox config %{public}s", path);
        int rc = parseConfig(root, context);
        if (rc != 0) {
            ret = rc;
        }
        cJSON_Delete(root);
    }
    FreeCfgFiles(files);
    return ret;
}

static FILE *g_dumpToStream = NULL;
void SetDumpToStream(FILE *stream)
{
    g_dumpToStream = stream;
}
