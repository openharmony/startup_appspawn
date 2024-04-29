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

#include "appspawnclient_fuzzer.h"
#include <string>
#include "appspawn.h"

namespace OHOS {
    int FuzzAppSpawnClientInit(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        return AppSpawnClientInit(name, &handle);
    }

    int FuzzAppSpawnClientDestroy(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
        std::string processName(reinterpret_cast<const char*>(data), size);
        AppSpawnReqMsgHandle reqHandle = nullptr;
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        AppSpawnReqMsgFree(reqHandle);
        return AppSpawnClientDestroy(handle);
    }

    int FuzzAppSpawnReqMsgAddStringInfo(const uint8_t *data, size_t size)
    {
        const char *name = APPSPAWN_SERVER_NAME;
        AppSpawnClientHandle handle = nullptr;
        if (AppSpawnClientInit(name, &handle) != 0) {
            return -1;
        }
        AppSpawnReqMsgHandle reqHandle = nullptr;
        std::string processName(reinterpret_cast<const char*>(data), size);
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        (void)AppSpawnReqMsgAddStringInfo(reqHandle, processName.c_str(), processName.c_str());
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
        pid_t pid = static_cast<pid_t>(size);
        AppSpawnReqMsgHandle reqHandle = nullptr;
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
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
        AppSpawnResult appResult = {static_cast<int>(size), 0};
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        uint32_t bundleIndex = static_cast<uint32_t>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        AppFlagsIndex flagIndex = static_cast<AppFlagsIndex>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        std::string apl(reinterpret_cast<const char*>(data), size);
        uint32_t hapFlags = static_cast<uint32_t>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        uint8_t allow = static_cast<uint8_t>(size);
        uint8_t setAllow = static_cast<uint8_t>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
        std::string processName(reinterpret_cast<const char*>(data), size);
        (void)AppSpawnReqMsgCreate(msgType, processName.c_str(), &reqHandle);
        uint64_t accessTokenIdEx = static_cast<uint64_t>(size);
        (void)AppSpawnReqMsgSetAppAccessToken(reqHandle, accessTokenIdEx);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
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
        AppSpawnMsgType msgType = static_cast<AppSpawnMsgType>(size);
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
        return GetMaxPermissionIndex(nullptr);
    }

    int FuzzGetPermissionByIndex(const uint8_t *data, size_t size)
    {
        int32_t index = static_cast<int32_t>(size);
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
    OHOS::FuzzAppSpawnClientInit(data, size);
    OHOS::FuzzAppSpawnClientDestroy(data, size);
    OHOS::FuzzAppSpawnReqMsgCreate(data, size);
    OHOS::FuzzAppSpawnTerminateMsgCreate(data, size);
    OHOS::FuzzAppSpawnReqMsgFree(data, size);
    OHOS::FuzzAppSpawnClientSendMsg(data, size);
    OHOS::FuzzAppSpawnReqMsgSetBundleInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppFlag(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppDacInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppDomainInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppInternetPermissionInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppAccessToken(data, size);
    OHOS::FuzzAppSpawnReqMsgSetAppOwnerId(data, size);
    OHOS::FuzzAppSpawnReqMsgAddPermission(data, size);
    OHOS::FuzzAppSpawnReqMsgAddExtInfo(data, size);
    OHOS::FuzzAppSpawnReqMsgAddStringInfo(data, size);
    OHOS::FuzzGetPermissionIndex(data, size);
    OHOS::FuzzGetMaxPermissionIndex(data, size);
    OHOS::FuzzGetPermissionByIndex(data, size);
    return 0;
}
