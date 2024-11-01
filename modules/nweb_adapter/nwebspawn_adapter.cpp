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
#include "appspawn_utils.h"

#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#endif

#ifndef APPSPAWN_TEST
#define APPSPAWN_STATIC static
#else
#define APPSPAWN_STATIC
#endif

namespace {
#if defined(webview_arm64)
    const std::string ARK_WEB_CORE_HAP_LIB_PATH =
        "/data/storage/el1/bundle/arkwebcore/libs/arm64";
#elif defined(webview_x86_64)
    const std::string ARK_WEB_CORE_HAP_LIB_PATH =
        "/data/storage/el1/bundle/arkwebcore/libs/x86_64";
#else
    const std::string ARK_WEB_CORE_HAP_LIB_PATH =
        "/data/storage/el1/bundle/arkwebcore/libs/arm";
#endif
    const std::string ARK_WEB_ENGINE_LIB_NAME = "libarkweb_engine.so";
    const std::string ARK_WEB_RENDER_LIB_NAME = "libarkweb_render.so";
    const std::string WEB_ENGINE_LIB_NAME = "libweb_engine.so";
    const std::string WEB_RENDER_LIB_NAME = "libnweb_render.so";
}  // namespace

APPSPAWN_STATIC bool SetSeccompPolicyForRenderer(void *nwebRenderHandle)
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

APPSPAWN_STATIC std::string GetArkWebEngineLibName()
{
    std::string arkWebEngineLibPath = ARK_WEB_CORE_HAP_LIB_PATH + "/"
            + ARK_WEB_ENGINE_LIB_NAME;
    bool isArkWebEngineLibPathExist = access(arkWebEngineLibPath.c_str(), F_OK) == 0;
    return isArkWebEngineLibPathExist ?
            ARK_WEB_ENGINE_LIB_NAME : WEB_ENGINE_LIB_NAME;
}

APPSPAWN_STATIC std::string GetArkWebRenderLibName()
{
    std::string arkWebRenderLibPath = ARK_WEB_CORE_HAP_LIB_PATH + "/"
            + ARK_WEB_RENDER_LIB_NAME;
    bool isArkWebRenderLibPathExist = access(arkWebRenderLibPath.c_str(), F_OK) == 0;
    return isArkWebRenderLibPathExist ?
            ARK_WEB_RENDER_LIB_NAME : WEB_RENDER_LIB_NAME;
}

APPSPAWN_STATIC int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    EnableCache();
    uint32_t len = 0;
    char *renderCmd = reinterpret_cast<char *>(GetAppPropertyExt(
        reinterpret_cast<AppSpawningCtx *>(client), MSG_EXT_NAME_RENDER_CMD, &len));
    APPSPAWN_CHECK_ONLY_EXPER(renderCmd != nullptr, return -1);
    std::string renderStr(renderCmd);
    void *webEngineHandle = nullptr;
    void *nwebRenderHandle = nullptr;

    const std::string& libPath = ARK_WEB_CORE_HAP_LIB_PATH;
    const std::string engineLibName = GetArkWebEngineLibName();
    const std::string renderLibName = GetArkWebRenderLibName();

#ifdef __MUSL__
    Dl_namespace dlns;
    dlns_init(&dlns, "nweb_ns");
    dlns_create(&dlns, libPath.c_str());

    Dl_namespace ndkns;
    dlns_get("ndkns", &ndkns);
    dlns_inherit(&dlns, &ndkns, "allow_all_shared_libs");
    
    // preload libweb_engine
    webEngineHandle =
        dlopen_ns(&dlns, engineLibName.c_str(), RTLD_NOW | RTLD_GLOBAL);
    // load libnweb_render
    nwebRenderHandle =
        dlopen_ns(&dlns, renderLibName.c_str(), RTLD_NOW | RTLD_GLOBAL);
#else
    // preload libweb_engine
    const std::string engineLibPath = libPath + "/" + engineLibName;
    webEngineHandle = dlopen(engineLibPath.c_str(), RTLD_NOW | RTLD_GLOBAL);

    // load libnweb_render
    const std::string renderLibPath = libPath + "/" + renderLibName;
    nwebRenderHandle = dlopen(renderLibPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (webEngineHandle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libweb_engine.so, errno: %{public}d", errno);
    }
    if (nwebRenderHandle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libnweb_render.so, errno: %{public}d", errno);
        return -1;
    }

    std::string processType = reinterpret_cast<char *>(GetAppPropertyExt(
        reinterpret_cast<AppSpawningCtx *>(client), MSG_EXT_NAME_PROCESS_TYPE, &len));
    if (processType == "render" && !SetSeccompPolicyForRenderer(nwebRenderHandle)) {
        return -1;
    }
    using FuncType = void (*)(const char *cmd);

    FuncType funcNWebRenderMain = reinterpret_cast<FuncType>(dlsym(nwebRenderHandle, "NWebRenderMain"));
    if (funcNWebRenderMain == nullptr) {
        APPSPAWN_LOGE("webviewspawn dlsym errno: %{public}d", errno);
        return -1;
    }
    AppSpawnEnvClear(content, client);
    APPSPAWN_LOGI("RunChildProcessorNweb %{public}s", renderStr.c_str());
    funcNWebRenderMain(renderStr.c_str());
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
