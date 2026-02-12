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

#ifndef CODESIGN_TEST_HELPER_H
#define CODESIGN_TEST_HELPER_H

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
class CodeSignTestHelper {
public:
    CodeSignTestHelper()
    {
        CodeSignTestSetDefaultData();
    }
    ~CodeSignTestHelper() {}

    AppSpawnReqMsgHandle CodeSignTestCreateSendMsg(AppSpawnClientHandle handle, uint32_t msgType = MSG_APP_SPAWN);
    AppSpawningCtx *CodeSignTestGetAppProperty(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle);

private:
    uint32_t codeSignTestDefaultMsgFlags_ = 0;
    uid_t codeSignTestDefaultTestUid_ = 0;
    gid_t codeSignTestDefaultTestGid_ = 0;
    gid_t codeSignTestDefaultTestGidGroup_ = 0;
    int32_t codeSignTestDefaultTestBundleIndex_ = 0;
    std::string codeSignTestProcessName_;
    std::string codeSignTestDefaultApl_ = "system_core";
    std::vector<const char *> codeSignTestPermissions_ = {
        const_cast<char *>("ohos.permission.MANAGE_PRIVATE_PHOTOS"),
        const_cast<char *>("ohos.permission.ACTIVATE_THEME_PACKAGE"),
        const_cast<char *>("ohos.permission.GET_WALLPAPER"),
        const_cast<char *>("ohos.permission.FILE_ACCESS_MANAGER")
    };

    void CodeSignTestSetDefaultData(void);
    int CodeSignTestAddDacInfo(AppSpawnReqMsgHandle &reqHandle);
    int CodeSignTestAddFdInfo(AppSpawnReqMsgHandle &reqHandle);
    int CodeSignTestSetMsgFlags(AppSpawnReqMsgHandle reqHandle, uint32_t tlv, uint32_t flags);
    AppSpawnMsgNode *CodeSignTestCreateAppSpawnMsg(AppSpawnMsg *msg);
};
}  // namespace OHOS
#endif  // CODESIGN_TEST_HELPER_H
