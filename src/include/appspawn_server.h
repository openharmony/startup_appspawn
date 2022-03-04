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

#ifndef APPSPAWN_SERVER_H
#define APPSPAWN_SERVER_H

#include <queue>
#include <string>
#include <map>

#include "appspawn_msg_peer.h"
#include "server_socket.h"
#include "nocopyable.h"

namespace OHOS {
namespace AppSpawn {
class AppSpawnServer {
public:
    /**
     * Constructor used to delete the default constructor.
     */
    AppSpawnServer() = delete;

    /**
     * Constructor used to create a AppSpawnServer.
     */
    explicit AppSpawnServer(const std::string &socketName);

    /**
     * Destructor used to destroy a AppSpawnServer
     */
    ~AppSpawnServer() = default;

    /**
     * Disables copying and moving for the AppSpawnServer.
     */
    DISALLOW_COPY_AND_MOVE(AppSpawnServer);

    /**
     * Provides the AppSpawn core function for the server to receive messages from ability manager service.
     *
     * @param longProcName Indicates the long process name.
     * @param longProcNameLen Indicates the length of long process name.
     */
    bool ServerMain(char *longProcName, int64_t longProcNameLen);

    /**
     * Controls the server listening socket.
     *
     * @param isRunning Indicates whether the server is running. Value false means to stop the server and exit.
     */
    void SetRunning(bool isRunning);

    /**
     * Set its value to the member variable socket_.
     *
     * @param serverSocket Indicates the server socket.
     */
    void SetServerSocket(const std::shared_ptr<ServerSocket> &serverSocket);

    int AppColdStart(char *longProcName,
        int64_t longProcNameLen, const ClientSocket::AppProperty *appProperty, int fd);
private:
    int DoColdStartApp(ClientSocket::AppProperty *appProperty, int fd);

    static constexpr uint8_t BITLEN32 = 32;
    static constexpr uint8_t FDLEN2 = 2;
    static constexpr uint8_t FD_INIT_VALUE = 0;

    /**
     * Use the MsgPeer method in the AppSpawnMsgPeer class to read message from socket, and notify the ServerMain to
     * unlock.
     *
     * @param connectFd Indicates the connect FDs.
     */
    void MsgPeer(int connectFd);

    /**
     * Gets the connect fd and creates a thread to receive messages.
     */
    void ConnectionPeer();

    /**
     * Sets a name for an application process.
     *
     * @param longProcName Indicates the length of long process name.
     * @param longProcNameLen Indicates the long process name.
     * @param processName Indicates the process name from the ability manager service.
     * @param len Indicates the size of processName.
     */
    int32_t SetProcessName(char *longProcName, int64_t longProcNameLen, const char *processName, int32_t len);

    /**
     * Sets keep capabilities.
     */
    int32_t SetKeepCapabilities(uint32_t uid);

    /**
     * Sets the uid and gid of an application process.
     *
     * @param uid Indicates the uid of the application process.
     * @param gid Indicates the gid of the application process.
     * @param gidTable Indicates an array of application processes.
     * @param gidCount Indicates the number of GIDs.
     */
    int32_t SetUidGid(const uint32_t uid, const uint32_t gid, const uint32_t *gidTable, const uint32_t gidCount);

    /**
     * Sets FDs in an application process.
     */
    int32_t SetFileDescriptors();

    /**
     * Sets capabilities of an application process.
     */
    int32_t SetCapabilities();

    /**
     * Create sandbox root folder file
     */
    int32_t DoSandboxRootFolderCreate(std::string sandboxPackagePath);

    /**
     * Create sandbox root folder file with wargnar device
     */
    int32_t DoSandboxRootFolderCreateAdapt(std::string sandboxPackagePath);

    /**
     * Do app sandbox original path mount common
     */
    int32_t DoAppSandboxMountOnce(const std::string originPath, const std::string destinationPath);

    /**
     * Do app sandbox original path mount
     */
    int32_t DoAppSandboxMount(const ClientSocket::AppProperty *appProperty, std::string rootPath);

    /**
     * Do app sandbox original path mount for some customized packages
     */
    int32_t DoAppSandboxMountCustomized(const ClientSocket::AppProperty *appProperty, std::string rootPath);

    /**
     * Do app sandbox mkdir /mnt/sandbox/<packagename>/
     */
    void DoAppSandboxMkdir(std::string rootPath, const ClientSocket::AppProperty *appProperty);

    /**
     * Sets app sandbox property.
     */
    int32_t SetAppSandboxProperty(const ClientSocket::AppProperty *appProperty);

    /**
     * Sets app process property.
     */
    bool SetAppProcProperty(const ClientSocket::AppProperty *appProperty, char *longProcName,
        int64_t longProcNameLen, const int32_t fd);

    /**
     * Notify
     */
    void NotifyResToParentProc(const int32_t fd, const int32_t value);

    /**
     * Special app process property.
     */
    void SpecialHandle(ClientSocket::AppProperty *appProperty);

    /**
     * Check app process property.
     */
    bool CheckAppProperty(const ClientSocket::AppProperty *appProperty);

    void LoadAceLib();

    void SetAppAccessToken(const ClientSocket::AppProperty *appProperty);

    int StartApp(char *longProcName, int64_t longProcNameLen,
        ClientSocket::AppProperty *appProperty, int connectFd, pid_t &pid);

    void WaitRebootEvent();

    void HandleSignal();
private:
    const std::string deviceNull_ = "/dev/null";
    std::string socketName_ {};
    std::shared_ptr<ServerSocket> socket_ = nullptr;
    std::mutex mut_ {};
    mutable std::condition_variable dataCond_ {};
    std::queue<std::unique_ptr<AppSpawnMsgPeer>> appQueue_ {};
    std::function<int(const ClientSocket::AppProperty &)> propertyHandler_ = nullptr;
    std::function<void(const std::string &)> errHandlerHook_ = nullptr;
    bool isRunning_ {};
    bool isStop_ { false };
    bool isChildDie_ { false };
    pid_t childPid_ {};
    std::map<pid_t, std::string> appMap_;
#ifdef NWEB_SPAWN
    void *nwebHandle = nullptr;
#endif
};
}  // namespace AppSpawn
}  // namespace OHOS

#endif
