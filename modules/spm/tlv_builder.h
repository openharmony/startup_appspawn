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

/**
 * @file tlv_builder.h
 * @brief TLV (Type-Length-Value) builder for message reconstruction
 *
 * This module provides a two-phase TLV building mechanism:
 * Phase 1: Collect TLV descriptors into a linked list
 * Phase 2: Calculate exact size, allocate buffer, write all TLVs
 *
 * Separated from spm.c for better modularity and reusability.
 */

#ifndef APPSPAWN_TLV_BUILDER_H
#define APPSPAWN_TLV_BUILDER_H

#include <stdint.h>
#include <stdbool.h>
#include "list.h"
#include "appspawn_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration to avoid circular dependency
struct TagAppSpawnMsgNode;
typedef struct TagAppSpawnMsgNode AppSpawnMsgNode;

// TLV descriptor entry type
typedef enum {
    TLV_ENTRY_STANDARD,  // Standard TLV: AppSpawnTlv + data
    TLV_ENTRY_EXTENDED,  // Extended TLV: AppSpawnTlvExt + data
} TlvEntryType;

// TLV descriptor node for two-phase rebuild (linked list)
typedef struct TagRebuildTlvEntry {
    ListNode node;            // Linked list node
    TlvEntryType entryType;
    uint16_t tlvType;         // Standard TLV type (for TLV_ENTRY_STANDARD)
    const char *tlvName;      // Extended TLV name (for TLV_ENTRY_EXTENDED)
    uint16_t dataType;        // DATA_TYPE_STRING or 0
    uint8_t *data;            // Data pointer
    uint32_t dataLen;         // Actual data length
    uint32_t totalLen;        // Aligned complete TLV length (including header)
    bool ownsData;            // true means data should be freed
} RebuildTlvEntry;

/**
 * @brief Allocate a new TLV descriptor node
 * @return Pointer to new node, or NULL on failure
 */
RebuildTlvEntry *CreateTlvEntry(void);

/**
 * @brief Add a TLV descriptor node to the list tail
 * @param head List head
 * @param entry Node to add
 */
void AddTlvEntryToList(ListNode *head, RebuildTlvEntry *entry);

/**
 * @brief Create and add a standard TLV descriptor to the list
 *
 * Similar to client-side AddAppData — caller only provides data.
 *
 * @param head       List head
 * @param tlvType    Standard TLV type (TLV_DAC_INFO, etc.)
 * @param data       Allocated data pointer (helper takes ownership when ownsData=true)
 * @param dataLen    Data length (already APPSPAWN_ALIGN'd)
 * @return 0 on success, -1 on failure
 */
int AddStandardTlv(ListNode *head, uint16_t tlvType, uint8_t *data, uint32_t dataLen);

/**
 * @brief Create and add an extended TLV descriptor to the list
 *
 * Similar to client-side AddAppDataEx — caller only provides data.
 *
 * @param head       List head
 * @param tlvName    Extended TLV name
 * @param dataType   DATA_TYPE_STRING or 0
 * @param data       Allocated data pointer
 * @param dataLen    Actual data length (raw, not aligned; function computes alignment)
 * @return 0 on success, -1 on failure
 */
int AddExtTlv(ListNode *head, const char *tlvName, uint16_t dataType, uint8_t *data, uint32_t dataLen);

/**
 * @brief Destroy callback for OH_ListRemoveAll: free entry data and the node itself
 * @param node Node to destroy
 */
void DestroyTlvEntry(ListNode *node);

/**
 * @brief Free all TLV descriptor nodes in the list
 * @param head List head
 */
void FreeTlvEntries(ListNode *head);

/**
 * @brief Write TLV descriptor entries from linked list to message buffer
 *
 * @param newMsg New message node
 * @param head List head of TLV descriptors
 * @param currentOffset Current offset in buffer (in/out)
 * @return 0 on success, -1 on failure
 */
int WriteTlvEntriesToBuffer(AppSpawnMsgNode *newMsg, ListNode *head, uint32_t *currentOffset);

/**
 * @brief Collect a single standard TLV from old message (copy as-is)
 *
 * @param oldMsg Old message node
 * @param tlvType TLV type index
 * @param head List head to append entry to
 * @return 0 on success (copied or not present), -1 on error
 */
int CopyStandardTlv(const AppSpawnMsgNode *oldMsg, uint32_t tlvType, ListNode *head);

/**
 * @brief Copy other extended TLV as-is from old message
 *
 * @param tlvExt Pointer to the extended TLV header in old message buffer
 * @param head List head to append entry to
 * @return 0 on success, -1 on failure
 */
int CopyOtherExtTlv(const AppSpawnTlvExt *tlvExt, ListNode *head);

#ifdef __cplusplus
}
#endif

#endif  // APPSPAWN_TLV_BUILDER_H
