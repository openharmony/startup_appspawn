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

int InitDebugSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
    const AppSpawningCtx *property, int nwebspawn)
{
    return 0;
}

void UmountAndRmdirDir(const char *targetPath)
{
    if (targetPath == nullptr) {
        printf("UmountAndRmdirDir targetPath is nullptr");
    }
}

int RemoveDebugBaseConfig(SandboxSection *section, const char *debugRootPath)
{
    return 0;
}

int RemoveDebugAppVarConfig(const AppSpawnSandboxCfg *sandboxCfg, const char *debugRootPath)
{
    return 0;
}

int RemoveDebugPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandboxCfg,
    const char *debugRootPath)
{
    return 0;
}

int ConvertUserIdPath(const AppSpawningCtx *property, char *debugRootPath, char *debugTmpRootPath)
{
    return 0;
}

int UninstallPrivateDirs(const AppSpawnMgr *content, const AppSpawningCtx *property,
    RemoveDebugDirInfo *removeDebugDirInfo)
{
    return 0;
}

int UninstallAllDirs(const AppSpawnMgr *content, const AppSpawningCtx *property,
    RemoveDebugDirInfo *removeDebugDirInfo)
{
    return 0;
}

int UninstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int SetDebugAppVarConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetDebugPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetDebugAutomicTmpRootPath(SandboxContext *context, const AppSpawningCtx *property)
{
    return 0;
}

int InitDebugSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
    const AppSpawningCtx *property, int nwebspawn)
{
    return 0;
}

int MountDebugTmpConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int MountDebugDirBySharefs(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int InstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
