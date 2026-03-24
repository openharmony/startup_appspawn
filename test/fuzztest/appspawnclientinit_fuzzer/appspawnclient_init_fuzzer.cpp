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

#include "appspawnclient_init_fuzzer.h"
#include <string>
#include "appspawn.h"
#include "fuzzer/FuzzedDataProvider.h"

namespace OHOS {
    int FuzzAppSpawnClientInit(const uint8_t *data, size_t size)
    {
        std::string serviceName(reinterpret_cast<const char*>(data), size);
        AppSpawnClientHandle handle = nullptr;
        return AppSpawnClientInit(serviceName.c_str(), &handle);
    }

    int FuzzAppSpawnClientDestroy(const uint8_t *data, size_t size)
    {
        std::string serviceName(reinterpret_cast<const char*>(data), size);
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(serviceName.c_str(), &handle) != 0) {
            return -1;
        }
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgCreate(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
            return -1;
        }
        FuzzedDataProvider provider(data, size);
        int type = provider.ConsumeIntegralInRange<int>(static_cast<int>(MSG_APP_SPAWN),
            static_cast<int>(MAX_TYPE_INVALID));
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(type);
        std::string processName(reinterpret_cast<const char*>(data), size);
        AppSpawnReqMsgHandle reqHandle = nullptr;
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnTerminateMsgCreate(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
            return -1;
        }
        FuzzedDataProvider provider(data, size);
        pid_t pid = provider.ConsumeIntegral<pid_t>();
        AppSpawnReqMsgHandle reqHandle = nullptr;
        int type = provider.ConsumeIntegralInRange<int>(static_cast<int>(MSG_APP_SPAWN),
            static_cast<int>(MAX_TYPE_INVALID));
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(type);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        (void)AppSpawnTerminateMsgCreate(pid, &reqHandle);
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnClientSendMsg(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
            return -1;
        }
        AppSpawnReqMsgHandle reqHandle = nullptr;
        FuzzedDataProvider provider(data, size);
        AppSpawnResult appResult = {provider.ConsumeIntegral<int>(), 0};
        int type = provider.ConsumeIntegralInRange<int>(static_cast<int>(MSG_APP_SPAWN),
            static_cast<int>(MAX_TYPE_INVALID));
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(type);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        (void)AppSpawnClientSendMsg(handle, reqHandle, &appResult);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgFree(const uint8_t *data, size_t size)
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
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

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
        std::string processName = provider.ConsumeRandomLengthString();
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        uint32_t bundleIndex = provider.ConsumeIntegral<uint64_t>();
        std::string bundleName = provider.ConsumeRandomLengthString();
        (void)AppSpawnReqMsgSetBundleInfo(reqHandle, bundleIndex, bundleName.c_str());
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzAppSpawnClientInit(data, size);
    OHOS::FuzzAppSpawnClientDestroy(data, size);
    OHOS::FuzzAppSpawnReqMsgCreate(data, size);
    OHOS::FuzzAppSpawnTerminateMsgCreate(data, size);
    OHOS::FuzzAppSpawnReqMsgFree(data, size);
    OHOS::FuzzAppSpawnClientSendMsg(data, size);
    OHOS::FuzzAppSpawnReqMsgSetBundleInfo(data, size);
    return 0;
}
