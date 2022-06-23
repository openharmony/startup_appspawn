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

#include "foundation/ability/ability_runtime/interfaces/kits/native/appkit/app/main_thread.h"

void LoadExtendLib(AppSpawnContent *content)
{
#ifdef __aarch64__
    const char *acelibdir("/system/lib64/libace.z.so");
#else
    const char *acelibdir("/system/lib/libace.z.so");
#endif
    APPSPAWN_LOGI("MainThread::LoadAbilityLibrary. Start calling dlopen acelibdir.");
#ifndef APPSPAWN_TEST
    void *AceAbilityLib = NULL;
    AceAbilityLib = dlopen(acelibdir, RTLD_NOW | RTLD_GLOBAL);
    APPSPAWN_CHECK(AceAbilityLib != NULL, return, "Fail to dlopen %s, [%s]", acelibdir, dlerror());
#endif
    APPSPAWN_LOGI("Success to dlopen %s", acelibdir);
    APPSPAWN_LOGI("MainThread::LoadAbilityLibrary. End calling dlopen");
}

void RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGI("AppExecFwk::MainThread::Start");
#ifndef APPSPAWN_TEST
    OHOS::AppExecFwk::MainThread::Start();
#endif
}
