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

#include <gtest/gtest.h>
#include "tlv_builder.h"
#include "appspawn_msg.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "securec.h"
#include "list.h"
#include <cstring>
#include <cstdlib>

using namespace testing;
using namespace testing::ext;

/**
 * @brief TLV Builder module test class
 */
class AppSpawnTlvBuilderTest : public testing::Test {
protected:
    ListNode head;

    void SetUp() override
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        OH_ListInit(&head);
    }

    void TearDown() override
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        FreeTlvEntries(&head);
    }

    /**
     * @brief Create a mock AppSpawnMsgNode with buffer and tlvOffset
     */
    AppSpawnMsgNode *CreateMockMsgNode(uint32_t bufSize)
    {
        AppSpawnMsgNode *node = (AppSpawnMsgNode *)calloc(1, sizeof(AppSpawnMsgNode));
        if (node == nullptr) {
            return nullptr;
        }
        node->connection = nullptr;
        node->tlvCount = 0;

        uint32_t offsetSize = (TLV_MAX + MAX_TLV_COUNT) * sizeof(uint32_t);
        node->tlvOffset = (uint32_t *)calloc(1, offsetSize);
        if (node->tlvOffset == nullptr) {
            free(node);
            return nullptr;
        }
        // Initialize all offsets to INVALID_OFFSET
        for (uint32_t i = 0; i < TLV_MAX + MAX_TLV_COUNT; i++) {
            node->tlvOffset[i] = INVALID_OFFSET;
        }

        node->buffer = (uint8_t *)calloc(1, bufSize);
        if (node->buffer == nullptr) {
            free(node->tlvOffset);
            free(node);
            return nullptr;
        }
        return node;
    }

    /**
     * @brief Free a mock AppSpawnMsgNode
     */
    void FreeMockMsgNode(AppSpawnMsgNode *node)
    {
        if (node == nullptr) {
            return;
        }
        if (node->buffer != nullptr) {
            free(node->buffer);
        }
        if (node->tlvOffset != nullptr) {
            free(node->tlvOffset);
        }
        free(node);
    }

    /**
     * @brief Count nodes in the TLV list
     */
    int CountListNodes()
    {
        int count = 0;
        ListNode *node = nullptr;
        ForEachListEntry(&head, node) {
            count++;
        }
        return count;
    }
};

// ============================================================================
// CreateTlvEntry tests
// ============================================================================

/**
 * @brief TC1: Create TLV entry normally
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_CreateTlvEntry_001, TestSize.Level1)
{
    RebuildTlvEntry *entry = CreateTlvEntry();
    ASSERT_NE(entry, nullptr);
    // Verify fields are zero-initialized
    EXPECT_EQ(entry->entryType, 0);
    EXPECT_EQ(entry->tlvType, 0);
    EXPECT_EQ(entry->tlvName, nullptr);
    EXPECT_EQ(entry->dataType, 0);
    EXPECT_EQ(entry->data, nullptr);
    EXPECT_EQ(entry->dataLen, 0u);
    EXPECT_EQ(entry->totalLen, 0u);
    EXPECT_EQ(entry->ownsData, false);
    // Verify node is initialized (points to itself for empty circular list)
    EXPECT_EQ(entry->node.next, &entry->node);
    EXPECT_EQ(entry->node.prev, &entry->node);
    // Cleanup: manually free since not added to list
    free(entry);
}

/**
 * @brief TC2: Create multiple TLV entries
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_CreateTlvEntry_002, TestSize.Level1)
{
    RebuildTlvEntry *entry1 = CreateTlvEntry();
    RebuildTlvEntry *entry2 = CreateTlvEntry();
    RebuildTlvEntry *entry3 = CreateTlvEntry();

    ASSERT_NE(entry1, nullptr);
    ASSERT_NE(entry2, nullptr);
    ASSERT_NE(entry3, nullptr);
    // Each should be independently allocated
    EXPECT_NE(entry1, entry2);
    EXPECT_NE(entry2, entry3);

    free(entry1);
    free(entry2);
    free(entry3);
}

// ============================================================================
// AddTlvEntryToList tests
// ============================================================================

/**
 * @brief TC3: Add TLV entries to list and verify order
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddTlvEntryToList_001, TestSize.Level1)
{
    RebuildTlvEntry *entry1 = CreateTlvEntry();
    RebuildTlvEntry *entry2 = CreateTlvEntry();
    RebuildTlvEntry *entry3 = CreateTlvEntry();
    ASSERT_NE(entry1, nullptr);
    ASSERT_NE(entry2, nullptr);
    ASSERT_NE(entry3, nullptr);

    entry1->tlvType = 1;
    entry2->tlvType = 2;
    entry3->tlvType = 3;

    AddTlvEntryToList(&head, entry1);
    AddTlvEntryToList(&head, entry2);
    AddTlvEntryToList(&head, entry3);

    // Verify list has 3 nodes
    EXPECT_EQ(CountListNodes(), 3);

    // Verify order: entry1 -> entry2 -> entry3
    ListNode *node = nullptr;
    int idx = 0;
    uint16_t expectedOrder[] = {1, 2, 3};
    ForEachListEntry(&head, node) {
        RebuildTlvEntry *e = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(e->tlvType, expectedOrder[idx]);
        idx++;
    }
    EXPECT_EQ(idx, 3);
}

// ============================================================================
// DestroyTlvEntry tests
// ============================================================================

/**
 * @brief TC4: Destroy TLV entry that owns its data
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_DestroyTlvEntry_001, TestSize.Level1)
{
    RebuildTlvEntry *entry = CreateTlvEntry();
    ASSERT_NE(entry, nullptr);

    uint8_t *data = (uint8_t *)calloc(1, 16);
    ASSERT_NE(data, nullptr);
    memcpy_s(data, 16, "testdata", 9);

    entry->data = data;
    entry->dataLen = 16;
    entry->ownsData = true;

    // Destroy should free both data and entry without crash
    // We just verify no crash occurs; memory is freed internally
    DestroyTlvEntry(&entry->node);
    // entry is freed, no use-after-free possible
}

/**
 * @brief TC5: Destroy TLV entry that does NOT own its data
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_DestroyTlvEntry_002, TestSize.Level1)
{
    RebuildTlvEntry *entry = CreateTlvEntry();
    ASSERT_NE(entry, nullptr);

    uint8_t stackData[] = "stack_data";
    entry->data = stackData;
    entry->dataLen = sizeof(stackData);
    entry->ownsData = false;

    // Destroy should NOT free stackData, only free entry itself
    // Verify no crash
    DestroyTlvEntry(&entry->node);
}

// ============================================================================
// FreeTlvEntries tests
// ============================================================================

/**
 * @brief TC6: Free entire TLV list
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_FreeTlvEntries_001, TestSize.Level1)
{
    // Add several entries with owned data
    uint8_t *data1 = (uint8_t *)calloc(1, 8);
    uint8_t *data2 = (uint8_t *)calloc(1, 8);
    uint8_t *data3 = (uint8_t *)calloc(1, 8);
    ASSERT_NE(data1, nullptr);
    ASSERT_NE(data2, nullptr);
    ASSERT_NE(data3, nullptr);

    EXPECT_EQ(AddStandardTlv(&head, TLV_DAC_INFO, data1, 8), 0);
    EXPECT_EQ(AddStandardTlv(&head, TLV_DOMAIN_INFO, data2, 8), 0);
    EXPECT_EQ(AddStandardTlv(&head, TLV_OWNER_INFO, data3, 8), 0);
    EXPECT_EQ(CountListNodes(), 3);

    // Free all entries
    FreeTlvEntries(&head);

    // List should be empty now (head points to itself)
    EXPECT_EQ(CountListNodes(), 0);
    EXPECT_TRUE(ListEmpty(head));
}

// ============================================================================
// AddStandardTlv tests
// ============================================================================

/**
 * @brief TC7: Add standard TLV normally
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddStandardTlv_001, TestSize.Level1)
{
    uint8_t *data = (uint8_t *)calloc(1, 32);
    ASSERT_NE(data, nullptr);
    memcpy_s(data, 32, "hello_tlv_data", 15);

    int ret = AddStandardTlv(&head, TLV_DAC_INFO, data, 32);
    EXPECT_EQ(ret, 0);

    // Verify the entry
    EXPECT_EQ(CountListNodes(), 1);
    ListNode *node = nullptr;
    ForEachListEntry(&head, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_STANDARD);
        EXPECT_EQ(entry->tlvType, TLV_DAC_INFO);
        EXPECT_EQ(entry->data, data);
        EXPECT_EQ(entry->dataLen, 32u);
        EXPECT_EQ(entry->totalLen, APPSPAWN_ALIGN(sizeof(AppSpawnTlv) + 32));
        EXPECT_EQ(entry->ownsData, true);
        EXPECT_EQ(entry->tlvName, nullptr);
    }
}

/**
 * @brief TC8: Add standard TLV with dataLen=0 (empty data)
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddStandardTlv_002, TestSize.Level2)
{
    // Use a heap-allocated pointer with dataLen=0; private repo requires data != NULL
    uint8_t *emptyData = (uint8_t *)calloc(1, 1);
    ASSERT_NE(emptyData, nullptr);
    int ret = AddStandardTlv(&head, TLV_MSG_FLAGS, emptyData, 0);
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(CountListNodes(), 1);
    ListNode *node = nullptr;
    ForEachListEntry(&head, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_STANDARD);
        EXPECT_EQ(entry->tlvType, TLV_MSG_FLAGS);
        EXPECT_NE(entry->data, nullptr);
        EXPECT_EQ(entry->dataLen, 0u);
        EXPECT_EQ(entry->totalLen, APPSPAWN_ALIGN(sizeof(AppSpawnTlv)));
        EXPECT_EQ(entry->ownsData, true);
    }
}

/**
 * @brief TC9: AddStandardTlv with NULL head returns -1
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddStandardTlv_003, TestSize.Level2)
{
    uint8_t data[] = {0x01, 0x02};
    int ret = AddStandardTlv(nullptr, TLV_DAC_INFO, data, sizeof(data));
    EXPECT_EQ(ret, -1);
}

/**
 * @brief TC10: AddStandardTlv with NULL data and non-zero dataLen returns -1
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddStandardTlv_004, TestSize.Level2)
{
    int ret = AddStandardTlv(&head, TLV_DAC_INFO, nullptr, 10);
    EXPECT_EQ(ret, -1);
    // Verify nothing was added to the list
    EXPECT_EQ(CountListNodes(), 0);
}

// ============================================================================
// AddExtTlv tests
// ============================================================================

/**
 * @brief TC11: Add extended TLV with string data type
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddExtTlv_001, TestSize.Level1)
{
    uint8_t *data = (uint8_t *)calloc(1, 16);
    ASSERT_NE(data, nullptr);
    memcpy_s(data, 16, "ext_string", 11);

    int ret = AddExtTlv(&head, "test_ext", DATA_TYPE_STRING, data, 11);
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(CountListNodes(), 1);
    ListNode *node = nullptr;
    ForEachListEntry(&head, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_EXTENDED);
        EXPECT_STREQ(entry->tlvName, "test_ext");
        EXPECT_EQ(entry->dataType, DATA_TYPE_STRING);
        EXPECT_EQ(entry->data, data);
        EXPECT_EQ(entry->dataLen, 11u);
        // nullExtra = 1 for DATA_TYPE_STRING
        uint32_t expectedTotal = APPSPAWN_ALIGN(sizeof(AppSpawnTlvExt) + 11 + 1);
        EXPECT_EQ(entry->totalLen, expectedTotal);
        EXPECT_EQ(entry->ownsData, true);
    }
}

/**
 * @brief TC12: Add extended TLV with non-string data type
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddExtTlv_002, TestSize.Level2)
{
    uint8_t *data = (uint8_t *)calloc(1, 8);
    ASSERT_NE(data, nullptr);
    memcpy_s(data, 8, "\x01\x02\x03\x04\x05\x06\x07\x08", 8);

    int ret = AddExtTlv(&head, "binary_ext", 0, data, 8);
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(CountListNodes(), 1);
    ListNode *node = nullptr;
    ForEachListEntry(&head, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_EXTENDED);
        EXPECT_STREQ(entry->tlvName, "binary_ext");
        EXPECT_EQ(entry->dataType, 0);
        EXPECT_EQ(entry->dataLen, 8u);
        // nullExtra = 0 for non-string
        uint32_t expectedTotal = APPSPAWN_ALIGN(sizeof(AppSpawnTlvExt) + 8);
        EXPECT_EQ(entry->totalLen, expectedTotal);
    }
}

/**
 * @brief TC13: AddExtTlv with NULL tlvName returns -1
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddExtTlv_003, TestSize.Level2)
{
    uint8_t data[] = {0x01};
    int ret = AddExtTlv(&head, nullptr, DATA_TYPE_STRING, data, sizeof(data));
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(CountListNodes(), 0);
}

/**
 * @brief TC14: AddExtTlv with NULL data returns -1
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_AddExtTlv_004, TestSize.Level2)
{
    int ret = AddExtTlv(&head, "test_ext", DATA_TYPE_STRING, nullptr, 10);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(CountListNodes(), 0);
}

// ============================================================================
// WriteTlvEntriesToBuffer tests
// ============================================================================

/**
 * @brief TC15: Write standard TLV to buffer
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_WriteTlvEntriesToBuffer_001, TestSize.Level1)
{
    // Create message node with buffer
    uint32_t bufSize = 1024;
    AppSpawnMsgNode *msg = CreateMockMsgNode(bufSize);
    ASSERT_NE(msg, nullptr);

    // Add a standard TLV
    uint8_t *data = (uint8_t *)calloc(1, 16);
    ASSERT_NE(data, nullptr);
    memcpy_s(data, 16, "write_test_data", 16);

    int ret = AddStandardTlv(&head, TLV_DAC_INFO, data, 16);
    ASSERT_EQ(ret, 0);

    // Write to buffer
    uint32_t offset = 0;
    ret = WriteTlvEntriesToBuffer(msg, &head, &offset);
    EXPECT_EQ(ret, 0);

    // Verify AppSpawnTlv in buffer
    AppSpawnTlv *tlv = (AppSpawnTlv *)(msg->buffer);
    EXPECT_EQ(tlv->tlvType, TLV_DAC_INFO);
    EXPECT_EQ(tlv->tlvLen, (uint16_t)APPSPAWN_ALIGN(sizeof(AppSpawnTlv) + 16));

    // Verify data was written after the TLV header
    uint8_t *dataInBuf = (uint8_t *)(tlv + 1);
    EXPECT_EQ(memcmp(dataInBuf, "write_test_data", 16), 0);

    // Verify tlvOffset was updated
    EXPECT_EQ(msg->tlvOffset[TLV_DAC_INFO], 0u);
    EXPECT_EQ(offset, APPSPAWN_ALIGN(sizeof(AppSpawnTlv) + 16));

    FreeMockMsgNode(msg);
}

/**
 * @brief TC16: Write extended TLV to buffer
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_WriteTlvEntriesToBuffer_002, TestSize.Level1)
{
    uint32_t bufSize = 1024;
    AppSpawnMsgNode *msg = CreateMockMsgNode(bufSize);
    ASSERT_NE(msg, nullptr);

    // Add an extended TLV with string data
    const char *extName = "ext_field";
    uint32_t dataLen = 5;
    uint8_t *data = (uint8_t *)calloc(1, dataLen);
    ASSERT_NE(data, nullptr);
    memcpy_s(data, dataLen, "hello", dataLen);

    int ret = AddExtTlv(&head, extName, DATA_TYPE_STRING, data, dataLen);
    ASSERT_EQ(ret, 0);

    // Write to buffer
    uint32_t offset = 0;
    ret = WriteTlvEntriesToBuffer(msg, &head, &offset);
    EXPECT_EQ(ret, 0);

    // Verify AppSpawnTlvExt in buffer
    AppSpawnTlvExt *tlvExt = (AppSpawnTlvExt *)(msg->buffer);
    EXPECT_EQ(tlvExt->tlvType, TLV_MAX);
    EXPECT_EQ(tlvExt->dataType, DATA_TYPE_STRING);
    EXPECT_EQ(tlvExt->dataLen, dataLen);
    EXPECT_STREQ(tlvExt->tlvName, extName);

    // Verify data and null terminator
    char *dataPtr = (char *)(tlvExt + 1);
    EXPECT_EQ(memcmp(dataPtr, "hello", dataLen), 0);
    EXPECT_EQ(dataPtr[dataLen], '\0');  // null terminator for string type

    // Verify tlvOffset was updated for ext TLV
    EXPECT_EQ(msg->tlvOffset[TLV_MAX], 0u);
    EXPECT_EQ(msg->tlvCount, 1u);

    FreeMockMsgNode(msg);
}

/**
 * @brief TC17: Write mixed standard and extended TLVs to buffer
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_WriteTlvEntriesToBuffer_003, TestSize.Level1)
{
    uint32_t bufSize = 2048;
    AppSpawnMsgNode *msg = CreateMockMsgNode(bufSize);
    ASSERT_NE(msg, nullptr);

    // Add standard TLV first
    uint8_t *stdData = (uint8_t *)calloc(1, 8);
    ASSERT_NE(stdData, nullptr);
    memcpy_s(stdData, 8, "std_data", 8);
    ASSERT_EQ(AddStandardTlv(&head, TLV_BUNDLE_INFO, stdData, 8), 0);

    // Then add extended TLV
    uint8_t *extData = (uint8_t *)calloc(1, 10);
    ASSERT_NE(extData, nullptr);
    memcpy_s(extData, 10, "ext_data!!", 10);
    ASSERT_EQ(AddExtTlv(&head, "mixed_ext", 0, extData, 10), 0);

    // Write to buffer
    uint32_t offset = 0;
    int ret = WriteTlvEntriesToBuffer(msg, &head, &offset);
    EXPECT_EQ(ret, 0);

    // Verify standard TLV at offset 0
    AppSpawnTlv *stdTlv = (AppSpawnTlv *)(msg->buffer);
    EXPECT_EQ(stdTlv->tlvType, TLV_BUNDLE_INFO);
    uint32_t stdTotalLen = APPSPAWN_ALIGN(sizeof(AppSpawnTlv) + 8);
    EXPECT_EQ(stdTlv->tlvLen, (uint16_t)stdTotalLen);
    EXPECT_EQ(msg->tlvOffset[TLV_BUNDLE_INFO], 0u);

    // Verify extended TLV at next offset
    uint8_t *extBufPtr = msg->buffer + stdTotalLen;
    AppSpawnTlvExt *extTlv = (AppSpawnTlvExt *)extBufPtr;
    EXPECT_EQ(extTlv->tlvType, TLV_MAX);
    EXPECT_STREQ(extTlv->tlvName, "mixed_ext");
    EXPECT_EQ(extTlv->dataLen, 10u);
    EXPECT_EQ(msg->tlvOffset[TLV_MAX], stdTotalLen);
    EXPECT_EQ(msg->tlvCount, 1u);

    // Verify offset incremented correctly
    uint32_t extTotalLen = APPSPAWN_ALIGN(sizeof(AppSpawnTlvExt) + 10);
    EXPECT_EQ(offset, stdTotalLen + extTotalLen);

    FreeMockMsgNode(msg);
}

/**
 * @brief TC18: WriteTlvEntriesToBuffer with NULL newMsg returns -1
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_WriteTlvEntriesToBuffer_004, TestSize.Level2)
{
    uint32_t offset = 0;
    int ret = WriteTlvEntriesToBuffer(nullptr, &head, &offset);
    EXPECT_EQ(ret, -1);
}

/**
 * @brief TC19: WriteTlvEntriesToBuffer with NULL head returns -1
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_WriteTlvEntriesToBuffer_005, TestSize.Level2)
{
    uint32_t bufSize = 256;
    AppSpawnMsgNode *msg = CreateMockMsgNode(bufSize);
    ASSERT_NE(msg, nullptr);

    uint32_t offset = 0;
    int ret = WriteTlvEntriesToBuffer(msg, nullptr, &offset);
    EXPECT_EQ(ret, -1);

    FreeMockMsgNode(msg);
}

// ============================================================================
// CopyStandardTlv tests
// ============================================================================

/**
 * @brief TC20: Copy standard TLV from old message
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_CopyStandardTlv_001, TestSize.Level1)
{
    // Build an "old message" with a TLV in its buffer
    uint32_t bufSize = 512;
    AppSpawnMsgNode *oldMsg = CreateMockMsgNode(bufSize);
    ASSERT_NE(oldMsg, nullptr);

    // Write a standard TLV directly into oldMsg buffer
    const char *testData = "copy_test_data";
    uint32_t dataLen = strlen(testData) + 1;
    uint32_t totalLen = APPSPAWN_ALIGN(sizeof(AppSpawnTlv) + dataLen);
    AppSpawnTlv *tlv = (AppSpawnTlv *)(oldMsg->buffer);
    tlv->tlvType = TLV_OWNER_INFO;
    tlv->tlvLen = (uint16_t)totalLen;
    memcpy_s(tlv + 1, dataLen, testData, dataLen);
    oldMsg->tlvOffset[TLV_OWNER_INFO] = 0;

    // Copy TLV
    int ret = CopyStandardTlv(oldMsg, TLV_OWNER_INFO, &head);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(CountListNodes(), 1);

    // Verify the copied entry
    uint32_t rawDataLen = totalLen - sizeof(AppSpawnTlv);
    ListNode *node = nullptr;
    ForEachListEntry(&head, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_STANDARD);
        EXPECT_EQ(entry->tlvType, TLV_OWNER_INFO);
        EXPECT_NE(entry->data, nullptr);
        EXPECT_EQ(entry->dataLen, rawDataLen);
        // Only compare the original data portion (rawDataLen may include alignment padding)
        EXPECT_EQ(memcmp(entry->data, testData, dataLen), 0);
    }

    FreeMockMsgNode(oldMsg);
}

/**
 * @brief TC21: CopyStandardTlv when TLV not present (INVALID_OFFSET)
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_CopyStandardTlv_002, TestSize.Level2)
{
    uint32_t bufSize = 256;
    AppSpawnMsgNode *oldMsg = CreateMockMsgNode(bufSize);
    ASSERT_NE(oldMsg, nullptr);

    // TLV_ACCESS_TOKEN_INFO offset is INVALID_OFFSET (default from CreateMockMsgNode)
    int ret = CopyStandardTlv(oldMsg, TLV_ACCESS_TOKEN_INFO, &head);
    EXPECT_EQ(ret, 0);  // Should return 0 (skip successfully)
    EXPECT_EQ(CountListNodes(), 0);  // Nothing added

    FreeMockMsgNode(oldMsg);
}

// ============================================================================
// CopyOtherExtTlv tests
// ============================================================================

/**
 * @brief TC22: Copy extended TLV with string type
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_CopyOtherExtTlv_001, TestSize.Level1)
{
    // Build an AppSpawnTlvExt in a buffer to simulate old message
    uint32_t bufSize = 512;
    uint8_t *buf = (uint8_t *)calloc(1, bufSize);
    ASSERT_NE(buf, nullptr);

    const char *extValue = "ext_string_val";
    uint32_t dataLen = strlen(extValue);

    AppSpawnTlvExt *srcExt = (AppSpawnTlvExt *)buf;
    srcExt->tlvType = TLV_MAX;
    srcExt->dataLen = (uint16_t)dataLen;
    srcExt->dataType = DATA_TYPE_STRING;
    int nameRet = strcpy_s(srcExt->tlvName, APPSPAWN_TLV_NAME_LEN, "source_ext");
    ASSERT_EQ(nameRet, 0);
    memcpy_s(srcExt + 1, dataLen, extValue, dataLen);

    // Copy
    int ret = CopyOtherExtTlv(srcExt, &head);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(CountListNodes(), 1);

    // Verify the copied entry
    ListNode *node = nullptr;
    ForEachListEntry(&head, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_EXTENDED);
        EXPECT_STREQ(entry->tlvName, "source_ext");
        EXPECT_EQ(entry->dataType, DATA_TYPE_STRING);
        EXPECT_EQ(entry->dataLen, dataLen);
        EXPECT_NE(entry->data, nullptr);
        // For string type, allocLen = dataLen + 1, calloc provides null terminator
        EXPECT_EQ(memcmp(entry->data, extValue, dataLen), 0);
        EXPECT_EQ(entry->data[dataLen], '\0');  // null terminator
    }

    free(buf);
}

/**
 * @brief TC23: Copy extended TLV with non-string type
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_CopyOtherExtTlv_002, TestSize.Level2)
{
    uint32_t bufSize = 512;
    uint8_t *buf = (uint8_t *)calloc(1, bufSize);
    ASSERT_NE(buf, nullptr);

    uint8_t binData[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint32_t dataLen = sizeof(binData);

    AppSpawnTlvExt *srcExt = (AppSpawnTlvExt *)buf;
    srcExt->tlvType = TLV_MAX;
    srcExt->dataLen = (uint16_t)dataLen;
    srcExt->dataType = 0;  // non-string
    int nameRet = strcpy_s(srcExt->tlvName, APPSPAWN_TLV_NAME_LEN, "binary_src");
    ASSERT_EQ(nameRet, 0);
    memcpy_s(srcExt + 1, dataLen, binData, dataLen);

    int ret = CopyOtherExtTlv(srcExt, &head);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(CountListNodes(), 1);

    ListNode *node = nullptr;
    ForEachListEntry(&head, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_EXTENDED);
        EXPECT_STREQ(entry->tlvName, "binary_src");
        EXPECT_EQ(entry->dataType, 0);
        EXPECT_EQ(entry->dataLen, dataLen);
        EXPECT_NE(entry->data, nullptr);
        EXPECT_EQ(memcmp(entry->data, binData, dataLen), 0);
    }

    free(buf);
}

/**
 * @brief TC24: Copy extended TLV with dataLen=0
 */
HWTEST_F(AppSpawnTlvBuilderTest, App_Spawn_Tlv_CopyOtherExtTlv_003, TestSize.Level2)
{
    uint32_t bufSize = 512;
    uint8_t *buf = (uint8_t *)calloc(1, bufSize);
    ASSERT_NE(buf, nullptr);

    AppSpawnTlvExt *srcExt = (AppSpawnTlvExt *)buf;
    srcExt->tlvType = TLV_MAX;
    srcExt->dataLen = 0;
    srcExt->dataType = 0;
    int nameRet = strcpy_s(srcExt->tlvName, APPSPAWN_TLV_NAME_LEN, "empty_ext");
    ASSERT_EQ(nameRet, 0);

    // CopyOtherExtTlv calls AddExtTlv which requires data != NULL.
    // When dataLen=0, allocLen=0, data remains NULL, so AddExtTlv returns -1.
    int ret = CopyOtherExtTlv(srcExt, &head);
    EXPECT_EQ(ret, -1);
    EXPECT_EQ(CountListNodes(), 0);

    free(buf);
}
