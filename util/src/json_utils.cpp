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

#include "json_utils.h"
#include <fstream>
#include <sstream>
#include "hilog/log.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0, "AppSpawn_JsonUtil"};

namespace OHOS {
namespace AppSpawn {
bool JsonUtils::GetJsonObjFromJson(nlohmann::json &jsonObj, const std::string &jsonPath)
{
    if (jsonPath.length() > PATH_MAX) {
        HiLog::Error(LABEL, "jsonPath is too long");
        return false;
    }

    std::ifstream jsonFileStream;
    jsonFileStream.open(jsonPath.c_str(), std::ios::in);
    if (!jsonFileStream.is_open()) {
        HiLog::Error(LABEL, "Open json file failed.");
        return false;
    }

    std::ostringstream buf;
    char ch;
    while (buf && jsonFileStream.get(ch)) {
        buf.put(ch);
    }
    jsonFileStream.close();

    jsonObj = nlohmann::json::parse(buf.str(), nullptr, false);
    if (!jsonObj.is_structured()) {
        HiLog::Error(LABEL, "Parse json file into jsonObj failed.");
        return false;
    }
    return true;
}

bool JsonUtils::GetStringFromJson(const nlohmann::json &json, const std::string &key, std::string &value)
{
    if (!json.is_object()) {
        HiLog::Error(LABEL, "json is not object.");
        return false;
    }
    if (json.find(key) != json.end() && json.at(key).is_string()) {
        HiLog::Error(LABEL, "Find key[%{public}s] successful.", key.c_str());
        value = json.at(key).get<std::string>();
        return true;
    }

    return false;
}

bool JsonUtils::GetIntFromJson(const nlohmann::json &json, const std::string &key, int &value)
{
    if (!json.is_object()) {
        HiLog::Error(LABEL, "json is not object.");
        return false;
    }
    if (json.find(key) != json.end() && json.at(key).is_number()) {
        HiLog::Error(LABEL, "Find key[%{public}s] successful.", key.c_str());
        value = json.at(key).get<int>();
        return true;
    }

    return false;
}

bool JsonUtils::GetStringVecFromJson(const nlohmann::json &json, const std::string &key,
                                     std::vector<std::string> &value)
{
    if (!json.is_object()) {
        HiLog::Error(LABEL, "json is not object.");
        return false;
    }
    if (json.find(key) != json.end() && json.at(key).is_array()) {
        HiLog::Error(LABEL, "Find key[%{public}s] successful.", key.c_str());
        value = json.at(key).get<std::vector<std::string>>();
        return true;
    }

    return false;
}

bool JsonUtils::ParseObjVecFromJson(const nlohmann::json &json, const std::string &key,
                                    std::vector<nlohmann::json> &value)
{
    if (!json.is_object()) {
        HiLog::Error(LABEL, "json is not object.");
        return false;
    }
    if (json.find(key) != json.end() && json.at(key).is_array()) {
        HiLog::Error(LABEL, "Find key[%{public}s] successful.", key.c_str());
        value = json.at(key).get<std::vector<nlohmann::json>>();
        return true;
    }

    return false;
}
} // namespace AppSpawn
} // namespace OHOS