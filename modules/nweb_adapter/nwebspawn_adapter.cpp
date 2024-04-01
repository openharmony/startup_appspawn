/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <cerrno>
#include <ctime>
#include <dlfcn.h>
#include <map>
#include <mutex>
#include <string>

#ifdef __MUSL__
#include <cerrno>
#include <dlfcn_ext.h>
#include <sys/mman.h>
#endif

#include "appspawn_hook.h"
#include "appspawn_manager.h"

#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#endif

namespace {
#if defined(webview_arm64)
    const std::string NWEB_HAP_LIB_PATH = "/data/storage/el1/bundle/nweb/libs/arm64";
#elif defined(webview_x86_64)
    const std::string NWEB_HAP_LIB_PATH = "/data/storage/el1/bundle/nweb/libs/x86_64";
#else
    const std::string NWEB_HAP_LIB_PATH = "/data/storage/el1/bundle/nweb/libs/arm";
#endif
}  // namespace

static bool SetSeccompPolicyForRenderer(void *nwebRenderHandle)
{
#ifdef WITH_SECCOMP
    if (IsEnableSeccomp()) {
        using SeccompFuncType = bool (*)(void);
        SeccompFuncType funcSetRendererSeccompPolicy =
                reinterpret_cast<SeccompFuncType>(dlsym(nwebRenderHandle, "SetRendererSeccompPolicy"));
        if (funcSetRendererSeccompPolicy != nullptr && funcSetRendererSeccompPolicy()) {
            return true;
        }
        APPSPAWN_LOGE("SetRendererSeccompPolicy dlsym errno: %{public}d", errno);
        return false;
    }
#endif
    return true;
}

static int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    uint32_t len = 0;
    char *renderCmd = reinterpret_cast<char *>(GetAppPropertyExt(
        reinterpret_cast<AppSpawningCtx *>(client), MSG_EXT_NAME_RENDER_CMD, &len));
    if (renderCmd == nullptr) {
        return -1;
    }
    std::string renderStr(renderCmd);
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
        APPSPAWN_LOGE("Fail to dlopen libweb_engine.so, errno: %{public}d", errno);
    }

    if (nwebRenderHandle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libnweb_render.so, errno: %{public}d", errno);
        return -1;
    }

    if (!SetSeccompPolicyForRenderer(nwebRenderHandle)) {
        return -1;
    }
    using FuncType = void (*)(const char *cmd);

    FuncType funcNWebRenderMain = reinterpret_cast<FuncType>(dlsym(nwebRenderHandle, "NWebRenderMain"));
    if (funcNWebRenderMain == nullptr) {
        APPSPAWN_LOGE("webviewspawn dlsym errno: %{public}d", errno);
        return -1;
    }
    AppSpawnEnvClear(content, client);
    funcNWebRenderMain(renderStr.c_str());
    APPSPAWN_LOGI("RunChildProcessorNweb %{public}s", renderStr.c_str());
    return 0;
}

static int PreLoadNwebSpawn(AppSpawnMgr *content)
{
    APPSPAWN_LOGI("PreLoadNwebSpawn %{public}d", IsNWebSpawnMode(content));
    if (!IsNWebSpawnMode(content)) {
        return 0;
    }
    // register
    RegChildLooper(&content->content, RunChildProcessor);
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGI("Load nweb module ...");
    AddPreloadHook(HOOK_PRIO_HIGHEST, PreLoadNwebSpawn);
}
