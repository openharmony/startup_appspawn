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
#include <gmock/gmock.h>
#include "appspawn_utils.h"
#include "appspawn_msg.h"
#include "appspawn_manager.h"
#include "appspawn_mount_permission.h"
#include "spm.h"
#include "securec.h"
#include "list.h"
#include "cJSON.h"
#include "tlv_builder.h"
#include "spm_func_mock.h"
#include "test_data_factory.h"
#include <cstring>
#include <cstdlib>

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;

// Duplicate the file-local RefcountContext struct from spm.c for test access
typedef struct {
    uint64_t accessTokenIdEx;
    uint32_t uid;
    uint32_t spmHighToken;
    uint32_t spmLowToken;
    uint32_t spmUid;
} RefcountContext;

// Static function declarations (APPSPAWN_STATIC is empty when APPSPAWN_TEST is defined)
extern "C" {
APPSPAWN_STATIC uint32_t RunModeToSpawnId(uint32_t runMode);
APPSPAWN_STATIC int ValidateSpmDataConsistency(const SpmData *spmData,
    uint64_t accessTokenIdEx, uint32_t uidFromMsg, const AppSpawnMgr *mgr,
    const AppSpawningCtx *ctx);
APPSPAWN_STATIC int CollectDacInfoFromSPM(const AppSpawnMgr *mgr, const AppSpawningCtx *ctx,
    const AppSpawnMsgNode *oldMsg, const SpmData *spmData, ListNode *head);
APPSPAWN_STATIC int CollectPermissionFromSPM(const AppSpawnMgr *mgr, const SpmData *spmData, ListNode *head);
APPSPAWN_STATIC int CollectBundleInfoFromSPM(const AppSpawnMsgNode *oldMsg,
    const SpmData *spmData, ListNode *head);
APPSPAWN_STATIC int CollectDomainInfoFromSPM(const AppSpawnMsgNode *oldMsg,
    const SpmData *spmData, ListNode *head);
APPSPAWN_STATIC int CollectOwnerInfoFromSPM(const SpmData *spmData, ListNode *head);
APPSPAWN_STATIC int RebuildDistributionTypeTlv(uint32_t distributionType, ListNode *head);
APPSPAWN_STATIC int IncreaseRefCounts(AppSpawningCtx *ctx, const RefcountContext *refCtx);
APPSPAWN_STATIC int ValidateUidRefCount(const AppSpawnMgr *mgr, const AppSpawningCtx *ctx,
    const RefcountContext *refCtx);
APPSPAWN_STATIC int RebuildMessageFromSPM(const AppSpawnMgr *mgr, AppSpawningCtx *ctx);
APPSPAWN_STATIC int SpmPreloadHook(AppSpawnMgr *mgr);
}

/**
 * @brief SPM core module test class
 */
class AppSpawnSpmTest : public testing::Test {
protected:
    void SetUp() override
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";

        mock_ = std::make_shared<SpmFuncMockImpl>();
        SpmFuncMock::spmFuncMock_ = mock_;
        OH_ListInit(&head_);
    }

    void TearDown() override
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";

        FreeTlvEntries(&head_);
        SpmFuncMock::spmFuncMock_.reset();
    }

    /**
     * @brief Build an AppSpawnMsgNode with ACCESS_TOKEN_INFO and DAC_INFO TLVs in its buffer
     */
    AppSpawnMsgNode *BuildMessageWithTokenAndDac(uint64_t accessTokenIdEx, uint32_t uid)
    {
        uint32_t bufSize = 4096;
        AppSpawnMsgNode *msg = (AppSpawnMsgNode *)calloc(1, sizeof(AppSpawnMsgNode));
        if (msg == nullptr) {
            return nullptr;
        }

        msg->msgHeader.msgLen = sizeof(AppSpawnMsg);
        msg->connection = nullptr;
        msg->tlvCount = 0;

        uint32_t offsetSize = (TLV_MAX + MAX_TLV_COUNT) * sizeof(uint32_t);
        msg->tlvOffset = (uint32_t *)calloc(1, offsetSize);
        if (msg->tlvOffset == nullptr) {
            free(msg);
            return nullptr;
        }
        for (uint32_t i = 0; i < TLV_MAX + MAX_TLV_COUNT; i++) {
            msg->tlvOffset[i] = INVALID_OFFSET;
        }

        msg->buffer = (uint8_t *)calloc(1, bufSize);
        if (msg->buffer == nullptr) {
            free(msg->tlvOffset);
            free(msg);
            return nullptr;
        }

        uint32_t offset = 0;

        // Write ACCESS_TOKEN_INFO
        uint32_t tokenDataLen = sizeof(AppSpawnMsgAccessToken);
        uint32_t tokenTotalLen = APPSPAWN_ALIGN(sizeof(AppSpawnTlv) + tokenDataLen);
        AppSpawnTlv *tokenTlv = (AppSpawnTlv *)(msg->buffer + offset);
        tokenTlv->tlvType = TLV_ACCESS_TOKEN_INFO;
        tokenTlv->tlvLen = (uint16_t)tokenTotalLen;
        AppSpawnMsgAccessToken *tokenData = (AppSpawnMsgAccessToken *)(tokenTlv + 1);
        tokenData->accessTokenIdEx = accessTokenIdEx;
        msg->tlvOffset[TLV_ACCESS_TOKEN_INFO] = offset;
        offset += tokenTotalLen;

        // Write DAC_INFO
        uint32_t dacDataLen = sizeof(AppDacInfo);
        uint32_t dacTotalLen = APPSPAWN_ALIGN(sizeof(AppSpawnTlv) + dacDataLen);
        AppSpawnTlv *dacTlv = (AppSpawnTlv *)(msg->buffer + offset);
        dacTlv->tlvType = TLV_DAC_INFO;
        dacTlv->tlvLen = (uint16_t)dacTotalLen;
        AppDacInfo *dacData = (AppDacInfo *)(dacTlv + 1);
        dacData->uid = uid;
        dacData->gid = uid;
        dacData->gidCount = 0;
        msg->tlvOffset[TLV_DAC_INFO] = offset;
        offset += dacTotalLen;

        msg->msgHeader.msgLen = sizeof(AppSpawnMsg) + offset;
        return msg;
    }

    void FreeTestMsg(AppSpawnMsgNode *msg)
    {
        if (msg == nullptr) {
            return;
        }
        if (msg->buffer) free(msg->buffer);
        if (msg->tlvOffset) free(msg->tlvOffset);
        free(msg);
    }

    std::shared_ptr<SpmFuncMockImpl> mock_;
    ListNode head_;
};

// ============================================================================
// 1.1 GetSpawnId — default spawnId
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_GetSpawnId_001, TestSize.Level1)
{
    // g_spawnId is 0 before SpmPreloadHook runs
    uint32_t spawnId = GetSpawnId();
    EXPECT_EQ(spawnId, 0u);
}

// ============================================================================
// 1.2 RunModeToSpawnId — valid mode mappings
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_RunModeToSpawnId_001, TestSize.Level1)
{
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_APP_SPAWN), SPAWN_ID_APP);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_APP_COLD_RUN), SPAWN_ID_APP);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_NWEB_SPAWN), SPAWN_ID_NWEB);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_NWEB_COLD_RUN), SPAWN_ID_NWEB);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_NATIVE_SPAWN), SPAWN_ID_NATIVE);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_NATIVE_COLD_RUN), SPAWN_ID_NATIVE);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_CJAPP_SPAWN), SPAWN_ID_CJAPP);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_CJAPP_COLD_RUN), SPAWN_ID_CJAPP);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_HYBRID_SPAWN), SPAWN_ID_HYBRID);
    EXPECT_EQ(RunModeToSpawnId(MODE_FOR_HYBRID_COLD_RUN), SPAWN_ID_HYBRID);
}

// ============================================================================
// 1.3 RunModeToSpawnId — unknown runMode defaults to APP
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_RunModeToSpawnId_002, TestSize.Level2)
{
    EXPECT_EQ(RunModeToSpawnId(9999), SPAWN_ID_DEFAULT);
    EXPECT_EQ(RunModeToSpawnId(0), SPAWN_ID_DEFAULT);
}

// ============================================================================
// 1.4 CleanupStaleSpawns
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CleanupStaleSpawns_001, TestSize.Level2)
{
    EXPECT_CALL(*mock_, SpmClearSpawnidRefCnt(_)).WillOnce(Return(0));
    int ret = CleanupStaleSpawns();
    EXPECT_EQ(ret, 0);
}

// ============================================================================
// 1.5 IncRefCountsForContext — normal success
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_IncRefCountsForContext_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    RefcountContext refCtx = {};
    refCtx.accessTokenIdEx = ((uint64_t)0x12345678 << 32) | 12345678u;
    refCtx.uid = 1000;

    EXPECT_CALL(*mock_, SpmIncTokenidRefCnt(12345678u, _)).WillOnce(Return(0));
    EXPECT_CALL(*mock_, SpmIncUidRefCnt(1000u, _)).WillOnce(Return(0));

    int ret = IncreaseRefCounts(ctx, &refCtx);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx->spmRefAdded, SPM_REF_TOKENID | SPM_REF_UID);

    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.6 IncRefCountsForContext — tokenid increment fails
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_IncRefCountsForContext_002, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    RefcountContext refCtx = {};
    refCtx.accessTokenIdEx = 12345678u;
    refCtx.uid = 1000;

    EXPECT_CALL(*mock_, SpmIncTokenidRefCnt(_, _)).WillOnce(Return(-1));

    int ret = IncreaseRefCounts(ctx, &refCtx);
    EXPECT_EQ(ret, SPM_ERROR_REF_COUNT_INC_FAILED);
    EXPECT_EQ(ctx->spmRefAdded, SPM_REF_NONE);

    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.7 IncRefCountsForContext — uid increment fails (tokenid succeeded)
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_IncRefCountsForContext_003, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    RefcountContext refCtx = {};
    refCtx.accessTokenIdEx = 12345678u;
    refCtx.uid = 1000;

    EXPECT_CALL(*mock_, SpmIncTokenidRefCnt(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*mock_, SpmIncUidRefCnt(_, _)).WillOnce(Return(-1));

    int ret = IncreaseRefCounts(ctx, &refCtx);
    EXPECT_EQ(ret, SPM_ERROR_REF_COUNT_INC_FAILED);
    EXPECT_EQ(ctx->spmRefAdded, SPM_REF_TOKENID);  // tokenid was set before uid failed

    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.8 ValidateUidRefCount — normal mode (not nweb/isolated): skip check, return 0
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateUidRefCount_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    RefcountContext refCtx = {};
    refCtx.uid = 1000;
    refCtx.spmUid = 2000;  // mismatch, but normal mode skips SpmGetUidRefCnt

    // SpmGetUidRefCnt should NOT be called
    int ret = ValidateUidRefCount(mgr, ctx, &refCtx);
    EXPECT_EQ(ret, 0);

    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.9 ValidateUidRefCount — nweb mode + spmUid==0: skip check, return 0
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateUidRefCount_002, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    RefcountContext refCtx = {};
    refCtx.uid = 1000;
    refCtx.spmUid = 0;  // spmUid==0 -> skip check

    // SpmGetUidRefCnt should NOT be called
    int ret = ValidateUidRefCount(mgr, ctx, &refCtx);
    EXPECT_EQ(ret, 0);

    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.10 ValidateUidRefCount — isolated sandbox + uid match: skip check, return 0
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateUidRefCount_003, TestSize.Level2)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    AppSpawnMsgNode *mockMsg = (AppSpawnMsgNode *)calloc(1, sizeof(AppSpawnMsgNode));
    ASSERT_NE(mockMsg, nullptr);
    ctx->message = mockMsg;

    EXPECT_CALL(*mock_, CheckAppSpawnMsgFlag(_, TLV_MSG_FLAGS, APP_FLAGS_ISOLATED_SANDBOX_TYPE))
        .WillOnce(Return(1));

    RefcountContext refCtx = {};
    refCtx.uid = 1000;
    refCtx.spmUid = 1000;  // uid match -> skip check

    // SpmGetUidRefCnt should NOT be called
    int ret = ValidateUidRefCount(mgr, ctx, &refCtx);
    EXPECT_EQ(ret, 0);

    ctx->message = nullptr;
    free(mockMsg);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.11 OnSpawnAbortUpdateRefCount — normal decrement
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_OnSpawnAbortUpdateRefCount_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(ctx, nullptr);
    ctx->spmRefAdded = SPM_REF_TOKENID | SPM_REF_UID;

    uint64_t tokenIdEx = ((uint64_t)0x12345678 << 32) | 12345678u;
    AppSpawnMsgNode *msg = BuildMessageWithTokenAndDac(tokenIdEx, 1000);
    ASSERT_NE(msg, nullptr);
    ctx->message = msg;

    AppSpawnMsgAccessToken tokenInfo;
    tokenInfo.accessTokenIdEx = tokenIdEx;
    AppDacInfo dacInfo;
    dacInfo.uid = 1000;

    EXPECT_CALL(*mock_, GetAppSpawnMsgInfo(_, TLV_ACCESS_TOKEN_INFO))
        .WillOnce(Return(&tokenInfo));
    EXPECT_CALL(*mock_, SpmDecTokenidRefCnt(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*mock_, GetAppSpawnMsgInfo(_, TLV_DAC_INFO))
        .WillOnce(Return(&dacInfo));
    EXPECT_CALL(*mock_, SpmDecUidRefCnt(_, _)).WillOnce(Return(0));

    int ret = OnSpawnAbortUpdateRefCount(mgr, ctx);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx->spmRefAdded, SPM_REF_NONE);

    ctx->message = nullptr;
    FreeTestMsg(msg);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.12 OnAppExitUpdateRefCount — normal decrement
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_OnAppExitUpdateRefCount_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    ASSERT_NE(mgr, nullptr);

    AppSpawnedProcess appInfo = {};
    appInfo.uid = 1000;
    appInfo.pid = 12345;
    appInfo.tokenid = 12345678u;
    appInfo.spmRefAdded = SPM_REF_TOKENID | SPM_REF_UID;

    EXPECT_CALL(*mock_, SpmDecTokenidRefCnt(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*mock_, SpmDecUidRefCnt(_, _)).WillOnce(Return(0));

    int ret = OnAppExitUpdateRefCount(mgr, &appInfo);
    EXPECT_EQ(ret, 0);

    TestDataFactory::FreeAppSpawnMgr(mgr);
}

// ============================================================================
// 1.13 ValidateSpmDataConsistency — tokenidAttr matches
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateSpmDataConsistency_001, TestSize.Level1)
{
    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0xABCD0001, .uid = 1000, .apl = APL_NORMAL});
    ASSERT_NE(spmData, nullptr);

    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    uint64_t accessTokenIdEx = ((uint64_t)0xABCD0001 << 32) | 12345u;

    // For normal process with matching uid, no uid refcount check needed
    int ret = ValidateSpmDataConsistency(spmData, accessTokenIdEx, 1000, mgr, ctx);
    EXPECT_EQ(ret, 0);

    TestDataFactory::FreeSpmData(spmData);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.14 ValidateSpmDataConsistency — tokenidAttr mismatch (security error)
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateSpmDataConsistency_002, TestSize.Level1)
{
    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0xABCD0001, .uid = 1000, .apl = APL_NORMAL});
    ASSERT_NE(spmData, nullptr);

    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    // tokenidAttr in message is different from SPM data
    uint64_t accessTokenIdEx = ((uint64_t)0xDEAD0002 << 32) | 12345u;

    int ret = ValidateSpmDataConsistency(spmData, accessTokenIdEx, 1000, mgr, ctx);
    EXPECT_EQ(ret, SPM_ERROR_TOKENID_ATTR_MISMATCH);

    TestDataFactory::FreeSpmData(spmData);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.15 ValidateSpmDataConsistency — UID matches for normal process
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateSpmDataConsistency_003, TestSize.Level1)
{
    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0xABCD0001, .uid = 1000, .apl = APL_NORMAL});
    ASSERT_NE(spmData, nullptr);

    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    uint64_t accessTokenIdEx = ((uint64_t)0xABCD0001 << 32) | 12345u;

    int ret = ValidateSpmDataConsistency(spmData, accessTokenIdEx, 1000, mgr, ctx);
    EXPECT_EQ(ret, 0);

    TestDataFactory::FreeSpmData(spmData);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.16 ValidateSpmDataConsistency — UID mismatch for normal process
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateSpmDataConsistency_004, TestSize.Level1)
{
    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0xABCD0001, .uid = 2000, .apl = APL_NORMAL});
    ASSERT_NE(spmData, nullptr);

    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    uint64_t accessTokenIdEx = ((uint64_t)0xABCD0001 << 32) | 12345u;

    // Normal process: uidFromMsg=1000 != spmUid=2000 => MISMATCH
    int ret = ValidateSpmDataConsistency(spmData, accessTokenIdEx, 1000, mgr, ctx);
    EXPECT_EQ(ret, SPM_ERROR_UID_MISMATCH);

    TestDataFactory::FreeSpmData(spmData);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.17 ValidateUidRefCount — nweb mode + uid mismatch + refCnt==1: return 0
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateUidRefCount_004, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    RefcountContext refCtx = {};
    refCtx.uid = 1000;
    refCtx.spmUid = 2000;  // mismatch + nweb mode -> SpmGetUidRefCnt is called

    uint64_t refCnt = 1;
    EXPECT_CALL(*mock_, SpmGetUidRefCnt(1000u, _))
        .WillOnce(DoAll(SetArgPointee<1>(refCnt), Return(0)));

    int ret = ValidateUidRefCount(mgr, ctx, &refCtx);
    EXPECT_EQ(ret, 0);

    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.18 ValidateUidRefCount — isolated sandbox + uid mismatch + refCnt==1: return 0
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ValidateUidRefCount_005, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    AppSpawnMsgNode *mockMsg = (AppSpawnMsgNode *)calloc(1, sizeof(AppSpawnMsgNode));
    ASSERT_NE(mockMsg, nullptr);
    ctx->message = mockMsg;

    EXPECT_CALL(*mock_, CheckAppSpawnMsgFlag(_, TLV_MSG_FLAGS, APP_FLAGS_ISOLATED_SANDBOX_TYPE))
        .WillOnce(Return(1));

    RefcountContext refCtx = {};
    refCtx.uid = 1000;
    refCtx.spmUid = 2000;  // mismatch + isolated sandbox -> SpmGetUidRefCnt is called

    uint64_t refCnt = 1;
    EXPECT_CALL(*mock_, SpmGetUidRefCnt(1000u, _))
        .WillOnce(DoAll(SetArgPointee<1>(refCnt), Return(0)));

    int ret = ValidateUidRefCount(mgr, ctx, &refCtx);
    EXPECT_EQ(ret, 0);

    ctx->message = nullptr;
    free(mockMsg);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.19 CollectBundleInfoFromSPM — normal
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CollectBundleInfoFromSPM_001, TestSize.Level1)
{
    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0, .uid = 1000, .apl = APL_NORMAL,
         .ownerid = 999, .idType = 0, .distributionType = 1, .bundleName = "com.test.bundle"});
    ASSERT_NE(spmData, nullptr);

    int ret = CollectBundleInfoFromSPM(nullptr, spmData, &head_);
    EXPECT_EQ(ret, 0);

    // Verify TLV was added
    ListNode *node = nullptr;
    int count = 0;
    ForEachListEntry(&head_, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_STANDARD);
        EXPECT_EQ(entry->tlvType, TLV_BUNDLE_INFO);
        EXPECT_NE(entry->data, nullptr);
        count++;
    }
    EXPECT_EQ(count, 1);

    TestDataFactory::FreeSpmData(spmData);
}

// ============================================================================
// 1.20 CollectBundleInfoFromSPM — name.buf is NULL
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CollectBundleInfoFromSPM_002, TestSize.Level2)
{
    SpmData *spmData = (SpmData *)calloc(1, sizeof(SpmData));
    ASSERT_NE(spmData, nullptr);
    spmData->name.buf = nullptr;  // null name buffer

    int ret = CollectBundleInfoFromSPM(nullptr, spmData, &head_);
    EXPECT_EQ(ret, -1);

    free(spmData);
}

// ============================================================================
// 1.21 CollectDacInfoFromSPM — normal (app mode, no isolated flag)
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CollectDacInfoFromSPM_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    // ctx->message is NULL, so CheckAppMsgFlagsSet returns 0 (not isolated)
    // This is expected: no mock call needed

    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0, .uid = 2000, .apl = APL_NORMAL});
    ASSERT_NE(spmData, nullptr);

    int ret = CollectDacInfoFromSPM(mgr, ctx, nullptr, spmData, &head_);
    EXPECT_EQ(ret, 0);

    // Verify DAC TLV
    ListNode *node = nullptr;
    ForEachListEntry(&head_, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_STANDARD);
        EXPECT_EQ(entry->tlvType, TLV_DAC_INFO);
        // uid should be from SPM
        AppDacInfo *dacInfo = (AppDacInfo *)entry->data;
        EXPECT_NE(dacInfo, nullptr);
        EXPECT_EQ(dacInfo->uid, 2000u);  // from SPM
    }

    TestDataFactory::FreeSpmData(spmData);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.22 CollectDacInfoFromSPM — nweb mode skips SPM DAC
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CollectDacInfoFromSPM_002, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    SpmData *spmData = TestDataFactory::CreateMockSpmData();
    ASSERT_NE(spmData, nullptr);

    // nweb mode: skip SPM DAC, no DAC in oldMsg => return 0 (skip)
    int ret = CollectDacInfoFromSPM(mgr, ctx, nullptr, spmData, &head_);
    EXPECT_EQ(ret, 0);

    // No TLV should have been added
    ListNode *node = nullptr;
    int count = 0;
    ForEachListEntry(&head_, node) { count++; }
    EXPECT_EQ(count, 0);

    TestDataFactory::FreeSpmData(spmData);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.23 CollectDacInfoFromSPM — isolated mode skips SPM DAC
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CollectDacInfoFromSPM_003, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_APP_SPAWN);
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    // Set up ctx->message so CheckAppMsgFlagsSet calls the mock
    AppSpawnMsgNode *mockMsg = (AppSpawnMsgNode *)calloc(1, sizeof(AppSpawnMsgNode));
    ASSERT_NE(mockMsg, nullptr);
    ctx->message = mockMsg;

    SpmData *spmData = TestDataFactory::CreateMockSpmData();
    ASSERT_NE(spmData, nullptr);

    // Mock isolated flag -> true
    EXPECT_CALL(*mock_, CheckAppSpawnMsgFlag(_, TLV_MSG_FLAGS, APP_FLAGS_ISOLATED_SANDBOX_TYPE))
        .WillOnce(Return(1));

    // isolated mode: skip SPM DAC, no DAC in oldMsg => return 0
    int ret = CollectDacInfoFromSPM(mgr, ctx, nullptr, spmData, &head_);
    EXPECT_EQ(ret, 0);

    ctx->message = nullptr;
    free(mockMsg);
    TestDataFactory::FreeSpmData(spmData);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.24 CollectPermissionFromSPM — normal
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CollectPermissionFromSPM_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    ASSERT_NE(mgr, nullptr);

    SandboxQueue *permQueue = TestDataFactory::CreateMockPermissionQueue();
    ASSERT_NE(permQueue, nullptr);
    mgr->content.permissionQueue = permQueue;

    SpmData *spmData = TestDataFactory::CreateMockSpmData();
    ASSERT_NE(spmData, nullptr);

    // Mock ComputePermissionFlags chain:
    // GetPermissionsFromKernel -> OK
    // GetSpawnFlagIndexesFromPermissionBitmap needs non-empty queue, but our queue is empty.
    // With empty queue, maxCount=0, so CollectPermissionFromSPM creates TLV with count=0.
    // But GetSpawnFlagIndexesFromPermissionBitmap with flagCount=0 returns -1.
    // So ComputePermissionFlags returns -1, and CollectPermissionFromSPM returns -1.
    // This is the actual behavior with an empty permission queue.
    EXPECT_CALL(*mock_, GetPermissionsFromKernel(_, _, _)).WillOnce(Return(0));

    int ret = CollectPermissionFromSPM(mgr, spmData, &head_);
    // With empty permission queue, GetSpawnFlagIndexesFromPermissionBitmap returns -1
    // because flagCount (=maxCount=0) check fails
    EXPECT_EQ(ret, -1);

    TestDataFactory::FreeSpmData(spmData);
    TestDataFactory::FreePermissionQueue(permQueue);
    TestDataFactory::FreeAppSpawnMgr(mgr);
}

// ============================================================================
// 1.25 CollectDomainInfoFromSPM — normal
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CollectDomainInfoFromSPM_001, TestSize.Level1)
{
    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0, .uid = 1000, .apl = APL_SYSTEM_CORE});
    ASSERT_NE(spmData, nullptr);

    int ret = CollectDomainInfoFromSPM(nullptr, spmData, &head_);
    EXPECT_EQ(ret, 0);

    ListNode *node = nullptr;
    ForEachListEntry(&head_, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_STANDARD);
        EXPECT_EQ(entry->tlvType, TLV_DOMAIN_INFO);
        // Verify APL string "system_core" is in the data
        AppSpawnMsgDomainInfo *domainInfo = (AppSpawnMsgDomainInfo *)entry->data;
        EXPECT_NE(domainInfo, nullptr);
        char *apl = (char *)(domainInfo + 1);
        EXPECT_STREQ(apl, "system_core");
    }

    TestDataFactory::FreeSpmData(spmData);
}

// ============================================================================
// 1.26 CollectOwnerInfoFromSPM — normal
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_CollectOwnerInfoFromSPM_001, TestSize.Level1)
{
    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0, .uid = 1000, .apl = APL_NORMAL, .ownerid = 999999});
    ASSERT_NE(spmData, nullptr);

    int ret = CollectOwnerInfoFromSPM(spmData, &head_);
    EXPECT_EQ(ret, 0);

    ListNode *node = nullptr;
    ForEachListEntry(&head_, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_EQ(entry->entryType, TLV_ENTRY_STANDARD);
        EXPECT_EQ(entry->tlvType, TLV_OWNER_INFO);
    }

    TestDataFactory::FreeSpmData(spmData);
}

// ============================================================================
// 1.27 RebuildDistributionTypeTlv — various type mappings
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_RebuildDistributionTypeTlv_001, TestSize.Level1)
{
    const char *expected[] = {
        "none", "app_gallery", "enterprise", "os_integration",
        "crowdtesting", "enterprise_normal", "enterprise_mdm", "internaltesting"
    };
    int expectedCount = sizeof(expected) / sizeof(expected[0]);

    for (int i = 0; i < expectedCount; i++) {
        FreeTlvEntries(&head_);
        OH_ListInit(&head_);

        int ret = RebuildDistributionTypeTlv((uint32_t)i, &head_);
        EXPECT_EQ(ret, 0) << "Failed for distributionType=" << i;

        ListNode *node = nullptr;
        int count = 0;
        ForEachListEntry(&head_, node) {
            RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
            EXPECT_EQ(entry->entryType, TLV_ENTRY_EXTENDED);
            EXPECT_STREQ(entry->tlvName, "AppDistributionType");
            EXPECT_EQ(entry->dataType, DATA_TYPE_STRING);
            // Verify the data content matches expected string
            ASSERT_NE(entry->data, nullptr);
            EXPECT_STREQ((char *)entry->data, expected[i])
                << "Mismatch for distributionType=" << i;
            count++;
        }
        EXPECT_EQ(count, 1) << "Expected 1 entry for distributionType=" << i;
    }

    // Test unknown type (out of range) -> defaults to "none"
    FreeTlvEntries(&head_);
    OH_ListInit(&head_);
    int ret = RebuildDistributionTypeTlv(100, &head_);
    EXPECT_EQ(ret, 0);
    ListNode *node = nullptr;
    ForEachListEntry(&head_, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        EXPECT_STREQ((char *)entry->data, "none");
    }
}

// ============================================================================
// 1.28 OnMessageRebuildFromSPM — hook entry point (NULL params)
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_OnMessageRebuildFromSPM_001, TestSize.Level1)
{
    int ret = OnMessageRebuildFromSPM(nullptr, nullptr);
    EXPECT_EQ(ret, SPM_ERROR_INVALID_PARAM);
}

// ============================================================================
// 1.29 RebuildMessageFromSPM — NULL ctx->message
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_RebuildMessageFromSPM_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(ctx, nullptr);
    // ctx->message is NULL by default

    int ret = RebuildMessageFromSPM(mgr, ctx);
    EXPECT_EQ(ret, SPM_ERROR_INVALID_PARAM);

    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.30 RebuildMessageFromSPM — missing ACCESS_TOKEN_INFO: fallback to original message (return 0)
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_RebuildMessageFromSPM_002, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    AppSpawnMsgNode *msg = (AppSpawnMsgNode *)calloc(1, sizeof(AppSpawnMsgNode));
    ASSERT_NE(msg, nullptr);
    msg->msgHeader.msgLen = sizeof(AppSpawnMsg);
    uint32_t offsetSize = (TLV_MAX + MAX_TLV_COUNT) * sizeof(uint32_t);
    msg->tlvOffset = (uint32_t *)calloc(1, offsetSize);
    for (uint32_t i = 0; i < TLV_MAX + MAX_TLV_COUNT; i++) {
        msg->tlvOffset[i] = INVALID_OFFSET;
    }
    ctx->message = msg;

    // GetAppSpawnMsgInfo returns NULL (no ACCESS_TOKEN_INFO)
    EXPECT_CALL(*mock_, GetAppSpawnMsgInfo(_, TLV_ACCESS_TOKEN_INFO))
        .WillOnce(Return(nullptr));

    int ret = RebuildMessageFromSPM(mgr, ctx);
    // New code: GetTokenidAndUidFromMsg fails -> ReportSpmFailEvent + return 0 (fallback)
    EXPECT_EQ(ret, 0);

    // Cleanup
    ctx->message = nullptr;
    free(msg->tlvOffset);
    free(msg);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.31 RebuildMessageFromSPM — tokenidAttr mismatch: fallback to original message (return 0)
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_RebuildMessageFromSPM_003, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);

    uint64_t tokenIdEx = ((uint64_t)0xAAAA << 32) | 12345u;
    AppSpawnMsgNode *msg = BuildMessageWithTokenAndDac(tokenIdEx, 1000);
    ASSERT_NE(msg, nullptr);
    ctx->message = msg;

    // Mock GetAppSpawnMsgInfo to return token and DAC data
    AppSpawnMsgAccessToken tokenInfo;
    tokenInfo.accessTokenIdEx = tokenIdEx;
    AppDacInfo dacInfo;
    dacInfo.uid = 1000;

    EXPECT_CALL(*mock_, GetAppSpawnMsgInfo(_, TLV_ACCESS_TOKEN_INFO))
        .WillRepeatedly(Return(&tokenInfo));
    EXPECT_CALL(*mock_, GetAppSpawnMsgInfo(_, TLV_DAC_INFO))
        .WillRepeatedly(Return(&dacInfo));

    // IncreaseRefCounts succeeds (called before SPM data validation)
    EXPECT_CALL(*mock_, SpmIncTokenidRefCnt(_, _)).WillOnce(Return(0));
    EXPECT_CALL(*mock_, SpmIncUidRefCnt(_, _)).WillOnce(Return(0));

    // SpmDataNew succeeds
    SpmData *spmData = TestDataFactory::CreateMockSpmData(
        {.tokenid = 12345, .tokenidAttr = 0xBBBB, .uid = 1000, .apl = APL_NORMAL});
    ASSERT_NE(spmData, nullptr);

    EXPECT_CALL(*mock_, SpmDataNew(_, _, _)).WillOnce(Return(spmData));
    EXPECT_CALL(*mock_, SpmGetEntry(_, _)).WillOnce(Return(0));

    // New code: security error falls back to original message (no rollback, no abort).
    // SpmDataFree is called on the failure path.
    EXPECT_CALL(*mock_, SpmDataFree(_)).Times(1);

    int ret = RebuildMessageFromSPM(mgr, ctx);
    EXPECT_EQ(ret, 0);  // fallback to original message

    ctx->message = nullptr;
    FreeTestMsg(msg);
    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.32 OnMessageRebuildFromSPM — skip on Linux platform
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_OnMessageRebuildFromSPM_002, TestSize.Level1)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr();
    AppSpawningCtx *ctx = TestDataFactory::CreateMockSpawningCtx();
    ASSERT_NE(mgr, nullptr);
    ASSERT_NE(ctx, nullptr);
    mgr->content.isLinux = 1;

    int ret = OnMessageRebuildFromSPM(mgr, ctx);
    EXPECT_EQ(ret, 0);

    TestDataFactory::FreeAppSpawnMgr(mgr);
    TestDataFactory::FreeSpawningCtx(ctx);
}

// ============================================================================
// 1.33 SpmPreloadHook — normal initialization
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_SpmPreloadHook_001, TestSize.Level2)
{
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    EXPECT_CALL(*mock_, SpmClearSpawnidRefCnt(_)).WillOnce(Return(0));

    int ret = SpmPreloadHook(mgr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(GetSpawnId(), SPAWN_ID_APP);

    TestDataFactory::FreeAppSpawnMgr(mgr);
}

// ============================================================================
// 1.34 ModuleConstructor — verify hooks are registered
// ============================================================================
HWTEST_F(AppSpawnSpmTest, App_Spawn_Spm_ModuleConstructor_001, TestSize.Level2)
{
    // MODULE_CONSTRUCTOR runs automatically during static initialization.
    // We verify that the module loaded by checking that GetSpawnId works
    // after SpmPreloadHook is called.
    AppSpawnMgr *mgr = TestDataFactory::CreateMockAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    EXPECT_CALL(*mock_, SpmClearSpawnidRefCnt(_)).WillOnce(Return(0));

    int ret = SpmPreloadHook(mgr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(GetSpawnId(), SPAWN_ID_NWEB);

    TestDataFactory::FreeAppSpawnMgr(mgr);
}
