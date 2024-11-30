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
#define SOCKET_PORT 8080
using namespace testing;
using namespace testing::ext;

namespace OHOS {
    class CloseClientSocketTest : public ::testing::Test {
    protected:
        int socketId;

        void SetUp() override
        {
            // 创建一个实际的 TCP socket
            socketId = socket(AF_INET, SOCK_STREAM, 0);
            ASSERT_GE(socketId, 0) << "Failed to create socket"; // 确保 socket 创建成功

            // 设置一个连接目标 (假设使用本地地址)
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(SOCKET_PORT);
            addr.sin_addr.s_addr = INADDR_LOOPBACK;

            int result = connect(socketId, (struct sockaddr *)&addr, sizeof(addr));
            ASSERT_EQ(result, 0) << "Failed to connect socket";
        }

        void TearDown() override
        {
            if (socketId >= 0) {
                close(socketId);
                socketId = -1;
            }
        }
    };

    // 测试正常关闭的情况
    TEST_F(CloseClientSocketTest, TestCloseValidSocket)
    {
        int socketIdValid = socketId;                      // 使用有效的 socketId
        EXPECT_NO_THROW(CloseClientSocket(socketIdValid)); // 调用 CloseClientSocket，应该正常关闭

        // 检查 socket 是否已经关闭
        char buffer[10];
        int result = recv(socketIdValid, buffer, sizeof(buffer), MSG_DONTWAIT);

        EXPECT_EQ(result, -1);   // 如果套接字关闭，应该返回 -1，表示连接被重置或关闭
        EXPECT_EQ(errno, EBADF); // 文件描述符无效，errno 应为 EBADF
    }

    // 测试无效 socketId
    TEST_F(CloseClientSocketTest, TestCloseInvalidSocket)
    {
        int socketIdInvalid = -1;                            // 无效的 socketId
        EXPECT_NO_THROW(CloseClientSocket(socketIdInvalid)); // 对无效 socketId 调用不应该抛出异常
    }

    // 测试关闭已关闭的套接字
    TEST_F(CloseClientSocketTest, TestCloseAlreadyClosedSocket)
    {
        int socketIdValid = socketId;                      // 使用有效的 socketId
        CloseClientSocket(socketIdValid);                  // 首先关闭它

        EXPECT_NO_THROW(CloseClientSocket(socketIdValid)); // 再次调用不应该出错
    }

    // 测试关闭负值 socketId
    TEST_F(CloseClientSocketTest, TestCloseNegativeSocket)
    {
        int socketIdNegative = -1;                            // 使用负值 socketId
        EXPECT_NO_THROW(CloseClientSocket(socketIdNegative)); // 不会发生错误
    }
}