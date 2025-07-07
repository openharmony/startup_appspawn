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

#include "app_spawn_hisysevent_stub.h"
#include "appspawn_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

bool g_CreateTimerStatus = true;

void SetCreateTimerStatus(bool status)
{
    if (status) {
        g_CreateTimerStatus = true;
    } else {
        g_CreateTimerStatus = false;
    }
}

int CreateHisysTimerLoop(AppSpawnHisyseventInfo *hisyseventInfo)
{
    return 0;
}

#ifdef __cplusplus
}
#endif