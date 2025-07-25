/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <sched.h>

#include "securec.h"
#include "appspawn_manager.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "modulemgr.h"
#include "parameter.h"
#include "sandbox_shared.h"

static void FreePathMountNode(SandboxMountNode *node)
{
    PathMountNode *sandboxNode = (PathMountNode *)node;
    if (sandboxNode->source) {
        free(sandboxNode->source);
        sandboxNode->source = NULL;
    }
    if (sandboxNode->target) {
        free(sandboxNode->target);
        sandboxNode->target = NULL;
    }
    if (sandboxNode->appAplName) {
        free(sandboxNode->appAplName);
        sandboxNode->appAplName = NULL;
    }
    for (uint32_t i = 0; i < sandboxNode->decPolicyPaths.decPathCount; i++) {
        if (sandboxNode->decPolicyPaths.decPath[i] != NULL) {
            free(sandboxNode->decPolicyPaths.decPath[i]);
            sandboxNode->decPolicyPaths.decPath[i] = NULL;
        }
    }
    sandboxNode->decPolicyPaths.decPathCount = 0;
    free(sandboxNode->decPolicyPaths.decPath);
    sandboxNode->decPolicyPaths.decPath = NULL;
    free(sandboxNode);
}

static void FreeSymbolLinkNode(SandboxMountNode *node)
{
    SymbolLinkNode *sandboxNode = (SymbolLinkNode *)node;
    if (sandboxNode->target) {
        free(sandboxNode->target);
        sandboxNode->target = NULL;
    }
    if (sandboxNode->linkName) {
        free(sandboxNode->linkName);
        sandboxNode->linkName = NULL;
    }
    free(sandboxNode);
}

static int SandboxNodeCompareProc(ListNode *node, ListNode *newNode)
{
    SandboxMountNode *sandbox1 = (SandboxMountNode *)ListEntry(node, SandboxMountNode, node);
    SandboxMountNode *sandbox2 = (SandboxMountNode *)ListEntry(newNode, SandboxMountNode, node);
    return sandbox1->type - sandbox2->type;
}

SandboxMountNode *CreateSandboxMountNode(uint32_t dataLen, uint32_t type)
{
    APPSPAWN_CHECK(dataLen >= sizeof(SandboxMountNode) && dataLen <= sizeof(PathMountNode),
        return NULL, "Invalid dataLen %{public}u", dataLen);
    SandboxMountNode *node = (SandboxMountNode *)calloc(1, dataLen);
    APPSPAWN_CHECK(node != NULL, return NULL, "Failed to create mount node %{public}u", type);
    OH_ListInit(&node->node);
    node->type = type;
    return node;
}

void AddSandboxMountNode(SandboxMountNode *node, SandboxSection *queue)
{
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return);
    APPSPAWN_CHECK_ONLY_EXPER(queue != NULL, return);
    OH_ListAddWithOrder(&queue->front, &node->node, SandboxNodeCompareProc);
}

static int PathMountNodeCompare(ListNode *node, void *data)
{
    PathMountNode *node1 = (PathMountNode *)ListEntry(node, SandboxMountNode, node);
    PathMountNode *node2 = (PathMountNode *)data;
    return (node1->sandboxNode.type == node2->sandboxNode.type) &&
        (strcmp(node1->source, node2->source) == 0) &&
        (strcmp(node1->target, node2->target) == 0) ? 0 : 1;
}

static int SymbolLinkNodeCompare(ListNode *node, void *data)
{
    SymbolLinkNode *node1 = (SymbolLinkNode *)ListEntry(node, SandboxMountNode, node);
    SymbolLinkNode *node2 = (SymbolLinkNode *)data;
    return (node1->sandboxNode.type == node2->sandboxNode.type) &&
        (strcmp(node1->target, node2->target) == 0) &&
        (strcmp(node1->linkName, node2->linkName) == 0) ? 0 : 1;
}

PathMountNode *GetPathMountNode(const SandboxSection *section, int type, const char *source, const char *target)
{
    APPSPAWN_CHECK_ONLY_EXPER(section != NULL, return NULL);
    APPSPAWN_CHECK_ONLY_EXPER(source != NULL && target != NULL, return NULL);
    PathMountNode pathNode = {};
    pathNode.sandboxNode.type = type;
    pathNode.source = (char *)source;
    pathNode.target = (char *)target;
    ListNode *node = OH_ListFind(&section->front, (void *)&pathNode, PathMountNodeCompare);
    if (node == NULL) {
        return NULL;
    }
    return (PathMountNode *)ListEntry(node, SandboxMountNode, node);
}

SymbolLinkNode *GetSymbolLinkNode(const SandboxSection *section, const char *target, const char *linkName)
{
    APPSPAWN_CHECK_ONLY_EXPER(section != NULL, return NULL);
    APPSPAWN_CHECK_ONLY_EXPER(linkName != NULL && target != NULL, return NULL);
    SymbolLinkNode linkNode = {};
    linkNode.sandboxNode.type = SANDBOX_TAG_SYMLINK;
    linkNode.target = (char *)target;
    linkNode.linkName = (char *)linkName;
    ListNode *node = OH_ListFind(&section->front, (void *)&linkNode, SymbolLinkNodeCompare);
    if (node == NULL) {
        return NULL;
    }
    return (SymbolLinkNode *)ListEntry(node, SandboxMountNode, node);
}

void DeleteSandboxMountNode(SandboxMountNode *sandboxNode)
{
    APPSPAWN_CHECK_ONLY_EXPER(sandboxNode != NULL, return);
    OH_ListRemove(&sandboxNode->node);
    OH_ListInit(&sandboxNode->node);
    switch (sandboxNode->type) {
        case SANDBOX_TAG_MOUNT_PATH:
        case SANDBOX_TAG_MOUNT_FILE:
            FreePathMountNode(sandboxNode);
            break;
        case SANDBOX_TAG_SYMLINK:
            FreeSymbolLinkNode(sandboxNode);
            break;
        default:
            APPSPAWN_LOGE("Invalid type %{public}u", sandboxNode->type);
            free(sandboxNode);
            break;
    }
}

SandboxMountNode *GetFirstSandboxMountNode(const SandboxSection *section)
{
    if (section == NULL || ListEmpty(section->front)) {
        return NULL;
    }
    return (SandboxMountNode *)ListEntry(section->front.next, SandboxMountNode, node);
}

void DumpSandboxMountNode(const SandboxMountNode *sandboxNode, uint32_t index)
{
    APPSPAWN_CHECK_ONLY_EXPER(sandboxNode != NULL, return);
    switch (sandboxNode->type) {
        case SANDBOX_TAG_MOUNT_PATH:
        case SANDBOX_TAG_MOUNT_FILE: {
            PathMountNode *pathNode = (PathMountNode *)sandboxNode;
            APPSPAWN_DUMP("        ****************************** %{public}u", index);
            APPSPAWN_DUMP("        sandbox node source: %{public}s", pathNode->source ? pathNode->source : "null");
            APPSPAWN_DUMP("        sandbox node target: %{public}s", pathNode->target ? pathNode->target : "null");
            DumpMountPathMountNode(pathNode);
            APPSPAWN_DUMP("        sandbox node apl: %{public}s",
                pathNode->appAplName ? pathNode->appAplName : "null");
            APPSPAWN_DUMP("        sandbox node checkErrorFlag: %{public}s",
                pathNode->checkErrorFlag ? "true" : "false");
            break;
        }
        case SANDBOX_TAG_SYMLINK: {
            SymbolLinkNode *linkNode = (SymbolLinkNode *)sandboxNode;
            APPSPAWN_DUMP("        ***********************************");
            APPSPAWN_DUMP("        sandbox node target: %{public}s", linkNode->target ? linkNode->target : "null");
            APPSPAWN_DUMP("        sandbox node linkName: %{public}s",
                linkNode->linkName ? linkNode->linkName : "null");
            APPSPAWN_DUMP("        sandbox node destMode: %{public}x", linkNode->destMode);
            APPSPAWN_DUMP("        sandbox node checkErrorFlag: %{public}s",
                linkNode->checkErrorFlag ? "true" : "false");
            break;
        }
        default:
            break;
    }
}

static inline void InitSandboxSection(SandboxSection *section, int type)
{
    OH_ListInit(&section->front);
    section->sandboxSwitch = 0;
    section->sandboxShared = 0;
    section->number = 0;
    section->gidCount = 0;
    section->gidTable = NULL;
    section->nameGroups = NULL;
    section->name = NULL;
    OH_ListInit(&section->sandboxNode.node);
    section->sandboxNode.type = type;
}

static void ClearSandboxSection(SandboxSection *section)
{
    if (section->gidTable) {
        free(section->gidTable);
        section->gidTable = NULL;
    }
    // free name group
    if (section->nameGroups) {
        free(section->nameGroups);
        section->nameGroups = NULL;
    }
    if (section->name) {
        free(section->name);
        section->name = NULL;
    }
    if (section->sandboxNode.type == SANDBOX_TAG_NAME_GROUP) {
        SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)section;
        if (groupNode->depNode) {
            DeleteSandboxMountNode((SandboxMountNode *)groupNode->depNode);
        }
    }
    // free mount path
    ListNode *node = section->front.next;
    while (node != &section->front) {
        SandboxMountNode *sandboxNode = ListEntry(node, SandboxMountNode, node);
        // delete node
        OH_ListRemove(&sandboxNode->node);
        OH_ListInit(&sandboxNode->node);
        DeleteSandboxMountNode(sandboxNode);
        // get next
        node = section->front.next;
    }
}

static void DumpSandboxQueue(const ListNode *front,
    void (*dumpSandboxMountNode)(const SandboxMountNode *node, uint32_t count))
{
    uint32_t count = 0;
    ListNode *node = front->next;
    while (node != front) {
        SandboxMountNode *sandboxNode = (SandboxMountNode *)ListEntry(node, SandboxMountNode, node);
        count++;
        dumpSandboxMountNode(sandboxNode, count);
        // get next
        node = node->next;
    }
}

static void DumpSandboxSection(const SandboxSection *section)
{
    APPSPAWN_DUMP("    sandboxSwitch %{public}s", section->sandboxSwitch ? "true" : "false");
    APPSPAWN_DUMP("    sandboxShared %{public}s", section->sandboxShared ? "true" : "false");
    APPSPAWN_DUMP("    gidCount: %{public}u", section->gidCount);
    for (uint32_t index = 0; index < section->gidCount; index++) {
        APPSPAWN_DUMP("        gidTable[%{public}u]: %{public}u", index, section->gidTable[index]);
    }
    APPSPAWN_DUMP("    mount group count: %{public}u", section->number);
    for (uint32_t i = 0; i < section->number; i++) {
        if (section->nameGroups[i]) {
            SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)section->nameGroups[i];
            APPSPAWN_DUMP("        name[%{public}d] %{public}s", i, groupNode->section.name);
        }
    }
    APPSPAWN_DUMP("    mount-paths: ");
    DumpSandboxQueue(&section->front, DumpSandboxMountNode);
}

SandboxSection *CreateSandboxSection(const char *name, uint32_t dataLen, uint32_t type)
{
    APPSPAWN_CHECK(type < SANDBOX_TAG_INVALID && type >= SANDBOX_TAG_PERMISSION,
        return NULL, "Invalid type %{public}u", type);
    APPSPAWN_CHECK(name != NULL && strlen(name) > 0, return NULL, "Invalid name %{public}u", type);
    APPSPAWN_CHECK(dataLen >= sizeof(SandboxSection), return NULL, "Invalid dataLen %{public}u", dataLen);
    APPSPAWN_CHECK(dataLen <= sizeof(SandboxNameGroupNode), return NULL, "Invalid dataLen %{public}u", dataLen);
    SandboxSection *section = (SandboxSection *)calloc(1, dataLen);
    APPSPAWN_CHECK(section != NULL, return NULL, "Failed to create base node");
    InitSandboxSection(section, type);
    section->name = strdup(name);
    if (section->name == NULL) {
        ClearSandboxSection(section);
        free(section);
        return NULL;
    }
    return section;
}

static int SandboxConditionalNodeCompareName(ListNode *node, void *data)
{
    SandboxSection *tmpNode = (SandboxSection *)ListEntry(node, SandboxMountNode, node);
    return strcmp(tmpNode->name, (char *)data);
}

static int SandboxConditionalNodeCompareNode(ListNode *node, ListNode *newNode)
{
    SandboxSection *tmpNode = (SandboxSection *)ListEntry(node, SandboxMountNode, node);
    SandboxSection *tmpNewNode = (SandboxSection *)ListEntry(newNode, SandboxMountNode, node);
    return strcmp(tmpNode->name, tmpNewNode->name);
}

SandboxSection *GetSandboxSection(const SandboxQueue *queue, const char *name)
{
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL && queue != NULL, return NULL);
    ListNode *node = OH_ListFind(&queue->front, (void *)name, SandboxConditionalNodeCompareName);
    if (node == NULL) {
        return NULL;
    }
    return (SandboxSection *)ListEntry(node, SandboxMountNode, node);
}

void AddSandboxSection(SandboxSection *node, SandboxQueue *queue)
{
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL && queue != NULL, return);
    if (ListEmpty(node->sandboxNode.node)) {
        OH_ListAddWithOrder(&queue->front, &node->sandboxNode.node, SandboxConditionalNodeCompareNode);
    }
}

void DeleteSandboxSection(SandboxSection *section)
{
    APPSPAWN_CHECK_ONLY_EXPER(section != NULL, return);
    // delete node
    OH_ListRemove(&section->sandboxNode.node);
    OH_ListInit(&section->sandboxNode.node);
    ClearSandboxSection(section);
    free(section);
}

static void SandboxQueueClear(SandboxQueue *queue)
{
    ListNode *node = queue->front.next;
    while (node != &queue->front) {
        SandboxSection *sandboxNode = (SandboxSection *)ListEntry(node, SandboxMountNode, node);
        DeleteSandboxSection(sandboxNode);
        // get first
        node = queue->front.next;
    }
}

static int AppSpawnExtDataCompareDataId(ListNode *node, void *data)
{
    AppSpawnExtData *extData = (AppSpawnExtData *)ListEntry(node, AppSpawnExtData, node);
    return extData->dataId - *(uint32_t *)data;
}

AppSpawnSandboxCfg *GetAppSpawnSandbox(const AppSpawnMgr *content, ExtDataType type)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return NULL);
    uint32_t dataId = type;
    ListNode *node = OH_ListFind(&content->extData, (void *)&dataId, AppSpawnExtDataCompareDataId);
    if (node == NULL) {
        return NULL;
    }
    return (AppSpawnSandboxCfg *)ListEntry(node, AppSpawnSandboxCfg, extData);
}

void DeleteAppSpawnSandbox(AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_CHECK_ONLY_EXPER(sandbox != NULL, return);
    APPSPAWN_LOGV("DeleteAppSpawnSandbox");
    OH_ListRemove(&sandbox->extData.node);
    OH_ListInit(&sandbox->extData.node);

    // delete all queue
    SandboxQueueClear(&sandbox->requiredQueue);
    SandboxQueueClear(&sandbox->permissionQueue);
    SandboxQueueClear(&sandbox->packageNameQueue);
    SandboxQueueClear(&sandbox->spawnFlagsQueue);
    SandboxQueueClear(&sandbox->nameGroupsQueue);
    if (sandbox->rootPath) {
        free(sandbox->rootPath);
    }
    free(sandbox->depGroupNodes);
    sandbox->depGroupNodes = NULL;
    free(sandbox);
    sandbox = NULL;
}

static void DumpSandboxPermission(const SandboxMountNode *node, uint32_t index)
{
    SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)node;
    APPSPAWN_DUMP("    ========================================= ");
    APPSPAWN_DUMP("    Section %{public}s", permissionNode->section.name);
    APPSPAWN_DUMP("    Section permission index %{public}d", permissionNode->permissionIndex);
    DumpSandboxSection(&permissionNode->section);
}

static void DumpSandboxSectionNode(const SandboxMountNode *node, uint32_t index)
{
    SandboxSection *section = (SandboxSection *)node;
    APPSPAWN_DUMP("    ========================================= ");
    APPSPAWN_DUMP("    Section %{public}s", section->name);
    DumpSandboxSection(section);
}

static void DumpSandboxNameGroupNode(const SandboxMountNode *node, uint32_t index)
{
    SandboxNameGroupNode *nameGroupNode = (SandboxNameGroupNode *)node;
    APPSPAWN_DUMP("    ========================================= ");
    APPSPAWN_DUMP("    Section %{public}s", nameGroupNode->section.name);
    APPSPAWN_DUMP("    Section dep mode %{public}s",
        nameGroupNode->depMode == MOUNT_MODE_ALWAYS ? "always" : "not-exists");
    if (nameGroupNode->depNode != NULL) {
        APPSPAWN_DUMP("    mount-paths-deps: ");
        DumpMountPathMountNode(nameGroupNode->depNode);
    }
    DumpSandboxSection(&nameGroupNode->section);
}


static void DumpSandbox(struct TagAppSpawnExtData *data)
{
    AppSpawnSandboxCfg *sandbox = (AppSpawnSandboxCfg *)data;
    DumpAppSpawnSandboxCfg(sandbox);
}

static inline void InitSandboxQueue(SandboxQueue *queue, uint32_t type)
{
    OH_ListInit(&queue->front);
    queue->type = type;
}

static void FreeAppSpawnSandbox(struct TagAppSpawnExtData *data)
{
    AppSpawnSandboxCfg *sandbox = ListEntry(data, AppSpawnSandboxCfg, extData);
    // delete all var
    DeleteAppSpawnSandbox(sandbox);
}

AppSpawnSandboxCfg *CreateAppSpawnSandbox(ExtDataType type)
{
    // create sandbox
    AppSpawnSandboxCfg *sandbox = (AppSpawnSandboxCfg *)calloc(1, sizeof(AppSpawnSandboxCfg));
    APPSPAWN_CHECK(sandbox != NULL, return NULL, "Failed to create sandbox");

    // ext data init
    OH_ListInit(&sandbox->extData.node);
    sandbox->extData.dataId = type;
    sandbox->extData.freeNode = FreeAppSpawnSandbox;
    sandbox->extData.dumpNode = DumpSandbox;

    // queue
    InitSandboxQueue(&sandbox->requiredQueue, SANDBOX_TAG_REQUIRED);
    InitSandboxQueue(&sandbox->permissionQueue, SANDBOX_TAG_PERMISSION);
    InitSandboxQueue(&sandbox->packageNameQueue, SANDBOX_TAG_PACKAGE_NAME);
    InitSandboxQueue(&sandbox->spawnFlagsQueue, SANDBOX_TAG_SPAWN_FLAGS);
    InitSandboxQueue(&sandbox->nameGroupsQueue, SANDBOX_TAG_NAME_GROUP);

    sandbox->topSandboxSwitch = 0;
    sandbox->appFullMountEnable = 0;
    sandbox->topSandboxSwitch = 0;
    sandbox->pidNamespaceSupport = 0;
    sandbox->sandboxNsFlags = 0;
    sandbox->maxPermissionIndex = -1;
    sandbox->depNodeCount = 0;
    sandbox->depGroupNodes = NULL;

    AddDefaultVariable();
    AddDefaultExpandAppSandboxConfigHandle();
    return sandbox;
}

void DumpAppSpawnSandboxCfg(AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_CHECK_ONLY_EXPER(sandbox != NULL, return);
    APPSPAWN_DUMP("Sandbox root path: %{public}s", sandbox->rootPath);
    APPSPAWN_DUMP("Sandbox sandboxNsFlags: %{public}x ", sandbox->sandboxNsFlags);
    APPSPAWN_DUMP("Sandbox topSandboxSwitch: %{public}s", sandbox->topSandboxSwitch ? "true" : "false");
    APPSPAWN_DUMP("Sandbox appFullMountEnable: %{public}s", sandbox->appFullMountEnable ? "true" : "false");
    APPSPAWN_DUMP("Sandbox pidNamespaceSupport: %{public}s", sandbox->pidNamespaceSupport ? "true" : "false");
    APPSPAWN_DUMP("Sandbox common info: ");
    DumpSandboxQueue(&sandbox->requiredQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->packageNameQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->permissionQueue.front, DumpSandboxPermission);
    DumpSandboxQueue(&sandbox->spawnFlagsQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->nameGroupsQueue.front, DumpSandboxNameGroupNode);
}

static int PreLoadSandboxCfgByType(AppSpawnMgr *content, ExtDataType type)
{
    if (type >= EXT_DATA_COUNT || type < EXT_DATA_APP_SANDBOX) {
        return APPSPAWN_ARG_INVALID;
    }

    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content, type);
    APPSPAWN_CHECK(sandbox == NULL, return 0, "type %{public}d sandbox has been load", type);

    sandbox = CreateAppSpawnSandbox(type);
    APPSPAWN_CHECK_ONLY_EXPER(sandbox != NULL, return APPSPAWN_SYSTEM_ERROR);
    OH_ListAddTail(&content->extData, &sandbox->extData.node);

    // load sandbox config by type
    LoadAppSandboxConfig(sandbox, type);
    sandbox->maxPermissionIndex = PermissionRenumber(&sandbox->permissionQueue);

    content->content.sandboxNsFlags = 0;
    if (IsNWebSpawnMode(content) || sandbox->pidNamespaceSupport) {
        content->content.sandboxNsFlags = sandbox->sandboxNsFlags;
    }
    return 0;
}

APPSPAWN_STATIC int PreLoadDebugSandboxCfg(AppSpawnMgr *content)
{
    if (IsNWebSpawnMode(content)) {
        return 0;
    }

    int ret = PreLoadSandboxCfgByType(content, EXT_DATA_DEBUG_HAP_SANDBOX);
    APPSPAWN_CHECK(ret == 0, return ret, "debug hap sandbox cfg preload failed");
    return 0;
}

APPSPAWN_STATIC int PreLoadIsoLatedSandboxCfg(AppSpawnMgr *content)
{
    if (IsNWebSpawnMode(content)) {
        return 0;
    }

    int ret = PreLoadSandboxCfgByType(content, EXT_DATA_ISOLATED_SANDBOX);
    APPSPAWN_CHECK(ret == 0, return ret, "isolated sandbox cfg preload failed");
    return 0;
}

APPSPAWN_STATIC int PreLoadAppSandboxCfg(AppSpawnMgr *content)
{
    if (IsNWebSpawnMode(content)) {
        return 0;
    }

    int ret = PreLoadSandboxCfgByType(content, EXT_DATA_APP_SANDBOX);
    APPSPAWN_CHECK(ret == 0, return ret, "app sandbox cfg preload failed");
    return 0;
}

APPSPAWN_STATIC int PreLoadNWebSandboxCfg(AppSpawnMgr *content)
{
    if (IsNWebSpawnMode(content)) {
        int ret = PreLoadSandboxCfgByType(content, EXT_DATA_RENDER_SANDBOX);
        APPSPAWN_CHECK(ret == 0, return ret, "render sandbox cfg preload failed");

        ret = PreLoadSandboxCfgByType(content, EXT_DATA_GPU_SANDBOX);
        APPSPAWN_CHECK(ret == 0, return ret, "gpu sandbox cfg preload failed");
    }
    return 0;
}

APPSPAWN_STATIC int IsolatedSandboxHandleServerExit(AppSpawnMgr *content)
{
    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content, EXT_DATA_ISOLATED_SANDBOX);
    APPSPAWN_CHECK(sandbox != NULL, return 0, "Isolated sandbox not load");

    return 0;
}

APPSPAWN_STATIC int SandboxHandleServerExit(AppSpawnMgr *content)
{
    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content, EXT_DATA_APP_SANDBOX);
    APPSPAWN_CHECK(sandbox != NULL, return 0, "Sandbox not load");

    return 0;
}

static ExtDataType GetSandboxType(AppSpawnMgr *content, AppSpawningCtx *property)
{
    ExtDataType type = EXT_DATA_APP_SANDBOX;
    if (IsNWebSpawnMode(content)) {
        char *processType = (char *)GetAppPropertyExt(property, MSG_EXT_NAME_PROCESS_TYPE, NULL);
        APPSPAWN_CHECK(processType != NULL, return type, "Invalid processType data");
        if (strcmp(processType, "render") == 0) {
            type = EXT_DATA_RENDER_SANDBOX;
        } else if (strcmp(processType, "gpu") == 0) {
            type = EXT_DATA_GPU_SANDBOX;
        }
    } else {
        type = CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ? EXT_DATA_ISOLATED_SANDBOX :
                                                                                EXT_DATA_APP_SANDBOX;
    }
    return type;
}

/**
 * 构建沙箱环境有以下条件
 * 1.如果是孵化普通hap，ExtDataType为EXT_DATA_APP_SANDBOX
 * 2.如果是孵化普通hap且hap中携带APP_FLAGS_ISOLATED_SANDBOX_TYPE标志位，ExtDataType为EXT_DATA_ISOLATED_SANDBOX
 * 3.如果是孵化render进程，ExtDataType为EXT_DATA_RENDER_SANDBOX
 * 4.如果是孵化gpu进程，ExtDataType为EXT_DATA_GPU_SANDBOX
 * 5.如果孵化hap过程中，携带APP_FLAG_DEBUGABLE标志位同时开启了开发者模式，需增量配置ExtDataType为EXT_DATA_DEBUG_HAP_SANDBOX
 */

static int SpawnBuildSandboxEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    // Don't build sandbox env
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_NO_SANDBOX)) {
        return 0;
    }

    ExtDataType type = GetSandboxType(content, property);
    AppSpawnSandboxCfg *appSandbox = GetAppSpawnSandbox(content, type);
    APPSPAWN_CHECK(appSandbox != NULL, return APPSPAWN_SANDBOX_INVALID, "Failed to get sandbox for %{public}s",
                   GetProcessName(property));
    content->content.sandboxType = type;

    // CLONE_NEWPID 0x20000000
    // CLONE_NEWNET 0x40000000
    if ((content->content.sandboxNsFlags & CLONE_NEWPID) == CLONE_NEWPID) {
        int ret = getprocpid();
        if (ret < 0) {
            APPSPAWN_LOGE("getprocpid failed, ret: %{public}d errno: %{public}d", ret, errno);
            return ret;
        }
    }
    int ret = MountSandboxConfigs(appSandbox, property, IsNWebSpawnMode(content));
    appSandbox->mounted = 1;
    // for module test do not create sandbox, use APP_FLAGS_IGNORE_SANDBOX to ignore sandbox result
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_IGNORE_SANDBOX)) {
        APPSPAWN_LOGW("Do not care sandbox result %{public}d", ret);
        return 0;
    }
    return ret == 0 ? 0 : APPSPAWN_SANDBOX_MOUNT_FAIL;
}

static int AppendPermissionGid(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, GetProcessName(property));

    APPSPAWN_LOGV("AppendPermissionGid %{public}s", GetProcessName(property));
    ListNode *node = sandbox->permissionQueue.front.next;
    while (node != &sandbox->permissionQueue.front) {
        SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
        if (!CheckAppPermissionFlagSet(property, (uint32_t)permissionNode->permissionIndex)) {
            node = node->next;
            continue;
        }
        if (permissionNode->section.gidCount == 0) {
            node = node->next;
            continue;
        }
        APPSPAWN_LOGV("Add permission %{public}s gid %{public}d to %{public}s",
            permissionNode->section.name, permissionNode->section.gidTable[0], GetProcessName(property));

        size_t copyLen = permissionNode->section.gidCount;
        if ((permissionNode->section.gidCount + dacInfo->gidCount) > APP_MAX_GIDS) {
            APPSPAWN_LOGW("More gid for %{public}s msg count %{public}u permission %{public}u",
                GetProcessName(property), dacInfo->gidCount, permissionNode->section.gidCount);
            copyLen = APP_MAX_GIDS - dacInfo->gidCount;
        }
        int ret = memcpy_s(&dacInfo->gidTable[dacInfo->gidCount], sizeof(gid_t) * copyLen,
            permissionNode->section.gidTable, sizeof(gid_t) * copyLen);
        if (ret != EOK) {
            APPSPAWN_LOGW("Failed to append permission %{public}s gid to %{public}s",
                permissionNode->section.name, GetProcessName(property));
            node = node->next;
            continue;
        }
        dacInfo->gidCount += copyLen;
        node = node->next;
    }
    return 0;
}

static int AppendPackageNameGids(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, GetProcessName(property));
    
    SandboxPackageNameNode *sandboxNode =
        (SandboxPackageNameNode *)GetSandboxSection(&sandbox->packageNameQueue, GetProcessName(property));
    if (sandboxNode == NULL || sandboxNode->section.gidCount == 0) {
        return 0;
    }

    size_t copyLen = sandboxNode->section.gidCount;
    if ((sandboxNode->section.gidCount + dacInfo->gidCount) > APP_MAX_GIDS) {
        APPSPAWN_LOGW("More gid for %{public}s msg count %{public}u permission %{public}u",
                      GetProcessName(property),
                      dacInfo->gidCount,
                      sandboxNode->section.gidCount);
        copyLen = APP_MAX_GIDS - dacInfo->gidCount;
    }
    int ret = memcpy_s(&dacInfo->gidTable[dacInfo->gidCount], sizeof(gid_t) * copyLen,
                       sandboxNode->section.gidTable, sizeof(gid_t) * copyLen);
    if (ret != EOK) {
        APPSPAWN_LOGW("Failed to append permission %{public}s gid to %{public}s",
                      sandboxNode->section.name,
                      GetProcessName(property));
    }
    dacInfo->gidCount += copyLen;

    return 0;
}

static void UpdateMsgFlagsWithPermission(AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property,
    const char *permissionMode, uint32_t flag)
{
    int32_t processIndex = GetPermissionIndexInQueue(&sandbox->permissionQueue, permissionMode);
    int res = CheckAppPermissionFlagSet(property, (uint32_t)processIndex);
    if (res == 0) {
        APPSPAWN_LOGV("Don't need set %{public}s flag", permissionMode);
        return;
    }

    int ret = SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, flag);
    if (ret != 0) {
        APPSPAWN_LOGE("Set %{public}s flag failed", permissionMode);
    }
    return;
}

static int UpdatePermissionFlags(AppSpawnMgr *content, AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    if (IsNWebSpawnMode(content)) {
        return 0;
    }

    int32_t index = 0;
    if (sandbox->appFullMountEnable) {
        index = GetPermissionIndexInQueue(&sandbox->permissionQueue, FILE_CROSS_APP_MODE);
    } else {
        index = GetPermissionIndexInQueue(&sandbox->permissionQueue, FILE_ACCESS_COMMON_DIR_MODE);
    }

    int32_t fileMgrIndex = GetPermissionIndexInQueue(&sandbox->permissionQueue, FILE_ACCESS_MANAGER_MODE);
    int32_t userFileIndex = GetPermissionIndexInQueue(&sandbox->permissionQueue, READ_WRITE_USER_FILE_MODE);
    int fileMgrRes = CheckAppPermissionFlagSet(property, (uint32_t)fileMgrIndex);
    int userFileRes = CheckAppPermissionFlagSet(property, (uint32_t)userFileIndex);
    //If both FILE_ACCESS_MANAGER_MODE and READ_WRITE_USER_FILE_MODE exist, the value is invalid.
    if (fileMgrRes != 0 && userFileRes != 0) {
        APPSPAWN_LOGE("invalid msg request.");
        return -1;
    }
    // If FILE_ACCESS_MANAGER_MODE and READ_WRITE_USER_FILE_MODE do not exist,set the flag bit.
    if (index > 0 && (fileMgrIndex > 0 && userFileIndex > 0) && (fileMgrRes == 0 && userFileRes == 0)) {
        if (SetAppPermissionFlags(property, index) != 0) {
            return -1;
        }
    }
    return 0;
}

static int AppendGids(AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    int ret = AppendPermissionGid(sandbox, property);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add gid for %{public}s", GetProcessName(property));
    ret = AppendPackageNameGids(sandbox, property);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add gid for %{public}s", GetProcessName(property));
    return ret;
}

int SpawnPrepareSandboxCfg(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(property != NULL, return -1);
    APPSPAWN_LOGV("Prepare sandbox config %{public}s", GetProcessName(property));
    ExtDataType type = GetSandboxType(content, property);
    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content, type);
    APPSPAWN_CHECK(sandbox != NULL, return -1, "Failed to get sandbox for %{public}s", GetProcessName(property));
    content->content.sandboxType = type;

    int ret = UpdatePermissionFlags(content, sandbox, property);
    if (ret != 0) {
        APPSPAWN_LOGW("set sandbox permission flag failed.");
        return APPSPAWN_SANDBOX_ERROR_SET_PERMISSION_FLAG_FAIL;
    }
    UpdateMsgFlagsWithPermission(sandbox, property, GET_ALL_PROCESSES_MODE, APP_FLAGS_GET_ALL_PROCESSES);
    UpdateMsgFlagsWithPermission(sandbox, property, APP_ALLOW_IOURING, APP_FLAGS_ALLOW_IOURING);

    ret = AppendGids(sandbox, property);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add gid for %{public}s", GetProcessName(property));
    ret = StagedMountSystemConst(sandbox, property, IsNWebSpawnMode(content));
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to mount system-const for %{public}s", GetProcessName(property));
    return 0;
}

static int SpawnMountDirToShared(AppSpawnMgr *content, AppSpawningCtx *property)
{
    ExtDataType type = GetSandboxType(content, property);
    AppSpawnSandboxCfg *appSandbox = GetAppSpawnSandbox(content, type);
    APPSPAWN_CHECK(appSandbox != NULL, return APPSPAWN_SANDBOX_INVALID,
                   "Failed to get sandbox cfg for %{public}s", GetProcessName(property));
    content->content.sandboxType = type;

    SandboxContext *context = GetSandboxContext();  // need free after mount
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return APPSPAWN_SYSTEM_ERROR);
    int ret = InitSandboxContext(context, appSandbox, property, IsNWebSpawnMode(content));  // need free after mount
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, DeleteSandboxContext(&context);
                                        return ret);

    ret = MountDirsToShared(content, context, appSandbox);
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "Failed to mount dirs to shared");
    DeleteSandboxContext(&context);
    return ret;
}

#ifdef APPSPAWN_SANDBOX_NEW
MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load sandbox module ...");
    (void)AddServerStageHook(STAGE_SERVER_PRELOAD, HOOK_PRIO_SANDBOX, PreLoadAppSandboxCfg);
    (void)AddServerStageHook(STAGE_SERVER_PRELOAD, HOOK_PRIO_SANDBOX, PreLoadIsoLatedSandboxCfg);
    (void)AddServerStageHook(STAGE_SERVER_PRELOAD, HOOK_PRIO_SANDBOX, PreLoadNWebSandboxCfg);
    (void)AddServerStageHook(STAGE_SERVER_PRELOAD, HOOK_PRIO_SANDBOX, PreLoadDebugSandboxCfg);
    (void)AddServerStageHook(STAGE_SERVER_EXIT, HOOK_PRIO_SANDBOX, SandboxHandleServerExit);
    (void)AddServerStageHook(STAGE_SERVER_EXIT, HOOK_PRIO_SANDBOX, IsolatedSandboxHandleServerExit);
    (void)AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_SANDBOX, SpawnPrepareSandboxCfg);
    (void)AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_SANDBOX, SpawnMountDirToShared);
    (void)AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_SANDBOX, SpawnBuildSandboxEnv);
}

MODULE_DESTRUCTOR(void)
{
    ClearVariable();
    ClearExpandAppSandboxConfigHandle();
}
#endif
