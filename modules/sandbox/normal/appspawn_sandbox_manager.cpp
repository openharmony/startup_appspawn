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
#include "appspawn_utils.h"
#include "sandbox_core.h"

#define USER_ID_SIZE 16
#define DIR_MODE 0711

int32_t SetAppSandboxProperty(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK(property != nullptr, return -1, "Invalid appspwn client");
    APPSPAWN_CHECK(content != nullptr, return -1, "Invalid appspwn content");
    // clear g_mountInfo in the child process
    std::map<std::string, int>* mapPtr = static_cast<std::map<std::string, int>*>(GetEl1BundleMountCount());
    if (mapPtr == nullptr) {
        APPSPAWN_LOGE("Get el1 bundle mount count failed");
        return APPSPAWN_ARG_INVALID;
    }
    mapPtr->clear();
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

    if (IsNWebSpawnMode(content)) {
        ret = OHOS::AppSpawn::SandboxCore::SetAppSandboxPropertyNweb(property, sandboxNsFlags);
    } else {
        ret = OHOS::AppSpawn::SandboxCore::SetAppSandboxProperty(property, sandboxNsFlags);
    }
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

static void UmountDir(const char *rootPath, const char *targetPath, const AppSpawnedProcessInfo *appInfo)
{
    size_t allPathSize = strlen(rootPath) + USER_ID_SIZE + strlen(appInfo->name) + strlen(targetPath) + 2;
    char *path = reinterpret_cast<char *>(malloc(sizeof(char) * (allPathSize)));
    APPSPAWN_CHECK(path != nullptr, return, "Failed to malloc path");

    int ret = sprintf_s(path, allPathSize, "%s%u/%s%s", rootPath, appInfo->uid / UID_BASE,
        appInfo->name, targetPath);
    APPSPAWN_CHECK(ret > 0 && ((size_t)ret < allPathSize), free(path);
        return, "Failed to get sandbox path errno %{public}d", errno);

    ret = umount2(path, MNT_DETACH);
    if (ret == 0) {
        APPSPAWN_LOGI("Umount2 sandbox path %{public}s success", path);
    } else {
        APPSPAWN_LOGW("Failed to umount2 sandbox path %{public}s errno %{public}d", path, errno);
    }
    free(path);
}

static int UmountSandboxPath(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
{
    APPSPAWN_CHECK(content != nullptr && appInfo != nullptr && appInfo->name != NULL,
        return -1, "Invalid content or appInfo");
    if (!IsAppSpawnMode(content)) {
        return 0;
    }
    APPSPAWN_LOGV("UmountSandboxPath name %{public}s pid %{public}d", appInfo->name, appInfo->pid);
    const char rootPath[] = "/mnt/sandbox/";
    const char el1Path[] = "/data/storage/el1/bundle";

    std::string varBundleName = std::string(appInfo->name);
    if (appInfo->appIndex > 0) {
        varBundleName = "+clone-" + std::to_string(appInfo->appIndex) + "+" + varBundleName;
    }

    uint32_t userId = appInfo->uid / UID_BASE;
    std::string key = std::to_string(userId) + "-" + varBundleName;
    std::map<std::string, int> *el1BundleCountMap = static_cast<std::map<std::string, int>*>(GetEl1BundleMountCount());
    if (el1BundleCountMap == nullptr || el1BundleCountMap->find(key) == el1BundleCountMap->end()) {
        return 0;
    }
    (*el1BundleCountMap)[key]--;
    if ((*el1BundleCountMap)[key] == 0) {
        APPSPAWN_LOGV("no app %{public}s use it in userId %{public}u, need umount", appInfo->name, userId);
        UmountDir(rootPath, el1Path, appInfo);
        el1BundleCountMap->erase(key);
    } else {
        APPSPAWN_LOGV("app %{public}s use it mount times %{public}d in userId %{public}u, not need umount",
            appInfo->name, (*el1BundleCountMap)[key], userId);
    }
    return 0;
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
    (void)AddProcessMgrHook(STAGE_SERVER_APP_UMOUNT, HOOK_PRIO_SANDBOX, UmountSandboxPath);
    (void)AddServerStageHook(STAGE_SERVER_EXIT, HOOK_PRIO_SANDBOX,
                             OHOS::AppSpawn::SandboxCommon::FreeAppSandboxConfigCJson);
}
#endif