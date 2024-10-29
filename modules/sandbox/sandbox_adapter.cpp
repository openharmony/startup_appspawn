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

#include "sandbox_adapter.h"
#include "init_utils.h"

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif

using namespace std;

void MakeAtomicServiceDir(const SandboxContext *context, const char *originPath, const char *varPackageName)
{
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL && originPath != NULL && varPackageName != NULL, return);
    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetSpawningMsgInfo(context, TLV_DAC_INFO);
    APPSPAWN_CHECK(dacInfo != NULL, return, "No dac info for %{public}s", context->bundleName);
    string path(originPath);
    if (path.find("/mnt/share") != string::npos) {
        path = "/data/service/el2/" + to_string(dacInfo->uid / UID_BASE) + "/share/" + string(varPackageName);
    }
    if (access(path.c_str(), F_OK) == 0) {
        APPSPAWN_LOGV("path %{public}s already exist, no need to recreate", path.c_str());
        return;
    }
    int ret = mkdir(path.c_str(), S_IRWXU);
    APPSPAWN_CHECK(ret == 0, return, "mkdir %{public}s failed, errno %{public}d", path.c_str(), errno);

    if (path.find("/database") != string::npos || path.find("/data/service/el2") != string::npos) {
        ret = chmod(path.c_str(), S_IRWXU | S_IRWXG | S_ISGID);
    } else if (path.find("/log") != string::npos) {
        ret = chmod(path.c_str(), S_IRWXU | S_IRWXG);
    }
    APPSPAWN_CHECK(ret == 0, return, "chmod %{public}s failed, errno %{public}d", path.c_str(), errno);

#ifdef WITH_SELINUX
    AppSpawnMsgDomainInfo *msgDomainInfo = (AppSpawnMsgDomainInfo *)GetSpawningMsgInfo(context, TLV_DOMAIN_INFO);
    APPSPAWN_CHECK(msgDomainInfo != NULL, return, "No domain info for %{public}s", context->bundleName);
    HapContext hapContext;
    HapFileInfo hapFileInfo;
    hapFileInfo.pathNameOrig.push_back(path);
    hapFileInfo.apl = msgDomainInfo->apl;
    hapFileInfo.packageName = context->bundleName;
    hapFileInfo.hapFlags = msgDomainInfo->hapFlags;
    if (CheckAppSpawnMsgFlag(context->message, TLV_MSG_FLAGS, APP_FLAGS_DEBUGGABLE)) {
        hapFileInfo.hapFlags |= SELINUX_HAP_DEBUGGABLE;
    }
    if ((path.find("/base") != string::npos) || (path.find("/database") != string::npos)) {
        ret = hapContext.HapFileRestorecon(hapFileInfo);
        APPSPAWN_CHECK(ret == 0, return, "set dir %{public}s selinuxLabel failed, apl %{public}s, ret %{public}d",
            path.c_str(), hapFileInfo.apl.c_str(), ret);
    }
#endif
    if (path.find("/base") != string::npos || path.find("/data/service/el2") != string::npos) {
        ret = chown(path.c_str(), dacInfo->uid, dacInfo->gid);
    } else if (path.find("/database") != string::npos) {
        ret = chown(path.c_str(), dacInfo->uid, DecodeGid("ddms"));
    } else if (path.find("/log") != string::npos) {
        ret = chown(path.c_str(), dacInfo->uid, DecodeGid("log"));
    }
    APPSPAWN_CHECK(ret == 0, return, "chown %{public}s failed, errno %{public}d", path.c_str(), errno);
    return;
}
