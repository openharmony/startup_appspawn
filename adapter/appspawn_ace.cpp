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

#include "appspawn_adapter.h"

#include "hitrace_meter.h"
#include "js_runtime.h"
#include "parameters.h"
#include "runtime.h"

#include "foundation/ability/ability_runtime/interfaces/kits/native/appkit/app/main_thread.h"

void LoadExtendLib(AppSpawnContent *content)
{
#ifdef __aarch64__
    const char *acelibdir("/system/lib64/libace.z.so");
#else
    const char *acelibdir("/system/lib/libace.z.so");
#endif
    APPSPAWN_LOGI("LoadExtendLib: Start calling dlopen acelibdir.");
#ifndef APPSPAWN_TEST
    void *aceAbilityLib = NULL;
    aceAbilityLib = dlopen(acelibdir, RTLD_NOW | RTLD_GLOBAL);
    APPSPAWN_CHECK(aceAbilityLib != NULL, return, "Fail to dlopen %s, [%s]", acelibdir, dlerror());
#endif
    APPSPAWN_LOGI("LoadExtendLib: Success to dlopen %s", acelibdir);
    APPSPAWN_LOGI("LoadExtendLib: End calling dlopen");

    bool preload = OHOS::system::GetBoolParameter("const.appspawn.preload", false);
    if (!preload) {
        APPSPAWN_LOGI("LoadExtendLib: Do not preload JS VM");
        return;
    }

    APPSPAWN_LOGI("LoadExtendLib: Start preload JS VM");
    SetTraceDisabled(true);
    OHOS::AbilityRuntime::Runtime::Options options;
    options.lang = OHOS::AbilityRuntime::Runtime::Language::JS;
    options.loadAce = true;
    options.preload = true;

    auto runtime = OHOS::AbilityRuntime::Runtime::Create(options);
    if (!runtime) {
        APPSPAWN_LOGE("LoadExtendLib: Failed to create runtime");
        SetTraceDisabled(false);
        return;
    }

    // Preload napi module
    runtime->PreloadSystemModule("application.Ability");
    runtime->PreloadSystemModule("application.Context");
    runtime->PreloadSystemModule("application.AbilityContext");

    // Save preloaded runtime
    OHOS::AbilityRuntime::Runtime::SavePreloaded(std::move(runtime));
    SetTraceDisabled(false);
    APPSPAWN_LOGI("LoadExtendLib: End preload JS VM");
}

void RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGI("AppExecFwk::MainThread::Start");
#ifndef APPSPAWN_TEST
    if (client->flags == UI_SERVICE_DIALOG) {
        APPSPAWN_LOGI("UiServiceDialogImpl dlopen and call InitAceDialog()");
        constexpr char libName[] = "libuiservicedialog.z.so";
        constexpr char funName[] = "OHOS_ACE_InitDialog";
        void* libHandle = dlopen(libName, RTLD_LAZY);
        if (libHandle == nullptr) {
            APPSPAWN_LOGE("Failed to open %s, error: %s", libName, dlerror());
            return;
        }
        auto func = reinterpret_cast<void(*)()>(dlsym(libHandle, funName));
        if (func == nullptr) {
            APPSPAWN_LOGE("Failed to get func %s, error: %s", funName, dlerror());
            return;
        }
        func();
    } else {
        OHOS::AppExecFwk::MainThread::Start();
    }
#endif
}
