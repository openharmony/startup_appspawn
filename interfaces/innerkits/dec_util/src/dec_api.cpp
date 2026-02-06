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

#include "appspawn_utils.h"
#include "cJSON.h"
#include "config_policy_utils.h"
#include "dec_api.h"
#include "hilog/log.h"
#include "json_utils.h"

static std::vector<cJSON *> InitDecConfig(void)
{
    std::vector<cJSON *> sandboxConfig = {};
    CfgFiles *files = GetCfgFiles("etc/sandbox");
    for (int i = 0; (files != nullptr) && (i < MAX_CFG_POLICY_DIRS_CNT); ++i) {
        if (files->paths[i] == nullptr) {
            continue;
        }
        std::string path = files->paths[i];
        std::string appPath = path + "/appdata-sandbox.json";
        APPSPAWN_LOGV("Init sandbox dec config %{public}s", appPath.c_str());
        cJSON *config = GetJsonObjFromFile(appPath.c_str());
        APPSPAWN_CHECK((config != nullptr && cJSON_IsObject(config)), continue,
            "Failed to init sandbox dec config %{public}s", appPath.c_str());
        sandboxConfig.push_back(config);
    }
    FreeCfgFiles(files);
    return sandboxConfig;
}

static void DestroyDecConfig(std::vector<cJSON *> &sandboxConfig)
{
    for (auto& config : sandboxConfig) {
        APPSPAWN_CHECK_ONLY_EXPER(config != nullptr, continue);
        cJSON_Delete(config);
        config = nullptr;
    }
    sandboxConfig.clear();
}

static std::string ConvertDecPath(std::string path)
{
    std::vector<std::pair<std::string, std::string>> replacements = {
        {"<currentUserId>", "currentUser"},
        {"<PackageName>", "com.ohos.dlpmanager"}
    };

    for (const auto& [from, to] : replacements) {
        size_t pos = 0;
        while ((pos = path.find(from, pos)) != std::string::npos) {
            path.replace(pos, from.length(), to);
            pos += to.length();
        }
    }
    return path;
}

static void AddDecPathsByPermission(std::map<std::string, std::vector<std::string>> &decMap,
                                    const std::string permission, const cJSON *permItem)
{
    cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(permItem, "mount-paths");
    APPSPAWN_CHECK(mountPoints != nullptr && cJSON_IsArray(mountPoints), return,
        "Don't get mountPoints json from permission");

    std::vector<std::string> decPaths = {};
    int arraySize = cJSON_GetArraySize(mountPoints);
    for (int i = 0; i < arraySize; ++i) {
        cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
        APPSPAWN_CHECK(mntPoint != nullptr && cJSON_IsObject(mntPoint), continue,
            "Don't get mntPoint json from mountPoints");

        cJSON *decPathJson = cJSON_GetObjectItemCaseSensitive(mntPoint, "dec-paths");
        if (decPathJson == nullptr || !cJSON_IsArray(decPathJson)) {
            APPSPAWN_LOGV("Don't get decPath json from mntPoint");
            continue;
        }

        int count = cJSON_GetArraySize(decPathJson);
        for (int j = 0; j < count; ++j) {
            cJSON *item = cJSON_GetArrayItem(decPathJson, j);
            APPSPAWN_CHECK(item != nullptr && cJSON_IsString(item), continue, "Don't get decPath item");
            const char *strValue = cJSON_GetStringValue(item);
            APPSPAWN_CHECK_ONLY_EXPER(strValue != nullptr, continue);

            std::string decPath = ConvertDecPath(strValue);
            APPSPAWN_LOGV("Get decPath %{public}s from %{public}s", decPath.c_str(), permission.c_str());
            decPaths.push_back(decPath);
        }
    }

    if (!decPaths.empty()) {
        auto it = decMap.find(permission);
        if (it == decMap.end()) {
            decMap[permission] = decPaths;
        } else {
            for (const auto& path : decPaths) {
                it->second.push_back(path);
            }
        }
    }
}

static void ProcessConfig(const cJSON *config, std::map<std::string, std::vector<std::string>> &decMap)
{
    cJSON *permission = cJSON_GetObjectItemCaseSensitive(config, "permission");
    APPSPAWN_CHECK(permission != nullptr && cJSON_IsArray(permission), return,
        "Don't get permission json from config");

    int arraySize = cJSON_GetArraySize(permission);
    for (int i = 0; i < arraySize; ++i) {
        cJSON *item = cJSON_GetArrayItem(permission, i);
        APPSPAWN_CHECK(item != nullptr && cJSON_IsObject(item), continue, "Invalid item in permission array");
        cJSON *permissionChild = item->child;
        while (permissionChild != nullptr && cJSON_IsArray(permissionChild)) {
            cJSON *permItem = cJSON_GetArrayItem(permissionChild, 0);
            APPSPAWN_CHECK(permItem != nullptr && cJSON_IsObject(permItem), permissionChild = permissionChild->next;
                continue, "Don't get permission item");
            APPSPAWN_LOGV("AddDecPathsByPermission %{public}s", permissionChild->string);
            AddDecPathsByPermission(decMap, permissionChild->string, permItem);
            permissionChild = permissionChild->next;
        }
    }
}

std::map<std::string, std::vector<std::string>> GetDecPathMap(void)
{
    std::vector<cJSON *> sandboxConfig = InitDecConfig();
    APPSPAWN_CHECK(!sandboxConfig.empty(), return {}, "Sandbox dec config is empty");

    std::map<std::string, std::vector<std::string>> decMap = {};
    for (auto& config : sandboxConfig) {
        ProcessConfig(config, decMap);
    }

    DestroyDecConfig(sandboxConfig);
    return decMap;
}
