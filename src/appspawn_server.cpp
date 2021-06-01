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

#include "appspawn_server.h"

#include <fcntl.h>
#include <memory>
#include <signal.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <thread>

#include "errors.h"
#include "hilog/log.h"
#include "main_thread.h"
#include "securec.h"

#define GRAPHIC_PERMISSION_CHECK

namespace OHOS {
namespace AppSpawn {

namespace {
constexpr int32_t ERR_PIPE_FAIL = -100;
constexpr int32_t MAX_LEN_SHORT_NAME = 16;
constexpr int32_t WAIT_DELAY_US = 100 * 1000;  // 100ms
}  // namespace

using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0, "AppSpawnServer"};

#ifdef __cplusplus
extern "C" {
#endif

static void SignalHandler(int) /* signal_number */
{
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status)) {
            HiLog::Info(LABEL, "Process %{public}d exited cleanly %{public}d", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            if (WTERMSIG(status) != SIGKILL) {
                HiLog::Info(LABEL, "Process %{public}d exited due to signal %{public}d", pid, WTERMSIG(status));
            }
            if (WCOREDUMP(status)) {
                HiLog::Info(LABEL, "Process %{public}d dumped core.", pid);
            }
        }
    }
}

static void InstallSigHandler()
{
    struct sigaction sa = {};
    sa.sa_handler = SignalHandler;
    int err = sigaction(SIGCHLD, &sa, nullptr);
    if (err < 0) {
        HiLog::Error(LABEL, "Error installing SIGCHLD handler: %{public}s", strerror(errno));
        return;
    }

    struct sigaction sah = {};
    sah.sa_handler = SIG_IGN;
    err = sigaction(SIGHUP, &sah, nullptr);
    if (err < 0) {
        HiLog::Error(LABEL, "Error installing SIGHUP handler: %{public}s", strerror(errno));
    }
}

static void UninstallSigHandler()
{
    struct sigaction sa = {};
    sa.sa_handler = SIG_DFL;
    int err = sigaction(SIGCHLD, &sa, nullptr);
    if (err < 0) {
        HiLog::Error(LABEL, "Error uninstalling SIGCHLD handler: %s", strerror(errno));
    }

    struct sigaction sah = {};
    sah.sa_handler = SIG_DFL;
    err = sigaction(SIGHUP, &sah, nullptr);
    if (err < 0) {
        HiLog::Error(LABEL, "Error uninstalling SIGHUP handler: %s", strerror(errno));
    }
}
#ifdef __cplusplus
}
#endif

AppSpawnServer::AppSpawnServer(const std::string &socketName)
{
    socketName_ = socketName;
    socket_ = std::make_shared<ServerSocket>(socketName_);
    isRunning_ = true;
}

void AppSpawnServer::MsgPeer(int connectFd)
{
    std::unique_ptr<AppSpawnMsgPeer> msgPeer = std::make_unique<AppSpawnMsgPeer>(socket_, connectFd);
    if (msgPeer == nullptr || msgPeer->MsgPeer() != 0) {
        HiLog::Error(LABEL, "Failed to listen connection %d, %s", connectFd, strerror(errno));
        return;
    }

    std::lock_guard<std::mutex> lock(mut_);
    appQueue_.push(std::move(msgPeer));
    dataCond_.notify_one();
}

void AppSpawnServer::ConnectionPeer()
{
    int connectFd;

    /* AppSpawn keeps receiving msg from AppMgr and never exits */
    while (isRunning_) {
        connectFd = socket_->WaitForConnection();
        if (connectFd < 0) {
            usleep(WAIT_DELAY_US);
            HiLog::Info(LABEL, "AppSpawnServer::ConnectionPeer connectFd is %{public}d", connectFd);
            continue;
        }

        mut_.lock();  // Ensure that mutex in SaveConnection is unlocked before being forked
        socket_->SaveConnection(connectFd);
        mut_.unlock();
        std::thread(&AppSpawnServer::MsgPeer, this, connectFd).detach();
    }
}

bool AppSpawnServer::ServerMain(char *longProcName, int64_t longProcNameLen)
{
    if (socket_->RegisterServerSocket() != 0) {
        HiLog::Error(LABEL, "AppSpawnServer::Failed to register server socket");
        return false;
    }

    std::thread(&AppSpawnServer::ConnectionPeer, this).detach();

    while (isRunning_) {
        std::unique_lock<std::mutex> lock(mut_);
        dataCond_.wait(lock, [this] { return !this->appQueue_.empty(); });
        std::unique_ptr<AppSpawnMsgPeer> msg = std::move(appQueue_.front());
        appQueue_.pop();
        int connectFd = msg->GetConnectFd();
        ClientSocket::AppProperty *appProperty = msg->GetMsg();

        // check appProperty
        if (appProperty == nullptr) {
            HiLog::Error(LABEL, "appProperty is nullptr");
            continue;
        }

        if (appProperty->gidCount > ClientSocket::MAX_GIDS) {
            HiLog::Error(LABEL, "gidCount error: %{public}u", appProperty->gidCount);
            msg->Response(-EINVAL);
            continue;
        }

        if (strlen(appProperty->processName) == 0) {
            HiLog::Error(LABEL, "process name length is 0");
            msg->Response(-EINVAL);
            continue;
        }

        InstallSigHandler();

        int32_t fd[FDLEN2] = {FD_INIT_VALUE, FD_INIT_VALUE};
        int32_t buff = 0;
        if (pipe(fd) == -1) {
            HiLog::Error(LABEL, "create pipe fail");
            msg->Response(ERR_PIPE_FAIL);
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            HiLog::Error(LABEL, "AppSpawnServer::Failed to fork new process, errno = %{public}d", errno);
            // close pipe fds
            close(fd[0]);
            close(fd[1]);
            msg->Response(-errno);
            continue;
        }

        if (pid == 0) {
            return SetAppProcProperty(connectFd, appProperty, longProcName, longProcNameLen, fd);
        }
        // parent process
        // close write end
        close(fd[1]);
        // wait child process result
        read(fd[0], &buff, sizeof(buff));
        // close read end
        close(fd[0]);

        HiLog::Info(LABEL, "child process init %{public}s", (buff == ERR_OK) ? "success" : "fail");

        if (buff == ERR_OK) {
            // send pid to AppManagerService
            msg->Response(pid);
        } else {
            // send error code to AppManagerService
            msg->Response(buff);
        }

        // close socket connection
        socket_->CloseConnection(connectFd);
        HiLog::Debug(LABEL, "AppSpawnServer::parent process create app finish, pid = %{public}d", pid);
    }

    return false;
}

int32_t AppSpawnServer::SetProcessName(
    char *longProcName, int64_t longProcNameLen, const char *processName, int32_t len)
{
    if (longProcName == nullptr || processName == nullptr || len <= 0) {
        HiLog::Error(LABEL, "process name is nullptr or length error");
        return -EINVAL;
    }

    char shortName[MAX_LEN_SHORT_NAME];
    if (memset_s(shortName, sizeof(shortName), 0, sizeof(shortName)) != EOK) {
        HiLog::Error(LABEL, "Failed to memset short name");
        return -EINVAL;
    }

    // process short name max length 16 bytes.
    if (len > MAX_LEN_SHORT_NAME) {
        if (strncpy_s(shortName, MAX_LEN_SHORT_NAME, processName, MAX_LEN_SHORT_NAME - 1) != EOK) {
            HiLog::Error(LABEL, "strncpy_s short name error: %{public}s", strerror(errno));
            return -EINVAL;
        }
    } else {
        if (strncpy_s(shortName, MAX_LEN_SHORT_NAME, processName, len) != EOK) {
            HiLog::Error(LABEL, "strncpy_s short name error: %{public}s", strerror(errno));
            return -EINVAL;
        }
    }

    // set short name
    if (prctl(PR_SET_NAME, shortName) == -1) {
        HiLog::Error(LABEL, "prctl(PR_SET_NAME) error: %{public}s", strerror(errno));
        return (-errno);
    }

    // reset longProcName
    if (memset_s(longProcName, longProcNameLen, 0, longProcNameLen) != EOK) {
        HiLog::Error(LABEL, "Failed to memset long process name");
        return -EINVAL;
    }

    // set long process name
    if (strncpy_s(longProcName, len, processName, len) != EOK) {
        HiLog::Error(LABEL, "strncpy_s long name error: %{public}s", strerror(errno));
        return -EINVAL;
    }

    return ERR_OK;
}

int32_t AppSpawnServer::SetUidGid(
    const uint32_t uid, const uint32_t gid, const uint32_t *gitTable, const uint32_t gidCount)
{
    // set gids
    if (setgroups(gidCount, reinterpret_cast<const gid_t *>(&gitTable[0])) == -1) {
        HiLog::Error(LABEL, "setgroups failed: %{public}s, gids.size=%{public}u", strerror(errno), gidCount);
        return (-errno);
    }

    // set gid
    if (setresgid(gid, gid, gid) == -1) {
        HiLog::Error(LABEL, "setgid(%{public}u) failed: %{public}s", gid, strerror(errno));
        return (-errno);
    }

    // If the effective user ID is changed from 0 to nonzero, then all capabilities are cleared from the effective set
    if (setresuid(uid, uid, uid) == -1) {
        HiLog::Error(LABEL, "setuid(%{public}u) failed: %{public}s", uid, strerror(errno));
        return (-errno);
    }
    return ERR_OK;
}

int32_t AppSpawnServer::SetFileDescriptors()
{
    // close stdin stdout stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // redirect to /dev/null
    int dev_null_fd = open(deviceNull_.c_str(), O_RDWR);
    if (dev_null_fd == -1) {
        HiLog::Error(LABEL, "open dev_null error: %{public}s", strerror(errno));
        return (-errno);
    }

    // stdin
    if (dup2(dev_null_fd, STDIN_FILENO) == -1) {
        HiLog::Error(LABEL, "dup2 STDIN error: %{public}s", strerror(errno));
        return (-errno);
    };

    // stdout
    if (dup2(dev_null_fd, STDOUT_FILENO) == -1) {
        HiLog::Error(LABEL, "dup2 STDOUT error: %{public}s", strerror(errno));
        return (-errno);
    };

    // stderr
    if (dup2(dev_null_fd, STDERR_FILENO) == -1) {
        HiLog::Error(LABEL, "dup2 STDERR error: %{public}s", strerror(errno));
        return (-errno);
    };

    return ERR_OK;
}

int32_t AppSpawnServer::SetCapabilities()
{
    // init cap
    __user_cap_header_struct cap_header;
    if (memset_s(&cap_header, sizeof(cap_header), 0, sizeof(cap_header)) != EOK) {
        HiLog::Error(LABEL, "Failed to memset cap header");
        return -EINVAL;
    }
    cap_header.version = _LINUX_CAPABILITY_VERSION_3;
    cap_header.pid = 0;

    __user_cap_data_struct cap_data[2];
    if (memset_s(&cap_data, sizeof(cap_data), 0, sizeof(cap_data)) != EOK) {
        HiLog::Error(LABEL, "Failed to memset cap data");
        return -EINVAL;
    }

    // init inheritable permitted effective zero
#ifdef GRAPHIC_PERMISSION_CHECK
    uint64_t inheriTable = 0;
    uint64_t permitted = 0;
    uint64_t effective = 0;
#else
    uint64_t inheriTable = 0x3fffffffff;
    uint64_t permitted = 0x3fffffffff;
    uint64_t effective = 0x3fffffffff;
#endif

    cap_data[0].inheritable = inheriTable;
    cap_data[1].inheritable = inheriTable >> BITLEN32;
    cap_data[0].permitted = permitted;
    cap_data[1].permitted = permitted >> BITLEN32;
    cap_data[0].effective = effective;
    cap_data[1].effective = effective >> BITLEN32;

    // set capabilities
    if (capset(&cap_header, &cap_data[0]) == -1) {
        HiLog::Error(LABEL, "capset failed: %{public}s", strerror(errno));
        return errno;
    }

    return ERR_OK;
}

void AppSpawnServer::SetRunning(bool isRunning)
{
    isRunning_ = isRunning;
}

void AppSpawnServer::SetServerSocket(const std::shared_ptr<ServerSocket> &serverSocket)
{
    socket_ = serverSocket;
}

bool AppSpawnServer::SetAppProcProperty(int connectFd, const ClientSocket::AppProperty *appProperty, char *longProcName,
    int64_t longProcNameLen, const int32_t fd[FDLEN2])
{
    pid_t newPid = getpid();
    HiLog::Debug(LABEL, "AppSpawnServer::Success to fork new process, pid = %{public}d", newPid);
    // close socket connection in child process
    socket_->CloseConnection(connectFd);
    // close peer socket
    socket_->CloseServerMonitor();
    // close read end
    close(fd[0]);
    UninstallSigHandler();

    // set process name
    int32_t ret = ERR_OK;
    ret = SetProcessName(longProcName, longProcNameLen, appProperty->processName, strlen(appProperty->processName) + 1);
    if (FAILED(ret)) {
        HiLog::Error(LABEL, "SetProcessName error, ret %{public}d", ret);
        write(fd[1], &ret, sizeof(ret));
        close(fd[1]);
        return false;
    }

#ifdef GRAPHIC_PERMISSION_CHECK
    // set uid gid
    ret = SetUidGid(appProperty->uid, appProperty->gid, appProperty->gidTable, appProperty->gidCount);
    if (FAILED(ret)) {
        HiLog::Error(LABEL, "SetUidGid error, ret %{public}d", ret);
        write(fd[1], &ret, sizeof(ret));
        close(fd[1]);
        return false;
    }
#endif

    // set file descriptors
    ret = SetFileDescriptors();
    if (FAILED(ret)) {
        HiLog::Error(LABEL, "SetFileDescriptors error, ret %{public}d", ret);
        write(fd[1], &ret, sizeof(ret));
        close(fd[1]);
        return false;
    }

    // set capabilities
    ret = SetCapabilities();
    if (FAILED(ret)) {
        HiLog::Error(LABEL, "SetCapabilities error, ret %{public}d", ret);
        write(fd[1], &ret, sizeof(ret));
        close(fd[1]);
        return false;
    }

    // child process init success, send to father process
    write(fd[1], &ret, sizeof(ret));
    close(fd[1]);

    // start app process
    AppExecFwk::MainThread::Start();

    HiLog::Error(LABEL, "Failed to start process, pid = %{public}d", newPid);
    return false;
}
}  // namespace AppSpawn
}  // namespace OHOS
