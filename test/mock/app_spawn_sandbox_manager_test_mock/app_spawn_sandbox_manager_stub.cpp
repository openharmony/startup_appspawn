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

#include "access_token.h"
#include "hilog/log.h"
#include "securec.h"
#include "token_setproc.h"
#include "tokenid_kit.h"

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif
#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#include <sys/prctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void FreePathMountNode(SandboxMountNode *node)
{
    if (node == nullptr) {
        printf("FreePathMountNode node is nullptr");
    }
}

void FreeSymbolLinkNode(SandboxMountNode *node)
{
    if (node == nullptr) {
        printf("FreeSymbolLinkNode node is nullptr");
    }
}

int SandboxNodeCompareProc(ListNode *node, ListNode *newNode)
{
    return 0;
}

SandboxMountNode *CreateSandboxMountNode(uint32_t dataLen, uint32_t type)
{
    return nullptr;
}

void AddSandboxMountNode(SandboxMountNode *node, SandboxSection *queue)
{
    if (node == nullptr) {
        printf("AddSandboxMountNode node is nullptr");
    }
}

int PathMountNodeCompare(ListNode *node, void *data)
{
    return 0;
}

int SymbolLinkNodeCompare(ListNode *node, void *data)
{
    return 0;
}

PathMountNode *GetPathMountNode(const SandboxSection *section, int type, const char *source, const char *target)
{
    return nullptr;
}

SymbolLinkNode *GetSymbolLinkNode(const SandboxSection *section, const char *target, const char *linkName)
{
    return nullptr;
}

void DeleteSandboxMountNode(SandboxMountNode *sandboxNode)
{
    if (sandboxNode == nullptr) {
        printf("DeleteSandboxMountNode sandboxNode is nullptr");
    }
}

SandboxMountNode *GetFirstSandboxMountNode(const SandboxSection *section)
{
    return nullptr;
}

void DumpSandboxMountNode(const SandboxMountNode *sandboxNode, uint32_t index)
{
    if (sandboxNode == nullptr) {
        printf("DumpSandboxMountNode sandboxNode is nullptr");
    }
}

void InitSandboxSection(SandboxSection *section, int type)
{
    if (section == nullptr) {
        printf("InitSandboxSection section is nullptr");
    }
}

void ClearSandboxSection(SandboxSection *section)
{
    if (section == nullptr) {
        printf("ClearSandboxSection section is nullptr");
    }
}

void DumpSandboxQueue(const ListNode *front,
    void (*dumpSandboxMountNode)(const SandboxMountNode *node, uint32_t count))
{
    if (node == nullptr) {
        printf("DumpSandboxQueue node is nullptr");
    }
}

void DumpSandboxSection(const SandboxSection *section)
{
    if (section == nullptr) {
        printf("DumpSandboxSection section is nullptr");
    }
}

SandboxSection *CreateSandboxSection(const char *name, uint32_t dataLen, uint32_t type)
{
    return nullptr;
}

int SandboxConditionalNodeCompareName(ListNode *node, void *data)
{
    return 0;
}

int SandboxConditionalNodeCompareNode(ListNode *node, ListNode *newNode)
{
    return 0;
}

SandboxSection *GetSandboxSection(const SandboxQueue *queue, const char *name)
{
    return nullptr;
}

void AddSandboxSection(SandboxSection *node, SandboxQueue *queue)
{
    if (node == nullptr) {
        printf("AddSandboxSection node is nullptr");
    }
}

void DeleteSandboxSection(SandboxSection *section)
{
    if (section == nullptr) {
        printf("DeleteSandboxSection section is nullptr");
    }
}

void SandboxQueueClear(SandboxQueue *queue)
{
    if (queue == nullptr) {
        printf("SandboxQueueClear queue is nullptr");
    }
}

int AppSpawnExtDataCompareDataId(ListNode *node, void *data)
{
    return 0;
}

AppSpawnSandboxCfg *GetAppSpawnSandbox(const AppSpawnMgr *content, ExtDataType type)
{
    return nullptr;
}

void DeleteAppSpawnSandbox(AppSpawnSandboxCfg *sandbox)
{
    if (sandbox == nullptr) {
        printf("DeleteAppSpawnSandbox sandbox is nullptr");
    }
}

void DumpSandboxPermission(const SandboxMountNode *node, uint32_t index)
{
    if (node == nullptr) {
        printf("DumpSandboxPermission node is nullptr");
    }
}

void DumpSandboxSectionNode(const SandboxMountNode *node, uint32_t index)
{
    if (node == nullptr) {
        printf("DumpSandboxSectionNode node is nullptr");
    }
}

void DumpSandboxNameGroupNode(const SandboxMountNode *node, uint32_t index)
{
    if (node == nullptr) {
        printf("DumpSandboxNameGroupNode node is nullptr");
    }
}

void DumpSandbox(struct TagAppSpawnExtData *data)
{
    if (data == nullptr) {
        printf("DumpSandbox data is nullptr");
    }
}

void InitSandboxQueue(SandboxQueue *queue, uint32_t type)
{
    if (queue == nullptr) {
        printf("InitSandboxQueue queue is nullptr");
    }
}

void FreeAppSpawnSandbox(struct TagAppSpawnExtData *data)
{
    if (data == nullptr) {
        printf("FreeAppSpawnSandbox data is nullptr");
    }
}

AppSpawnSandboxCfg *CreateAppSpawnSandbox(ExtDataType type)
{
    return nullptr;
}

void DumpAppSpawnSandboxCfg(AppSpawnSandboxCfg *sandbox)
{
    if (sandbox == nullptr) {
        printf("DumpAppSpawnSandboxCfg sandbox is nullptr");
    }
}

int PreLoadSandboxCfgByType(AppSpawnMgr *content, ExtDataType type)
{
    return 0;
}

int PreLoadDebugSandboxCfg(AppSpawnMgr *content)
{
    return 0;
}

int PreLoadIsoLatedSandboxCfg(AppSpawnMgr *content)
{
    return 0;
}

int PreLoadAppSandboxCfg(AppSpawnMgr *content)
{
    return 0;
}

int PreLoadNWebSandboxCfg(AppSpawnMgr *content)
{
    return 0;
}

int IsolatedSandboxHandleServerExit(AppSpawnMgr *content)
{
    return 0;
}

int SandboxHandleServerExit(AppSpawnMgr *content)
{
    return 0;
}

ExtDataType GetSandboxType(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return nullptr;
}

int SpawnBuildSandboxEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int AppendPermissionGid(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    return 0;
}

int AppendPackageNameGids(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    return 0;
}

void UpdateMsgFlagsWithPermission(AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property,
    const char *permissionMode, uint32_t flag)
{
    if (sandbox == nullptr) {
        printf("UpdateMsgFlagsWithPermission sandbox is nullptr");
    }
}

int UpdatePermissionFlags(AppSpawnMgr *content, AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    return 0;
}

int AppendGids(AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    return 0;
}

int SpawnPrepareSandboxCfg(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int SpawnMountDirToShared(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
