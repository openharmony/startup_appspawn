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
    OH_ListAddWithOrder(&queue->front, &node->node, SandboxNodeCompareProc);
}

void DeleteSandboxMountNode(SandboxMountNode *sandboxNode)
{
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
    if (ListEmpty(section->front)) {
        return NULL;
    }
    return (SandboxMountNode *)ListEntry(section->front.next, SandboxMountNode, node);
}

void DumpSandboxMountNode(const SandboxMountNode *sandboxNode, uint32_t index)
{
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
    APPSPAPWN_DUMP("    ========================================= ");
    APPSPAPWN_DUMP("    Section %{public}s", section->name);
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
    APPSPAWN_CHECK(name != NULL, return NULL, "Invalid name %{public}u", type);
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
    OH_ListAddWithOrder(&queue->front, &node->sandboxNode.node, SandboxConditionalNodeCompareNode);
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
    free(sandbox->depGroupNodes);
    sandbox->depGroupNodes = NULL;

    free(sandbox);
    sandbox = NULL;
}

static void DumpSandboxSectionNode(const SandboxMountNode *node, uint32_t index)
{
    SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)node;
    DumpSandboxSection(&permissionNode->section);
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
    UnmountSandboxConfigs(sandbox);
    // delete all var
    DeleteAppSpawnSandbox(sandbox);
}

static void ClearAppSpawnSandbox(struct TagAppSpawnExtData *data)
{
    // clear no use sand box config
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
    sandbox->extData.clearNode = ClearAppSpawnSandbox;
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

    for (uint32_t i = 0; i < ARRAY_LENGTH(sandbox->systemUids); i++) {
        sandbox->systemUids[i] = INVALID_UID;
    }

    AddDefaultVariable();
    AddDefaultExpandAppSandboxConfigHandle();
    return sandbox;
}

void DumpAppSpawnSandboxCfg(AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_CHECK_ONLY_EXPER(sandbox != NULL, return);
    APPSPAPWN_DUMP("Sandbox sandboxNsFlags: %{public}x ", sandbox->sandboxNsFlags);
    APPSPAPWN_DUMP("Sandbox topSandboxSwitch: %{public}s", sandbox->topSandboxSwitch ? "true" : "false");
    APPSPAPWN_DUMP("Sandbox appFullMountEnable: %{public}s", sandbox->appFullMountEnable ? "true" : "false");
    APPSPAPWN_DUMP("Sandbox pidNamespaceSupport: %{public}s", sandbox->pidNamespaceSupport ? "true" : "false");
    APPSPAPWN_DUMP("Sandbox common info: ");

    DumpSandboxQueue(&sandbox->requiredQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->packageNameQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->permissionQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->spawnFlagsQueue.front, DumpSandboxSectionNode);
    DumpSandboxQueue(&sandbox->nameGroupsQueue.front, DumpSandboxSectionNode);
}

static int PreLoadSandboxCfg(AppSpawnMgr *content)
{
    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(content);
    APPSPAWN_CHECK(sandbox == NULL, return 0, "Sandbox has been load");

    sandbox = CreateAppSpawnSandbox();
    APPSPAWN_CHECK_ONLY_EXPER(sandbox != NULL, return APPSPAWN_SYSTEM_ERROR);
    OH_ListAddTail(&sandbox->extData.node, &content->extData);

    // load app sandbox config
    LoadAppSandboxConfig(sandbox, IsNWebSpawnMode(content));
    sandbox->maxPermissionIndex = PermissionRenumber(&sandbox->permissionQueue);

    content->content.sandboxNsFlags = 0;
    if (IsNWebSpawnMode(content) || sandbox->pidNamespaceSupport) {
        content->content.sandboxNsFlags = sandbox->sandboxNsFlags;
    }
    return 0;
}

int SpawnBuildSandboxEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    AppSpawnSandboxCfg *appSandbox = GetAppSpawnSandbox(content);
    APPSPAWN_CHECK(appSandbox != NULL, return -1, "Failed to get sandbox for %{public}s", GetProcessName(property));
    // CLONE_NEWPID 0x20000000
    // CLONE_NEWNET 0x40000000
    if ((content->content.sandboxNsFlags & CLONE_NEWPID) == CLONE_NEWPID) {
        int ret = getprocpid();
        if (ret < 0) {
            return ret;
        }
    }
    int ret = MountSandboxConfigs(appSandbox, property, IsNWebSpawnMode(content));
    // for module test do not create sandbox
    if (strncmp(GetBundleName(property), MODULE_TEST_BUNDLE_NAME, strlen(MODULE_TEST_BUNDLE_NAME)) == 0) {
        return 0;
    }
    return ret == 0 ? 0 : APPSPAWN_SANDBOX_MOUNT_FAIL;
}

static int AppendPermissionGid(const AppSpawnSandboxCfg *sandbox, AppSpawningCtx *property)
{
    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, GetProcessName(property));

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
        APPSPAWN_LOGV("Append permission gid %{public}s to %{public}s permission: %{public}u message: %{public}u",
            permissionNode->section.name, GetProcessName(property), permissionNode->section.gidCount,
            dacInfo->gidCount);
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
        }
        node = node->next;
    }
    return 0;
}

static bool IsSystemConstMounted(const AppSpawnSandboxCfg *sandbox, AppSpawnMsgDacInfo *info)
{
    uid_t uid = info->uid / UID_BASE;
    for (uint32_t i = 0; i < ARRAY_LENGTH(sandbox->systemUids); i++) {
        if (sandbox->systemUids[i] == uid) {
            return true;
        }
    }
    return false;
}

static int SetSystemConstMounted(AppSpawnSandboxCfg *sandbox, AppSpawnMsgDacInfo *info)
{
    uid_t uid = info->uid / UID_BASE;
    for (uint32_t i = 0; i < ARRAY_LENGTH(sandbox->systemUids); i++) {
        if (sandbox->systemUids[i] == INVALID_UID) {
            sandbox->systemUids[i] = uid;
            break;
        }
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

    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK(info != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, GetProcessName(property));
    if (!IsSystemConstMounted(sandbox, info)) {
        ret = StagedMountSystemConst(sandbox, property, IsNWebSpawnMode(content));
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to mount system-const for %{public}s", GetProcessName(property));
        SetSystemConstMounted(sandbox, info);
    } else {
        APPSPAWN_LOGW("System-const config [uid: %{public}u] has been set", info->uid / UID_BASE);
    }
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load sandbox module ...");
    AddPreloadHook(HOOK_PRIO_SANDBOX, PreLoadSandboxCfg);
    AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_SANDBOX, SpawnPrepareSandboxCfg);
    AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_SANDBOX, SpawnBuildSandboxEnv);
}

MODULE_DESTRUCTOR(void)
{
    ClearVariable();
    ClearExpandAppSandboxConfigHandle();
}
