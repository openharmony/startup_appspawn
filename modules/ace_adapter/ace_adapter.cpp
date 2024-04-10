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
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "command_lexer.h"
#include "config_policy_utils.h"
#include "hitrace_meter.h"
#include "js_runtime.h"
#include "json_utils.h"
#include "parameters.h"
#include "resource_manager.h"
#ifndef APPSPAWN_TEST
#include "ace_forward_compatibility.h"
#include "foundation/ability/ability_runtime/interfaces/kits/native/appkit/app/main_thread.h"
#include "runtime.h"
#endif

using namespace OHOS::AppSpawn;
using namespace OHOS::Global;

#ifdef ASAN_DETECTOR
static const bool DEFAULT_PRELOAD_VALUE = false;
#else
static const bool DEFAULT_PRELOAD_VALUE = true;
#endif
static const std::string PRELOAD_JSON_CONFIG("/appspawn_preload.json");

typedef struct TagParseJsonContext {
    std::set<std::string> modules;
} ParseJsonContext;

static void GetModules(const cJSON *root, std::set<std::string> &modules)
{
    // no config
    cJSON *modulesJson = cJSON_GetObjectItemCaseSensitive(root, "napi");
    if (modulesJson == nullptr) {
        return;
    }

    uint32_t moduleCount = cJSON_GetArraySize(modulesJson);
    for (uint32_t i = 0; i < moduleCount; ++i) {
        const char *moduleName = cJSON_GetStringValue(cJSON_GetArrayItem(modulesJson, i));
        if (moduleName == nullptr) {
            continue;
        }
        APPSPAWN_LOGV("moduleName %{public}s", moduleName);
        if (!modules.count(moduleName)) {
            modules.insert(moduleName);
        }
    }
}

static int GetModuleSet(const cJSON *root, ParseJsonContext *context)
{
    GetModules(root, context->modules);
    return 0;
}

static void PreloadModule(void)
{
    OHOS::AbilityRuntime::Runtime::Options options;
    options.lang = OHOS::AbilityRuntime::Runtime::Language::JS;
    options.loadAce = true;
    options.preload = true;

    auto runtime = OHOS::AbilityRuntime::Runtime::Create(options);
    if (!runtime) {
        APPSPAWN_LOGE("LoadExtendLib: Failed to create runtime");
        return;
    }

    ParseJsonContext context = {};
    (void)ParseJsonConfig("etc/appspawn", PRELOAD_JSON_CONFIG.c_str(), GetModuleSet, &context);
    for (std::string moduleName : context.modules) {
        APPSPAWN_LOGI("moduleName %{public}s", moduleName.c_str());
        runtime->PreloadSystemModule(moduleName);
    }
    // Save preloaded runtime
    OHOS::AbilityRuntime::Runtime::SavePreloaded(std::move(runtime));
}

static void LoadExtendLib(void)
{
    const char *acelibdir = OHOS::Ace::AceForwardCompatibility::GetAceLibName();
    APPSPAWN_LOGI("LoadExtendLib: Start calling dlopen acelibdir.");
    void *aceAbilityLib = dlopen(acelibdir, RTLD_NOW | RTLD_LOCAL);
    APPSPAWN_CHECK(aceAbilityLib != nullptr, return, "Fail to dlopen %{public}s, [%{public}s]", acelibdir, dlerror());
    APPSPAWN_LOGI("LoadExtendLib: Success to dlopen %{public}s", acelibdir);

    OHOS::AppExecFwk::MainThread::PreloadExtensionPlugin();
    bool preload = OHOS::system::GetBoolParameter("persist.appspawn.preload", DEFAULT_PRELOAD_VALUE);
    if (!preload) {
        APPSPAWN_LOGI("LoadExtendLib: Do not preload JS VM");
        return;
    }

    APPSPAWN_LOGI("LoadExtendLib: Start preload JS VM");
    SetTraceDisabled(true);
    PreloadModule();
    SetTraceDisabled(false);

    Resource::ResourceManager *systemResMgr = Resource::GetSystemResourceManagerNoSandBox();
    APPSPAWN_CHECK(systemResMgr != nullptr, return, "Fail to get system resource manager");
    APPSPAWN_LOGI("LoadExtendLib: End preload JS VM");
}

static int RunChildThread(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    AppSpawnEnvClear((AppSpawnContent *)&content->content, (AppSpawnClient *)&property->client);
    std::string checkExit;
    if (OHOS::system::GetBoolParameter("persist.init.debug.checkexit", true)) {
        checkExit = std::to_string(getpid());
    }
    setenv(APPSPAWN_CHECK_EXIT, checkExit.c_str(), true);
    OHOS::AppExecFwk::MainThread::Start();
    unsetenv(APPSPAWN_CHECK_EXIT);
    return 0;
}

static int RunChildByRenderCmd(const AppSpawnMgr *content, const AppSpawningCtx *property)
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
    AppSpawnEnvClear((AppSpawnContent *)&content->content, (AppSpawnClient *)&property->client);
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

static int PreLoadAppSpawn(AppSpawnMgr *content)
{
    if (IsNWebSpawnMode(content)) {
        return 0;
    }
    // register
    RegChildLooper(&content->content, RunChildProcessor);
    LoadExtendLib();
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load ace module ...");
    AddPreloadHook(HOOK_PRIO_HIGHEST, PreLoadAppSpawn);
}
