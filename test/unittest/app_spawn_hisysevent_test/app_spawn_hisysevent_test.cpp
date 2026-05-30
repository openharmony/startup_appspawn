/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <cerrno>

#include <gtest/gtest.h>
#include "securec.h"

#include "app_spawn_hisysevent_stub.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnHisysEventTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
    }
};

/**
* @tc.name: AppSpawnHisysEvent_InitHisyseventTimer_01
* @tc.desc: Verify InitHisyseventTimer success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_InitHisyseventTimer_01, TestSize.Level0)
{
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, nullptr);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = nullptr;
}

/**
* @tc.name: AppSpawnHisysEvent_AddStatisticEventInfo_01
* @tc.desc: Verify Add bootevent count success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_AddStatisticEventInfo_01, TestSize.Level0)
{
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, nullptr);
    AddStatisticEventInfo(hisyseventInfo, 20, true);
    int count = hisyseventInfo->bootEvent.eventCount;
    EXPECT_EQ(count, 1);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = nullptr;
}

/**
* @tc.name: AppSpawnHisysEvent_AddStatisticEventInfo_02
* @tc.desc: Verify Add bootevent duration success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_AddStatisticEventInfo_02, TestSize.Level0)
{
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, nullptr);
    AddStatisticEventInfo(hisyseventInfo, 20, true);
    int duration = hisyseventInfo->bootEvent.totalDuration;
    EXPECT_EQ(duration, 20);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = nullptr;
}

/**
* @tc.name: AppSpawnHisysEvent_AddStatisticEventInfo_03
* @tc.desc: Verify Add manualevent duration success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_AddStatisticEventInfo_03, TestSize.Level0)
{
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, nullptr);
    AddStatisticEventInfo(hisyseventInfo, 20, false);
    int duration = hisyseventInfo->manualEvent.totalDuration;
    EXPECT_EQ(duration, 20);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = nullptr;
}

/**
* @tc.name: AppSpawnHisysEvent_AddStatisticEventInfo_04
* @tc.desc: Verify manualevent maxduration and minduration success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_AddStatisticEventInfo_04, TestSize.Level0)
{
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, nullptr);
    AddStatisticEventInfo(hisyseventInfo, 10, true);
    AddStatisticEventInfo(hisyseventInfo, 20, true);
    AddStatisticEventInfo(hisyseventInfo, 30, true);
    int maxduration = hisyseventInfo->bootEvent.maxDuration;
    int minduration = hisyseventInfo->bootEvent.minDuration;
    EXPECT_EQ(maxduration, 30);
    EXPECT_EQ(minduration, 10);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = nullptr;
}

} // namespace OHOS
