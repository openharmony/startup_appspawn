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
 * @file tlv_builder.c
 * @brief TLV (Type-Length-Value) builder implementation
 *
 * This module provides a two-phase TLV building mechanism for message reconstruction.
 * Separated from spm.c for better modularity and reusability.
 */

#include "tlv_builder.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn_manager.h"
#include "securec.h"
#include <stdlib.h>
#include <string.h>
#include "list.h"

// ============================================================================
// TLV Entry Management
// ============================================================================

RebuildTlvEntry *CreateTlvEntry(void)
{
    RebuildTlvEntry *entry = (RebuildTlvEntry *)calloc(1, sizeof(RebuildTlvEntry));
    APPSPAWN_CHECK(entry != NULL, return NULL, "Failed to allocate TLV entry");
    OH_ListInit(&entry->node);
    return entry;
}

void AddTlvEntryToList(ListNode *head, RebuildTlvEntry *entry)
{
    OH_ListAddTail(head, &entry->node);
}

int AddStandardTlv(ListNode *head, uint16_t tlvType, uint8_t *data, uint32_t dataLen)
{
    APPSPAWN_CHECK(head != NULL, return -1, "Invalid parameter: head is NULL");
    // Allow data==NULL when dataLen==0 (TLV with no data payload)
    APPSPAWN_CHECK(data != NULL || dataLen == 0, return -1, "Invalid parameter: data is NULL but dataLen is non-zero");

    RebuildTlvEntry *entry = CreateTlvEntry();
    APPSPAWN_CHECK(entry != NULL, return -1, "Failed to allocate TLV entry");
    entry->entryType = TLV_ENTRY_STANDARD;
    entry->tlvType = tlvType;
    entry->tlvName = NULL;
    entry->dataType = 0;
    entry->data = data;
    entry->dataLen = dataLen;
    entry->totalLen = APPSPAWN_ALIGN(sizeof(AppSpawnTlv) + dataLen);
    entry->ownsData = true;
    AddTlvEntryToList(head, entry);
    return 0;
}

int AddExtTlv(ListNode *head, const char *tlvName, uint16_t dataType,
    uint8_t *data, uint32_t dataLen)
{
    APPSPAWN_CHECK(head != NULL, return -1, "Invalid parameter: head is NULL");
    APPSPAWN_CHECK(tlvName != NULL, return -1, "Invalid parameter: tlvName is NULL");
    APPSPAWN_CHECK(data != NULL, return -1, "Invalid parameter: data is NULL");

    RebuildTlvEntry *entry = CreateTlvEntry();
    APPSPAWN_CHECK(entry != NULL, return -1, "Failed to allocate TLV entry");
    entry->entryType = TLV_ENTRY_EXTENDED;
    entry->tlvType = 0;
    entry->tlvName = tlvName;
    entry->dataType = dataType;
    entry->data = data;
    entry->dataLen = dataLen;
    uint32_t nullExtra = (dataType == DATA_TYPE_STRING) ? 1 : 0;
    entry->totalLen = APPSPAWN_ALIGN(sizeof(AppSpawnTlvExt) + dataLen + nullExtra);
    entry->ownsData = true;
    AddTlvEntryToList(head, entry);
    return 0;
}

void DestroyTlvEntry(ListNode *node)
{
    if (node == NULL) {
        return;
    }
    RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
    if (entry->ownsData && entry->data != NULL) {
        free(entry->data);
    }
    free(entry);
}

void FreeTlvEntries(ListNode *head)
{
    OH_ListRemoveAll(head, DestroyTlvEntry);
}

int WriteTlvEntriesToBuffer(AppSpawnMsgNode *newMsg, ListNode *head, uint32_t *currentOffset)
{
    APPSPAWN_CHECK(newMsg != NULL && head != NULL && currentOffset != NULL,
                   return -1, "Invalid parameters");

    ListNode *node = NULL;
    ForEachListEntry(head, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);

        if (entry->entryType == TLV_ENTRY_STANDARD) {
            // Standard TLV: [AppSpawnTlv] [data] [padding]
            AppSpawnTlv *tlv = (AppSpawnTlv *)(newMsg->buffer + *currentOffset);
            tlv->tlvType = entry->tlvType;
            // tlvLen: aligned size (header + data + padding) for traversal
            tlv->tlvLen = (uint16_t)entry->totalLen;

            if (entry->data != NULL && entry->dataLen > 0) {
                int32_t ret = memcpy_s(tlv + 1, entry->dataLen, entry->data, entry->dataLen);
                APPSPAWN_CHECK(ret == 0, return -1,
                              "Failed to copy standard TLV data for type %{public}u", entry->tlvType);
            }

            newMsg->tlvOffset[entry->tlvType] = *currentOffset;
        } else {
            // Extended TLV: [AppSpawnTlvExt] [data]
            AppSpawnTlvExt *tlvExt = (AppSpawnTlvExt *)(newMsg->buffer + *currentOffset);
            tlvExt->tlvType = TLV_MAX;
            tlvExt->tlvLen = (uint16_t)entry->totalLen;
            tlvExt->dataLen = (uint16_t)entry->dataLen;
            tlvExt->dataType = entry->dataType;

            int ret = strcpy_s(tlvExt->tlvName, APPSPAWN_TLV_NAME_LEN, entry->tlvName);
            if (ret != 0) {
                APPSPAWN_LOGE("Failed to copy TLV name");
                return -1;
            }

            if (entry->data != NULL && entry->dataLen > 0) {
                char *dataPtr = (char *)(tlvExt + 1);
                int32_t retCpy = memcpy_s(dataPtr, entry->dataLen, entry->data, entry->dataLen);
                APPSPAWN_CHECK(retCpy == 0, return -1, "Failed to copy ext TLV data");
                APPSPAWN_ONLY_EXPER(entry->dataType == DATA_TYPE_STRING, dataPtr[entry->dataLen] = '\0');
            }

            newMsg->tlvOffset[TLV_MAX + newMsg->tlvCount] = *currentOffset;
            newMsg->tlvCount++;
        }

        *currentOffset += entry->totalLen;
    }

    return 0;
}

// ============================================================================
// TLV Copying
// ============================================================================

int CopyStandardTlv(const AppSpawnMsgNode *oldMsg, uint32_t tlvType, ListNode *head)
{
    if (oldMsg->tlvOffset[tlvType] == INVALID_OFFSET) {
        return 0;  // not present, skip successfully
    }

    AppSpawnTlv *tlv = (AppSpawnTlv *)(oldMsg->buffer + oldMsg->tlvOffset[tlvType]);
    uint32_t rawDataLen = tlv->tlvLen - sizeof(AppSpawnTlv);

    uint8_t *data = NULL;
    if (rawDataLen > 0) {
        data = (uint8_t *)calloc(1, rawDataLen);
        APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate TLV data");
        int32_t cpRet = memcpy_s(data, rawDataLen, tlv + 1, rawDataLen);
        APPSPAWN_CHECK(cpRet == 0, free(data); return -1, "Failed to copy TLV data");
    }

    APPSPAWN_LOGV("CopyStandardTlv: TLV %{public}u, len %{public}u", tlvType, rawDataLen);
    int ret = AddStandardTlv(head, (uint16_t)tlvType, data, rawDataLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1, "Failed to add TLV %{public}u", tlvType);

    return 0;
}

int CopyOtherExtTlv(const AppSpawnTlvExt *tlvExt, ListNode *head)
{
    // Step 1: Check if STRING type and get required length
    uint32_t dataLen = tlvExt->dataLen;
    bool isString = (tlvExt->dataType == DATA_TYPE_STRING);
    uint32_t allocLen = isString ? (dataLen + 1) : dataLen;

    // Step 2: Allocate memory and copy data (calloc initialized with 0, '\0' set for STRING)
    uint8_t *data = NULL;
    if (allocLen > 0) {
        data = (uint8_t *)calloc(1, allocLen);
        APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate ext TLV data");
        int32_t cpRet = memcpy_s(data, allocLen, tlvExt + 1, dataLen);
        APPSPAWN_CHECK(cpRet == 0, free(data); return -1, "Failed to copy ext TLV data");
    }

    // Step 4: Add to TLV list (dataLen does not include '\0')
    int ret = AddExtTlv(head, tlvExt->tlvName, tlvExt->dataType, data, dataLen);
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to add ext TLV");
        free(data);
        return -1;
    }
    return 0;
}

// ============================================================================
// Debug Utilities
// ============================================================================

const char *GetTlvTypeName(uint32_t tlvType)
{
    switch (tlvType) {
        case TLV_BUNDLE_INFO: return "BUNDLE_INFO";
        case TLV_MSG_FLAGS: return "MSG_FLAGS";
        case TLV_DAC_INFO: return "DAC_INFO";
        case TLV_DOMAIN_INFO: return "DOMAIN_INFO";
        case TLV_OWNER_INFO: return "OWNER_INFO";
        case TLV_ACCESS_TOKEN_INFO: return "ACCESS_TOKEN_INFO";
        case TLV_PERMISSION: return "PERMISSION";
        case TLV_INTERNET_INFO: return "INTERNET_INFO";
        case TLV_RENDER_TERMINATION_INFO: return "RENDER_TERMINATION_INFO";
        case TLV_CHECK_POINT_INFO: return "CHECK_POINT_INFO";
        default: return NULL;
    }
}
