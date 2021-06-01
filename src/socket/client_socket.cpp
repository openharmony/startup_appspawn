/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "client_socket.h"

#include <cerrno>
#include <iostream>
#include <sys/socket.h>

#include "hilog/log.h"
#include "securec.h"

namespace OHOS {
namespace AppSpawn {
using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0, "ClientSocket"};

ClientSocket::ClientSocket(const std::string &client) : AppSpawnSocket(client)
{}

int ClientSocket::CreateClient()
{
    if (socketFd_ < 0) {
        socketFd_ = CreateSocket();
        if (socketFd_ < 0) {
            HiLog::Error(LABEL, "Client: Create socket failed");
            return -1;
        }
    }

    HiLog::Debug(LABEL, "Client: CreateClient socket fd %d", socketFd_);
    return 0;
}

void ClientSocket::CloseClient()
{
    if (socketFd_ < 0) {
        HiLog::Error(LABEL, "Client: Invalid connectFd %d", socketFd_);
        return;
    }

    CloseSocket(socketFd_);
    socketFd_ = -1;
}

int ClientSocket::ConnectSocket(int connectFd)
{
    if (connectFd < 0) {
        HiLog::Error(LABEL, "Client: Invalid socket fd: %d", connectFd);
        return -1;
    }

    if (PackSocketAddr() != 0) {
        return -1;
    }

    if ((setsockopt(connectFd, SOL_SOCKET, SO_RCVTIMEO, &SOCKET_TIMEOUT, sizeof(SOCKET_TIMEOUT)) != 0) ||
        (setsockopt(connectFd, SOL_SOCKET, SO_SNDTIMEO, &SOCKET_TIMEOUT, sizeof(SOCKET_TIMEOUT)) != 0)) {
        HiLog::Warn(LABEL, "Client: Failed to set opt of socket %d, err %s", connectFd, strerror(errno));
    }

    if (connect(connectFd, reinterpret_cast<struct sockaddr *>(&socketAddr_), socketAddrLen_) < 0) {
        HiLog::Warn(LABEL, "Client: Connect on socket fd %d, failed: %s", connectFd, strerror(errno));
        CloseSocket(connectFd);
        return -1;
    }

    HiLog::Debug(LABEL, "Client: Connected on socket fd %d, name '%s'", connectFd, socketAddr_.sun_path);
    return 0;
}

int ClientSocket::ConnectSocket()
{
    return ConnectSocket(socketFd_);
}

int ClientSocket::WriteSocketMessage(const void *buf, int len)
{
    return WriteSocketMessage(socketFd_, buf, len);
}

int ClientSocket::ReadSocketMessage(void *buf, int len)
{
    return ReadSocketMessage(socketFd_, buf, len);
}
}  // namespace AppSpawn
}  // namespace OHOS
