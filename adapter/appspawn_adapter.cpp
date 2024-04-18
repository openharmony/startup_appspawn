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
#include "param_helper.h"

#include <cerrno>

#include "selinux/selinux.h"

#include "appspawn_service.h"
#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif
#include "token_setproc.h"
#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#include <sys/prctl.h>

const char* RENDERER_NAME = "renderer";
#endif

#include "tokenid_kit.h"
#include "access_token.h"
#define NWEBSPAWN_SERVER_NAME "nwebspawn"
using namespace OHOS::Security::AccessToken;

int SetAppAccessToken(struct AppSpawnContent *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
    int32_t ret = 0;
    uint64_t tokenId = 0;
    if (content->isNweb) {
        TokenIdKit tokenIdKit;
        tokenId = tokenIdKit.GetRenderTokenID(appProperty->property.accessTokenIdEx);
        if (tokenId == static_cast<uint64_t>(INVALID_TOKENID)) {
            APPSPAWN_LOGE("AppSpawnServer::Failed to get render token id, renderTokenId =%{public}llu",
                static_cast<unsigned long long>(tokenId));
            return -1;
        }
    } else {
        tokenId = appProperty->property.accessTokenIdEx;
    }
    ret = SetSelfTokenID(tokenId);
    if (ret != 0) {
        APPSPAWN_LOGE("AppSpawnServer::set access token id failed, ret = %{public}d", ret);
        return -1;
    }

    APPSPAWN_LOGV("AppSpawnServer::set access token id = %{public}llu, ret = %{public}d %{public}d",
        static_cast<unsigned long long>(appProperty->property.accessTokenIdEx), ret, getuid());
    return 0;
}

int SetSelinuxCon(struct AppSpawnContent *content, AppSpawnClient *client)
{
#ifdef WITH_SELINUX
    if (content->isNweb) {
        setcon("u:r:isolated_render:s0");
    } else {
        UNUSED(content);
        AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
        if (appProperty->property.code == SPAWN_NATIVE_PROCESS) {
            if (!IsDeveloperModeOn()) {
                APPSPAWN_LOGE("Denied Launching a native process: not in developer mode");
                return -1;
            }
            return 0;
        }
        HapContext hapContext;
        HapDomainInfo hapDomainInfo;
        hapDomainInfo.apl = appProperty->property.apl;
        hapDomainInfo.packageName = appProperty->property.processName;
        hapDomainInfo.hapFlags = appProperty->property.hapFlags;
        if ((appProperty->property.flags & APP_DEBUGGABLE) != 0) {
            hapDomainInfo.hapFlags |= SELINUX_HAP_DEBUGGABLE;
        }
        int32_t ret = hapContext.HapDomainSetcontext(hapDomainInfo);
        if (ret != 0) {
            APPSPAWN_LOGE("AppSpawnServer::Failed to hap domain set context, errno = %{public}d %{public}s",
                errno, appProperty->property.apl);
            return -1;
        } else {
            APPSPAWN_LOGV("AppSpawnServer::Success to hap domain set context, ret = %{public}d", ret);
        }
    }
#endif
    return 0;
}

void SetUidGidFilter(struct AppSpawnContent *content)
{
#ifdef WITH_SECCOMP
    bool ret = false;
    if (content->isNweb) {
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
            APPSPAWN_LOGE("Failed to set no new privs");
        }
        ret = SetSeccompPolicyWithName(INDIVIDUAL, NWEBSPAWN_NAME);
    } else {
        ret = SetSeccompPolicyWithName(INDIVIDUAL, APPSPAWN_NAME);
    }
    if (!ret) {
        APPSPAWN_LOGE("Failed to set APPSPAWN seccomp filter and exit");
#ifndef APPSPAWN_TEST
        _exit(0x7f);
#endif
    } else {
        APPSPAWN_LOGI("Success to set APPSPAWN seccomp filter");
    }
#endif
}

int SetSeccompFilter(struct AppSpawnContent *content, AppSpawnClient *client)
{
#ifdef WITH_SECCOMP
    const char *appName = APP_NAME;
    SeccompFilterType type = APP;
    if (content->isNweb) {
        return 0;
    }
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
    const AppSpawnClientExt *appPropertyExt = reinterpret_cast<const AppSpawnClientExt *>(client);
    APPSPAWN_CHECK(appPropertyExt != nullptr, return, "Invalid client");
    APPSPAWN_LOGV("HandleInternetPermission id %{public}d setAllowInternet %hhu allowInternet %hhu",
        client->id, appPropertyExt->property.setAllowInternet, appPropertyExt->property.allowInternet);
    if (appPropertyExt->property.setAllowInternet == 1 && appPropertyExt->property.allowInternet == 0) {
        DisallowInternet();
    }
}
