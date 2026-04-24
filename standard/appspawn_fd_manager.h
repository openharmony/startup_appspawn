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

#ifndef APPSPAWN_FD_MANAGER_H
#define APPSPAWN_FD_MANAGER_H

/**
 * @file appspawn_fd_manager.h
 * @brief Spawning FD management module.
 *
 * Manages file descriptors created during the app spawning process,
 * including prefork pipes and parent-child communication pipes.
 * FDs are tracked in a per-pid linked list (spawningFdsQueue in AppSpawnMgr)
 * to ensure proper lifecycle management and cleanup on process death.
 */

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward declaration to avoid circular dependency with appspawn_manager.h.
 * Full definition of TagAppSpawnMgr is in appspawn_manager.h.
 */
typedef struct TagAppSpawnMgr AppSpawnMgr;

/**
 * @brief Spawning FD type classification.
 */
typedef enum {
    TYPE_FOR_DEC,         /**< Decoupling FD */
    TYPE_FOR_FORK_ALL,    /**< FD shared across all fork children */
    TYPE_CHILD_PARENT,    /**< Child-to-parent pipe FD (fork result notification) */
    TYPE_PARENT_CHILD,    /**< Parent-to-child pipe FD (for sending fork requests) */
    TYPE_INVALID          /**< Sentinel value, must be last */
} SpawningFdType;

/** Maximum number of FDs allowed per single AppSpawnFds node */
#define MAX_SPAWNING_FDS_PER_NODE 8

/**
 * @brief Save the fd information during the incubation process
 * @param node Linked list head node
 * @param type Fd type
 * @param count Number of current Fd types
 * @param fd File handle set
 * @param pid Process ID (0 for global fd)
 * @param timestamp Creation timestamp
 */
typedef struct {
    struct ListNode node;
    SpawningFdType type;
    uint32_t count;
    pid_t pid;
    uint64_t timestamp;
    int fds[0];
} AppSpawnFds;

/**
 * @brief Key type for spawning fd lookup by pid and type.
 * Used as search criteria in OH_ListFind with SpawningFdPidComparePro.
 */
typedef struct {
    pid_t pid;           /**< Target process ID */
    SpawningFdType type; /**< FD type to match */
} SpawningFdKey;

/**
 * @brief Registration info for spawning fd.
 * Input parameter for RegisterSpawningFds(). Caller provides fd array
 * which will be copied into the newly allocated AppSpawnFds node.
 */
typedef struct {
    SpawningFdType type;  /**< FD type classification */
    uint32_t count;       /**< Number of FDs in fds array */
    const int *fds;       /**< FD array to register (copied, caller retains ownership) */
    pid_t pid;            /**< Process ID that owns these FDs */
} SpawningFdRegInfo;

/**
 * @brief Register spawning FDs to the management queue.
 *
 * Allocates a new AppSpawnFds node, copies the provided FDs, and appends
 * it to mgr->spawningFdsQueue. Can be used before or after fork() to track
 * pipe FDs associated with a specific child process.
 *
 * @param mgr      AppSpawn manager instance (must not be NULL)
 * @param regInfo  Registration info containing FDs to track
 * @return Pointer to the newly created AppSpawnFds node on success, NULL on failure
 */
AppSpawnFds *RegisterSpawningFds(AppSpawnMgr *mgr, const SpawningFdRegInfo *regInfo);

/**
 * @brief Find spawning FD node by pid and type.
 *
 * Searches mgr->spawningFdsQueue for a node matching the given pid and type.
 * The returned pointer remains owned by the queue; do not free it.
 *
 * @param mgr  AppSpawn manager instance
 * @param pid  Process ID to match
 * @param type FD type to match
 * @return Pointer to found node, or NULL if not found
 */
AppSpawnFds *FindSpawningFdsByPid(const AppSpawnMgr *mgr, pid_t pid, SpawningFdType type);

/**
 * @brief Unregister and close spawning FDs.
 *
 * Removes the node from the queue, closes all FDs it holds, and frees memory.
 * Use when FDs are no longer needed (e.g., pipe write completed).
 *
 * @param mgr  AppSpawn manager instance
 * @param pid  Process ID to match
 * @param type FD type to match
 * @return 0 on success, APPSPAWN_ARG_INVALID if not found
 */
int UnregisterSpawningFdsByPid(AppSpawnMgr *mgr, pid_t pid, SpawningFdType type);

/**
 * @brief Remove spawning FD node without closing FDs (ownership transfer).
 *
 * Removes the node from the queue and frees memory, but does NOT close
 * the FDs. Use when FD ownership is being transferred to another context
 * (e.g., from spawningFdsQueue to forkCtx).
 *
 * @param mgr  AppSpawn manager instance
 * @param pid  Process ID to match
 * @param type FD type to match
 * @return 0 on success, APPSPAWN_ARG_INVALID if not found
 */
int RemoveSpawningFdsByPid(AppSpawnMgr *mgr, pid_t pid, SpawningFdType type);

/**
 * @brief Delete spawning FD node by pointer without closing FDs.
 *
 * Removes the node from the queue and frees memory, but does NOT close FDs.
 * Sets the caller's pointer to NULL to prevent dangling pointer access.
 * Use when FD ownership is being transferred or node was registered with
 * a placeholder pid that needs to be cleaned up.
 *
 * @param fds  Pointer to the AppSpawnFds node pointer (obtained from RegisterSpawningFds)
 */
void DeleteSpawningFds(AppSpawnFds **fds);

/**
 * @brief Cleanup all spawning FDs for a given pid.
 *
 * Iterates the queue, removes ALL nodes matching the pid (regardless of type),
 * closes their FDs, and frees memory. Used when a child process dies or
 * needs to be cleaned up.
 *
 * @param mgr  AppSpawn manager instance
 * @param pid  Process ID to clean up
 * @return Number of nodes cleaned up
 */
uint32_t CleanupSpawningFdsByPid(AppSpawnMgr *mgr, pid_t pid);

/**
 * @brief Dump spawning FD queue info for debug.
 *
 * Iterates the entire queue and logs type, pid, timestamp,
 * and individual FD values via APPSPAWN_DUMP.
 *
 * @param mgr  AppSpawn manager instance
 */
void DumpSpawningFds(const AppSpawnMgr *mgr);

/**
 * @brief Get spawning FD statistics.
 *
 * Counts total nodes, prefork nodes, and parent-child nodes in the queue.
 *
 * @param mgr              AppSpawn manager instance
 * @param total            [out] Total number of nodes
 * @param childParentCount [out] Number of TYPE_CHILD_PARENT nodes
 * @param parentChildCount [out] Number of TYPE_PARENT_CHILD nodes
 */
void GetSpawningFdsStats(const AppSpawnMgr *mgr, uint32_t *total, uint32_t *childParentCount,
    uint32_t *parentChildCount);

/**
 * @brief Destroy callback for spawning FD node.
 *
 * Closes all FDs held by the node and frees the node memory.
 * Designed as a callback for list traversal cleanup.
 *
 * @param node  ListNode embedded in AppSpawnFds
 */
void SpawningFdsDestroy(ListNode *node);

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_FD_MANAGER_H
