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

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "appspawn_server.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

// 全局变量用于验证回调函数是否被调用
bool g_callCalled = false;
int g_result = -1;
AppSpawnContent *CALL_CONTENT = nullptr;
AppSpawnClient *CALL_CLIENT = nullptr;

// 测试用的回调函数
void TestCallback(AppSpawnContent *content, AppSpawnClient *client, int result)
{
    g_callCalled = true;    // 设置标志
    g_result = result;      // 保存结果
    CALL_CONTENT = content; // 保存内容
    CALL_CLIENT = client;   // 保存客户端
}
using namespace testing;
using namespace testing::ext;

namespace OHOS {
    class NotifyResToParentTest : public testing::Test {
    protected:
        void SetUp() override
        {
            // 在每个测试前初始化
            content = new AppSpawnContent();
            client = new AppSpawnClient();
            g_callCalled = false;                 // 每个测试开始时重置标志
            content->notifyResToParent = nullptr; // 默认没有回调
        }

        void TearDown() override
        {
            // 清理资源
            delete content;
            content = nullptr;
            delete client;
            client = nullptr;
        }

        AppSpawnContent *content;
        AppSpawnClient *client;
    };

    // 测试：notifyResToParent非空时，函数被正确调用
    HWTEST_F(NotifyResToParentTest, TestNotifyResToParent_001, TestSize.Level0)
    {
        content->notifyResToParent = TestCallback; // 设置自定义回调

        // 调用NotifyResToParent
        NotifyResToParent(content, client, 42);

        // 验证回调是否被调用，并且结果是否正确
        EXPECT_TRUE(g_callCalled);
        EXPECT_EQ(g_result, 42);
        EXPECT_EQ(CALL_CONTENT, content);
        EXPECT_EQ(CALL_CLIENT, client);
    }

    // 测试：notifyResToParent为空时，函数不被调用
    HWTEST_F(NotifyResToParentTest, TestNotifyResToParent_002, TestSize.Level0)
    {
        content->notifyResToParent = nullptr; // 设置回调为NULL

        // 调用NotifyResToParent
        NotifyResToParent(content, client, 42);

        // 验证回调没有被调用
        EXPECT_FALSE(g_callCalled);
    }

    // 测试：传递不同的result值，回调函数被正确调用
    HWTEST_F(NotifyResToParentTest, TestNotifyResToParent_003, TestSize.Level0)
    {
        content->notifyResToParent = TestCallback; // 设置自定义回调

        // 测试不同的result值
        NotifyResToParent(content, client, -1);
        EXPECT_TRUE(g_callCalled);
        EXPECT_EQ(g_result, -1);

        // 重置回调标志
        g_callCalled = false;

        NotifyResToParent(content, client, 0);
        EXPECT_TRUE(g_callCalled);
        EXPECT_EQ(g_result, 0);

        // 重置回调标志
        g_callCalled = false;

        NotifyResToParent(content, client, 100);
        EXPECT_TRUE(g_callCalled);
        EXPECT_EQ(g_result, 100);
    }
} // namespace OHOS
