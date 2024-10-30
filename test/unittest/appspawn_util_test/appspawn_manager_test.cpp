/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include "appspawn_utils.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnManagerTest : public testing::Test {
public:
    AppSpawnMgr content;
    AppSpawningCtx property;
    AppSpawnMsgNode message;
    static void SetUpTestCase()
    {
        APPSPAWN_LOGI("SetUpTestCase");
    }

    static void TearDownTestCase()
    {
        APPSPAWN_LOGI("TearDownTestCase");
    }

    void SetUp()
    {
        APPSPAWN_LOGI("SetUp");
    }

    void TearDown()
    {
        APPSPAWN_LOGI("TearDown");
    }
};

// 测试用例
HWTEST_F(AppSpawnManagerTest, IsSpawnServer_01, TestSize.Level0)
{
    // 测试 content 为 NULL 的情况
    EXPECT_EQ(IsSpawnServer(NULL), 0);
}

HWTEST_F(AppSpawnManagerTest, IsSpawnServer_02, TestSize.Level0)
{
    // 测试 servicePid 不等于当前进程 PID 的情况
    content.servicePid = getpid() + 1;  // 设置为不同的 PID
    EXPECT_EQ(IsSpawnServer(&content), 0);
}

HWTEST_F(AppSpawnManagerTest, IsSpawnServer_03, TestSize.Level0)
{
    // 测试 servicePid 等于当前进程 PID 的情况
    content.servicePid = getpid();  // 设置为当前进程 PID
    EXPECT_EQ(IsSpawnServer(&content), 1);
}

// 测试用例
HWTEST_F(AppSpawnManagerTest, IsSpawnServer_03, TestSize.Level0)
{
    // 测试 content 为 NULL 的情况
    EXPECT_EQ(IsNWebSpawnMode(NULL), 0);
}

HWTEST_F(AppSpawnManagerTest, IsSpawnServer_03, TestSize.Level0)
{
    // 测试 mode 为 MODE_FOR_NWEB_SPAWN 的情况
    content.content.mode = MODE_FOR_NWEB_SPAWN;
    EXPECT_EQ(IsNWebSpawnMode(&content), 1);
}

HWTEST_F(AppSpawnManagerTest, IsSpawnServer_03, TestSize.Level0)
{
    // 测试 mode 为 MODE_FOR_NWEB_COLD_RUN 的情况
    content.content.mode = MODE_FOR_NWEB_COLD_RUN;
    EXPECT_EQ(IsNWebSpawnMode(&content), 1);
}

HWTEST_F(AppSpawnManagerTest, IsSpawnServer_03, TestSize.Level0)
{
    // 测试 mode 为其他值的情况
    content.content.mode = MODE_FOR_APP_SPAWN;
    EXPECT_EQ(IsNWebSpawnMode(&content), 0);
}

// 测试用例
HWTEST_F(AppSpawnManagerTest, IsColdRunMode_01, TestSize.Level0)
{
    // 测试 content 为 NULL 的情况
    EXPECT_EQ(IsColdRunMode(NULL), 0);
}

HWTEST_F(AppSpawnManagerTest, IsColdRunMode_02, TestSize.Level0)
{
    // 测试 mode 为 MODE_FOR_APP_COLD_RUN 的情况
    content.content.mode = MODE_FOR_APP_COLD_RUN;
    EXPECT_EQ(IsColdRunMode(&content), 1);
}

HWTEST_F(AppSpawnManagerTest, IsColdRunMode_03, TestSize.Level0)
{
    // 测试 mode 为 MODE_FOR_NWEB_COLD_RUN 的情况
    content.content.mode = MODE_FOR_NWEB_COLD_RUN;
    EXPECT_EQ(IsColdRunMode(&content), 1);
}

HWTEST_F(AppSpawnManagerTest, IsColdRunMode_04, TestSize.Level0)
{
    // 测试 mode 为 MODE_FOR_APP_SPAWN 的情况
    content.content.mode = MODE_FOR_APP_SPAWN;
    EXPECT_EQ(IsColdRunMode(&content), 0);
}

HWTEST_F(AppSpawnManagerTest, IsColdRunMode_05, TestSize.Level0)
{
    // 测试 mode 为 MODE_FOR_NWEB_SPAWN 的情况
    content.content.mode = MODE_FOR_NWEB_SPAWN;
    EXPECT_EQ(IsColdRunMode(&content), 0);
}

HWTEST_F(AppSpawnManagerTest, IsColdRunMode_06, TestSize.Level0)
{
    // 测试 mode 为 MODE_INVALID 的情况
    content.content.mode = MODE_INVALID;
    EXPECT_EQ(IsColdRunMode(&content), 0);
}

// 测试用例
HWTEST_F(AppSpawnManagerTest, IsDeveloperModeOn_01, TestSize.Level0)
{
    // 测试 property 为 NULL 的情况
    EXPECT_EQ(IsDeveloperModeOn(NULL), 0);
}

HWTEST_F(AppSpawnManagerTest, IsDeveloperModeOn_02, TestSize.Level0)
{
    // 测试开发者模式开启的情况
    property.client.flags = APP_DEVELOPER_MODE; // 只设置开发者模式标志
    EXPECT_EQ(IsDeveloperModeOn(&property), 1);
}

HWTEST_F(AppSpawnManagerTest, IsDeveloperModeOn_03, TestSize.Level0)
{
    // 测试开发者模式关闭的情况
    property.client.flags = 0; // 不设置任何标志
    EXPECT_EQ(IsDeveloperModeOn(&property), 0);
}

HWTEST_F(AppSpawnManagerTest, IsDeveloperModeOn_04, TestSize.Level0)
{
    // 测试其他标志设置但未启用开发者模式
    property.client.flags = 0x01; // 只设置其他标志
    EXPECT_EQ(IsDeveloperModeOn(&property), 0);
}

HWTEST_F(AppSpawnManagerTest, IsDeveloperModeOn_05, TestSize.Level0)
{
    // 测试开发者模式与其他标志同时设置
    property.client.flags = APP_DEVELOPER_MODE | 0x01; // 同时设置开发者模式和其他标志
    EXPECT_EQ(IsDeveloperModeOn(&property), 1);
}

// 测试用例
HWTEST_F(AppSpawnManagerTest, IsJitFortModeOn_01, TestSize.Level0)
{
    // 测试 property 为 NULL 的情况
    EXPECT_EQ(IsJitFortModeOn(NULL), 0);
}

HWTEST_F(AppSpawnManagerTest, IsJitFortModeOn_02, TestSize.Level0)
{
    // 测试 JIT Fort 模式开启的情况
    property.client.flags = APP_JITFORT_MODE; // 只设置 JIT Fort 模式标志
    EXPECT_EQ(IsJitFortModeOn(&property), 1);
}

HWTEST_F(AppSpawnManagerTest, IsJitFortModeOn_03, TestSize.Level0)
{
    // 测试 JIT Fort 模式关闭的情况
    property.client.flags = 0; // 不设置任何标志
    EXPECT_EQ(IsJitFortModeOn(&property), 0);
}

HWTEST_F(AppSpawnManagerTest, IsJitFortModeOn_04, TestSize.Level0)
{
    // 测试其他标志设置但未启用 JIT Fort 模式
    property.client.flags = 0x01; // 只设置其他标志
    EXPECT_EQ(IsJitFortModeOn(&property), 0);
}

HWTEST_F(AppSpawnManagerTest, IsJitFortModeOn_05, TestSize.Level0)
{
    // 测试 JIT Fort 模式与其他标志同时设置
    property.client.flags = APP_JITFORT_MODE | 0x01; // 同时设置 JIT Fort 模式和其他标志
    EXPECT_EQ(IsJitFortModeOn(&property), 1);
}

// 测试用例
HWTEST_F(AppSpawnManagerTest, GetAppSpawnMsgType_01, TestSize.Level0)
{
    // 测试 appProperty 为 NULL 的情况
    EXPECT_EQ(GetAppSpawnMsgType(NULL), MAX_TYPE_INVALID);
}

HWTEST_F(AppSpawnManagerTest, GetAppSpawnMsgType_02, TestSize.Level0)
{
    // 测试 appProperty->message 为 NULL 的情况
    property.message = NULL;
    EXPECT_EQ(GetAppSpawnMsgType(&property), MAX_TYPE_INVALID);
}

HWTEST_F(AppSpawnManagerTest, GetAppSpawnMsgType_03, TestSize.Level0)
{
    // 测试 appProperty->message 有效的情况
    message.msgHeader.msgType = 42; // 设置有效的消息类型
    property.message = &message; // 将消息指针指向有效消息
    EXPECT_EQ(GetAppSpawnMsgType(&property), 42);
}

HWTEST_F(AppSpawnManagerTest, GetAppSpawnMsgType_04, TestSize.Level0)
{
    // 测试 appProperty->message 有效且 msgType 为负值的情况
    message.msgHeader.msgType = -5; // 设置负值的消息类型
    property.message = &message;
    EXPECT_EQ(GetAppSpawnMsgType(&property), -5);
}

// 测试用例
HWTEST_F(AppSpawnManagerTest, GetProcessName_01, TestSize.Level0)
{
    // 测试 property 为 NULL 的情况
    EXPECT_EQ(GetProcessName(NULL), nullptr);
}

HWTEST_F(AppSpawnManagerTest, GetProcessName_02, TestSize.Level0)
{
    // 测试 property->message 为 NULL 的情况
    property.message = NULL;
    EXPECT_EQ(GetProcessName(&property), nullptr);
}

HWTEST_F(AppSpawnManagerTest, GetProcessName_03, TestSize.Level0)
{
    // 测试 property->message 有效且 processName 有效的情况
    const char *expectedName = "TestProcess";
    message.msgHeader.processName = expectedName; // 设置有效的进程名称
    property.message = &message; // 将消息指针指向有效消息
    EXPECT_EQ(GetProcessName(&property), expectedName);
}

HWTEST_F(AppSpawnManagerTest, GetProcessName_04, TestSize.Level0)
{
    // 测试 processName 为 NULL 的情况
    message.msgHeader.processName = NULL; // 设置 processName 为 NULL
    property.message = &message;
    EXPECT_EQ(GetProcessName(&property), nullptr);
}
}  // namespace OHOS
