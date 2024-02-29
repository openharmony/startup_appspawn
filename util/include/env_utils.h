/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef ENV_UTILS_H
#define ENV_UTILS_H

#include <stdint.h>

#include "appspawn_server.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int32_t SetEnvInfo(struct AppSpawnContent_ *content, AppSpawnClient *client);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // ENV_UTILS_H
