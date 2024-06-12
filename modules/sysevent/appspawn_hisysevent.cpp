/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "appspawn_hisysevent.h"
#include "appspawn_utils.h"
#include <chrono>
#include "hisysevent.h"

static constexpr char PERFORMANCE_DOMAIN[] = "PERFORMANCE";
const std::string CPU_SCENE_ENTRY = "CPU_SCENE_ENTRY";
const std::string PACKAGE_NAME = "PACKAGE_NAME";
const std::string SCENE_ID = "SCENE_ID";
const std::string HAPPEN_TIME = "HAPPEN_TIME";
const std::string APPSPAWN = "APPSPAWN";
void AppSpawnHiSysEventWrite()
{
    auto now = std::chrono::system_clock::now();
    int64_t timeStamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    int32_t sceneld = 0;
    int ret = HiSysEventWrite(PERFORMANCE_DOMAIN, CPU_SCENE_ENTRY,
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        PACKAGE_NAME, APPSPAWN,
        SCENE_ID, std::to_string(sceneld).c_str(),
        HAPPEN_TIME, timeStamp);
    if (ret != 0) {
        APPSPAWN_LOGI("HiSysEventWrite error,ret = %{public}d", ret);
    }
}
