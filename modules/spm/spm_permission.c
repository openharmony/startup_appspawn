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
#include "spm_permission.h"
#include "appspawn_utils.h"

const SandboxPermissionNode *GetPermissionNodeByOpcode(const SandboxQueue *queue, uint32_t opcode)
{
    APPSPAWN_CHECK_ONLY_EXPER(queue != NULL, return NULL);

    ListNode *node = queue->front.next;
    while (node != &queue->front) {
        const SandboxPermissionNode *permissionNode =
            (const SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
        if (permissionNode->opcode == opcode) {
            return permissionNode;
        }
        node = node->next;
    }
    return NULL;
}

const char *GetPermissionNameByOpcode(const SandboxQueue *queue, uint32_t opcode)
{
    const SandboxPermissionNode *node = GetPermissionNodeByOpcode(queue, opcode);
    if (node != NULL) {
#ifndef APPSPAWN_CLIENT
        return node->section.name;
#else
        return node->name;
#endif
    }
    return NULL;
}

int32_t GetSpawnFlagIndexesFromPermissionBitmap(const SandboxQueue *queue,
    const uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint32_t *flagIndexes, uint32_t flagCount)
{
    APPSPAWN_CHECK_ONLY_EXPER(queue != NULL && perms != NULL && flagIndexes != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(flagCount > 0, return -1);

    uint32_t maxWords = flagCount;
    uint32_t grantedCount = 0;

    // flagIndexes is a bitmap: each word covers 32 permission bits
    // permissionIndex / 32 = word index, permissionIndex % 32 = bit position

    // 遍历 appspawn 定义的权限列表
    ListNode *node = queue->front.next;
    while (node != &queue->front) {
        const SandboxPermissionNode *permNode =
            (const SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);

        // 检查权限是否有 opcode（未定义 opcode 的权限跳过）
        if (permNode->opcode == 0) {
            node = node->next;
            continue;
        }

        // 检查 opcode 是否在应用的授权位图中
        uint32_t bitmapIndex = permNode->opcode / 32;
        uint32_t bitIndex = permNode->opcode % 32;

        if (bitmapIndex < MAX_PERM_BIT_MAP_SIZE && (perms[bitmapIndex] & (1U << bitIndex))) {
            // 权限已授权，将 permissionIndex 对应的 bit 置位到 flagIndexes bitmap
            uint32_t wordIdx = (uint32_t)permNode->permissionIndex / 32;
            uint32_t bitIdx = (uint32_t)permNode->permissionIndex % 32;
            if (wordIdx < maxWords) {
                flagIndexes[wordIdx] |= (1U << bitIdx);
                grantedCount++;
            }

            APPSPAWN_LOGV("GetSpawnFlagIndexesFromPermissionBitmap: permission opcode=%{public}u, "
                          "permIndex=%{public}d is granted, word[%{public}u] bit[%{public}u]",
                          permNode->opcode, permNode->permissionIndex, wordIdx, bitIdx);
        }

        node = node->next;
    }

    APPSPAWN_LOGI("GetSpawnFlagIndexesFromPermissionBitmap: %{public}u granted permissions, maxWords=%{public}u",
                  grantedCount, maxWords);
    return 0;
}
