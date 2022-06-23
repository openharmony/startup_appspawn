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
#include "appspawn_server.h"

#include <sstream>
#include <fstream>

using namespace std;
using namespace OHOS;

namespace OHOS {
namespace AppSpawn {
bool JsonUtils::GetJsonObjFromJson(nlohmann::json &jsonObj, const std::string &jsonPath)
{
    APPSPAWN_CHECK(jsonPath.length() <= PATH_MAX, return false, "jsonPath is too long");
    std::ifstream jsonFileStream;
    jsonFileStream.open(jsonPath.c_str(), std::ios::in);
    APPSPAWN_CHECK(jsonFileStream.is_open(), return false, "Open json file failed.");
    std::ostringstream buf;
    char ch;
    while (buf && jsonFileStream.get(ch)) {
        buf.put(ch);
    }
    jsonFileStream.close();
    jsonObj = nlohmann::json::parse(buf.str(), nullptr, false);
    APPSPAWN_CHECK(jsonObj.is_structured(), return false, "Parse json file into jsonObj failed.");
    return true;
}

bool JsonUtils::GetStringFromJson(const nlohmann::json &json, const std::string &key, std::string &value)
{
    APPSPAWN_CHECK(json.is_object(), return false, "json is not object.");
    bool isRet = json.find(key) != json.end() && json.at(key).is_string();
    APPSPAWN_CHECK(!isRet, value = json.at(key).get<std::string>();
        return true, "Find key[%s] successful.", key.c_str());
    return false;
}
} // namespace AppSpawn
} // namespace OHOS