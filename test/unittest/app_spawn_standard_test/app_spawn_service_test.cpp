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
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>

#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "json_utils.h"
#include "parameter.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static OHOS::AppSpawnTestServer *g_testServer = nullptr;
class AppSpawnServiceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief 正常消息发送和接收，完整应用孵化过程
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_001, TestSize.Level0)
{
    g_testServer = new OHOS::AppSpawnTestServer("appspawn -mode appspawn");
    ASSERT_EQ(g_testServer != nullptr, 1);
    g_testServer->Start(nullptr);

    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        APPSPAWN_LOGV("App_Spawn_001 start");
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NO_SANDBOX);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("App_Spawn_001 recv result %{public}d  %{public}d", result.result, result.pid);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
        if (ret == 0 && result.pid > 0) {
            APPSPAWN_LOGV("App_Spawn_001 Kill pid %{public}d ", result.pid);
            kill(result.pid, SIGKILL);
        }
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 模拟测试，孵化进程退出后，MSG_GET_RENDER_TERMINATION_STATUS
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_002, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        APPSPAWN_LOGV("App_Spawn_002 start");
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("App_Spawn_002 recv result %{public}d  %{public}d", result.result, result.pid);
        if (ret != 0 || result.pid == 0) {
            ret = -1;
            break;
        }
        // stop child and termination
        APPSPAWN_LOGI("App_Spawn_002 Kill pid %{public}d ", result.pid);
        kill(result.pid, SIGKILL);
        // MSG_GET_RENDER_TERMINATION_STATUS
        ret = AppSpawnTerminateMsgCreate(result.pid, &reqHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create termination msg %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("Send MSG_GET_RENDER_TERMINATION_STATUS %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 模拟测试，MSG_GET_RENDER_TERMINATION_STATUS 关闭孵化进程
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_003, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("App_Spawn_003 recv result %{public}d  %{public}d", result.result, result.pid);
        if (ret != 0 || result.pid == 0) {
            ret = -1;
            break;
        }
        // MSG_GET_RENDER_TERMINATION_STATUS
        ret = AppSpawnTerminateMsgCreate(result.pid, &reqHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create termination msg %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("Send MSG_GET_RENDER_TERMINATION_STATUS %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief dump 消息
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_004, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_DUMP, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief MSG_SPAWN_NATIVE_PROCESS 正常消息发送和接收，完整应用孵化过程
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_005, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
        if (ret == 0 && result.pid > 0) {
            APPSPAWN_LOGI("App_Spawn_005 Kill pid %{public}d ", result.pid);
            kill(result.pid, SIGKILL);
        }
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 多线程发送消息
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_006, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        auto sendMsg = [](AppSpawnClientHandle clientHandle) {
            AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);

            AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
            AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
            AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
            AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);

            AppSpawnResult result = {};
            int ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
            APPSPAWN_CHECK(ret == 0, return, "Failed to send msg %{public}d", ret);
            if (ret == 0 && result.pid > 0) {
                printf("App_Spawn_006 Kill pid %d \n", result.pid);
                kill(result.pid, SIGKILL);
            }
            ASSERT_EQ(ret, 0);
        };
        std::thread thread1(sendMsg, clientHandle);
        std::thread thread2(sendMsg, clientHandle);
        std::thread thread3(sendMsg, clientHandle);
        std::thread thread4(sendMsg, clientHandle);
        std::thread thread5(sendMsg, clientHandle);

        thread1.join();
        thread2.join();
        thread3.join();
        thread4.join();
        thread5.join();
    } while (0);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST(AppSpawnServiceTest, App_Spawn_Msg_001, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    // 没有tlv的消息，返回错误
    do {
        socketId = g_testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer(sizeof(AppSpawnResponseMsg));
        uint32_t msgLen = 0;
        ret = g_testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());

        int len = write(socketId, buffer.data(), msgLen);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        // recv
        APPSPAWN_LOGV("Start recv ... ");
        len = read(socketId, buffer.data(), buffer.size());
        APPSPAWN_CHECK(len >= static_cast<int>(sizeof(AppSpawnResponseMsg)), ret = -1;
            break, "Failed to recv msg errno: %{public}d", errno);
        AppSpawnResponseMsg *respMsg = reinterpret_cast<AppSpawnResponseMsg *>(buffer.data());
        APPSPAWN_LOGV("Recv msg %{public}s result: %{public}d", respMsg->msgHdr.processName, respMsg->result.result);
        ret = respMsg->result.result;
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_NE(ret, 0);
}

static int RecvMsg(int socketId, uint8_t *buffer, uint32_t buffSize)
{
    ssize_t rLen = TEMP_FAILURE_RETRY(read(socketId, buffer, buffSize));
    return static_cast<int>(rLen);
}

/**
 * @brief 消息不完整，断开连接
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_Msg_002, TestSize.Level0)
{
    int ret = 0;
    int socketId = -1;
    do {
        socketId = g_testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer(sizeof(AppSpawnResponseMsg));
        uint32_t msgLen = 0;
        ret = g_testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());

        ret = -1;
        int len = write(socketId, buffer.data(), msgLen - 10);  // 10
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        // recv timeout
        len = RecvMsg(socketId, buffer.data(), buffer.size());
        APPSPAWN_CHECK(len <= 0, break, "Failed to recv msg len: %{public}d", len);
        ret = 0;
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试异常tlv
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_Msg_003, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = g_testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer(sizeof(AppSpawnResponseMsg));
        uint32_t msgLen = 0;
        ret = g_testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());

        int len = write(socketId, buffer.data(), msgLen);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        // recv
        len = RecvMsg(socketId, buffer.data(), buffer.size());
        APPSPAWN_CHECK(len >= static_cast<int>(sizeof(AppSpawnResponseMsg)), ret = -1;
            break, "Failed to recv msg %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnResponseMsg *respMsg = reinterpret_cast<AppSpawnResponseMsg *>(buffer.data());
        APPSPAWN_LOGV("Recv msg %{public}s result: %{public}d", respMsg->msgHdr.processName, respMsg->result.result);
        ret = respMsg->result.result == APPSPAWN_MSG_INVALID ? 0 : -1;
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试小包发送，随机获取发送大小，发送数据。消息20时，按33发送
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_Msg_004, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = g_testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer(1024, 0);  // 1024 1k
        uint32_t msgLen = 0;
        ret = g_testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());

        // 分片发送
        uint32_t sendStep = OHOS::AppSpawnTestHelper::GenRandom() % 70;  // 70 一次发送的字节数
        sendStep = (sendStep < 20) ? 33 : sendStep;                      // 20 33 一次发送的字节数
        APPSPAWN_LOGV("App_Spawn_Msg_004 msgLen %{public}u sendStep: %{public}u", msgLen, sendStep);
        uint32_t currIndex = 0;
        int len = 0;
        do {
            if ((currIndex + sendStep) > msgLen) {
                break;
            }
            len = write(socketId, buffer.data() + currIndex, sendStep);
            APPSPAWN_CHECK(len > 0, break, "Failed to send %{public}s", g_testServer->GetDefaultTestAppBundleName());
            usleep(2000);  // wait recv
            currIndex += sendStep;
        } while (1);
        APPSPAWN_CHECK(len > 0, break, "Failed to send %{public}s", g_testServer->GetDefaultTestAppBundleName());
        if (msgLen > currIndex) {
            len = write(socketId, buffer.data() + currIndex, msgLen - currIndex);
            APPSPAWN_CHECK(len > 0, break, "Failed to send %{public}s", g_testServer->GetDefaultTestAppBundleName());
        }

        // recv
        len = RecvMsg(socketId, buffer.data(), buffer.size());
        APPSPAWN_CHECK(len >= static_cast<int>(sizeof(AppSpawnResponseMsg)), ret = -1;
            break, "Failed to recv msg %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnResponseMsg *respMsg = reinterpret_cast<AppSpawnResponseMsg *>(buffer.data());
        APPSPAWN_LOGV("App_Spawn_Msg_004 recv msg %{public}s result: %{public}d",
            respMsg->msgHdr.processName, respMsg->result.result);
        ret = respMsg->result.result;
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试2个消息一起发送
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_Msg_005, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = g_testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer1(1024);  // 1024
        std::vector<uint8_t> buffer2(1024);  // 1024
        uint32_t msgLen1 = 0;
        uint32_t msgLen2 = 0;
        ret = g_testServer->CreateSendMsg(buffer1, MSG_APP_SPAWN, msgLen1, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        ret = g_testServer->CreateSendMsg(buffer2, MSG_APP_SPAWN, msgLen2, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());

        buffer1.insert(buffer1.begin() + msgLen1, buffer2.begin(), buffer2.end());
        int len = write(socketId, buffer1.data(), msgLen1 + msgLen2);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        // recv
        len = RecvMsg(socketId, buffer2.data(), buffer2.size());
        APPSPAWN_CHECK(len >= static_cast<int>(sizeof(AppSpawnResponseMsg)), ret = -1;
            break, "Failed to recv msg %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnResponseMsg *respMsg = reinterpret_cast<AppSpawnResponseMsg *>(buffer2.data());
        APPSPAWN_LOGV("App_Spawn_Msg_005 Recv msg %{public}s result: %{public}d",
            respMsg->msgHdr.processName, respMsg->result.result);
        ret = respMsg->result.result;
        (void)RecvMsg(socketId, buffer2.data(), buffer2.size());
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试连续2个消息，spawn和dump 消息
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_Msg_006, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = g_testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer1(2 * 1024);  // 2 * 1024
        std::vector<uint8_t> buffer2(1024);  // 1024
        uint32_t msgLen1 = 0;
        uint32_t msgLen2 = 0;
        ret = g_testServer->CreateSendMsg(buffer1, MSG_APP_SPAWN, msgLen1, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        ret = g_testServer->CreateSendMsg(buffer2, MSG_DUMP, msgLen2, {});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());

        buffer1.insert(buffer1.begin() + msgLen1, buffer2.begin(), buffer2.end());
        int len = write(socketId, buffer1.data(), msgLen1 + msgLen2);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        // recv
        len = RecvMsg(socketId, buffer2.data(), buffer2.size());
        APPSPAWN_CHECK(len >= static_cast<int>(sizeof(AppSpawnResponseMsg)), ret = -1;
            break, "Failed to recv msg %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnResponseMsg *respMsg = reinterpret_cast<AppSpawnResponseMsg *>(buffer2.data());
        APPSPAWN_LOGV("App_Spawn_Msg_006 recv msg %{public}s result: %{public}d",
            respMsg->msgHdr.processName, respMsg->result.result);
        ret = respMsg->result.result;
        (void)RecvMsg(socketId, buffer2.data(), buffer2.size());
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试连接中断
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_Msg_007, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = g_testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);
        std::vector<uint8_t> buffer(1024, 0);  // 1024 1k
        uint32_t msgLen = 0;
        ret = g_testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());

        int len = write(socketId, buffer.data(), msgLen);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        // close socket
        APPSPAWN_LOGV("CloseClientSocket");
        CloseClientSocket(socketId);
        socketId = -1;
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 发送不完整报文，等待超时
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_Msg_008, TestSize.Level0)
{
    int ret = 0;
    int socketId = -1;
    do {
        socketId = g_testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);
        std::vector<uint8_t> buffer(1024, 0);  // 1024 1k
        uint32_t msgLen = 0;
        ret = g_testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        int len = write(socketId, buffer.data(), msgLen - 20);  // 20 test
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", g_testServer->GetDefaultTestAppBundleName());
        // recv
        len = read(socketId, buffer.data(), buffer.size());  // timeout EAGAIN
        APPSPAWN_CHECK(len <= 0, ret = -1; break, "Can not receive timeout %{public}d", errno);
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 必须最后一个，kill nwebspawn，appspawn的线程结束
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_NWebSpawn_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
        // kill nwebspawn
        APPSPAWN_LOGV("App_Spawn_NWebSpawn_001 Kill nwebspawn");
        g_testServer->KillNWebSpawnServer();
    } while (0);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);

    g_testServer->Stop();
    delete g_testServer;
}

/**
 * @brief 测试环境变量设置是否正确
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_InitCommonEnv_001, TestSize.Level0)
{
    char *env;

    InitCommonEnv();

    if (IsDeveloperModeOpen() == true) {
        env = getenv("HNP_PRIVATE_HOME");
        EXPECT_NE(env, NULL);
        if (env != nullptr) {
            EXPECT_EQ(strcmp(env, "/data/app"), 0);
        }
        env = getenv("HNP_PUBLIC_HOME");
        EXPECT_NE(env, NULL);
        if (env != nullptr) {
            EXPECT_EQ(strcmp(env, "/data/service/hnp"), 0);
        }
        env = getenv("PATH");
        EXPECT_NE(env, NULL);
        if (env != nullptr) {
            EXPECT_NE((uint64_t)strstr(env, "/data/app/bin:/data/service/hnp/bin"), 0);
        }
    }
    env = getenv("HOME");
    EXPECT_NE(env, NULL);
    if (env != nullptr) {
        EXPECT_EQ(strcmp(env, "/storage/Users/currentUser"), 0);
    }
    env = getenv("TMPDIR");
    EXPECT_NE(env, NULL);
    if (env != nullptr) {
        EXPECT_EQ(strcmp(env, "/data/storage/el2/base/cache"), 0);
    }
    env = getenv("SHELL");
    EXPECT_NE(env, NULL);
    if (env != nullptr) {
        EXPECT_EQ(strcmp(env, "/bin/sh"), 0);
    }
    env = getenv("PWD");
    EXPECT_NE(env, NULL);
    if (env != nullptr) {
        EXPECT_EQ(strcmp(env, "/storage/Users/currentUser"), 0);
    }
}

/**
 * @brief 测试环境变量转换功能是否正常
 *
 */
HWTEST(AppSpawnServiceTest, App_Spawn_ConvertEnvValue_001, TestSize.Level0)
{
    char outEnv[MAX_ENV_VALUE_LEN];

    outEnv[0] = 0;
    EXPECT_EQ(ConvertEnvValue("/path/to/lib", outEnv, MAX_ENV_VALUE_LEN), 0);
    EXPECT_EQ(strcmp(outEnv, "/path/to/lib"), 0);

    EXPECT_EQ(ConvertEnvValue("/path/to/lib/$ENV_TEST_VALUE", outEnv, MAX_ENV_VALUE_LEN), 0);
    EXPECT_EQ(strcmp(outEnv, "/path/to/lib/$ENV_TEST_VALUE"), 0);

    EXPECT_EQ(ConvertEnvValue("/path/to/lib/${ENV_TEST_VALUE", outEnv, MAX_ENV_VALUE_LEN), 0);
    EXPECT_EQ(strcmp(outEnv, "/path/to/lib/${ENV_TEST_VALUE"), 0);

    EXPECT_EQ(ConvertEnvValue("/path/to/lib/${", outEnv, MAX_ENV_VALUE_LEN), 0);
    EXPECT_EQ(strcmp(outEnv, "/path/to/lib/${"), 0);
    
    EXPECT_EQ(ConvertEnvValue("/path/to/lib/${ENV_TEST_VALUE}", outEnv, MAX_ENV_VALUE_LEN), 0);
    EXPECT_EQ(strcmp(outEnv, "/path/to/lib/"), 0);

    EXPECT_EQ(setenv("ENV_TEST_VALUE", "envtest", 1), 0);
    EXPECT_EQ(ConvertEnvValue("/path/to/lib/${ENV_TEST_VALUE}", outEnv, MAX_ENV_VALUE_LEN), 0);
    EXPECT_EQ(strcmp(outEnv, "/path/to/lib/envtest"), 0);
    EXPECT_EQ(unsetenv("ENV_TEST_VALUE"), 0);
}
}  // namespace OHOS
