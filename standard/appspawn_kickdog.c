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

    int procFd = open(procName, O_RDWR | O_CLOEXEC);
    if (procFd == -1) {
        APPSPAWN_LOGE("open %{public}s fail,errno:%{public}d", procName, errno);
        return procFd;
    }

    int writeResult = write(procFd, writeStr, writeLen);
    if (writeResult != (int)writeLen) {
        APPSPAWN_LOGE("write %{public}s fail,result:%{public}d", writeStr, writeResult);
    } else {
        APPSPAWN_LOGI("write %{public}s success", writeStr);
    }

    close(procFd);
    return writeResult;
}

// Enable watchdog monitoring
static int OpenAppSpawnWatchdogFile(AppSpawnContent *content)
{
    APPSPAWN_LOGI("OpenAppSpawnWatchdogFile");
    int result = 0;
    if (access(LINUX_APPSPAWN_WATCHDOG_FILE, F_OK) == 0) {
        result = OpenAndWriteToProc(LINUX_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_ON,
            strlen(LINUX_APPSPAWN_WATCHDOG_ON));
    } else {
        result = OpenAndWriteToProc(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_ON,
            strlen(HM_APPSPAWN_WATCHDOG_ON));
    }
    content->wdgOpened = (result != -1);
    APPSPAWN_LOGI("open finish,result:%{public}d", result);
    return result;
}

static void KickAppSpawnWatchdog(void)
{
    APPSPAWN_LOGI("kick proc begin");
    int result = 0;
    if (access(LINUX_APPSPAWN_WATCHDOG_FILE, F_OK) == 0) {
        result = OpenAndWriteToProc(LINUX_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_KICK,
            strlen(LINUX_APPSPAWN_WATCHDOG_KICK));
    } else {
        result = OpenAndWriteToProc(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_KICK,
            strlen(HM_APPSPAWN_WATCHDOG_KICK));
    }
    APPSPAWN_LOGI("kick end,result:%{public}d", result);
}

static void ProcessTimerHandle(const TimerHandle taskHandle, void *context)
{
    AppSpawnContent *wdgContent = (AppSpawnContent *)context;

    APPSPAWN_LOGI("kick appspawn dog start");

    if (!wdgContent->wdgOpened) {  //first time deal kernel file
        OpenAppSpawnWatchdogFile(wdgContent);
        return;
    }

    KickAppSpawnWatchdog();
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

void AppSpawnKickDogStart(AppSpawnContent *content)
{
    if (content == NULL) {
        APPSPAWN_LOGE("Content is null");
        return;
    }
    CreateTimerLoopTask(content);
    OpenAppSpawnWatchdogFile(content);
}