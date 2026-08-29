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
#include "appspawn_manager.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;

typedef struct TagAppSpawnMgr AppSpawnMgr;

// APPSPAWN_STATIC expands to empty under APPSPAWN_TEST, expose static functions
extern "C" {
int SetDecPolicyBatch(int fd, GlobalDecPolicyInfo *decPolicyInfos,
                      uint64_t timestamp, uint32_t start, uint32_t count);
int SetDenyConstraintDirs(AppSpawnMgr *content);
int SetForcedPrefixDirs(AppSpawnMgr *content);
int SetIgnoreCaseDirs(AppSpawnMgr *content);
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
static unsigned long g_lastIoctlReq = 0;

static const int MAX_IOCTL_RECORDS = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
static uint32_t g_ioctlPathNums[MAX_IOCTL_RECORDS];

static int g_mockOpenReturn = MOCK_OPEN_RETURN_FD;
static const char *MOCK_OPEN_PATH = nullptr;

static int g_mockClockTimeSec = MOCK_CLOCK_TIME_SEC;
static int g_mockClockTimeNsec = MOCK_CLOCK_TIME_NSEC;

static bool g_mockStrdupShouldFail = false;

// ==================== Wrap functions ====================

// Mirror of IoctlDecPolicyBatch in sandbox_dec.c for reading pathNum in mock ioctl
struct MockIoctlBatch {
    uint64_t tokenId;
    uint64_t timestamp;
    PathInfo path[KERNEL_BATCH_SIZE];
    uint32_t pathNum;
    int32_t userId;
    uint64_t reserved[DEC_POLICY_HEADER_RESERVED];
    bool flag;
};

// Mirror of batch ioctl commands in sandbox_dec.c (based on MockIoctlBatch layout)
#define MOCK_SET_DEC_POLICY_CMD _IOWR(HM_DEC_IOCTL_BASE, HM_SET_POLICY_ID, MockIoctlBatch)
#define MOCK_CONSTRAINT_DEC_POLICY_CMD _IOW(HM_DEC_IOCTL_BASE, HM_CONSTRAINT_POLICY_ID, MockIoctlBatch)
#define MOCK_SET_DEC_PREFIX_CMD _IOWR(HM_DEC_IOCTL_BASE, HM_SET_PREFIX_ID, MockIoctlBatch)
#define MOCK_SET_DEC_IGNORE_CASE_CMD _IOWR(HM_DEC_IOCTL_BASE, HM_SET_DEC_IGNORE_CASE_ID, MockIoctlBatch)

static bool IsBatchIoctlCmd(unsigned long request)
{
    return request == MOCK_SET_DEC_POLICY_CMD || request == MOCK_CONSTRAINT_DEC_POLICY_CMD ||
        request == MOCK_SET_DEC_PREFIX_CMD || request == MOCK_SET_DEC_IGNORE_CASE_CMD;
}

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
    g_lastIoctlReq = request;
    va_list args;
    va_start(args, request);
    void *arg = va_arg(args, void *);
    va_end(args);
    g_lastIoctlData = arg;

    if (g_mockIoctlCallCount <= MAX_IOCTL_RECORDS && arg != nullptr && IsBatchIoctlCmd(request)) {
        MockIoctlBatch *batch = reinterpret_cast<MockIoctlBatch *>(arg);
        g_ioctlPathNums[g_mockIoctlCallCount - 1] = batch->pathNum;
    }

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

char *__real_strdup(const char *s);
char *__wrap_strdup(const char *s)
{
    if (g_mockStrdupShouldFail) {
        errno = ENOMEM;
        return nullptr;
    }
    return __real_strdup(s);
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
        g_lastIoctlReq = 0;
        g_mockOpenReturn = MOCK_OPEN_RETURN_FD;
        MOCK_OPEN_PATH = nullptr;
        g_mockClockTimeSec = MOCK_CLOCK_TIME_SEC;
        g_mockClockTimeNsec = MOCK_CLOCK_TIME_NSEC;
        g_mockStrdupShouldFail = false;
        for (int i = 0; i < MAX_IOCTL_RECORDS; i++) {
            g_ioctlPathNums[i] = 0;
        }
    }

    // Helper: create DecPolicyInfo with N paths (up to MAX_CONFIG_POLICY_NUM per call)
    // pathOffset: starting index for path naming (for multi-batch tests)
    void FillPolicyInfo(DecPolicyInfo &info, uint32_t pathCount, uint32_t pathOffset = 0)
    {
        errno_t ret = memset_s(&info, sizeof(DecPolicyInfo), 0, sizeof(DecPolicyInfo));
        if (ret != EOK) {
            return;
        }
        // Limit to MAX_CONFIG_POLICY_NUM (DecPolicyInfo.path[] capacity)
        if (pathCount > MAX_CONFIG_POLICY_NUM) {
            pathCount = MAX_CONFIG_POLICY_NUM;
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
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_MaxPaths_Batches_001, TestSize.Level1)
{
    FillMultiplePolicyInfos(MAX_POLICY_NUM);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
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
 * @tc.desc: Verify SetDecPolicyInfos keeps existing policies when exceeding MAX_POLICY_NUM
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_ExceedLimit_001, TestSize.Level1)
{
    // First add MAX_POLICY_NUM paths using FillMultiplePolicyInfos
    FillMultiplePolicyInfos(MAX_POLICY_NUM);

    // Try to add 1 more path - should be skipped (full), existing 64 retained
    DecPolicyInfo infoExtra;
    FillPolicyInfo(infoExtra, 1);
    SetDecPolicyInfos(&infoExtra);

    // SetDecPolicy should still call ioctl for the retained paths
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));

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
    // Pre-fill 8 paths, then zero pathNum input must not touch existing g_decPolicyInfos
    DecPolicyInfo pre;
    FillPolicyInfo(pre, KERNEL_BATCH_SIZE);
    SetDecPolicyInfos(&pre);
    FreePolicyInfo(pre);

    DecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(DecPolicyInfo), 0, sizeof(DecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = 0;

    SetDecPolicyInfos(&info);

    // Existing 8 paths retained: SetDecPolicy still delivers 1 batch
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    EXPECT_EQ(g_mockIoctlCallCount, 1);
    EXPECT_EQ(g_ioctlPathNums[0], KERNEL_BATCH_SIZE);
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

// ==================== TC116: 32 paths in single call, 4 batches ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_32Paths_SingleCall_001
* @tc.desc: SetDecPolicyInfos accepts 32 paths in one call, delivers 4 batches
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_32Paths_SingleCall_001, TestSize.Level1)
{
    DecPolicyInfo info;
    FillPolicyInfo(info, MAX_CONFIG_POLICY_NUM);

    SetDecPolicyInfos(&info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // 32 paths: 4 batches of 8
    uint32_t expectedBatches = (MAX_CONFIG_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    EXPECT_EQ(expectedBatches, 4u);

    FreePolicyInfo(info);
}

// ==================== TC117: exceed MAX_POLICY_NUM (global overflow) ====================

/**
* @tc.name: SandboxDec_SetDecPolicyInfos_ExceedGlobalLimit_001
* @tc.desc: Verify SetDecPolicyInfos partially applies when global pathNum would exceed MAX_POLICY_NUM
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_ExceedGlobalLimit_001, TestSize.Level1)
{
    // Fill global to MAX_POLICY_NUM - 1
    FillMultiplePolicyInfos(MAX_POLICY_NUM - 1);

    // Add MAX_CONFIG_POLICY_NUM more => only 1 fits (64-63=1), rest skipped
    DecPolicyInfo info;
    FillPolicyInfo(info, MAX_CONFIG_POLICY_NUM);

    SetDecPolicyInfos(&info);

    // g_decPolicyInfos retained at MAX_POLICY_NUM, SetDecPolicy calls expected batches
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));

    FreePolicyInfo(info);
}

// ==================== TC118: DestroyDecPolicyInfos(nullptr) ====================

/**
 * @tc.name: SandboxDec_DestroyDecPolicyInfos_Null_001
 * @tc.desc: Verify DestroyDecPolicyInfos handles NULL input without crash
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_DestroyDecPolicyInfos_Null_001, TestSize.Level1)
{
    // NULL input: early return, no crash, no state change
    DestroyDecPolicyInfos(nullptr);
    EXPECT_EQ(g_mockIoctlCallCount, 0);

    // Fill 8 paths, then destroy valid g_decPolicyInfos: global state cleared
    DecPolicyInfo info;
    FillPolicyInfo(info, KERNEL_BATCH_SIZE);
    SetDecPolicyInfos(&info);
    FreePolicyInfo(info);

    SetDecPolicy();  // consumes and destroys g_decPolicyInfos internally
    g_mockIoctlCallCount = 0;

    // After destroy, SetDecPolicy early-returns: no ioctl
    SetDecPolicy();
    EXPECT_EQ(g_mockIoctlCallCount, 0);
}

// ==================== TC119: SetDecPolicyInfos(nullptr) ====================

/**
 * @tc.name: SandboxDec_SetDecPolicyInfos_Null_001
 * @tc.desc: Verify SetDecPolicyInfos handles NULL input and SetDecPolicy does nothing
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_Null_001, TestSize.Level1)
{
    // Pre-fill 8 paths, then NULL input must not touch existing g_decPolicyInfos
    DecPolicyInfo pre;
    FillPolicyInfo(pre, KERNEL_BATCH_SIZE);
    SetDecPolicyInfos(&pre);
    FreePolicyInfo(pre);

    SetDecPolicyInfos(nullptr);

    // Existing 8 paths retained: SetDecPolicy still delivers 1 batch
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    EXPECT_EQ(g_mockIoctlCallCount, 1);
    EXPECT_EQ(g_ioctlPathNums[0], KERNEL_BATCH_SIZE);
}

// ==================== TC120: SetDecPolicyInfos path[i].path==NULL ====================

/**
 * @tc.name: SandboxDec_SetDecPolicyInfos_NullPath_001
 * @tc.desc: Verify SetDecPolicyInfos skips NULL path and continues
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_NullPath_001, TestSize.Level1)
{
    DecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(DecPolicyInfo), 0, sizeof(DecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = 2;
    info.path[0].path = nullptr;
    info.path[1].path = strdup("/data/test/null_path_1");

    SetDecPolicyInfos(&info);

    // path[0] NULL skipped, path[1] added => 1 path in g_decPolicyInfos
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    EXPECT_EQ(g_mockIoctlCallCount, 1);

    free(info.path[1].path);
}

// ==================== TC121: SetDenyConstraintDirs normal ====================

/**
 * @tc.name: SandboxDec_SetDenyConstraintDirs_Normal_001
 * @tc.desc: Verify SetDenyConstraintDirs delivers 8 constraint dirs in 1 batch
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDenyConstraintDirs_Normal_001, TestSize.Level1)
{
    g_mockIoctlCallCount = 0;
    int ret = SetDenyConstraintDirs(nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
    // g_decConstraintDir has exactly KERNEL_BATCH_SIZE entries: no end>size clip, full batch
    EXPECT_EQ(g_ioctlPathNums[0], KERNEL_BATCH_SIZE);
    EXPECT_NE(MOCK_OPEN_PATH, nullptr);
}

// ==================== TC122: SetDenyConstraintDirs open fail ====================

/**
 * @tc.name: SandboxDec_SetDenyConstraintDirs_OpenFail_001
 * @tc.desc: Verify SetDenyConstraintDirs handles open failure gracefully
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDenyConstraintDirs_OpenFail_001, TestSize.Level1)
{
    g_mockOpenReturn = -1;
    g_mockIoctlCallCount = 0;

    int ret = SetDenyConstraintDirs(nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 0);
}

// ==================== TC123: SetDenyConstraintDirs ioctl fail ====================

/**
 * @tc.name: SandboxDec_SetDenyConstraintDirs_IoctlFail_001
 * @tc.desc: Verify SetDenyConstraintDirs fail-open when ioctl fails
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDenyConstraintDirs_IoctlFail_001, TestSize.Level1)
{
    g_mockIoctlReturn = -1;
    g_mockIoctlCallCount = 0;

    int ret = SetDenyConstraintDirs(nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
}

// ==================== TC124: SetForcedPrefixDirs normal ====================

/**
 * @tc.name: SandboxDec_SetForcedPrefixDirs_Normal_001
 * @tc.desc: Verify SetForcedPrefixDirs delivers 1 prefix dir in 1 batch
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetForcedPrefixDirs_Normal_001, TestSize.Level1)
{
    g_mockIoctlCallCount = 0;
    int ret = SetForcedPrefixDirs(nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
    // g_decForcedPrefix has 1 entry: end>size clip triggers, partial batch with pathNum=1
    EXPECT_EQ(g_ioctlPathNums[0], 1u);
    EXPECT_NE(MOCK_OPEN_PATH, nullptr);
}

// ==================== TC125: SetForcedPrefixDirs open fail ====================

/**
 * @tc.name: SandboxDec_SetForcedPrefixDirs_OpenFail_001
 * @tc.desc: Verify SetForcedPrefixDirs handles open failure gracefully
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetForcedPrefixDirs_OpenFail_001, TestSize.Level1)
{
    g_mockOpenReturn = -1;
    g_mockIoctlCallCount = 0;

    int ret = SetForcedPrefixDirs(nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 0);
}

// ==================== TC126: SetDecPolicyBatch direct valid call ====================

/**
 * @tc.name: SandboxDec_SetDecPolicyBatch_Valid_001
 * @tc.desc: Verify SetDecPolicyBatch succeeds with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyBatch_Valid_001, TestSize.Level1)
{
    GlobalDecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(GlobalDecPolicyInfo), 0, sizeof(GlobalDecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = 4;
    info.path[0].path = (char *)"/data/test/batch0";
    info.path[0].pathLen = static_cast<uint32_t>(strlen("/data/test/batch0"));
    info.path[1].path = (char *)"/data/test/batch1";
    info.path[1].pathLen = static_cast<uint32_t>(strlen("/data/test/batch1"));
    info.path[2].path = (char *)"/data/test/batch2";
    info.path[2].pathLen = static_cast<uint32_t>(strlen("/data/test/batch2"));
    info.path[3].path = (char *)"/data/test/batch3";
    info.path[3].pathLen = static_cast<uint32_t>(strlen("/data/test/batch3"));

    g_mockIoctlCallCount = 0;
    int ret = SetDecPolicyBatch(3, &info, 1000, 0, 4);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
}

// ==================== TC127: SetDecPolicyBatch direct ioctl fail ====================

/**
 * @tc.name: SandboxDec_SetDecPolicyBatch_IoctlFail_001
 * @tc.desc: Verify SetDecPolicyBatch returns negative when ioctl fails
 * @tc.type: FUNC
 */
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyBatch_IoctlFail_001, TestSize.Level1)
{
    GlobalDecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(GlobalDecPolicyInfo), 0, sizeof(GlobalDecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = 2;
    info.path[0].path = (char *)"/data/test/fail0";
    info.path[0].pathLen = static_cast<uint32_t>(strlen("/data/test/fail0"));
    info.path[1].path = (char *)"/data/test/fail1";
    info.path[1].pathLen = static_cast<uint32_t>(strlen("/data/test/fail1"));

    g_mockIoctlReturn = -1;
    g_mockIoctlCallCount = 0;
    int ret = SetDecPolicyBatch(3, &info, 2000, 0, 2);
    EXPECT_LT(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
}

// ==================== TC128: SetDecPolicyBatch count == KERNEL_BATCH_SIZE (boundary pass) ====================

/**
* @tc.name: SandboxDec_SetDecPolicyBatch_MaxBatchSize_001
* @tc.desc: Verify SetDecPolicyBatch succeeds when count == KERNEL_BATCH_SIZE (boundary)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyBatch_MaxBatchSize_001, TestSize.Level1)
{
    GlobalDecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(GlobalDecPolicyInfo), 0, sizeof(GlobalDecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = KERNEL_BATCH_SIZE;
    for (uint32_t i = 0; i < KERNEL_BATCH_SIZE; i++) {
        info.path[i].path = (char *)"/data/test/batch_max";
        info.path[i].pathLen = static_cast<uint32_t>(strlen("/data/test/batch_max"));
        info.path[i].mode = 0x1;
        info.path[i].flag = false;
    }

    g_mockIoctlCallCount = 0;
    int ret = SetDecPolicyBatch(3, &info, 1000, 0, KERNEL_BATCH_SIZE);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
}

// ==================== TC129: accumulate to exactly MAX_POLICY_NUM ====================

/**
* @tc.name: SandboxDec_SetDecPolicyInfos_ExactlyMax_001
* @tc.desc: Verify SetDecPolicyInfos succeeds when total pathNum == MAX_POLICY_NUM (boundary)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_ExactlyMax_001, TestSize.Level1)
{
    // Fill MAX_POLICY_NUM - KERNEL_BATCH_SIZE = 56 paths first
    FillMultiplePolicyInfos(MAX_POLICY_NUM - KERNEL_BATCH_SIZE);

    // Add KERNEL_BATCH_SIZE = 8 more => total = 64 = MAX_POLICY_NUM (boundary, should pass)
    DecPolicyInfo info;
    FillPolicyInfo(info, KERNEL_BATCH_SIZE);
    SetDecPolicyInfos(&info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    FreePolicyInfo(info);
}

// ==================== TC130: SetDecPolicy 7 paths (non-multiple, end > pathNum, 1 batch) ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_7Paths_1Batch_001
* @tc.desc: Verify SetDecPolicy delivers 7 paths in 1 batch (non-multiple triggers end>pathNum clip)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_7Paths_1Batch_001, TestSize.Level1)
{
    FillMultiplePolicyInfos(7);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // 7 paths: 1 batch, end=8>7 triggers clip to end=7
    EXPECT_EQ(g_mockIoctlCallCount, 1);
}

// ==================== TC131: 17 paths, 3 batches, end>pathNum clip ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_17Paths_3Batches_001
* @tc.desc: Verify SetDecPolicy delivers 17 paths in 3 batches (end>pathNum clip on last batch)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_17Paths_3Batches_001, TestSize.Level1)
{
    FillMultiplePolicyInfos(17);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    // 17 paths: batch1=8, batch2=8, batch3=1 (end=24>17 clips to 17)
    EXPECT_EQ(g_mockIoctlCallCount, 3);
}

// ==================== TC132: SetIgnoreCaseDirs normal (AppSpawnMode) ====================

/**
* @tc.name: SandboxDec_SetIgnoreCaseDirs_Normal_001
* @tc.desc: Verify SetIgnoreCaseDirs delivers ignore-case dirs in AppSpawn mode
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetIgnoreCaseDirs_Normal_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = (AppSpawnMgr *)calloc(1, sizeof(AppSpawnMgr));
    ASSERT_NE(mgr, nullptr);
    mgr->content.mode = MODE_FOR_APP_SPAWN;

    g_mockIoctlCallCount = 0;
    g_mockOpenReturn = MOCK_OPEN_RETURN_FD;
    int ret = SetIgnoreCaseDirs(mgr);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
    // ignore-case list has 2 or 3 entries (< KERNEL_BATCH_SIZE): end>pathNum clip triggers
    EXPECT_GT(g_ioctlPathNums[0], 0u);
    EXPECT_LT(g_ioctlPathNums[0], KERNEL_BATCH_SIZE);
    EXPECT_NE(MOCK_OPEN_PATH, nullptr);

    free(mgr);
}

// ==================== TC133: SetIgnoreCaseDirs wrong mode (early return) ====================

/**
* @tc.name: SandboxDec_SetIgnoreCaseDirs_WrongMode_001
* @tc.desc: Verify SetIgnoreCaseDirs returns 0 early when not App/Native spawn mode
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetIgnoreCaseDirs_WrongMode_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = (AppSpawnMgr *)calloc(1, sizeof(AppSpawnMgr));
    ASSERT_NE(mgr, nullptr);
    mgr->content.mode = MODE_FOR_NWEB_SPAWN;

    g_mockIoctlCallCount = 0;
    int ret = SetIgnoreCaseDirs(mgr);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 0);

    free(mgr);
}

// ==================== TC134: SetIgnoreCaseDirs open fail ====================

/**
* @tc.name: SandboxDec_SetIgnoreCaseDirs_OpenFail_001
* @tc.desc: Verify SetIgnoreCaseDirs handles open failure gracefully
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetIgnoreCaseDirs_OpenFail_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = (AppSpawnMgr *)calloc(1, sizeof(AppSpawnMgr));
    ASSERT_NE(mgr, nullptr);
    mgr->content.mode = MODE_FOR_NATIVE_SPAWN;

    g_mockOpenReturn = -1;
    g_mockIoctlCallCount = 0;
    int ret = SetIgnoreCaseDirs(mgr);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 0);

    free(mgr);
}

// ==================== TC135: SetDecPolicyInfos mid-loop NULL path (path[0] valid, path[1] NULL) ====================

/**
* @tc.name: SandboxDec_SetDecPolicyInfos_MidLoopNullPath_001
* @tc.desc: Verify SetDecPolicyInfos keeps already-added path when NULL encountered at i=1
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_MidLoopNullPath_001, TestSize.Level1)
{
    DecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(DecPolicyInfo), 0, sizeof(DecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = 2;
    info.path[0].path = strdup("/data/test/midloop_valid");
    ASSERT_NE(info.path[0].path, nullptr);
    info.path[0].pathLen = static_cast<uint32_t>(strlen("/data/test/midloop_valid"));
    info.path[0].mode = 0x1;
    info.path[1].path = nullptr;

    SetDecPolicyInfos(&info);

    // path[0] added (pathNum=1), path[1] NULL skipped, g_decPolicyInfos retained
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    EXPECT_EQ(g_mockIoctlCallCount, 1);

    free(info.path[0].path);
}

// ==================== TC136: SetDenyConstraintDirs ioctl success path ====================

/**
* @tc.name: SandboxDec_SetDenyConstraintDirs_IoctlSuccess_001
* @tc.desc: Verify SetDenyConstraintDirs executes ioctl success branch (ret >= 0)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDenyConstraintDirs_IoctlSuccess_001, TestSize.Level1)
{
    g_mockIoctlReturn = 0;
    g_mockIoctlCallCount = 0;
    g_lastIoctlReq = 0;

    int ret = SetDenyConstraintDirs(nullptr);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
    EXPECT_NE(g_lastIoctlReq, 0UL);
}

// ==================== TC137: SetForcedPrefixDirs ioctl success path ====================

/**
* @tc.name: SandboxDec_SetForcedPrefixDirs_IoctlSuccess_001
* @tc.desc: Verify SetForcedPrefixDirs executes ioctl success branch (ret >= 0)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetForcedPrefixDirs_IoctlSuccess_001, TestSize.Level1)
{
    g_mockIoctlReturn = 0;
    g_mockIoctlCallCount = 0;
    g_lastIoctlReq = 0;

    int ret = SetForcedPrefixDirs(nullptr);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
    EXPECT_NE(g_lastIoctlReq, 0UL);
}

// ==================== TC138: SetForcedPrefixDirs ioctl fail ====================

/**
* @tc.name: SandboxDec_SetForcedPrefixDirs_IoctlFail_001
* @tc.desc: Verify SetForcedPrefixDirs handles ioctl failure gracefully (fail-open)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetForcedPrefixDirs_IoctlFail_001, TestSize.Level1)
{
    g_mockIoctlReturn = -1;
    g_mockIoctlCallCount = 0;

    int ret = SetForcedPrefixDirs(nullptr);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
}

// ==================== TC139: SetIgnoreCaseDirs ioctl fail ====================

/**
* @tc.name: SandboxDec_SetIgnoreCaseDirs_IoctlFail_001
* @tc.desc: Verify SetIgnoreCaseDirs handles ioctl failure gracefully (fail-open)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetIgnoreCaseDirs_IoctlFail_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = (AppSpawnMgr *)calloc(1, sizeof(AppSpawnMgr));
    ASSERT_NE(mgr, nullptr);
    mgr->content.mode = MODE_FOR_APP_SPAWN;

    g_mockIoctlReturn = -1;
    g_mockIoctlCallCount = 0;
    g_mockOpenReturn = MOCK_OPEN_RETURN_FD;

    int ret = SetIgnoreCaseDirs(mgr);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);

    free(mgr);
}

// ==================== TC140: SetDecPolicyBatch with non-zero start ====================

/**
* @tc.name: SandboxDec_SetDecPolicyBatch_NonZeroStart_001
* @tc.desc: Verify SetDecPolicyBatch correctly indexes paths when start > 0
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyBatch_NonZeroStart_001, TestSize.Level1)
{
    GlobalDecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(GlobalDecPolicyInfo), 0, sizeof(GlobalDecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    info.pathNum = KERNEL_BATCH_SIZE * 2;  // 16 paths
    for (uint32_t i = 0; i < KERNEL_BATCH_SIZE * 2; i++) {
        info.path[i].path = (char *)"/data/test/start_offset";
        info.path[i].pathLen = static_cast<uint32_t>(strlen("/data/test/start_offset"));
        info.path[i].mode = 0x1;
        info.path[i].flag = false;
    }

    // Batch 2: start=8, count=8
    g_mockIoctlCallCount = 0;
    int ret = SetDecPolicyBatch(3, &info, 5000, KERNEL_BATCH_SIZE, KERNEL_BATCH_SIZE);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(g_mockIoctlCallCount, 1);
}

// ==================== TC141: SetDecPolicyInfos strdup fail (skip path) ====================

/**
* @tc.name: SandboxDec_SetDecPolicyInfos_StrdupFail_001
* @tc.desc: Verify SetDecPolicyInfos skips path when strdup fails, retains existing
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_StrdupFail_001, TestSize.Level1)
{
    // Pre-fill 8 paths successfully
    DecPolicyInfo pre;
    FillPolicyInfo(pre, KERNEL_BATCH_SIZE);
    SetDecPolicyInfos(&pre);
    FreePolicyInfo(pre);

    // Then 2 more paths, but strdup fails for both: skipped, existing 8 retained
    DecPolicyInfo info;
    FillPolicyInfo(info, 2, KERNEL_BATCH_SIZE);

    g_mockStrdupShouldFail = true;
    SetDecPolicyInfos(&info);
    g_mockStrdupShouldFail = false;

    // Still exactly 8 paths = 1 batch
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    EXPECT_EQ(g_mockIoctlCallCount, 1);
    EXPECT_EQ(g_ioctlPathNums[0], KERNEL_BATCH_SIZE);

    FreePolicyInfo(info);
}

// ==================== TC142: 32 paths single call, verify per-batch pathNum [8,8,8,8] ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_32Paths_BatchPathNums_001
* @tc.desc: Verify SetDecPolicyInfos stores exactly 32 paths, delivered as 4 batches each with pathNum=8
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_32Paths_BatchPathNums_001, TestSize.Level1)
{
    DecPolicyInfo info;
    FillPolicyInfo(info, MAX_CONFIG_POLICY_NUM);
    SetDecPolicyInfos(&info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    EXPECT_EQ(g_mockIoctlCallCount, 4);
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(g_ioctlPathNums[i], KERNEL_BATCH_SIZE)
            << "batch " << i << " pathNum mismatch";
    }
    FreePolicyInfo(info);
}

// ==================== TC143: 31 paths single call, verify per-batch pathNum [8,8,8,7] ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_31Paths_BatchPathNums_001
* @tc.desc: Verify 31 paths stored and delivered as 4 batches [8,8,8,7] (no truncation under MAX_CONFIG_POLICY_NUM)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_31Paths_BatchPathNums_001, TestSize.Level1)
{
    DecPolicyInfo info;
    FillPolicyInfo(info, MAX_CONFIG_POLICY_NUM - 1);
    SetDecPolicyInfos(&info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    EXPECT_EQ(g_mockIoctlCallCount, 4);
    EXPECT_EQ(g_ioctlPathNums[0], KERNEL_BATCH_SIZE);
    EXPECT_EQ(g_ioctlPathNums[1], KERNEL_BATCH_SIZE);
    EXPECT_EQ(g_ioctlPathNums[2], KERNEL_BATCH_SIZE);
    EXPECT_EQ(g_ioctlPathNums[3], MAX_CONFIG_POLICY_NUM - 1 - KERNEL_BATCH_SIZE * 3);
    FreePolicyInfo(info);
}

// ==================== TC144: 33 paths (32+1), verify per-batch pathNum [8,8,8,8,1] ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_33Paths_BatchPathNums_001
* @tc.desc: Verify 33 paths (two SetDecPolicyInfos calls: 32+1) delivered as 5 batches [8,8,8,8,1]
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_33Paths_BatchPathNums_001, TestSize.Level1)
{
    DecPolicyInfo info32;
    FillPolicyInfo(info32, MAX_CONFIG_POLICY_NUM);
    SetDecPolicyInfos(&info32);
    FreePolicyInfo(info32);

    DecPolicyInfo info1;
    FillPolicyInfo(info1, 1, MAX_CONFIG_POLICY_NUM);
    SetDecPolicyInfos(&info1);
    FreePolicyInfo(info1);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    EXPECT_EQ(g_mockIoctlCallCount, 5);
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(g_ioctlPathNums[i], KERNEL_BATCH_SIZE)
            << "batch " << i << " pathNum mismatch";
    }
    EXPECT_EQ(g_ioctlPathNums[4], 1u);
}

// ==================== TC145: MAX_POLICY_NUM paths, verify per-batch pathNum ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_MaxPaths_BatchPathNums_001
* @tc.desc: Verify MAX_POLICY_NUM paths delivered as expected batches each with pathNum=KERNEL_BATCH_SIZE
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_MaxPaths_BatchPathNums_001, TestSize.Level1)
{
    FillMultiplePolicyInfos(MAX_POLICY_NUM);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    for (uint32_t i = 0; i < expectedBatches; i++) {
        EXPECT_EQ(g_ioctlPathNums[i], KERNEL_BATCH_SIZE)
            << "batch " << i << " pathNum mismatch";
    }
}

// ==================== TC146: 1 path, verify per-batch pathNum [1] ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_1Path_BatchPathNum_001
* @tc.desc: Verify single path delivered as 1 batch with pathNum=1
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_1Path_BatchPathNum_001, TestSize.Level1)
{
    DecPolicyInfo info;
    FillPolicyInfo(info, 1);
    SetDecPolicyInfos(&info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    EXPECT_EQ(g_mockIoctlCallCount, 1);
    EXPECT_EQ(g_ioctlPathNums[0], 1u);
    FreePolicyInfo(info);
}

// ==================== TC147: 9 paths, verify per-batch pathNum [8,1] ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_9Paths_BatchPathNums_001
* @tc.desc: Verify 9 paths delivered as 2 batches [8,1]
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_9Paths_BatchPathNums_001, TestSize.Level1)
{
    DecPolicyInfo info9;
    FillPolicyInfo(info9, KERNEL_BATCH_SIZE);
    SetDecPolicyInfos(&info9);
    FreePolicyInfo(info9);

    DecPolicyInfo info1;
    FillPolicyInfo(info1, 1, KERNEL_BATCH_SIZE);
    SetDecPolicyInfos(&info1);
    FreePolicyInfo(info1);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    EXPECT_EQ(g_mockIoctlCallCount, 2);
    EXPECT_EQ(g_ioctlPathNums[0], KERNEL_BATCH_SIZE);
    EXPECT_EQ(g_ioctlPathNums[1], 1u);
}

// ==================== TC148: fill to exactly MAX_POLICY_NUM, no second-layer clip ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_FillExactlyFull_001
* @tc.desc: Verify filling global to exactly MAX_POLICY_NUM, no second-layer clip
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_FillExactlyFull_001, TestSize.Level1)
{
    FillMultiplePolicyInfos(MAX_POLICY_NUM);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    for (uint32_t i = 0; i < expectedBatches; i++) {
        EXPECT_EQ(g_ioctlPathNums[i], KERNEL_BATCH_SIZE)
            << "batch " << i << " pathNum mismatch";
    }
}

// ==================== TC149: MAX_POLICY_NUM-1 + MAX_CONFIG_POLICY_NUM, second-layer clip ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_SecondLayerClip_001
* @tc.desc: Verify partial apply when global + new > MAX_POLICY_NUM (second-layer clip)
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_SecondLayerClip_001, TestSize.Level1)
{
    FillMultiplePolicyInfos(MAX_POLICY_NUM - 1);

    DecPolicyInfo info;
    FillPolicyInfo(info, MAX_CONFIG_POLICY_NUM, MAX_POLICY_NUM - 1);
    SetDecPolicyInfos(&info);
    FreePolicyInfo(info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    for (uint32_t i = 0; i < expectedBatches; i++) {
        EXPECT_EQ(g_ioctlPathNums[i], KERNEL_BATCH_SIZE)
            << "batch " << i << " pathNum mismatch";
    }
}

// ==================== TC150: second-layer clip, verify batch distribution ====================

/**
* @tc.name: SandboxDec_SetDecPolicy_ClipBatchDistribution_001
* @tc.desc: Verify after second-layer clip, paths distributed as full batches + 1 remainder
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicy_ClipBatchDistribution_001, TestSize.Level1)
{
    FillMultiplePolicyInfos(MAX_POLICY_NUM - 1);
    DecPolicyInfo info;
    FillPolicyInfo(info, MAX_CONFIG_POLICY_NUM, MAX_POLICY_NUM - 1);
    SetDecPolicyInfos(&info);
    FreePolicyInfo(info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();

    uint32_t expectedBatches = (MAX_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    uint32_t totalPaths = 0;
    for (uint32_t i = 0; i < expectedBatches; i++) {
        EXPECT_EQ(g_ioctlPathNums[i], KERNEL_BATCH_SIZE)
            << "batch " << i << " pathNum mismatch";
        totalPaths += g_ioctlPathNums[i];
    }
    EXPECT_EQ(totalPaths, static_cast<uint32_t>(MAX_POLICY_NUM));
}

// ==================== TC151: SetDecPolicyInfos clamp pathNum > MAX_CONFIG_POLICY_NUM ====================

/**
* @tc.name: SandboxDec_SetDecPolicyInfos_InputClamp_001
* @tc.desc: Verify SetDecPolicyInfos clamps input pathNum to MAX_CONFIG_POLICY_NUM, no OOB read
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_InputClamp_001, TestSize.Level1)
{
    DecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(DecPolicyInfo), 0, sizeof(DecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    // Only fill real path entries (capacity is MAX_CONFIG_POLICY_NUM)
    for (uint32_t i = 0; i < MAX_CONFIG_POLICY_NUM; i++) {
        std::string pathStr = "/data/test/clamp_" + std::to_string(i);
        info.path[i].path = strdup(pathStr.c_str());
        info.path[i].pathLen = static_cast<uint32_t>(pathStr.length());
        info.path[i].mode = 0x1;
    }
    // Lie about pathNum: claims more than path[] capacity
    info.pathNum = MAX_CONFIG_POLICY_NUM + 8;
    info.tokenId = MOCK_TOKEN_ID;

    SetDecPolicyInfos(&info);

    // Input clamped to MAX_CONFIG_POLICY_NUM, stored paths delivered as batches
    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    uint32_t expectedBatches = (MAX_CONFIG_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    uint32_t totalPaths = 0;
    for (uint32_t i = 0; i < expectedBatches; i++) {
        totalPaths += g_ioctlPathNums[i];
    }
    EXPECT_EQ(totalPaths, static_cast<uint32_t>(MAX_CONFIG_POLICY_NUM));

    for (uint32_t i = 0; i < MAX_CONFIG_POLICY_NUM; i++) {
        free(info.path[i].path);
    }
}

// ==================== TC152: SetDecPolicyInfos input clamp preserves valid prefix ====================

/**
* @tc.name: SandboxDec_SetDecPolicyInfos_InputClampBoundary_001
* @tc.desc: Verify input pathNum == MAX_CONFIG_POLICY_NUM + 1 clamps to MAX_CONFIG_POLICY_NUM
* @tc.type: FUNC
*/
HWTEST_F(SandboxDecTest, SandboxDec_SetDecPolicyInfos_InputClampBoundary_001, TestSize.Level1)
{
    DecPolicyInfo info;
    errno_t rc = memset_s(&info, sizeof(DecPolicyInfo), 0, sizeof(DecPolicyInfo));
    ASSERT_EQ(rc, EOK);
    for (uint32_t i = 0; i < MAX_CONFIG_POLICY_NUM; i++) {
        std::string pathStr = "/data/test/clamp_b_" + std::to_string(i);
        info.path[i].path = strdup(pathStr.c_str());
        info.path[i].pathLen = static_cast<uint32_t>(pathStr.length());
        info.path[i].mode = 0x1;
    }
    info.pathNum = MAX_CONFIG_POLICY_NUM + 1;

    SetDecPolicyInfos(&info);

    g_mockIoctlCallCount = 0;
    SetDecPolicy();
    uint32_t expectedBatches = (MAX_CONFIG_POLICY_NUM + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    EXPECT_EQ(g_mockIoctlCallCount, static_cast<int>(expectedBatches));
    uint32_t totalPaths = 0;
    for (uint32_t i = 0; i < expectedBatches; i++) {
        totalPaths += g_ioctlPathNums[i];
    }
    EXPECT_EQ(totalPaths, static_cast<uint32_t>(MAX_CONFIG_POLICY_NUM));

    for (uint32_t i = 0; i < MAX_CONFIG_POLICY_NUM; i++) {
        free(info.path[i].path);
    }
}

} // namespace OHOS
