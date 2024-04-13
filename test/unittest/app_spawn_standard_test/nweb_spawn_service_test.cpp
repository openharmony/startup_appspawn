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
#include <unistd.h>

#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_client.h"
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
using AddTlvFunction = std::function<int(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount)>;
class NWebSpawnServiceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief 基本流程测试，启动nwebspawn 处理
 *
 */
HWTEST(NWebSpawnServiceTest, NWeb_Spawn_001, TestSize.Level0)
{
    g_testServer = new OHOS::AppSpawnTestServer("appspawn -mode nwebspawn");
    ASSERT_EQ(g_testServer != nullptr, 1);
    g_testServer->Start(nullptr);
    usleep(2000); // 2000

    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NWEBSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("NWeb_Spawn_001 recv result %{public}d", ret);
        if (ret != 0 || result.pid == 0) {
            ret = -1;
            break;
        }
        // stop child and termination
        APPSPAWN_LOGI("NWeb_Spawn_001 Kill pid %{public}d ", result.pid);
        kill(result.pid, SIGKILL);

        usleep(2000);  // wait，to get exit status
        // MSG_GET_RENDER_TERMINATION_STATUS
        AppSpawnReqMsgHandle reqHandle2 = nullptr;
        ret = AppSpawnTerminateMsgCreate(result.pid, &reqHandle2);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create req %{public}s", NWEBSPAWN_SERVER_NAME);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle2, &result);
        APPSPAWN_LOGV("Send MSG_GET_RENDER_TERMINATION_STATUS %{public}d", ret);
        ret = 0;
    } while (0);

    AppSpawnClientDestroy(clientHandle);
}

HWTEST(NWebSpawnServiceTest, NWeb_Spawn_002, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        // 发送消息到nweb spawn
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NWEBSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("NWeb_Spawn_002 recv result %{public}d %{public}d", ret, result.pid);
        if (ret != 0 || result.pid == 0) {
            ret = -1;
            break;
        }
        // MSG_GET_RENDER_TERMINATION_STATUS
        AppSpawnReqMsgHandle reqHandle2 = nullptr;
        ret = AppSpawnTerminateMsgCreate(result.pid, &reqHandle2);
        APPSPAWN_CHECK(ret == 0, break, "Failed to set pid %{public}s", NWEBSPAWN_SERVER_NAME);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle2, &result);
        APPSPAWN_LOGV("Send MSG_GET_RENDER_TERMINATION_STATUS %{public}d", ret);
        ret = 0;  // ut can not get result
    } while (0);

    AppSpawnClientDestroy(clientHandle);
}

HWTEST(NWebSpawnServiceTest, NWeb_Spawn_004, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NWEBSPAWN_SERVER_NAME);
        // MSG_DUMP
        AppSpawnResult result = {};
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_DUMP, 0);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("Send MSG_DUMP %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
}

HWTEST(NWebSpawnServiceTest, NWeb_Spawn_005, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    do {
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NWEBSPAWN_SERVER_NAME);
        // MAX_TYPE_INVALID
        AppSpawnResult result = {};
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MAX_TYPE_INVALID, 0);
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("Send MAX_TYPE_INVALID %{public}d", ret);
    } while (0);

    AppSpawnClientDestroy(clientHandle);
    ASSERT_NE(ret, 0);
    g_testServer->Stop();
    delete g_testServer;
}
}  // namespace OHOS
