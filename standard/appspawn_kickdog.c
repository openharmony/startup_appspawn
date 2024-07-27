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

#ifdef CONFIG_DFX_LIBLINUX
static int g_notUnchip = 0;
#endif
static int g_wdgOpened = 0;

#ifdef CONFIG_DFX_LIBLINUX
static void CheckNotUnchip(void)
{
    int procFd = open(APPSPAWN_WATCHDOG_FILE, O_RDWR | O_CLOEXEC);
    if (procFd != -1) {
        close(procFd);
    }
    g_notUnchip = (procFd != -1);
}
#endif

static int OpenAndWriteToProc(const char *procName, const char *writeStr, size_t writeLen)
{
    if (writeStr == NULL) {
        APPSPAWN_LOGI("Write string is null");
        return -1;
    }

    int procFd = open(procName, O_RDWR | O_CLOEXEC);
    if (procFd == -1) {
        APPSPAWN_LOGI("open %{public}s fail,errno:%{public}d", procName, errno);
        return procFd;
    }

    int writeResult = write(procFd, writeStr, writeLen);
    if (writeResult != (int)writeLen) {
        APPSPAWN_LOGI("write %{public}s fail,result:%{public}d", writeStr, writeResult);
        close(procFd);
        return writeResult;
    }

    close(procFd);
    APPSPAWN_LOGI("write %{public}s success", writeStr);
    return writeResult;
}

// Enable watchdog monitoring
static int OpenAppSpawnWatchdogFile(void)
{
    APPSPAWN_LOGI("OpenAppSpawnWatchdogFile");
    int result = 0;
#ifdef CONFIG_DFX_LIBLINUX
    CheckNotUnchip();
    if (g_notUnchip) {
        result = OpenAndWriteToProc(APPSPAWN_WATCHDOG_FILE, APPSPAWNWATCHDOGON, strlen(APPSPAWNWATCHDOGON));
    } else {
        result = OpenAndWriteToProc(UNCHIP_APPSPAWN_WATCHDOG_FILE, UNCHIP_APPSPAWNWATCHDOGON,
            strlen(UNCHIP_APPSPAWNWATCHDOGON));
    }
#else
    result = OpenAndWriteToProc(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_ON, strlen(HM_APPSPAWN_WATCHDOG_ON));
#endif
    g_wdgOpened = (result != -1);
    APPSPAWN_LOGI("open finish,result:%{puiblic}d", result);
    return result;
}

static void KickAppSpawnWatchdog(void)
{
    APPSPAWN_LOGI("kick proc begin");
    int result = 0;
#ifdef CONFIG_DFX_LIBLINUX
    if (g_notUnchip) {
        result = OpenAndWriteToProc(APPSPAWN_WATCHDOG_FILE, APPSPAWNWATCHDOGKICK, strlen(APPSPAWNWATCHDOGKICK));
    } else {
        result = OpenAndWriteToProc(UNCHIP_APPSPAWN_WATCHDOG_FILE, APPSPAWNWATCHDOGKICK, strlen(APPSPAWNWATCHDOGKICK));
    }
#else
    result = OpenAndWriteToProc(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_KICK,
        strlen(HM_APPSPAWN_WATCHDOG_KICK));
#endif
    APPSPAWN_LOGI("kick end,result:%{puiblic}d", result);
    return;
}

static void ProcessTimerHandle(const TimerHandle taskHandle, void *context)
{
    APPSPAWN_LOGI("kick appspawn dog start");

    if (!g_wdgOpened) {  //first time deal kernel file
        OpenAppSpawnWatchdogFile();
        return;
    }

    KickAppSpawnWatchdog();
    return;
}

static void CreateTimerLoopTask(void)
{
    LoopHandle loop = LE_GetDefaultLoop();
    TimerHandle timer = NULL;
    int ret = LE_CreateTimer(loop, &timer, ProcessTimerHandle, NULL);
    APPSPAWN_CHECK(ret == 0, return, "Failed to create timerLoopTask ret %{public}d", ret);
    // start a timer to write kernel file every 10s
    ret = LE_StartTimer(loop, timer, APPSPAWN_WATCHDOG_KICKTIME, INT64_MAX);
    APPSPAWN_CHECK(ret == 0, return, "Failed to start timerLoopTask ret %{public}d", ret);
    return;
}

void AppSpawnKickDogStart(void)
{
    CreateTimerLoopTask();
    OpenAppSpawnWatchdogFile();
}