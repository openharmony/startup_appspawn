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

#ifndef APPSPAWN_KICKDOG_H
#define APPSPAWN_KICKDOG_H

#include <fcntl.h>
#include <stdio.h>
#include "loop_event.h"
#include "signal.h"
#include "appspawn_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef APPSPAWN_TEST    //macro for test
#ifdef CONFIG_DFX_LIBLINUX
#undef CONFIG_DFX_LIBLINUX
#endif
#define HM_APPSPAWN_WATCHDOG_FILE    "./TestDataFile.txt"
#endif

#ifdef CONFIG_DFX_LIBLINUX    //linux kernel
#define UNCHIP_APPSPAWN_WATCHDOG_FILE    "/sys/kernel/hungtask/userlist"
#define APPSPAWN_WATCHDOG_FILE    "/sys/kernel/appspawn_watchdog/appspawn"
#define UNCHIP_APPSPAWNWATCHDOGON    "on,30"
#define APPSPAWNWATCHDOGON    "on"
#define APPSPAWNWATCHDOGKICK    "kick"
#else    //harmony kernel
#ifndef APPSPAWN_TEST
#define HM_APPSPAWN_WATCHDOG_FILE    "/proc/sys/hguard/user_list"
#endif
#define HM_APPSPAWN_WATCHDOG_ON    "on,300,appspawn"
#define HM_APPSPAWN_WATCHDOG_KICK    "kick,appspawn"
#endif

#define APPSPAWN_WATCHDOG_KICKTIME    (10 * 1000)  //10s

void AppSpawnKickDogStart(void);

#ifdef __cplusplus
}
#endif
#endif  //APPSPAWN_KICKDOG_H