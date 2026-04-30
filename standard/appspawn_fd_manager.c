/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "appspawn_fd_manager.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"

void SpawningFdsDestroy(ListNode *node)
{
    AppSpawnFds *fdSets = ListEntry(node, AppSpawnFds, node);
    for (size_t i = 0; i < fdSets->count; i++) {
        APPSPAWN_ONLY_EXPER(fdSets->fds[i] >= 0, close(fdSets->fds[i]);
            fdSets->fds[i] = -1);
    }
    free(fdSets);
}

/**
 * @brief Compare function for finding spawning fd by pid and type.
 * @return 0 if matched, 1 otherwise (convention for OH_ListFind)
 */
static int SpawningFdPidComparePro(ListNode *node, void *data)
{
    APPSPAWN_ONLY_EXPER(node == NULL || data == NULL, return 1);
    AppSpawnFds *fdsNode = ListEntry(node, AppSpawnFds, node);
    SpawningFdKey *key = data;
    return (fdsNode->pid == key->pid && fdsNode->type == key->type) ? 0 : 1;
}

/**
 * @brief Get current timestamp in microseconds (CLOCK_MONOTONIC).
 * @return Timestamp in microseconds, or 0 if clock_gettime fails
 */
static uint64_t GetCurrentTimestamp(void)
{
    struct timespec ts;
    APPSPAWN_ONLY_EXPER(clock_gettime(CLOCK_MONOTONIC, &ts) != 0, return 0);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

AppSpawnFds *RegisterSpawningFds(AppSpawnMgr *mgr, const SpawningFdRegInfo *regInfo)
{
    APPSPAWN_CHECK(mgr != NULL, return NULL, "Invalid mgr");
    APPSPAWN_CHECK(regInfo != NULL, return NULL, "Invalid regInfo");
    APPSPAWN_CHECK(regInfo->type >= TYPE_FOR_DEC && regInfo->type < TYPE_INVALID, return NULL,
        "Invalid type %{public}d", regInfo->type);
    APPSPAWN_CHECK(regInfo->count > 0 && regInfo->fds != NULL,
        return NULL, "Invalid fds");
    APPSPAWN_CHECK(regInfo->count <= MAX_SPAWNING_FDS_PER_NODE,
        return NULL, "Invalid count %{public}u exceed max %{public}u",
        regInfo->count, MAX_SPAWNING_FDS_PER_NODE);

    size_t size = sizeof(AppSpawnFds) + regInfo->count * sizeof(int);
    AppSpawnFds *spawnFds = (AppSpawnFds *)calloc(1, size);
    APPSPAWN_CHECK(spawnFds != NULL, return NULL, "Failed to alloc spawning fds");

    OH_ListInit(&spawnFds->node);
    spawnFds->type = regInfo->type;
    spawnFds->count = regInfo->count;
    spawnFds->pid = regInfo->pid;
    spawnFds->timestamp = GetCurrentTimestamp();
    for (uint32_t i = 0; i < regInfo->count; i++) {
        spawnFds->fds[i] = regInfo->fds[i];
    }

    OH_ListAddTail(&mgr->spawningFdsQueue, &spawnFds->node);
    APPSPAWN_LOGV("RegisterSpawningFds type=%{public}d pid=%{public}d count=%{public}u",
        regInfo->type, regInfo->pid, regInfo->count);
    return spawnFds;
}

AppSpawnFds *FindSpawningFdsByPid(const AppSpawnMgr *mgr, pid_t pid, SpawningFdType type)
{
    APPSPAWN_CHECK_ONLY_EXPER(mgr != NULL, return NULL);
    SpawningFdKey key = { pid, type };
    ListNode *node = OH_ListFind(&mgr->spawningFdsQueue, &key, SpawningFdPidComparePro);
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return NULL);
    return ListEntry(node, AppSpawnFds, node);
}

int UnregisterSpawningFdsByPid(AppSpawnMgr *mgr, pid_t pid, SpawningFdType type)
{
    APPSPAWN_CHECK(mgr != NULL, return APPSPAWN_ARG_INVALID, "Invalid mgr");
    SpawningFdKey key = { pid, type };
    ListNode *node = OH_ListFind(&mgr->spawningFdsQueue, &key, SpawningFdPidComparePro);
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return APPSPAWN_ARG_INVALID);

    AppSpawnFds *spawnFds = ListEntry(node, AppSpawnFds, node);
    for (uint32_t i = 0; i < spawnFds->count; i++) {
        if (spawnFds->fds[i] >= 0) {
            close(spawnFds->fds[i]);
            spawnFds->fds[i] = -1;
        }
    }
    OH_ListRemove(&spawnFds->node);
    APPSPAWN_LOGV("UnregisterSpawningFdsByPid type=%{public}d pid=%{public}d", type, pid);
    free(spawnFds);
    return 0;
}

int RemoveSpawningFdsByPid(AppSpawnMgr *mgr, pid_t pid, SpawningFdType type)
{
    APPSPAWN_CHECK(mgr != NULL, return APPSPAWN_ARG_INVALID, "Invalid mgr");
    SpawningFdKey key = { pid, type };
    ListNode *node = OH_ListFind(&mgr->spawningFdsQueue, &key, SpawningFdPidComparePro);
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return APPSPAWN_ARG_INVALID);

    AppSpawnFds *spawnFds = ListEntry(node, AppSpawnFds, node);
    OH_ListRemove(&spawnFds->node);
    APPSPAWN_LOGV("RemoveSpawningFdsByPid type=%{public}d pid=%{public}d (fd not closed)", type, pid);
    free(spawnFds);
    return 0;
}

void DeleteSpawningFds(AppSpawnFds **fds)
{
    APPSPAWN_CHECK_ONLY_EXPER(fds != NULL && *fds != NULL, return);
    OH_ListRemove(&(*fds)->node);
    APPSPAWN_LOGV("DeleteSpawningFds type=%{public}d pid=%{public}d (fd not closed)", (*fds)->type, (*fds)->pid);
    free(*fds);
    *fds = NULL;
}

/**
 * @brief Close all valid FDs in a spawning fd node (does not free the node).
 */
static void CloseSpawnFds(AppSpawnFds *spawnFds)
{
    for (uint32_t i = 0; i < spawnFds->count; i++) {
        APPSPAWN_ONLY_EXPER(spawnFds->fds[i] >= 0, close(spawnFds->fds[i]);
            spawnFds->fds[i] = -1);
    }
}

uint32_t CleanupSpawningFdsByPid(AppSpawnMgr *mgr, pid_t pid)
{
    APPSPAWN_CHECK_ONLY_EXPER(mgr != NULL, return 0);
    uint32_t cleanedCount = 0;
    ListNode *node = mgr->spawningFdsQueue.next;
    while (node != &mgr->spawningFdsQueue) {
        ListNode *next = node->next;
        AppSpawnFds *spawnFds = ListEntry(node, AppSpawnFds, node);
        if (spawnFds->pid == pid) {
            CloseSpawnFds(spawnFds);
            OH_ListRemove(&spawnFds->node);
            OH_ListInit(&spawnFds->node);
            APPSPAWN_LOGI("CleanupSpawningFdsByPid type=%{public}d pid=%{public}d", spawnFds->type, spawnFds->pid);
            free(spawnFds);
            cleanedCount++;
        }
        node = next;
    }
    return cleanedCount;
}

void DumpSpawningFds(const AppSpawnMgr *mgr)
{
    APPSPAWN_CHECK_ONLY_EXPER(mgr != NULL, return);
    APPSPAWN_DUMP("Spawning Fds Queue:");
    ListNode *node = mgr->spawningFdsQueue.next;
    while (node != &mgr->spawningFdsQueue) {
        AppSpawnFds *spawnFds = ListEntry(node, AppSpawnFds, node);
        APPSPAWN_DUMP("  type=%{public}d pid=%{public}d timestamp=%{public}" PRIu64,
            spawnFds->type, spawnFds->pid, spawnFds->timestamp);
        for (uint32_t i = 0; i < spawnFds->count; i++) {
            APPSPAWN_DUMP("    fd[%{public}u]=%{public}d", i, spawnFds->fds[i]);
        }
        node = node->next;
    }
}

void GetSpawningFdsStats(const AppSpawnMgr *mgr, uint32_t *total, uint32_t *childParentCount,
    uint32_t *parentChildCount)
{
    APPSPAWN_CHECK_ONLY_EXPER(mgr != NULL && total != NULL && childParentCount != NULL && parentChildCount != NULL,
        return);
    *total = 0;
    *childParentCount = 0;
    *parentChildCount = 0;
    ListNode *node = mgr->spawningFdsQueue.next;
    while (node != &mgr->spawningFdsQueue) {
        AppSpawnFds *spawnFds = ListEntry(node, AppSpawnFds, node);
        (*total)++;
        if (spawnFds->type == TYPE_CHILD_PARENT) {
            (*childParentCount)++;
        } else if (spawnFds->type == TYPE_PARENT_CHILD) {
            (*parentChildCount)++;
        }
        node = node->next;
    }
}
