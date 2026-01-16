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
    if (node == nullptr) {
        printf("SandboxNodeCompareProc node is nullptr");
    }

    if (newNode == nullptr) {
        printf("SandboxNodeCompareProc node is nullptr");
    }

    return 0;
}

SandboxMountNode *CreateSandboxMountNode(uint32_t dataLen, uint32_t type)
{
    if (dataLen == 0) {
        printf("SandboxNodeCompareProc dataLen is 0");
    }

    if (type == 0) {
        printf("SandboxNodeCompareProc type is 0");
    }

    return nullptr;
}

void AddSandboxMountNode(SandboxMountNode *node, SandboxSection *queue)
{
    if (node == nullptr) {
        printf("AddSandboxMountNode node is nullptr");
    }

    if (queue == nullptr) {
        printf("AddSandboxMountNode queue is nullptr");
    }
}

int PathMountNodeCompare(ListNode *node, void *data)
{
    if (node == nullptr) {
        printf("PathMountNodeCompare node is null");
    }

    if (data == nullptr) {
        printf("PathMountNodeCompare data is null");
    }

    return 0;
}

int SymbolLinkNodeCompare(ListNode *node, void *data)
{
    if (node == nullptr) {
        printf("SymbolLinkNodeCompare node is null");
    }

    if (data == nullptr) {
        printf("SymbolLinkNodeCompare data is null");
    }

    return 0;
}

PathMountNode *GetPathMountNode(const SandboxSection *section, int type, const char *source, const char *target)
{
    if (section == nullptr) {
        printf("GetPathMountNode section is null");
    }

    if (type == 0) {
        printf("GetPathMountNode type is 0");
    }

    if (source == nullptr) {
        printf("GetPathMountNode source is null");
    }

    if (target == nullptr) {
        printf("GetPathMountNode target is null");
    }

    return nullptr;
}

SymbolLinkNode *GetSymbolLinkNode(const SandboxSection *section, const char *target, const char *linkName)
{
    if (section == nullptr) {
        printf("GetSymbolLinkNode section is null");
    }

    if (target == nullptr) {
        printf("GetSymbolLinkNode target is null");
    }

    if (linkName == nullptr) {
        printf("GetSymbolLinkNode linkName is null");
    }

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
    if (section == nullptr) {
        printf("GetFirstSandboxMountNode section is null");
    }

    return nullptr;
}

void DumpSandboxMountNode(const SandboxMountNode *sandboxNode, uint32_t index)
{
    if (sandboxNode == nullptr) {
        printf("DumpSandboxMountNode sandboxNode is nullptr");
    }

    if (index == 0) {
        printf("DumpSandboxMountNode index is 0");
    }
}

void InitSandboxSection(SandboxSection *section, int type)
{
    if (section == nullptr) {
        printf("InitSandboxSection section is nullptr");
    }

    if (type == 0) {
        printf("InitSandboxSection type is 0");
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
    if (front == nullptr) {
        printf("DumpSandboxQueue front is nullptr");
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
    if (name == nullptr) {
        printf("CreateSandboxSection name is null");
    }

    if (dataLen == 0) {
        printf("CreateSandboxSection dataLen is 0");
    }

    if (type == 0) {
        printf("CreateSandboxSection type is 0");
    }

    return nullptr;
}

int SandboxConditionalNodeCompareName(ListNode *node, void *data)
{
    if (node == nullptr) {
        printf("SandboxConditionalNodeCompareName");
    }

    if (data == nullptr) {
        printf("SandboxConditionalNodeCompareName");
    }

    return 0;
}

int SandboxConditionalNodeCompareNode(ListNode *node, ListNode *newNode)
{
    if (node == nullptr) {
        printf("SandboxConditionalNodeCompareNode node is null");
    }

    if (newNode == nullptr) {
        printf("SandboxConditionalNodeCompareNode newNode is null");
    }

    return 0;
}

SandboxSection *GetSandboxSection(const SandboxQueue *queue, const char *name)
{
    if (queue == nullptr) {
        printf("GetSandboxSection queue is null");
    }

    if (name == nullptr) {
        printf("GetSandboxSection name is null");
    }

    return nullptr;
}

void AddSandboxSection(SandboxSection *node, SandboxQueue *queue)
{
    if (node == nullptr) {
        printf("AddSandboxSection node is null");
    }

    if (queue == nullptr) {
        printf("AddSandboxSection queue is null");
    }
}

void DeleteSandboxSection(SandboxSection *section)
{
    if (section == nullptr) {
        printf("DeleteSandboxSection section is null");
    }
}

void SandboxQueueClear(SandboxQueue *queue)
{
    if (queue == nullptr) {
        printf("SandboxQueueClear queue is null");
    }
}

int AppSpawnExtDataCompareDataId(ListNode *node, void *data)
{
    if (node == nullptr) {
        printf("AppSpawnExtDataCompareDataId node is null");
    }

    if (data == nullptr) {
        printf("AppSpawnExtDataCompareDataId data is null");
    }

    return 0;
}

AppSpawnSandboxCfg *GetAppSpawnSandbox(const AppSpawnMgr *content, ExtDataType type)
{
    if (content == nullptr) {
        printf("GetAppSpawnSandbox content is null");
    }

    if (type == EXT_DATA_APP_SANDBOX) {
        printf("GetAppSpawnSandbox type is EXT_DATA_APP_SANDBOX");
    }

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

    if (index == 0) {
        printf("DumpSandboxPermission index is 0");
    }
}

void DumpSandboxSectionNode(const SandboxMountNode *node, uint32_t index)
{
    if (node == nullptr) {
        printf("DumpSandboxSectionNode node is nullptr");
    }

    if (index == 0) {
        printf("DumpSandboxSectionNode index is 0");
    }
}

void DumpSandboxNameGroupNode(const SandboxMountNode *node, uint32_t index)
{
    if (node == nullptr) {
        printf("DumpSandboxNameGroupNode node is nullptr");
    }

    if (index == 0) {
        printf("DumpSandboxNameGroupNode index is 0");
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

    if (type == EXT_DATA_APP_SANDBOX) {
        printf("InitSandboxQueue type is EXT_DATA_APP_SANDBOX");
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
    if (type == EXT_DATA_APP_SANDBOX) {
        printf("CreateAppSpawnSandbox type is EXT_DATA_APP_SANDBOX");
    }

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
    if (content == nullptr) {
        printf("PreLoadSandboxCfgByType content is null");
    }

    if (type == EXT_DATA_APP_SANDBOX) {
        printf("PreLoadSandboxCfgByType type is EXT_DATA_APP_SANDBOX");
    }

    return 0;
}

int PreLoadDebugSandboxCfg(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("PreLoadDebugSandboxCfg content is null");
    }

    return 0;
}

int PreLoadIsoLatedSandboxCfg(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("PreLoadIsoLatedSandboxCfg content is null");
    }

    return 0;
}

int PreLoadAppSandboxCfg(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("PreLoadAppSandboxCfg content is null");
    }

    return 0;
}

int PreLoadNWebSandboxCfg(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("PreLoadNWebSandboxCfg content is null");
    }

    return 0;
}

int IsolatedSandboxHandleServerExit(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("IsolatedSandboxHandleServerExit content is null");
    }

    return 0;
}

int SandboxHandleServerExit(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("SandboxHandleServerExit content is null");
    }

    return 0;
}

ExtDataType GetSandboxType(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("GetSandboxType content is null");
    }

    if (property == nullptr) {
        printf("GetSandboxType property is null");
    }

    return nullptr;
}

int SpawnBuildSandboxEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnBuildSandboxEnv content is null");
    }

    if (property == nullptr) {
        printf("SpawnBuildSandboxEnv property is null");
    }

    return 0;
}

int AppendPermissionGid(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("AppendPermissionGid content is null");
    }

    if (property == nullptr) {
        printf("AppendPermissionGid property is null");
    }

    return 0;
}

int AppendPackageNameGids(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("AppendPackageNameGids content is null");
    }

    if (property == nullptr) {
        printf("AppendPackageNameGids property is null");
    }

    return 0;
}

void UpdateMsgFlagsWithPermission(AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property,
    const char *permissionMode, uint32_t flag)
{
    if (sandbox == nullptr) {
        printf("UpdateMsgFlagsWithPermission sandbox is nullptr");
    }

    if (property == nullptr) {
        printf("UpdateMsgFlagsWithPermission property is nullptr");
    }

    if (permissionMode == nullptr) {
        printf("UpdateMsgFlagsWithPermission permissionMode is nullptr");
    }

    if (flag == 0) {
        printf("UpdateMsgFlagsWithPermission flag is 0");
    }
}

int UpdatePermissionFlags(AppSpawnMgr *content, AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("UpdatePermissionFlags content is null");
    }

    if (sandbox == nullptr) {
        printf("UpdatePermissionFlags sandbox is null");
    }

    if (property == nullptr) {
        printf("UpdatePermissionFlags property is null");
    }

    return 0;
}

int AppendGids(AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    if (sandbox == nullptr) {
        printf("AppendGids sandbox is null");
    }

    if (property == nullptr) {
        printf("AppendGids property is null");
    }

    return 0;
}

int SpawnPrepareSandboxCfg(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnPrepareSandboxCfg content is null");
    }

    if (property == nullptr) {
        printf("SpawnPrepareSandboxCfg property is null");
    }

    return 0;
}

int SpawnMountDirToShared(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("SpawnMountDirToShared content is null");
    }

    if (property == nullptr) {
        printf("SpawnMountDirToShared property is null");
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
