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
    return 0;
}

int MountAllGroup(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox,
    const cJSON *groups)
{
    return 0;
}

int SetOverlayAppPath(const char *hapPath, void *context)
{
    return 0;
}

int SetOverlayAppSandboxConfig(const SandboxContext *context, const char *overlayInfo)
{
    return 0;
}

cJSON *GetJsonObjFromProperty(const SandboxContext *context, const char *name)
{
    return nullptr;
}

int ProcessHSPListConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    return 0;
}

int ProcessDataGroupConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    return 0;
}

int ProcessOverlayAppConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    return 0;
}

int AppSandboxExpandAppCfgCompareName(ListNode *node, void *data)
{
    return 0;
}

int AppSandboxExpandAppCfgComparePrio(ListNode *node1, ListNode *node2)
{
    return 0;
}

AppSandboxExpandAppCfgNode *GetAppSandboxExpandAppCfg(const char *name)
{
    return nullptr;
}

int RegisterExpandSandboxCfgHandler(const char *name, int prio, ProcessExpandSandboxCfg handleExpandCfg)
{
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
