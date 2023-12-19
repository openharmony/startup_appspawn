/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "param_helper.h"

#include <string.h>

#include "parameter.h"

#define DEVEVELOPER_MODE_KEY              "const.security.developermode.state"
#define DEVEVELOPER_MODE_VALUE_ON         "true"
#define DEVEVELOPER_MODE_VALUE_DEFAULT    ""

int IsDeveloperModeOn()
{
    char value[10] = {0};
    int ret = GetParameter(DEVEVELOPER_MODE_KEY, DEVEVELOPER_MODE_VALUE_DEFAULT,
        value, sizeof(value));
    return ret > 0 && strncmp(value, DEVEVELOPER_MODE_VALUE_ON, sizeof(value)) == 0;
}
