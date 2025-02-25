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

#include "appspawn_adapter.h"
#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "json_utils.h"
#include "parameter.h"
#include "securec.h"
#include "cJSON.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
class AppSpawnServiceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        testServer = std::make_unique<OHOS::AppSpawnTestServer>("appspawn -mode appspawn");
        if (testServer != nullptr) {
            testServer->Start(nullptr);
        }
    }
    void TearDown()
    {
        if (testServer != nullptr) {
            testServer->Stop();
        }
    }
public:
    std::unique_ptr<OHOS::AppSpawnTestServer> testServer = nullptr;
};

/**
 * @brief 正常消息发送和接收，完整应用孵化过程
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        APPSPAWN_LOGV("App_Spawn_001 start");
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

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
            DumpSpawnStack(result.pid);
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_002, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        APPSPAWN_LOGV("App_Spawn_002 start");
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_003, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_004, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_DUMP, 0);
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_005, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);

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
HWTEST_F(AppSpawnServiceTest, App_Spawn_006, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        auto sendMsg = [this](AppSpawnClientHandle clientHandle) {
            AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);

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

HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_001, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    // 没有tlv的消息，返回错误
    do {
        socketId = testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer(sizeof(AppSpawnResponseMsg));
        uint32_t msgLen = 0;
        ret = testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());

        int len = write(socketId, buffer.data(), msgLen);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", testServer->GetDefaultTestAppBundleName());
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_002, TestSize.Level0)
{
    int ret = 0;
    int socketId = -1;
    do {
        socketId = testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer(sizeof(AppSpawnResponseMsg));
        uint32_t msgLen = 0;
        ret = testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());

        ret = -1;
        int len = write(socketId, buffer.data(), msgLen - 10);  // 10
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", testServer->GetDefaultTestAppBundleName());
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_003, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer(sizeof(AppSpawnResponseMsg));
        uint32_t msgLen = 0;
        ret = testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());

        int len = write(socketId, buffer.data(), msgLen);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", testServer->GetDefaultTestAppBundleName());
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_004, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer(1024, 0);  // 1024 1k
        uint32_t msgLen = 0;
        ret = testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());

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
            APPSPAWN_CHECK(len > 0, break, "Failed to send %{public}s", testServer->GetDefaultTestAppBundleName());
            usleep(2000);  // wait recv
            currIndex += sendStep;
        } while (1);
        APPSPAWN_CHECK(len > 0, break, "Failed to send %{public}s", testServer->GetDefaultTestAppBundleName());
        if (msgLen > currIndex) {
            len = write(socketId, buffer.data() + currIndex, msgLen - currIndex);
            APPSPAWN_CHECK(len > 0, break, "Failed to send %{public}s", testServer->GetDefaultTestAppBundleName());
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_005, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer1(1024);  // 1024
        std::vector<uint8_t> buffer2(1024);  // 1024
        uint32_t msgLen1 = 0;
        uint32_t msgLen2 = 0;
        ret = testServer->CreateSendMsg(buffer1, MSG_APP_SPAWN, msgLen1, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());
        ret = testServer->CreateSendMsg(buffer2, MSG_APP_SPAWN, msgLen2, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());

        buffer1.insert(buffer1.begin() + msgLen1, buffer2.begin(), buffer2.end());
        int len = write(socketId, buffer1.data(), msgLen1 + msgLen2);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", testServer->GetDefaultTestAppBundleName());
        // recv
        sleep(2);
        len = RecvMsg(socketId, buffer2.data(), buffer2.size());
        APPSPAWN_CHECK(len >= static_cast<int>(sizeof(AppSpawnResponseMsg)), ret = -1;
            break, "Failed to recv msg %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnResponseMsg *respMsg = reinterpret_cast<AppSpawnResponseMsg *>(buffer2.data());
        APPSPAWN_LOGV("App_Spawn_Msg_005 Recv msg %{public}s result: %{public}d",
            respMsg->msgHdr.processName, respMsg->result.result);
        ret = respMsg->result.result;
        (void)RecvMsg(socketId, buffer2.data(), buffer2.size());
    } while (0);
    ret = 0; // test for case
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试连续2个消息，spawn和dump 消息
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_006, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);

        std::vector<uint8_t> buffer1(2 * 1024);  // 2 * 1024
        std::vector<uint8_t> buffer2(1024);  // 1024
        uint32_t msgLen1 = 0;
        uint32_t msgLen2 = 0;
        ret = testServer->CreateSendMsg(buffer1, MSG_APP_SPAWN, msgLen1, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());
        ret = testServer->CreateSendMsg(buffer2, MSG_DUMP, msgLen2, {});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());

        buffer1.insert(buffer1.begin() + msgLen1, buffer2.begin(), buffer2.end());
        int len = write(socketId, buffer1.data(), msgLen1 + msgLen2);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", testServer->GetDefaultTestAppBundleName());
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_007, TestSize.Level0)
{
    int ret = -1;
    int socketId = -1;
    do {
        socketId = testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);
        std::vector<uint8_t> buffer(1024, 0);  // 1024 1k
        uint32_t msgLen = 0;
        ret = testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());

        int len = write(socketId, buffer.data(), msgLen);
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", testServer->GetDefaultTestAppBundleName());
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
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_008, TestSize.Level0)
{
    int ret = 0;
    int socketId = -1;
    do {
        socketId = testServer->CreateSocket();
        APPSPAWN_CHECK(socketId >= 0, break, "Failed to create socket %{public}s", APPSPAWN_SERVER_NAME);
        std::vector<uint8_t> buffer(1024, 0);  // 1024 1k
        uint32_t msgLen = 0;
        ret = testServer->CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
        APPSPAWN_CHECK(ret == 0, break, "Failed to create msg %{public}s", testServer->GetDefaultTestAppBundleName());
        int len = write(socketId, buffer.data(), msgLen - 20);  // 20 test
        APPSPAWN_CHECK(len > 0, break, "Failed to send msg %{public}s", testServer->GetDefaultTestAppBundleName());
        // recv
        len = read(socketId, buffer.data(), buffer.size());  // timeout EAGAIN
        APPSPAWN_CHECK(len <= 0, ret = -1; break, "Can not receive timeout %{public}d", errno);
    } while (0);
    if (socketId >= 0) {
        CloseClientSocket(socketId);
    }
    ASSERT_EQ(ret, 0);
}

HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_009, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnResult result = {};
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);

        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);

        AppSpawnedProcess *app = GetSpawnedProcessByName(testServer->GetDefaultTestAppBundleName());
        ASSERT_NE(app, nullptr);

        AppSpawnReqMsgHandle reqHandle2;
        ret = AppSpawnReqMsgCreate(MSG_DEVICE_DEBUG, "devicedebug", &reqHandle2);
        cJSON *args = cJSON_CreateObject();
        ASSERT_NE(args, nullptr);
        cJSON_AddNumberToObject(args, "signal", 9);
        cJSON *root = cJSON_CreateObject();
        ASSERT_NE(root, nullptr);
        cJSON_AddNumberToObject(root, "app", app->pid);
        cJSON_AddStringToObject(root, "op", "kill");
        cJSON_AddItemToObject(root, "args", args);
        char *jsonString = cJSON_Print(root);
        cJSON_Delete(root);
        ret = AppSpawnReqMsgAddExtInfo(reqHandle2, "devicedebug", (uint8_t *)jsonString, strlen(jsonString) + 1);
        ASSERT_EQ(ret, 0);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle2, &result);
        AppSpawnClientDestroy(clientHandle);
        free(jsonString);
        APPSPAWN_CHECK(ret == 0 && result.result == 0, break, "Failed to send msg ret:%{public}d, result:%{public}d",
            ret, result.result);
        ASSERT_EQ(kill(app->pid, SIGKILL), 0);
    } while (0);

    ASSERT_EQ(ret, 0);
    ASSERT_EQ(result.result, APPSPAWN_DEVICEDEBUG_ERROR_APP_NOT_DEBUGGABLE);
}

HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_010, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnResult result = {};
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);

        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);

        AppSpawnedProcess *app = GetSpawnedProcessByName(testServer->GetDefaultTestAppBundleName());
        ASSERT_NE(app, nullptr);

        AppSpawnReqMsgHandle reqHandle2;
        ret = AppSpawnReqMsgCreate(MSG_DEVICE_DEBUG, "devicedebug", &reqHandle2);
        cJSON *args = cJSON_CreateObject();
        ASSERT_NE(args, nullptr);
        cJSON_AddNumberToObject(args, "signal", 9);
        cJSON *root = cJSON_CreateObject();
        ASSERT_NE(root, nullptr);
        cJSON_AddNumberToObject(root, "app", app->pid);
        cJSON_AddStringToObject(root, "op", "kill");
        cJSON_AddItemToObject(root, "args", args);
        char *jsonString = cJSON_Print(root);
        cJSON_Delete(root);
        ret = AppSpawnReqMsgAddExtInfo(reqHandle2, "devicedebug", (uint8_t *)jsonString, strlen(jsonString) + 1);
        ASSERT_EQ(ret, 0);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle2, &result);
        AppSpawnClientDestroy(clientHandle);
        free(jsonString);
        APPSPAWN_CHECK(ret == 0 && result.result == 0, break, "Failed to send msg ret:%{public}d, result:%{public}d",
            ret, result.result);
    } while (0);

    ASSERT_EQ(ret, 0);
    ASSERT_EQ(result.result, 0);
}

HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_011, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnResult result = {};
    do {
        int pipefd[2]; // 2 pipe fd
        char buffer[1024]; //1024 1k
        APPSPAWN_CHECK(pipe(pipefd) == 0, break, "Failed to pipe fd errno:%{public}d", errno);
        ret = SpawnListenFdSet(pipefd[0]);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);

        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
        AppSpawnClientDestroy(clientHandle);
        sleep(1); // wait child process stand up

        AppSpawnedProcess *app = GetSpawnedProcessByName(testServer->GetDefaultTestAppBundleName());
        ASSERT_NE(app, nullptr);
        char commamd[16]; // command len 16
        APPSPAWN_CHECK(sprintf_s(commamd, 16, "kill -9 %d", app->pid) > 0, break, "sprintf command unsuccess");
        system(commamd);

        bool isFind = false;
        int count = 0;
        while (count < 10) {
            if (read(pipefd[1], buffer, sizeof(buffer)) <= 0) {
                count++;
                continue;
            }
            if (strstr(buffer, std::to_string(app->pid).c_str()) != NULL) {
                isFind = true;
                break;
            }
            count++;
        }
        close(pipefd[0]);
        close(pipefd[1]);
        ASSERT_EQ(isFind, false);
        SpawnListenCloseSet();
    } while (0);
}

HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_012, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnResult result = {};
    do {
        int pipefd[2]; // 2 pipe fd
        char buffer[1024]; //1024 1k
        APPSPAWN_CHECK(pipe(pipefd) == 0, break, "Failed to pipe fd errno:%{public}d", errno);
        ret = SpawnListenFdSet(pipefd[1]);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_NATIVEDEBUG);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BUNDLE_RESOURCES);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ACCESS_BUNDLE_DIR);

        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
        AppSpawnClientDestroy(clientHandle);
        sleep(1); // wait child process stand up

        AppSpawnedProcess *app = GetSpawnedProcessByName(testServer->GetDefaultTestAppBundleName());
        ASSERT_NE(app, nullptr);
        char commamd[16]; // command len 16
        APPSPAWN_CHECK(sprintf_s(commamd, 16, "kill -9 %d", app->pid) > 0, break, "sprintf command unsuccess");
        system(commamd);

        bool isFind = false;
        int count = 0;
        while (count < 10) {
            if (read(pipefd[0], buffer, sizeof(buffer)) <= 0) {
                count++;
                continue;
            }
            if (strstr(buffer, std::to_string(app->pid).c_str()) != NULL) {
                isFind = true;
                break;
            }
            count++;
        }
        close(pipefd[0]);
        close(pipefd[1]);
        ASSERT_EQ(isFind, true);
        SpawnListenCloseSet();
    } while (0);
}

/**
 * @brief 必须最后一个，kill nwebspawn，appspawn的线程结束
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_NWebSpawn_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
        // kill nwebspawn
        APPSPAWN_LOGV("App_Spawn_NWebSpawn_001 Kill nwebspawn");
        testServer->KillNWebSpawnServer();
    } while (0);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试环境变量设置是否正确
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_InitCommonEnv_001, TestSize.Level0)
{
    char *env;

    InitCommonEnv();

    if (IsDeveloperModeOpen() == true) {
        env = getenv("HNP_PRIVATE_HOME");
        EXPECT_NE(env, nullptr);
        if (env != nullptr) {
            EXPECT_EQ(strcmp(env, "/data/app"), 0);
        }
        env = getenv("HNP_PUBLIC_HOME");
        EXPECT_NE(env, nullptr);
        if (env != nullptr) {
            EXPECT_EQ(strcmp(env, "/data/service/hnp"), 0);
        }
        env = getenv("PATH");
        EXPECT_NE(env, nullptr);
        if (env != nullptr) {
            EXPECT_NE(strstr(env, "/data/app/bin:/data/service/hnp/bin"), nullptr);
        }
    }
    env = getenv("HOME");
    EXPECT_NE(env, nullptr);
    if (env != nullptr) {
        EXPECT_EQ(strcmp(env, "/storage/Users/currentUser"), 0);
    }
    env = getenv("TMPDIR");
    EXPECT_NE(env, nullptr);
    if (env != nullptr) {
        EXPECT_EQ(strcmp(env, "/data/storage/el2/base/cache"), 0);
    }
    env = getenv("SHELL");
    EXPECT_NE(env, nullptr);
    if (env != nullptr) {
        EXPECT_EQ(strcmp(env, "/bin/sh"), 0);
    }
    env = getenv("PWD");
    EXPECT_NE(env, nullptr);
    if (env != nullptr) {
        EXPECT_EQ(strcmp(env, "/storage/Users/currentUser"), 0);
    }
}

/**
 * @brief 测试环境变量转换功能是否正常
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_ConvertEnvValue_001, TestSize.Level0)
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
