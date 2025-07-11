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

#include <gtest/gtest.h>
#include "app_spawn_hisysevent_stub.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnHisysEventTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppSpawnHisysEventTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "AppSpawn_HisysEvent_TEST SetUpTestCase";
}

void AppSpawnHisysEventTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "AppSpawn_HisysEvent_TEST TearDownTestCase";
}

void AppSpawnHisysEventTest::SetUp()
{
    GTEST_LOG_(INFO) << "AppSpawn_HisysEvent_TEST SetUp";
}

void AppSpawnHisysEventTest::TearDown()
{
    GTEST_LOG_(INFO) << "AppSpawn_HisysEvent_TEST TearDown";
}

/**
* @tc.name: AppSpawnHisysEvent_001
* @tc.desc:  Verify InitHisyseventTimer success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "AppSpawnHisysEvent_001 test....";
    SetCreateTimerStatus(true);
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, NULL);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = NULL;
}

/**
* @tc.name: AppSpawnHisysEvent_002
* @tc.desc:  Verify InitHisyseventTimer fail.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "AppSpawnHisysEvent_002 test....";
    SetCreateTimerStatus(false);
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, NULL);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = NULL;
}

/**
* @tc.name: AppSpawnHisysEvent_003
* @tc.desc:  Verify Add bootevent count success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "AppSpawnHisysEvent_003 test....";
    SetCreateTimerStatus(true);
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, NULL);
    AddStatisticEventInfo(hisyseventInfo, 20, true);
    int count = hisyseventInfo->bootEvent.eventCount;
    EXPECT_EQ(count, 1);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = NULL;
}

/**
* @tc.name: AppSpawnHisysEvent_004
* @tc.desc:  Verify Add bootevent duration success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "AppSpawnHisysEvent_004 test....";
    SetCreateTimerStatus(true);
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, NULL);
    AddStatisticEventInfo(hisyseventInfo, 20, true);
    int duration = hisyseventInfo->bootEvent.totalDuration;
    EXPECT_EQ(duration, 20);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = NULL;
}

/**
* @tc.name: AppSpawnHisysEvent_005
* @tc.desc:  Verify Add manualevent duration success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "AppSpawnHisysEvent_005 test....";
    SetCreateTimerStatus(true);
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, NULL);
    AddStatisticEventInfo(hisyseventInfo, 20, false);
    int duration = hisyseventInfo->manualEvent.totalDuration;
    EXPECT_EQ(duration, 20);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = NULL;
}

/**
* @tc.name: AppSpawnHisysEvent_006
* @tc.desc:  Verify manualevent maxduration and minduration success.
* @tc.type: FUNC
*/
HWTEST_F(AppSpawnHisysEventTest, AppSpawnHisysEvent_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "AppSpawnHisysEvent_006 test....";
    SetCreateTimerStatus(true);
    AppSpawnHisyseventInfo * hisyseventInfo = InitHisyseventTimer();
    EXPECT_NE(hisyseventInfo, NULL);
    AddStatisticEventInfo(hisyseventInfo, 10, true);
    AddStatisticEventInfo(hisyseventInfo, 20, true);
    AddStatisticEventInfo(hisyseventInfo, 30, true);
    int maxduration = hisyseventInfo->bootEvent.maxDuration;
    int minduration = hisyseventInfo->bootEvent.minDuration;
    EXPECT_EQ(maxduration, 30);
    EXPECT_EQ(minduration, 10);
    DeleteHisyseventInfo(hisyseventInfo);
    hisyseventInfo = NULL;
}
} // namespace OHOS
