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

#include "appspawn_manager.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "modulemgr.h"
#include "parameter.h"
#include "securec.h"

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
            APPSPAPWN_DUMP("        ****************************** %{public}u", index);
            APPSPAPWN_DUMP("        sandbox node source: %{public}s", pathNode->source ? pathNode->source : "null");
            APPSPAPWN_DUMP("        sandbox node target: %{public}s", pathNode->target ? pathNode->target : "null");
            DumpMountPathMountNode(pathNode);
            APPSPAPWN_DUMP("        sandbox node apl: %{public}s",
                pathNode->appAplName ? pathNode->appAplName : "null");
            APPSPAPWN_DUMP("        sandbox node checkErrorFlag: %{public}s",
                pathNode->checkErrorFlag ? "true" : "false");
            break;
        }
        case SANDBOX_TAG_SYMLINK: {
            SymbolLinkNode *linkNode = (SymbolLinkNode *)sandboxNode;
            APPSPAPWN_DUMP("        ***********************************");
            APPSPAPWN_DUMP("        sandbox node target: %{public}s", linkNode->target ? linkNode->target : "null");
            APPSPAPWN_DUMP("        sandbox node linkName: %{public}s",
                linkNode->linkName ? linkNode->linkName : "null");
            APPSPAPWN_DUMP("        sandbox node destMode: %{public}x", linkNode->destMode);
            APPSPAPWN_DUMP("        sandbox node checkErrorFlag: %{public}s",
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
    APPSPAPWN_DUMP("    sandboxSwitch %{public}s", section->sandboxSwitch ? "true" : "false");
    APPSPAPWN_DUMP("    sandboxShared %{public}s", section->sandboxShared ? "true" : "false");
    APPSPAPWN_DUMP("    gidCount: %{public}u", section->gidCount);
    for (uint32_t index = 0; index < section->gidCount; index++) {
        APPSPAPWN_DUMP("        gidTable[%{public}u]: %{public}u", index, section->gidTable[index]);
    }
    APPSPAPWN_DUMP("    mount group count: %{public}u", section->number);
    for (uint32_t i = 0; i < section->number; i++) {
        if (section->nameGroups[i]) {
            SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)section->nameGroups[i];
            APPSPAPWN_DUMP("        name[%{public}d] %{public}s", i, groupNode->section.name);
        }
    }
    APPSPAPWN_DUMP("    mount-paths: ");
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

AppSpawnSandboxCfg *GetAppSpawnSandbox(const AppSpawnMgr *content)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return NULL);
    uint32_t dataId = EXT_DATA_SANDBOX;
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
    APPSPAPWN_DUMP("    ========================================= ");
    APPSPAPWN_DUMP("    Section %{public}s", permissionNode->section.name);
    APPSPAPWN_DUMP("    Section permission index %{public}d", permissionNode->permissionIndex);
    DumpSandboxSection(&permissionNode->section);
}

static void DumpSandboxSectionNode(const SandboxMountNode *node, uint32_t index)
{
    SandboxSection *section = (SandboxSection *)node;
    APPSPAPWN_DUMP("    ========================================= ");
    APPSPAPWN_DUMP("    Section %{public}s", section->name);
    DumpSandboxSection(section);
}

static void DumpSandboxNameGroupNode(const SandboxMountNode *node, uint32_t index)
{
    SandboxNameGroupNode *nameGroupNode = (SandboxNameGroupNode *)node;
    APPSPAPWN_DUMP("    ========================================= ");
    APPSPAPWN_DUMP("    Section %{public}s", nameGroupNode->section.name);
    APPSPAPWN_DUMP("    Section dep mode %{public}s",
        nameGroupNode->depMode == MOUNT_MODE_ALWAYS ? "always" : "not-exists");
    if (nameGroupNode->depNode != NULL) {
        APPSPAPWN_DUMP("    mount-paths-deps: ");
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

AppSpawnSandboxCfg *CreateAppSpawnSandbox(void)
{
    // create sandbox
    AppSpawnSandboxCfg *sandbox = (AppSpawnSandboxCfg *)calloc(1, sizeof(AppSpawnSandboxCfg));
    APPSPAWN_CHECK(sandbox != NULL, return NULL, "Failed to create sandbox");

    // ext data init
    OH_ListInit(&sandbox->extData.node);
    sandbox->extData.dataId = EXT_DATA_SANDBOX;
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
    APPSPAPWN_DUMP("Sandbox root path: %{public}s", sandbox->rootPath);
    APPSPAPWN_DUMP("Sandbox sandboxNsFlags: %{public}x ", sandbox->sandboxNsFlags);
    APPSPAPWN_DUMP("Sandbox topSandboxSwitch: %{public}s", sandbox->topSandboxSwitch ? "true" : "false");
    APPSPAPWN_DUMP("Sandbox appFullMountEnable: %{public}s", sandbox->appFullMountEnable ? "true" : "false");
    APPSPAPWN_DUMP("Sandbox pidNamespaceSupport: %{public}s", sandbox->pidNamespaceSupport ? "true" : "false");
    APPSPAPWN_DUMP("Sandbox common info: ");
    DumpSandboxQueue(&sandbox->requiredQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->packageNameQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->permissionQueue.front, DumpSandboxPermission);
    DumpSandboxQueue(&sandbox->spawnFlagsQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->nameGroupsQueue.front, DumpSandboxNameGroupNode);
}

APPSPAWN_STATIC int PreLoadSandboxCfg(AppSpawnMgr *content)
{
    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content);
    APPSPAWN_CHECK(sandbox == NULL, return 0, "Sandbox has been load");

    sandbox = CreateAppSpawnSandbox();
    APPSPAWN_CHECK_ONLY_EXPER(sandbox != NULL, return APPSPAWN_SYSTEM_ERROR);
    OH_ListAddTail(&content->extData, &sandbox->extData.node);

    // load app sandbox config
    LoadAppSandboxConfig(sandbox, IsNWebSpawnMode(content));
    sandbox->maxPermissionIndex = PermissionRenumber(&sandbox->permissionQueue);

    content->content.sandboxNsFlags = 0;
    if (IsNWebSpawnMode(content) || sandbox->pidNamespaceSupport) {
        content->content.sandboxNsFlags = sandbox->sandboxNsFlags;
    }
    return 0;
}

APPSPAWN_STATIC int SandboxHandleServerExit(AppSpawnMgr *content)
{
    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content);
    APPSPAWN_CHECK(sandbox != NULL, return 0, "Sandbox not load");

    return 0;
}

int SpawnBuildSandboxEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    AppSpawnSandboxCfg *appSandbox = GetAppSpawnSandbox(content);
    APPSPAWN_CHECK(appSandbox != NULL, return -1, "Failed to get sandbox for %{public}s", GetProcessName(property));
    // no sandbox
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_NO_SANDBOX)) {
        return 0;
    }
    // CLONE_NEWPID 0x20000000
    // CLONE_NEWNET 0x40000000
    if ((content->content.sandboxNsFlags & CLONE_NEWPID) == CLONE_NEWPID) {
        int ret = getprocpid();
        if (ret < 0) {
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

int SpawnPrepareSandboxCfg(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(property != NULL, return -1);
    APPSPAWN_LOGV("Prepare sandbox config %{public}s", GetProcessName(property));
    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content);
    APPSPAWN_CHECK(sandbox != NULL, return -1, "Failed to get sandbox for %{public}s", GetProcessName(property));

    if (sandbox->appFullMountEnable) {
        int index = GetPermissionIndexInQueue(&sandbox->permissionQueue, FILE_CROSS_APP_MODE);
        if (index > 0) {
            SetAppPermissionFlags(property, index);
        }
    }
    int ret = AppendPermissionGid(sandbox, property);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add gid for %{public}s", GetProcessName(property));
    ret = StagedMountSystemConst(sandbox, property, IsNWebSpawnMode(content));
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to mount system-const for %{public}s", GetProcessName(property));
    return 0;
}

APPSPAWN_STATIC int SandboxUnmountPath(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(appInfo != NULL, return -1);
    APPSPAWN_LOGV("Sandbox process %{public}s %{public}u exit", appInfo->name, appInfo->uid);
    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content);
    return UnmountDepPaths(sandbox, appInfo->uid);
}

#ifdef APPSPAWN_SANDBOX_NEW
MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load sandbox module ...");
    (void)AddServerStageHook(STAGE_SERVER_PRELOAD, HOOK_PRIO_SANDBOX, PreLoadSandboxCfg);
    (void)AddServerStageHook(STAGE_SERVER_EXIT, HOOK_PRIO_SANDBOX, SandboxHandleServerExit);
    (void)AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_SANDBOX, SpawnPrepareSandboxCfg);
    (void)AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_SANDBOX, SpawnBuildSandboxEnv);
    (void)AddProcessMgrHook(STAGE_SERVER_APP_DIED, 0, SandboxUnmountPath);
}

MODULE_DESTRUCTOR(void)
{
    ClearVariable();
    ClearExpandAppSandboxConfigHandle();
}
#endif
