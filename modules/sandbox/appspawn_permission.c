/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "appspawn_permission.h"
#ifdef APPSPAWN_CLIENT
#include "appspawn_mount_permission.h"
#else
#include "appspawn_sandbox.h"
#endif
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "securec.h"

static int PermissionNodeCompareIndex(ListNode *node, void *data)
{
    SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
    return permissionNode->permissionIndex - *(int32_t *)data;
}

static int PermissionNodeCompareName(ListNode *node, void *data)
{
    SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
#ifdef APPSPAWN_CLIENT
    return strcmp(permissionNode->name, (char *)data);
#else
    return strcmp(permissionNode->section.name, (char *)data);
#endif
}

static int PermissionNodeCompareProc(ListNode *node, ListNode *newNode)
{
    SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
    SandboxPermissionNode *newPermissionNode = (SandboxPermissionNode *)ListEntry(newNode, SandboxMountNode, node);
#ifdef APPSPAWN_CLIENT
    return strcmp(permissionNode->name, newPermissionNode->name);
#else
    return strcmp(permissionNode->section.name, newPermissionNode->section.name);
#endif
}

int AddSandboxPermissionNode(const char *name, SandboxQueue *queue)
{
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL && queue != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_LOGV("Add permission name %{public}s ", name);
    if (GetPermissionNodeInQueue(queue, name) != NULL) {
        APPSPAWN_LOGW("Permission name %{public}s has been exist", name);
        return 0;
    }
#ifndef APPSPAWN_CLIENT
    size_t len = sizeof(SandboxPermissionNode);
    SandboxPermissionNode *node = (SandboxPermissionNode *)CreateSandboxSection(
        name, len, SANDBOX_TAG_PERMISSION);
    APPSPAWN_CHECK(node != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to create permission node");
    node->permissionIndex = 0;
    OH_ListAddWithOrder(&queue->front, &node->section.sandboxNode.node, PermissionNodeCompareProc);
#else
    size_t len = APPSPAWN_ALIGN(strlen(name) + 1) + sizeof(SandboxPermissionNode);
    SandboxPermissionNode *node = (SandboxPermissionNode *)calloc(1, len);
    APPSPAWN_CHECK(node != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to create permission node");
    OH_ListInit(&node->sandboxNode.node);
    node->permissionIndex = 0;
    int ret = strcpy_s(node->name, len, name);
    APPSPAWN_CHECK(ret == 0, free(node);
        return APPSPAWN_SYSTEM_ERROR, "Failed to copy name");
    OH_ListAddWithOrder(&queue->front, &node->sandboxNode.node, PermissionNodeCompareProc);
#endif
    return 0;
}

int32_t DeleteSandboxPermissions(SandboxQueue *queue)
{
    APPSPAWN_CHECK_ONLY_EXPER(queue != NULL, return APPSPAWN_ARG_INVALID);
    ListNode *node = queue->front.next;
    while (node != &queue->front) {
        SandboxMountNode *sandboxNode = (SandboxMountNode *)ListEntry(node, SandboxMountNode, node);
        OH_ListRemove(&sandboxNode->node);
        OH_ListInit(&sandboxNode->node);
#ifndef APPSPAWN_CLIENT
        DeleteSandboxSection((SandboxSection *)sandboxNode);
#else
        free(sandboxNode);
#endif
        // get first
        node = queue->front.next;
    }
    return 0;
}

int32_t PermissionRenumber(SandboxQueue *queue)
{
    APPSPAWN_CHECK_ONLY_EXPER(queue != NULL, return -1);
    ListNode *node = queue->front.next;
    int index = -1;
    while (node != &queue->front) {
        SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
        permissionNode->permissionIndex = ++index;
#ifdef APPSPAWN_CLIENT
        APPSPAWN_LOGV("Permission index %{public}d name %{public}s",
            permissionNode->permissionIndex, permissionNode->name);
#else
        APPSPAWN_LOGV("Permission index %{public}d name %{public}s",
            permissionNode->permissionIndex, permissionNode->section.name);
#endif
        node = node->next;
    }
    return index + 1;
}

const SandboxPermissionNode *GetPermissionNodeInQueue(SandboxQueue *queue, const char *permission)
{
    if (queue == NULL || permission == NULL) {
        return NULL;
    }
    ListNode *node = OH_ListFind(&queue->front, (void *)permission, PermissionNodeCompareName);
    return (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
}

const SandboxPermissionNode *GetPermissionNodeInQueueByIndex(SandboxQueue *queue, int32_t index)
{
    if (queue == NULL) {
        return NULL;
    }
    ListNode *node = OH_ListFind(&queue->front, (void *)&index, PermissionNodeCompareIndex);
    return (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
}

int32_t GetPermissionIndexInQueue(SandboxQueue *queue, const char *permission)
{
    APPSPAWN_CHECK_ONLY_EXPER(queue != NULL && permission != NULL, return INVALID_PERMISSION_INDEX);
    const SandboxPermissionNode *permissionNode = GetPermissionNodeInQueue(queue, permission);
    return permissionNode == NULL ? INVALID_PERMISSION_INDEX : permissionNode->permissionIndex;
}