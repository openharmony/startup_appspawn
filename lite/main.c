/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "samgr_lite.h"
#include "appspawn_server.h"
#include "appspawn_service.h"

void __attribute__((weak)) HOS_SystemInit(void)
{
    SAMGR_Bootstrap();
    APPSPAWN_LOGI("[appspawn] HOS_SystemInit is called!");
}

static void SignalHandler(int sig)
{
    switch (sig) {
        case SIGCHLD: {
            pid_t sigPID;
            int procStat = 0;
            while (1) {
                sigPID = waitpid(-1, &procStat, WNOHANG);
                if (sigPID <= 0) {
                    break;
                }
                APPSPAWN_LOGE("SignalHandler sigPID %d.", sigPID);
            }
            break;
        }
        default:
            break;
    }
}

void SignalRegist(void)
{
    struct sigaction act;
    act.sa_handler = SignalHandler;
    act.sa_flags   = SA_RESTART;
    if (sigfillset(&act.sa_mask) != 0) {
        APPSPAWN_LOGE("[appspawn] sigfillset failed! err %d.", errno);
    }

    if (sigaction(SIGCHLD, &act, NULL) != 0) {
        APPSPAWN_LOGE("[appspawn] sigaction failed! err %d.", errno);
    }
}

int main(int argc, char * const argv[])
{
    sleep(1);
    APPSPAWN_LOGI("[appspawn] main, enter.");

    AppSpawnContent *content = AppSpawnCreateContent(APPSPAWN_SERVICE_NAME, NULL, 0, 0);
    if (content == NULL) {
        APPSPAWN_LOGE("Failed to create content for appspawn");
        return -1;
    }
    SetContentFunction(content);
    // 1. ipc module init
    HOS_SystemInit();

    // 2. register signal for SIGCHLD
    SignalRegist();

    // 3. keep process alive
    APPSPAWN_LOGI("[appspawn] main, entering wait.");
    while (1) {
        // pause only returns when a signal was caught and the signal-catching function returned.
        // pause only returns -1, no need to process the return value.
        (void)pause();
    }
}
