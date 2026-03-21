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
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <string>
#include <sys/random.h>
#include <sys/stat.h>
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
const std::string LOCAL_PATH = "/system/etc/SwitchOffList/";
const std::string CLOUD_PATH = "/data/service/el1/public/update/param_service/install/system/etc/SwitchOffList/";
const std::string APPSPAWN_DF_SWITCH_OFF_FLAG = "appspawn_df_switch_off_flag_v_0";
constexpr int32_t VERSION_LEN = 4; // the number of digits extracted from the fixed format string "x.x.x.x"
constexpr int32_t GWP_ASAN_NAME_LEN = 256;
static std::atomic<uint8_t> g_processSampleRate = 128;
static std::atomic<uint8_t> g_previousRandomValue = 0;
constexpr int32_t MAX_VERSION_FILE_LEN = 1024 * 5;
constexpr int32_t MAX_SWITCH_OFF_FILE_LEN = 1024 * 5;
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
    static inline void Trim(std::string& inputStr)
    {
        if (inputStr.empty()) {
            return;
        }
        inputStr.erase(inputStr.begin(), std::find_if(inputStr.begin(), inputStr.end(),
            [](unsigned char ch) { return !std::isspace(static_cast<unsigned char>(ch)); }));
        inputStr.erase(std::find_if(inputStr.rbegin(), inputStr.rend(),
            [](unsigned char ch) { return !std::isspace(static_cast<unsigned char>(ch)); }).base(), inputStr.end());
    }

    static inline bool IsNumber(const std::string &str)
    {
        return !str.empty() && std::all_of(str.begin(), str.end(), [](unsigned char c) {
            return std::isdigit(c);
        });
    }

    std::vector<std::string> SplitString(const std::string& str, char pattern)
    {
        std::stringstream iss(str);
        std::vector<std::string> result;
        std::string token;
        while (getline(iss, token, pattern)) {
            result.emplace_back(token);
        }
        return result;
    }

    std::vector<std::string> GetVersionNums(const std::string& filePath)
    {
        struct stat fileStat;
        if (stat(filePath.c_str(), &fileStat) != 0 ||
            fileStat.st_size <= 0 || fileStat.st_size > MAX_VERSION_FILE_LEN) {
            APPSPAWN_LOGE("AppSpawnDfUtils::GetVersionNums: stat failed");
            return {};
        }

        std::ifstream file(filePath);
        if (!file.is_open()) {
            return {};
        }
        std::string line;
        std::getline(file, line);
        file.close();
        if (line.empty()) {
            APPSPAWN_LOGE("AppSpawnDfUtils::GetVersionNums line is empty!");
            return {};
        }
        std::vector<std::string> versionStr = SplitString(line, '=');
        const size_t expectedSize = 2; // 2: expect to divide the string into two parts base on "="
        if (versionStr.size() != expectedSize) {
            APPSPAWN_LOGE("AppSpawnDfUtils::GetVersionNums: version num is not valid, expected format 'key=value'.");
            return {};
        }
        if (versionStr[1].empty()) { // index 1 indicates string representing the version number.
            APPSPAWN_LOGE("AppSpawnDfUtils::GetVersionNums: version value is empty.");
            return {};
        }
        Trim(versionStr[1]);
        std::vector<std::string> versionNum = SplitString(versionStr[1], '.');
        return versionNum;
    }

    bool CompareVersion(const std::vector<std::string> &localVersion, const std::vector<std::string> &cloudVersion)
    {
        if (localVersion.size() != VERSION_LEN || cloudVersion.size() != VERSION_LEN) {
            APPSPAWN_LOGI("AppSpawnDfUtils::CompareVersion: version num is not valid, use localVersion.");
            return false;
        }
        for (int32_t i = 0; i < VERSION_LEN; i++) {
            if (localVersion[i] != cloudVersion[i]) {
                if (!IsNumber(localVersion[i]) || !IsNumber(cloudVersion[i])) {
                    APPSPAWN_LOGE("AppSpawnDfUtils::CompareVersion: version part contains non-numeric characters.");
                    return false;
                }
                int32_t localValue = std::atoi(localVersion[i].c_str());
                int32_t cloudValue = std::atoi(cloudVersion[i].c_str());
                return localValue < cloudValue;
            }
        }
        return false;
    }

    // Get path which contains a higher version configration file
    std::string GetHigherVersionPath()
    {
        std::string localVersionFile = LOCAL_PATH + "version.txt"; // local version path
        std::string cloudVersionFile = CLOUD_PATH + "version.txt"; // cloud version path
        std::vector<std::string> localVersionNums = GetVersionNums(localVersionFile);
        std::vector<std::string> cloudVersionNums = GetVersionNums(cloudVersionFile);
        if (CompareVersion(localVersionNums, cloudVersionNums)) {
            return CLOUD_PATH;
        }
        return LOCAL_PATH;
    }

    bool IsDisableDfmalloc()
    {
        std::string filePath = GetHigherVersionPath() + "switch_off_list";
        struct stat fileStat;
        if (stat(filePath.c_str(), &fileStat) != 0 ||
            fileStat.st_size <= 0 || fileStat.st_size > MAX_SWITCH_OFF_FILE_LEN) {
            APPSPAWN_LOGE("AppSpawnDfUtils::IsDisableDfmalloc: stat failed");
            return false;
        }
        std::ifstream inputFile(filePath);
        if (!inputFile.is_open()) {
            APPSPAWN_LOGI("AppSpawnDfUtils::IsDisableDfmalloc: Can not open the file.");
            return false;
        }
        std::string line;
        while (std::getline(inputFile, line)) {
            Trim(line);
            if (line == APPSPAWN_DF_SWITCH_OFF_FLAG) {
                APPSPAWN_LOGI("AppSpawnDfUtils::IsDisableDfmalloc: close dfmalloc");
                inputFile.close();
                return true;
            }
        }
        inputFile.close();
        return false;
    }

    void LoadWhiteList()
    {
        if (IsDisableDfmalloc()) {
            return;
        }

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