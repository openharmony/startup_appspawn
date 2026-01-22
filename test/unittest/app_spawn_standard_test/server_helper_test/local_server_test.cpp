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

#include "local_server_test.h"

#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#include "securec.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn_service.h"

namespace OHOS {

void LocalServerTest::OnClose(const TaskHandle taskHandle)
{
    TestConnection *connection = (TestConnection *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(connection != nullptr, return, "Invalid connection");
    APPSPAWN_LOGI("OnClose connection.id %{public}d socket %{public}d",
        connection->connectionId, LE_GetSocketFd(taskHandle));

    AppSpawnConnection *spawnConnection = (AppSpawnConnection *)LE_GetUserData(taskHandle);
    if (spawnConnection != nullptr) {
        int fdCount = spawnConnection->receiverCtx.fdCount;
        for (int i = 0; i < fdCount; i++) {
            APPSPAWN_LOGI("OnClose close fd %d", spawnConnection->receiverCtx.fds[i]);
            if (spawnConnection->receiverCtx.fds[i] >= 0) {
                close(spawnConnection->receiverCtx.fds[i]);
            }
        }
    }
}

void LocalServerTest::SendMessageComplete(const TaskHandle taskHandle, BufferHandle handle)
{
    return;
}

void LocalServerTest::OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen)
{
    TestConnection *connection = (TestConnection *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(connection != nullptr, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
        return, "Failed to get client form socket");

    if (connection->recvMsgProcess) {
        connection->recvMsgProcess(connection, buffer, buffLen);
    }
}

int LocalServerTest::OnConnection(const LoopHandle loopHandle, const TaskHandle server)
{
    static uint32_t connectionId = 0;
    TaskHandle stream;
    LE_StreamInfo info = {};
    info.baseInfo.flags = TASK_STREAM | TASK_PIPE | TASK_CONNECT;
    info.baseInfo.close = OnClose;
    info.baseInfo.userDataSize = sizeof(TestConnection);
    info.disConnectComplete = nullptr;
    info.sendMessageComplete = SendMessageComplete;
    info.recvMessage = OnReceiveRequest;
    info.handleRecvMsg = HandleRecvMessage;

    ServerInfo *serverInfo = (ServerInfo *)LE_GetUserData(server);
    APPSPAWN_CHECK(serverInfo != nullptr, return -1, "Failed to alloc stream");

    LE_STATUS ret = LE_AcceptStreamClient(loopHandle, server, &stream, &info);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to alloc stream");
    TestConnection *connection = (TestConnection *)LE_GetUserData(stream);
    APPSPAWN_CHECK(connection != nullptr, return -1, "Failed to alloc stream");
    connection->connectionId = ++connectionId;
    connection->stream = stream;
    connection->msgRecvLen = 0;
    (void)memset_s(&connection->msg, sizeof(connection->msg), 0, sizeof(connection->msg));
    connection->buffer = nullptr;
    connection->recvMsgProcess = serverInfo->recvMsgProcess;
    APPSPAWN_LOGI("OnConnection connection.id %{public}d fd %{public}d ",
        connection->connectionId, LE_GetSocketFd(stream));
    return 0;
}

int LocalServerTest::HandleRecvMessage(const TaskHandle taskHandle, uint8_t * buffer, int bufferSize, int flags)
{
    int socketFd = LE_GetSocketFd(taskHandle);
    struct iovec iov = {
        .iov_base = buffer,
        .iov_len = bufferSize,
    };
    char ctrlBuffer[CMSG_SPACE(APP_MAX_FD_COUNT * sizeof(int))];
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = ctrlBuffer,
        .msg_controllen = sizeof(ctrlBuffer),
    };

    AppSpawnConnection *connection = (AppSpawnConnection *) LE_GetUserData(taskHandle);
    errno = 0;
    int recvLen = recvmsg(socketFd, &msg, flags);
    APPSPAWN_CHECK_ONLY_LOG(errno == 0, "recvmsg with errno %d", errno);
    struct cmsghdr *cmsg = nullptr;
    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            int fdCount = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
            int* fd = reinterpret_cast<int*>(CMSG_DATA(cmsg));
            APPSPAWN_CHECK(fdCount <= APP_MAX_FD_COUNT,
                return -1, "failed to recv fd %d %d", connection->receiverCtx.fdCount, fdCount);
            APPSPAWN_CHECK(memcpy_s(connection->receiverCtx.fds, fdCount * sizeof(int), fd,
                fdCount * sizeof(int)) == 0, return -1, "memcpy_s fd failed");
            connection->receiverCtx.fdCount = fdCount;
        }
    }

    return recvLen;
}

int LocalServerTest::Run(const char *socketName, RecvMsgProcess recvMsg)
{
    char path[128] = {0};  // 128 max path
    int ret = snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s%s", APPSPAWN_SOCKET_DIR, socketName);
    APPSPAWN_CHECK(ret >= 0, return -1, "Failed to snprintf_s %{public}d", ret);
    LE_StreamServerInfo info = {};
    info.baseInfo.flags = TASK_STREAM | TASK_PIPE | TASK_SERVER;
    info.baseInfo.userDataSize = sizeof(ServerInfo);
    info.socketId = -1;
    info.server = path;
    info.baseInfo.close = nullptr;
    info.incommingConnect = OnConnection;

    MakeDirRec(path, 0711, 0);  // 0711 default mask
    ret = LE_CreateStreamServer(LE_GetDefaultLoop(), &serverHandle_, &info);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to create socket for %{public}s errno: %{public}d", path, errno);
    APPSPAWN_LOGI("LocalTestServer path %{public}s fd %{public}d", path, LE_GetSocketFd(serverHandle_));

    ServerInfo *serverInfo = (ServerInfo *)LE_GetUserData(serverHandle_);
    APPSPAWN_CHECK(serverInfo != nullptr, return -1, "Failed to alloc stream");
    serverInfo->local = this;
    serverInfo->recvMsgProcess = recvMsg;
    LE_RunLoop(LE_GetDefaultLoop());
    LE_CloseStreamTask(LE_GetDefaultLoop(), serverHandle_);
    LE_StopLoop(LE_GetDefaultLoop());
    LE_CloseLoop(LE_GetDefaultLoop());
    APPSPAWN_LOGI("LocalTestServer exit");
    return 0;
}

void LocalServerTest::Stop()
{
    APPSPAWN_LOGI("Stop LocalTestServer ");
    LE_StopLoop(LE_GetDefaultLoop());
}
}  // namespace OHOS