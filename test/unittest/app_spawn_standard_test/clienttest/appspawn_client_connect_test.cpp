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

#define SOCKET_NAME "loopserver"

// 假设 CreateClientSocket 函数已经定义
namespace OHOS {
    class CreateClientSocketTest : public ::testing::Test {
    protected:
        const char *socketName = SOCKET_NAME;
        const char *socketPath;
        int serverFd = -1;
        int clientFd = -1;

        void SetUp() override
        {
            // 准备 Unix 域套接字文件路径
            socketPath = std::string(_SOCKET_DIR) + socketName;

            // 创建并启动一个模拟的服务器套接字
            serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
            ASSERT_GE(serverFd, 0) << "Failed to create server socket";

            struct sockaddr_un serverAddr;
            serverAddr.sun_family = AF_UNIX;

            // 删除旧的套接字文件
            unlink(socketPath);

            int ret = bind(serverFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
            ASSERT_EQ(ret, 0) << "Failed to bind server socket";

            ret = listen(serverFd, 1);
            ASSERT_EQ(ret, 0) << "Failed to listen on server socket";

            // 创建客户端套接字
            clientFd = socket(AF_UNIX, SOCK_STREAM, 0);
            ASSERT_GE(clientFd, 0) << "Failed to create client socket";
        }

        void TearDown() override
        {
            // 清理资源
            if (clientFd >= 0) {
                close(clientFd);
                clientFd = -1;
            }

            if (serverFd >= 0) {
                close(serverFd);
                serverFd = -1;
            }

            unlink(socketPath); // 删除 Unix 域套接字文件
            socketPath = nullptr;
        }
    };

    // 测试套接字成功创建
    TEST_F(CreateClientSocketTest, TestCreateSocketSuccess)
    {
        uint32_t timeout = 1; // 设置一个合理的超时值
        int socketFd = CreateClientSocket(timeout);

        // 验证是否创建成功
        ASSERT_GE(socketFd, 0) << "Failed to create client socket";

        // 连接到服务器
        struct sockaddr_un clientAddr;
        clientAddr.sun_family = AF_UNIX;

        int ret = connect(socketFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        ASSERT_EQ(ret, 0) << "Failed to connect to server";

        // 关闭套接字
        CloseClientSocket(socketFd);
    }

    // 测试套接字创建失败
    TEST_F(CreateClientSocketTest, TestCreateSocketFailure)
    {
        uint32_t timeout = 1;

        // 模拟套接字创建失败
        // 比如，通过删除目录，来模拟无法创建套接字
        unlink(socketPath); // 删除存在的套接字文件
        int socketFd = CreateClientSocket(timeout);

        // 验证套接字创建失败，返回 -1
        ASSERT_EQ(socketFd, -1) << "Socket creation should have failed";

        // 确保错误信息打印（可以通过检查日志输出）
    }

    // 测试套接字连接失败
    TEST_F(CreateClientSocketTest, TestConnectSocketFailure)
    {
        uint32_t timeout = 1;

        // 在未启动服务器的情况下，尝试连接
        unlink(socketPath); // 确保没有现有的套接字文件
        int socketFd = CreateClientSocket(timeout);

        // 验证套接字创建成功，但连接失败
        ASSERT_GE(socketFd, 0) << "Socket creation should have succeeded";

        // 在没有服务器的情况下，连接应该失败
        struct sockaddr_un clientAddr;
        clientAddr.sun_family = AF_UNIX;

        int ret = connect(socketFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
        ASSERT_EQ(ret, -1) << "Expected connection failure";
        ASSERT_EQ(errno, ECONNREFUSED) << "Expected ECONNREFUSED error";

        // 关闭套接字
        CloseClientSocket(socketFd);
    }

    // 测试超时设置
    TEST_F(CreateClientSocketTest, TestSetSocketTimeouts)
    {
        uint32_t timeout = 1; // 设置超时时间
        int socketFd = CreateClientSocket(timeout);
        ASSERT_GE(socketFd, 0) << "Failed to create socket";

        // 通过查看设置的超时来验证是否生效
        struct timeval recvTimeout;
        socklen_t optlen = sizeof(recvTimeout);

        int ret = getsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &recvTimeout, &optlen);
        ASSERT_EQ(ret, 0) << "Failed to get SO_RCVTIMEO";
        ASSERT_EQ(recvTimeout.tv_sec, 1) << "Expected receive timeout to be 1 second";

        struct timeval sendTimeout;
        ret = getsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, &sendTimeout, &optlen);

        ASSERT_EQ(ret, 0) << "Failed to get SO_SNDTIMEO";
        ASSERT_EQ(sendTimeout.tv_sec, 1) << "Expected send timeout to be 1 second";

        // 关闭套接字
        CloseClientSocket(socketFd);
    }
}