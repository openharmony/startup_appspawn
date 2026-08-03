/**
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "sandbox_controlled_app.h"

#include <cstdlib>
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "parameters.h"
#include "securec.h"
#include "json_utils.h"
#include "cJSON.h"

namespace OHOS {
namespace AppSpawn {

static const char* CONTROLLED_APPS_JSON =
    "/data/service/el1/public/dlp_credential_service/ControlledAppList.json";

ControlledAppCache& ControlledAppCache::GetInstance()
{
    static ControlledAppCache instance;
    return instance;
}

// caller must hold mutex_
bool ControlledAppCache::LoadFromJsonLocked()
{
    cJSON *root = GetJsonObjFromFile(CONTROLLED_APPS_JSON);
    if (root == nullptr) {
        APPSPAWN_LOGW("controlled: ControlledAppList.json not found, no apps controlled");
        return false;
    }
    if (!cJSON_IsObject(root)) {
        APPSPAWN_LOGW("controlled: ControlledAppList.json root is not object");
        cJSON_Delete(root);
        return false;
    }

    std::map<std::string, std::set<std::string>> tempCache;
    for (cJSON *item = root->child; item != nullptr; item = item->next) {
        if (!cJSON_IsArray(item)) {
            APPSPAWN_LOGW("controlled: non-array value for key %{public}s, skip",
                          item->string ? item->string : "(null)");
            continue;
        }
        if (item->string == nullptr) {
            APPSPAWN_LOGW("controlled: skip entry with null key");
            continue;
        }
        std::string userId(item->string);
        if (userId.empty() || userId.find_first_not_of("0123456789") != std::string::npos) {
            APPSPAWN_LOGW("controlled: invalid userId %{public}s, skip", userId.c_str());
            continue;
        }
        int32_t count = cJSON_GetArraySize(item);
        for (int32_t i = 0; i < count; i++) {
            cJSON *idItem = cJSON_GetArrayItem(item, i);
            if (idItem == nullptr || !cJSON_IsString(idItem) || idItem->valuestring == nullptr) {
                continue;
            }
            tempCache[userId].insert(std::string(idItem->valuestring));
        }
    }
    cJSON_Delete(root);

    controlledApps_.swap(tempCache);
    return true;
}

bool ControlledAppCache::LoadFromJson()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return LoadFromJsonLocked();
}

bool ControlledAppCache::IsControlled(uint32_t userId, const std::string& ownerId) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::string userIdStr = std::to_string(userId);
    auto it = controlledApps_.find(userIdStr);
    if (it == controlledApps_.end()) {
        return false;
    }
    return it->second.count(ownerId) > 0;
}


// Clear client-settable flag 44 before server-side determination.
static void ClearControlledFlag(const AppSpawningCtx *property)
{
    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppSpawnMsgInfo(property->message, TLV_MSG_FLAGS);
    if (msgFlags == nullptr) {
        return;
    }
    uint32_t blockIndex = APP_FLAGS_CONTROLLED_APP / 32;
    uint32_t bitIndex = APP_FLAGS_CONTROLLED_APP % 32;
    if (blockIndex < msgFlags->count) {
        msgFlags->flags[blockIndex] &= ~(1U << bitIndex);
    }
}


int32_t ControlledAppCache::EnsureCacheLoaded(int32_t cryptoStatus)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (cryptoStatus == CRYPTO_UPDATE) {
        int32_t setRet = system::SetParameter("security.dlp.transparent.crypto.status", "1");
        if (setRet != 0) {
            APPSPAWN_LOGW("controlled_app: SetParam status=1 failed: %{public}d", setRet);
        }
        if (!LoadFromJsonLocked()) {
            cacheLoaded = false;
            APPSPAWN_LOGE("controlled_app: DLP notified update (status=2) but LoadFromJson failed, degraded mount");
            return -1;
        }
        cacheLoaded = true;
    } else if (!cacheLoaded) {
        APPSPAWN_LOGV("ctrl: cache lost, reload from disk");
        if (!LoadFromJsonLocked()) {
            APPSPAWN_LOGE("controlled_app: appspawn restarted (cache lost, status=1) but failed to reload"
                          " from disk, degraded mount");
            return -1;
        }
        cacheLoaded = true;
    }
    return 0;
}

int32_t ControlledAppCache::ComputeForSpawn(const AppSpawningCtx *property)
{
    ClearControlledFlag(property);

    std::string cryptoStr = system::GetParameter("security.dlp.transparent.crypto.status", "0");
    char *endPtr = nullptr;
    long val = strtol(cryptoStr.c_str(), &endPtr, 10);
    int32_t cryptoStatus = (endPtr != cryptoStr.c_str() && *endPtr == '\0' && val >= 0)
        ? static_cast<int32_t>(val) : CRYPTO_UNAVAILABLE;
    APPSPAWN_LOGV("ctrl: crypto status=%{public}d", cryptoStatus);

    if (cryptoStatus == CRYPTO_UNAVAILABLE) {
        APPSPAWN_LOGV("ctrl: crypto unavailable, skip controlled flow");
        return 0;
    }

    int32_t cacheRet = EnsureCacheLoaded(cryptoStatus);
    if (cacheRet != 0) {
        system::SetParameter("startup.appspawn.dlp_errorcode", "-1");
        APPSPAWN_LOGW("ctrl: cache load failed, set dlp_errorcode=-1, degraded mount");
        return 0;
    }

    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(
        GetAppProperty(property, TLV_DAC_INFO));
    AppSpawnMsgOwnerId *ownerInfo = reinterpret_cast<AppSpawnMsgOwnerId *>(
        GetAppProperty(property, TLV_OWNER_INFO));
    if (dacInfo == nullptr || ownerInfo == nullptr) {
        APPSPAWN_LOGE("ctrl: security context missing (crypto=%{public}d), aborting", cryptoStatus);
        return -1;
    }

    std::string ownerId(ownerInfo->ownerId);
    uint32_t userId = dacInfo->uid / UID_BASE;
    bool matched = IsControlled(userId, ownerId);
    if (matched) {
        int32_t rc = SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_CONTROLLED_APP);
        APPSPAWN_LOGV("ctrl: flag %{public}d set ret=%{public}d", APP_FLAGS_CONTROLLED_APP, rc);
        if (rc != 0) {
            APPSPAWN_LOGE("controlled_app: matched controlled app but SetAppSpawnMsgFlag(44) returned"
                          " %{public}d, aborting spawn", APP_FLAGS_CONTROLLED_APP);
            return -1;
        }
    } else {
        APPSPAWN_LOGV("ctrl: flag %{public}d NOT SET, matched=%{public}d",
                      APP_FLAGS_CONTROLLED_APP, static_cast<int32_t>(matched));
    }
    return matched ? 1 : 0;
}

} // namespace AppSpawn
} // namespace OHOS
