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

#ifndef SPAWNING_TEST_HELPER_H
#define SPAWNING_TEST_HELPER_H

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
class SpawningTestHelper {
public:
    SpawningTestHelper()
    {
        SpawningTestSetDefaultData();
    }
    ~SpawningTestHelper() {}

    AppSpawnReqMsgHandle SpawningTestCreateSendMsg(AppSpawnClientHandle handle,
        uint32_t msgType = MSG_APP_SPAWN, uint32_t flag = 0);
    AppSpawningCtx *SpawningTestGetAppProperty(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle);
    void SetTestUid(uid_t uid)
    {
        spawningTestDefaultTestUid_ = uid;
    }

    const char *GetDefaultTestAppBundleName()
    {
        return spawningTestProcessName_.c_str();
    }

private:
    uint32_t spawningTestDefaultMsgFlags_ = 0;
    uid_t spawningTestDefaultTestUid_ = 0;
    gid_t spawningTestDefaultTestGid_ = 0;
    gid_t spawningTestDefaultTestGidGroup_ = 0;
    int32_t spawningTestDefaultTestBundleIndex_ = 0;
    std::string spawningTestProcessName_;
    std::string spawningTestDefaultApl_ = "system_core";
    std::vector<const char *> spawningTestPermissions_ = {
        const_cast<char *>("ohos.permission.MANAGE_PRIVATE_PHOTOS"),
        const_cast<char *>("ohos.permission.ACTIVATE_THEME_PACKAGE"),
        const_cast<char *>("ohos.permission.GET_WALLPAPER"),
        const_cast<char *>("ohos.permission.FILE_ACCESS_MANAGER")
    };

    void SpawningTestSetDefaultData(void);
    int SpawningTestAddDacInfo(AppSpawnReqMsgHandle &reqHandle);
    int SpawningTestAddFdInfo(AppSpawnReqMsgHandle &reqHandle);
    int SpawningTestSetMsgFlags(AppSpawnReqMsgHandle reqHandle, uint32_t tlv, uint32_t flags);
    AppSpawnMsgNode *SpawningTestCreateAppSpawnMsg(AppSpawnMsg *msg);
};
}  // namespace OHOS
#endif  // SPAWNING_TEST_HELPER_H
