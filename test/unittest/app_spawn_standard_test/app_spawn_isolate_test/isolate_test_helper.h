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

#ifndef ISOLATE_TEST_HELPER_H
#define ISOLATE_TEST_HELPER_H

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
class IsolateTestHelper {
public:
    IsolateTestHelper()
    {
        IsolateTestSetDefaultData();
    }
    ~IsolateTestHelper() {}

    AppSpawnReqMsgHandle IsolateTestCreateSendMsg(AppSpawnClientHandle handle, uint32_t msgType = MSG_APP_SPAWN);
    AppSpawningCtx *IsolateTestGetAppProperty(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle);

private:
    uint32_t isolateTestDefaultMsgFlags_ = 0;
    uid_t isolateTestDefaultTestUid_ = 0;
    gid_t isolateTestDefaultTestGid_ = 0;
    gid_t isolateTestDefaultTestGidGroup_ = 0;
    int32_t isolateTestDefaultTestBundleIndex_ = 0;
    std::string isolateTestProcessName_;
    std::string isolateTestDefaultApl_ = "system_core";
    std::vector<const char *> isolateTestPermissions_ = {
        const_cast<char *>("ohos.permission.MANAGE_PRIVATE_PHOTOS"),
        const_cast<char *>("ohos.permission.ACTIVATE_THEME_PACKAGE"),
        const_cast<char *>("ohos.permission.GET_WALLPAPER"),
        const_cast<char *>("ohos.permission.FILE_ACCESS_MANAGER")
    };

    void IsolateTestSetDefaultData(void);
    int IsolateTestAddDacInfo(AppSpawnReqMsgHandle &reqHandle);
    int IsolateTestAddFdInfo(AppSpawnReqMsgHandle &reqHandle);
    int IsolateTestSetMsgFlags(AppSpawnReqMsgHandle reqHandle, uint32_t tlv, uint32_t flags);
    AppSpawnMsgNode *IsolateTestCreateAppSpawnMsg(AppSpawnMsg *msg);
};
}  // namespace OHOS
#endif  // ISOLATE_TEST_HELPER_H
