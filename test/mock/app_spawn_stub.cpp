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

#include "app_spawn_stub.h"

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdbool>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>

#include <linux/capability.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "access_token.h"
#include "hilog/log.h"
#include "securec.h"
#include "token_setproc.h"
#include "tokenid_kit.h"

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif
#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#include <sys/prctl.h>
#endif

static uint32_t g_preloadParamResult = 0;
static uint32_t g_preloadEtsParamResult = 0;
void SetBoolParamResult(const char *key, bool flag)
{
    if (strcmp(key, "persist.appspawn.preload") == 0) {
        flag ? (g_preloadParamResult = true) : (g_preloadParamResult = false);
    }
    if (strcmp(key, "persist.appspawn.preloadets") == 0) {
        flag ? (g_preloadEtsParamResult = true) : (g_preloadEtsParamResult = false);
    }
}

namespace OHOS {
namespace system {
    bool GetIntParameter(const std::string &key, bool def, bool arg1 = false, bool arg2 = false)
    {
        return def;
    }

    bool GetBoolParameter(const std::string &key, bool def)
    {
        if (strcmp(key.c_str(), "persist.appspawn.preload") == 0) {
            return g_preloadParamResult ? true : false;
        }
        if (strcmp(key.c_str(), "persist.appspawn.preloadets") == 0) {
            return g_preloadEtsParamResult ? true : false;
        }
        return def;
    }
}  // namespace system

namespace Security {
    namespace AccessToken {
        uint64_t TokenIdKit::GetRenderTokenID(uint64_t tokenId)
        {
            return tokenId;
        }
    }  // namespace AccessToken
}  // namespace Security
}  // namespace OHOS

#ifdef WITH_SELINUX
HapContext::HapContext() {}
HapContext::~HapContext() {}
int HapContext::HapDomainSetcontext(HapDomainInfo &hapDomainInfo)
{
    return 0;
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
void ResetParamSecurityLabel() {}

int SetSelfTokenID(uint64_t tokenId)
{
    return 0;
}

void SetTraceDisabled(int disable) {}

#ifdef WITH_SECCOMP
bool SetSeccompPolicyWithName(SeccompFilterType filter, const char *filterName)
{
    return true;
}

bool IsEnableSeccomp(void)
{
    return true;
}
#endif

int GetControlSocket(const char *name)
{
    return -1;
}

static bool g_developerMode = true;
void SetDeveloperMode(bool mode)
{
    g_developerMode = mode;
}

int GetParameter(const char *key, const char *def, char *value, uint32_t len)
{
    static uint32_t count = 0;
    count++;
    if (strcmp(key, "startup.appspawn.cold.boot") == 0) {
        return strcpy_s(value, len, "true") == 0 ? strlen("true") : -1;
    }
    if (strcmp(key, "persist.appspawn.reqMgr.timeout") == 0) {
        const char *tmp = def;
        if ((count % 3) == 0) { // 3 test
            return -1;
        } else if ((count % 3) == 1) { // 3 test
            tmp = "a";
        } else {
            tmp = "5";
        }
        return strcpy_s(value, len, tmp) == 0 ? strlen(tmp) : -1;
    }
    if (strcmp(key, "const.security.developermode.state") == 0) {
        return g_developerMode ? (strcpy_s(value, len, "true") == 0 ? strlen("true") : -1) : -1;
    }
    if (strcmp(key, "persist.nweb.sandbox.src_path") == 0) {
        return strcpy_s(value, len, def) == 0 ? strlen(def) : -1;
    }
    if (strcmp(key, "test.variable.001") == 0) {
        return strcpy_s(value, len, "test.variable.001") == 0 ? strlen("test.variable.001") : -1;
    }
    if (strcmp(key, "persist.arkwebcore.package_name") == 0) {
        return strcpy_s(value, len, "com.ohos.arkwebcore") == 0 ? strlen("com.ohos.arkwebcore") : -1;
    }
    if (strcmp(key, "persist.arkwebcore.install_path") == 0) {
        return strcpy_s(value, len, "/data/app/el1/bundle/public/com.ohos.nweb/ArkWWebCore.hap") == 0 ?
            strlen("/data/app/el1/bundle/public/com.ohos.nweb/ArkWWebCore.hap") : -1;
    }
    return -1;
}

int SetParameter(const char *key, const char *value)
{
    return 0;
}

int InUpdaterMode(void)
{
    return 0;
}


#ifdef __cplusplus
}
#endif
