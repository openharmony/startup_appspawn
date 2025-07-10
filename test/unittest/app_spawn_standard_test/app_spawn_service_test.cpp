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
 * @brief 向appspawn发送MSG_APP_SPAWN类型的消息
 * @note 预期结果：appspawn接收到请求消息，孵化应用进程
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_APP_SPAWN_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
            break, "Failed to send MSG_APP_SPAWN, ret %{public}d pid %{public}d", ret, result.pid);

        DumpSpawnStack(result.pid);
        ret = kill(result.pid, SIGKILL);
        APPSPAWN_CHECK(ret == 0, break, "unable to kill pid %{public}d, err %{public}d", result.pid, errno);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 通过多线程模拟多个客户端向appspawn发送MSG_APP_SPAWN类型的消息
 * @note 预期结果：appspawn接收到请求消息，逐个孵化应用进程
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_APP_SPAWN_002, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        auto sendMsg = [this, &ret](AppSpawnClientHandle clientHandle) {
            AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

            AppSpawnResult result = {};
            ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
            APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
                return, "Failed to send MSG_APP_SPAWN, ret %{public}d pid %{public}d", ret, result.pid);

            ret = kill(result.pid, SIGKILL);
            APPSPAWN_CHECK(ret == 0, return, "unable to kill pid %{public}d, err %{public}d", result.pid, errno);
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

/**
 * @brief appspawn孵化的应用进程退出后，向appspawn发送MSG_GET_RENDER_TERMINATION_STATUS类型的消息
 * @note 预期结果：appspawn不支持处理该类型消息，不会根据消息中传入的pid去查询进程的退出状态
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_GET_RENDER_TERMINATION_STATUS_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
            break, "Failed to send MSG_APP_SPAWN, ret %{public}d pid %{public}d", ret, result.pid);

        ret = kill(result.pid, SIGKILL);
        APPSPAWN_CHECK(ret == 0, break, "unable to kill pid %{public}d, err %{public}d", result.pid, errno);

        ret = AppSpawnTerminateMsgCreate(result.pid, &reqHandle);   // MSG_GET_RENDER_TERMINATION_STATUS
        APPSPAWN_CHECK(ret == 0, break, "Failed to create termination msg %{public}s", APPSPAWN_SERVER_NAME);

        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.result == APPSPAWN_MSG_INVALID, ret = -1;
            break, "Failed to send MSG_GET_RENDER_TERMINATION_STATUS, ret %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 向appspawn发送MSG_SPAWN_NATIVE_PROCESS类型的消息
 * @note 预期结果：appspawn接收到请求消息，孵化native进程
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_SPAWN_NATIVE_PROCESS_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
            break, "Failed to send MSG_SPAWN_NATIVE_PROCESS, ret %{public}d pid %{public}d", ret, result.pid);

        ret = kill(result.pid, SIGKILL);
        APPSPAWN_CHECK(ret == 0, break, "unable to kill pid %{public}d, err %{public}d", result.pid, errno);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn孵化应用进程后，向appspawn发送MSG_DUMP类型的消息
 * @note 预期结果：appspawn接收到请求消息，dump打印出appQueue等信息
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_DUMP_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
            break, "Failed to send MSG_APP_SPAWN, ret %{public}d pid %{public}d", ret, result.pid);

        reqHandle = testServer->CreateMsg(clientHandle, MSG_DUMP, 0);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send MSG_DUMP, ret %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 非开发者模式下，向appspawn发送MSG_BEGET_CMD类型的消息
 * @note 预期结果：非开发者模式下，appspawn不支持处理该类型消息，不会根据消息中传入的pid进入应用沙箱
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_BEGET_CMD_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    SetDeveloperMode(false);
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
            break, "Failed to send MSG_APP_SPAWN, ret %{public}d pid %{public}d", ret, result.pid);

        reqHandle = testServer->CreateMsg(clientHandle, MSG_BEGET_CMD, 0);
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BEGETCTL_BOOT);
        APPSPAWN_CHECK(ret == 0, break, "Failed to set msg flag 0x%{public}x", APP_FLAGS_BEGETCTL_BOOT);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_BEGET_PID, std::to_string(result.pid).c_str());
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_BEGET_PID %{public}d", result.pid);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_BEGET_PTY_NAME, "/dev/pts/1");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_BEGET_PTY_NAME %{public}s", "/dev/pts/1");
    
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.result == APPSPAWN_DEBUG_MODE_NOT_SUPPORT, ret = -1;
            break, "Failed to send MSG_BEGET_CMD, ret %{public}d result %{public}d", ret, result.result);
    } while (0);

    SetDeveloperMode(true);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 向appspawn发送MSG_BEGET_SPAWNTIME类型的消息
 * @note 预期结果：appspawn接收到请求消息，获取appspawn启动时各hook执行的最小时间(result.result)和最大时间(result.pid)
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_BEGET_SPAWNTIME_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    int minAppspawnTime = 0;
    int maxAppspawnTime = APPSPAWN_MAX_TIME;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_BEGET_SPAWNTIME, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        minAppspawnTime = result.result;
        maxAppspawnTime = result.pid;
        APPSPAWN_CHECK(ret == 0, break, "Failed to send MSG_BEGET_SPAWNTIME, ret %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    EXPECT_GT(minAppspawnTime, 0);
    EXPECT_LT(maxAppspawnTime, APPSPAWN_MAX_TIME);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 向appspawn发送MSG_UPDATE_MOUNT_POINTS类型的消息
 * @note 预期结果：appspawn接收到请求消息，遍历沙箱包名更新挂载点
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_UPDATE_MOUNT_POINTS_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_UPDATE_MOUNT_POINTS, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.result == 0, ret = -1;
            break, "Failed to send MSG_UPDATE_MOUNT_POINTS, ret %{public}d result %{public}d", ret, result.result);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 向appspawn发送MSG_RESTART_SPAWNER类型的消息
 * @note 预期结果：appspawn不支持处理该类型消息，返回APPSPAWN_MSG_INVALID
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_RESTART_SPAWNER_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_RESTART_SPAWNER, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.result == APPSPAWN_MSG_INVALID, ret = -1;
            break, "Failed to send MSG_RESTART_SPAWNER, ret %{public}d result %{public}d", ret, result.result);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn孵化debuggable应用后，向appspawn发送MSG_DEVICE_DEBUG类型的消息
 * @note 预期结果：appspawn接收到请求消息，根据消息中传入的debuggable应用pid和signal杀死先前孵化的应用
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_DEVICE_DEBUG_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);
        APPSPAWN_CHECK(ret == 0, break, "Failed to set msg flag 0x%{public}x", APP_FLAGS_DEBUGGABLE);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
            break, "Failed to send MSG_APP_SPAWN, ret %{public}d pid %{public}d", ret, result.pid);

        reqHandle = testServer->CreateMsg(clientHandle, MSG_DEVICE_DEBUG, 0);
        char buffer[256] = {0};
        ret = snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1,
            "{ \"app\": %d, \"op\": \"kill\", \"args\": { \"signal\": 9 } }", result.pid);
        APPSPAWN_CHECK(ret > 0, ret = -1;
            break, "Failed to snprintf_s devicedebug extra info, err %{public}d", errno);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, "devicedebug", buffer);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add devicedebug extra info %{public}s", buffer);
    
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.result == 0, ret = -1;
            break, "Failed to send MSG_DEVICE_DEBUG, ret %{public}d result %{public}d", ret, result.result);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn孵化非debuggable应用后，向appspawn发送MSG_DEVICE_DEBUG类型的消息
 * @note 预期结果：appspawn接收到请求消息，但不支持向非debuggable应用发送signal
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_DEVICE_DEBUG_002, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
            break, "Failed to send MSG_APP_SPAWN, ret %{public}d pid %{public}d", ret, result.pid);

        reqHandle = testServer->CreateMsg(clientHandle, MSG_DEVICE_DEBUG, 0);
        char buffer[256] = {0};
        ret = snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1,
            "{ \"app\": %d, \"op\": \"kill\", \"args\": { \"signal\": 9 } }", result.pid);
        APPSPAWN_CHECK(ret > 0, ret = -1;
            break, "Failed to snprintf_s devicedebug extra info, err %{public}d", errno);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, "devicedebug", buffer);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add devicedebug extra info %{public}s", buffer);
    
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.result == APPSPAWN_DEVICEDEBUG_ERROR_APP_NOT_DEBUGGABLE, ret = -1;
            break, "Failed to send MSG_DEVICE_DEBUG, ret %{public}d result %{public}d", ret, result.result);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief appspawn孵化带debug沙箱的应用后，向appspawn发送MSG_UNINSTALL_DEBUG_HAP类型的消息
 * @note 预期结果：appspawn接收到请求消息，卸载掉指定用户下的所有debug沙箱
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_UNINSTALL_DEBUG_HAP_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PROVISION_TYPE, "debug");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_PROVISION_TYPE %{public}s", "debug");

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.pid > 0, ret = -1;
            break, "Failed to send MSG_APP_SPAWN, ret %{public}d pid %{public}d", ret, result.pid);

        reqHandle = testServer->CreateMsg(clientHandle, MSG_UNINSTALL_DEBUG_HAP, 0);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_USERID, "100");
        APPSPAWN_CHECK(ret == 0, break, "Failed to add MSG_EXT_NAME_USERID %{public}s", "100");
    
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.result == 0, ret = -1;
            break, "Failed to send MSG_UNINSTALL_DEBUG_HAP, ret %{public}d result %{public}d", ret, result.result);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 向appspawn发送MSG_LOCK_STATUS类型的消息
 * @note 预期结果：appspawn接收到请求消息，根据消息中的传入的解锁状态对lockstatus参数进行设值
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MSG_LOCK_STATUS_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_LOCK_STATUS, 0);

        char lockstatus[8] = {0};
        ret = snprintf_s(lockstatus, sizeof(lockstatus), sizeof(lockstatus) - 1, "%u:%d", 100, 0);
        APPSPAWN_CHECK(ret > 0, ret = -1;
            break, "Failed to snprintf_s lockstatus extra info, err %{public}d", errno);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, "lockstatus", lockstatus);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add lockstatus extra info %{public}s", lockstatus);
    
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0 && result.result == 0, ret = -1;
            break, "Failed to send MSG_LOCK_STATUS, ret %{public}d result %{public}d", ret, result.result);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 向appspawn发送MAX_TYPE_INVALID类型的消息
 * @note 预期结果：不支持创建该类型消息，发送失败，appspawn接收不到该消息请求
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_MAX_TYPE_INVALID_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MAX_TYPE_INVALID, 0);

        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == APPSPAWN_ARG_INVALID, break, "Failed to send MAX_TYPE_INVALID, ret %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, APPSPAWN_ARG_INVALID);
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
 * @brief 测试子进程退出时，nativespawn通过fd中发送子进程退出状态，发送失败
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_013, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnResult result = {};
    do {
        int pipefd[2]; // 2 pipe fd
        char buffer[1024]; // 1024 1k
        APPSPAWN_CHECK(pipe(pipefd) == 0, break, "Failed to pipe fd errno:%{public}d", errno);
        ret = NativeSpawnListenFdSet(pipefd[0]);

        ret = AppSpawnClientInit(NATIVESPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NATIVESPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

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
        NativeSpawnListenCloseSet();
    } while (0);
}

/**
 * @brief 测试子进程退出时，nativespawn通过fd中发送子进程退出状态，发送成功
 *
 */
HWTEST_F(AppSpawnServiceTest, App_Spawn_Msg_014, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnResult result = {};
    do {
        int pipefd[2]; // 2 pipe fd
        char buffer[1024]; // 1024 1k
        APPSPAWN_CHECK(pipe(pipefd) == 0, break, "Failed to pipe fd errno:%{public}d", errno);
        ret = NativeSpawnListenFdSet(pipefd[1]);

        ret = AppSpawnClientInit(NATIVESPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NATIVESPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);

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
        NativeSpawnListenCloseSet();
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
