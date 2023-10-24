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

#ifndef APPSPAWN_MOUNT_PERMISSION_H
#define APPSPAWN_MOUNT_PERMISSION_H

#include <string>
#include <set>
#include <mutex>
#include "nlohmann/json.hpp"

namespace OHOS {
namespace AppSpawn {
class AppspawnMountPermission {
public:
    static void GetPermissionFromJson(
        std::set<std::string> &appSandboxPremissionSet, nlohmann::json &appSandboxPremission);
    static void LoadPermissionNames(void);
    static std::set<std::string> GetMountPermissionList();
    static uint32_t GenPermissionCode(const std::set<std::string> &permissions);
    static bool IsMountPermission(uint32_t code, const std::string permission);
private:
    static bool isLoad_;
    static std::set<std::string> appSandboxPremission_;
    static std::mutex mutex_;
};
} // AppSpawn
} // OHOS

#endif

