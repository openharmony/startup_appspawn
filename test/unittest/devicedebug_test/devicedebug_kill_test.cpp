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
* @tc.name: Devicedebug_Kill_001
* @tc.desc:  Verify set Arg if HnpCmdInstall succeed.
* @tc.type: FUNC
* @tc.require:issueI9BU5F
* @tc.author:
*/
HWTEST_F(DevicedebugKillTest, Devicedebug_Kill_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "Devicedebug_Kill_001 test....";
}

}