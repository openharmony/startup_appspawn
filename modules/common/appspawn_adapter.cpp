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

#include "accesstoken_kit.h"
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
#ifdef SECCOMP_PRIVILEGE
#include <dlfcn.h>
#define GET_ALL_PROCESSES "ohos.permission.GET_ALL_PROCESSES"
#define GET_PERMISSION_INDEX "GetPermissionIndex"
using GetPermissionFunc = int32_t (*)(void *, const char *);
#endif
#endif
#define MSG_EXT_NAME_PROCESS_TYPE "ProcessType"
#define NWEBSPAWN_SERVER_NAME "nwebspawn"
#define MAX_USERID_LEN  32
using namespace OHOS::Security::AccessToken;

static const uint64_t TOKEN_ID_LOWMASK = 0xffffffff;
void CheckSpecialSpawnMode(const AppSpawnMgr *content, const AppSpawningCtx *property,
    HapDomainInfo *hapDomainInfo);

int SetAppAccessToken(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    int32_t ret = 0;
    uint64_t tokenId = 0;
    AppSpawnMsgAccessToken *tokenInfo =
        reinterpret_cast<AppSpawnMsgAccessToken *>(GetAppProperty(property, TLV_ACCESS_TOKEN_INFO));
    APPSPAWN_CHECK(tokenInfo != NULL, return APPSPAWN_MSG_INVALID,
        "No access token in msg %{public}s", GetProcessName(property));

    if (IsNWebSpawnMode(content) || IsIsolatedNativeSpawnMode(content, property)) {
        TokenIdKit tokenIdKit;
        tokenId = tokenIdKit.GetRenderTokenID(tokenInfo->accessTokenIdEx);
    } else {
        tokenId = tokenInfo->accessTokenIdEx;
    }
    AccessTokenID accessTokenId = tokenId & TOKEN_ID_LOWMASK;
    ATokenTypeEnum type = AccessTokenKit::GetTokenTypeFlag(accessTokenId);
    APPSPAWN_CHECK(type == TOKEN_HAP, return APPSPAWN_ACCESS_TOKEN_INVALID, "token type %{public}d is invalid", type);

    ret = SetSelfTokenID(tokenId);
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_ACCESS_TOKEN_INVALID,
        "set access token id failed, ret: %{public}d %{public}s", ret, GetProcessName(property));
    APPSPAWN_LOGV("SetAppAccessToken success for %{public}s", GetProcessName(property));
    return 0;
}

#ifdef WITH_SELINUX
void SetHapDomainInfo(const AppSpawnMgr *content, const AppSpawningCtx *property,
    AppSpawnMsgDomainInfo *msgDomainInfo, HapDomainInfo *hapDomainInfo)
{
    hapDomainInfo->apl = msgDomainInfo->apl;
    hapDomainInfo->packageName = GetBundleName(property);
    hapDomainInfo->hapFlags = msgDomainInfo->hapFlags;
    // The value of 0 is invalid. Its purpose is to initialize.
    AppDacInfo *appInfo = reinterpret_cast<AppDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    hapDomainInfo->uid = appInfo == nullptr ? 0 : appInfo->uid;
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_DEBUGGABLE)) {
        hapDomainInfo->hapFlags |= SELINUX_HAP_DEBUGGABLE;
    }
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_DLP_MANAGER_FULL_CONTROL)) {
        hapDomainInfo->hapFlags |= SELINUX_HAP_DLP_FULL_CONTROL;
    }
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_DLP_MANAGER_READ_ONLY)) {
        hapDomainInfo->hapFlags |= SELINUX_HAP_DLP_READ_ONLY;
    }
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX)) {
        hapDomainInfo->hapFlags |= SELINUX_HAP_INPUT_ISOLATE;
    }
#ifdef CUSTOM_SANDBOX
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_CUSTOM_SANDBOX)) {
        hapDomainInfo->hapFlags |= SELINUX_HAP_CUSTOM_SANDBOX;
    }
#endif
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SELINUX_LABEL)) {
        uint32_t len = 0;
        char *extensionTypeChar =
            reinterpret_cast<char *>(GetAppPropertyExt(property, MSG_EXT_NAME_EXTENSION_TYPE, &len));
        std::string extensionType = (extensionTypeChar != nullptr) ? std::string(extensionTypeChar) : "";
        hapDomainInfo->extensionType = extensionType;
    }
    CheckSpecialSpawnMode(content, property, hapDomainInfo);
}
#endif

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
    AppSpawnMsgDomainInfo *msgDomainInfo =
        reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(property, TLV_DOMAIN_INFO));
    APPSPAWN_CHECK(msgDomainInfo != NULL, return APPSPAWN_TLV_NONE,
        "No domain info in req form %{public}s", GetProcessName(property));
    HapDomainInfo hapDomainInfo;
    SetHapDomainInfo(content, property, msgDomainInfo, &hapDomainInfo);
    HapContext hapContext;
    int32_t ret = hapContext.HapDomainSetcontext(hapDomainInfo);
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
#ifdef SECCOMP_PRIVILEGE
        return 0;
#endif
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
    APPSPAWN_CHECK(property != nullptr, return 0, "property is NULL");
    const char *appName = APP_NAME;
    SeccompFilterType type = APP;

    if (IsNWebSpawnMode(content)) {
        uint32_t len = 0;
        char *processTypeChar =
            reinterpret_cast<char *>(GetAppPropertyExt(property, MSG_EXT_NAME_PROCESS_TYPE, &len));
        std::string processType = (processTypeChar != NULL) ? std::string(processTypeChar) : "";
        if (processType == "render") {
            return 0;
        }
    }

#ifdef SECCOMP_PRIVILEGE
    if (IsDeveloperModeOpen()) {
        // Enable high permission seccomp policy for hishell in developer mode.
        if (CheckAppMsgFlagsSet(property, APP_FLAGS_GET_ALL_PROCESSES) != 0) {
            appName = APP_PRIVILEGE;
        }
    }
#endif

#ifdef CUSTOM_SANDBOX
    // Set seccomp policy for custom process.
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_CUSTOM_SANDBOX) != 0) {
        appName = APP_CUSTOM;
    }
#endif

    // Set seccomp policy for input method security mode.
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX) != 0) {
        appName = IMF_EXTENTOIN_NAME;
    }

    // Set seccomp policy for atomic service process.
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ATOMIC_SERVICE) != 0) {
        appName = APP_ATOMIC;
    }

    // Set seccomp policy for processes that have ohos.permission.ALLOW_IOURING.
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ALLOW_IOURING) != 0) {
        appName = APP_ALLOW_IOURING;
    }

    if (!SetSeccompPolicyWithName(type, appName)) {
        APPSPAWN_LOGE("Failed to set %{public}s seccomp filter and exit %{public}d", appName, errno);
        return -EINVAL;
    }
    APPSPAWN_LOGV("SetSeccompPolicyWithName success for %{public}s", appName);
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
#ifndef APPSPAWN_ALLOW_INTERNET_PERMISSION
        DisallowInternet();
#endif
    }
    return 0;
}

static void InitAppCommonEnv(const AppSpawningCtx *property)
{
    AppDacInfo *appInfo = reinterpret_cast<AppDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    if (appInfo == NULL) {
        return;
    }
    const uint32_t userId = appInfo->uid / UID_BASE;
    char user[MAX_USERID_LEN] = {0};
    int len = sprintf_s(user, MAX_USERID_LEN, "%u", userId);
    APPSPAWN_CHECK(len > 0, return, "Failed to format userid: %{public}u", userId);
    int ret = setenv("USER", user, 1);
    APPSPAWN_CHECK(ret == 0, return, "setenv failed, userid:%{public}u, errno: %{public}d", userId, errno);
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

int LoadSeLinuxConfig(void)
{
    return HapContextLoadConfig();
}

uint32_t GetHostId(const AppSpawningCtx *property)
{
    uint32_t hostId = 0; // The value of 0 is invalid. Its purpose is to initialize.
    const char *userId =
        (const char *)(GetAppSpawnMsgExtInfo(property->message, MSG_EXT_NAME_PARENT_UID, nullptr));
    if (userId == nullptr) {
        APPSPAWN_LOGE("AppSpawn get hostId failed, userId is null");
        return hostId;
    }

    char *end;
    errno = 0;
    const int numberBase = 10;
    hostId = strtoul(userId, &end, numberBase);
    APPSPAWN_CHECK(errno != ERANGE && *end == '\0', return 0, "AppSpawn get hostId failed, errno=%d.", errno);
    return hostId;
}

void CheckSpecialSpawnMode(const AppSpawnMgr *content, const AppSpawningCtx *property,
    HapDomainInfo *hapDomainInfo)
{
    if (IsNWebSpawnMode(content)) {
        hapDomainInfo->uid = GetHostId(property);
        uint32_t len = 0;
        char *processTypeChar =
            reinterpret_cast<char *>(GetAppPropertyExt(property, MSG_EXT_NAME_PROCESS_TYPE, &len));
        std::string processType = (processTypeChar != nullptr) ? std::string(processTypeChar) : "";
        if (processType == "render") {
            hapDomainInfo->hapFlags |= SELINUX_HAP_ISOLATED_RENDER;
        } else {
            hapDomainInfo->hapFlags |= SELINUX_HAP_ISOLATED_GPU;
        }
    } else if (IsIsolatedNativeSpawnMode(content, property)) {
        hapDomainInfo->uid = GetHostId(property);
        hapDomainInfo->hapFlags |= SELINUX_HAP_ISOLATED_RENDER;
    }
}
