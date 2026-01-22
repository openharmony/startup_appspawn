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

#ifndef SERVER_TEST_HELPER_H
#define SERVER_TEST_HELPER_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <functional>
#include <atomic>

#include "loop_event.h"
#include "appspawn_server.h"

#include "local_server_test.h"

namespace OHOS {
typedef struct {
    int argc;
    char *argv[0];
} CmdArgs;

class ServerTestHelper {
public:
    explicit ServerTestHelper(const char *cmd)
        : serviceCmd_(cmd), testServer_(true), protectTime_(defaultProtectTime)
    {
        serverId_ = ServerTestHelper::serverId;
        ServerTestHelper::serverId++;
    }
    ~ServerTestHelper() {}

    void Start(RecvMsgProcess process, uint32_t time = defaultProtectTime);  // 创建线程
    void Stop();                                                             // 关闭线程

private:
    void CloseCheckHandler(void);    // 关闭线程
    void StartCheckHandler(void);    // 保活线程

    static void ProcessIdle(const TimerHandle taskHandle, void *context);
    static int TestChildLoopRun(AppSpawnContent *content, AppSpawnClient *client);
    static CmdArgs *TestParseCmdList(const char *cmd);
    static AppSpawnContent *TestStartSpawnServer(std::string &cmd, CmdArgs *&args);
    void StopSpawnService(void);
    void ServiceThread();

    static const uint32_t defaultProtectTime;
    static uint32_t serverId;

    std::string serviceCmd_{};
    bool running_ = false;
    bool testServer_ = false;
    bool serverStoped_ = false;
    uint32_t protectTime_ = 0;
    uint32_t serverId_ = 0;
    pthread_t threadId_ = 0;
    std::atomic<bool> stop_{false};
    struct timespec startTime_{};
    RecvMsgProcess recvMsgProcess_ = nullptr;
    AppSpawnContent *content_ = nullptr;
    TimerHandle timer_ = nullptr;
    LocalServerTest *localServer_ = nullptr;
};
}  // namespace OHOS
#endif   // SERVER_TEST_HELPER_H