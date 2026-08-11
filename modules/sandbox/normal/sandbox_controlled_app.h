/*
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

#ifndef SANDBOX_CONTROLLED_APP_H
#define SANDBOX_CONTROLLED_APP_H

#ifdef WITH_CONTROLLED_APP

#include <atomic>
#include <map>
#include <set>
#include <string>
#include <mutex>

typedef struct TagAppSpawningCtx AppSpawningCtx;

namespace OHOS {
namespace AppSpawn {

enum CryptoStatus {
    CRYPTO_UNAVAILABLE = 0,
    CRYPTO_READY = 1,
    CRYPTO_UPDATE = 2
};

class ControlledAppCache {
public:
    static ControlledAppCache& GetInstance();

    bool LoadFromJsonLocked();   // caller must hold mutex_
    bool IsControlled(uint32_t userId, const std::string& ownerId) const;
    int32_t EnsureCacheLoaded(int32_t cryptoStatus);
    int32_t ComputeForSpawn(const AppSpawningCtx *property);

private:
    bool cacheLoaded = false;                          // protected by mutex_
    ControlledAppCache() = default;
    std::map<std::string, std::set<std::string>> controlledApps_;
    mutable std::mutex mutex_;
};

} // namespace AppSpawn
} // namespace OHOS

#endif // WITH_CONTROLLED_APP

#endif
