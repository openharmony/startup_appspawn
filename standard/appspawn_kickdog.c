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

#include <fcntl.h>
#include <stdio.h>
#include "appspawn_kickdog.h"

static int OpenAndWriteToProc(const char *procName, const char *writeStr, size_t writeLen)
{
    if (writeStr == NULL) {
        APPSPAWN_LOGE("Write string is null");
        return -1;
    }

    int procFd = open(procName, O_WRONLY | O_CLOEXEC);
    if (procFd == -1) {
        APPSPAWN_LOGE("open %{public}s fail,errno:%{public}d", procName, errno);
        return procFd;
    }
    fdsan_exchange_owner_tag(procFd, 0, APPSPAWN_DOMAIN);

    int writeResult = write(procFd, writeStr, writeLen);
    if (writeResult != (int)writeLen) {
        APPSPAWN_LOGE("write %{public}s fail,result:%{public}d", writeStr, writeResult);
    } else {
        APPSPAWN_LOGV("write %{public}s success", writeStr);
    }

    fdsan_close_with_tag(procFd, APPSPAWN_DOMAIN);
    return writeResult;
}

static const char *GetProcFile(bool isLinux)
{
    if (isLinux) {
        return LINUX_APPSPAWN_WATCHDOG_FILE;
    }
    return HM_APPSPAWN_WATCHDOG_FILE;
}

static const char *GetProcContent(bool isLinux, bool isOpen, int mode)
{
    if (isOpen) {
        if (isLinux) {
            return LINUX_APPSPAWN_WATCHDOG_ON;
        }
        return (mode == MODE_FOR_NWEB_SPAWN) ? HM_NWEBSPAWN_WATCHDOG_ON : HM_APPSPAWN_WATCHDOG_ON;
    }

    if (isLinux) {
        return LINUX_APPSPAWN_WATCHDOG_KICK;
    }
    return (mode == MODE_FOR_NWEB_SPAWN) ? HM_NWEBSPAWN_WATCHDOG_KICK : HM_APPSPAWN_WATCHDOG_KICK;
}

static void DealSpawnWatchdog(AppSpawnContent *content, bool isOpen)
{
    int result = 0;
    const char *procFile = GetProcFile(content->isLinux);
    const char *procContent = GetProcContent(content->isLinux, isOpen, content->mode);
    result = OpenAndWriteToProc(procFile, procContent, strlen(procContent));
    if (isOpen) {
        content->wdgOpened = (result != -1);
    }
    APPSPAWN_LOGI("procFile: %{public}s procContent: %{public}s %{public}s %{public}s watchdog end, result:%{public}d",
        procFile, procContent, (content->mode == MODE_FOR_NWEB_SPAWN) ?
            "Nwebspawn" : "Appspawn", isOpen ? "enable" : "kick", result);
}

static void ProcessTimerHandle(const TimerHandle taskHandle, void *context)
{
    AppSpawnContent *wdgContent = (AppSpawnContent *)context;

    if (!wdgContent->wdgOpened) {  //first time deal kernel file
        DealSpawnWatchdog(wdgContent, true);
        return;
    }

    DealSpawnWatchdog(wdgContent, false);
}

static void CreateTimerLoopTask(AppSpawnContent *content)
{
    LoopHandle loop = LE_GetDefaultLoop();
    TimerHandle timer = NULL;
    int ret = LE_CreateTimer(loop, &timer, ProcessTimerHandle, (void *)content);
    APPSPAWN_CHECK(ret == 0, return, "Failed to create timerLoopTask ret %{public}d", ret);
    // start a timer to write kernel file every 10s
    ret = LE_StartTimer(loop, timer, APPSPAWN_WATCHDOG_KICKTIME, INT64_MAX);
    APPSPAWN_CHECK(ret == 0, return, "Failed to start timerLoopTask ret %{public}d", ret);
}

static int CheckKernelType(bool *isLinux)
{
    struct utsname uts;
    if (uname(&uts) == -1) {
        APPSPAWN_LOGE("Kernel type get failed,errno:%{public}d", errno);
        return -1;
    }

    if (strcmp(uts.sysname, "Linux") == 0) {
        APPSPAWN_LOGI("Kernel type is linux");
        *isLinux = true;
        return 0;
    }

    *isLinux = false;
    return 0;
}

APPSPAWN_STATIC int SpawnKickDogStart(AppSpawnMgr *mgrContent)
{
    APPSPAWN_CHECK(mgrContent != NULL, return 0, "content is null");
    APPSPAWN_CHECK((mgrContent->content.mode == MODE_FOR_APP_SPAWN) ||
        (mgrContent->content.mode == MODE_FOR_NWEB_SPAWN), return 0, "Mode %{public}u no need enable watchdog",
        mgrContent->content.mode);

    if (CheckKernelType(&mgrContent->content.isLinux) != 0) {
        return 0;
    }

    DealSpawnWatchdog(&mgrContent->content, true);
    CreateTimerLoopTask(&mgrContent->content);

    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    AddPreloadHook(HOOK_PRIO_COMMON, SpawnKickDogStart);
}