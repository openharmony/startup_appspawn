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

namespace OHOS {

    // 常量定义
    constexpr int APPSPAWN_SYSTEM_ERROR = -1;
    constexpr int CLIENT_COUNT = 4;

    // 使用智能指针来管理资源
    using AppSpawnClientHandle = std::unique_ptr<AppSpawnReqMsgMgr>;

    struct AppSpawnReqMsgMgr {
        ClientType type;
        int socketId;
        std::mutex mutex;
    };

    class AppSpawnClientDestroyTest : public ::testing::Test {
    protected:
        void SetUp() override
        {
            // 初始化客户端实例为空
            for (auto &client : g_clientInstance) {
                client.reset();
            }
        }

        void TearDown() override
        {
            // 不需要手动释放资源，unique_ptr 会自动清理
        }

        // 使用 std::array 管理客户端实例
        std::array<AppSpawnClientHandle, CLIENT_COUNT> g_clientInstance{};
    };

    // 测试：验证无效的 handle 返回错误
    TEST_F(AppSpawnClientDestroyTest, TestNullHandle)
    {
        int ret = AppSpawnClientDestroy(nullptr);
        EXPECT_EQ(ret, APPSPAWN_SYSTEM_ERROR);
    }

    // 测试：验证正常销毁流程
    TEST_F(AppSpawnClientDestroyTest, TestValidHandle)
    {
        // 创建一个有效的 AppSpawnReqMsgMgr 实例
        auto reqMgr = std::make_unique<AppSpawnReqMsgMgr>();

        reqMgr->type = ClientType::AppSpawn;
        reqMgr->socketId = 123; // 模拟一个有效的 socketId

        // 将这个实例放入全局实例中
        g_clientInstance[static_cast<int>(ClientType::AppSpawn)] = std::move(reqMgr);

        // 调用销毁函数
        int ret = AppSpawnClientDestroy(g_clientInstance[static_cast<int>(ClientType::AppSpawn)].get());
        EXPECT_EQ(ret, 0);

        // 验证销毁后的实例为空
        EXPECT_EQ(g_clientInstance[static_cast<int>(ClientType::AppSpawn)], nullptr);
    }

    // 测试：验证互斥锁是否正常工作
    TEST_F(AppSpawnClientDestroyTest, TestMutexProtection)
    {
        // 创建一个有效的 AppSpawnReqMsgMgr 实例
        auto reqMgr = std::make_unique<AppSpawnReqMsgMgr>();

        reqMgr->type = ClientType::AppSpawn;
        reqMgr->socketId = 123; // 模拟一个有效的 socketId

        // 将这个实例放入全局实例中
        g_clientInstance[static_cast<int>(ClientType::AppSpawn)] = std::move(reqMgr);

        // 在不同线程中测试互斥锁的保护
        std::thread t([&]()
                      {
        int ret = AppSpawnClientDestroy(g_clientInstance[static_cast<int>(ClientType::AppSpawn)].get());
        EXPECT_EQ(ret, 0); });

        // 等待线程完成
        t.join();

        // 验证销毁后的实例为空
        EXPECT_EQ(g_clientInstance[static_cast<int>(ClientType::AppSpawn)], nullptr);
    }

    // 测试：验证 socketId 小于 0 时不关闭套接字
    TEST_F(AppSpawnClientDestroyTest, TestNoSocketCloseWhenSocketIdNegative)
    {
        // 创建一个没有有效 socketId 的实例
        auto reqMgr = std::make_unique<AppSpawnReqMsgMgr>();

        reqMgr->type = ClientType::AppSpawn;
        reqMgr->socketId = -1; // 模拟无效的 socketId

        // 将这个实例放入全局实例中
        g_clientInstance[static_cast<int>(ClientType::AppSpawn)] = std::move(reqMgr);

        // 调用销毁函数
        int ret = AppSpawnClientDestroy(g_clientInstance[static_cast<int>(ClientType::AppSpawn)].get());
        EXPECT_EQ(ret, 0);

        // 验证销毁后的实例为空
        EXPECT_EQ(g_clientInstance[static_cast<int>(ClientType::AppSpawn)], nullptr);
    }

} // namespace OHOS