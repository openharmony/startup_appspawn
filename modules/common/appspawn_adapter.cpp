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

#include "appspawn_adapter.h"

#include "access_token.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "cJSON.h"
#include "token_setproc.h"
#include "tokenid_kit.h"
#include "securec.h"

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#include "selinux/selinux.h"
#endif
#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#include <sys/prctl.h>
const char *RENDERER_NAME = "renderer";
#endif

#define NWEBSPAWN_SERVER_NAME "nwebspawn"
#define MAX_USERID_LEN  32
using namespace OHOS::Security::AccessToken;

int SetAppAccessToken(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    int32_t ret = 0;
    uint64_t tokenId = 0;
    AppSpawnMsgAccessToken *tokenInfo =
        reinterpret_cast<AppSpawnMsgAccessToken *>(GetAppProperty(property, TLV_ACCESS_TOKEN_INFO));
    APPSPAWN_CHECK(tokenInfo != NULL, return APPSPAWN_MSG_INVALID,
        "No access token in msg %{public}s", GetProcessName(property));
    APPSPAWN_LOGV("AppSpawnServer::set access token %{public}" PRId64 " %{public}d",
        tokenInfo->accessTokenIdEx, IsNWebSpawnMode(content));

    if (IsNWebSpawnMode(content)) {
        TokenIdKit tokenIdKit;
        tokenId = tokenIdKit.GetRenderTokenID(tokenInfo->accessTokenIdEx);
    } else {
        tokenId = tokenInfo->accessTokenIdEx;
    }
    ret = SetSelfTokenID(tokenId);
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_ACCESS_TOKEN_INVALID,
        "set access token id failed, ret: %{public}d %{public}s", ret, GetProcessName(property));
    APPSPAWN_LOGV("SetAppAccessToken success for %{public}s", GetProcessName(property));
    return 0;
}

int SetSelinuxCon(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
#ifdef WITH_SELINUX
    APPSPAWN_LOGV("SetSelinuxCon IsDeveloperModeOn %{public}d", IsDeveloperModeOn(property));
    if (GetAppSpawnMsgType(property) == MSG_SPAWN_NATIVE_PROCESS) {
        if (!IsDeveloperModeOn(property)) {
            APPSPAWN_LOGE("Denied Launching a native process: not in developer mode");
            return APPSPAWN_NATIVE_NOT_SUPPORT;
        }
        return 0;
    }
    int32_t ret;
    if (IsNWebSpawnMode(content)) {
        ret = setcon("u:r:isolated_render:s0");
        APPSPAWN_CHECK_ONLY_LOG(ret == 0, "Setcon failed, errno: %{public}d", errno);
        return 0;
    }
    AppSpawnMsgDomainInfo *msgDomainInfo =
        reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(property, TLV_DOMAIN_INFO));
    APPSPAWN_CHECK(msgDomainInfo != NULL, return APPSPAWN_TLV_NONE,
        "No domain info in req form %{public}s", GetProcessName(property))
    HapContext hapContext;
    HapDomainInfo hapDomainInfo;
    hapDomainInfo.apl = msgDomainInfo->apl;
    hapDomainInfo.packageName = GetProcessName(property);
    hapDomainInfo.hapFlags = msgDomainInfo->hapFlags;
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_DEBUGGABLE)) {
        hapDomainInfo.hapFlags |= SELINUX_HAP_DEBUGGABLE;
    }
    ret = hapContext.HapDomainSetcontext(hapDomainInfo);
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ASANENABLED)) {
        ret = 0;
    }
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_ACCESS_TOKEN_INVALID,
        "Set domain context failed, ret: %{public}d %{public}s", ret, GetProcessName(property));
    APPSPAWN_LOGV("SetSelinuxCon success for %{public}s", GetProcessName(property));
#endif
    return 0;
}

int SetUidGidFilter(const AppSpawnMgr *content)
{
#ifdef WITH_SECCOMP
    bool ret = false;
    if (IsNWebSpawnMode(content)) {
        if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
            APPSPAWN_LOGE("Failed to set no new privs");
        }
        ret = SetSeccompPolicyWithName(INDIVIDUAL, NWEBSPAWN_NAME);
    } else {
        ret = SetSeccompPolicyWithName(INDIVIDUAL, APPSPAWN_NAME);
    }
    if (!ret) {
        APPSPAWN_LOGE("Failed to set APPSPAWN seccomp filter and exit");
        _exit(0x7f);
    }
    APPSPAWN_LOGV("SetUidGidFilter success");
#endif
    return 0;
}

int SetSeccompFilter(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
#ifdef WITH_SECCOMP
    const char *appName = APP_NAME;
    SeccompFilterType type = APP;
    if (IsNWebSpawnMode(content)) {
        return 0;
    }
    if (!SetSeccompPolicyWithName(type, appName)) {
        APPSPAWN_LOGE("Failed to set %{public}s seccomp filter and exit %{public}d", appName, errno);
        return -EINVAL;
    }
    APPSPAWN_LOGV("SetSeccompFilter success for %{public}s", GetProcessName(property));
#endif
    return 0;
}

int SetInternetPermission(const AppSpawningCtx *property)
{
    AppSpawnMsgInternetInfo *info =
        reinterpret_cast<AppSpawnMsgInternetInfo *>(GetAppProperty(property, TLV_INTERNET_INFO));
    APPSPAWN_CHECK(info != NULL, return 0,
        "No tlv internet permission info in req form %{public}s", GetProcessName(property));
    APPSPAWN_LOGV("Set internet permission %{public}d %{public}d", info->setAllowInternet, info->allowInternet);
    if (info->setAllowInternet == 1 && info->allowInternet == 0) {
        DisallowInternet();
    }
    return 0;
}

void InitAppCommonEnv(const AppSpawningCtx *property)
{
    AppDacInfo *appInfo = reinterpret_cast<AppDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    if (appInfo == NULL) {
        return;
    }
    const int userId = appInfo->uid / UID_BASE;
    char user[MAX_USERID_LEN] = {0};
    int len = sprintf_s(user, MAX_USERID_LEN, "%d", userId);
    APPSPAWN_CHECK(len > 0, return, "Failed to format userid: %{public}d", userId);
    int ret = setenv("USER", user, 1);
    APPSPAWN_CHECK(ret == 0, return, "setenv failed, userid:%{public}d, errno: %{public}d", userId, errno);
}

int32_t SetEnvInfo(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    InitAppCommonEnv(property);

    uint32_t size = 0;
    char *envStr = reinterpret_cast<char *>(GetAppPropertyExt(property, "AppEnv", &size));
    if (size == 0 || envStr == NULL) {
        return 0;
    }
    int ret = 0;
    cJSON *root = cJSON_Parse(envStr);
    APPSPAWN_CHECK(root != nullptr, return -1, "SetEnvInfo: json parse failed %{public}s", envStr);
    cJSON *config = nullptr;
    cJSON_ArrayForEach(config, root) {
        const char *name = config->string;
        const char *value = cJSON_GetStringValue(config);
        APPSPAWN_LOGV("SetEnvInfo name: %{public}s value: %{public}s", name, value);
        ret = setenv(name, value, 1);
        APPSPAWN_CHECK(ret == 0, break, "setenv failed, errno: %{public}d", errno);
    }
    cJSON_Delete(root);
    APPSPAWN_LOGV("SetEnvInfo success");
    return ret;
}
