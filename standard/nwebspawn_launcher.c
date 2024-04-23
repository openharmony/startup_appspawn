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
#include <sched.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/resource.h>

#include "appspawn.h"
#include "appspawn_modulemgr.h"
#include "appspawn_msg.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "appspawn_utils.h"
#include "parameter.h"
#include "securec.h"

#ifdef CODE_SIGNATURE_ENABLE
#include "code_sign_attr_utils.h"
#endif
#ifdef WITH_SELINUX
#include "selinux/selinux.h"
#endif

#define NWEB_UID 3081
#define NWEB_GID 3081
#define NWEB_NAME "nwebspawn"
#define CAP_NUM 2
#define BITLEN32 32

void NWebSpawnInit(void)
{
    APPSPAWN_LOGI("NWebSpawnInit");
#ifdef CODE_SIGNATURE_ENABLE
    // ownerId must been set before setcon & setuid
    (void)SetXpmOwnerId(PROCESS_OWNERID_EXTEND, NULL);
#endif
#ifdef WITH_SELINUX
    setcon("u:r:nwebspawn:s0");
#endif
    pid_t pid = getpid();
    setpriority(PRIO_PROCESS, pid, 0);
    struct  __user_cap_header_struct capHeader;
    capHeader.version = _LINUX_CAPABILITY_VERSION_3;
    capHeader.pid = 0;
    // old CAP_SYS_ADMIN | CAP_SETGID | CAP_SETUID
    const uint64_t inheriTable = CAP_TO_MASK(CAP_SYS_ADMIN) | CAP_TO_MASK(CAP_SETGID) |
        CAP_TO_MASK(CAP_SETUID) | CAP_TO_MASK(CAP_KILL);
    const uint64_t permitted = inheriTable;
    const uint64_t effective = inheriTable;
    struct __user_cap_data_struct capData[CAP_NUM] = {};
    for (int j = 0; j < CAP_NUM; ++j) {
        capData[0].inheritable = (__u32)(inheriTable);
        capData[1].inheritable = (__u32)(inheriTable >> BITLEN32);
        capData[0].permitted = (__u32)(permitted);
        capData[1].permitted = (__u32)(permitted >> BITLEN32);
        capData[0].effective = (__u32)(effective);
        capData[1].effective = (__u32)(effective >> BITLEN32);
    }
    capset(&capHeader, capData);
    for (int i = 0; i <= CAP_LAST_CAP; ++i) {
        prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, i, 0, 0);
    }
#ifndef APPSPAWN_TEST
    (void)prctl(PR_SET_NAME, NWEB_NAME);
#endif
    setuid(NWEB_UID);
    setgid(NWEB_GID);
}

pid_t NWebSpawnLaunch(void)
{
    pid_t ret = fork();
    if (ret == 0) {
        NWebSpawnInit();
    }
    APPSPAWN_LOGI("nwebspawn fork success pid: %{public}d", ret);
    return ret;
}
