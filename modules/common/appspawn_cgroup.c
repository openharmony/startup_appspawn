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
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_adapter.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "securec.h"
#include "parameter.h"

#define APPSPAWN_TAG_SUFFIX "_apptag"
#define NWEB_TAG_SUFFIX "_nwebtag"
#define NATIVE_TAG_SUFFIX "_nativetag"
#define CJ_TAG_SUFFIX "_cjtag"
#define HYBRID_TAG_SUFFIX "_hybridtag"
#define CGROUP_ROOT_PATH "/dev/pids/"
#define UID_DIR_MIN_LEN 3    // uid >= 100, min digit length
#define UID_DIR_MAX_LEN 10   // uid <= 9999999999, max digit length
#define APP_DIR_PREFIX "app_"
#define APP_DIR_PREFIX_LEN 4
#define DEFAULT_UID "0"
#define APPSPAWN_GRACEFUL_STOP_PARAM "startup.appspawn.graceful_stop.%s"
#define APPSPAWN_GRACEFUL_STOP_PARAM_LEN 64
#define CLEANUP_FLAG_NEED "1"
#define CLEANUP_FLAG_NONE "0"

typedef struct {
    RunMode mode;
    const char *tagSuffix;
    const char *serverName;
} SpawnModeInfo;

static const SpawnModeInfo G_SPAWN_MODE_MAP[] = {
    {MODE_FOR_APP_SPAWN, APPSPAWN_TAG_SUFFIX, APPSPAWN_SERVER_NAME },
    {MODE_FOR_NWEB_SPAWN, NWEB_TAG_SUFFIX, NWEBSPAWN_SERVER_NAME },
    {MODE_FOR_NATIVE_SPAWN, NATIVE_TAG_SUFFIX, NATIVESPAWN_SERVER_NAME },
    {MODE_FOR_CJAPP_SPAWN, CJ_TAG_SUFFIX, CJAPPSPAWN_SERVER_NAME },
    {MODE_FOR_HYBRID_SPAWN, HYBRID_TAG_SUFFIX, HYBRIDSPAWN_SERVER_NAME },
};
static const size_t G_SPAWN_MODE_MAP_SIZE = sizeof(G_SPAWN_MODE_MAP) / sizeof(G_SPAWN_MODE_MAP[0]);

static const SpawnModeInfo *GetSpawnModeInfo(const AppSpawnMgr *content)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return NULL);
    RunMode mode = content->content.mode;
    for (size_t i = 0; i < G_SPAWN_MODE_MAP_SIZE; i++) {
        if (G_SPAWN_MODE_MAP[i].mode == mode) {
            return &G_SPAWN_MODE_MAP[i];
        }
    }
    return NULL;
}

APPSPAWN_STATIC int GetCgroupPath(const AppSpawnedProcessInfo *appInfo, char *buffer, uint32_t buffLen)
{
    const SpawnModeInfo *modeInfo = GetSpawnModeInfo(GetAppSpawnMgr());
    APPSPAWN_CHECK(modeInfo != NULL, return -1, "Failed to get spawn mode info");
    const char *tagSuffix = modeInfo->tagSuffix;
    const int userId = appInfo->uid / UID_BASE;
#ifdef APPSPAWN_TEST
    int ret = snprintf_s(buffer, buffLen, buffLen - 1, APPSPAWN_BASE_DIR "/dev/pids/testpids/%d/%s%s/app_%d/",
        userId, appInfo->name, tagSuffix, appInfo->pid);
#else
    int ret = snprintf_s(buffer, buffLen, buffLen - 1, "/dev/pids/%d/%s%s/app_%d/",
        userId, appInfo->name, tagSuffix, appInfo->pid);
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

// Set fork_denied for a cgroup directory by absolute path, write "1" to {appDirPath}/pids.fork_denied
APPSPAWN_STATIC void SetForkDeniedByPath(const char *appDirPath)
{
    char pathForkDenied[PATH_MAX] = {};
    int ret = snprintf_s(pathForkDenied, sizeof(pathForkDenied), sizeof(pathForkDenied) - 1,
        "%s/pids.fork_denied", appDirPath);
    APPSPAWN_CHECK(ret > 0, return, "Failed to build fork_denied path for %{public}s", appDirPath);
    int fd = open(pathForkDenied, O_RDWR);
    APPSPAWN_CHECK(fd >= 0, return,
        "Failed to open fork_denied for %{public}s errno: %{public}d", appDirPath, errno);
    do {
        ret = write(fd, "1", 1);
        APPSPAWN_CHECK(ret >= 0, break,
        "Failed to write file errno: %{public}d path: %{public}s %{public}d", errno, pathForkDenied, ret);
        APPSPAWN_CHECK_ONLY_LOG(fsync(fd) != -1, "Failed to fsync for target: %{public}d", errno);
        APPSPAWN_LOGI("SetForkDenied success for %{public}s", appDirPath);
    } while (0);
    close(fd);
}

static void SetForkDenied(const AppSpawnedProcessInfo *appInfo)
{
    char pathForkDenied[PATH_MAX] = {};
    int ret = GetCgroupPath(appInfo, pathForkDenied, sizeof(pathForkDenied));
    APPSPAWN_CHECK(ret == 0, return, "Failed to get cgroup path errno: %{public}d", errno);
    // Remove trailing '/' appended by GetCgroupPath for path concatenation
    size_t len = strlen(pathForkDenied);
    APPSPAWN_ONLY_EXPER(len > 0 && pathForkDenied[len - 1] == '/', pathForkDenied[len - 1] = '\0');
    SetForkDeniedByPath(pathForkDenied);
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

APPSPAWN_STATIC int ProcessMgrRemoveApp(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
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
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_ERROR_FILE_RMDIR_FAIL,
        "Failed to rmdir in ProcessMgrRemoveApp %{public}s errno: %{public}d", cgroupPath, errno);
    return ret;
}

// Check if directory name is a valid UID directory: "0" or 3-10 digit number
APPSPAWN_STATIC int IsUidDir(const char *name)
{
    if (name == NULL || *name == '\0') {
        return 0;
    }
    if (strcmp(name, DEFAULT_UID) == 0) {
        return 1;
    }
    size_t len = strlen(name);
    if (len < UID_DIR_MIN_LEN || len > UID_DIR_MAX_LEN) {
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        if (name[i] < '0' || name[i] > '9') {
            return 0;
        }
    }
    return 1;
}

// Check if directory name is a tag dir managed by current spawn server: must end with spawn-type tag suffix
APPSPAWN_STATIC int IsTagDir(const char *name)
{
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL && *name != '\0', return 0);
    const SpawnModeInfo *modeInfo = GetSpawnModeInfo(GetAppSpawnMgr());
    APPSPAWN_CHECK(modeInfo != NULL, return 0, "Failed to get spawn mode info");
    const char *suffix = modeInfo->tagSuffix;
    size_t nameLen = strlen(name);
    size_t suffixLen = strlen(suffix);
    if (nameLen <= suffixLen) {
        return 0;
    }
    return strcmp(name + nameLen - suffixLen, suffix) == 0;
}

// Check if directory name matches "app_" prefix followed by digits, e.g. "app_12345"
APPSPAWN_STATIC int IsAppDir(const char *name)
{
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL && *name != '\0', return 0);
    size_t prefixLen = APP_DIR_PREFIX_LEN;
    APPSPAWN_CHECK_ONLY_EXPER(strncmp(name, APP_DIR_PREFIX, prefixLen) == 0, return 0);
    for (const char *p = name + prefixLen; *p != '\0'; p++) {
        APPSPAWN_CHECK_ONLY_EXPER(*p >= '0' && *p <= '9', return 0);
    }
    return *(name + prefixLen) != '\0';
}

// Read cgroup.procs and send SIGKILL to each listed PID
static void KillOrphanedProcessesInFile(const char *procPath, const char *appDirName)
{
    FILE *file = fopen(procPath, "r");
    APPSPAWN_CHECK(file != NULL, return,
        "Failed to open %{public}s errno: %{public}d", procPath, errno);
    pid_t pid = 0;
    while (fscanf_s(file, "%d\n", &pid) == 1 && pid > 0) {
        APPSPAWN_LOGI("Kill orphaned cgroup child pid %{public}d in %{public}s", pid, appDirName);
        APPSPAWN_CHECK(kill(pid, SIGKILL) == 0, continue,
            "Unable to kill process, pid: %{public}d errno: %{public}d", pid, errno);
    }
    (void)fclose(file);
}

// Cleanup a single app cgroup dir: set fork_denied -> kill processes -> remove files -> rmdir
static void CleanupOrphanedAppDir(const char *tagDirPath, const char *appDirName)
{
    char appDirPath[PATH_MAX] = {};
    int ret = snprintf_s(appDirPath, sizeof(appDirPath), sizeof(appDirPath) - 1,
        "%s/%s", tagDirPath, appDirName);
    APPSPAWN_CHECK(ret > 0, return,
        "Failed to build app dir path for %{public}s/%{public}s", tagDirPath, appDirName);

    SetForkDeniedByPath(appDirPath);

    char procPath[PATH_MAX] = {};
    ret = snprintf_s(procPath, sizeof(procPath), sizeof(procPath) - 1,
        "%s/cgroup.procs", appDirPath);
    APPSPAWN_CHECK(ret > 0, return,
        "Failed to build proc path for %{public}s/%{public}s", tagDirPath, appDirName);
    KillOrphanedProcessesInFile(procPath, appDirName);

    // Remove files before rmdir
    char filePath[PATH_MAX] = {};
    ret = snprintf_s(filePath, sizeof(filePath), sizeof(filePath) - 1,
        "%s/cgroup.procs", appDirPath);
    APPSPAWN_ONLY_EXPER(ret > 0, (void)unlink(filePath));
    ret = snprintf_s(filePath, sizeof(filePath), sizeof(filePath) - 1,
        "%s/pids.fork_denied", appDirPath);
    APPSPAWN_ONLY_EXPER(ret > 0, (void)unlink(filePath));
    APPSPAWN_CHECK(rmdir(appDirPath) == 0, return,
        "Failed to rmdir %{public}s errno: %{public}d", appDirPath, errno);
}

// Cleanup all app subdirectories under a tag dir, then remove the tag dir itself
APPSPAWN_STATIC void CleanupOrphanedTagDir(const char *uidPath, const char *dirName)
{
    APPSPAWN_CHECK_ONLY_EXPER(IsTagDir(dirName), return);
    char tagDirPath[PATH_MAX] = {};
    int ret = snprintf_s(tagDirPath, sizeof(tagDirPath), sizeof(tagDirPath) - 1, "%s/%s", uidPath, dirName);
    APPSPAWN_CHECK(ret > 0, return, "Failed to build tag dir path for %{public}s", dirName);
    DIR *tagDir = opendir(tagDirPath);
    APPSPAWN_CHECK(tagDir != NULL, return,
        "Failed to open tag dir %{public}s errno: %{public}d", tagDirPath, errno);
    struct dirent *appDp = NULL;
    while ((appDp = readdir(tagDir)) != NULL) {
        if (appDp->d_type != DT_DIR || !IsAppDir(appDp->d_name)) {
            continue;
        }
        CleanupOrphanedAppDir(tagDirPath, appDp->d_name);
    }
    (void)closedir(tagDir);
    ret = rmdir(tagDirPath);
    APPSPAWN_ONLY_EXPER(ret != 0 && errno != ENOENT,
        APPSPAWN_LOGW("Failed to rmdir tag dir %{public}s errno: %{public}d", tagDirPath, errno));
}

// Top-level orphan cgroup cleanup: traverse all UID dirs and tag dirs under CGROUP_ROOT_PATH
APPSPAWN_STATIC void CleanupOrphanedCgroupProcesses(void)
{
    DIR *dir = opendir(CGROUP_ROOT_PATH);
    APPSPAWN_CHECK_ONLY_EXPER(dir != NULL, return);
    struct dirent *dp = NULL;
    while ((dp = readdir(dir)) != NULL) {
        if (dp->d_type != DT_DIR || strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        if (!IsUidDir(dp->d_name)) {
            continue;
        }
        char uidPath[PATH_MAX] = {};
        int ret = snprintf_s(uidPath, sizeof(uidPath), sizeof(uidPath) - 1, "%s%s", CGROUP_ROOT_PATH, dp->d_name);
        APPSPAWN_CHECK(ret > 0, continue, "Failed to build uid path for %{public}s", dp->d_name);
        DIR *uidDir = opendir(uidPath);
        APPSPAWN_CHECK(uidDir != NULL, continue,
            "Failed to open uid dir %{public}s errno: %{public}d", uidPath, errno);
        struct dirent *entry = NULL;
        while ((entry = readdir(uidDir)) != NULL) {
            if (entry->d_type != DT_DIR || strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            CleanupOrphanedTagDir(uidPath, entry->d_name);
        }
        (void)closedir(uidDir);
    }
    (void)closedir(dir);
}

// STAGE_SERVER_PRELOAD hook: detect abnormal stop via system parameter, cleanup orphaned cgroup if needed
APPSPAWN_STATIC int CgroupPreloadHook(AppSpawnMgr *content)
{
    // Cold start child processes should not cleanup orphaned cgroup directories
    APPSPAWN_ONLY_EXPER(IsColdRunMode(content), return 0);
    const SpawnModeInfo *modeInfo = GetSpawnModeInfo(content);
    APPSPAWN_CHECK(modeInfo != NULL, return -1, "Failed to get spawn mode info");
    char paramName[APPSPAWN_GRACEFUL_STOP_PARAM_LEN] = {};
    int ret = snprintf_s(paramName, sizeof(paramName), sizeof(paramName) - 1,
        APPSPAWN_GRACEFUL_STOP_PARAM, modeInfo->serverName);
    APPSPAWN_CHECK(ret > 0, return -1, "Failed to build graceful stop param name");
    char value[2] = {};
    ret = GetParameter(paramName, CLEANUP_FLAG_NONE, value, sizeof(value));
    APPSPAWN_ONLY_EXPER(ret > 0 && strcmp(value, CLEANUP_FLAG_NEED) == 0,
        APPSPAWN_LOGI("Appspawn abnormal stop detected, cleanup orphaned cgroup processes");
        CleanupOrphanedCgroupProcesses());
    // Mark current run: set to NEED so next start after crash will cleanup
    SetParameter(paramName, CLEANUP_FLAG_NEED);
    return 0;
}

// STAGE_SERVER_EXIT hook: clear graceful stop flag to indicate normal exit
APPSPAWN_STATIC int CgroupExitHook(AppSpawnMgr *content)
{
    const SpawnModeInfo *modeInfo = GetSpawnModeInfo(content);
    APPSPAWN_CHECK(modeInfo != NULL, return -1, "Failed to get spawn mode info");
    char paramName[APPSPAWN_GRACEFUL_STOP_PARAM_LEN] = {};
    int ret = snprintf_s(paramName, sizeof(paramName), sizeof(paramName) - 1,
        APPSPAWN_GRACEFUL_STOP_PARAM, modeInfo->serverName);
    APPSPAWN_CHECK(ret > 0, return 0, "Failed to build graceful stop param name");
    // Normal exit, clear flag so next start skips cleanup
    SetParameter(paramName, CLEANUP_FLAG_NONE);
    APPSPAWN_LOGI("Clear abnormal stop flag before appspawn exit");
    return 0;
}

APPSPAWN_STATIC int ProcessMgrAddApp(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(appInfo != NULL, return -1);
    APPSPAWN_ONLY_EXPER(IsNWebSpawnMode(content), return 0);
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
    AddServerStageHook(STAGE_SERVER_PRELOAD, 0, CgroupPreloadHook);
    AddServerStageHook(STAGE_SERVER_EXIT, 0, CgroupExitHook);
    AddProcessMgrHook(STAGE_SERVER_APP_ADD, 0, ProcessMgrAddApp);
    AddProcessMgrHook(STAGE_SERVER_APP_DIED, 0, ProcessMgrRemoveApp);
}
