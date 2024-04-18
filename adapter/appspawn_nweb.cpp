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

#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#endif

struct RenderProcessNode {
    RenderProcessNode(time_t now, int exit):recordTime_(now), exitStatus_(exit) {}
    time_t recordTime_;
    int exitStatus_;
};

namespace {
    constexpr int32_t RENDER_PROCESS_MAX_NUM = 16;
    std::map<int32_t, RenderProcessNode> g_renderProcessMap;
    std::mutex g_mutex;

#if defined(webview_arm64)
    const std::string NWEB_HAP_LIB_PATH = "/data/storage/el1/bundle/nweb/libs/arm64";
#elif defined(webview_x86_64)
    const std::string NWEB_HAP_LIB_PATH = "/data/storage/el1/bundle/nweb/libs/x86_64";
#else
    const std::string NWEB_HAP_LIB_PATH = "/data/storage/el1/bundle/nweb/libs/arm";
#endif
}

void LoadExtendLibNweb(AppSpawnContent *content)
{
}

#ifdef WITH_SECCOMP
static bool SetSeccompPolicyForRenderer(void *nwebRenderHandle)
{
    if (IsEnableSeccomp()) {
        using SeccompFuncType = bool (*)(void);
        SeccompFuncType funcSetRendererSeccompPolicy =
                reinterpret_cast<SeccompFuncType>(dlsym(nwebRenderHandle, "SetRendererSeccompPolicy"));
        if (funcSetRendererSeccompPolicy == nullptr) {
            APPSPAWN_LOGE("SetRendererSeccompPolicy dlsym ERROR=%{public}s", dlerror());
            return false;
        }
        if (!funcSetRendererSeccompPolicy()) {
            APPSPAWN_LOGE("Failed to set seccomp policy.");
            return false;
        }
    }
    return true;
}
#endif

void RunChildProcessorNweb(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGI("RunChildProcessorNweb");
    void *webEngineHandle = nullptr;
    void *nwebRenderHandle = nullptr;

#ifdef __MUSL__
    Dl_namespace dlns;
    dlns_init(&dlns, "nweb_ns");
    dlns_create(&dlns, NWEB_HAP_LIB_PATH.c_str());

    // preload libweb_engine
    webEngineHandle = dlopen_ns(&dlns, "libweb_engine.so", RTLD_NOW | RTLD_GLOBAL);

    // load libnweb_render
    nwebRenderHandle = dlopen_ns(&dlns, "libnweb_render.so", RTLD_NOW | RTLD_GLOBAL);
#else
    // preload libweb_engine
    const std::string engineLibDir = NWEB_HAP_LIB_PATH + "/libweb_engine.so";
    webEngineHandle = dlopen(engineLibDir.c_str(), RTLD_NOW | RTLD_GLOBAL);

    // load libnweb_render
    const std::string renderLibDir = NWEB_HAP_LIB_PATH + "/libnweb_render.so";
    nwebRenderHandle = dlopen(renderLibDir.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (webEngineHandle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libweb_engine.so, [%{public}s]", dlerror());
    } else {
        APPSPAWN_LOGI("Success to dlopen libweb_engine.so");
    }

    if (nwebRenderHandle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libnweb_render.so, [%{public}s]", dlerror());
        return;
    } else {
        APPSPAWN_LOGI("Success to dlopen libnweb_render.so");
    }

#ifdef WITH_SECCOMP
    if (!SetSeccompPolicyForRenderer(nwebRenderHandle)) {
        return;
    }
#endif

    AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
    using FuncType = void (*)(const char *cmd);

    FuncType funcNWebRenderMain = reinterpret_cast<FuncType>(dlsym(nwebRenderHandle, "NWebRenderMain"));
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

static int GetRenderProcessTerminationStatus(int32_t pid, int *status)
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
    if (pid <= 0) {
        return -1;
    }
    if (kill(pid, SIGKILL) != 0) {
        APPSPAWN_LOGE("unable to kill render process, pid: %{public}d ret %{public}d", pid, errno);
    }

    pid_t exitPid = waitpid(pid, status, WNOHANG);
    if (exitPid != pid) {
        APPSPAWN_LOGE("waitpid failed, return : %{public}d, pid: %{public}d, status: %{public}d",
            exitPid, pid, *status);
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
    APPSPAWN_LOGI("AppSpawnServer::get render process termination status, status ="
        "%{public}d pid = %{public}d uid %{public}d %{public}s %{public}s",
        exitStatus, appProperty->property.pid, appProperty->property.uid,
        appProperty->property.processName, appProperty->property.bundleName);
    return exitStatus;
}
