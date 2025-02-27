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
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "securec.h"

APPSPAWN_STATIC int GetCgroupPath(const AppSpawnedProcessInfo *appInfo, char *buffer, uint32_t buffLen)
{
    const int userId = appInfo->uid / UID_BASE;
#ifdef APPSPAWN_TEST
    int ret = snprintf_s(buffer, buffLen, buffLen - 1, APPSPAWN_BASE_DIR "/dev/pids/testpids/%d/%s/%d/",
        userId, appInfo->name, appInfo->pid);
#else
    int ret = snprintf_s(buffer, buffLen, buffLen - 1, "/dev/pids/%d/%s/app_%d/", userId, appInfo->name, appInfo->pid);
#endif
    APPSPAWN_CHECK(ret > 0, return ret, "Failed to snprintf_s errno: %{public}d", errno);
    APPSPAWN_LOGV("Cgroup path %{public}s ", buffer);
    return 0;
}


APPSPAWN_STATIC int WriteToFile(const char *path, int truncated, pid_t pids[], uint32_t count)
{
    char pidName[32] = {0}; // 32 max len
    int fd = open(path, O_RDWR | (truncated ? O_TRUNC : O_APPEND));
    APPSPAWN_CHECK(fd >= 0, return APPSPAWN_SYSTEM_ERROR,
        "Failed to open file errno: %{public}d path: %{public}s", errno, path);
    int ret = 0;
    for (uint32_t i = 0; i < count; i++) {
        APPSPAWN_LOGV(" WriteToFile pid %{public}d ", pids[i]);
        ret = snprintf_s(pidName, sizeof(pidName), sizeof(pidName) - 1, "%d\n", pids[i]);
        APPSPAWN_CHECK(ret > 0, break, "Failed to snprintf_s errno: %{public}d", errno);
        ret = write(fd, pidName, strlen(pidName));
        APPSPAWN_CHECK(ret > 0, break,
            "Failed to write file errno: %{public}d path: %{public}s %{public}s", errno, path, pidName);
        ret = 0;
    }
    close(fd);
    return ret;
}

static void SetForkDenied(const AppSpawnedProcessInfo *appInfo)
{
    char pathForkDenied[PATH_MAX] = {};
    int ret = GetCgroupPath(appInfo, pathForkDenied, sizeof(pathForkDenied));
    APPSPAWN_CHECK(ret == 0, return, "Failed to get cgroup path errno: %{public}d", errno);
    ret = strcat_s(pathForkDenied, sizeof(pathForkDenied), "pids.fork_denied");
    APPSPAWN_CHECK(ret == 0, return, "Failed to strcat_s fork_denied path errno: %{public}d", errno);
    int fd = open(pathForkDenied, O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGW("SetForkDenied %{public}d open failed ", appInfo->pid);
        return;
    }
    do {
        ret = write(fd, "1", 1);
        APPSPAWN_CHECK(ret >= 0, break,
        "Failed to write file errno: %{public}d path: %{public}s %{public}d", errno, pathForkDenied, ret);
        fsync(fd);
        APPSPAWN_LOGI("SetForkDenied success, cgroup's owner:%{public}d", appInfo->pid);
    } while (0);
    close(fd);
}

static void KillProcessesByCGroup(const char *path, AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
{
    SetForkDenied(appInfo);
    FILE *file = fopen(path, "r");
    APPSPAWN_CHECK(file != NULL, return, "Open file fail %{public}s errno: %{public}d", path, errno);
    pid_t pid = 0;
    while (fscanf_s(file, "%d\n", &pid) == 1 && pid > 0) {
        APPSPAWN_LOGV(" KillProcessesByCGroup pid %{public}d ", pid);
        if (pid == appInfo->pid) {
            continue;
        }
        AppSpawnedProcessInfo *tmp = GetSpawnedProcess(pid);
        if (tmp != NULL) {
            APPSPAWN_LOGI("Got app %{public}s in same group for pid %{public}d.", tmp->name, pid);
            continue;
        }
        APPSPAWN_LOGI("Kill app pid %{public}d now ...", pid);
#ifndef APPSPAWN_TEST
        if (kill(pid, SIGKILL) != 0) {
            APPSPAWN_LOGE("unable to kill process, pid: %{public}d ret %{public}d", pid, errno);
        }
#endif
    }
    (void)fclose(file);
}

static int ProcessMgrRemoveApp(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(appInfo != NULL, return -1);
    if (IsNWebSpawnMode(content) || strcmp(appInfo->name, NWEBSPAWN_SERVER_NAME) == 0) {
        return 0;
    }
    char cgroupPath[PATH_MAX] = {};
    APPSPAWN_LOGV("ProcessMgrRemoveApp %{public}d %{public}d to cgroup ", appInfo->pid, appInfo->uid);
    int ret = GetCgroupPath(appInfo, cgroupPath, sizeof(cgroupPath));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get real path errno: %{public}d", errno);
    char procPath[PATH_MAX] = {};
    ret = memcpy_s(procPath, sizeof(procPath), cgroupPath, sizeof(cgroupPath));
    if (ret != 0) {
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }
    ret = strcat_s(procPath, sizeof(procPath), "cgroup.procs");
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to strcat_s errno: %{public}d", errno);
    KillProcessesByCGroup(procPath, (AppSpawnMgr *)content, appInfo);
    ret = rmdir(cgroupPath);
    if (ret != 0) {
        return APPSPAWN_ERROR_FILE_RMDIR_FAIL;
    }
    return ret;
}

static int ProcessMgrAddApp(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(appInfo != NULL, return -1);
    if (IsNWebSpawnMode(content)) {
        return 0;
    }
    char path[PATH_MAX] = {};
    APPSPAWN_LOGV("ProcessMgrAddApp %{public}d %{public}d to cgroup ", appInfo->pid, appInfo->uid);
    int ret = GetCgroupPath(appInfo, path, sizeof(path));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get real path errno: %{public}d", errno);
    (void)CreateSandboxDir(path, 0755);  // 0755 default mode
    ret = strcat_s(path, sizeof(path), "cgroup.procs");
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to strcat_s errno: %{public}d", errno);
    ret = WriteToFile(path, 0, (pid_t *)&appInfo->pid, 1);
    APPSPAWN_CHECK(ret == 0, return ret, "write pid to cgroup.procs fail %{public}s", path);
    APPSPAWN_LOGV("Add app %{public}d to cgroup %{public}s success", appInfo->pid, path);
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    AddProcessMgrHook(STAGE_SERVER_APP_ADD, 0, ProcessMgrAddApp);
    AddProcessMgrHook(STAGE_SERVER_APP_DIED, 0, ProcessMgrRemoveApp);
}
