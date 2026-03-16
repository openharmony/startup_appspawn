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

#include "appspawnclient_budle_fuzzer.h"
#include <string>
#include "appspawn.h"
#include "fuzzer/FuzzedDataProvider.h"

namespace OHOS {
    int FuzzAppSpawnReqMsgSetBundleInfo(const uint8_t *data, size_t size)
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
        uint32_t bundleIndex = provider.ConsumeIntegral<uint32_t>();
        std::string bundleName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgSetBundleInfo(reqHandle, bundleIndex, bundleName.c_str());
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgSetAppFlag(const uint8_t *data, size_t size)
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
        int flags = provider.ConsumeIntegralInRange<int>(static_cast<int>(APP_FLAGS_COLD_BOOT),
            static_cast<int>(MAX_FLAGS_INDEX));
        AppFlagsIndex flagIndex = static_cast<AppFlagsIndex>(flags);
        (void)AppSpawnReqMsgSetAppFlag(reqHandle, flagIndex);
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgSetAppDacInfo(const uint8_t *data, size_t size)
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
        AppDacInfo dacInfo = {};
        (void)AppSpawnReqMsgSetAppDacInfo(reqHandle, &dacInfo);
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgSetAppDomainInfo(const uint8_t *data, size_t size)
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
        std::string apl(reinterpret_cast<const char*>(data), size);
        uint32_t hapFlags = provider.ConsumeIntegral<uint32_t>();
        (void)AppSpawnReqMsgSetAppDomainInfo(reqHandle, hapFlags, apl.c_str());
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgSetAppInternetPermissionInfo(const uint8_t *data, size_t size)
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
        uint8_t allow = provider.ConsumeIntegral<uint8_t>();
        uint8_t setAllow = provider.ConsumeIntegral<uint8_t>();
        (void)AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, allow, setAllow);
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgSetAppAccessToken(const uint8_t *data, size_t size)
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
        uint64_t accessTokenIdEx = provider.ConsumeIntegral<uint64_t>();
        (void)AppSpawnReqMsgSetAppAccessToken(reqHandle, accessTokenIdEx);
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzAppSpawnReqMsgSetBundleInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppFlag(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppDacInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppDomainInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppInternetPermissionInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppAccessToken(data, size);
    return 0;
}
