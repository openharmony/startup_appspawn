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

#ifndef APPSPAWN_TEST_STUB_H
#define APPSPAWN_TEST_STUB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "cJSON.h"
#include "appspawn_client.h"
#include "appspawn_hook.h"
#include "appspawn_encaps.h"

#ifdef __cplusplus
extern "C" {
#endif

bool CheckDirRecursive(const char *path);
int SandboxMountPath(const MountArg *arg);
int BuildRootPath(char *buffer, uint32_t bufferLen, const AppSpawnSandboxCfg *sandbox, uid_t uid);
SandboxContext *GetSandboxContext(void);
void DeleteSandboxContext(SandboxContext **context);
bool NeedNetworkIsolated(SandboxContext *context, const AppSpawningCtx *property);
int InitSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
                       const AppSpawningCtx *property, int nwebspawn);
VarExtraData *GetVarExtraData(const SandboxContext *context, const SandboxSection *section);
uint32_t GetMountArgs(const SandboxContext *context,
    const PathMountNode *sandboxNode, uint32_t operation, MountArg *args);
int CheckSandboxMountNode(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation);
int32_t SandboxMountFusePath(const SandboxContext *context, const MountArg *args);
void CheckAndCreateSandboxFile(const char *file);
void CreateDemandSrc(const SandboxContext *context, const PathMountNode *sandboxNode,
    const MountArg *args);
char *GetRealSrcPath(const SandboxContext *context, const char *source, VarExtraData *extraData);
int32_t SetMountArgsOption(const SandboxContext *context, uint32_t category, uint32_t operation, MountArg *args);
int DoSandboxMountByCategory(const SandboxContext *context, const PathMountNode *sandboxNode,
    MountArg *args, uint32_t operation);
void FreeDecPolicyPaths(DecPolicyInfo *decPolicyInfo);
int SetDecPolicyWithCond(const SandboxContext *context, const PathMountNode *sandboxNode,
    VarExtraData *extraData);
int SetDecPolicyWithDir(const SandboxContext *context);
int DoSandboxPathNodeMount(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation);
int DoSandboxPathSymLink(const SandboxContext *context,
    const SandboxSection *section, const SymbolLinkNode *sandboxNode);
int DoSandboxNodeMount(const SandboxContext *context, const SandboxSection *section, uint32_t operation);
int UpdateMountPathDepsPath(const SandboxContext *context, SandboxNameGroupNode *groupNode);
bool CheckAndCreateDepPath(const SandboxContext *context, const SandboxNameGroupNode *groupNode);
int MountSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
                       const SandboxSection *section, uint32_t op);
int SetExpandSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int SetSandboxSpawnFlagsConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int SetSandboxPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int SetOverlayAppSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int SetBundleResourceSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int SandboxRootFolderCreateNoShare(
    const SandboxContext *context, const AppSpawnSandboxCfg *sandbox, bool remountProc);
bool IsSandboxMounted(const AppSpawnSandboxCfg *sandbox, const char *name, const char *rootPath);
void UnmountPath(char *rootPath, uint32_t len, const SandboxMountNode *sandboxNode);
int UnmountDepPaths(const AppSpawnSandboxCfg *sandbox, uid_t uid);
int UnmountSandboxConfigs(const AppSpawnSandboxCfg *sandbox, uid_t uid, const char *name);
bool IsADFPermission(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property);
int StagedMountSystemConst(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn);
int MountDepGroups(const SandboxContext *context, SandboxNameGroupNode *groupNode);
int SetSystemConstDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox);
int SetAppVariableDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox);
int SetSpawnFlagsDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox);
int SetPackageNameDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox);
int StagedDepGroupMounts(const SandboxContext *context, AppSpawnSandboxCfg *sandbox);
int StagedMountPreUnShare(const SandboxContext *context, AppSpawnSandboxCfg *sandbox);
int SetAppVariableConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int StagedMountPostUnshare(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int MountSandboxConfigs(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
