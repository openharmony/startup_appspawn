/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "nwebspawn_lancher.h"
#include "appspawn_server.h"
#ifdef CODE_SIGNATURE_ENABLE
#include "code_sign_attr_utils.h"
#endif

#define NWEB_UID 3081
#define NWEB_GID 3081
#define NWEB_NAME "nwebspawn"
#define CAP_NUM 2
#define BITLEN32 32

pid_t NwebSpawnLanch()
{
    pid_t ret = fork();
    if (ret == 0) {
#ifdef CODE_SIGNATURE_ENABLE
        // ownerId must been set before setcon & setuid
        (void)SetXpmOwnerId(PROCESS_OWNERID_EXTEND, NULL);
#endif
        setcon("u:r:nwebspawn:s0");
        pid_t pid = getpid();
        setpriority(PRIO_PROCESS, pid, 0);
        struct  __user_cap_header_struct capHeader;
        capHeader.version = _LINUX_CAPABILITY_VERSION_3;
        capHeader.pid = 0;
        const uint64_t inheriTable = 0x2000c0;
        const uint64_t permitted = 0x2000c0;
        const uint64_t effective = 0x2000c0;
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
        (void)prctl(PR_SET_NAME, NWEB_NAME);
        setuid(NWEB_UID);
        setgid(NWEB_GID);
        APPSPAWN_LOGI("nwebspawn fork success");
    }
    return ret;
}