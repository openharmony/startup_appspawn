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

#include "appspawn_adapter.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "appspawn_hook.h"
#include "appspawn_fd_manager.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "securec.h"

#define KILL_LOG_BASE 'S'
#define SYSLOAD_SET_KILL_INFO_MAGIC 0xE5AC03

struct KillEventInfo {
    int id;
    int adj;
    bool processed;
    bool foreground;
    pid_t pid;
    int uid;
    int64_t timestamp;
    int64_t eventParamFirst;
    int64_t eventParamSecond;
    int64_t eventParamThird;
    int64_t eventParamFourth;
    int64_t eventParamFifth;
    int64_t eventParamSixth;
    int64_t eventParamSeventh;
};

struct KillInfo {
    unsigned int magic;
    pid_t pid;
    struct KillEventInfo data;
    unsigned int structSize;
};

#define KILL_INFO_SIZE (sizeof(struct KillInfo))
#define SET_KILL_INFO _IOWR(KILL_LOG_BASE, 0x07, int32_t)

APPSPAWN_STATIC void InitKillInfo(struct KillInfo *info, pid_t pid, uid_t uid, int reason)
{
    if (info == NULL) {
        APPSPAWN_LOGE("InitKillInfo: invalid info pointer");
        return;
    }
    int ret = memset_s(info, sizeof(*info), 0, sizeof(*info));
    if (ret != 0) {
        APPSPAWN_LOGE("InitKillInfo: memset_s failed, ret=%{public}d", ret);
        return;
    }
    info->magic = SYSLOAD_SET_KILL_INFO_MAGIC;
    info->pid = pid;
    info->data.id = reason;
    info->data.processed = false;
    info->data.foreground = false;
    info->data.pid = pid;
    info->data.uid = uid;
    info->structSize = KILL_INFO_SIZE;
}

APPSPAWN_STATIC int GetKillReasonFd(AppSpawnMgr *mgr)
{
    AppSpawnFds *killReasonFds = FindSpawningFdsByPid(mgr, getpid(), TYPE_KILL_REASON_FD);
    if (killReasonFds != NULL && killReasonFds->count > 0) {
        return killReasonFds->fds[0];
    }
    int fd = open(DEV_SYSLOAD, O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGE("open %{public}s failed, errno:%{public}d", DEV_SYSLOAD, errno);
        return -1;
    }
    SpawningFdRegInfo regInfo = { TYPE_KILL_REASON_FD, 1, &fd, getpid() };
    AppSpawnFds *fds = RegisterSpawningFds(mgr, &regInfo);
    if (fds == NULL) {
        APPSPAWN_LOGE("RegisterSpawningFds failed, fd:%{public}d", fd);
        close(fd);
        return -1;
    }
    return fd;
}

void SetKillReason(const AppSpawnMgr *mgr, pid_t pid, uid_t uid, int reason)
{
    int fd = GetKillReasonFd((AppSpawnMgr *)mgr);
    if (fd < 0) {
        APPSPAWN_LOGE("SetKillReason: no fd, pid:%{public}d, uid:%{public}d, reason:%{public}d",
            pid, uid, reason);
        return;
    }
    struct KillInfo info;
    InitKillInfo(&info, pid, uid, reason);
    int res = ioctl(fd, SET_KILL_INFO, &info);
    if (res != 0) {
        APPSPAWN_LOGE("SetKillReason: ioctl failed, pid:%{public}d, uid:%{public}d, reason:%{public}d, "
            "errno:%{public}d", pid, uid, reason, errno);
    } else {
        APPSPAWN_LOGI("SetKillReason: pid:%{public}d, uid:%{public}d, reason:%{public}d",
            pid, uid, reason);
    }
}

APPSPAWN_STATIC int KillReasonReportHook(const AppSpawnMgr *mgr, const AppSpawnedProcessInfo *appInfo)
{
    if (appInfo == NULL || appInfo->killReason == 0) {
        return 0;
    }
    APPSPAWN_LOGI("KillReasonReportHook: pid:%{public}d uid:%{public}d reason:%{public}d",
        appInfo->pid, appInfo->uid, appInfo->killReason);
    SetKillReason(mgr, appInfo->pid, appInfo->uid, appInfo->killReason);
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    AddProcessMgrHook(STAGE_SERVER_APP_CLEANUP, HOOK_PRIO_LOWEST, KillReasonReportHook);
}
