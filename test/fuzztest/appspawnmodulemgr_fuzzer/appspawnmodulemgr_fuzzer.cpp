/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "appspawnmodulemgr_fuzzer.h"
#include <string>
#include "appspawn_modulemgr.h"

namespace OHOS {

    int FuzzAppSpawnModuleMgrInstall(const uint8_t *data, size_t size)
    {
        std::string moduleName(reinterpret_cast<const char*>(data), size);
        return AppSpawnModuleMgrInstall(moduleName.c_str());
    }

    int FuzzAppSpawnLoadAutoRunModules(const uint8_t *data, size_t size)
    {
        int type = static_cast<int>(size);
        return AppSpawnLoadAutoRunModules(type);
    }

    int FuzzAppSpawnModuleMgrUnInstall(const uint8_t *data, size_t size)
    {
        int type = static_cast<int>(size);
        return AppSpawnModuleMgrUnInstall(type);
    }

    int FuzzzAppSpawnModuleMgrUnInstall(const uint8_t *data, size_t size)
    {
        AppSpawnModuleMgrUnInstall();
        return 0;
    }

}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzAppSpawnModuleMgrInstall(data, size);
    OHOS::FuzzAppSpawnLoadAutoRunModules(data, size);
    OHOS::FuzzAppSpawnModuleMgrUnInstall(data, size);
    OHOS::FuzzzAppSpawnModuleMgrUnInstall(data, size);
    return 0;
}
