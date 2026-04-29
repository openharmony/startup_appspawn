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

#include <cerrno>
#include <csignal>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include "securec.h"

extern "C" {
// ===== Real function declarations for --wrap =====
extern DIR *__real_opendir(const char *name);
extern FILE *__real_fopen(const char *path, const char *mode);
extern int __real_open(const char *path, int flags, ...);
extern ssize_t __real_write(int fd, const void *buf, size_t count);
extern int __real_rmdir(const char *path);
extern int __real_GetParameter(const char *key, const char *def, char *value, uint32_t len);
extern int __real_SetParameter(const char *key, const char *value);

// ===== Mock state =====

static const size_t MOCK_KILLED_PIDS_MAX = 128;
static const size_t MOCK_RMDIR_PATHS_MAX = 64;

char g_mockTestRoot[PATH_MAX] = "";

pid_t g_mockKilledPids[MOCK_KILLED_PIDS_MAX] = {};
int g_mockKilledCount = 0;
pid_t g_mockKillFailPid = -1;

char g_mockRmdirPaths[MOCK_RMDIR_PATHS_MAX][PATH_MAX] = {};
int g_mockRmdirCount = 0;
int g_mockRmdirFailIndex = -1;

char g_mockGracefulStopValue[8] = "0";
int g_mockGetParamFail = 0;

char g_mockSetParamKey[256] = "";
char g_mockSetParamValue[64] = "";
int g_mockSetParamCalled = 0;

int g_mockOpenForceFail = 0;
int g_mockWriteForceFail = 0;

// ===== Helpers =====

static const char *CGROUP_ROOT = "/dev/pids/";
static const size_t CGROUP_ROOT_LEN = 10;

static int IsCgroupPath(const char *path)
{
    if (path == nullptr) {
        return 0;
    }
    return strncmp(path, CGROUP_ROOT, CGROUP_ROOT_LEN) == 0 && g_mockTestRoot[0] != '\0';
}

static void RedirectPath(const char *src, char *dst, size_t dstLen)
{
    int ret = snprintf_s(dst, dstLen, dstLen - 1, "%s/%s", g_mockTestRoot, src + CGROUP_ROOT_LEN);
    if (ret <= 0) {
        dst[0] = '\0';
    }
}

// ===== Mock implementations =====

DIR *__wrap_opendir(const char *name)
{
    if (IsCgroupPath(name)) {
        char buf[PATH_MAX] = {};
        RedirectPath(name, buf, sizeof(buf));
        return __real_opendir(buf);
    }
    return __real_opendir(name);
}

FILE *__wrap_fopen(const char *path, const char *mode)
{
    if (IsCgroupPath(path)) {
        char buf[PATH_MAX] = {};
        RedirectPath(path, buf, sizeof(buf));
        return __real_fopen(buf, mode);
    }
    return __real_fopen(path, mode);
}

int __wrap_open(const char *path, int flags, ...)
{
    if (g_mockOpenForceFail) {
        errno = ENOENT;
        return -1;
    }
    char buf[PATH_MAX] = {};
    const char *realPath = path;
    if (IsCgroupPath(path)) {
        RedirectPath(path, buf, sizeof(buf));
        realPath = buf;
    }
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode_t mode = va_arg(args, int);
        va_end(args);
        return __real_open(realPath, flags, mode);
    }
    return __real_open(realPath, flags);
}

ssize_t __wrap_write(int fd, const void *buf, size_t count)
{
    if (g_mockWriteForceFail) {
        errno = EIO;
        return -1;
    }
    return __real_write(fd, buf, count);
}

int __wrap_kill(pid_t pid, int sig)
{
    if (static_cast<size_t>(g_mockKilledCount) < MOCK_KILLED_PIDS_MAX) {
        g_mockKilledPids[g_mockKilledCount++] = pid;
    }
    if (g_mockKillFailPid == pid) {
        errno = ESRCH;
        return -1;
    }
    return 0;
}

int __wrap_rmdir(const char *path)
{
    if (static_cast<size_t>(g_mockRmdirCount) < MOCK_RMDIR_PATHS_MAX) {
        int ret = strncpy_s(g_mockRmdirPaths[g_mockRmdirCount], PATH_MAX, path, PATH_MAX - 1);
        if (ret != EOK) {
            g_mockRmdirPaths[g_mockRmdirCount][0] = '\0';
        }
    }
    int idx = g_mockRmdirCount++;
    if (g_mockRmdirFailIndex >= 0 && idx == g_mockRmdirFailIndex) {
        errno = ENOTEMPTY;
        return -1;
    }
    if (IsCgroupPath(path)) {
        char buf[PATH_MAX] = {};
        RedirectPath(path, buf, sizeof(buf));
        return __real_rmdir(buf);
    }
    return __real_rmdir(path);
}

int __wrap_GetParameter(const char *key, const char *def, char *value, uint32_t len)
{
    if (g_mockGetParamFail) {
        return -1;
    }
    if (strcmp(key, "startup.appspawn.graceful_stop") == 0) {
        size_t valLen = strlen(g_mockGracefulStopValue);
        if (valLen >= len) {
            valLen = len - 1;
        }
        int ret = strncpy_s(value, len, g_mockGracefulStopValue, valLen);
        if (ret != EOK) {
            return -1;
        }
        value[valLen] = '\0';
        return static_cast<int>(valLen);
    }
    return __real_GetParameter(key, def, value, len);
}

int __wrap_SetParameter(const char *key, const char *value)
{
    int ret = strncpy_s(g_mockSetParamKey, sizeof(g_mockSetParamKey), key, sizeof(g_mockSetParamKey) - 1);
    if (ret != EOK) {
        g_mockSetParamKey[0] = '\0';
    }
    ret = strncpy_s(g_mockSetParamValue, sizeof(g_mockSetParamValue), value, sizeof(g_mockSetParamValue) - 1);
    if (ret != EOK) {
        g_mockSetParamValue[0] = '\0';
    }
    g_mockSetParamCalled = 1;
    return 0;
}

// ===== Mock reset =====

void MockCgroupReset(void)
{
    g_mockTestRoot[0] = '\0';
    g_mockKilledCount = 0;
    g_mockKillFailPid = -1;
    g_mockRmdirCount = 0;
    g_mockRmdirFailIndex = -1;
    g_mockGetParamFail = 0;
    g_mockSetParamCalled = 0;
    g_mockSetParamKey[0] = '\0';
    g_mockSetParamValue[0] = '\0';
    int ret = strcpy_s(g_mockGracefulStopValue, sizeof(g_mockGracefulStopValue), "0");
    if (ret != EOK) {
        g_mockGracefulStopValue[0] = '0';
        g_mockGracefulStopValue[1] = '\0';
    }
    g_mockOpenForceFail = 0;
    g_mockWriteForceFail = 0;
}

} // extern "C"
