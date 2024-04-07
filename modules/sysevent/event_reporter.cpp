/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hisysevent.h>

#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"

using namespace OHOS::HiviewDFX;
namespace OHOS {
namespace system {
constexpr char KEY_PROCESS_EXIT[] = "PROCESS_EXIT";
constexpr char KEY_NAME[] = "PROCESS_NAME";
constexpr char KEY_PID[] = "PID";
constexpr char KEY_UID[] = "UID";
constexpr char KEY_STATUS[] = "STATUS";
constexpr int32_t MAX_NAME_LENGTH = 1024;

void ProcessMgrRemoveApp(const char* processName, int pid, int uid, int status)
{
    std::string pname = "Unknown";
    if ((processName != NULL) && (strlen(processName) <= MAX_NAME_LENGTH)) {
        pname = std::string(processName, strlen(processName));
    }

    HiSysEventWrite(HiSysEvent::Domain::STARTUP, KEY_PROCESS_EXIT, HiSysEvent::EventType::BEHAVIOR,
        KEY_NAME, pname, KEY_PID, pid, KEY_UID, uid, KEY_STATUS, status);
}
}  // namespace system
}  // namespace OHOS

static int ProcessMgrRemoveApp(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
{
    OHOS::system::ProcessMgrRemoveApp(appInfo->name, appInfo->pid, appInfo->uid, appInfo->exitStatus);
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load sys event module ...");
    AddProcessMgrHook(STAGE_SERVER_APP_DIED, 0, ProcessMgrRemoveApp);
}
