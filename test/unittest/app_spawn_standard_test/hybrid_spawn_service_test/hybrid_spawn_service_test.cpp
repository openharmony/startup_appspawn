/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <unistd.h>

#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "securec.h"

#include "appspawn_manager.h"
#include "appspawn_server.h"
#include "server_test_helper.h"
#include "lib_wrapper.h"
#include "spawning_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static SpawningTestHelper g_spawningTestHelper;
class HybridSpawnServiceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());

        testServer = std::make_unique<OHOS::ServerTestHelper>("hybridspawn -mode hybridspawn");
        if (testServer != nullptr) {
            testServer->Start(nullptr);
        }
    }
    void TearDown()
    {
        if (testServer != nullptr) {
            testServer->Stop();
        }

        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
    }
public:
    std::unique_ptr<OHOS::ServerTestHelper> testServer = nullptr;
};

/**
 * @brief 测试子进程退出时，hybridspawn通过错误的fd（pipe读端）中写入子进程退出状态，发送失败
 *
 */
HWTEST_F(HybridSpawnServiceTest, Hybrid_Spawn_SignalFd_01, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnResult result = {};
    MountFunc mountFunc = [](const char *, const char *, const char *, unsigned long, const void *) -> int {
        return 0;
    };
    UpdateMountFunc(mountFunc);
    do {
        int pipefd[2]; // 2 pipe fd
        char buffer[1024]; // 1024 1k
        APPSPAWN_CHECK(pipe(pipefd) == 0, break, "Failed to pipe fd errno:%{public}d", errno);
        ret = HybridSpawnListenFdSet(pipefd[0]);

        ret = AppSpawnClientInit(HYBRIDSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", HYBRIDSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_spawningTestHelper.SpawningTestCreateSendMsg(clientHandle, MSG_APP_SPAWN, 0);

        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
        AppSpawnClientDestroy(clientHandle);
        sleep(1); // wait child process stand up

        AppSpawnedProcess *app = GetSpawnedProcessByName(g_spawningTestHelper.GetDefaultTestAppBundleName());
        ASSERT_NE(app, nullptr);
        char commamd[16]; // command len 16
        APPSPAWN_CHECK(snprintf_s(commamd, 16, 15, "kill -9 %d", app->pid) > 0, break, "sprintf command unsuccess");
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
        HybridSpawnListenCloseSet();
    } while (0);
    UpdateMountFunc(nullptr);
}

/**
 * @brief 测试子进程退出时，hybridspawn通过正常的fd（pipe写端）中写入子进程退出状态，发送成功
 *
 */
HWTEST_F(HybridSpawnServiceTest, Hybrid_Spawn_SignalFd_02, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnResult result = {};
    MountFunc mountFunc = [](const char *, const char *, const char *, unsigned long, const void *) -> int {
        return 0;
    };
    UpdateMountFunc(mountFunc);
    do {
        int pipefd[2]; // 2 pipe fd
        char buffer[1024]; // 1024 1k
        APPSPAWN_CHECK(pipe(pipefd) == 0, break, "Failed to pipe fd errno:%{public}d", errno);
        ret = HybridSpawnListenFdSet(pipefd[1]);

        ret = AppSpawnClientInit(NATIVESPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", HYBRIDSPAWN_SERVER_NAME);
        AppSpawnReqMsgHandle reqHandle = g_spawningTestHelper.SpawningTestCreateSendMsg(clientHandle, MSG_APP_SPAWN, 0);

        ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
        APPSPAWN_CHECK(ret == 0, break, "Failed to send msg %{public}d", ret);
        AppSpawnClientDestroy(clientHandle);
        sleep(1); // wait child process stand up

        AppSpawnedProcess *app = GetSpawnedProcessByName(g_spawningTestHelper.GetDefaultTestAppBundleName());
        ASSERT_NE(app, nullptr);
        char commamd[16]; // command len 16
        APPSPAWN_CHECK(snprintf_s(commamd, 16, 15, "kill -9 %d", app->pid) > 0, break, "sprintf command unsuccess");
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
        HybridSpawnListenCloseSet();
    } while (0);
    UpdateMountFunc(nullptr);
}
}  // namespace OHOS
