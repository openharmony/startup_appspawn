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

#ifndef APPSPAWN_EXIT_LOCK_STUBH
#define APPSPAWN_EXIT_LOCK_STUBH

#include <cstring>
#include <cstdint>
#include "appspawn_manager.h"
#include "appspawn_hook.h"

#define LOCK_STATUS_UNLOCK_MAX 5

void ClearEnvAndReturnSuccess(AppSpawnContent *content, AppSpawnClient *client);

#ifdef __cplusplus
extern "C" {
#endif

void ProcessExit(int code);
void RegChildLooper(AppSpawnContent *content, ChildLoop loop);

#ifdef __cplusplus
}
bool IsUnlockStatus(uint32_t uid);
#endif

#endif