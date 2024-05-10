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

#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "appspawn.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
extern "C" {
    void RunAppSandbox(const char *ptyName);
}
static OHOS::AppSpawnTestServer *g_testServer = nullptr;
static std::string g_ptyName = {};
static AppSpawnTestHelper g_testHelper;
class AppSpawnBegetCtlTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

static void InitPtyInterface()
{
    int pfd = open("/dev/ptmx", O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
    APPSPAWN_CHECK(pfd >= 0, return, "Failed open pty err=%d", errno);
    APPSPAWN_CHECK(grantpt(pfd) >= 0, close(pfd); return, "Failed to call grantpt");
    APPSPAWN_CHECK(unlockpt(pfd) >= 0, close(pfd); return, "Failed to call unlockpt");
    char ptsbuffer[128] = {0};
    int ret = ptsname_r(pfd, ptsbuffer, sizeof(ptsbuffer));
    APPSPAWN_CHECK(ret >= 0, close(pfd); return, "Failed to get pts name err=%d", errno);
    APPSPAWN_LOGI("ptsbuffer is %{public}s", ptsbuffer);
    APPSPAWN_CHECK(chmod(ptsbuffer, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == 0, close(pfd);
        return, "Failed to chmod %{public}s, err=%d", ptsbuffer, errno);
    g_ptyName = std::string(ptsbuffer);
}

static int TestSendAppspawnCmdMessage(const char *cmd, const char *ptyName)
{
    AppSpawnClientHandle clientHandle;
    int ret = AppSpawnClientInit("AppSpawn", &clientHandle);
    APPSPAWN_CHECK(ret == 0, return -1, "AppSpawnClientInit error");
    AppSpawnReqMsgHandle reqHandle;
    ret = AppSpawnReqMsgCreate(AppSpawnMsgType::MSG_BEGET_CMD, cmd, &reqHandle);
    APPSPAWN_CHECK(ret == 0, return -1, "AppSpawnReqMsgCreate error");
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BEGETCTL_BOOT);
    APPSPAWN_CHECK(ret == 0,  return -1, "AppSpawnReqMsgSetAppFlag error");
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_BEGET_PID, cmd);
    APPSPAWN_CHECK(ret == 0,  return -1, "add %{public}s request message error", cmd);
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_BEGET_PTY_NAME, ptyName);
    APPSPAWN_CHECK(ret == 0,  return -1, "add %{public}s request message error", ptyName);
    AppSpawnResult result = {};
    ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    if (ret == 0 && result.pid > 0) {
        kill(result.pid, SIGKILL);
    }
    AppSpawnClientDestroy(clientHandle);
    return 0;
}

HWTEST(AppSpawnBegetCtlTest, App_Spawn_BetgetCtl_001, TestSize.Level0)
{
    InitPtyInterface();
    EXPECT_NE(g_ptyName.c_str(), nullptr);
    APPSPAWN_LOGI(" ***ptsbuffer is %{public}s", g_ptyName.c_str());
    int ret = TestSendAppspawnCmdMessage("1111", g_ptyName.c_str());
    EXPECT_EQ(ret, 0);
    RunAppSandbox(g_ptyName.c_str());
    RunAppSandbox(nullptr);
}

HWTEST(AppSpawnBegetCtlTest, App_Spawn_BetgetCtl_002, TestSize.Level0)
{
    g_testServer = new OHOS::AppSpawnTestServer("appspawn -mode appspawn");
    ASSERT_EQ(g_testServer != nullptr, 1);
    g_testServer->Start(nullptr);
    int ret = -1;
    InitPtyInterface();
    EXPECT_NE(g_ptyName.c_str(), nullptr);
    ret = TestSendAppspawnCmdMessage("1008", g_ptyName.c_str());
    EXPECT_EQ(ret, 0);
    g_testServer->KillNWebSpawnServer();
    g_testServer->Stop();
    delete g_testServer;
}

HWTEST(AppSpawnBegetCtlTest, App_Spawn_BetgetCtl_003, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnContent *content = nullptr;
    int ret = -1;
    do {
        InitPtyInterface();
        EXPECT_NE(g_ptyName.c_str(), nullptr);
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        char path[PATH_MAX] = {};
        content = AppSpawnCreateContent(APPSPAWN_SOCKET_NAME, path, sizeof(path), MODE_FOR_APP_SPAWN);
        APPSPAWN_CHECK_ONLY_EXPER(content != nullptr, break);

        ServerStageHookExecute(STAGE_SERVER_PRELOAD, content);  // 预加载，解析sandbox

        ret = APPSPAWN_ARG_INVALID;
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_BEGETCTL_BOOT);
        APPSPAWN_CHECK(ret == 0,  break, "AppSpawnReqMsgSetAppFlag error");
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_BEGET_PID, "1008");
        APPSPAWN_CHECK(ret == 0,  break, "add app pid request message error");
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_BEGET_PTY_NAME, g_ptyName.c_str());
        APPSPAWN_CHECK(ret == 0,  break, "add %{public}s request message error", g_ptyName.c_str());
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        reqHandle = nullptr;
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        // spawn prepare process
        AppSpawnHookExecute(STAGE_PARENT_PRE_FORK, 0, content, &property->client);

        // spawn
        ret = AppSpawnChild(content, &property->client);
        property = nullptr;
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}
}  // namespace OHOS
