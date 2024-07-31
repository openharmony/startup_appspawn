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

#include "appspawnmanager_fuzzer.h"
#include <string>
#include "appspawn_manager.h"
/**
* @brief App Spawn Mgr object op
*
*/
namespace OHOS {

    int FuzzCreateAppSpawnMgr(const uint8_t *data, size_t size)
    {
        AppSpawnMgr *mgr = CreateAppSpawnMgr(size);
        if (mgr) {
            return 1;
        }
        return -1;
    }

    int FuzzGetAppSpawnMgr(const uint8_t *data, size_t size)
    {
        AppSpawnMgr *mgr = GetAppSpawnMgr();
        if (mgr) {
            return 1;
        }
        return -1;
    }

    int FuzzDeleteAppSpawnMgr(const uint8_t *data, size_t size)
    {
        AppSpawnMgr *mgr = GetAppSpawnMgr();
        if (mgr) {
            DeleteAppSpawnMgr(mgr)
            return 1;
        }
        return -1;
    }

    int FuzzGetAppSpawnContent(const uint8_t *data, size_t size)
    {
        AppSpawnContent *content = GetAppSpawnContent();
        if (mgr) {
            return 1;
        }
        return -1;
    }

}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzCreateAppSpawnMgr(data, size);
    OHOS::FuzzGetAppSpawnMgr(data, size);
    OHOS::FuzzDeleteAppSpawnMgr(data, size);
    OHOS::FuzzGetAppSpawnContent(data, size);
    return 0;
}
