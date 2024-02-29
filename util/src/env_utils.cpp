/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "env_utils.h"
#include "appspawn_service.h"
#include "sandbox_utils.h"
#include "nlohmann/json.hpp"

const std::string APPENVLIST_TYPE = "|AppEnv|";

int32_t SetEnvInfo(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    int ret = 0;
    OHOS::AppSpawn::ClientSocket::AppProperty *appProperty = &((AppSpawnClientExt *)client)->property;
    std::string appEnvInfo = OHOS::AppSpawn::SandboxUtils::GetExtraInfoByType(appProperty, APPENVLIST_TYPE);
    if (appEnvInfo.length() == 0) {
        return ret;
    }

    nlohmann::json envs = nlohmann::json::parse(appEnvInfo.c_str(), nullptr, false);
    APPSPAWN_CHECK(!envs.is_discarded(), return -1, "SetEnvInfo: json parse failed");

    for (nlohmann::json::iterator it = envs.begin(); it != envs.end(); ++it) {
        APPSPAWN_CHECK(it.value().is_string(), return -1, "SetEnvInfo: element type error");
        std::string name = it.key();
        std::string value = it.value();
        ret = setenv(name.c_str(), value.c_str(), 1);
        APPSPAWN_CHECK(ret == 0, return ret, "setenv failed, errno is %{public}d", errno);
    }
    return ret;
}