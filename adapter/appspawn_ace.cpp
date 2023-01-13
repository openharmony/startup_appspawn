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
    const char *acelibdir("libace.z.so");
    APPSPAWN_LOGI("LoadExtendLib: Start calling dlopen acelibdir.");
#ifndef APPSPAWN_TEST
    void *aceAbilityLib = NULL;
    aceAbilityLib = dlopen(acelibdir, RTLD_NOW | RTLD_GLOBAL);
    APPSPAWN_CHECK(aceAbilityLib != NULL, return, "Fail to dlopen %s, [%s]", acelibdir, dlerror());
#endif
    APPSPAWN_LOGI("LoadExtendLib: Success to dlopen %s", acelibdir);
    APPSPAWN_LOGI("LoadExtendLib: End calling dlopen");

#if defined(APPSPAWN_TEST) || defined(ASAN_DETECTOR)
    bool preload = OHOS::system::GetBoolParameter("const.appspawn.preload", false);
#else
    bool preload = OHOS::system::GetBoolParameter("const.appspawn.preload", true);
#endif
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
    runtime->PreloadSystemModule("request");

    // Save preloaded runtime
    OHOS::AbilityRuntime::Runtime::SavePreloaded(std::move(runtime));
    SetTraceDisabled(false);
    APPSPAWN_LOGI("LoadExtendLib: End preload JS VM");
}

void RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGI("AppExecFwk::MainThread::Start");
#ifndef APPSPAWN_TEST
    OHOS::AppExecFwk::MainThread::Start();
#endif
}
