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

#include <cerrno>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/wait.h>

#include <algorithm>
#include <ctime>
#include <map>
#include <mutex>
#include <string>

#ifdef __MUSL__
#include <dlfcn_ext.h>
#include <cerrno>
#include <sys/mman.h>
#endif

#include "appspawn_service.h"
#include "appspawn_adapter.h"
struct RenderProcessNode {
    RenderProcessNode(time_t now, int exit):recordTime_(now), exitStatus_(exit) {}
    time_t recordTime_;
    int exitStatus_;
};

namespace {
    constexpr int32_t RENDER_PROCESS_MAX_NUM = 16;
    std::map<int32_t, RenderProcessNode> g_renderProcessMap;
    void *g_nwebHandle = nullptr;
    std::mutex g_mutex;
}

#ifdef __MUSL__
void *LoadWithRelroFile(const std::string &lib, const std::string &nsName,
                        const std::string &nsPath)
{
#ifdef webview_arm64
    const std::string nwebRelroPath =
        "/data/misc/shared_relro/libwebviewchromium64.relro";
    size_t nwebReservedSize = 1 * 1024 * 1024 * 1024;
#else
    const std::string nwebRelroPath =
        "/data/misc/shared_relro/libwebviewchromium32.relro";
    size_t nwebReservedSize = 130 * 1024 * 1024;
#endif
    if (unlink(nwebRelroPath.c_str()) != 0 && errno != ENOENT) {
        APPSPAWN_LOGI("LoadWithRelroFile unlink failed");
    }
    int relroFd =
        open(nwebRelroPath.c_str(), O_RDWR | O_TRUNC | O_CLOEXEC | O_CREAT,
            S_IRUSR | S_IRGRP | S_IROTH);
    if (relroFd < 0) {
        int tmpNo = errno;
        APPSPAWN_LOGE("LoadWithRelroFile open failed, error=[%{public}s]", strerror(tmpNo));
        return nullptr;
    }
    void *nwebReservedAddress = mmap(nullptr, nwebReservedSize, PROT_NONE,
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (nwebReservedAddress == MAP_FAILED) {
        close(relroFd);
        int tmpNo = errno;
        APPSPAWN_LOGE("LoadWithRelroFile mmap failed, error=[%{public}s]", strerror(tmpNo));
        return nullptr;
    }
    Dl_namespace dlns;
    dlns_init(&dlns, nsName.c_str());
    dlns_create(&dlns, nsPath.c_str());
    dl_extinfo extinfo = {
        .flag = DL_EXT_WRITE_RELRO | DL_EXT_RESERVED_ADDRESS_RECURSIVE |
                DL_EXT_RESERVED_ADDRESS,
        .relro_fd = relroFd,
        .reserved_addr = nwebReservedAddress,
        .reserved_size = nwebReservedSize,
    };
    void *result =
        dlopen_ns_ext(&dlns, lib.c_str(), RTLD_NOW | RTLD_GLOBAL, &extinfo);
    close(relroFd);
    return result;
}
#endif

void LoadExtendLib(AppSpawnContent *content)
{
#if defined(webview_arm64)
    const std::string loadLibDir = "/data/app/el1/bundle/public/com.ohos.nweb/libs/arm64";
#elif defined(webview_x86_64)
    const std::string loadLibDir = "/data/app/el1/bundle/public/com.ohos.nweb/libs/x86_64";
#else
    const std::string loadLibDir = "/data/app/el1/bundle/public/com.ohos.nweb/libs/arm";
#endif

#ifdef __MUSL__
    Dl_namespace dlns;
    dlns_init(&dlns, "nweb_ns");
    dlns_create(&dlns, loadLibDir.c_str());
#if defined(webview_x86_64)
    void *handle = dlopen_ns(&dlns, "libweb_engine.so", RTLD_NOW | RTLD_GLOBAL);
#else
    void *handle = LoadWithRelroFile("libweb_engine.so", "nweb_ns", loadLibDir);
    if (handle == nullptr) {
        APPSPAWN_LOGE("dlopen_ns_ext failed, fallback to dlopen_ns");
        handle = dlopen_ns(&dlns, "libweb_engine.so", RTLD_NOW | RTLD_GLOBAL);
    }
#endif
#else
    const std::string engineLibDir = loadLibDir + "/libweb_engine.so";
    void *handle = dlopen(engineLibDir.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (handle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libweb_engine.so, [%{public}s]", dlerror());
    } else {
        APPSPAWN_LOGI("Success to dlopen libweb_engine.so");
    }

#ifdef __MUSL__
    g_nwebHandle = dlopen_ns(&dlns, "libnweb_render.so", RTLD_NOW | RTLD_GLOBAL);
#else
    const std::string renderLibDir = loadLibDir + "/libnweb_render.so";
    g_nwebHandle = dlopen(renderLibDir.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (g_nwebHandle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libnweb_render.so, [%{public}s]", dlerror());
    } else {
        APPSPAWN_LOGI("Success to dlopen libnweb_render.so");
    }
}

void RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
    using FuncType = void (*)(const char *cmd);

    FuncType funcNWebRenderMain = reinterpret_cast<FuncType>(dlsym(g_nwebHandle, "NWebRenderMain"));
    if (funcNWebRenderMain == nullptr) {
        APPSPAWN_LOGI("webviewspawn dlsym ERROR=%{public}s", dlerror());
        return;
    }

    funcNWebRenderMain(appProperty->property.renderCmd);
}

static void DumpRenderProcessExitedMap()
{
    APPSPAWN_LOGI("dump render process exited array:");

    for (auto& it : g_renderProcessMap) {
        APPSPAWN_LOGV("[pid, time, exitedStatus] = [%{public}d, %{public}ld, %{public}d]",
            it.first, static_cast<long>(it.second.recordTime_), it.second.exitStatus_);
    }
}

void RecordRenderProcessExitedStatus(pid_t pid, int status)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_renderProcessMap.size() < RENDER_PROCESS_MAX_NUM) {
        RenderProcessNode node(time(nullptr), status);
        g_renderProcessMap.insert({pid, node});
        return;
    }

    APPSPAWN_LOGV("render process map size reach max, need to erase oldest data.");
    DumpRenderProcessExitedMap();
    auto oldestData = std::min_element(g_renderProcessMap.begin(), g_renderProcessMap.end(),
        [](const std::pair<int32_t, RenderProcessNode>& left, const std::pair<int32_t, RenderProcessNode>& right) {
            return left.second.recordTime_ < right.second.recordTime_;
        });

    g_renderProcessMap.erase(oldestData);
    RenderProcessNode node(time(nullptr), status);
    g_renderProcessMap.insert({pid, node});
    DumpRenderProcessExitedMap();
}

int GetRenderProcessTerminationStatus(int32_t pid, int *status)
{
    if (status == nullptr) {
        return -1;
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_renderProcessMap.find(pid);
    if (it != g_renderProcessMap.end()) {
        *status = it->second.exitStatus_;
        g_renderProcessMap.erase(it);
        return 0;
    }

    APPSPAWN_LOGE("not find pid[%{public}d] in render process exited map", pid);
    DumpRenderProcessExitedMap();
    return -1;
}

static int GetProcessTerminationStatusInner(int32_t pid, int *status)
{
    if (status == nullptr) {
        return -1;
    }

    if (GetRenderProcessTerminationStatus(pid, status) == 0) {
        // this shows that the parent process has received SIGCHLD signal.
        return 0;
    }

    if (kill(pid, SIGKILL) != 0) {
        APPSPAWN_LOGE("unable to kill render process, pid: %d ret %d", pid, errno);
    }

    pid_t exitPid = waitpid(pid, status, WNOHANG);
    if (exitPid != pid) {
        APPSPAWN_LOGE("waitpid failed, return : %d, pid: %d, status: %d", exitPid, pid, *status);
        return -1;
    }
    return 0;
}

int GetProcessTerminationStatus(AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
    APPSPAWN_CHECK(appProperty != nullptr, return -1, "Invalid client");
    int exitStatus = 0;
    int ret = GetProcessTerminationStatusInner(appProperty->property.pid, &exitStatus);
    if (ret) {
        exitStatus = ret;
    }
    APPSPAWN_LOGI("AppSpawnServer::get render process termination status, status = %d pid = %d uid %d %s %s",
        exitStatus, appProperty->property.pid, appProperty->property.uid,
        appProperty->property.processName, appProperty->property.bundleName);
    return exitStatus;
}
