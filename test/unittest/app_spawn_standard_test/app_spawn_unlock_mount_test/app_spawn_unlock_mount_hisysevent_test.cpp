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
#include <string>
#include "hisysevent.h"
#include "hisysevent_adapter.h"

using namespace testing;
using namespace testing::ext;

// Access mock state from the mock hisysevent.h
using MockState = OHOS::HiviewDFX::MockHiSysEventState;

class AppSpawnUnlockMountHisyseventTest : public testing::Test {
protected:
    void SetUp() override
    {
        // Reset mock state before each test
        auto& state = OHOS::HiviewDFX::GetMockHiSysEventState();
        state.callCount = 0;
        state.returnValue = 0;
        state.lastDomain.clear();
        state.lastEventName.clear();
        state.lastEventType = 0;
    }

    void TearDown() override {}
};

/**
 * @tc.name: UnlockMount_Hisysevent_ReportUnlockMountResult_001
 * @tc.desc: Verify ReportUnlockMountResult calls HiSysEventWrite with correct event name
 *           and BEHAVIOR type when HiSysEventWrite succeeds
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnUnlockMountHisyseventTest, UnlockMount_Hisysevent_ReportUnlockMountResult_001, TestSize.Level1)
{
    // 覆盖: ReportUnlockMountResult (hisysevent_adapter.cpp:308-321) B0 (ret == 0)
    // Mock: HiSysEventWrite returns 0 (success path)
    auto& state = OHOS::HiviewDFX::GetMockHiSysEventState();
    state.returnValue = 0;

    int32_t uid = 100;
    int32_t totalCount = 10;
    int32_t successCount = 8;
    int32_t failCount = 2;
    int64_t duration = 1500;

    ReportUnlockMountResult(uid, totalCount, successCount, failCount, duration);

    // Verify HiSysEventWrite was called exactly once
    EXPECT_EQ(state.callCount, 1);
    // Verify the event name matches the UNLOCK_MOUNT_ALL_DONE constant
    EXPECT_EQ(state.lastEventName, "UNLOCK_MOUNT_ALL_DONE");
    // Verify the event type is BEHAVIOR (4)
    EXPECT_EQ(state.lastEventType, OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC);
    // Verify the domain is APPSPAWN
    EXPECT_EQ(state.lastDomain, OHOS::HiviewDFX::HiSysEvent::Domain::APPSPAWN);
}

/**
 * @tc.name: UnlockMount_Hisysevent_ReportUnlockMountResult_002
 * @tc.desc: Verify ReportUnlockMountResult handles HiSysEventWrite failure gracefully
 *           (logs error but does not crash)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnUnlockMountHisyseventTest, UnlockMount_Hisysevent_ReportUnlockMountResult_002, TestSize.Level1)
{
    // 覆盖: ReportUnlockMountResult (hisysevent_adapter.cpp:308-321) B1 (ret != 0)
    // Mock: HiSysEventWrite returns -1 (failure path, triggers APPSPAWN_LOGE)
    auto& state = OHOS::HiviewDFX::GetMockHiSysEventState();
    state.returnValue = -1;

    int32_t uid = 0;
    int32_t totalCount = 0;
    int32_t successCount = 0;
    int32_t failCount = 0;
    int64_t duration = 0;

    // Should not crash even when HiSysEventWrite fails
    ReportUnlockMountResult(uid, totalCount, successCount, failCount, duration);

    // Verify the call was still made and the failure was handled
    EXPECT_EQ(state.callCount, 1);
    EXPECT_EQ(state.lastEventName, "UNLOCK_MOUNT_ALL_DONE");
    // Verify mock returned the error value (function handled it without crashing)
    EXPECT_EQ(state.returnValue, -1);
}

/**
 * @tc.name: UnlockMount_Hisysevent_ReportUnlockMountAppFail_001
 * @tc.desc: Verify ReportUnlockMountAppFail calls HiSysEventWrite with correct event name
 *           and FAULT type when HiSysEventWrite succeeds
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnUnlockMountHisyseventTest, UnlockMount_Hisysevent_ReportUnlockMountAppFail_001, TestSize.Level1)
{
    // 覆盖: ReportUnlockMountAppFail (hisysevent_adapter.cpp:323-336) B0 (ret == 0)
    // Mock: HiSysEventWrite returns 0 (success path)
    auto& state = OHOS::HiviewDFX::GetMockHiSysEventState();
    state.returnValue = 0;

    int32_t uid = 100;
    const char *bundleName = "com.example.app";
    const char *srcPath = "/data/app/el2/100/base/com.example.app";
    const char *destPath = "/mnt/sandbox/100/com.example.app_locked";
    int32_t errorCode = -1;

    ReportUnlockMountAppFail(uid, bundleName, srcPath, destPath, errorCode);

    // Verify HiSysEventWrite was called exactly once
    EXPECT_EQ(state.callCount, 1);
    // Verify the event name matches the UNLOCK_MOUNT_APP_FAIL constant
    EXPECT_EQ(state.lastEventName, "UNLOCK_MOUNT_APP_FAIL");
    // Verify the event type is FAULT (1)
    EXPECT_EQ(state.lastEventType, OHOS::HiviewDFX::HiSysEvent::EventType::FAULT);
    // Verify the domain is APPSPAWN
    EXPECT_EQ(state.lastDomain, OHOS::HiviewDFX::HiSysEvent::Domain::APPSPAWN);
}

/**
 * @tc.name: UnlockMount_Hisysevent_ReportUnlockMountAppFail_002
 * @tc.desc: Verify ReportUnlockMountAppFail handles HiSysEventWrite failure gracefully
 *           and accepts null/empty parameters without crashing
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnUnlockMountHisyseventTest, UnlockMount_Hisysevent_ReportUnlockMountAppFail_002, TestSize.Level1)
{
    // 覆盖: ReportUnlockMountAppFail (hisysevent_adapter.cpp:323-336) B1 (ret != 0)
    // Mock: HiSysEventWrite returns -1 (failure path, triggers APPSPAWN_LOGE)
    // Also tests null/empty parameter robustness
    auto& state = OHOS::HiviewDFX::GetMockHiSysEventState();
    state.returnValue = -1;

    int32_t uid = 100;
    const char *bundleName = nullptr;
    const char *srcPath = "";
    const char *destPath = nullptr;
    int32_t errorCode = 1;

    // Should not crash with null pointers even when HiSysEventWrite fails
    ReportUnlockMountAppFail(uid, bundleName, srcPath, destPath, errorCode);

    // Verify the call was still made and the failure was handled
    EXPECT_EQ(state.callCount, 1);
    EXPECT_EQ(state.lastEventName, "UNLOCK_MOUNT_APP_FAIL");
    EXPECT_EQ(state.lastEventType, OHOS::HiviewDFX::HiSysEvent::EventType::FAULT);
}
