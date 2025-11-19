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
    return 0;
}

int SetKeepCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    return 0;
}

int SetAmbientCapability(int cap)
{
    return 0;
}

int SetAmbientCapabilities(const AppSpawningCtx *property)
{
    return 0;
}

int SetCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    return 0;
}

void InitDebugParams(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("InitDebugParams node is null");
    }
}

void ClearEnvironment(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("ClearEnvironment node is null");
    }
}

int SetXpmConfig(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    return 0;
}

int SetUidGid(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    return 0;
}

int32_t SetFileDescriptors(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    return 0;
}

int32_t CheckTraceStatus(void)
{
    return 0;
}

int32_t WaitForDebugger(const AppSpawningCtx *property)
{
    return 0;
}

int SpawnSetPreInstalledFlag(AppSpawningCtx *property)
{
    return 0;
}

int SpawnInitSpawningEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int SpawnSetAppEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int SpawnEnableCache(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

void SpawnLoadSilk(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnLoadSilk node is null");
    }
}

int FilterAppSpawnTrace(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int SpawnSetProperties(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int PreLoadSetSeccompFilter(AppSpawnMgr *content)
{
    return 0;
}

int SpawnComplete(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int CheckEnabled(const char *param, const char *value)
{
    return 0;
}

int SpawnGetSpawningFlag(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int SpawnLoadConfig(AppSpawnMgr *content)
{
    return 0;
}

int SpawnLoadSeLinuxConfig(AppSpawnMgr *content)
{
    return 0;
}

int CloseFdArgs(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int SetFdEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int RecordStartTime(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
