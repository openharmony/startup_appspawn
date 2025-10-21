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

#include <linux/capability.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

bool CheckDirRecursive(const char *path)
{
    return true;
}

int SandboxMountPath(const MountArg *arg)
{
    return 0;
}

int BuildRootPath(char *buffer, uint32_t bufferLen, const AppSpawnSandboxCfg *sandbox, uid_t uid)
{
    return 0;
}

SandboxContext *GetSandboxContext(void)
{
    return nullptr;
}

void DeleteSandboxContext(SandboxContext **context)
{
    if (context == nullptr) {
        printf("DeleteSandboxContext context is nullptr");
    }
}

bool NeedNetworkIsolated(SandboxContext *context, const AppSpawningCtx *property)
{
    return true;
}

int InitSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
                       const AppSpawningCtx *property, int nwebspawn)
{
    return 0;
}

VarExtraData *GetVarExtraData(const SandboxContext *context, const SandboxSection *section)
{
    return nullptr;
}

uint32_t GetMountArgs(const SandboxContext *context,
    const PathMountNode *sandboxNode, uint32_t operation, MountArg *args)
{
    return 0;
}

int CheckSandboxMountNode(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation)
{
    return 0;
}

int32_t SandboxMountFusePath(const SandboxContext *context, const MountArg *args)
{
    return 0;
}

void CheckAndCreateSandboxFile(const char *file)
{
    if (file == nullptr) {
        printf("CheckAndCreateSandboxFile file is nullptr");
    }
}

void CreateDemandSrc(const SandboxContext *context, const PathMountNode *sandboxNode,
    const MountArg *args)
{
    if (args == nullptr) {
        printf("CreateDemandSrc args is nullptr");
    }
}

char *GetRealSrcPath(const SandboxContext *context, const char *source, VarExtraData *extraData)
{
    return "";
}

int32_t SetMountArgsOption(const SandboxContext *context, uint32_t category, uint32_t operation, MountArg *args)
{
    return 0;
}

int DoSandboxMountByCategory(const SandboxContext *context, const PathMountNode *sandboxNode,
    MountArg *args, uint32_t operation)
{
    return 0;
}

void FreeDecPolicyPaths(DecPolicyInfo *decPolicyInfo)
{
    if (decPolicyInfo == nullptr) {
        printf("FreeDecPolicyPaths decPolicyInfo is nullptr");
    }
}

int SetDecPolicyWithCond(const SandboxContext *context, const PathMountNode *sandboxNode,
    VarExtraData *extraData)
{
    return 0;
}

int SetDecPolicyWithDir(const SandboxContext *context)
{
    return 0;
}

int DoSandboxPathNodeMount(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation)
{
    return 0;
}

int DoSandboxPathSymLink(const SandboxContext *context,
    const SandboxSection *section, const SymbolLinkNode *sandboxNode)
{
    return 0;
}

int DoSandboxNodeMount(const SandboxContext *context, const SandboxSection *section, uint32_t operation)
{
    return 0;
}

int UpdateMountPathDepsPath(const SandboxContext *context, SandboxNameGroupNode *groupNode)
{
    return 0;
}

bool CheckAndCreateDepPath(const SandboxContext *context, const SandboxNameGroupNode *groupNode)
{
    return false;
}

int MountSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
                       const SandboxSection *section, uint32_t op)
{
    return 0;
}

int SetExpandSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetSandboxPackageNameConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetSandboxSpawnFlagsConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetSandboxPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetOverlayAppSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetBundleResourceSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return ret;
}

int32_t ChangeCurrentDir(const SandboxContext *context)
{
    return ret;
}

int SandboxRootFolderCreateNoShare(
    const SandboxContext *context, const AppSpawnSandboxCfg *sandbox, bool remountProc)
{
    return 0;
}

int SandboxRootFolderCreate(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

bool IsSandboxMounted(const AppSpawnSandboxCfg *sandbox, const char *name, const char *rootPath)
{
    return true;
}

int SetSandboxMounted(const AppSpawnSandboxCfg *sandbox, const char *name, char *rootPath)
{
    return 0;
}

void UnmountPath(char *rootPath, uint32_t len, const SandboxMountNode *sandboxNode)
{
    if (rootPath == nullptr) {
        printf("UnmountPath rootPath is nullptr");
    }
}

int UnmountDepPaths(const AppSpawnSandboxCfg *sandbox, uid_t uid)
{
    return 0;
}

int UnmountSandboxConfigs(const AppSpawnSandboxCfg *sandbox, uid_t uid, const char *name)
{
    return 0;
}

bool IsADFPermission(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property)
{
    return true;
}

int StagedMountSystemConst(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
{
    return 0;
}

int MountDepGroups(const SandboxContext *context, SandboxNameGroupNode *groupNode)
{
    return 0;
}

int SetSystemConstDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetAppVariableDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetSpawnFlagsDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetPackageNameDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetPermissionDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int StagedDepGroupMounts(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int StagedMountPreUnShare(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int SetAppVariableConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int StagedMountPostUnshare(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

int MountSandboxConfigs(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
