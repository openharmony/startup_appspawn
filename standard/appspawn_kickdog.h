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
#include <signal.h>
#include "loop_event.h"
#include "appspawn_utils.h"
#include "appspawn_server.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef APPSPAWN_TEST    //macro for test
#define LINUX_APPSPAWN_WATCHDOG_FILE    "./TestDataFile.txt"
#define HM_APPSPAWN_WATCHDOG_FILE    "./TestDataFile.txt"
#else
#define LINUX_APPSPAWN_WATCHDOG_FILE    "/sys/kernel/hungtask/userlist"
#define HM_APPSPAWN_WATCHDOG_FILE    "/proc/sys/hguard/user_list"
#endif

#define LINUX_APPSPAWN_WATCHDOG_ON    "on,30"
#define LINUX_APPSPAWN_WATCHDOG_KICK    "kick"
#define HM_APPSPAWN_WATCHDOG_ON    "on,10,appspawn"
#define HM_APPSPAWN_WATCHDOG_KICK    "kick,appspawn"

#define APPSPAWN_WATCHDOG_KICKTIME    (10 * 1000)  //10s

void AppSpawnKickDogStart(AppSpawnContent *content);

#ifdef __cplusplus
}
#endif
#endif  //APPSPAWN_KICKDOG_H