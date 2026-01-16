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
    if (path == nullptr) {
        printf("CheckDirRecursive path is null");
    }

    return true;
}

int SandboxMountPath(const MountArg *arg)
{
    if (arg == nullptr) {
        printf("SandboxMountPath is null");
    }

    return 0;
}

int BuildRootPath(char *buffer, uint32_t bufferLen, const AppSpawnSandboxCfg *sandbox, uid_t uid)
{
    if (buffer == nullptr) {
        printf("BuildRootPath buffer is null");
    }

    if (bufferLen == 0) {
        printf("BuildRootPath bufferLen is 0");
    }

    if (sandbox == nullptr) {
        printf("BuildRootPath sandbox is null");
    }

    if (uid == 0) {
        printf("BuildRootPath uid is 0");
    }

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
    if (context == nullptr) {
        printf("NeedNetworkIsolated context is null");
    }

    if (property == nullptr) {
        printf("NeedNetworkIsolated property is null");
    }

    return true;
}

int InitSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
                       const AppSpawningCtx *property, int nwebspawn)
{
    if (context == nullptr) {
        printf("InitSandboxContext context is null");
    }

    if (sandbox == nullptr) {
        printf("InitSandboxContext sandbox is null");
    }

    if (property == nullptr) {
        printf("InitSandboxContext property is null");
    }

    if (nwebspawn == 0) {
        printf("InitSandboxContext nwebspawn is 0");
    }

    return 0;
}

VarExtraData *GetVarExtraData(const SandboxContext *context, const SandboxSection *section)
{
    if (context == nullptr) {
        printf("GetVarExtraData contex is null");
    }

    if (section == nullptr) {
        printf("GetVarExtraData section is null");
    }

    return nullptr;
}

uint32_t GetMountArgs(const SandboxContext *context,
    const PathMountNode *sandboxNode, uint32_t operation, MountArg *args)
{
    if (context == nullptr) {
        printf("GetMountArgs context is null");
    }

    if (sandboxNode == nullptr) {
        printf("GetMountArgs sandboxNode is null");
    }

    if (operation == 0) {
        printf("GetMountArgs operation is 0");
    }

    if (args == nullptr) {
        printf("GetMountArgs args is null");
    }

    return 0;
}

int CheckSandboxMountNode(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation)
{
    if (context == nullptr) {
        printf("CheckSandboxMountNode context is null");
    }

    if (section == nullptr) {
        printf("CheckSandboxMountNode section is null");
    }

    if (sandboxNode == nullptr) {
        printf("CheckSandboxMountNode sandboxNode is null");
    }

    if (operation == 0) {
        printf("CheckSandboxMountNode operation is 0");
    }

    return 0;
}

int32_t SandboxMountFusePath(const SandboxContext *context, const MountArg *args)
{
    if (context == nullptr) {
        printf("SandboxMountFusePath context is null");
    }

    if (args == nullptr) {
        printf("SandboxMountFusePath args is null");
    }

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
    if (context == nullptr) {
        printf("CreateDemandSrc context is null");
    }

    if (sandboxNode == nullptr) {
        printf("CreateDemandSrc sandboxNode is null");
    }

    if (args == nullptr) {
        printf("CreateDemandSrc args is null");
    }
}

char *GetRealSrcPath(const SandboxContext *context, const char *source, VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("GetRealSrcPath context is null");
    }

    if (source == nullptr) {
        printf("GetRealSrcPath source is null");
    }

    if (extraData == nullptr) {
        printf("GetRealSrcPath extraData is null");
    }

    return "";
}

int32_t SetMountArgsOption(const SandboxContext *context, uint32_t category, uint32_t operation, MountArg *args)
{
    if (context == nullptr) {
        printf("SetMountArgsOption context is null");
    }

    if (category == 0) {
        printf("SetMountArgsOption category is 0");
    }

    if (operation == 0) {
        printf("SetMountArgsOption operation is 0");
    }

    if (args == nullptr) {
        printf("SetMountArgsOption args is null");
    }

    return 0;
}

int DoSandboxMountByCategory(const SandboxContext *context, const PathMountNode *sandboxNode,
    MountArg *args, uint32_t operation)
{
    if (context == nullptr) {
        printf("DoSandboxMountByCategory context is null");
    }

    if (sandboxNode == nullptr) {
        printf("DoSandboxMountByCategory sandboxNode is null");
    }

    if (args == nullptr) {
        printf("DoSandboxMountByCategory args is null");
    }

    if (operation == 0) {
        printf("DoSandboxMountByCategory operation is 0");
    }

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
    if (context == nullptr) {
        printf("SetDecPolicyWithCond context is null");
    }

    if (sandboxNode == nullptr) {
        printf("SetDecPolicyWithCond sandboxNode is null");
    }

    if (extraData == nullptr) {
        printf("SetDecPolicyWithCond extraData is null");
    }

    return 0;
}

int SetDecPolicyWithDir(const SandboxContext *context)
{
    if (context == nullptr) {
        printf("SetDecPolicyWithDir context is null");
    }

    return 0;
}

int DoSandboxPathNodeMount(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation)
{
    if (context == nullptr) {
        printf("DoSandboxPathNodeMount context is null");
    }

    if (section == nullptr) {
        printf("DoSandboxPathNodeMount section is null");
    }

    if (sandboxNode == nullptr) {
        printf("DoSandboxPathNodeMount sandboxNode is null");
    }

    if (operation == 0) {
        printf("DoSandboxPathNodeMount operation is null");
    }

    return 0;
}

int DoSandboxPathSymLink(const SandboxContext *context,
    const SandboxSection *section, const SymbolLinkNode *sandboxNode)
{
    if (context == nullptr) {
        printf("DoSandboxPathSymLink context is null");
    }

    if (section == nullptr) {
        printf("DoSandboxPathSymLink section is null");
    }

    if (sandboxNode == nullptr) {
        printf("DoSandboxPathSymLink sandboxNode is null");
    }

    return 0;
}

int DoSandboxNodeMount(const SandboxContext *context, const SandboxSection *section, uint32_t operation)
{
    if (context == nullptr) {
        printf("DoSandboxNodeMount context is null");
    }

    if (section == nullptr) {
        printf("DoSandboxNodeMount section is null");
    }

    if (operation == 0) {
        printf("DoSandboxNodeMount operation is 0");
    }

    return 0;
}

int UpdateMountPathDepsPath(const SandboxContext *context, SandboxNameGroupNode *groupNode)
{
    if (context == nullptr) {
        printf("UpdateMountPathDepsPath context is null");
    }

    if (groupNode == nullptr) {
        printf("UpdateMountPathDepsPath groupNode is null");
    }

    return 0;
}

bool CheckAndCreateDepPath(const SandboxContext *context, const SandboxNameGroupNode *groupNode)
{
    if (context == nullptr) {
        printf("CheckAndCreateDepPath context is null");
    }

    if (groupNode == nullptr) {
        printf("CheckAndCreateDepPath groupNode is null");
    }

    return false;
}

int MountSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
                       const SandboxSection *section, uint32_t op)
{
    if (context == nullptr) {
        printf("MountSandboxConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("MountSandboxConfig sandbox is null");
    }

    if (section == nullptr) {
        printf("MountSandboxConfig section is null");
    }

    if (op == 0) {
        printf("MountSandboxConfig op is 0");
    }

    return 0;
}

int SetExpandSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetExpandSandboxConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetExpandSandboxConfig sandbox is null");
    }

    return 0;
}

int SetSandboxPackageNameConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetSandboxPackageNameConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetSandboxPackageNameConfig sandbox is null");
    }

    return 0;
}

int SetSandboxSpawnFlagsConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetSandboxSpawnFlagsConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetSandboxSpawnFlagsConfig sandbox is null");
    }

    return 0;
}

int SetSandboxPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetSandboxPermissionConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetSandboxPermissionConfig sandbox is null");
    }

    return 0;
}

int SetOverlayAppSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetOverlayAppSandboxConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetOverlayAppSandboxConfig sandbox is null");
    }

    return 0;
}

int SetBundleResourceSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetBundleResourceSandboxConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetBundleResourceSandboxConfig sandbox is null");
    }

    return ret;
}

int32_t ChangeCurrentDir(const SandboxContext *context)
{
    if (context == nullptr) {
        printf("ChangeCurrentDir context is null");
    }

    return ret;
}

int SandboxRootFolderCreateNoShare(
    const SandboxContext *context, const AppSpawnSandboxCfg *sandbox, bool remountProc)
{
    if (context == nullptr) {
        printf("SandboxRootFolderCreateNoShare context is null");
    }

    if (sandbox == nullptr) {
        printf("SandboxRootFolderCreateNoShare sandbox is null");
    }

    if (remountProc == true) {
        printf("SandboxRootFolderCreateNoShare remountProc is null");
    }

    return 0;
}

int SandboxRootFolderCreate(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SandboxRootFolderCreate context is null");
    }

    if (sandbox == nullptr) {
        printf("SandboxRootFolderCreate sandbox is null");
    }

    return 0;
}

bool IsSandboxMounted(const AppSpawnSandboxCfg *sandbox, const char *name, const char *rootPath)
{
    if (sandbox == nullptr) {
        printf("IsSandboxMounted sandbox is null");
    }

    if (name == nullptr) {
        printf("IsSandboxMounted name is null");
    }

    if (rootPath == nullptr) {
        printf("IsSandboxMounted rootPath is null");
    }

    return true;
}

int SetSandboxMounted(const AppSpawnSandboxCfg *sandbox, const char *name, char *rootPath)
{
    if (sandbox == nullptr) {
        printf("SetSandboxMounted sandbox is null");
    }

    if (name == nullptr) {
        printf("SetSandboxMounted name is null");
    }

    if (rootPath == nullptr) {
        printf("SetSandboxMounted rootPath is null");
    }

    return 0;
}

void UnmountPath(char *rootPath, uint32_t len, const SandboxMountNode *sandboxNode)
{
    if (rootPath == nullptr) {
        printf("UnmountPath rootPath is nullptr");
    }

    if (len == 0) {
        printf("UnmountPath len is 0");
    }

    if (sandboxNode == nullptr) {
        printf("UnmountPath sandboxNode is null");
    }
}

int UnmountDepPaths(const AppSpawnSandboxCfg *sandbox, uid_t uid)
{
    if (sandbox == nullptr) {
        printf("UnmountDepPaths sandbox is null");
    }

    if (uid == 0) {
        printf("UnmountDepPaths uid is 0");
    }

    return 0;
}

int UnmountSandboxConfigs(const AppSpawnSandboxCfg *sandbox, uid_t uid, const char *name)
{
    if (sandbox == nullptr) {
        printf("UnmountSandboxConfigs sandbox is null");
    }

    if (uid == 0) {
        printf("UnmountSandboxConfigs uid is 0");
    }

    if (name == nullptr) {
        printf("UnmountSandboxConfigs name is null");
    }

    return 0;
}

bool IsADFPermission(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property)
{
    if (sandbox == nullptr) {
        printf("IsADFPermission sandbox is null");
    }

    if (property == nullptr) {
        printf("IsADFPermission property is null");
    }

    return true;
}

int StagedMountSystemConst(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
{
    if (sandbox == nullptr) {
        printf("StagedMountSystemConst sandbox is null");
    }

    if (property == nullptr) {
        printf("StagedMountSystemConst property is null");
    }

    if (nwebspawn == 0) {
        printf("StagedMountSystemConst nwebspawn is 0");
    }

    return 0;
}

int MountDepGroups(const SandboxContext *context, SandboxNameGroupNode *groupNode)
{
    if (context == nullptr) {
        printf("MountDepGroups context is null");
    }

    if (groupNode == nullptr) {
        printf("MountDepGroups groupNode is null");
    }

    return 0;
}

int SetSystemConstDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetSystemConstDepGroups context is null");
    }

    if (sandbox == nullptr) {
        printf("SetSystemConstDepGroups sandbox is null");
    }

    return 0;
}

int SetAppVariableDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetAppVariableDepGroups context is null");
    }

    if (sandbox == nullptr) {
        printf("SetAppVariableDepGroups sandbox is null");
    }

    return 0;
}

int SetSpawnFlagsDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetSpawnFlagsDepGroups context is null");
    }

    if (sandbox == nullptr) {
        printf("SetSpawnFlagsDepGroups sandbox is null");
    }

    return 0;
}

int SetPackageNameDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetPackageNameDepGroups context is null");
    }

    if (sandbox == nullptr) {
        printf("SetPackageNameDepGroups sandbox is null");
    }

    return 0;
}

int SetPermissionDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetPermissionDepGroups context is null");
    }

    if (sandbox == nullptr) {
        printf("SetPermissionDepGroups sandbox is null");
    }

    return 0;
}

int StagedDepGroupMounts(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("StagedDepGroupMounts context is null");
    }

    if (sandbox == nullptr) {
        printf("StagedDepGroupMounts sandbox is null");
    }

    return 0;
}

int StagedMountPreUnShare(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("StagedMountPreUnShare context is null");
    }

    if (sandbox == nullptr) {
        printf("StagedMountPreUnShare sandbox is null");
    }

    return 0;
}

int SetAppVariableConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("SetAppVariableConfig context is null");
    }

    if (sandbox == nullptr) {
        printf("SetAppVariableConfig sandbox is null");
    }

    return 0;
}

int StagedMountPostUnshare(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context == nullptr) {
        printf("StagedMountPostUnshare context is null");
    }

    if (sandbox == nullptr) {
        printf("StagedMountPostUnshare sandbox is null");
    }

    return 0;
}

int MountSandboxConfigs(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
{
    if (sandbox == nullptr) {
        printf("MountSandboxConfigs sandbox is null");
    }

    if (property == nullptr) {
        printf("MountSandboxConfigs property is null");
    }

    if (nwebspawn == 0) {
        printf("MountSandboxConfigs nwebspawn is 0");
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
