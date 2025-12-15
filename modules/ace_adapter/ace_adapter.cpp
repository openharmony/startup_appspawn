/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstring>
#include <dlfcn.h>
#include <set>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "appspawn_hook.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#ifdef ARKWEB_UTILS_ENABLE
#include "arkweb_utils.h"
#endif
#include "command_lexer.h"
#include "config_policy_utils.h"
#include "hitrace_meter.h"
#include "js_runtime.h"
#include "json_utils.h"
#include "parameters.h"
#include "resource_manager.h"
#ifndef APPSPAWN_TEST
#include "ace_forward_compatibility.h"
#include "main_thread.h"
#include "runtime.h"
#endif

using namespace OHOS::AppSpawn;
using namespace OHOS::Global;

#define SYSTEMLIB_JSON "/appspawn_systemLib.json"
#ifdef ASAN_DETECTOR
static const bool DEFAULT_PRELOAD_VALUE = false;
#else
static const bool DEFAULT_PRELOAD_VALUE = true;
#endif
static const bool DEFAULT_PRELOAD_ETS_VALUE = true;
static const std::string PRELOAD_JSON_CONFIG("/appspawn_preload.json");
static const std::string PRELOAD_ETS_JSON_CONFIG("/appspawn_preload_ets.json");

typedef struct TagParseJsonContext {
    std::set<std::string> names;
    std::string key;
} ParseJsonContext;

static void GetNames(const cJSON *root, std::set<std::string> &names, std::string key)
{
    cJSON *namesJson = cJSON_GetObjectItemCaseSensitive(root, key.c_str());
    APPSPAWN_CHECK_ONLY_EXPER(namesJson != nullptr, return);

    uint32_t count = (uint32_t)cJSON_GetArraySize(namesJson);
    for (uint32_t i = 0; i < count; ++i) {
        const char *name = cJSON_GetStringValue(cJSON_GetArrayItem(namesJson, i));
        APPSPAWN_CHECK_ONLY_EXPER(name != nullptr, continue);
        APPSPAWN_LOGV("name %{public}s", name);
        if (!names.count(name)) {
            names.insert(name);
        }
    }
}

static int GetNameSet(const cJSON *root, ParseJsonContext *context)
{
    GetNames(root, context->names, context->key);
    return 0;
}

static void PreloadModule(bool isHybrid)
{
    bool preloadEts = OHOS::system::GetBoolParameter("persist.appspawn.preloadets", DEFAULT_PRELOAD_ETS_VALUE);
    APPSPAWN_LOGI("LoadExtendLib: preloadets param value is %{public}s, isHybrid is %{public}s",
        preloadEts ? "true" : "false", isHybrid ? "true" : "false");

    OHOS::AbilityRuntime::Runtime::Options options;
    options.loadAce = true;
    options.preload = true;
    options.lang = OHOS::AbilityRuntime::Runtime::Language::JS;
    if (isHybrid && preloadEts) {
        options.lang = OHOS::AbilityRuntime::Runtime::Language::ETS;
    }
    auto runtime = OHOS::AbilityRuntime::Runtime::Create(options);
    if (!runtime) {
        APPSPAWN_LOGE("LoadExtendLib: Failed to create runtime");
        return;
    }

    ParseJsonContext jsContext = {{}, "napi"};
    (void)ParseJsonConfig("etc/appspawn", PRELOAD_JSON_CONFIG.c_str(), GetNameSet, &jsContext);
    for (std::string moduleName : jsContext.names) {
        APPSPAWN_LOGI("moduleName %{public}s", moduleName.c_str());
        runtime->PreloadSystemModule(moduleName);
    }

    ParseJsonContext etsContext = {{}, "class"};
    (void)ParseJsonConfig("etc/appspawn", PRELOAD_ETS_JSON_CONFIG.c_str(), GetNameSet, &etsContext);
    for (std::string className : etsContext.names) {
        APPSPAWN_LOGI("className %{public}s", className.c_str());
        runtime->PreloadSystemClass(className.c_str());
    }

    OHOS::AbilityRuntime::Runtime::SavePreloaded(std::move(runtime));
}

static void LoadExtendLib(bool isHybrid)
{
    const char *acelibdir = OHOS::Ace::AceForwardCompatibility::GetAceLibName();
    APPSPAWN_LOGI("LoadExtendLib: Start calling dlopen acelibdir");
    void *aceAbilityLib = dlopen(acelibdir, RTLD_NOW | RTLD_LOCAL);
    APPSPAWN_CHECK(aceAbilityLib != nullptr, return, "Fail to dlopen %{public}s, [%{public}s]", acelibdir, dlerror());
    APPSPAWN_LOGI("LoadExtendLib: Success to dlopen %{public}s", acelibdir);

    OHOS::AppExecFwk::MainThread::PreloadExtensionPlugin();
    bool preload = OHOS::system::GetBoolParameter("persist.appspawn.preload", DEFAULT_PRELOAD_VALUE);
    if (!preload) {
        APPSPAWN_LOGI("LoadExtendLib: Do not preload VM");
        return;
    }

    APPSPAWN_LOGI("LoadExtendLib: Start preload VM");
    SetTraceDisabled(true);
    PreloadModule(isHybrid);
    SetTraceDisabled(false);

    Resource::ResourceManager *systemResMgr = Resource::GetSystemResourceManagerNoSandBox();
    APPSPAWN_CHECK(systemResMgr != nullptr, return, "Fail to get system resource manager");
    APPSPAWN_LOGI("LoadExtendLib: End preload VM");
}

static void PreloadCJLibs(void)
{
    const char* cjEnvLibName = "libcj_environment.z.so";
    const char* cjEnvInitName = "OHOS_InitSpawnEnv";
    void* cjEnvLib = dlopen(cjEnvLibName, RTLD_NOW | RTLD_LOCAL);
    APPSPAWN_CHECK(cjEnvLib != nullptr, return, "Failed to dlopen %{public}s, [%{public}s]", cjEnvLibName, dlerror());
    auto symbol = dlsym(cjEnvLib, cjEnvInitName);
    if (!symbol) {
        dlclose(cjEnvLib);
        APPSPAWN_LOGE("Failed to dlsym %{public}s, [%{public}s]", cjEnvInitName, dlerror());
        return;
    }
    auto initSpawnEnv = reinterpret_cast<void (*)()>(symbol);
    initSpawnEnv();
}

static void LoadExtendCJLib(void)
{
    const char *acelibdir = OHOS::Ace::AceForwardCompatibility::GetAceLibName();
    APPSPAWN_LOGI("LoadExtendLib: Start calling dlopen acelibdir.");
    void *aceAbilityLib = dlopen(acelibdir, RTLD_NOW | RTLD_LOCAL);
    APPSPAWN_CHECK(aceAbilityLib != nullptr, return, "Fail to dlopen %{public}s, [%{public}s]", acelibdir, dlerror());
    APPSPAWN_LOGI("LoadExtendLib: Success to dlopen %{public}s", acelibdir);

    OHOS::AppExecFwk::MainThread::PreloadExtensionPlugin();

    PreloadCJLibs();
}

static int BuildFdInfoMap(const AppSpawnMsgNode *message, std::map<std::string, int> &fdMap, int isColdRun)
{
    APPSPAWN_CHECK_ONLY_EXPER(message != NULL && message->buffer != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(message->tlvOffset != NULL, return -1);
    int findFdIndex = 0;
    AppSpawnMsgReceiverCtx recvCtx;
    if (!isColdRun) {
        APPSPAWN_CHECK_ONLY_EXPER(message->connection != NULL, return -1);
        recvCtx = message->connection->receiverCtx;
        if (recvCtx.fdCount <= 0) {
            APPSPAWN_LOGI("no need to build fd info %{public}d, %{public}d", recvCtx.fds != NULL, recvCtx.fdCount);
            return 0;
        }
    }
    for (uint32_t index = TLV_MAX; index < (TLV_MAX + message->tlvCount); index++) {
        if (message->tlvOffset[index] == INVALID_OFFSET) {
            return -1;
        }
        uint8_t *data = message->buffer + message->tlvOffset[index];
        if (((AppSpawnTlv *)data)->tlvType != TLV_MAX) {
            continue;
        }
        AppSpawnTlvExt *tlv = (AppSpawnTlvExt *)data;
        if (strcmp(tlv->tlvName, MSG_EXT_NAME_APP_FD) != 0) {
            continue;
        }
        std::string key((char *)data + sizeof(AppSpawnTlvExt));
        if (isColdRun) {
            std::string envKey = std::string(APP_FDENV_PREFIX) + key;
            char *fdChar = getenv(envKey.c_str());
            APPSPAWN_CHECK(fdChar != NULL, continue, "getfd from env failed %{public}s", envKey.c_str());
            int fd = atoi(fdChar);
            APPSPAWN_CHECK(fd > 0, continue, "getfd from env atoi errno %{public}s,%{public}d", envKey.c_str(), fd);
            fdMap[key] = fd;
        } else {
            APPSPAWN_CHECK(findFdIndex < recvCtx.fdCount && recvCtx.fds[findFdIndex] > 0,
                return -1, "invalid fd info  %{public}d %{public}d", findFdIndex, recvCtx.fds[findFdIndex]);
            fdMap[key] = recvCtx.fds[findFdIndex++];
            if (findFdIndex >= recvCtx.fdCount) {
                break;
            }
        }
    }
    return 0;
}

APPSPAWN_STATIC void ClearEnvAndReturnSuccess(AppSpawnContent *content, AppSpawnClient *client)
{
    AppSpawningCtx *property = (AppSpawningCtx *)client;
    APPSPAWN_CHECK(content != NULL && property != NULL, return, "invalid param in clearEnv");
    int fd = property->forkCtx.fd[1];
    property->forkCtx.fd[1] = -1;
    AppSpawnEnvClear(content, client);
    APPSPAWN_CHECK(fd >= 0, return, "invalid fd for notify parent");
    int ret = 0;
    ssize_t written = write(fd, &ret, sizeof(ret));
    (void)close(fd);
    APPSPAWN_DUMPI("ClearEnvAndReturnSuccess %{public}zd", written);
}

APPSPAWN_STATIC int RunChildThread(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    std::string checkExit;
    if (OHOS::system::GetBoolParameter("persist.init.debug.checkexit", true)) {
        checkExit = std::to_string(getpid());
    }
#ifdef ARKWEB_UTILS_ENABLE
    const char* bundleName = GetBundleName(property);
    std::string bundleNameStr = (bundleName != NULL) ? std::string(bundleName) : "";
    OHOS::ArkWeb::SelectWebcoreBeforeProcessRun(bundleNameStr);
#endif
    setenv(APPSPAWN_CHECK_EXIT, checkExit.c_str(), true);
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_CHILDPROCESS)) {
        std::map<std::string, int> fdMap;
        BuildFdInfoMap(property->message, fdMap, IsColdRunMode(content));
        ClearEnvAndReturnSuccess((AppSpawnContent *)&content->content, (AppSpawnClient *)&property->client);
        OHOS::AppExecFwk::MainThread::StartChild(fdMap);
    } else {
        ClearEnvAndReturnSuccess((AppSpawnContent *)&content->content, (AppSpawnClient *)&property->client);
        OHOS::AppExecFwk::MainThread::Start();
    }
    unsetenv(APPSPAWN_CHECK_EXIT);
    return 0;
}

APPSPAWN_STATIC int RunChildByRenderCmd(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    uint32_t len = 0;
    char *renderCmd = reinterpret_cast<char *>(GetAppPropertyExt(property, MSG_EXT_NAME_RENDER_CMD, &len));
    if (renderCmd == NULL || !IsDeveloperModeOn(property)) {
        APPSPAWN_LOGE("Denied launching a native process: not in developer mode");
        return -1;
    }
    APPSPAWN_LOGI("renderCmd %{public}s", renderCmd);
    std::vector<std::string> args;
    std::string command(renderCmd);
    CommandLexer lexer(command);
    if (!lexer.GetAllArguments(args)) {
        return -1;
    }
    if (args.empty()) {
        APPSPAWN_LOGE("Failed to run a native process: empty command %{public}s", renderCmd);
        return -1;
    }
    std::vector<char *> options;
    for (const auto &arg : args) {
        options.push_back(const_cast<char *>(arg.c_str()));
    }
    options.push_back(nullptr);
    // clear appspawn env, do not user any content and property
    ClearEnvAndReturnSuccess((AppSpawnContent *)&content->content, (AppSpawnClient *)&property->client);
    execvp(args[0].c_str(), options.data());
    // If it succeeds calling execvp, it never returns.
    int err = errno;
    APPSPAWN_LOGE("Failed to launch a native process with execvp: %{public}s", strerror(err));
    return 0;
}

static int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_CHECK(client != NULL && content != NULL, return -1, "Invalid client");
    AppSpawningCtx *property = reinterpret_cast<AppSpawningCtx *>(client);
    int ret = 0;
    if (GetAppSpawnMsgType(property) == MSG_SPAWN_NATIVE_PROCESS) {
        ret = RunChildByRenderCmd(reinterpret_cast<AppSpawnMgr *>(content), property);
    } else {
        ret = RunChildThread(reinterpret_cast<AppSpawnMgr *>(content), property);
    }
    return ret;
}

APPSPAWN_STATIC int PreLoadAppSpawn(AppSpawnMgr *content)
{
    if (IsNWebSpawnMode(content)) {
        return 0;
    }
    // register
    RegChildLooper(&content->content, RunChildProcessor);
    if (strcmp(content->content.longProcName, CJAPPSPAWN_SERVER_NAME) == 0) {
        LoadExtendCJLib();
        return 0;
    }

    bool isHybrid = IsHybridSpawnMode(content) ? true : false;
    APPSPAWN_LOGI("PreLoadAppSpawn: mode %{public}d, isHybrid is %{public}d", content->content.mode, isHybrid);
    LoadExtendLib(isHybrid);
    return 0;
}

APPSPAWN_STATIC int DoDlopenLibs(const cJSON *root, ParseJsonContext *context)
{
    cJSON *systemLibs = cJSON_GetObjectItemCaseSensitive(root, "systemLib");
    if (systemLibs == nullptr) {
        return 0;
    }

    uint32_t libsCount = (uint32_t)cJSON_GetArraySize(systemLibs);
    for (uint32_t i = 0; i < libsCount; ++i) {
        const char *libName = cJSON_GetStringValue(cJSON_GetArrayItem(systemLibs, i));
        char reaLibPath[PATH_MAX] = {0};
        if (libName == nullptr || realpath(libName, reaLibPath) == nullptr) {
            continue;
        }
        APPSPAWN_LOGV("reaLibPath %{public}s", reaLibPath);
        void *lib = dlopen(reaLibPath, RTLD_LAZY);
        if (lib == nullptr) {
            APPSPAWN_LOGE("FAILED to dlopen %{public}s %{public}s", libName, dlerror());
        }
    }
    return 0;
}

APPSPAWN_STATIC int DlopenAppSpawn(AppSpawnMgr *content)
{
    if (!(IsAppSpawnMode(content) || IsHybridSpawnMode(content))) {
        return 0;
    }
    (void)ParseJsonConfig("etc/appspawn", SYSTEMLIB_JSON, DoDlopenLibs, nullptr);

#ifdef ARKWEB_UTILS_ENABLE
    OHOS::ArkWeb::PreloadArkWebLibForBrowser();
#endif
    APPSPAWN_LOGI("DlopenAppSpawn: Start reclaim file cache");
    OHOS::Ace::AceForwardCompatibility::ReclaimFileCache(getpid());
    return 0;
}

APPSPAWN_STATIC int ProcessSpawnDlopenMsg(AppSpawnMgr *content)
{
#ifdef ARKWEB_UTILS_ENABLE
    // dlopen libarkweb_engine.so
    OHOS::ArkWeb::DlopenArkWebLib();
    OHOS::Ace::AceForwardCompatibility::ReclaimFileCache(getpid());
#endif
    return 0;
}

APPSPAWN_STATIC int ProcessSpawnDlcloseMsg(AppSpawnMgr *content)
{
#ifdef ARKWEB_UTILS_ENABLE
    // dlclose libarkweb_engine.so
    return OHOS::ArkWeb::DlcloseArkWebLib();
#endif
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load ace module ...");
    AddPreloadHook(HOOK_PRIO_HIGHEST, PreLoadAppSpawn);
    AddPreloadHook(HOOK_PRIO_HIGHEST, DlopenAppSpawn);
    AddServerStageHook(STAGE_SERVER_ARKWEB_PRELOAD, HOOK_PRIO_COMMON, ProcessSpawnDlopenMsg);
    AddServerStageHook(STAGE_SERVER_ARKWEB_UNLOAD, HOOK_PRIO_COMMON, ProcessSpawnDlcloseMsg);
}
