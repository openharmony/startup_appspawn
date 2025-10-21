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

void FreePathMountNode(SandboxMountNode *node);
void FreeSymbolLinkNode(SandboxMountNode *node);
int SandboxNodeCompareProc(ListNode *node, ListNode *newNode);
SandboxMountNode *CreateSandboxMountNode(uint32_t dataLen, uint32_t type);
void AddSandboxMountNode(SandboxMountNode *node, SandboxSection *queue);
int PathMountNodeCompare(ListNode *node, void *data);
int SymbolLinkNodeCompare(ListNode *node, void *data);
PathMountNode *GetPathMountNode(const SandboxSection *section, int type, const char *source, const char *target);
SymbolLinkNode *GetSymbolLinkNode(const SandboxSection *section, const char *target, const char *linkName);
void DeleteSandboxMountNode(SandboxMountNode *sandboxNode);
SandboxMountNode *GetFirstSandboxMountNode(const SandboxSection *section);
void DumpSandboxMountNode(const SandboxMountNode *sandboxNode, uint32_t index);
void InitSandboxSection(SandboxSection *section, int type);
void ClearSandboxSection(SandboxSection *section);
void DumpSandboxQueue(const ListNode *front,
    void (*dumpSandboxMountNode)(const SandboxMountNode *node, uint32_t count));
void DumpSandboxSection(const SandboxSection *section);
SandboxSection *CreateSandboxSection(const char *name, uint32_t dataLen, uint32_t type);
int SandboxConditionalNodeCompareName(ListNode *node, void *data);
int SandboxConditionalNodeCompareNode(ListNode *node, ListNode *newNode);
SandboxSection *GetSandboxSection(const SandboxQueue *queue, const char *name);
void AddSandboxSection(SandboxSection *node, SandboxQueue *queue);
void DeleteSandboxSection(SandboxSection *section);
void SandboxQueueClear(SandboxQueue *queue);
int AppSpawnExtDataCompareDataId(ListNode *node, void *data);
AppSpawnSandboxCfg *GetAppSpawnSandbox(const AppSpawnMgr *content, ExtDataType type);
void DeleteAppSpawnSandbox(AppSpawnSandboxCfg *sandbox);
void DumpSandboxPermission(const SandboxMountNode *node, uint32_t index);
void DumpSandboxSectionNode(const SandboxMountNode *node, uint32_t index);
void DumpSandboxNameGroupNode(const SandboxMountNode *node, uint32_t index);
void DumpSandbox(struct TagAppSpawnExtData *data);
void InitSandboxQueue(SandboxQueue *queue, uint32_t type);
void FreeAppSpawnSandbox(struct TagAppSpawnExtData *data);
AppSpawnSandboxCfg *CreateAppSpawnSandbox(ExtDataType type);
void DumpAppSpawnSandboxCfg(AppSpawnSandboxCfg *sandbox);
int PreLoadSandboxCfgByType(AppSpawnMgr *content, ExtDataType type);
int PreLoadDebugSandboxCfg(AppSpawnMgr *content);
int PreLoadIsoLatedSandboxCfg(AppSpawnMgr *content);
int PreLoadAppSandboxCfg(AppSpawnMgr *content);
int PreLoadNWebSandboxCfg(AppSpawnMgr *content);
int IsolatedSandboxHandleServerExit(AppSpawnMgr *content);
int SandboxHandleServerExit(AppSpawnMgr *content);
ExtDataType GetSandboxType(AppSpawnMgr *content, AppSpawningCtx *property);
int SpawnBuildSandboxEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int AppendPermissionGid(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property);
int AppendPackageNameGids(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property);
void UpdateMsgFlagsWithPermission(AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property,
    const char *permissionMode, uint32_t flag);
int UpdatePermissionFlags(AppSpawnMgr *content, AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property);
int AppendGids(AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property);
int SpawnPrepareSandboxCfg(AppSpawnMgr *content, AppSpawningCtx *property);
int SpawnMountDirToShared(AppSpawnMgr *content, AppSpawningCtx *property);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
