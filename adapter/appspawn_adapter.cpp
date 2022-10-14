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
#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#endif


void SetAppAccessToken(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
    int32_t ret = SetSelfTokenID(appProperty->property.accessTokenId);
    APPSPAWN_LOGI("AppSpawnServer::set access token id = %d, ret = %d %d",
        appProperty->property.accessTokenId, ret, getuid());
}

void SetSelinuxCon(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifdef WITH_SELINUX
    UNUSED(content);
    AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
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

void SetUidGidFilter(struct AppSpawnContent_ *content)
{
#ifdef WITH_SECCOMP
    if (!SetSeccompPolicyWithName(APPSPAWN_NAME)) {
        APPSPAWN_LOGE("AppSpawnServer::Failed to set APPSPAWN seccomp filter");
    } else {
        APPSPAWN_LOGI("AppSpawnServer::Success to set APPSPAWN seccomp filter");
    }
#endif
}

void SetSeccompFilter(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifdef WITH_SECCOMP
#ifdef NWEB_SPAWN
    if (!SetSeccompPolicyWithName(NWEBSPAWN_NAME)) {
        APPSPAWN_LOGE("NwebspawnServer::Failed to set NWEBSPAWN seccomp filter");
    } else {
        APPSPAWN_LOGI("NwebspawnServer::Success to set NWEBSPAWN seccomp filter");
    }
#else
    if (!SetSeccompPolicyWithName(APP_NAME)) {
        APPSPAWN_LOGE("AppSpawnServer::Failed to set APP seccomp filter");
    } else {
        APPSPAWN_LOGI("AppSpawnServer::Success to set APP seccomp filter");
    }
#endif
#endif
}
