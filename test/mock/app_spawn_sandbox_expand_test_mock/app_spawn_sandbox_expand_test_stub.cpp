/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "app_spawn_stub.h"

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdbool>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>


#ifdef __cplusplus
extern "C" {
#endif

int MountAllHsp(const SandboxContext *context, const cJSON *hsps)
{
    if (context == nullptr) {
        printf("MountAllHsp context is null");
    }

    if (hsps == nullptr) {
        printf("MountAllHsp hsps is null");
    }

    return 0;
}

int MountAllGroup(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox,
    const cJSON *groups)
{
    if (context == nullptr) {
        printf("MountAllGroup context is null");
    }

    if (appSandbox == nullptr) {
        printf("MountAllGroup appSandbox is null");
    }

    if (groups == nullptr) {
        printf("MountAllGroup groups is null");
    }

    return 0;
}

int SetOverlayAppPath(const char *hapPath, void *context)
{
    if (hapPath == nullptr) {
        printf("SetOverlayAppPath hapPath is null");
    }

    if (context == nullptr) {
        printf("SetOverlayAppPath context is null");
    }

    return 0;
}

int SetOverlayAppSandboxConfig(const SandboxContext *context, const char *overlayInfo)
{
    if (context == nullptr) {
        printf("SetOverlayAppSandboxConfig context is null");
    }

    if (overlayInfo == nullptr) {
        printf("SetOverlayAppSandboxConfig overlayInfo is null");
    }

    return 0;
}

cJSON *GetJsonObjFromProperty(const SandboxContext *context, const char *name)
{
    if (context == nullptr) {
        printf("GetJsonObjFromProperty context is null");
    }

    if (name == nullptr) {
        printf("GetJsonObjFromProperty name is null");
    }

    return nullptr;
}

int ProcessHSPListConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    if (context == nullptr) {
        printf("ProcessHSPListConfig context is null");
    }

    if (appSandbox == nullptr) {
        printf("ProcessHSPListConfig appSandbox is null");
    }

    if (name == nullptr) {
        printf("ProcessHSPListConfig name is null");
    }

    return 0;
}

int ProcessDataGroupConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    if (context == nullptr) {
        printf("ProcessDataGroupConfig context is null");
    }

    if (appSandbox == nullptr) {
        printf("ProcessDataGroupConfig appSandbox is null");
    }

    if (name == nullptr) {
        printf("ProcessDataGroupConfig name is null");
    }

    return 0;
}

int ProcessOverlayAppConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    if (context == nullptr) {
        printf("ProcessOverlayAppConfig context is null");
    }

    if (appSandbox == nullptr) {
        printf("ProcessOverlayAppConfig appSandbox is null");
    }

    if (name == nullptr) {
        printf("ProcessOverlayAppConfig name is null");
    }

    return 0;
}

int AppSandboxExpandAppCfgCompareName(ListNode *node, void *data)
{
    if (node == nullptr) {
        printf("AppSandboxExpandAppCfgCompareName node is null");
    }

    if (data == nullptr) {
        printf("AppSandboxExpandAppCfgCompareName data is null");
    }

    return 0;
}

int AppSandboxExpandAppCfgComparePrio(ListNode *node1, ListNode *node2)
{
    if (node1 == nullptr) {
        printf("AppSandboxExpandAppCfgComparePrio node1 is null");
    }

    if (node2 == nullptr) {
        printf("AppSandboxExpandAppCfgComparePrio node2 is null");
    }

    return 0;
}

AppSandboxExpandAppCfgNode *GetAppSandboxExpandAppCfg(const char *name)
{
    if (name == nullptr) {
        printf("GetAppSandboxExpandAppCfg name is null");
    }

    return nullptr;
}

int RegisterExpandSandboxCfgHandler(const char *name, int prio, ProcessExpandSandboxCfg handleExpandCfg)
{
    if (name == nullptr) {
        printf("RegisterExpandSandboxCfgHandler name is null");
    }

    if (prio == nullptr) {
        printf("RegisterExpandSandboxCfgHandler prio is null");
    }

    if (handleExpandCfg == nullptr) {
        printf("RegisterExpandSandboxCfgHandler handleExpandCfg is null");
    }

    return 0;
}

void AddDefaultExpandAppSandboxConfigHandle(void)
{
    printf("AddDefaultExpandAppSandboxConfigHandle");
}

void ClearExpandAppSandboxConfigHandle(void)
{
    printf("ClearExpandAppSandboxConfigHandle");
}

#ifdef __cplusplus
}
#endif
