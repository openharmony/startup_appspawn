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
#include <dirent.h>
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
#include "json_utils.h"
#include "parameter.h"
#include "securec.h"

static const AppSpawnCommonEnv COMMON_ENV[] = {
    {"HNP_PRIVATE_HOME", "/data/app", true},
    {"HNP_PUBLIC_HOME", "/data/service/hnp", true},
    {"PATH", "${HNP_PRIVATE_HOME}/bin:${HNP_PUBLIC_HOME}/bin:${PATH}", true},
    {"HOME", "/storage/Users/currentUser", false},
    {"TMPDIR", "/data/storage/el2/base/cache", false},
    {"SHELL", "/bin/sh", false},
    {"PWD", "/storage/Users/currentUser", false}
};

int ConvertEnvValue(const char *srcEnv, char *dstEnv, int len)
{
    char *tmpEnv = NULL;
    char *ptr;
    char *tmpPtr1;
    char *tmpPtr2;
    char *envGet;

    int srcLen = strlen(srcEnv) + 1;
    tmpEnv = malloc(srcLen);
    APPSPAWN_CHECK(tmpEnv != NULL, return -1, "malloc size=%{public}d fail", srcLen);

    int ret = memcpy_s(tmpEnv, srcLen, srcEnv, srcLen);
    APPSPAWN_CHECK(ret == EOK, {free(tmpEnv); return -1;}, "Failed to copy env value");

    ptr = tmpEnv;
    dstEnv[0] = 0;
    while (((tmpPtr1 = strchr(ptr, '$')) != NULL) && (*(tmpPtr1 + 1) == '{') &&
        ((tmpPtr2 = strchr(tmpPtr1, '}')) != NULL)) {
        *tmpPtr1 = 0;
        ret = strcat_s(dstEnv, len, ptr);
        APPSPAWN_CHECK(ret == 0, {free(tmpEnv); return -1;}, "Failed to strcat env value");
        *tmpPtr2 = 0;
        tmpPtr1++;
        envGet = getenv(tmpPtr1 + 1);
        if (envGet != NULL) {
            ret = strcat_s(dstEnv, len, envGet);
            APPSPAWN_CHECK(ret == 0, {free(tmpEnv); return -1;}, "Failed to strcat env value");
        }
        ptr = tmpPtr2 + 1;
    }
    ret = strcat_s(dstEnv, len, ptr);
    APPSPAWN_CHECK(ret == 0, {free(tmpEnv); return -1;}, "Failed to strcat env value");
    free(tmpEnv);
    return 0;
}

void InitCommonEnv(void)
{
    uint32_t count = ARRAY_LENGTH(COMMON_ENV);
    int32_t ret;
    char envValue[MAX_ENV_VALUE_LEN];
    int developerMode = IsDeveloperModeOpen();

    for (uint32_t i = 0; i < count; i++) {
        if ((COMMON_ENV[i].developerModeEnable == true && developerMode == false)) {
            continue;
        }
        ret = ConvertEnvValue(COMMON_ENV[i].envValue, envValue, MAX_ENV_VALUE_LEN);
        APPSPAWN_CHECK(ret == 0, return, "Convert env value fail name=%{public}s, value=%{public}s",
            COMMON_ENV[i].envName, COMMON_ENV[i].envValue);
        ret = setenv(COMMON_ENV[i].envName, envValue, true);
        APPSPAWN_CHECK(ret == 0, return, "Set env fail name=%{public}s, value=%{public}s",
            COMMON_ENV[i].envName, envValue);
    }
}

uint64_t DiffTime(const struct timespec *startTime, const struct timespec *endTime)
{
    APPSPAWN_CHECK_ONLY_EXPER(startTime != NULL, return 0);
    APPSPAWN_CHECK_ONLY_EXPER(endTime != NULL, return 0);

    uint64_t diff = (uint64_t)((endTime->tv_sec - startTime->tv_sec) * 1000000);  // 1000000 s-us
    if (endTime->tv_nsec > startTime->tv_nsec) {
        diff += (endTime->tv_nsec - startTime->tv_nsec) / 1000;  // 1000 ns - us
    } else {
        diff -= (startTime->tv_nsec - endTime->tv_nsec) / 1000;  // 1000 ns - us
    }
    return diff;
}

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
    APPSPAWN_CHECK_ONLY_EXPER(str != NULL, return NULL);
    APPSPAWN_CHECK_ONLY_EXPER(dst != NULL, return NULL);

    char *end = (char *)str + strlen(str);
    size_t len = strlen(dst);
    while (end != str) {
        if (isspace(*end)) {  // clear space
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

        buffer = (char *)malloc((size_t)(fileStat.st_size + 1));
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
    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    return json;
}

int ParseJsonConfig(const char *basePath, const char *fileName, ParseConfig parseConfig, ParseJsonContext *context)
{
    APPSPAWN_CHECK_ONLY_EXPER(basePath != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_CHECK_ONLY_EXPER(fileName != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_CHECK_ONLY_EXPER(parseConfig != NULL, return APPSPAWN_ARG_INVALID);

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

void DumpCurrentDir(char *buffer, uint32_t bufferLen, const char *dirPath)
{
    APPSPAWN_CHECK_ONLY_EXPER(buffer != NULL, return);
    APPSPAWN_CHECK_ONLY_EXPER(dirPath != NULL, return);
    APPSPAWN_CHECK_ONLY_EXPER(bufferLen > 0 , return);

    char tmp[32] = {0};  // 32 max
    int ret = GetParameter("startup.appspawn.cold.boot", "", tmp, sizeof(tmp));
    if (ret <= 0 || strcmp(tmp, "1") != 0) {
        return;
    }

    struct stat st = {};
    if (stat(dirPath, &st) == 0 && S_ISREG(st.st_mode)) {
        APPSPAWN_LOGW("file %{public}s", dirPath);
        if (access(dirPath, F_OK) != 0) {
            APPSPAWN_LOGW("file %{public}s not exist", dirPath);
        }
        return;
    }

    DIR *pDir = opendir(dirPath);
    APPSPAWN_CHECK(pDir != NULL, return, "Read dir :%{public}s failed.%{public}d", dirPath, errno);

    struct dirent *dp;
    while ((dp = readdir(pDir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        if (dp->d_type == DT_DIR) {
            APPSPAWN_LOGW(" Current path %{public}s/%{public}s ", dirPath, dp->d_name);
            ret = snprintf_s(buffer, bufferLen, bufferLen - 1, "%s/%s", dirPath, dp->d_name);
            APPSPAWN_CHECK(ret > 0, break, "Failed to snprintf_s errno: %{public}d", errno);
            char *path = strdup(buffer);
            DumpCurrentDir(buffer, bufferLen, path);
            free(path);
        }
    }
    closedir(pDir);
    return;
}

static FILE *g_dumpToStream = NULL;
void SetDumpToStream(FILE *stream)
{
    g_dumpToStream = stream;
}

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wvarargs"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wvarargs"
#elif defined(_MSC_VER)
#    pragma warning(push)
#endif

void AppSpawnDump(const char *fmt, ...)
{
    if (g_dumpToStream == NULL) {
        return;
    }
    char format[128] = {0};  // 128 max buffer for format
    uint32_t size = strlen(fmt);
    int curr = 0;
    for (uint32_t index = 0; index < size; index++) {
        if (curr >= (int)sizeof(format)) {
            format[curr - 1] = '\0';
        }
        if (fmt[index] == '%' && (strncmp(&fmt[index + 1], "{public}", strlen("{public}")) == 0)) {
            format[curr++] = fmt[index];
            index += strlen("{public}");
            continue;
        }
        format[curr++] = fmt[index];
    }
    va_list vargs;
    va_start(vargs, format);
    (void)vfprintf(g_dumpToStream, format, vargs);
    va_end(vargs);
    (void)fflush(g_dumpToStream);
}

int IsDeveloperModeOpen()
{
    char tmp[32] = {0};  // 32 max
    int ret = GetParameter("const.security.developermode.state", "", tmp, sizeof(tmp));
    APPSPAWN_LOGV("IsDeveloperModeOpen ret %{public}d result: %{public}s", ret, tmp);
    int enabled = (ret > 0 && strcmp(tmp, "true") == 0);
    return enabled;
}

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

uint32_t GetSpawnTimeout(uint32_t def)
{
    uint32_t value = def;
    char data[32] = {};  // 32 length
    int ret = GetParameter("persist.appspawn.reqMgr.timeout", "0", data, sizeof(data));
    if (ret > 0 && strcmp(data, "0") != 0) {
        errno = 0;
        value = atoi(data);
        return (errno != 0) ? def : ((value < def) ? def : value);
    }
    return value;
}
