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
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "preload_client.h"
#include "securec.h"

static int CheckSupportDfxColdStart(const AppSpawningCtx *property)
{
    if (property->client.flags & APP_COLD_START) {
        APPSPAWN_LOGW("Asan cold start is already enabled, do not enable dfx cold start");
        return APPSPAWN_PRELOAD_DFX_NOT_ALLOW;
    }

    if (!CheckAppMsgFlagsSet(property, APP_FLAGS_DEBUGGABLE) ||
        !CheckEnabled("const.security.developermode.state", "true") ||
        !CheckEnabled("hiviewdfx.hiprofiler.preload", "1")) {
        return APPSPAWN_PRELOAD_DFX_NOT_ALLOW;
    }

    return 0;
}

#define DFX_PRELOAD_PATH_LEN 256
APPSPAWN_STATIC int SetDfxPreloadEnableEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK_ONLY_EXPER(GetAppSpawnMsgType(property) != MSG_SPAWN_NATIVE_PROCESS, return 0);
    const char *bundleName = GetBundleName(property);
    APPSPAWN_CHECK(bundleName != NULL, return APPSPAWN_ARG_INVALID, "Can not get bundle name");

    if (CheckSupportDfxColdStart(property) == 0) {
        char buff[DFX_PRELOAD_PATH_LEN] = {0};
        (void)memset_s(buff, sizeof(buff), 0, sizeof(buff));
        bool ret = GetPreloadSoList(bundleName, buff, sizeof(buff));
        APPSPAWN_CHECK(ret && (strlen(buff) > 0), return 0, "Failed to get preload so list");
        APPSPAWN_CHECK(strstr(buff, "..") == nullptr, return 0, "Invalid preload so list %{public}s", buff);

        /* If there are multiple lib so in the preload path, they should be separated by colons */
        APPSPAWN_LOGI("SetDfxPreloadEnableEnv cold start app %{public}s load %{public}s", bundleName, buff);
        setenv("LD_PRELOAD", buff, 1);
        property->client.flags |= APP_COLD_START;
    }

    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    /* SetDfxPreloadEnableEnv needs to be executed after AsanSpawnInitSpawningEnv */
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, HOOK_PRIO_DFX_PRELOAD, SetDfxPreloadEnableEnv);
}
