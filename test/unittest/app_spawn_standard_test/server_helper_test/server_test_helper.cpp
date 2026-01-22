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

#include "server_test_helper.h"

#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/signalfd.h>
#include <csignal>

#include "securec.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn_service.h"
#include "appspawn_modulemgr.h"
#include "appspawn_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

APPSPAWN_STATIC void ProcessSignal(const struct signalfd_siginfo *siginfo);

#ifdef __cplusplus
}
#endif

namespace OHOS {
const uint32_t ServerTestHelper::defaultProtectTime = 60000; // 60000 60s
uint32_t ServerTestHelper::serverId = 0;

int ServerTestHelper::TestChildLoopRun(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGV("ChildLoopRun ...");
    AppSpawningCtx *property = (AppSpawningCtx *)client;
    APPSPAWN_CHECK(content != NULL && property != NULL, return 0, "invalid param in clearEnv");
    int fd = property->forkCtx.fd[1];
    property->forkCtx.fd[1] = -1;
    APPSPAWN_CHECK(fd >= 0, return 0, "invalid fd for notify parent");
    int ret = 0;
    (void)write(fd, &ret, sizeof(ret));
    (void)close(fd);
    sleep(1);
    return 0;
}

CmdArgs *ServerTestHelper::TestParseCmdList(const char *cmd)
{
    const uint32_t maxArgc = 20;
    const uint32_t length = sizeof(CmdArgs) + maxArgc * sizeof(char *) + strlen(cmd) + APP_LEN_PROC_NAME + 1 + 2;
    char *buffer = static_cast<char *>(malloc(length));
    CmdArgs *args = reinterpret_cast<CmdArgs *>(buffer);
    APPSPAWN_CHECK(buffer != nullptr, return nullptr, "Failed to alloc args");
    (void)memset_s(args, length, 0, length);
    char *start = buffer + sizeof(CmdArgs) + maxArgc * sizeof(char *);
    char *end = buffer + length;
    uint32_t index = 0;
    char *curr = const_cast<char *>(cmd);
    while (isspace(*curr)) {
        curr++;
    }

    while (index < (maxArgc - 1) && *curr != '\0') {
        if (args->argv[index] == nullptr) {
            args->argv[index] = start;
        }
        *start = *curr;
        if (isspace(*curr)) {
            *start = '\0';
            // 为SetProcessName 预留空间
            start = (index == 0) ? start + APP_LEN_PROC_NAME : start + 1;
            while (isspace(*curr) && *curr != '\0') {
                curr++;
            }
            if (*curr != '\0') {
                index++;
            }
        } else {
            start++;
            curr++;
        }
    }

    index++;
    args->argv[index] = end - 2;  // 2 last
    args->argv[index][0] = '#';
    args->argv[index][1] = '\0';
    args->argc = index + 1;
    return args;
}

AppSpawnContent *ServerTestHelper::TestStartSpawnServer(std::string &cmd, CmdArgs *&args)
{
    args = TestParseCmdList(cmd.c_str());
    APPSPAWN_CHECK(args != nullptr, return nullptr, "Failed to alloc args");

    AppSpawnStartArg startRrg = {};
    startRrg.mode = MODE_FOR_APP_SPAWN;
    startRrg.socketName = APPSPAWN_SOCKET_NAME;
    startRrg.serviceName = APPSPAWN_SERVER_NAME;
    startRrg.moduleType = MODULE_APPSPAWN;
    startRrg.initArg = 1;
    if (args->argc <= MODE_VALUE_INDEX) {  // appspawn start
        startRrg.mode = MODE_FOR_APP_SPAWN;
    } else if (strcmp(args->argv[MODE_VALUE_INDEX], "app_cold") == 0) {  // app cold start
        APPSPAWN_CHECK(args->argc >= ARG_NULL, free(args);
            return nullptr, "Invalid arg for cold start %{public}d", args->argc);
        startRrg.mode = MODE_FOR_APP_COLD_RUN;
        startRrg.initArg = 0;
    } else if (strcmp(args->argv[MODE_VALUE_INDEX], "nweb_cold") == 0) {  // nweb cold start
        APPSPAWN_CHECK(args->argc >= ARG_NULL, free(args);
            return nullptr, "Invalid arg for cold start %{public}d", args->argc);
        startRrg.mode = MODE_FOR_NWEB_COLD_RUN;
        startRrg.serviceName = NWEBSPAWN_SERVER_NAME;
        startRrg.initArg = 0;
    } else if (strcmp(args->argv[MODE_VALUE_INDEX], NWEBSPAWN_SERVER_NAME) == 0) {  // nweb spawn start
        startRrg.mode = MODE_FOR_NWEB_SPAWN;
        startRrg.moduleType = MODULE_NWEBSPAWN;
        startRrg.socketName = NWEBSPAWN_SOCKET_NAME;
        startRrg.serviceName = NWEBSPAWN_SERVER_NAME;
    } else if (strcmp(args->argv[MODE_VALUE_INDEX], HYBRIDSPAWN_SERVER_NAME) == 0) {  // hybrid spawn start
        startRrg.mode = MODE_FOR_HYBRID_SPAWN;
        startRrg.moduleType = MODULE_HYBRIDSPAWN;
        startRrg.socketName = HYBRIDSPAWN_SOCKET_NAME;
        startRrg.serviceName = HYBRIDSPAWN_SERVER_NAME;
    }
    APPSPAWN_LOGV("Start service %{public}s", startRrg.serviceName);
    // 起一个spawner服务
    AppSpawnContent *content = StartSpawnService(&startRrg, APP_LEN_PROC_NAME, args->argc, args->argv);
    if (content == nullptr) {
        free(args);
    }
    return content;
}

void ServerTestHelper::CloseCheckHandler(void)
{
    APPSPAWN_LOGV("CloseCheckHandler");
    if (timer_ != nullptr) {
        LE_StopTimer(LE_GetDefaultLoop(), timer_);
        timer_ = nullptr;
    }
}

void ServerTestHelper::StartCheckHandler(void)
{
    int ret = LE_CreateTimer(LE_GetDefaultLoop(), &timer_, ProcessIdle, this);
    if (ret == 0) {
        ret = LE_StartTimer(LE_GetDefaultLoop(), timer_, 100, 10000000);  // 100 10000000 repeat
    }
}

void ServerTestHelper::StopSpawnService(void)
{
    APPSPAWN_LOGV("StopSpawnService ");
    if (serverStoped_) {
        CloseCheckHandler();
    }
    serverStoped_ = true;
    if (testServer_) {
        struct signalfd_siginfo siginfo = {};
        siginfo.ssi_signo = SIGTERM;
        siginfo.ssi_uid = 0;
        ProcessSignal(&siginfo);
    } else {
        localServer_->Stop();
    }
}


void ServerTestHelper::ProcessIdle(const TimerHandle taskHandle, void *context)
{
    APPSPAWN_LOGV("ServerTestHelper::ProcessIdle");
    ServerTestHelper *server = reinterpret_cast<ServerTestHelper *>(const_cast<void *>(context));
    if (server->stop_) {
        server->StopSpawnService();
        return;
    }

    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &end);
    uint64_t diff = DiffTime(&server->startTime_, &end);
    if (diff >= (server->protectTime_ * 1000)) {  // 1000 ms -> us
        APPSPAWN_LOGV("ServerTestHelper:: timeout %{public}u %{public}" PRIu64 "", server->protectTime_, diff);
        server->StopSpawnService();
        return;
    }
}

void ServerTestHelper::ServiceThread()
{
    CmdArgs *args = nullptr;
    pid_t pid = getpid();
    APPSPAWN_LOGV("ServerTestHelper::ServiceThread %{public}s", serviceCmd_.c_str());

    running_ = true;
    // 测试server时，使用appspawn的server
    if (testServer_) {
        content_ = ServerTestHelper::TestStartSpawnServer(serviceCmd_, args);
        if (content_ == nullptr) {
            return;
        }
        if (pid == getpid()) {  // 主进程进行处理
            APPSPAWN_LOGV("Service start timer %{public}s ", serviceCmd_.c_str());
            StartCheckHandler();
            AppSpawnMgr *content = reinterpret_cast<AppSpawnMgr *>(content_);
            APPSPAWN_CHECK_ONLY_EXPER(content != nullptr, return);
            // register
            RegChildLooper(&content->content, TestChildLoopRun);
        }
        content_->runAppSpawn(content_, args->argc, args->argv);
        if (pid != getpid()) {  // 子进程退出
            exit(0);
        } else {
            content_ = nullptr;
        }
    } else {
        StartCheckHandler();
        localServer_ = new LocalServerTest();
        localServer_->Run(APPSPAWN_SOCKET_NAME, recvMsgProcess_);
    }
    APPSPAWN_LOGV("ServerTestHelper::ServiceThread finish %{public}s ", serviceCmd_.c_str());
    if (args) {
        free(args);
    }
    return;
}

static void *ServiceHelperThread(void *arg)
{
    ServerTestHelper *server = reinterpret_cast<ServerTestHelper *>(arg);
    APPSPAWN_LOGV("ServerTestHelper::thread ");
    server->ServiceThread();
    return nullptr;
}

void ServerTestHelper::Start(RecvMsgProcess process, uint32_t time)
{
    APPSPAWN_LOGV("ServerTestHelper::Start serverId %{public}u", ServerTestHelper::serverId);
    protectTime_ = time;
    uint32_t retry = 0;
    if (threadId_ != 0) {
        return;
    }
    clock_gettime(CLOCK_MONOTONIC, &startTime_);
    recvMsgProcess_ = process;
    errno = 0;
    int ret = 0;
    do {
        threadId_ = 0;
        ret = pthread_create(&threadId_, nullptr, ServiceHelperThread, static_cast<void *>(this));
        if (ret == 0) {
            break;
        }
        APPSPAWN_LOGE("ServerTestHelper::Start create thread fail %{public}d %{public}d", ret, errno);
        usleep(20000); // 20000 20ms
        retry++;
    } while (ret == EAGAIN && retry < 10); // 10 max retry

    // wait server thread run
    retry = 0;
    while (!running_ && (retry < 10)) { // 10 max retry
        usleep(20000); // 20000 20ms
        retry++;
    }
    APPSPAWN_LOGV("ServerTestHelper::Start retry %{public}u", retry);
}

void ServerTestHelper::Stop()
{
    APPSPAWN_LOGV("ServerTestHelper::Stop serverId %{public}u", ServerTestHelper::serverId);
    if (threadId_ != 0) {
        stop_ = true;
        pthread_join(threadId_, nullptr);
        threadId_ = 0;
        APPSPAWN_LOGV("Stop");
    }
}
}  // namespace OHOS