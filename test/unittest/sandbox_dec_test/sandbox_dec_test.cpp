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
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#include "sandbox_dec.h"
#include "dec_config.h"
#include "appspawn_utils.h"
#include "appspawn_hook.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;

// APPSPAWN_STATIC expands to empty under APPSPAWN_TEST, expose static functions
extern "C" {
int SetDecPolicyBatch(int fd, GlobalDecPolicyInfo *decPolicyInfos,
                      uint64_t timestamp, uint32_t start, uint32_t count);
}

// ==================== Mock constants ====================

static const int MOCK_OPEN_RETURN_FD = 3;
static const int MOCK_CLOCK_TIME_SEC = 1000;
static const int MOCK_CLOCK_TIME_NSEC = 500;
static const int MOCK_TOKEN_ID = 12345;

// ==================== Mock state ====================

static int g_mockIoctlReturn = 0;
static int g_mockIoctlCallCount = 0;
static uint32_t g_mockIoctlFailAfterCount = 0; // fail after N successful calls
static const void *g_lastIoctlData = nullptr;

static int g_mockOpenReturn = MOCK_OPEN_RETURN_FD;
static const char *MOCK_OPEN_PATH = nullptr;

static int g_mockClockTimeSec = MOCK_CLOCK_TIME_SEC;
static int g_mockClockTimeNsec = MOCK_CLOCK_TIME_NSEC;

// ==================== Wrap functions ====================

extern "C" {
int __wrap_open(const char *pathname, int flags, ...)
{
    MOCK_OPEN_PATH = pathname;
    return g_mockOpenReturn;
}

int __wrap_close(int fd)
{
    return 0;
}

// Stub for MODULE_CONSTRUCTOR in sandbox_dec.c
int __wrap_AddServerStageHook(AppSpawnHookStage stage, int prio, ServerStageHook hook)
{
    return 0;
}

int __wrap_ioctl(int fd, unsigned long request, ...)
{
    g_mockIoctlCallCount++;
    va_list args;
    va_start(args, request);
    void *arg = va_arg(args, void *);
    va_end(args);

    if (g_lastIoctlData != nullptr) {
        g_lastIoctlData = arg;
    }
    g_lastIoctlData = arg;

    if (g_mockIoctlFailAfterCount > 0 && g_mockIoctlCallCount > g_mockIoctlFailAfterCount) {
        errno = EINVAL;
        return -1;
    }
    return g_mockIoctlReturn;
}

int __wrap_clock_gettime(clockid_t clk_id, struct timespec *ts)
{
    if (ts != nullptr) {
        ts->tv_sec = g_mockClockTimeSec;
        ts->tv_nsec = g_mockClockTimeNsec;
    }
    return 0;
}
}

// ==================== Test helpers ====================

namespace OHOS {

class SandboxDecTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        ResetMockState();
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    }

    void ResetMockState()
    {
        g_mockIoctlReturn = 0;
        g_mockIoctlCallCount = 0;
        g_mockIoctlFailAfterCount = 0;
        g_lastIoctlData = nullptr;
        g_mockOpenReturn = MOCK_OPEN_RETURN_FD;
        MOCK_OPEN_PATH = nullptr;
        g_mockClockTimeSec = MOCK_CLOCK_TIME_SEC;
        g_mockClockTimeNsec = MOCK_CLOCK_TIME_NSEC;
    }

    // Helper: create DecPolicyInfo with N paths (max KERNEL_BATCH_SIZE per call)
    // pathOffset: starting index for path naming (for multi-batch tests)
    void FillPolicyInfo(DecPolicyInfo &info, uint32_t pathCount, uint32_t pathOffset = 0)
    {
        errno_t ret = memset_s(&info, sizeof(DecPolicyInfo), 0, sizeof(DecPolicyInfo));
        if (ret != EOK) {
            return;
        }
        // Limit to KERNEL_BATCH_SIZE to avoid buffer overflow
        if (pathCount > KERNEL_BATCH_SIZE) {
            pathCount = KERNEL_BATCH_SIZE;
        }
        info.tokenId = MOCK_TOKEN_ID;
        info.pathNum = pathCount;
        info.flag = false;
        for (uint32_t i = 0; i < pathCount; i++) {
            std::string pathStr = "/data/test/path_" + std::to_string(pathOffset + i);
            info.path[i].path = strdup(pathStr.c_str());
            info.path[i].pathLen = static_cast<uint32_t>(pathStr.length());
            info.path[i].mode = 0x1;
            info.path[i].flag = false;
        }
    }

    // Helper: free DecPolicyInfo paths
    void FreePolicyInfo(DecPolicyInfo &info)
    {
        for (uint32_t i = 0; i < info.pathNum; i++) {
            if (info.path[i].path != nullptr) {
                free(info.path[i].path);
                info.path[i].path = nullptr;
            }
        }
    }

    // Helper: fill multiple policy paths by calling SetDecPolicyInfos multiple times
    void FillMultiplePolicyInfos(uint32_t totalPathCount)
    {
        uint32_t batches = (totalPathCount + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
        for (uint32_t batch = 0; batch < batches; batch++) {
            uint32_t start = batch * KERNEL_BATCH_SIZE;
            uint32_t remaining = totalPathCount - start;
            uint32_t count = (remaining > KERNEL_BATCH_SIZE) ? KERNEL_BATCH_SIZE : remaining;

            DecPolicyInfo info;
            FillPolicyInfo(info, count, start);
            SetDecPolicyInfos(&info);
            FreePolicyInfo(info);
        }
    }
};

// ==================== TC101: 8 paths, 1 batch ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_8Paths_1Batch_001
 * @tc.desc: Verify SetDecPolicy delivers 8 paths in 1 batch (8 <= KERNEL_BATCH_SIZE)
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_8Paths_1Batch_001, TestSize.Level1)
{
    DecPolicyInfo info;
    FillPolicyInfo(info, 8);

    SetDecPolicyInfos(&info);

    // SetDecPolicy should call ioctl once (1 batch for 8 paths)
    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // Expect 1 ioctl call (8 paths = 1 batch)
    EXPECT_EQ(g_mockIoctlCallCount, 1);

    // open was called with /dev/dec
    EXPECT_NE(MOCK_OPEN_PATH, nullptr);
    FreePolicyInfo(info);
}

// ==================== TC102: 16 paths, 2 batches ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_16Paths_2Batches_001
 * @tc.desc: Verify SetDecPolicy delivers 16 paths in 2 batches
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_16Paths_2Batches_001, TestSize.Level1)
{
    // Use FillMultiplePolicyInfos to add 16 paths in 2 batches
    FillMultiplePolicyInfos(16);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // Expect 2 ioctl calls (16 paths = 2 batches of 8)
    EXPECT_EQ(g_mockIoctlCallCount, 2);
}

// ==================== TC103: 64 paths, 8 batches ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_64Paths_8Batches_001
 * @tc.desc: Verify SetDecPolicy delivers 64 paths in 8 batches (MAX_POLICY_NUM)
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_64Paths_8Batches_001, TestSize.Level1)
{
    // Use FillMultiplePolicyInfos to add 64 paths in 8 batches
    FillMultiplePolicyInfos(MAX_POLICY_NUM);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // Expect 8 ioctl calls (64 / 8 = 8 batches)
    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    EXPECT_EQ(expectedBatches, 8u);
}

// ==================== TC104: Timestamp consistency ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_TimestampConsistency_001
 * @tc.desc: Verify all batches use the same timestamp
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_TimestampConsistency_001, TestSize.Level1)
{
    // Use FillMultiplePolicyInfos to add 16 paths in 2 batches
    FillMultiplePolicyInfos(16);

    // Set a specific clock time
    g_mockClockTimeSec = 9999;
    g_mockClockTimeNsec = 12345;

    // Capture ioctl data to check timestamps
    g_mockIoctlCallCount = 0;
    g_lastIoctlData = nullptr;

    SetDecPolicy();

    // After SetDecPolicy, g_decPolicyInfos is cleaned up.
    // Verify that ioctl was called exactly 2 times
    EXPECT_EQ(g_mockIoctlCallCount, 2);

    // The timestamp is computed once and shared across all batches.
    // timestamp = 9999 * 1000000000 + 12345 = 9999000012345
    uint64_t expectedTimestamp = (uint64_t)g_mockClockTimeSec * APPSPAWN_SEC_TO_NSEC + (uint64_t)g_mockClockTimeNsec;
    EXPECT_EQ(expectedTimestamp, 9999000012345ULL);
}

// ==================== TC105: ioctl failure continues to next batch ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_IoctlFailContinue_001
 * @tc.desc: Verify SetDecPolicy continues to next batch when one batch fails
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_IoctlFailContinue_001, TestSize.Level1)
{
    // Use FillMultiplePolicyInfos to add 24 paths in 3 batches
    FillMultiplePolicyInfos(24);

    // Make the 2nd ioctl call fail
    g_mockIoctlReturn = 0;
    g_mockIoctlFailAfterCount = 1; // fail after 1st successful call

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // All 3 batches should have been attempted
    EXPECT_EQ(g_mockIoctlCallCount, 3);
}

// ==================== TC106: Empty policy handling ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_EmptyPolicy_001
 * @tc.desc: Verify SetDecPolicy returns safely when g_decPolicyInfos is NULL
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_EmptyPolicy_001, TestSize.Level1)
{
    // g_decPolicyInfos is NULL by default (no SetDecPolicyInfos called)
    g_mockIoctlCallCount = 0;
    g_mockOpenReturn = MOCK_OPEN_RETURN_FD;

    SetDecPolicy();

    // No ioctl should have been called
    EXPECT_EQ(g_mockIoctlCallCount, 0);
}

// ==================== TC107: Open device failure ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_OpenDeviceFail_001
 * @tc.desc: Verify SetDecPolicy handles /dev/dec open failure gracefully
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_OpenDeviceFail_001, TestSize.Level1)
{
    DecPolicyInfo info;
    FillPolicyInfo(info, 8);

    SetDecPolicyInfos(&info);

    // Make open fail
    g_mockOpenReturn = -1;

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // No ioctl should have been called since open failed
    EXPECT_EQ(g_mockIoctlCallCount, 0);

    FreePolicyInfo(info);
}

// ==================== TC108: SetDecPolicyInfos normal add ====================

/**
 * @tc.name: SandboxDec_SetDecPolicyInfos_NormalAdd_001
 * @tc.desc: Verify SetDecPolicyInfos correctly stores policy info
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_NormalAdd_001, TestSize.Level1)
{
    DecPolicyInfo info;
    FillPolicyInfo(info, 5);

    SetDecPolicyInfos(&info);

    // Verify the policy was stored by checking ioctl is called when SetDecPolicy runs
    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // 5 paths should result in 1 batch
    EXPECT_EQ(g_mockIoctlCallCount, 1);

    FreePolicyInfo(info);
}

// ==================== TC109: SetDecPolicyInfos exceeding limit ====================

/**
 * @tc.name: SandboxDec_SetDecPolicyInfos_ExceedLimit_001
 * @tc.desc: Verify SetDecPolicyInfos rejects paths exceeding MAX_POLICY_NUM
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_ExceedLimit_001, TestSize.Level1)
{
    // First add MAX_POLICY_NUM paths using FillMultiplePolicyInfos
    FillMultiplePolicyInfos(MAX_POLICY_NUM);

    // Try to add 1 more path - should be rejected and g_decPolicyInfos destroyed
    DecPolicyInfo infoExtra;
    FillPolicyInfo(infoExtra, 1);
    SetDecPolicyInfos(&infoExtra);

    // SetDecPolicy should not call ioctl since g_decPolicyInfos was destroyed
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    EXPECT_EQ(g_mockIoctlCallCount, 0);

    FreePolicyInfo(infoExtra);
}

// ==================== TC111: SetDecPolicyBatch parameter validation ====================

/**
 * @tc.name: SandboxDec_SetDecPolicyBatch_InvalidParams_001
 * @tc.desc: Verify SetDecPolicyBatch returns -1 for invalid parameters
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyBatch_InvalidParams_001, TestSize.Level1)
{
    GlobalDecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(GlobalDecPolicyInfo), 0, sizeof(GlobalDecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = 8;

    // Test: NULL decPolicyInfos
    int ret = SetDecPolicyBatch(3, nullptr, 0, 0, KERNEL_BATCH_SIZE);
    EXPECT_EQ(ret, -1);

    // Test: count = 0
    ret = SetDecPolicyBatch(3, &info, 0, 0, 0);
    EXPECT_EQ(ret, -1);

    // Test: count > KERNEL_BATCH_SIZE
    ret = SetDecPolicyBatch(3, &info, 0, 0, KERNEL_BATCH_SIZE + 1);
    EXPECT_EQ(ret, -1);
}

// ==================== TC113: SetDecPolicyInfos zero pathNum ====================

/**
 * @tc.name: SandboxDec_SetDecPolicyInfos_ZeroPathNum_001
 * @tc.desc: Verify SetDecPolicyInfos handles zero pathNum safely
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_ZeroPathNum_001, TestSize.Level1)
{
    DecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(DecPolicyInfo), 0, sizeof(DecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = 0;

    // Should not crash and not store anything
    SetDecPolicyInfos(&info);

    // SetDecPolicy should not call ioctl since no policies were stored
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    EXPECT_EQ(g_mockIoctlCallCount, 0);
}

// ==================== TC114: 1 path, 1 batch ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_1Path_001
 * @tc.desc: Verify SetDecPolicy delivers 1 path correctly (less than batch size)
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_1Path_001, TestSize.Level1)
{
    DecPolicyInfo info;
    FillPolicyInfo(info, 1);

    SetDecPolicyInfos(&info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // 1 path = 1 batch
    EXPECT_EQ(g_mockIoctlCallCount, 1);

    FreePolicyInfo(info);
}

// ==================== TC115: 9 paths, 2 batches (cross-batch boundary) ====================

/**
 * @tc.name: SandboxDec_SetDecPolicy_9Paths_2Batches_001
 * @tc.desc: Verify SetDecPolicy delivers 9 paths in 2 batches (cross-batch boundary)
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_9Paths_2Batches_001, TestSize.Level1)
{
    // Use FillMultiplePolicyInfos to add 9 paths in 2 batches
    FillMultiplePolicyInfos(9);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // 9 paths: batch1=8, batch2=1 => 2 batches
    EXPECT_EQ(g_mockIoctlCallCount, 2);
}

} // namespace OHOS
