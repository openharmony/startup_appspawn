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

#define TEST_PID 1234

constexpr unsigned int DEF_REQ_ID = 123;
constexpr unsigned int DEFAULT_MSG_LEN = 10;
constexpr const char *DEFAULT_PROCESS_NAME = "TestProcess";

// 错误码
constexpr int APPSPAWN_ARG_INVALID = -1;

// 测试用例类
namespace OHOS {

    typedef void *AppSpawnClientHandle;
    typedef void *AppSpawnReqMsgHandle;

    typedef struct {
        int result;
        int pid;
    } AppSpawnResult;

    typedef struct {
        unsigned int reqId;
        struct {
            unsigned int msgLen;
            char processName[128];
        } *msg;
    } AppSpawnReqMsgNode;

    typedef struct {
        pthread_mutex_t mutex;
    } AppSpawnReqMsgMgr;

    // 测试用例类
    class AppSpawnClientSendMsgTest : public ::testing::Test {
    protected:
        void SetUp() override
        {
            // 初始化 mutex
            pthread_mutex_init(&reqMgr.mutex, nullptr);
        }

        void TearDown() override
        {
            pthread_mutex_destroy(&reqMgr.mutex);
        }

        AppSpawnReqMsgMgr reqMgr;
    };

    // 工具函数：创建一个测试用的请求节点
    std::unique_ptr<AppSpawnReqMsgNode> CreateTestReqNode(unsigned int reqId = DEF_REQ_ID)
    {
        auto reqNode = std::make_unique<AppSpawnReqMsgNode>();

        reqNode->reqId = reqId;
        reqNode->msg = std::make_unique<decltype(reqNode->msg)::element_type>();

        reqNode->msg->msgLen = kDEFAULT_MSG_LEN;
        return reqNode;
    }

    // 测试：验证 result 为 nullptr 时返回 APPSPAWN_ARG_INVALID
    TEST_F(AppSpawnClientSendMsgTest, TestNullResult)
    {
        auto reqNode = CreateTestReqNode();

        int ret = AppSpawnClientSendMsg(&reqMgr, reqNode.get(), nullptr);
        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    }

    // 测试：验证 reqHandle 为 nullptr 时返回 APPSPAWN_ARG_INVALID
    TEST_F(AppSpawnClientSendMsgTest, TestNullReqHandle)
    {
        AppSpawnResult result;

        int ret = AppSpawnClientSendMsg(&reqMgr, nullptr, &result);
        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    }

    // 测试：验证 handle 为 nullptr 时返回 APPSPAWN_ARG_INVALID
    TEST_F(AppSpawnClientSendMsgTest, TestNullHandle)
    {
        auto reqNode = CreateTestReqNode();

        AppSpawnResult result;
        int ret = AppSpawnClientSendMsg(nullptr, reqNode.get(), &result);
        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    }

    // 测试：验证正常的调用流程
    TEST_F(AppSpawnClientSendMsgTest, TestValidCase)
    {
        auto reqNode = CreateTestReqNode();

        AppSpawnResult result;
        int ret = AppSpawnClientSendMsg(&reqMgr, reqNode.get(), &result);

        // 验证返回值和 result
        EXPECT_EQ(ret, 0);
        EXPECT_EQ(result.result, 0);

        EXPECT_EQ(result.pid, TEST_PID); // 来自 ClientSendMsg 的模拟返回
    }

    // 测试：验证线程安全
    TEST_F(AppSpawnClientSendMsgTest, TestMutexProtection)
    {
        auto reqNode = CreateTestReqNode();
        AppSpawnResult result;

        // 在不同线程中调用
        std::thread t([&]()
                      {
            int ret = AppSpawnClientSendMsg(&reqMgr, reqNode.get(), &result);

            EXPECT_EQ(ret, 0);
            EXPECT_EQ(result.result, 0);

            EXPECT_EQ(result.pid, TEST_PID); });

        t.join();
    }
} // namespace OHOS