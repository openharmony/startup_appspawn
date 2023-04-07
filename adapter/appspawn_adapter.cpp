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
    int32_t ret = SetSelfTokenID(appProperty->property.accessTokenIdEx);
    APPSPAWN_LOGV("AppSpawnServer::set access token id = %{public}llu, ret = %{public}d %{public}d",
        appProperty->property.accessTokenIdEx, ret, getuid());
}

void SetSelinuxCon(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifdef WITH_SELINUX
    UNUSED(content);
    AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
    HapContext hapContext;
    HapDomainInfo hapDomainInfo;
    hapDomainInfo.apl = appProperty->property.apl;
    hapDomainInfo.packageName = appProperty->property.processName;
    hapDomainInfo.hapFlags = appProperty->property.hapFlags;
    int32_t ret = hapContext.HapDomainSetcontext(hapDomainInfo);
    if (ret != 0) {
        APPSPAWN_LOGE("AppSpawnServer::Failed to hap domain set context, errno = %{public}d %{public}s",
            errno, appProperty->property.apl);
    } else {
        APPSPAWN_LOGV("AppSpawnServer::Success to hap domain set context, ret = %{public}d", ret);
    }
#endif
}

void SetUidGidFilter(struct AppSpawnContent_ *content)
{
#ifdef WITH_SECCOMP
    if (!SetSeccompPolicyWithName(INDIVIDUAL, APPSPAWN_NAME)) {
        APPSPAWN_LOGE("Failed to set APPSPAWN seccomp filter and exit");
#ifndef APPSPAWN_TEST
        _exit(0x7f);
#endif
    } else {
        APPSPAWN_LOGI("Success to set APPSPAWN seccomp filter");
    }
#endif
}

int SetSeccompFilter(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifdef WITH_SECCOMP
#ifdef NWEB_SPAWN
    const char *appName = NWEBSPAWN_NAME;
    SeccompFilterType type = INDIVIDUAL;
#else
    const char *appName = APP_NAME;
    SeccompFilterType type = APP;
#endif
    if (!SetSeccompPolicyWithName(type, appName)) {
        APPSPAWN_LOGE("Failed to set %{public}s seccomp filter and exit", appName);
#ifndef APPSPAWN_TEST
        return -EINVAL;
#endif
    } else {
        APPSPAWN_LOGI("Success to set %{public}s seccomp filter", appName);
    }
#endif
    return 0;
}

void HandleInternetPermission(const AppSpawnClient *client)
{
    AppSpawnClientExt *appPropertyExt = (AppSpawnClientExt *)client;
    APPSPAWN_LOGV("HandleInternetPermission id %{public}d setAllowInternet %hhu allowInternet %hhu",
        client->id, appPropertyExt->setAllowInternet, appPropertyExt->allowInternet);
    if (appPropertyExt->setAllowInternet == 1 && appPropertyExt->allowInternet == 0) {
        DisallowInternet();
    }
}
