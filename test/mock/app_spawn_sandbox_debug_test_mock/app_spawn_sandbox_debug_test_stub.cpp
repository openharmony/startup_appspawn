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
    if (context == nullptr) {
        printf("InitDebugSandboxContext context is null");
    }

    if (sandbox == nullptr) {
        printf("InitDebugSandboxContext sandbox is null");
    }

    if (property == nullptr) {
        printf("InitDebugSandboxContext property is null");
    }

    if (nwebspawn == 0) {
        printf("InitDebugSandboxContext nwebspawn is 0");
    }

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
    if (section == nullptr) {
        printf("RemoveDebugBaseConfig section is null");
    }

    if (debugRootPath == nullptr) {
        printf("RemoveDebugBaseConfig debugRootPath is null");
    }

    return 0;
}

int RemoveDebugAppVarConfig(const AppSpawnSandboxCfg *sandboxCfg, const char *debugRootPath)
{
    if (sandboxCfg == nullptr) {
        printf("RemoveDebugAppVarConfig sandboxCfg is null");
    }

    if (debugRootPath == nullptr) {
        printf("RemoveDebugAppVarConfig debugRootPath is null");
    }

    return 0;
}

int RemoveDebugPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandboxCfg,
    const char *debugRootPath)
{
    if (context == nullptr) {
        printf("RemoveDebugPermissionConfig context is null");
    }

    if (sandboxCfg == nullptr) {
        printf("RemoveDebugPermissionConfig sandboxCfg is null");
    }

    if (debugRootPath == nullptr) {
        printf("RemoveDebugPermissionConfig debugRootPath is null");
    }

    return 0;
}

int ConvertUserIdPath(const AppSpawningCtx *property, char *debugRootPath, char *debugTmpRootPath)
{
    if (property == nullptr) {
        printf("ConvertUserIdPath property is null");
    }

    if (debugRootPath == nullptr) {
        printf("ConvertUserIdPath debugRootPath is null");
    }

    if (debugTmpRootPath == nullptr) {
        printf("ConvertUserIdPath debugTmpRootPath is null");
    }

    return 0;
}

int UninstallPrivateDirs(const AppSpawnMgr *content, const AppSpawningCtx *property,
    RemoveDebugDirInfo *removeDebugDirInfo)
{
    if (content == nullptr) {
        printf("UninstallPrivateDirs content is null");
    }

    if (property == nullptr) {
        printf("UninstallPrivateDirs property is null");
    }

    if (removeDebugDirInfo == nullptr) {
        printf("UninstallPrivateDirss removeDebugDirInfo is null");
    }

    return 0;
}

int UninstallAllDirs(const AppSpawnMgr *content, const AppSpawningCtx *property,
    RemoveDebugDirInfo *removeDebugDirInfo)
{
    if (content == nullptr) {
        printf("UninstallAllDirs content is null");
    }

    if (property == nullptr) {
        printf("UninstallAllDirs property is null");
    }

    if (removeDebugDirInfo == nullptr) {
        printf("UninstallAllDirs removeDebugDirInfo is null");
    }

    return 0;
}

int UninstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("UninstallDebugSandbox content is null");
    }

    if (property == nullptr) {
        printf("UninstallDebugSandbox property is null");
    }

    return 0;
}

int SetDebugAppVarConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetDebugAppVarConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetDebugAppVarConfig sandbox is null");
    }

    return 0;
}

int SetDebugPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetDebugPermissionConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetDebugPermissionConfig sandbox is null");
    }

    return 0;
}

int SetDebugAutomicTmpRootPath(SandboxContext *context, const AppSpawningCtx *property)
{
    if (context == nullptr) {
        printf("SetDebugAutomicTmpRootPath context is null");
    }

    if (property == nullptr) {
        printf("SetDebugAutomicTmpRootPath property is null");
    }

    return 0;
}

int InitDebugSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
    const AppSpawningCtx *property, int nwebspawn)
{
    if (context == nullptr) {
        printf("InitDebugSandboxContext context is null");
    }

    if (sandbox == nullptr) {
        printf("InitDebugSandboxContext sandbox is null");
    }

    if (property == nullptr) {
        printf("InitDebugSandboxContext property is null");
    }

    if (nwebspawn == 0) {
        printf("InitDebugSandboxContext nwebspawn is 0");
    }

    return 0;
}

int MountDebugTmpConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("MountDebugTmpConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("MountDebugTmpConfig sandbox is null");
    }

    return 0;
}

int MountDebugDirBySharefs(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("MountDebugDirBySharefs context is null");
    }

    if (sandbox == nullptr) {
        printf("MountDebugDirBySharefs sandbox is null");
    }

    return 0;
}

int InstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("InstallDebugSandbox content is null");
    }

    if (property == nullptr) {
        printf("InstallDebugSandbox property is null");
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
