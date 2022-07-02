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
#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __LINUX__
#include <linux/capability.h>
#include <linux/securebits.h>
#include <sys/resource.h>
#include <sys/time.h>
#else
#include <sys/capability.h>
#endif // __LINUX__

#include "ability_main.h"
#include "appspawn_message.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "securec.h"

#define DEFAULT_UMASK 002
#define CAP_NUM 2
#define ENV_TITLE "LD_LIBRARY_PATH="
#define UPPER_BOUND_GID 999
#define LOWER_BOUND_GID 100
#define GRP_NUM 2
#define DEVMGR_GRP 99

static int SetAmbientCapability(int cap)
{
#ifdef __LINUX__
    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, cap, 0, 0)) {
        printf("[Init] prctl PR_CAP_AMBIENT failed\n");
        return -1;
    }
#endif
    return 0;
}

static int SetCapability(unsigned int capsCnt, const unsigned int *caps)
{
    struct __user_cap_header_struct capHeader;
    capHeader.version = _LINUX_CAPABILITY_VERSION_3;
    capHeader.pid = 0;

    // common user, clear all caps
    struct __user_cap_data_struct capData[CAP_NUM] = {0};
    for (unsigned int i = 0; i < capsCnt; ++i) {
        capData[CAP_TO_INDEX(caps[i])].effective |= CAP_TO_MASK(caps[i]);
        capData[CAP_TO_INDEX(caps[i])].permitted |= CAP_TO_MASK(caps[i]);
        capData[CAP_TO_INDEX(caps[i])].inheritable |= CAP_TO_MASK(caps[i]);
    }

    if (capset(&capHeader, capData) != 0) {
        APPSPAWN_LOGE("[appspawn] capset failed, err: %d.", errno);
        return -1;
    }
    for (unsigned int i = 0; i < capsCnt; ++i) {
        if (SetAmbientCapability(caps[i]) != 0) {
            APPSPAWN_LOGE("[appspawn] SetAmbientCapability failed, err: %d.", errno);
            return -1;
        }
    }
    return 0;
}

static int SetProcessName(struct AppSpawnContent_ *content, AppSpawnClient *client,
    char *longProcName, uint32_t longProcNameLen)
{
    AppSpawnClientLite *appProperty = (AppSpawnClientLite *)client;
    return prctl(PR_SET_NAME, appProperty->message.bundleName);
}

static int SetKeepCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    APPSPAWN_LOGE("SetKeepCapabilities");
#ifdef __LINUX__
    if (prctl(PR_SET_SECUREBITS, SECBIT_NO_SETUID_FIXUP | SECBIT_NO_SETUID_FIXUP_LOCKED)) {
        printf("prctl failed\n");
        return -1;
    }
#endif
    return 0;
}

static int SetUidGid(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientLite *appProperty = (AppSpawnClientLite *)client;
    APPSPAWN_LOGE("SetUidGid %d %d", appProperty->message.uID, appProperty->message.gID);
    if (setgid(appProperty->message.gID) != 0) {
        APPSPAWN_LOGE("[appspawn] setgid failed, gID %u, err: %d.", appProperty->message.gID, errno);
        return -1;
    }

    if (setuid(appProperty->message.uID) != 0) {
        APPSPAWN_LOGE("[appspawn] setuid failed, uID %u, err: %d.", appProperty->message.uID, errno);
        return -1;
    }
    gid_t groups[GRP_NUM];
    // add device groups for system app
    if (appProperty->message.gID >= LOWER_BOUND_GID && appProperty->message.gID <= UPPER_BOUND_GID) {
        groups[0] = appProperty->message.gID;
        groups[1] = DEVMGR_GRP;
        if (setgroups(GRP_NUM, groups)) {
            APPSPAWN_LOGE("[appspawn] setgroups failed, uID %u, err: %d.", appProperty->message.uID, errno);
            return -1;
        }
    }

    // umask call always succeeds and return the previous mask value which is not needed here
    (void)umask(DEFAULT_UMASK);
    return 0;
}

static int SetCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientLite *appProperty = (AppSpawnClientLite *)client;
    APPSPAWN_LOGE("SetCapabilities appProperty->message.capsCnt %d", appProperty->message.capsCnt);
    // set rlimit
#ifdef __LINUX__
    static const rlim_t DEFAULT_RLIMIT = 40;
    struct rlimit rlim = {0};
    rlim.rlim_cur = DEFAULT_RLIMIT;
    rlim.rlim_max = DEFAULT_RLIMIT;
    if (setrlimit(RLIMIT_NICE, &rlim) != 0) {
        APPSPAWN_LOGE("[appspawn] setrlimit failed, err: %d.", errno);
        return -1;
    }

#ifndef APPSPAWN_TEST
    unsigned int tmpCaps[] = {17};  // 17 means CAP_SYS_RAWIO
    unsigned int tmpsCapCnt = sizeof(tmpCaps) / sizeof(tmpCaps[0]);
    if (SetCapability(tmpsCapCnt, tmpCaps) != 0) {
        APPSPAWN_LOGE("[appspawn] setrlimit failed, err: %d.", errno);
        return -1;
    }
#endif
#else
    if (SetCapability(appProperty->message.capsCnt, appProperty->message.caps) != 0) {
        APPSPAWN_LOGE("[appspawn] SetCapability failed, err: %d.", errno);
        return -1;
    }
#endif  // __LINUX__
    APPSPAWN_LOGE("SetCapabilities appProperty->message.capsCnt %d", appProperty->message.capsCnt);
    return 0;
}

static void RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGI("AbilityMain");
    AppSpawnClientLite *appProperty = (AppSpawnClientLite *)client;
    APPSPAWN_LOGI("[appspawn] invoke, msg<%s,%s,%d,%d>",
        appProperty->message.bundleName, appProperty->message.identityID, appProperty->message.uID,
        appProperty->message.gID);

#ifndef APPSPAWN_TEST
    if (AbilityMain(appProperty->message.identityID) != 0) {
        APPSPAWN_LOGE("[appspawn] AbilityMain execute failed, pid %d.", getpid());
        exit(0x7f);  // 0x7f: user specified
    }
#endif
}

void SetContentFunction(AppSpawnContent *content)
{
    APPSPAWN_LOGI("SetContentFunction");
    content->setProcessName = SetProcessName;
    content->setKeepCapabilities = SetKeepCapabilities;
    content->setUidGid = SetUidGid;
    content->setCapabilities = SetCapabilities;
    content->runChildProcessor = RunChildProcessor;

    content->setFileDescriptors = NULL;
    content->setAppSandbox = NULL;
    content->setAppAccessToken = NULL;
    content->notifyResToParent = NULL;
    content->loadExtendLib = NULL;
    content->initAppSpawn = NULL;
    content->runAppSpawn = NULL;
    content->clearEnvironment = NULL;
}
