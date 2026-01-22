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

#ifndef LOCAL_SERVER_TEST_H
#define LOCAL_SERVER_TEST_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <functional>

#include "loop_event.h"
#include "appspawn_msg.h"

namespace OHOS {

using RecvMsgProcess = std::function<void(struct TestConnection *connection, const uint8_t *buffer,
                                          uint32_t bufferLen)>;

struct TestConnection {
    uint32_t connectionId;
    TaskHandle stream;
    uint32_t msgRecvLen;  // 已经接收的长度
    AppSpawnMsg msg;      // 保存不完整的消息，额外保存消息头信息
    uint8_t *buffer = nullptr;
    RecvMsgProcess recvMsgProcess = nullptr;
    int SendResponse(const AppSpawnMsg *msg, int result, pid_t pid);
};

class LocalServerTest {
public:
    LocalServerTest() {}
    ~LocalServerTest() {}

    int Run(const char *serverName, RecvMsgProcess recvMsg);
    void Stop();

private:
    using ServerInfo = struct ServerInfo_ {
        LocalServerTest *local = nullptr;
        RecvMsgProcess recvMsgProcess = nullptr;
    };

    static void OnClose(const TaskHandle taskHandle);
    static void SendMessageComplete(const TaskHandle taskHandle, BufferHandle handle);
    static void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen);
    static int OnConnection(const LoopHandle loopHandle, const TaskHandle server);
    static int HandleRecvMessage(const TaskHandle taskHandle, uint8_t *buffer, int bufferSize, int flags);
    TaskHandle serverHandle_ = 0;
};
}  // namespace OHOS
#endif  // LOCAL_SERVER_TEST_H