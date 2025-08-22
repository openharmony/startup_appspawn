/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "securec.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_trace.h"
#include "appspawn_utils.h"
#include "sandbox_core.h"

#define USER_ID_SIZE 16
#define DIR_MODE 0711

int32_t SetAppSandboxProperty(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK(property != nullptr, return -1, "Invalid appspawn client");
    APPSPAWN_CHECK(content != nullptr, return -1, "Invalid appspawn content");

    int ret = 0;
    // no sandbox
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_NO_SANDBOX)) {
        return 0;
    }
    if ((content->content.sandboxNsFlags & CLONE_NEWPID) == CLONE_NEWPID) {
        ret = getprocpid();
        if (ret < 0) {
            return ret;
        }
    }
    uint32_t sandboxNsFlags = CLONE_NEWNS;

    if (OHOS::AppSpawn::SandboxCore::NeedNetworkIsolated(property)) {
        sandboxNsFlags |= content->content.sandboxNsFlags & CLONE_NEWNET ? CLONE_NEWNET : 0;
    }

    APPSPAWN_LOGV("SetAppSandboxProperty sandboxNsFlags 0x%{public}x", sandboxNsFlags);

    StartAppspawnTrace("SetAppSandboxProperty");
    if (IsNWebSpawnMode(content)) {
        ret = OHOS::AppSpawn::SandboxCore::SetAppSandboxPropertyNweb(property, sandboxNsFlags);
    } else {
        ret = OHOS::AppSpawn::SandboxCore::SetAppSandboxProperty(property, sandboxNsFlags);
    }
    FinishAppspawnTrace();
    // for module test do not create sandbox, use APP_FLAGS_IGNORE_SANDBOX to ignore sandbox result
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_IGNORE_SANDBOX)) {
        APPSPAWN_LOGW("Do not care sandbox result %{public}d", ret);
        return 0;
    }
    return ret;
}

static int SpawnMountDirToShared(AppSpawnMgr *content, AppSpawningCtx *property)
{
#ifndef APPSPAWN_SANDBOX_NEW
    if (!IsNWebSpawnMode(content)) {
        // mount dynamic directory
        MountToShared(content, property);
    }
#endif
    return 0;
}

static int UninstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK(property != nullptr && content != nullptr, return -1,
        "Invalid appspawn client or property");
    return OHOS::AppSpawn::SandboxCore::UninstallDebugSandbox(content, property);
}

static int InstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK(property != nullptr && content != nullptr, return -1,
        "Invalid appspawn client or property");
    return OHOS::AppSpawn::SandboxCore::InstallDebugSandbox(content, property);
}

#ifndef APPSPAWN_SANDBOX_NEW
MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load sandbox module ...");
    (void)AddServerStageHook(STAGE_SERVER_PRELOAD, HOOK_PRIO_SANDBOX,
                             OHOS::AppSpawn::SandboxCommon::LoadAppSandboxConfigCJson);
    (void)AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_COMMON, SpawnMountDirToShared);
    (void)AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_SANDBOX, SetAppSandboxProperty);
    (void)AddAppSpawnHook(STAGE_PARENT_UNINSTALL, HOOK_PRIO_SANDBOX, UninstallDebugSandbox);
    (void)AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_SANDBOX, InstallDebugSandbox);
    (void)AddServerStageHook(STAGE_SERVER_EXIT, HOOK_PRIO_SANDBOX,
                             OHOS::AppSpawn::SandboxCommon::FreeAppSandboxConfigCJson);
}
#endif
