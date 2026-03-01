/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef DFX_PRELOAD_TEST_HELPER_H
#define DFX_PRELOAD_TEST_HELPER_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"

namespace OHOS {
class DfxPreloadTestHelper {
public:
    DfxPreloadTestHelper()
    {
        DfxPreloadTestSetDefaultData();
    }
    ~DfxPreloadTestHelper() {}

    AppSpawnReqMsgHandle DfxPreloadTestCreateMsg(AppSpawnClientHandle handle, uint32_t msgType = MSG_APP_SPAWN);
    AppSpawningCtx *DfxPreloadTestGetAppProperty(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle);

private:
    uint32_t defaultMsgFlags_ = 0;
    uid_t defaultTestUid_ = 0;
    gid_t defaultTestGid_ = 0;
    gid_t defaultTestGidGroup_ = 0;
    int32_t defaultTestBundleIndex_ = 0;
    std::string processName_;
    std::string defaultApl_ = "system_core";
    std::vector<const char *> permissions_ = {
        "ohos.permission.MANAGE_PRIVATE_PHOTOS",
        "ohos.permission.ACTIVATE_THEME_PACKAGE",
        "ohos.permission.GET_WALLPAPER",
        "ohos.permission.FILE_ACCESS_MANAGER"
    };

    void DfxPreloadTestSetDefaultData(void);
    int DfxPreloadTestAddDacInfo(AppSpawnReqMsgHandle &reqHandle);
    int DfxPreloadTestAddFdInfo(AppSpawnReqMsgHandle &reqHandle);
    int DfxPreloadTestSetMsgFlags(AppSpawnReqMsgHandle reqHandle, uint32_t tlv, uint32_t flags);
    AppSpawnMsgNode *DfxPreloadTestCreateAppSpawnMsg(AppSpawnMsg *msg);
};
}  // namespace OHOS
#endif  // DFX_PRELOAD_TEST_HELPER_H
