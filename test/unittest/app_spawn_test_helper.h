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

#ifndef APPSPAWN_TEST_HELPER_H
#define APPSPAWN_TEST_HELPER_H

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <mutex>
#include <pthread.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "appspawn.h"
#include "appspawn_client.h"
#include "appspawn_hook.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "appspawn_utils.h"
#include "list.h"
#include "loop_event.h"

#include "app_spawn_stub.h"

namespace OHOS {
typedef struct {
    int argc;
    char *argv[0];
} CmdArgs;

typedef struct AppSpawnClient AppSpawnClient;
struct TestConnection;
class LocalTestServer;
using RecvMsgProcess = std::function<void(struct TestConnection *connection, const uint8_t *buffer, uint32_t buffLen)>;
using AddTlvFunction = std::function<int(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount)>;

class AppSpawnTestHelper {
public:
    AppSpawnTestHelper()
    {
        SetDefaultTestData();
    }
    ~AppSpawnTestHelper() {}

    void SetDefaultTestData();
    const char *GetDefaultTestAppBundleName()
    {
        return processName_.c_str();
    }
    uid_t GetTestUid()
    {
        return defaultTestUid_;
    }
    gid_t GetTestGid()
    {
        return defaultTestGid_;
    }
    gid_t GetTestGidGroup()
    {
        return defaultTestGidGroup_;
    }
    int32_t GetTestBundleIndex()
    {
        return defaultTestBundleIndex_;
    }

    void SetTestMsgFlags(uint32_t flags)
    {
        defaultMsgFlags_ = flags;
    }
    void SetTestApl(const char *apl)
    {
        defaultApl_ = std::string(apl);
    }
    void SetTestUid(uid_t uid)
    {
        defaultTestUid_ = uid;
    }
    void SetTestGid(gid_t gid)
    {
        defaultTestGid_ = gid;
    }
    void SetProcessName(const char *name)
    {
        processName_ = std::string(name);
    }

    AppSpawnReqMsgHandle CreateMsg(AppSpawnClientHandle handle, uint32_t msgType = MSG_APP_SPAWN, int base = 0);
    AppSpawningCtx *GetAppProperty(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle);

    int CreateSocket(int type = 0);
    int CreateSendMsg(std::vector<uint8_t> &buffer, uint32_t msgType, uint32_t &msgLen,
        const std::vector<AddTlvFunction> &addTlvFuncs);
    const std::vector<const char *> &GetPermissions()
    {
        return permissions_;
    }

    static int AddBaseTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount);
    static uint32_t GenRandom(void);
    static CmdArgs *ToCmdList(const char *cmd);
    static AppSpawnContent *StartSpawnServer(std::string &cmd, CmdArgs *&args);

    int AppSpawnReqMsgSetFlags(AppSpawnReqMsgHandle reqHandle, uint32_t tlv, uint32_t flags)
    {
        AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
        APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
        if (tlv == TLV_MSG_FLAGS) {
            *(uint32_t *)reqNode->msgFlags->flags = flags;
        } else if (tlv == TLV_PERMISSION) {
            *(uint32_t *)reqNode->permissionFlags->flags = flags;
        }
        return 0;
    }
private:
    AppSpawnMsgNode *CreateAppSpawnMsg(AppSpawnMsg *msg);

    std::string processName_ = {};
    std::string defaultApl_ = "system_core";
    uid_t defaultTestUid_;
    gid_t defaultTestGid_;
    gid_t defaultTestGidGroup_;
    int32_t defaultTestBundleIndex_;
    uint32_t defaultMsgFlags_ = 0;
    std::vector<const char *> permissions_ = {
        const_cast<char *>("ohos.permission.READ_IMAGEVIDEO"),
        const_cast<char *>("ohos.permission.FILE_CROSS_APP"),
        const_cast<char *>("ohos.permission.ACTIVATE_THEME_PACKAGE"),
        const_cast<char *>("ohos.permission.GET_WALLPAPER"),
        const_cast<char *>("ohos.permission.ACCESS_DATA"),
        const_cast<char *>("ohos.permission.ACCESS_DEV_FUSE"),
        const_cast<char *>("ohos.permission.FILE_ACCESS_MANAGER")
    };
};

class AppSpawnTestServer : public AppSpawnTestHelper {
public:
    explicit AppSpawnTestServer(const char *cmd, bool testServer)
        : AppSpawnTestHelper(), serviceCmd_(cmd), testServer_(testServer), protectTime_(defaultProtectTime)
    {
        serverId_ = AppSpawnTestServer::serverId;
        AppSpawnTestServer::serverId++;
    }

    explicit AppSpawnTestServer(const char *cmd)
        : AppSpawnTestHelper(), serviceCmd_(cmd), testServer_(true), protectTime_(defaultProtectTime)
    {
        serverId_ = AppSpawnTestServer::serverId;
        AppSpawnTestServer::serverId++;
    }
    ~AppSpawnTestServer();

    void Start(void);
    void Start(RecvMsgProcess process, uint32_t time = defaultProtectTime);
    void Stop();
    void ServiceThread();
    void KillNWebSpawnServer();

    static const uint32_t defaultProtectTime;
private:
    void CloseCheckHandler(void);
    void StartCheckHandler(void);
    void StopSpawnService(void);

    static uint32_t serverId;
#ifdef USER_TIMER_TO_CHECK
    static void ProcessIdle(const TimerHandle taskHandle, void *context);
#else
    static void ProcessIdle(const IdleHandle taskHandle, void *context);
#endif

    AppSpawnContent *content_ = nullptr;
    std::atomic<long> appPid_{-1};
    std::string serviceCmd_{};
    bool running_{false};
#ifdef USER_TIMER_TO_CHECK
    TimerHandle timer_;
#else
    IdleHandle idle_ = nullptr;
#endif
    pthread_t threadId_ = 0;
    std::atomic<bool> stop_{false};
    RecvMsgProcess recvMsgProcess_ = nullptr;
    bool testServer_ = false;
    bool serverStoped = false;
    struct timespec startTime_ {};
    uint32_t protectTime_;
    uint32_t serverId_ = 0;
    LocalTestServer *localServer_ = nullptr;
};

struct TestConnection {
    uint32_t connectionId;
    TaskHandle stream;
    uint32_t msgRecvLen;  // 已经接收的长度
    AppSpawnMsg msg;      // 保存不完整的消息，额外保存消息头信息
    uint8_t *buffer = nullptr;
    RecvMsgProcess recvMsgProcess = nullptr;
    int SendResponse(const AppSpawnMsg *msg, int result, pid_t pid);
};

/**
 * @brief 用于client端测试，构建服务程序
 *
 */
class LocalTestServer {
public:
    LocalTestServer() {}
    ~LocalTestServer() {}

    int Run(const char *serverName, RecvMsgProcess recvMsg);
    void Stop();

private:
    using ServerInfo = struct ServerInfo_ {
        LocalTestServer *local = nullptr;
        RecvMsgProcess recvMsgProcess = nullptr;
    };

    static int OnConnection(const LoopHandle loopHandle, const TaskHandle server);
    static void SendMessageComplete(const TaskHandle taskHandle, BufferHandle handle);
    static void OnClose(const TaskHandle taskHandle);
    static void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen);
    TaskHandle serverHandle_ = 0;
};
}  // namespace OHOS
#endif  // APPSPAWN_TEST_HELPER_H
