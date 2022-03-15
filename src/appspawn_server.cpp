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
#include <csignal>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/capability.h>
#include <sys/syscall.h>
#include <thread>
#include <string>
#include <map>

#include "errors.h"
#include "hilog/log.h"
#include "main_thread.h"
#include "securec.h"
#include "bundle_mgr_interface.h"
#include "if_system_ability_manager.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "token_setproc.h"
#include "parameter.h"
#include "parameters.h"
#include "beget_ext.h"
#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif

#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

constexpr static mode_t FILE_MODE = 0711;
constexpr static mode_t NWEB_FILE_MODE = 0511;

#define APPSPAWN_LOGI(fmt, ...) STARTUP_LOGI("appspawn_server.log", "APPSPAWN", fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGE(fmt, ...) STARTUP_LOGE("appspawn_server.log", "APPSPAWN", fmt, ##__VA_ARGS__)
#define GRAPHIC_PERMISSION_CHECK

namespace OHOS {
namespace AppSpawn {
namespace {
constexpr int32_t ERR_PIPE_FAIL = -100;
constexpr int32_t MAX_LEN_SHORT_NAME = 16;
constexpr int32_t WAIT_DELAY_US = 100 * 1000;  // 100ms
constexpr int32_t GID_USER_DATA_RW = 1008;
constexpr int32_t MAX_GIDS = 64;
constexpr int32_t UID_BASE = 200000;
constexpr int32_t WAIT_PARAM_TIME = 5;

constexpr std::string_view BUNDLE_NAME_MEDIA_LIBRARY("com.ohos.medialibrary.MediaLibraryDataA");
constexpr std::string_view BUNDLE_NAME_SCANNER("com.ohos.medialibrary.MediaScannerAbilityA");
}  // namespace

using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0, "AppSpawnServer"};

#ifdef __cplusplus
extern "C" {
#endif

static void SignalHandler([[maybe_unused]] int sig)
{
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
    }
}

static void InstallSigHandler()
{
    struct sigaction sa = {};
    sa.sa_handler = SignalHandler;
    int err = sigaction(SIGCHLD, &sa, nullptr);
    if (err < 0) {
        HiLog::Error(LABEL, "Error installing SIGCHLD handler: %{public}d", errno);
        return;
    }

    struct sigaction sah = {};
    sah.sa_handler = SIG_IGN;
    err = sigaction(SIGHUP, &sah, nullptr);
    if (err < 0) {
        HiLog::Error(LABEL, "Error installing SIGHUP handler: %{public}d", errno);
    }
}

static void UninstallSigHandler()
{
    struct sigaction sa = {};
    sa.sa_handler = nullptr;
    int err = sigaction(SIGCHLD, &sa, nullptr);
    if (err < 0) {
        HiLog::Error(LABEL, "Error uninstalling SIGCHLD handler: %d", errno);
    }

    struct sigaction sah = {};
    sah.sa_handler = nullptr;
    err = sigaction(SIGHUP, &sah, nullptr);
    if (err < 0) {
        HiLog::Error(LABEL, "Error uninstalling SIGHUP handler: %d", errno);
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
        HiLog::Error(LABEL, "Failed to listen connection %d, %d", connectFd, errno);
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

void AppSpawnServer::WaitRebootEvent()
{
    APPSPAWN_LOGI("wait 'startup.device.ctl' event");
    while (isRunning_) {
        int ret = WaitParameter("startup.device.ctl", "stop", WAIT_PARAM_TIME);
        if (ret == 0) {
            std::lock_guard<std::mutex> lock(mut_);
            isStop_ = true;
            dataCond_.notify_one();
            break;
        } else {
            std::string value = OHOS::system::GetParameter("startup.device.ctl", "");
            if (value == "stop") {
                std::lock_guard<std::mutex> lock(mut_);
                isStop_ = true;
                dataCond_.notify_one();
                break;
            }
        }
    }
}

void AppSpawnServer::HandleSignal()
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, nullptr);
    int signalFd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (signalFd < 0) {
        APPSPAWN_LOGE("Error installing SIGHUP handler: %d", errno);
    }
    while (isRunning_) {
        struct signalfd_siginfo fdsi;
        ssize_t ret = read(signalFd, &fdsi, sizeof(fdsi));
        if (ret != sizeof(fdsi) || fdsi.ssi_signo != SIGCHLD) {
            continue;
        }
        pid_t pid;
        int status;
        while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
            APPSPAWN_LOGE("HandleSignal: %d", pid);
        }

        std::lock_guard<std::mutex> lock(mut_);
        isChildDie_ = true;
        childPid_ = fdsi.ssi_pid;
        APPSPAWN_LOGI("exit app pid = %d", childPid_);
        dataCond_.notify_one();
    }
    close(signalFd);
    signalFd = -1;
}

void AppSpawnServer::LoadAceLib()
{
#ifdef NWEB_SPAWN
    const std::string LOAD_LIB_DIR = "/data/app/el1/bundle/public/com.ohos.nweb/libs/arm";
#ifdef __MUSL__
    Dl_namespace dlns;
    dlns_init(&dlns, "nweb_ns");
    dlns_create(&dlns, LOAD_LIB_DIR.c_str());
    void *handle = dlopen_ns(&dlns, "libweb_engine.so", RTLD_NOW | RTLD_GLOBAL);
#else
    const std::string ENGINE_LIB_DIR = LOAD_LIB_DIR + "/libweb_engine.so";
    void *handle = dlopen(ENGINE_LIB_DIR.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (handle == nullptr) {
        HiLog::Error(LABEL, "Fail to dlopen libweb_engine.so, [%{public}s]", dlerror());
    } else {
        HiLog::Info(LABEL, "Success to dlopen libweb_engine.so");
    }
#ifdef __MUSL__
    nwebHandle = dlopen_ns(&dlns, "libnweb_render.so", RTLD_NOW | RTLD_GLOBAL);
#else
    const std::string RENDER_LIB_DIR = LOAD_LIB_DIR + "/libnweb_render.so";
    nwebHandle = dlopen(RENDER_LIB_DIR.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (nwebHandle == nullptr) {
        HiLog::Error(LABEL, "Fail to dlopen libnweb_render.so, [%{public}s]", dlerror());
    } else {
        HiLog::Info(LABEL, "Success to dlopen libnweb_render.so");
    }

#else
    std::string acelibdir("/system/lib/libace.z.so");
    void *AceAbilityLib = nullptr;
    HiLog::Info(LABEL, "MainThread::LoadAbilityLibrary. Start calling dlopen acelibdir.");
    AceAbilityLib = dlopen(acelibdir.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (AceAbilityLib == nullptr) {
        HiLog::Error(LABEL, "Fail to dlopen %{public}s, [%{public}s]", acelibdir.c_str(), dlerror());
    } else {
        HiLog::Info(LABEL, "Success to dlopen %{public}s", acelibdir.c_str());
    }
    HiLog::Info(LABEL, "MainThread::LoadAbilityLibrary. End calling dlopen.");
#endif
}

static void ClearEnvironment(void)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_UNBLOCK, &mask, nullptr);
    return;
}

int AppSpawnServer::DoColdStartApp(ClientSocket::AppProperty *appProperty, int fd)
{
    APPSPAWN_LOGI("DoColdStartApp::appName %s", appProperty->processName);
    std::vector<char *> extractedCmds;
    extractedCmds.push_back(const_cast<char *>("/system/bin/appspawntools"));
    std::string tmp = std::to_string(fd);
    APPSPAWN_LOGI("DoColdStartApp::fd %d %s", fd, tmp.c_str());
    extractedCmds.push_back(const_cast<char *>(tmp.c_str()));
    std::string uid = std::to_string(appProperty->uid);
    APPSPAWN_LOGI("DoColdStartApp::uid %d gid %d  %s", appProperty->uid, appProperty->gid, uid.c_str());
    extractedCmds.push_back(const_cast<char *>(uid.c_str()));
    std::string gid = std::to_string(appProperty->gid);
    extractedCmds.push_back(const_cast<char *>(gid.c_str()));
    extractedCmds.push_back(const_cast<char *>(appProperty->processName));
    extractedCmds.push_back(const_cast<char *>(appProperty->bundleName));
    extractedCmds.push_back(const_cast<char *>(appProperty->soPath));
    std::string accessTokenId = std::to_string(appProperty->accessTokenId);
    APPSPAWN_LOGI("DoColdStartApp::accessTokenId %d %s", appProperty->accessTokenId, accessTokenId.c_str());
    extractedCmds.push_back(const_cast<char *>(accessTokenId.c_str()));
    extractedCmds.push_back(const_cast<char *>(appProperty->apl));
    APPSPAWN_LOGI("DoColdStartApp renderCmd %s", appProperty->renderCmd);
    extractedCmds.push_back(const_cast<char *>(appProperty->renderCmd));
    std::string flags = std::to_string(appProperty->flags);
    extractedCmds.push_back(const_cast<char *>(flags.c_str()));
    std::string gidCount = std::to_string(appProperty->gidCount);
    APPSPAWN_LOGI("DoColdStartApp gidCount %d %s", appProperty->gidCount, gidCount.c_str());
    extractedCmds.push_back(const_cast<char *>(gidCount.c_str()));
    for (uint32_t i = 0; i < appProperty->gidCount; i++) {
        extractedCmds.push_back(const_cast<char *>(std::string(std::to_string(appProperty->gidTable[i])).c_str()));
    }
    extractedCmds.push_back(nullptr);
    APPSPAWN_LOGI("DoColdStartApp extractedCmds %d", extractedCmds.size());
    int ret = execv(extractedCmds[0], extractedCmds.data());
    if (ret) {
        HiLog::Error(LABEL, "Failed to execv, errno = %{public}d", errno);
        NotifyResToParentProc(fd, -1);
    }
    return 0;
}

int AppSpawnServer::StartApp(char *longProcName, int64_t longProcNameLen,
    ClientSocket::AppProperty *appProperty, int connectFd, pid_t &pid)
{
    if (!CheckAppProperty(appProperty)) {
        return -EINVAL;
    }
    int32_t fd[FDLEN2] = {FD_INIT_VALUE, FD_INIT_VALUE};
    int32_t buff = 0;
    if (pipe(fd) == -1) {
        HiLog::Error(LABEL, "create pipe fail, errno = %{public}d", errno);
        return ERR_PIPE_FAIL;
    }

    InstallSigHandler();
    pid = fork();
    if (pid < 0) {
        HiLog::Error(LABEL, "AppSpawnServer::Failed to fork new process, errno = %{public}d", errno);
        close(fd[0]);
        close(fd[1]);
        return -errno;
    } else if (pid == 0) {
        SpecialHandle(appProperty);
        // close socket connection and peer socket in child process
        if (socket_ != NULL) {
            socket_->CloseConnection(connectFd);
            socket_->CloseServerMonitor();
        }
        close(fd[0]); // close read fd
        ClearEnvironment();
        UninstallSigHandler();
        SetAppAccessToken(appProperty);
        if ((appProperty->flags == ClientSocket::APPSPAWN_COLD_BOOT) &&
            OHOS::system::GetBoolParameter("appspawn.cold.boot", false)) {
            DoColdStartApp(appProperty, fd[1]);
        } else {
            SetAppProcProperty(appProperty, longProcName, longProcNameLen, fd[1]);
        }
        _exit(0);
    }
    read(fd[0], &buff, sizeof(buff)); // wait child process resutl
    close(fd[0]);
    close(fd[1]);

    HiLog::Info(LABEL, "child process init %{public}s", (buff == ERR_OK) ? "success" : "fail");
    return (buff == ERR_OK) ? 0 : buff;
}

void AppSpawnServer::QuickExitMain()
{
#ifndef TEST_EXIT
    quick_exit(0);
#endif
    return;
}

bool AppSpawnServer::ServerMain(char *longProcName, int64_t longProcNameLen)
{
    if (socket_->RegisterServerSocket() != 0) {
        HiLog::Error(LABEL, "AppSpawnServer::Failed to register server socket");
        return false;
    }
    std::thread(&AppSpawnServer::ConnectionPeer, this).detach();
    LoadAceLib();

    std::thread(&AppSpawnServer::WaitRebootEvent, this).detach();
    std::thread(&AppSpawnServer::HandleSignal, this).detach();
    while (isRunning_) {
        std::unique_lock<std::mutex> lock(mut_);
        dataCond_.wait(lock, [this] { return !this->appQueue_.empty() || isStop_ || isChildDie_;});
        if (isStop_) { // finish
            break;
        }
        if (isChildDie_) { // process child die
            isChildDie_ = false;
            auto iter = appMap_.find(childPid_);
            if (iter != appMap_.end()) {
                APPSPAWN_LOGI("delete pid=%d in appMap", iter->first);
                appMap_.erase(iter);
            }
        }
        if (this->appQueue_.empty()) {
            continue;
        }
        std::unique_ptr<AppSpawnMsgPeer> msg = std::move(appQueue_.front());
        appQueue_.pop();
        int connectFd = msg->GetConnectFd();
        ClientSocket::AppProperty *appProperty = msg->GetMsg();
        pid_t pid = 0;
        int ret = StartApp(longProcName, longProcNameLen, appProperty, connectFd, pid);
        if (ret) {
            msg->Response(ret);
        } else {
            msg->Response(pid);
            appMap_[pid] = appProperty->processName;
        }
        socket_->CloseConnection(connectFd); // close socket connection
        APPSPAWN_LOGI("AppSpawnServer::parent process create app finish, pid = %d uid %d %s %s",
            pid, appProperty->uid, appProperty->processName, appProperty->bundleName);
    }

    while (appMap_.size() > 0) {
        auto iter = appMap_.begin();
        APPSPAWN_LOGI("kill app, pid = %d, processName = %s", iter->first, iter->second.c_str());
        kill(iter->first, SIGKILL);
        appMap_.erase(iter);
    }
    QuickExitMain();
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
            HiLog::Error(LABEL, "strncpy_s short name error: %{public}d", errno);
            return -EINVAL;
        }
    } else {
        if (strncpy_s(shortName, MAX_LEN_SHORT_NAME, processName, len) != EOK) {
            HiLog::Error(LABEL, "strncpy_s short name error: %{public}d", errno);
            return -EINVAL;
        }
    }

    // set short name
    if (prctl(PR_SET_NAME, shortName) == -1) {
        HiLog::Error(LABEL, "prctl(PR_SET_NAME) error: %{public}d", errno);
        return (-errno);
    }

    // reset longProcName
    if (memset_s(longProcName, static_cast<size_t>(longProcNameLen), 0, static_cast<size_t>(longProcNameLen)) != EOK) {
        HiLog::Error(LABEL, "Failed to memset long process name");
        return -EINVAL;
    }

    // set long process name
    if (strncpy_s(longProcName, len, processName, len) != EOK) {
        HiLog::Error(LABEL, "strncpy_s long name error: %{public}d", errno);
        return -EINVAL;
    }

    return ERR_OK;
}

int32_t AppSpawnServer::SetUidGid(
    const uint32_t uid, const uint32_t gid, const uint32_t *gitTable, const uint32_t gidCount)
{
    if (gitTable == nullptr) {
        HiLog::Error(LABEL, "gitTable is nullptr");
        return (-errno);
    }

    // set gids
    if (setgroups(gidCount, reinterpret_cast<const gid_t *>(&gitTable[0])) == -1) {
        HiLog::Error(LABEL, "setgroups failed: %{public}d, gids.size=%{public}u", errno, gidCount);
        return (-errno);
    }

    // set gid
    if (setresgid(gid, gid, gid) == -1) {
        HiLog::Error(LABEL, "setgid(%{public}u) failed: %{public}d", gid, errno);
        return (-errno);
    }

    // If the effective user ID is changed from 0 to nonzero, then all capabilities are cleared from the effective set
    if (setresuid(uid, uid, uid) == -1) {
        HiLog::Error(LABEL, "setuid(%{public}u) failed: %{public}d", uid, errno);
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
        HiLog::Error(LABEL, "open dev_null error: %{public}d", errno);
        return (-errno);
    }

    // stdin
    if (dup2(dev_null_fd, STDIN_FILENO) == -1) {
        HiLog::Error(LABEL, "dup2 STDIN error: %{public}d", errno);
        return (-errno);
    };

    // stdout
    if (dup2(dev_null_fd, STDOUT_FILENO) == -1) {
        HiLog::Error(LABEL, "dup2 STDOUT error: %{public}d", errno);
        return (-errno);
    };

    // stderr
    if (dup2(dev_null_fd, STDERR_FILENO) == -1) {
        HiLog::Error(LABEL, "dup2 STDERR error: %{public}d", errno);
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
    const uint64_t inheriTable = 0;
    const uint64_t permitted = 0;
    const uint64_t effective = 0;
#else
    const uint64_t inheriTable = 0x3fffffffff;
    const uint64_t permitted = 0x3fffffffff;
    const uint64_t effective = 0x3fffffffff;
#endif

    cap_data[0].inheritable = static_cast<__u32>(inheriTable);
    cap_data[1].inheritable = static_cast<__u32>(inheriTable >> BITLEN32);
    cap_data[0].permitted = static_cast<__u32>(permitted);
    cap_data[1].permitted = static_cast<__u32>(permitted >> BITLEN32);
    cap_data[0].effective = static_cast<__u32>(effective);
    cap_data[1].effective = static_cast<__u32>(effective >> BITLEN32);

    // set capabilities
    if (capset(&cap_header, &cap_data[0]) == -1) {
        HiLog::Error(LABEL, "capset failed: %{public}d", errno);
        return (-errno);
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

int32_t AppSpawnServer::DoAppSandboxMountOnce(const std::string originPath, const std::string destinationPath)
{
    int rc = 0;

    rc = mount(originPath.c_str(), destinationPath.c_str(), NULL, MS_BIND | MS_REC, NULL);
    if (rc) {
        HiLog::Error(LABEL, "bind mount %{public}s to %{public}s failed %{public}d", originPath.c_str(),
            destinationPath.c_str(), errno);
        return rc;
    }

    rc = mount(NULL, destinationPath.c_str(), NULL, MS_PRIVATE, NULL);
    if (rc) {
        HiLog::Error(LABEL, "private mount to %{public}s failed %{public}d", destinationPath.c_str(), errno);
        return rc;
    }

    return 0;
}

int32_t AppSpawnServer::DoAppSandboxMount(const ClientSocket::AppProperty *appProperty,
                                          std::string rootPath)
{
    std::string currentUserId = std::to_string(appProperty->uid / UID_BASE);
    std::string oriInstallPath = "/data/app/el1/bundle/public/";
    std::string oriel1DataPath = "/data/app/el1/" + currentUserId + "/base/";
    std::string oriel2DataPath = "/data/app/el2/" + currentUserId + "/base/";
    std::string oriDatabasePath = "/data/app/el2/" + currentUserId + "/database/";
    const std::string oriappdataPath = "/data/accounts/account_0/appdata/";
    std::string destDatabasePath = rootPath + "/data/storage/el2/database";
    std::string destInstallPath = rootPath + "/data/storage/el1/bundle";
    std::string destel1DataPath = rootPath + "/data/storage/el1/base";
    std::string destel2DataPath = rootPath + "/data/storage/el2/base";
    std::string destappdataPath = rootPath + oriappdataPath;

    int rc = 0;

    std::string bundleName = appProperty->bundleName;
    oriInstallPath += bundleName;
    oriel1DataPath += bundleName;
    oriel2DataPath += bundleName;
    oriDatabasePath += bundleName;

    std::map<std::string, std::string> mountMap;
    mountMap[destDatabasePath] = oriDatabasePath;
    mountMap[destInstallPath] = oriInstallPath;
    mountMap[destel1DataPath] = oriel1DataPath;
    mountMap[destel2DataPath] = oriel2DataPath;
    mountMap[destappdataPath] = oriappdataPath;

    std::map<std::string, std::string>::iterator iter;
    for (iter = mountMap.begin(); iter != mountMap.end(); ++iter) {
        rc = DoAppSandboxMountOnce(iter->second.c_str(), iter->first.c_str());
        if (rc) {
            return rc;
        }
    }

    // to create some useful dir when mount point created
    std::vector<std::string> mkdirInfo;
    std::string dirPath;
    mkdirInfo.push_back("/data/storage/el1/bundle/nweb");

    for (int i = 0; i < mkdirInfo.size(); i++) {
        dirPath = rootPath + mkdirInfo[i];
        mkdir(dirPath.c_str(), FILE_MODE);
    }

    return 0;
}

int32_t AppSpawnServer::DoAppSandboxMountCustomized(const ClientSocket::AppProperty *appProperty, std::string rootPath)
{
    std::string bundleName = appProperty->bundleName;
    std::string currentUserId = std::to_string(appProperty->uid / UID_BASE);
    std::string destInstallPath = rootPath + "/data/storage/el1/bundle";

    // account_0/applications/ dir can still access other packages' data now for compatibility purpose
    std::string oriapplicationsPath = "/data/app/el1/bundle/public/";
    std::string destapplicationsPath = rootPath + "/data/accounts/account_0/applications/";
    DoAppSandboxMountOnce(oriapplicationsPath.c_str(), destapplicationsPath.c_str());

    // need permission check for system app here
    std::string destbundlesPath = rootPath + "/data/bundles/";
    DoAppSandboxMountOnce(oriapplicationsPath.c_str(), destbundlesPath.c_str());

    std::string orimntHmdfsPath = "/mnt/hmdfs/";
    std::string destmntHmdfsPath = rootPath + orimntHmdfsPath;
    DoAppSandboxMountOnce(orimntHmdfsPath.c_str(), destmntHmdfsPath.c_str());

    // Add distributedfile module support, later reconstruct it
    std::string oriDistributedPath = "/mnt/hmdfs/" +  currentUserId + "/account/merge_view/data/" + bundleName;
    std::string destDistributedPath = rootPath + "/data/storage/el2/distributedfiles";
    DoAppSandboxMountOnce(oriDistributedPath.c_str(), destDistributedPath.c_str());

    std::string oriDistributedGroupPath = "/mnt/hmdfs/" +  currentUserId + "/non_account/merge_view/data/" + bundleName;
    std::string destDistributedGroupPath = rootPath + "/data/storage/el2/auth_groups";
    DoAppSandboxMountOnce(oriDistributedGroupPath.c_str(), destDistributedGroupPath.c_str());

    // do nweb adaption
    std::string orinwebPath = "/data/app/el1/bundle/public/com.ohos.nweb";
    std::string destnwebPath = destInstallPath + "/nweb";
    chmod(destnwebPath.c_str(), NWEB_FILE_MODE);
    DoAppSandboxMountOnce(orinwebPath.c_str(), destnwebPath.c_str());

    if (bundleName.find("medialibrary") != std::string::npos) {
        std::string oriMediaPath = "/storage/media/" +  currentUserId;
        std::string destMediaPath = rootPath + "/storage/media";
        DoAppSandboxMountOnce(oriMediaPath.c_str(), destMediaPath.c_str());
    }

    return 0;
}

void AppSpawnServer::DoAppSandboxMkdir(std::string sandboxPackagePath, const ClientSocket::AppProperty *appProperty)
{
    std::vector<std::string> mkdirInfo;
    std::string dirPath;

    mkdirInfo.push_back("/mnt/");
    mkdirInfo.push_back("/mnt/hmdfs/");
    mkdirInfo.push_back("/data/");
    mkdirInfo.push_back("/storage/");
    mkdirInfo.push_back("/storage/media");
    mkdirInfo.push_back("/data/storage");
    // to create /mnt/sandbox/<packagename>/data/storage/el1 related path, later should delete this code.
    mkdirInfo.push_back("/data/storage/el1");
    mkdirInfo.push_back("/data/storage/el1/bundle");
    mkdirInfo.push_back("/data/storage/el1/base");
    mkdirInfo.push_back("/data/storage/el1/database");
    mkdirInfo.push_back("/data/storage/el2");
    mkdirInfo.push_back("/data/storage/el2/base");
    mkdirInfo.push_back("/data/storage/el2/database");
    mkdirInfo.push_back("/data/storage/el2/distributedfiles");
    mkdirInfo.push_back("/data/storage/el2/auth_groups");
    // create applications folder for compatibility purpose
    mkdirInfo.push_back("/data/accounts");
    mkdirInfo.push_back("/data/accounts/account_0");
    mkdirInfo.push_back("/data/accounts/account_0/applications/");
    mkdirInfo.push_back("/data/accounts/account_0/appdata/");
    mkdirInfo.push_back("/data/bundles/");

    for (int i = 0; i < mkdirInfo.size(); i++) {
        dirPath = sandboxPackagePath + mkdirInfo[i];
        mkdir(dirPath.c_str(), FILE_MODE);
    }
}

int32_t AppSpawnServer::DoSandboxRootFolderCreateAdapt(std::string sandboxPackagePath)
{
    int rc = mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL);
    if (rc) {
        HiLog::Error(LABEL, "set propagation slave failed");
        return rc;
    }

    // bind mount "/" to /mnt/sandbox/<packageName> path
    // rootfs: to do more resources bind mount here to get more strict resources constraints
    rc = mount("/", sandboxPackagePath.c_str(), NULL, MS_BIND | MS_REC, NULL);
    if (rc) {
        HiLog::Error(LABEL, "mount bind / failed");
        return rc;
    }

    return 0;
}

int32_t AppSpawnServer::DoSandboxRootFolderCreate(std::string sandboxPackagePath)
{
    int rc = mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL);
    if (rc) {
        return rc;
    }

    // bind mount sandboxPackagePath to make it a mount point for pivot_root syscall
    DoAppSandboxMountOnce(sandboxPackagePath.c_str(), sandboxPackagePath.c_str());

    // do /mnt/sandbox/<packageName> path mkdir
    std::map<std::string, std::string> mountMap;
    std::vector<std::string> vecInfo;
    std::string tmpDir = "";

    vecInfo.push_back("/config");
    vecInfo.push_back("/dev");
    vecInfo.push_back("/proc");
    vecInfo.push_back("/sys");
    vecInfo.push_back("/sys-prod");
    vecInfo.push_back("/system");

    for (int i = 0; i < vecInfo.size(); i++) {
        tmpDir = sandboxPackagePath + vecInfo[i];
        mkdir(tmpDir.c_str(), FILE_MODE);
        mountMap[vecInfo[i]] = tmpDir;
    }

    // bind mount root folder to /mnt/sandbox/<packageName> path
    std::map<std::string, std::string>::iterator iter;
    for (iter = mountMap.begin(); iter != mountMap.end(); ++iter) {
        rc = DoAppSandboxMountOnce(iter->first.c_str(), iter->second.c_str());
        if (rc) {
            HiLog::Error(LABEL, "move root folder failed, %{public}s", sandboxPackagePath.c_str());
        }
    }

    // to create symlink at /mnt/sandbox/<packageName> path
    // bin -> /system/bin
    // d -> /sys/kernel/debug
    // etc -> /system/etc
    // init -> /system/bin/init
    // lib -> /system/lib
    // sdcard -> /storage/self/primary
    std::map<std::string, std::string> symlinkMap;
    symlinkMap["/system/bin"] = sandboxPackagePath + "/bin";
    symlinkMap["/sys/kernel/debug"] = sandboxPackagePath + "/d";
    symlinkMap["/system/etc"] = sandboxPackagePath + "/etc";
    symlinkMap["/system/bin/init"] = sandboxPackagePath + "/init";
    symlinkMap["/system/lib"] = sandboxPackagePath + "/lib";

    for (iter = symlinkMap.begin(); iter != symlinkMap.end(); ++iter) {
        symlink(iter->first.c_str(), iter->second.c_str());
    }

    return 0;
}

int32_t AppSpawnServer::SetAppSandboxProperty(const ClientSocket::AppProperty *appProperty)
{
    int rc = 0;

    // create /mnt/sandbox/<packagename> path， later put it to rootfs module
    std::string sandboxPackagePath = "/mnt/sandbox/";
    mkdir(sandboxPackagePath.c_str(), FILE_MODE);
    sandboxPackagePath += appProperty->bundleName;
    mkdir(sandboxPackagePath.c_str(), FILE_MODE);

    // add pid to a new mnt namespace
    rc = unshare(CLONE_NEWNS);
    if (rc) {
        HiLog::Error(LABEL, "unshare failed, packagename is %{public}s", appProperty->processName);
        return rc;
    }

    // to make wargnar work
    if (access("/3rdmodem", F_OK) == 0) {
        rc = DoSandboxRootFolderCreateAdapt(sandboxPackagePath);
    } else {
        rc = DoSandboxRootFolderCreate(sandboxPackagePath);
    }
    if (rc) {
        HiLog::Error(LABEL, "DoSandboxRootFolderCreate failed, %{public}s", appProperty->processName);
        return rc;
    }

    // to create /mnt/sandbox/<packagename>/data/storage related path
    DoAppSandboxMkdir(sandboxPackagePath, appProperty);

    rc = DoAppSandboxMount(appProperty, sandboxPackagePath);
    if (rc) {
        HiLog::Error(LABEL, "DoAppSandboxMount failed, packagename is %{public}s", appProperty->processName);
        return rc;
    }

    rc = DoAppSandboxMountCustomized(appProperty, sandboxPackagePath);
    if (rc) {
        HiLog::Error(LABEL, "DoAppSandboxMountCustomized failed, packagename is %{public}s", appProperty->processName);
        return rc;
    }

    rc = chdir(sandboxPackagePath.c_str());
    if (rc) {
        HiLog::Error(LABEL, "chdir failed, packagename is %{public}s, path is %{public}s", \
            appProperty->processName, sandboxPackagePath.c_str());
        return rc;
    }

    rc = syscall(SYS_pivot_root, sandboxPackagePath.c_str(), sandboxPackagePath.c_str());
    if (rc) {
        HiLog::Error(LABEL, "pivot root failed, packagename is %{public}s, errno is %{public}d", \
            appProperty->processName, errno);
        return rc;
    }

    rc = umount2(".", MNT_DETACH);
    if (rc) {
        HiLog::Error(LABEL, "MNT_DETACH failed, packagename is %{public}s", appProperty->processName);
        return rc;
    }

    return ERR_OK;
}

void AppSpawnServer::SetAppAccessToken(const ClientSocket::AppProperty *appProperty)
{
    int32_t ret = SetSelfTokenID(appProperty->accessTokenId);
    HiLog::Info(LABEL, "AppSpawnServer::set access token id = %{public}d, ret = %{public}d %{public}d",
        appProperty->accessTokenId, ret, getuid());

#ifdef WITH_SELINUX
    HapContext hapContext;
    ret = hapContext.HapDomainSetcontext(appProperty->apl, appProperty->processName);
    if (ret) {
        HiLog::Error(LABEL, "AppSpawnServer::Failed to hap domain set context, errno = %{public}d %{public}s",
            errno, appProperty->apl);
    } else {
        HiLog::Info(LABEL, "AppSpawnServer::Success to hap domain set context, ret = %{public}d", ret);
    }
#endif
}

bool AppSpawnServer::SetAppProcProperty(const ClientSocket::AppProperty *appProperty, char *longProcName,
    int64_t longProcNameLen, const int32_t fd)
{
    HiLog::Debug(LABEL, "AppSpawnServer::Success to fork new process, pid = %{public}d", getpid());
    int32_t ret = SetAppSandboxProperty(appProperty);
    if (FAILED(ret)) {
        NotifyResToParentProc(fd, ret);
        return false;
    }

    ret = SetKeepCapabilities(appProperty->uid);
    if (FAILED(ret)) {
        NotifyResToParentProc(fd, ret);
        return false;
    }

    ret = SetProcessName(longProcName, longProcNameLen, appProperty->processName, strlen(appProperty->processName) + 1);
    if (FAILED(ret)) {
        NotifyResToParentProc(fd, ret);
        return false;
    }

#ifdef GRAPHIC_PERMISSION_CHECK
    ret = SetUidGid(appProperty->uid, appProperty->gid, appProperty->gidTable, appProperty->gidCount);
    if (FAILED(ret)) {
        NotifyResToParentProc(fd, ret);
        return false;
    }
#endif

    ret = SetFileDescriptors();
    if (FAILED(ret)) {
        NotifyResToParentProc(fd, ret);
        return false;
    }

    ret = SetCapabilities();
    if (FAILED(ret)) {
        NotifyResToParentProc(fd, ret);
        return false;
    }
    // notify success to father process and start app process
    NotifyResToParentProc(fd, ret);

#ifdef NWEB_SPAWN
    using FuncType = void (*)(const char *cmd);
    FuncType funcNWebRenderMain = reinterpret_cast<FuncType>(dlsym(nwebHandle, "NWebRenderMain"));
    if (funcNWebRenderMain == nullptr) {
        HiLog::Error(LABEL, "nwebspawn dlsym ERROR=%{public}s", dlerror());
        return false;
    }
    funcNWebRenderMain(appProperty->renderCmd);
#else
    AppExecFwk::MainThread::Start();
#endif

    HiLog::Error(LABEL, "Failed to start process, pid = %{public}d", getpid());
    return false;
}

void AppSpawnServer::NotifyResToParentProc(const int32_t fd, const int32_t value)
{
    write(fd, &value, sizeof(value));
    close(fd);
}

void AppSpawnServer::SpecialHandle(ClientSocket::AppProperty *appProperty)
{
    if (appProperty == nullptr) {
        HiLog::Error(LABEL, "appProperty is nullptr");
        return;
    }
    // special handle bundle name medialibrary and scanner
    if ((strcmp(appProperty->processName, BUNDLE_NAME_MEDIA_LIBRARY.data()) == 0) ||
        (strcmp(appProperty->processName, BUNDLE_NAME_SCANNER.data()) == 0)) {
        if (appProperty->gidCount < MAX_GIDS) {
            appProperty->gidTable[appProperty->gidCount] = GID_USER_DATA_RW;
            appProperty->gidCount++;
        } else {
            HiLog::Info(LABEL, "gidCount out of bounds !");
        }
    }
}

int32_t AppSpawnServer::SetKeepCapabilities(uint32_t uid)
{
    // set keep capabilities when user not root.
    if (uid != 0) {
        if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == -1) {
            HiLog::Error(LABEL, "set keepcaps failed: %{public}d", errno);
            return (-errno);
        }
    }

    return ERR_OK;
}

bool AppSpawnServer::CheckAppProperty(const ClientSocket::AppProperty *appProperty)
{
    if (appProperty == nullptr) {
        HiLog::Error(LABEL, "appProperty is nullptr");
        return false;
    }

    if (appProperty->gidCount > ClientSocket::MAX_GIDS) {
        HiLog::Error(LABEL, "gidCount error: %{public}u", appProperty->gidCount);
        return false;
    }

    if (strlen(appProperty->processName) == 0) {
        HiLog::Error(LABEL, "process name length is 0");
        return false;
    }

    return true;
}

int AppSpawnServer::AppColdStart(char *longProcName,
    int64_t longProcNameLen, const ClientSocket::AppProperty *appProperty, int fd)
{
    APPSPAWN_LOGI("AppColdStart appName %s", appProperty->bundleName);
    LoadAceLib();
    if (!SetAppProcProperty(appProperty, longProcName, longProcNameLen, fd)) {
        return -1;
    }
    APPSPAWN_LOGI("AppColdStart appName %s success", appProperty->bundleName);
    return 0;
}
}  // namespace AppSpawn
}  // namespace OHOS
