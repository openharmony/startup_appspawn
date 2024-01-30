/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#include <fstream>
#include <sstream>
#include "appspawn_mount_permission.h"
#include "appspawn_server.h"
#include "config_policy_utils.h"

namespace OHOS {
namespace AppSpawn {
namespace {
const std::string APP_PERMISSION_PATH("/appdata-sandbox.json");
const std::string PERMISSION_FIELD("permission");
}
std::set<std::string> AppspawnMountPermission::appSandboxPremission_ = {};
bool AppspawnMountPermission::isLoad_ = false;
std::mutex AppspawnMountPermission::mutex_;

void AppspawnMountPermission::GetPermissionFromJson(
    std::set<std::string> &appSandboxPremissionSet, nlohmann::json &appSandboxPremission)
{
    auto item = appSandboxPremission.find(PERMISSION_FIELD);
    if (item != appSandboxPremission.end()) {
        for (auto config : appSandboxPremission[PERMISSION_FIELD]) {
            for (auto it : config.items()) {
            APPSPAWN_LOGI("LoadPermissionNames %{public}s", it.key().c_str());
            appSandboxPremissionSet.insert(it.key());
            }
        }
    } else {
        APPSPAWN_LOGI("permission does not exist");
    }
}

void AppspawnMountPermission::LoadPermissionNames(void)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (isLoad_) {
        return;
    }
    appSandboxPremission_.clear();
    nlohmann::json appSandboxPremission;
    CfgFiles *files = GetCfgFiles("etc/sandbox");
    for (int i = 0; (files != nullptr) && (i < MAX_CFG_POLICY_DIRS_CNT); ++i) {
        if (files->paths[i] == nullptr) {
            continue;
        }
        std::string path = files->paths[i];
        path += APP_PERMISSION_PATH;
        APPSPAWN_LOGI("LoadAppSandboxConfig %{public}s", path.c_str());
        std::ifstream jsonFileStream;
        jsonFileStream.open(path.c_str(), std::ios::in);
        APPSPAWN_CHECK_ONLY_EXPER(jsonFileStream.is_open(), return);
        std::stringstream buffer;
        buffer << jsonFileStream.rdbuf();
        appSandboxPremission = nlohmann::json::parse(buffer.str(), nullptr, false);
        APPSPAWN_CHECK(appSandboxPremission.is_structured(), return, "Parse json file into jsonObj failed.");
        GetPermissionFromJson(appSandboxPremission_, appSandboxPremission);
    }
    FreeCfgFiles(files);
    APPSPAWN_LOGI("LoadPermissionNames size: %{public}lu", static_cast<unsigned long>(appSandboxPremission_.size()));
    isLoad_ = true;
}

std::set<std::string> AppspawnMountPermission::GetMountPermissionList()
{
    if (!isLoad_) {
        LoadPermissionNames();
        APPSPAWN_LOGV("GetMountPermissionList LoadPermissionNames");
    }
    return appSandboxPremission_;
}

uint32_t AppspawnMountPermission::GenPermissionCode(const std::set<std::string> &permissions)
{
    uint32_t result = 0;
    if (permissions.size() == 0) {
        return result;
    }
    uint32_t flagIndex = 1;
    for (std::string mountPermission : GetMountPermissionList()) {
        for (std::string inputPermission : permissions) {
            if (mountPermission.compare(inputPermission) == 0) {
                result |= flagIndex;
            }
        }
        flagIndex <<= 1;
    }
    return result;
}

bool AppspawnMountPermission::IsMountPermission(uint32_t code, const std::string permission)
{
    for (std::string mountPermission : GetMountPermissionList()) {
        if (mountPermission.compare(permission) == 0) {
            return code & 1;
        }
        code >>= 1;
    }
    return false;
} // AppSpawn
} // OHOS
}
