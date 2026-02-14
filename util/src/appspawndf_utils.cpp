/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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
#include "appspawndf_utils.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sys/random.h>
#include "appspawn_utils.h"
#include "config_policy_utils.h"
#include "parameters.h"
#include "json_utils.h"
#include "securec.h"

namespace OHOS {
namespace AppSpawn {
constexpr const char* APPSPAWNDF_LIST_PATH = "etc/memmgr/appspawndf_process.json";
constexpr const char* APPSPAWN_DF_APPS = "appspawndf_apps";
constexpr const char* GWP_ASAN_ENABLE = "gwp_asan.enable.app.";
constexpr int32_t GWP_ASAN_NAME_LEN = 256;
static std::atomic<uint8_t> g_processSampleRate = 128;
static std::atomic<uint8_t> g_previousRandomValue = 0;
class AppSpawnDfUtils {
public:
    static AppSpawnDfUtils &GetInstance()
    {
        static AppSpawnDfUtils instance;
        return instance;
    }

    bool IsEnabled(AppSpawnClientInitPtr initFunc)
    {
        std::lock_guard<std::mutex> lock(listMutex_);
        if (!isLoaded_) {
            LoadWhiteList();
            if (!whiteList_.empty() && initFunc != nullptr) {
                initFunc_ = initFunc;
            }
            isLoaded_ = true;
        }
        return !whiteList_.empty();
    }

    AppSpawnClientHandle GetHandle()
    {
        std::lock_guard<std::mutex> lock(handleMutex_);
        if (dfHandle_ == nullptr && initFunc_ != nullptr) {
            int ret = initFunc_(APPSPAWNDF_SERVER_NAME, &dfHandle_);
            APPSPAWN_CHECK(ret == 0, return nullptr,
                "Appspawn df client failed to init, ret=%{public}d", ret);
        }
        return dfHandle_;
    }

    bool IsInWhiteList(const char *name)
    {
        if (name == nullptr || *name == '\0') {
            return false;
        }
        std::lock_guard<std::mutex> lock(listMutex_);
        return whiteList_.count(name) > 0;
    }

    bool IsBroadcast(uint32_t msgType)
    {
        static const std::unordered_set<uint32_t> bcastMsgs = {
            MSG_LOCK_STATUS, MSG_UNINSTALL_DEBUG_HAP, MSG_OBSERVE_PROCESS_SIGNAL_STATUS,
            MSG_UNLOAD_WEBLIB_IN_APPSPAWN, MSG_LOAD_WEBLIB_IN_APPSPAWN
        };
        return bcastMsgs.find(msgType) != bcastMsgs.end();
    }

    bool AppSpawndfIsAppEnableGWPASan(const char *name, AppSpawnMsgFlags* msgFlags)
    {
        if (name == nullptr || *name == '\0' || msgFlags == NULL) {
            return false;
        }
        
        if (CheckAppSpawnMsgFlag(msgFlags, APP_FLAGS_GWP_ENABLED_FORCE)) {
            return true;
        }

        char paraName[GWP_ASAN_NAME_LEN + 1] = {0};
        int ret = sprintf_s(paraName, GWP_ASAN_NAME_LEN + 1, "%s%s", GWP_ASAN_ENABLE, name);
        if (ret >= 0 && ret <= GWP_ASAN_NAME_LEN && system::GetBoolParameter(paraName, false)) {
            return true;
        }

        uint8_t randomValue;
        // If getting a random number using a non-blocking fails, the random value is incremented.
        if (getrandom(&randomValue, sizeof(randomValue), GRND_RANDOM | GRND_NONBLOCK) == -1) {
            randomValue = g_previousRandomValue.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        if ((randomValue % g_processSampleRate.load()) == 0) {
            SetAppSpawnMsgFlags(msgFlags, APP_FLAGS_GWP_ENABLED_FORCE);
            return true;
        }
        return false;
    }

    int SetAppSpawnMsgFlags(AppSpawnMsgFlags *msgFlags, uint32_t index)
    {
        uint32_t blockIndex = index / 32;   // 32 max bit in int
        uint32_t bitIndex = index % 32;     // 32 max bit in int
        if (blockIndex < msgFlags->count) {
            msgFlags->flags[blockIndex] |= (1 << bitIndex);
        }
        return 0;
    }
private:
    void LoadWhiteList()
    {
        char buf[PATH_MAX] = {0};
        /* 修正：使用 APPSPAWNDF_LIST_PATH */
        char *path = GetOneCfgFile(APPSPAWNDF_LIST_PATH, buf, PATH_MAX);
        if (path == nullptr) {
            return;
        }
        cJSON *root = GetJsonObjFromFile(path);
        if (root == nullptr) {
            return;
        }
        /* 修正：使用 APPSPAWN_DF_APPS */
        cJSON *arr = cJSON_GetObjectItemCaseSensitive(root, APPSPAWN_DF_APPS);
        if (cJSON_IsArray(arr)) {
            cJSON *item = nullptr;
            cJSON_ArrayForEach(item, arr) {
                const char *val = cJSON_GetStringValue(item);
                if (val != nullptr) {
                    whiteList_.insert(val);
                }
            }
        }
        cJSON_Delete(root);
    }

    inline int CheckAppSpawnMsgFlag(const AppSpawnMsgFlags *msgFlags, uint32_t index)
    {
        if (!msgFlags) {
            return 0;
        }
        uint32_t blockIndex = index / 32;   // 32 max bit in int
        uint32_t bitIndex = index % 32;     // 32 max bit in int
        if (blockIndex < msgFlags->count) {
            return CHECK_FLAGS_BY_INDEX(msgFlags->flags[blockIndex], bitIndex);
        }
        return 0;
    }

    bool isLoaded_ = false;
    std::unordered_set<std::string> whiteList_;
    AppSpawnClientInitPtr initFunc_ = nullptr;
    std::mutex listMutex_;
    AppSpawnClientHandle dfHandle_ = nullptr;
    std::mutex handleMutex_;
};

} // namespace AppSpawn
} // namespace OHOS

extern "C" {
bool AppSpawndfIsServiceEnabled(AppSpawnClientInitPtr initFunc)
{
    return OHOS::AppSpawn::AppSpawnDfUtils::GetInstance().IsEnabled(initFunc);
}

AppSpawnClientHandle AppSpawndfGetHandle(void)
{
    return OHOS::AppSpawn::AppSpawnDfUtils::GetInstance().GetHandle();
}

bool AppSpawndfIsAppInWhiteList(const char *name)
{
    return OHOS::AppSpawn::AppSpawnDfUtils::GetInstance().IsInWhiteList(name);
}

bool AppSpawndfIsAppEnableGWPASan(const char *name, AppSpawnMsgFlags* msgFlags)
{
    return OHOS::AppSpawn::AppSpawnDfUtils::GetInstance().AppSpawndfIsAppEnableGWPASan(name, msgFlags);
}

bool AppSpawndfIsBroadcastMsg(uint32_t type)
{
    return OHOS::AppSpawn::AppSpawnDfUtils::GetInstance().IsBroadcast(type);
}

void AppSpawndfMergeBroadcastResult(uint32_t msgType, AppSpawnResult *mainRes,
    AppSpawnResult *dfRes)
{
    if (mainRes == nullptr || dfRes == nullptr) {
        return;
    }
    if (msgType == MSG_UNINSTALL_DEBUG_HAP || msgType == MSG_LOCK_STATUS) {
        if (mainRes->result != 0 && dfRes->result == 0) {
            mainRes->result = 0;
            mainRes->pid = dfRes->pid;
        }
    } else {
        if (mainRes->result != 0 || dfRes->result != 0) {
            if (mainRes->result == 0) {
                mainRes->result = (dfRes->result != 0) ? dfRes->result : -1;
            }
        }
    }
}

int SetAppSpawnMsgFlags(AppSpawnMsgFlags *msgFlags, uint32_t index)
{
    return OHOS::AppSpawn::AppSpawnDfUtils::GetInstance().SetAppSpawnMsgFlags(msgFlags, index);
}
}