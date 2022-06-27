/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "appspawn_adapter.h"

#include <cerrno>

#include "appspawn_service.h"
#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif
#include "token_setproc.h"

void SetAppAccessToken(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    int32_t ret = SetSelfTokenID(appProperty->property.accessTokenId);
    APPSPAWN_LOGI("AppSpawnServer::set access token id = %d, ret = %d %d",
        appProperty->property.accessTokenId, ret, getuid());
}

void SetSelinuxCon(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifdef WITH_SELINUX
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    HapContext hapContext;
    int32_t ret = hapContext.HapDomainSetcontext(appProperty->property.apl, appProperty->property.processName);
    if (ret != 0) {
        APPSPAWN_LOGE("AppSpawnServer::Failed to hap domain set context, errno = %d %s",
            errno, appProperty->property.apl);
    } else {
        APPSPAWN_LOGI("AppSpawnServer::Success to hap domain set context, ret = %d", ret);
    }
#endif
}
