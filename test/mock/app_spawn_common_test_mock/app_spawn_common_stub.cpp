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

int SetProcessName(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SetProcessName content is null");
    }

    if (property == nullptr) {
        printf("SetProcessName property is null");
    }

    return 0;
}

int SetKeepCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SetKeepCapabilities content is null");
    }

    if (property == nullptr) {
        printf("SetKeepCapabilities property is null");
    }

    return 0;
}

int SetAmbientCapability(int cap)
{
    if (cap == 0) {
        printf("SetAmbientCapability cap is 0");
    }

    return 0;
}

int SetAmbientCapabilities(const AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("SetAmbientCapabilities property is null");
    }

    return 0;
}

int SetCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SetCapabilities content is null");
    }

    if (property == nullptr) {
        printf("SetCapabilities property is null");
    }

    return 0;
}

void InitDebugParams(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("InitDebugParams node is null");
    }

    if (property == nullptr) {
        printf("InitDebugParams property is null");
    }
}

void ClearEnvironment(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("ClearEnvironment node is null");
    }

    if (property == nullptr) {
        printf("ClearEnvironment property is null");
    }
}

int SetXpmConfig(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SetXpmConfig content is null");
    }

    if (property == nullptr) {
        printf("SetXpmConfig property is null");
    }
    return 0;
}

int SetUidGid(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SetUidGid content is null");
    }

    if (property == nullptr) {
        printf("SetUidGid property is null");
    }
    return 0;
}

int32_t SetFileDescriptors(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SetFileDescriptors content is null");
    }

    if (property == nullptr) {
        printf("SetFileDescriptors property is null");
    }

    return 0;
}

int32_t CheckTraceStatus(void)
{
    return 0;
}

int32_t WaitForDebugger(const AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("WaitForDebugger property is null");
    }

    return 0;
}

int SpawnSetPreInstalledFlag(AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("SpawnSetPreInstalledFlag property is null");
    }

    return 0;
}

int SpawnInitSpawningEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnInitSpawningEnv content is null");
    }

    if (property == nullptr) {
        printf("SpawnInitSpawningEnv property is null");
    }

    return 0;
}

int SpawnSetAppEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnSetAppEnv content is null");
    }

    if (property == nullptr) {
        printf("SpawnSetAppEnv property is null");
    }

    return 0;
}

int SpawnEnableCache(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnEnableCache content is null");
    }

    if (property == nullptr) {
        printf("SpawnEnableCache property is null");
    }
    return 0;
}

void SpawnLoadSilk(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnLoadSilk node is null");
    }

    if (property == nullptr) {
        printf("SpawnLoadSilk property is null");
    }
}

int FilterAppSpawnTrace(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("FilterAppSpawnTrace content is null");
    }

    if (property == nullptr) {
        printf("FilterAppSpawnTrace property is null");
    }

    return 0;
}

int SpawnSetProperties(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnSetProperties content is null");
    }

    if (property == nullptr) {
        printf("SpawnSetProperties property is null");
    }
    return 0;
}

int PreLoadSetSeccompFilter(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("PreLoadSetSeccompFilter content is null");
    }

    return 0;
}

int SpawnComplete(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnComplete content is null");
    }

    if (property == nullptr) {
        printf("SpawnComplete property is null");
    }

    return 0;
}

int CheckEnabled(const char *param, const char *value)
{
    if (param == nullptr) {
        printf("CheckEnabled param is null");
    }

    if (value == nullptr) {
        printf("CheckEnabled value is null");
    }

    return 0;
}

int SpawnGetSpawningFlag(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnGetSpawningFlag content is null");
    }

    if (property == nullptr) {
        printf("SpawnGetSpawningFlag property is null");
    }

    return 0;
}

int SpawnLoadConfig(AppSpawnMgr *content)
{
    if () {
        printf("SpawnLoadConfig ");
    }

    return 0;
}

int SpawnLoadSeLinuxConfig(AppSpawnMgr *content)
{
    if () {
        printf("SpawnLoadSeLinuxConfig ");
    }

    return 0;
}

int CloseFdArgs(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("CloseFdArgs content is null");
    }

    if (property == nullptr) {
        printf("CloseFdArgs property is null");
    }

    return 0;
}

int SetFdEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SetFdEnv content is null");
    }
    
    if (property == nullptr) {
        printf("SetFdEnv property is null");
    }

    return 0;
}

int RecordStartTime(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("RecordStartTime content is null");
    }

    if (property == nullptr) {
        printf("RecordStartTime property is null");
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
