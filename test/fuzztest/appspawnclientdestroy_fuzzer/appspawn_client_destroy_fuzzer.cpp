/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "appspawn_client_destroy_fuzzer.h"
#include <string>
#include "appspawn.h"

namespace OHOS {
    int FuzzAppSpawnClientDestroy(const uint8_t *data, size_t size)
    {
        std::string serviceName(reinterpret_cast<const char*>(data), size);
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(serviceName.c_str(), &handle) != 0) {
            return -1;
        }
        return AppSpawnClientDestroy(handle);
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    OHOS::FuzzAppSpawnClientDestroy(data, size);
    return 0;
}
