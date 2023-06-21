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

#include "appspawn_mount_permission.h"
#include "sandbox_utils.h"
#include "appspawn_adapter.h"
namespace OHOS {
namespace AppSpawn {

bool AppspawnMountPermission::g_IsLoad = false;

std::set<std::string> AppspawnMountPermission::GetMountPermissionList()
{
    if (!AppspawnMountPermission::g_IsLoad) {
        LoadAppSandboxConfig();
        AppspawnMountPermission::g_IsLoad = true;
    }
    return SandboxUtils::GetMountPermissionNames();
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
