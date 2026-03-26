/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "native_adapter_mock_test.h"

#include <string>

static uint32_t g_checkExitParamResult = 0;
void SetBoolParamResult(const char *key, bool flag)
{
    if (strcmp(key, "persist.init.debug.checkexit") == 0) {
        g_checkExitParamResult = flag;
    }
}

namespace OHOS {
namespace system {
    bool GetBoolParameter(const std::string &key, bool def)
    {
        if (strcmp(key.c_str(), "persist.init.debug.checkexit") == 0) {
            return g_checkExitParamResult;
        }
        return def;
    }
}  // namespace system
}  // namespace OHOS
