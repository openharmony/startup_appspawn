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

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_adapter.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "securec.h"

static int MakeDirRecursive(const char *path, mode_t mode)
{
    if (path == NULL || *path == '\0') {
        return -1;
    }

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
        if (len < 0) {
            break;
        }
        if (memcpy_s(buffer, PATH_MAX, path, p - path - 1) != 0) {
            return -1;
        }
        int ret = mkdir(buffer, mode);
        if (ret == -1 && errno != EEXIST) {
            return errno;
        }
        curPos = strchr(p, slash);
    }
    if (mkdir(path, mode) == -1 && errno != EEXIST) {
        return errno;
    }
    return 0;
}

APPSPAWN_STATIC int GetCgroupPath(const AppSpawnAppInfo *appInfo, char *buffer, uint32_t buffLen)
{
    const int userId = appInfo->uid / 200000;  // 200000 uid base
#ifdef APPSPAWN_TEST
    int ret = snprintf_s(buffer, buffLen, buffLen - 1, SOCKET_DIR "/dev/pids/testpids/%d/%s/%d/",
        userId, appInfo->name, appInfo->pid);
#else
    int ret = snprintf_s(buffer, buffLen, buffLen - 1, "/dev/pids/%d/%s/app_%d/", userId, appInfo->name, appInfo->pid);
#endif
    APPSPAWN_CHECK(ret > 0, return ret, "Failed to snprintf_s errno: %{public}d", errno);
    APPSPAWN_LOGV("Cgroup path %{public}s ", buffer);
    return 0;
}

static int WriteToFile(const char *path, int truncated, pid_t pids[], uint32_t count)
{
    char pidName[32] = {0}; // 32 max len
    int fd = open(path, O_RDWR | (truncated ? O_TRUNC : O_APPEND));
    APPSPAWN_CHECK(fd >= 0, return -1,
        "Failed to open file errno: %{public}d path: %{public}s", errno, path);
    int ret = 0;
    for (uint32_t i = 0; i < count; i++) {
        APPSPAWN_LOGV(" WriteToFile pid %{public}d ", pids[i]);
        if (pids[i] == 0) {
            continue;
        }
        ret = snprintf_s(pidName, sizeof(pidName), sizeof(pidName) - 1, "%d\n", pids[i]);
        APPSPAWN_CHECK(ret > 0, break, "Failed to snprintf_s errno: %{public}d", errno);
        ret = write(fd, pidName, strlen(pidName));
        APPSPAWN_CHECK(ret > 0, break,
            "Failed to write file errno: %{public}d path: %{public}s %{public}s %{public}d", errno, path, pidName, ret);
        ret = 0;
    }
    close(fd);
    return ret;
}

static void KillProcessesByCGroup(const char *path, const AppSpawnContentExt *content, const AppSpawnAppInfo *appInfo)
{
    FILE *file = fopen(path, "r");
    APPSPAWN_CHECK(file != NULL, return, "Open file fail %{public}s errno: %{public}d", path, errno);
    pid_t pid = 0;
    while (fscanf_s(file, "%d\n", &pid) == 1 && pid > 0) {
        APPSPAWN_LOGV(" KillProcessesByCGroup pid %{public}d ", pid);
        if (pid == appInfo->pid) {
            continue;
        }
        AppSpawnAppInfo *tmp = GetAppInfo(pid);
        if (tmp != NULL) {
            APPSPAWN_LOGI("Got app %{public}s in same group for pid %{public}d.", tmp->name, pid);
            continue;
        }
        APPSPAWN_LOGI("Kill app pid %{public}d now ...", pid);
        kill(pid, SIGKILL);
    }
    (void)fclose(file);
}

int ProcessAppDied(const AppSpawnContentExt *content, const AppSpawnAppInfo *appInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(appInfo != NULL, return -1);
    char path[PATH_MAX] = {};
    APPSPAWN_LOGV("ProcessAppDied %{public}d %{public}d to cgroup ", appInfo->pid, appInfo->uid);
    int ret = GetCgroupPath(appInfo, path, sizeof(path));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get real path errno: %d", errno);
    ret = strcat_s(path, sizeof(path), "cgroup.procs");
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to strcat_s errno: %{public}d", errno);
    KillProcessesByCGroup(path, content, appInfo);
    return ret;
}

int ProcessAppAdd(const AppSpawnContentExt *content, const AppSpawnAppInfo *appInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(appInfo != NULL, return -1);
    if (content->content.isNweb) {
        return 0;
    }
    char path[PATH_MAX] = {};
    APPSPAWN_LOGV("ProcessAppAdd %{public}d %{public}d to cgroup ", appInfo->pid, appInfo->uid);
    int ret = GetCgroupPath(appInfo, path, sizeof(path));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get real path errno: %d", errno);
    (void)MakeDirRecursive(path, 0755);  // 0755 default mode
    ret = strcat_s(path, sizeof(path), "cgroup.procs");
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to strcat_s errno: %{public}d", errno);
    ret = WriteToFile(path, 0, (pid_t *)&appInfo->pid, 1);
    APPSPAWN_CHECK(ret == 0, return ret, "write pid to cgroup.procs fail %{public}s", path);
    APPSPAWN_LOGV("Add app %{public}d to cgroup %{public}s success", appInfo->pid, path);
    return 0;
}
