/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <csignal>

#include "devicedebug_base.h"
#include "devicedebug_kill.h"
#include "devicedebug_stub.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class DevicedebugKillTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void DevicedebugKillTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "Devicedebug_Kill_TEST SetUpTestCase";
}

void DevicedebugKillTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "Devicedebug_Kill_TEST TearDownTestCase";
}

void DevicedebugKillTest::SetUp()
{
    GTEST_LOG_(INFO) << "Devicedebug_Kill_TEST SetUp";
}

void DevicedebugKillTest::TearDown()
{
    GTEST_LOG_(INFO) << "Devicedebug_Kill_TEST TearDown";
}

/**
* @tc.name: DevicedebugCmdKill_001
* @tc.desc:  Verify develop mode and argc validate.
* @tc.type: FUNC
* @tc.require:issueIASN4F
* @tc.author:
*/
HWTEST_F(DevicedebugKillTest, DevicedebugCmdKill_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DevicedebugCmdKill_001 test....";
    {
        int argc = 3;
        char argv0[] = "devicedebug";
        char argv1[] = "kill";
        char argv2[] = "-9";
        char* argv[] = {argv0, argv1, argv2};
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), DEVICEDEBUG_ERRNO_NOT_IN_DEVELOPER_MODE);
        DeveloperModeOpenSet(1);
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), DEVICEDEBUG_ERRNO_OPERATOR_ARGV_MISS);
    }
    {
        int argc = 2;
        char argv0[] = "devicedebug";
        char argv1[] = "-h";
        char* argv[] = {argv0, argv1};
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), 0);
    }
    {
        DeveloperModeOpenSet(1);
        int argc = 3;
        char argv0[] = "devicedebug";
        char argv1[] = "kill";
        char argv2[] = "-9";
        char* argv[] = {argv0, argv1, argv2};
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), DEVICEDEBUG_ERRNO_OPERATOR_ARGV_MISS);
    }
}

/**
* @tc.name: DevicedebugCmdKill_002
* @tc.desc:  Verify signal validate.
* @tc.type: FUNC
* @tc.require:issueIASN4F
* @tc.author:
*/
HWTEST_F(DevicedebugKillTest, DevicedebugCmdKill_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DevicedebugCmdKill_002 test....";

    DeveloperModeOpenSet(1);
    {
        int argc = 4;
        char argv0[] = "devicedebug";
        char argv1[] = "kill";
        char argv2[] = "-0";
        char argv3[] = "12111";
        char* argv[] = {argv0, argv1, argv2, argv3};
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), DEVICEDEBUG_ERRNO_PARAM_INVALID);
    }
    {
        int argc = 4;
        char argv0[] = "devicedebug";
        char argv1[] = "kill";
        char argv2[] = "-65";
        char argv3[] = "12111";
        char* argv[] = {argv0, argv1, argv2, argv3};
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), DEVICEDEBUG_ERRNO_PARAM_INVALID);
    }
}

/**
* @tc.name: DevicedebugCmdKill_003
* @tc.desc:  Verify devicedebug kill success.
* @tc.type: FUNC
* @tc.require:issueIASN4F
* @tc.author:
*/
HWTEST_F(DevicedebugKillTest, DevicedebugCmdKill_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DevicedebugCmdKill_003 test....";

    DeveloperModeOpenSet(1);
    int argc = 4;
    char argv0[] = "devicedebug";
    char argv1[] = "kill";
    char argv2[] = "-9";
    char argv3[] = "12111";
    char* argv[] = {argv0, argv1, argv2, argv3};
    {
        AppSpawnClientInitRetSet(-1);
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), -1);
    }
    AppSpawnClientInitRetSet(0);
    {
        AppSpawnReqMsgCreateRetSet(-2);
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), -2);
    }
    AppSpawnReqMsgCreateRetSet(0);
    {
        AppSpawnReqMsgAddExtInfoRetSet(-3);
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), -3);
    }
    AppSpawnReqMsgAddExtInfoRetSet(0);
    {
        AppSpawnClientSendMsgRetSet(-4);
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), -4);
    }
    AppSpawnClientSendMsgRetSet(0);
    {
        AppSpawnClientSendMsgResultSet(-5);
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), -5);
    }
    AppSpawnClientSendMsgResultSet(0);
    {
        EXPECT_EQ(DeviceDebugCmdKill(argc, argv), 0);
    }
}

}