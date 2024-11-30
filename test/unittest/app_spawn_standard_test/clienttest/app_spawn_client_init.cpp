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

// 常量定义
namespace OHOS {
    const std::string CJAPPSPAWN_SERVICE_NAME = "CJAppSpawn";
    const std::string NWEBSPAWN_SERVICE_NAME = "NWebSpawn";
    const std::string NATIVESPAWN_SERVICE_NAME = "NativeSpawn";
    const std::string NWEBSPAWN_SOCKET_NAME = "SomeNWebSocketName";
}

// 测试用例类
class AppSpawnClientInitTest : public ::testing::Test {
protected:
    void SetUp() {}
};

// 测试：验证参数校验
TEST_F(AppSpawnClientInitTest, TestInvalidArguments)
{
    AppSpawnClientHandle handle = nullptr;

    // 测试 serviceName 为 nullptr
    int ret = AppSpawnClientInit(nullptr, &handle);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    // 测试 handle 为 nullptr
    ret = AppSpawnClientInit(OHOS::CJAPPSPAWN_SERVICE_NAME.c_str(), nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

// 测试：验证正确的客户端类型选择
TEST_F(AppSpawnClientInitTest, TestClientTypeSelection)
{
    AppSpawnClientHandle handle = nullptr;

    // 测试 CJAPPSPAWN_SERVER_NAME
    int ret = AppSpawnClientInit(OHOS::CJAPPSPAWN_SERVICE_NAME.c_str(), &handle);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(handle, g_clientInstance[CLIENT_FOR_CJAPPSPAWN]);

    // 测试 NWEBSPAWN_SERVER_NAME
    ret = AppSpawnClientInit(OHOS::NWEBSPAWN_SERVICE_NAME.c_str(), &handle);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(handle, g_clientInstance[CLIENT_FOR_NWEBSPAWN]);

    // 测试 NATIVESPAWN_SERVER_NAME
    ret = AppSpawnClientInit(OHOS::NATIVESPAWN_SERVICE_NAME.c_str(), &handle);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(handle, g_clientInstance[CLIENT_FOR_NATIVESPAWN]);

    // 测试包含 NWEBSPAWN_SOCKET_NAME 的字符串
    ret = AppSpawnClientInit(OHOS::NWEBSPAWN_SOCKET_NAME.c_str(), &handle);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(handle, g_clientInstance[CLIENT_FOR_NWEBSPAWN]);
}

// 测试：验证客户端实例初始化成功
TEST_F(AppSpawnClientInitTest, TestClientInstanceInitSuccess)
{
    AppSpawnClientHandle handle = nullptr;

    // 测试 CJAPPSPAWN_SERVER_NAME
    int ret = AppSpawnClientInit(OHOS::CJAPPSPAWN_SERVICE_NAME.c_str(), &handle);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(handle, g_clientInstance[CLIENT_FOR_CJAPPSPAWN]);
}

// 测试：验证客户端实例初始化失败
TEST_F(AppSpawnClientInitTest, TestClientInstanceInitFailure)
{
    AppSpawnClientHandle handle = nullptr;

    // 模拟 InitClientInstance 失败
    int ret = AppSpawnClientInit("InvalidService", &handle);

    EXPECT_EQ(ret, APPSPAWN_SYSTEM_ERROR);
    EXPECT_EQ(handle, nullptr); // 由于初始化失败，handle 应该为 null
}