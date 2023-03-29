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

#include "appspawn_socket.h"

#include <sys/socket.h>
#include <cerrno>

#include "appspawn_server.h"
#include "pubdef.h"
#include "securec.h"

namespace OHOS {
namespace AppSpawn {
AppSpawnSocket::AppSpawnSocket(const std::string &name)
{
    socketName_ = name;
}

AppSpawnSocket::~AppSpawnSocket()
{
    if (socketFd_ > 0) {
        CloseSocket(socketFd_);
        socketFd_ = -1;
    }
}

int AppSpawnSocket::GetSocketFd() const
{
    return socketFd_;
}

int AppSpawnSocket::PackSocketAddr()
{
    APPSPAWN_CHECK(!socketName_.empty(), return -EINVAL, "Invalid socket name: empty");

    APPSPAWN_CHECK(memset_s(&socketAddr_, sizeof(socketAddr_), 0, sizeof(socketAddr_)) == EOK,
        return -1, "Failed to memset socket addr");

    socklen_t pathLen = 0;
    if (socketName_[0] == '/') {
        pathLen = socketName_.length();
    } else {
        pathLen = socketDir_.length() + socketName_.length();
    }
    socklen_t pathSize = sizeof(socketAddr_.sun_path);
    if (pathLen >= pathSize) {
        APPSPAWN_LOGE("Invalid socket name: '%s' too long", socketName_.c_str());
        return -1;
    }

    int len = 0;
    if (socketName_[0] == '/') {
        len = snprintf_s(socketAddr_.sun_path, pathSize, (pathSize - 1), "%s", socketName_.c_str());
    } else {
        len = snprintf_s(socketAddr_.sun_path, pathSize, (pathSize - 1), "%s%s",
            socketDir_.c_str(), socketName_.c_str());
    }
    APPSPAWN_CHECK(static_cast<int>(pathLen) == len, return -1, "Failed to copy socket path");

    socketAddr_.sun_family = AF_LOCAL;
    socketAddrLen_ = offsetof(struct sockaddr_un, sun_path) + pathLen + 1;

    return 0;
}

int AppSpawnSocket::CreateSocket()
{
    int socketFd = socket(AF_UNIX, SOCK_STREAM, 0); // SOCK_SEQPACKET
    APPSPAWN_CHECK(socketFd >= 0, return -errno, "Failed to create socket: %d", errno);

    APPSPAWN_LOGV("Created socket with fd %d", socketFd);
    return socketFd;
}

void AppSpawnSocket::CloseSocket(int &socketFd)
{
    if (socketFd >= 0) {
        APPSPAWN_LOGV("Closed socket with fd %d", socketFd);
        close(socketFd);
        socketFd = -1;
    }
}

int AppSpawnSocket::ReadSocketMessage(int socketFd, void *buf, int len)
{
    if (socketFd < 0 || len <= 0 || buf == nullptr) {
        APPSPAWN_LOGE("Invalid args: socket %d, len %d, buf might be nullptr", socketFd, len);
        return -1;
    }

    APPSPAWN_CHECK(memset_s(buf, len, 0, len) == EOK, return -1, "Failed to memset read buf");

    ssize_t rLen = TEMP_FAILURE_RETRY(read(socketFd, buf, len));
    while ((rLen < 0) && (errno == EAGAIN)) {
        rLen = TEMP_FAILURE_RETRY(read(socketFd, buf, len));
    }
    APPSPAWN_CHECK(rLen >= 0, return -EFAULT, "Read message from fd %d error %zd: %d",
        socketFd, rLen, errno);

    return rLen;
}

int AppSpawnSocket::WriteSocketMessage(int socketFd, const void *buf, int len)
{
    if (socketFd < 0 || len <= 0 || buf == nullptr) {
        APPSPAWN_LOGE("Invalid args: socket %d, len %d, buf might be nullptr", socketFd, len);
        return -1;
    }

    ssize_t written = 0;
    ssize_t remain = static_cast<ssize_t>(len);
    const uint8_t *offset = reinterpret_cast<const uint8_t *>(buf);
    for (ssize_t wLen = 0; remain > 0; offset += wLen, remain -= wLen, written += wLen) {
        wLen = write(socketFd, offset, remain);
        APPSPAWN_LOGV("socket fd %d, wLen %zd", socketFd, wLen);
        bool isRet = (wLen <= 0) && (errno != EINTR);
        APPSPAWN_CHECK(!isRet, return -errno, "Failed to write message to fd %d, error %zd: %d",
            socketFd, wLen, errno);
    }

    return written;
}
}  // namespace AppSpawn
}  // namespace OHOS
