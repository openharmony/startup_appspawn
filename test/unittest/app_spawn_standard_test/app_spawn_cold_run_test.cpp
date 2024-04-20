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

#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

#include <gtest/gtest.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
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
class AppSpawnColdRunTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase()
    {
        StubNode *stub = GetStubNode(STUB_MOUNT);
        if (stub) {
            stub->flags &= ~STUB_NEED_CHECK;
        }
    }
    void SetUp() {}
    void TearDown() {}
};

/**
 * 接管启动的exec 过程
 *
 */
static int ExecvAbortStub(const char *pathName, char *const argv[])
{
    if (!(strcmp(pathName, "/system/bin/appspawn") == 0 || strcmp(pathName, "/system/asan/bin/appspawn") == 0)) {
        return 0;
    }
    APPSPAWN_LOGV("ExecvAbortStub pathName: %{public}s ", pathName);
    _exit(0x7f);
    return 0;
}

int ExecvLocalProcessStub(const char *pathName, char *const argv[])
{
    if (!(strcmp(pathName, "/system/bin/appspawn") == 0 || strcmp(pathName, "/system/asan/bin/appspawn") == 0)) {
        return 0;
    }
    APPSPAWN_LOGV("ExecvLocalProcessStub pathName: %{public}s ", pathName);
    return 0;
}

static int ExecvTimeoutStub(const char *pathName, char *const argv[])
{
    if (!(strcmp(pathName, "/system/bin/appspawn") == 0 || strcmp(pathName, "/system/asan/bin/appspawn") == 0)) {
        return 0;
    }
    APPSPAWN_LOGV("ExecvLocalProcessStub pathName: %{public}s ", pathName);
    usleep(500000);  // 500000 500ms
    return 0;
}

static int HandleExecvStub(const char *pathName, char *const argv[])
{
    if (!(strcmp(pathName, "/system/bin/appspawn") == 0 || strcmp(pathName, "/system/asan/bin/appspawn") == 0)) {
        return 0;
    }
    std::string cmd;
    int index = 0;
    do {
        cmd += argv[index];
        cmd += " ";
        index++;
    } while (argv[index] != nullptr);
    APPSPAWN_LOGV("HandleExecvStub cmd: %{public}s ", cmd.c_str());

    CmdArgs *args = nullptr;
    AppSpawnContent *content = AppSpawnTestHelper::StartSpawnServer(cmd, args);
    if (content == nullptr) {
        free(args);
        return -1;
    }
    content->runAppSpawn(content, args->argc, args->argv);
    free(args);
    APPSPAWN_LOGV("HandleExecvStub %{public}s exit", pathName);
    _exit(0x7f); // 0x7f user exit
    return 0;
}

HWTEST(AppSpawnColdRunTest, App_Spawn_Cold_Run_001, TestSize.Level0)
{
    g_testServer = new OHOS::AppSpawnTestServer("appspawn -mode appspawn");
    ASSERT_EQ(g_testServer != nullptr, 1);
    g_testServer->Start(nullptr);

    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    StubNode *node = GetStubNode(STUB_EXECV);
    ASSERT_NE(node != nullptr, 0);
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        // set cold start flags
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_COLD_BOOT);

        ret = -1;
        node->flags |= STUB_NEED_CHECK;
        node->arg = reinterpret_cast<void *>(HandleExecvStub);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("App_Spawn_Cold_Run_001 Kill pid %{public}d %{public}d", result.pid, result.result);
        if (ret == 0 && result.pid > 0) {
            kill(result.pid, SIGKILL);
        }
        ret = 0;
    } while (0);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST(AppSpawnColdRunTest, App_Spawn_Cold_Run_002, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    StubNode *node = GetStubNode(STUB_EXECV);
    ASSERT_NE(node != nullptr, 0);
    do {
        ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", NWEBSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        // set cold start flags
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_COLD_BOOT);

        ret = -1;
        node->flags |= STUB_NEED_CHECK;
        node->arg = reinterpret_cast<void *>(HandleExecvStub);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("App_Spawn_Cold_Run_002 Kill pid %{public}d %{public}d", result.pid, result.result);
        if (ret == 0 && result.pid > 0) {
            kill(result.pid, SIGKILL);
        }
        ret = 0;
    } while (0);
    AppSpawnClientDestroy(clientHandle);
    node->flags &= ~STUB_NEED_CHECK;
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试子进程abort
 *
 */
HWTEST(AppSpawnColdRunTest, App_Spawn_Cold_Run_003, TestSize.Level0)
{
    // child abort
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    StubNode *node = GetStubNode(STUB_EXECV);
    ASSERT_NE(node != nullptr, 0);
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        // set cold start flags
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_COLD_BOOT);

        ret = -1;
        node->flags |= STUB_NEED_CHECK;
        node->arg = reinterpret_cast<void *>(ExecvAbortStub);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("App_Spawn_Cold_Run_003 Kill pid %{public}d %{public}d", result.pid, result.result);
        if (ret == 0 && result.pid > 0) {
            kill(result.pid, SIGKILL);
        }
        ret = 0;
    } while (0);
    AppSpawnClientDestroy(clientHandle);
    node->flags &= ~STUB_NEED_CHECK;
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试子进程不回复，导致等到超时
 *
 */
HWTEST(AppSpawnColdRunTest, App_Spawn_Cold_Run_004, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    StubNode *node = GetStubNode(STUB_EXECV);
    ASSERT_NE(node != nullptr, 0);
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_testServer->CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        // set cold start flags
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_COLD_BOOT);

        ret = -1;
        node->flags |= STUB_NEED_CHECK;
        node->arg = reinterpret_cast<void *>(ExecvTimeoutStub);
        AppSpawnResult result = {};
        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_LOGV("App_Spawn_Cold_Run_004 Kill pid %{public}d %{public}d", result.pid, result.result);
        if (ret == 0 && result.pid > 0) {
            kill(result.pid, SIGKILL);
        }
        ret = 0;
    } while (0);
    AppSpawnClientDestroy(clientHandle);
    node->flags &= ~STUB_NEED_CHECK;
    ASSERT_EQ(ret, 0);

    g_testServer->Stop();
    delete g_testServer;
}

}  // namespace OHOS
