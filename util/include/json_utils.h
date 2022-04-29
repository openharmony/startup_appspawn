/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <string>
#include <vector>
#include "nlohmann/json.hpp"

namespace OHOS {
namespace AppSpawn {
class JsonUtils {
public:
    static bool GetJsonObjFromJson(nlohmann::json& jsonObj, const std::string& jsonPath);
    static bool GetStringFromJson(const nlohmann::json& json, const std::string& key, std::string& value);
    static bool GetIntFromJson(const nlohmann::json& json, const std::string& key, int& value);
    static bool GetStringVecFromJson(
        const nlohmann::json& json, const std::string& key, std::vector<std::string>& value);
    static bool ParseObjVecFromJson(
        const nlohmann::json& json, const std::string& key, std::vector<nlohmann::json>& value);
};
} // namespace AppSpawn
} // namespace OHOS
#endif  // JSON_UTILS_H