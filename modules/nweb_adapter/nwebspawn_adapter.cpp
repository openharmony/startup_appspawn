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
#include "parameter.h"

#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#endif

#include "arkweb_utils.h"
#include "arkweb_preload_common.h"

namespace {
const std::string ARK_WEB_ENGINE_LIB_NAME = "libarkweb_engine.so";
const std::string ARK_WEB_RENDER_LIB_NAME = "libarkweb_render.so";
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

static void UpdateAppWebEngineVersion(std::string& renderCmd)
{
    size_t posLeft = renderCmd.rfind(APP_ENGINE_VERSION_PREFIX);
    if (posLeft == std::string::npos) {
        APPSPAWN_LOGE("not found app engine type arg");
        return;
    }
    size_t posRight = posLeft + strlen(APP_ENGINE_VERSION_PREFIX);
    size_t posEnd = renderCmd.find('#', posRight);
    std::string value = (posEnd == std::string::npos) ?
                            renderCmd.substr(posRight) :
                            renderCmd.substr(posRight, posEnd - posRight);

    char* end;
    long v = std::strtol(value.c_str(), &end, 10);
    if (*end != '\0' || v > INT_MAX || v < 0) {
        APPSPAWN_LOGE("invalid value: %{public}s", value.c_str());
        return;
    }
    auto version = static_cast<OHOS::ArkWeb::ArkWebEngineVersion>(v);
    OHOS::ArkWeb::SetActiveWebEngineVersionInner(version);

    // remove arg APP_ENGINE_VERSION_PREFIX
    size_t eraseLength = (posEnd == std::string::npos) ?
                            renderCmd.length() - posLeft :
                            posEnd - posLeft;
    renderCmd.erase(posLeft, eraseLength);
}

APPSPAWN_STATIC int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    uint32_t len = 0;
    char *renderCmd = reinterpret_cast<char *>(GetAppPropertyExt(
        reinterpret_cast<AppSpawningCtx *>(client), MSG_EXT_NAME_RENDER_CMD, &len));
    if (renderCmd == nullptr) {
        return -1;
    }
    std::string renderStr(renderCmd);
    UpdateAppWebEngineVersion(renderStr);

    void *webEngineHandle = nullptr;
    void *nwebRenderHandle = nullptr;

    const std::string libNsName = OHOS::ArkWeb::GetArkwebNameSpace();
    const std::string libPath = OHOS::ArkWeb::GetArkwebLibPath();
    const std::string engineLibName = ARK_WEB_ENGINE_LIB_NAME;
    const std::string renderLibName = ARK_WEB_RENDER_LIB_NAME;

#ifdef __MUSL__
    Dl_namespace dlns;
    Dl_namespace ndkns;
    dlns_init(&dlns, libNsName.c_str());
    dlns_create(&dlns, libPath.c_str());
    dlns_get("ndk", &ndkns);
    dlns_inherit(&dlns, &ndkns, "allow_all_shared_libs");
    // preload libweb_engine
    webEngineHandle = dlopen_ns(&dlns, engineLibName.c_str(), RTLD_NOW | RTLD_GLOBAL);
    // load libnweb_render
    nwebRenderHandle = dlopen_ns(&dlns, renderLibName.c_str(), RTLD_NOW | RTLD_GLOBAL);
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

APPSPAWN_STATIC int PreLoadNwebSpawn(AppSpawnMgr *content)
{
    APPSPAWN_LOGI("PreLoadNwebSpawn %{public}d", IsNWebSpawnMode(content));
    if (!IsNWebSpawnMode(content)) {
        return 0;
    }
    // register
    RegChildLooper(&content->content, RunChildProcessor);

    OHOS::ArkWeb::PreloadArkWebLibForRender();
    
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGI("Load nweb module ...");
    AddPreloadHook(HOOK_PRIO_HIGHEST, PreLoadNwebSpawn);
}
