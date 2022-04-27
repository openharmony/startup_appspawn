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

#ifndef APPSPAWN_SOCKET_CLIENT_H
#define APPSPAWN_SOCKET_CLIENT_H

#include "appspawn_socket.h"
#include "appspawn_msg.h"
#include "nocopyable.h"

namespace OHOS {
namespace AppSpawn {
class ClientSocket : public AppSpawnSocket {
public:
    /**
     * Constructor used to create a ClientSocket
     */
    explicit ClientSocket(const std::string &client);

    /**
     * Destructor used to destroy a ClientSocket
     */
    virtual ~ClientSocket() = default;

    /**
     * Disables copying and moving for the ClientSocket.
     */
    DISALLOW_COPY_AND_MOVE(ClientSocket);

    /**
     * Creates a local client socket.
     */
    virtual int CreateClient();

    /**
     * Closes a client socket.
     */
    virtual void CloseClient();

    /**
     * Connects a client socket.
     */
    virtual int ConnectSocket();

    /**
     * Writes messages to a client socket.
     *
     * @param buf Indicates the pointer to the message buffer.
     * @param len Indicates the message length.
     */
    virtual int WriteSocketMessage(const void *buf, int len);

    /**
     * Reads messages from a client socket.
     *
     * @param buf Indicates the pointer to the message buffer.
     * @param len Indicates the message length.
     */
    virtual int ReadSocketMessage(void *buf, int len);

    /**
     * Uses functions of the parent class.
     */
    using AppSpawnSocket::CloseSocket;
    using AppSpawnSocket::CreateSocket;
    using AppSpawnSocket::GetSocketFd;
    using AppSpawnSocket::ReadSocketMessage;
    using AppSpawnSocket::WriteSocketMessage;

    enum AppType {
        APP_TYPE_DEFAULT = 0,  // JavaScript app
        APP_TYPE_NATIVE        // Native C++ app
    };

    static constexpr int APPSPAWN_MSG_MAX_SIZE = APP_MSG_MAX_SIZE;  // appspawn message max size
    static constexpr int LEN_PROC_NAME = APP_LEN_PROC_NAME;         // process name length
    static constexpr int LEN_BUNDLE_NAME = APP_LEN_BUNDLE_NAME;     // bundle name length
    static constexpr int LEN_SO_PATH = APP_LEN_SO_PATH;             // load so lib
    static constexpr int MAX_GIDS = APP_MAX_GIDS;
    static constexpr int APL_MAX_LEN = APP_APL_MAX_LEN;
    static constexpr int RENDER_CMD_MAX_LEN = APP_RENDER_CMD_MAX_LEN;
    static constexpr int APPSPAWN_COLD_BOOT = APP_COLD_BOOT;

    using AppProperty = AppParameter;
    using AppOperateCode = AppOperateType;
private:
    /**
     * Connects a client socket.
     *
     * @param connectFd Indicates the connection's FD.
     */
    int ConnectSocket(int connectFd);
};
}  // namespace AppSpawn
}  // namespace OHOS
#endif
