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

#include "appspawnclient_string_fuzzer.h"
#include <string>
#include "appspawn.h"
#include "fuzzer/FuzzedDataProvider.h"

namespace OHOS {
    int FuzzAppSpawnReqMsgAddStringInfo(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
            return -1;
        }
        AppSpawnReqMsgHandle reqHandle = nullptr;
        std::string processName(reinterpret_cast<const char*>(data), size);
        FuzzedDataProvider provider(data, size);
        int type = provider.ConsumeIntegralInRange<int>(static_cast<int>(MSG_APP_SPAWN),
            static_cast<int>(MAX_TYPE_INVALID));
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(type);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        (void)AppSpawnReqMsgAddStringInfo(reqHandle, processName.c_str(), processName.c_str());
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgSetAppOwnerId(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
            return -1;
        }
        AppSpawnReqMsgHandle reqHandle = nullptr;
        FuzzedDataProvider provider(data, size);
        int type = provider.ConsumeIntegralInRange<int>(static_cast<int>(MSG_APP_SPAWN),
            static_cast<int>(MAX_TYPE_INVALID));
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(type);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        std::string ownerId(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgSetAppOwnerId(reqHandle, ownerId.c_str());
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgAddPermission(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
            return -1;
        }
        AppSpawnReqMsgHandle reqHandle = nullptr;
        FuzzedDataProvider provider(data, size);
        int type = provider.ConsumeIntegralInRange<int>(static_cast<int>(MSG_APP_SPAWN),
            static_cast<int>(MAX_TYPE_INVALID));
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(type);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        std::string permission(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgAddPermission(reqHandle, permission.c_str());
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgAddExtInfo(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
            return -1;
        }
        AppSpawnReqMsgHandle reqHandle = nullptr;
        FuzzedDataProvider provider(data, size);
        int type = provider.ConsumeIntegralInRange<int>(static_cast<int>(MSG_APP_SPAWN),
            static_cast<int>(MAX_TYPE_INVALID));
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(type);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        (void)AppSpawnReqMsgAddExtInfo(reqHandle, processName.c_str(), data, static_cast<uint32_t>(size));
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzGetPermissionIndex(const uint8_t *data, size_t size)
    {
        std::string permission(reinterpret_cast<const char*>(data), size);
        return GetPermissionIndex(nullptr, permission.c_str());
    }

    int FuzzGetMaxPermissionIndex(const uint8_t *data, size_t size)
    {
        std::string serviceName(reinterpret_cast<const char*>(data), size);
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(serviceName.c_str(), &handle) != 0) {
            return -1;
        }
        return GetMaxPermissionIndex(handle);
    }

    int FuzzGetPermissionByIndex(const uint8_t *data, size_t size)
    {
        if ((data == nullptr) || (size < sizeof(int32_t))) {
            return -1;
        }

        int32_t index = *(reinterpret_cast<const int32_t*>(data));
        if (GetPermissionByIndex(nullptr, index) == nullptr) {
            return -1;
        }
        return 0;
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzAppSpawnReqMsgSetAppOwnerId(data, size);
    OHOS::FuzzAppSpawnReqMsgAddPermission(data, size);
    OHOS::FuzzAppSpawnReqMsgAddExtInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgAddStringInfo(data, size);
    OHOS::FuzzGetPermissionIndex(data, size);
    OHOS::FuzzGetMaxPermissionIndex(data, size);
    OHOS::FuzzGetPermissionByIndex(data, size);
    return 0;
}
