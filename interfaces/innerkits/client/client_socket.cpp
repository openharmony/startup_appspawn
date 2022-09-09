/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include <sys/socket.h>
#include <cerrno>

#include "appspawn_server.h"
#include "parameters.h"

namespace OHOS {
#ifdef APPSPAWN_ASAN
static constexpr int TIMEOUT_DEF = 5;
#else
static constexpr int TIMEOUT_DEF = 2;
#endif
namespace AppSpawn {
ClientSocket::ClientSocket(const std::string &client) : AppSpawnSocket(client)
{}

int ClientSocket::CreateClient()
{
    if (socketFd_ < 0) {
        socketFd_ = CreateSocket();
        APPSPAWN_CHECK(socketFd_ >= 0, return socketFd_, "Client: Create socket failed");
    }

    int opt = 1;
    int ret = setsockopt(socketFd_, SOL_SOCKET, SO_PASSCRED, &opt, sizeof(opt));
    if (ret < 0) {
        APPSPAWN_LOGE("Client: setsockopt failed!");
        return -1;
    }

    APPSPAWN_LOGV("Client: CreateClient socket fd %d", socketFd_);
    return 0;
}

void ClientSocket::CloseClient()
{
    if (socketFd_ < 0) {
        APPSPAWN_LOGE("Client: Invalid connectFd %d", socketFd_);
        return;
    }

    CloseSocket(socketFd_);
    socketFd_ = -1;
}

int ClientSocket::ConnectSocket(int connectFd)
{
    if (connectFd < 0) {
        APPSPAWN_LOGE("Client: Invalid socket fd: %d", connectFd);
        return -1;
    }

    APPSPAWN_CHECK(PackSocketAddr() == 0, return -1,  "pack socket failed");

    struct timeval timeout = {TIMEOUT_DEF, 0};
    int value = OHOS::system::GetIntParameter("persist.appspawn.client.timeout", TIMEOUT_DEF);
    if (value != TIMEOUT_DEF && value != 0) {
        timeout.tv_sec = value;
    }
    APPSPAWN_LOGI("Client: Connected on socket fd %d value %d", connectFd, value);
    bool isRet = (setsockopt(connectFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) != 0) ||
        (setsockopt(connectFd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0);
    APPSPAWN_CHECK(!isRet, return (-1), "Client: Failed to set opt of socket %d, err %d", connectFd, errno);

    if (connect(connectFd, reinterpret_cast<struct sockaddr *>(&socketAddr_), socketAddrLen_) < 0) {
        APPSPAWN_LOGW("Client: Connect on socket fd %d, failed: %d", connectFd, errno);
        return -1;
    }
    return 0;
}

int ClientSocket::ConnectSocket()
{
    int ret = ConnectSocket(socketFd_);
    if (ret != 0) {
        CloseClient();
    }
    return ret;
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
