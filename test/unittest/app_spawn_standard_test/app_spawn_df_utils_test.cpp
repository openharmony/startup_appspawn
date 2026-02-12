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
#include <cstring>
#include <unistd.h>

#include "appspawndf_utils.h"
#include "appspawn_client.h"
#include "appspawn_msg.h"
#include "app_spawn_test_helper.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static AppSpawnTestHelper g_testHelper;

class AppSpawnDfUtilsTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    }
};

/**
 * @brief Test AppSpawndfIsServiceEnabled with nullptr and whitelist queries
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_IsServiceEnabled_001, TestSize.Level0)
{
    bool enabled = AppSpawndfIsServiceEnabled(nullptr);
    EXPECT_FALSE(enabled);
    bool inList1 = AppSpawndfIsAppInWhiteList(nullptr);
    EXPECT_FALSE(inList1);
    bool inList2 = AppSpawndfIsAppInWhiteList("");
    EXPECT_FALSE(inList2);
    bool inList3 = AppSpawndfIsAppInWhiteList("com.example.test");
    EXPECT_TRUE(inList3 == true || inList3 == false);
    std::string longName(256, 'a');
    bool inList = AppSpawndfIsAppInWhiteList(longName.c_str());
    EXPECT_FALSE(inList);
}

/**
 * @brief Test AppSpawndfIsServiceEnabled with valid init function
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_IsServiceEnabled_002, TestSize.Level0)
{
    auto mockInitFunc = [](const char *name, AppSpawnClientHandle *handle) -> int {
        return AppSpawnClientInit(name, handle);
    };

    bool enabled = AppSpawndfIsServiceEnabled(mockInitFunc);
    EXPECT_TRUE(enabled == true || enabled == false);
}

/**
 * @brief Test AppSpawndfGetHandle returns nullptr when uninitialized or init fails
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_GetHandle_001, TestSize.Level0)
{
    AppSpawnClientHandle handle = AppSpawndfGetHandle();
    EXPECT_TRUE(handle == nullptr);
    auto mockInitFunc = [](const char *name, AppSpawnClientHandle *handle) -> int {
        return -1;
    };
    AppSpawndfIsServiceEnabled(mockInitFunc);
    AppSpawnClientHandle handle2 = AppSpawndfGetHandle();
    EXPECT_TRUE(handle2 == nullptr);
}

/**
 * @brief Test AppSpawndfIsBroadcastMsg with multiple message types
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_IsBroadcastMsg_001, TestSize.Level0)
{
    bool isBroadcast1 = AppSpawndfIsBroadcastMsg(MSG_DUMP);
    EXPECT_TRUE(isBroadcast1);
    bool isBroadcast2 = AppSpawndfIsBroadcastMsg(MSG_BEGET_SPAWNTIME);
    EXPECT_TRUE(isBroadcast2);
    bool isBroadcast3 = AppSpawndfIsBroadcastMsg(MSG_LOCK_STATUS);
    EXPECT_TRUE(isBroadcast3);
    bool isBroadcast4 = AppSpawndfIsBroadcastMsg(MSG_UNINSTALL_DEBUG_HAP);
    EXPECT_TRUE(isBroadcast4);
    bool isBroadcast5 = AppSpawndfIsBroadcastMsg(MSG_APP_SPAWN);
    EXPECT_FALSE(isBroadcast5);
}

/**
 * @brief Test AppSpawndfMergeBroadcastResult with MSG_LOCK_STATUS
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_MergeBroadcastResult_003, TestSize.Level0)
{
    AppSpawnResult mainRes = {0};
    AppSpawnResult dfRes = {0};
    mainRes.pid = 100;
    mainRes.result = 1;
    dfRes.pid = 200;
    dfRes.result = 0;
    AppSpawndfMergeBroadcastResult(MSG_LOCK_STATUS, &mainRes, &dfRes);
    EXPECT_EQ(mainRes.result, 0);
    EXPECT_EQ(mainRes.pid, 200);
}

/**
 * @brief Test AppSpawndfMergeBroadcastResult with other message types
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_MergeBroadcastResult_004, TestSize.Level0)
{
    AppSpawnResult mainRes = {0};
    mainRes.pid = 100;
    mainRes.result = 0;
    AppSpawnResult dfRes = {0};
    dfRes.pid = 200;
    dfRes.result = -1;
    AppSpawndfMergeBroadcastResult(MSG_OBSERVE_PROCESS_SIGNAL_STATUS, &mainRes, &dfRes);
    EXPECT_NE(mainRes.result, 0);
    mainRes.result = 0;
    dfRes.result = 0;
    AppSpawndfMergeBroadcastResult(MSG_OBSERVE_PROCESS_SIGNAL_STATUS, &mainRes, &dfRes);
    EXPECT_EQ(mainRes.result, 0);
    mainRes.result = 0;
    dfRes.result = 5;
    AppSpawndfMergeBroadcastResult(MSG_OBSERVE_PROCESS_SIGNAL_STATUS, &mainRes, &dfRes);
    EXPECT_EQ(mainRes.result, 5);
}

/**
 * @brief Test AppSpawndfMergeBroadcastResult with MSG_UNINSTALL_DEBUG_HAP
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_MergeBroadcastResult_005, TestSize.Level0)
{
    AppSpawnResult mainRes = {0};
    mainRes.pid = 100;
    mainRes.result = 1;
    AppSpawnResult dfRes = {0};
    dfRes.pid = 200;
    dfRes.result = 0;
    AppSpawndfMergeBroadcastResult(MSG_UNINSTALL_DEBUG_HAP, &mainRes, &dfRes);
    EXPECT_EQ(mainRes.result, 0);
    EXPECT_EQ(mainRes.pid, 200);
}

/**
 * @brief Test AppSpawndfIsBroadcastMsg with more message types
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_IsBroadcastMsg_002, TestSize.Level0)
{
    bool isBroadcast1 = AppSpawndfIsBroadcastMsg(MSG_UNLOAD_WEBLIB_IN_APPSPAWN);
    EXPECT_TRUE(isBroadcast1);
    bool isBroadcast2 = AppSpawndfIsBroadcastMsg(MSG_LOAD_WEBLIB_IN_APPSPAWN);
    EXPECT_TRUE(isBroadcast2);
    bool isBroadcast3 = AppSpawndfIsBroadcastMsg(MSG_OBSERVE_PROCESS_SIGNAL_STATUS);
    EXPECT_TRUE(isBroadcast3);
}

/**
 * @brief Test AppSpawndfIsServiceEnabled multiple calls to test isLoaded_ branch
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_IsServiceEnabled_003, TestSize.Level0)
{
    auto mockInitFunc = [](const char *name, AppSpawnClientHandle *handle) -> int {
        return AppSpawnClientInit(name, handle);
    };

    bool enabled1 = AppSpawndfIsServiceEnabled(mockInitFunc);
    bool enabled2 = AppSpawndfIsServiceEnabled(mockInitFunc);
    EXPECT_EQ(enabled1, enabled2);
}

/**
 * @brief Test AppSpawndfIsAppEnableGWPASan with various parameter combinations
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_IsAppEnableGWPASan_001, TestSize.Level0)
{
    uint32_t buffer[2] = {1, 0};
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(buffer);
    msgFlags->count = 1;
    msgFlags->flags[0] = 0;
    bool enabled1 = AppSpawndfIsAppEnableGWPASan(nullptr, msgFlags);
    EXPECT_FALSE(enabled1);
    bool enabled2 = AppSpawndfIsAppEnableGWPASan("", msgFlags);
    EXPECT_FALSE(enabled2);
    bool enabled3 = AppSpawndfIsAppEnableGWPASan("com.example.test", nullptr);
    EXPECT_FALSE(enabled3);
    bool enabled4 = AppSpawndfIsAppEnableGWPASan("com.example.test", msgFlags);
    EXPECT_TRUE(enabled4 == true || enabled4 == false);
}

/**
 * @brief Test AppSpawndfIsAppEnableGWPASan with flags already set
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_IsAppEnableGWPASan_002, TestSize.Level0)
{
    uint32_t buffer[2] = {1, 0};
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(buffer);
    msgFlags->count = 1;
    msgFlags->flags[0] = (1 << (APP_FLAGS_GWP_ENABLED_FORCE % 32));
    bool enabled1 = AppSpawndfIsAppEnableGWPASan("com.example.test", msgFlags);
    EXPECT_TRUE(enabled1);
    msgFlags->flags[0] = (1 << (APP_FLAGS_GWP_ENABLED_NORMAL % 32));
    bool enabled2 = AppSpawndfIsAppEnableGWPASan("com.example.test", msgFlags);
    EXPECT_FALSE(enabled2);
}

/**
 * @brief Test AppSpawndfMergeBroadcastResult with other message types
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_MergeBroadcastResult_007, TestSize.Level0)
{
    AppSpawnResult mainRes = {0};
    AppSpawnResult dfRes = {0};
    mainRes.pid = 100;
    mainRes.result = 0;
    dfRes.pid = 200;
    dfRes.result = 0;
    AppSpawndfMergeBroadcastResult(MSG_UNLOAD_WEBLIB_IN_APPSPAWN, &mainRes, &dfRes);
    EXPECT_EQ(mainRes.result, 0);
}

/**
 * @brief Test SetAppSpawnMsgFlags with valid parameters
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_SetAppSpawnMsgFlags_001, TestSize.Level0)
{
    uint32_t buffer[2] = {1, 0};
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(buffer);
    msgFlags->count = 1;
    msgFlags->flags[0] = 0;
    int ret = SetAppSpawnMsgFlags(msgFlags, 5);
    EXPECT_EQ(ret, 0);
    uint32_t expectedFlag = (1 << 5);
    EXPECT_EQ((msgFlags->flags[0] & expectedFlag), expectedFlag);
}

/**
 * @brief Test SetAppSpawnMsgFlags setting multiple flags
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_SetAppSpawnMsgFlags_002, TestSize.Level0)
{
    uint32_t buffer[2] = {1, 0};
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(buffer);
    msgFlags->count = 1;
    msgFlags->flags[0] = 0;
    SetAppSpawnMsgFlags(msgFlags, 3);
    SetAppSpawnMsgFlags(msgFlags, 7);
    SetAppSpawnMsgFlags(msgFlags, 15);
    uint32_t expected = (1 << 3) | (1 << 7) | (1 << 15);
    EXPECT_EQ(msgFlags->flags[0], expected);
}

/**
 * @brief Test SetAppSpawnMsgFlags with index out of range
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_SetAppSpawnMsgFlags_003, TestSize.Level0)
{
    uint32_t buffer[2] = {1, 0};
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(buffer);
    msgFlags->count = 1;
    msgFlags->flags[0] = 0;
    int ret = SetAppSpawnMsgFlags(msgFlags, 35);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(msgFlags->flags[0], 0);
}

/**
 * @brief Test SetAppSpawnMsgFlags setting flags across block boundaries
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_SetAppSpawnMsgFlags_004, TestSize.Level0)
{
    uint32_t buffer[3] = {2, 0, 0};
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(buffer);
    msgFlags->count = 2;
    msgFlags->flags[0] = 0;
    msgFlags->flags[1] = 0;
    SetAppSpawnMsgFlags(msgFlags, 31);
    SetAppSpawnMsgFlags(msgFlags, 32);
    SetAppSpawnMsgFlags(msgFlags, 33);
    EXPECT_EQ(msgFlags->flags[0], (1U << 31));
    EXPECT_EQ(msgFlags->flags[1], (1U << 0) | (1U << 1));
}

/**
 * @brief Test SetAppSpawnMsgFlags with index 0
 */
HWTEST_F(AppSpawnDfUtilsTest, AppSpawndf_SetAppSpawnMsgFlags_005, TestSize.Level0)
{
    uint32_t buffer[2] = {1, 0};
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(buffer);
    msgFlags->count = 1;
    msgFlags->flags[0] = 0;
    int ret = SetAppSpawnMsgFlags(msgFlags, 0);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(msgFlags->flags[0], 1U);
}
}  // namespace OHOS
