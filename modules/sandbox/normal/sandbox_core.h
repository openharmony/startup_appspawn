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

#ifndef SANDBOX_CORE_H
#define SANDBOX_CORE_H

#include <string>
#include <vector>
#include <map>
#include "sandbox_def.h"
#include "appspawn_msg.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "sandbox_shared_mount.h"
#include "sandbox_common.h"

namespace OHOS {
namespace AppSpawn {

class SandboxCore {
public:
    // 沙箱挂载公共处理
    static int32_t DoAllMntPointsMount(const AppSpawningCtx *appProperty, cJSON *appConfig,
        const char *typeName, const std::string &section = "app-base");
    static int32_t DoAddGid(AppSpawningCtx *appProperty, cJSON *appConfig,
                            const char* permissionName, const std::string &section);
    static int32_t DoAllSymlinkPointslink(const AppSpawningCtx *appProperty, cJSON *appConfig);
    static int32_t DoSandboxRootFolderCreate(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t DoSandboxRootFolderCreateAdapt(std::string &sandboxPackagePath);
    static int32_t HandleFlagsPoint(const AppSpawningCtx *appProperty, cJSON *appConfig);
    static int32_t SetOverlayAppSandboxProperty(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t SetBundleResourceAppSandboxProperty(const AppSpawningCtx *appProperty,
                                                       std::string &sandboxPackagePath);
    static bool NeedNetworkIsolated(AppSpawningCtx *property);

    // 处理应用沙箱挂载
    static int32_t SetCommonAppSandboxProperty(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t SetPrivateAppSandboxProperty(const AppSpawningCtx *appProperty);
    static int32_t SetPermissionAppSandboxProperty(AppSpawningCtx *appProperty);
    static int32_t SetSandboxProperty(AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t SetAppSandboxProperty(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags = CLONE_NEWNS);

    static int32_t SetRenderSandboxPropertyNweb(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t SetAppSandboxPropertyNweb(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags = CLONE_NEWNS);

    // 沙箱目录切根
    static int32_t ChangeCurrentDir(std::string &sandboxPackagePath, const std::string &bundleName,
                                    bool sandboxSharedStatus);

    // 设置DEC规则
    static int32_t SetDecWithDir(const AppSpawningCtx *appProperty, uint32_t userId);
    static int32_t SetDecPolicyWithPermission(const AppSpawningCtx *appProperty, SandboxMountConfig &mountConfig);
    static void SetDecDenyWithDir(const AppSpawningCtx *appProperty);

    // debug hap
    static int32_t UninstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property);
    static int32_t InstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property);

private:
    // 获取应用信息
    static int EnableSandboxNamespace(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags);
    static uint32_t GetAppMsgFlags(const AppSpawningCtx *property);
    static bool CheckMountFlag(const AppSpawningCtx *appProperty, const std::string bundleName,
                               cJSON *appConfig);
    static void UpdateMsgFlagsWithPermission(AppSpawningCtx *appProperty,
        const std::string &permissionMode, uint32_t flag);
    static int32_t UpdatePermissionFlags(AppSpawningCtx *appProperty);
    static std::string GetSandboxPath(const AppSpawningCtx *appProperty, cJSON *mntPoint,
                                      const std::string &section, std::string sandboxRoot);

    // 解析挂载信息公共函数
    static cJSON *GetFirstCommonConfig(cJSON *wholeConfig, const char *prefix);
    static cJSON *GetFirstSubConfig(cJSON *parent, const char *key);

    // 处理dlpmanager挂载
    static int32_t DoDlpAppMountStrategy(const AppSpawningCtx *appProperty, const std::string &srcPath,
        const std::string &sandboxPath, const std::string &fsType, unsigned long mountFlags);
    static int32_t HandleSpecialAppMount(const AppSpawningCtx *appProperty, const std::string &srcPath,
        const std::string &sandboxPath, const std::string &fsType, unsigned long mountFlags);

    // 处理应用私有挂载
    static cJSON *GetPrivateJsonInfo(const AppSpawningCtx *appProperty, cJSON *wholeConfig);
    static int32_t DoSandboxFilePrivateBind(const AppSpawningCtx *appProperty, cJSON *wholeConfig);
    static int32_t DoSandboxFilePrivateSymlink(const AppSpawningCtx *appProperty, cJSON *wholeConfig);
    static int32_t DoSandboxFilePrivateFlagsPointHandle(const AppSpawningCtx *appProperty, cJSON *wholeConfig);
    static int32_t SetPrivateAppSandboxProperty_(const AppSpawningCtx *appProperty, cJSON *config);

    // 处理应用基于权限挂载
    static int32_t DoSandboxFilePermissionBind(AppSpawningCtx *appProperty, cJSON *wholeConfig);
    static int32_t SetPermissionAppSandboxProperty_(AppSpawningCtx *appProperty, cJSON *config);

    // 处理应用公共挂载
    static int32_t DoSandboxFileCommonBind(const AppSpawningCtx *appProperty, cJSON *wholeConfig);
    static int32_t DoSandboxFileCommonSymlink(const AppSpawningCtx *appProperty, cJSON *wholeConfig);
    static int32_t DoSandboxFileCommonFlagsPointHandle(const AppSpawningCtx *appProperty, cJSON *wholeConfig);
    static int32_t SetCommonAppSandboxProperty_(const AppSpawningCtx *appProperty, cJSON *config);

    // 处理可变参数的挂载
    static int32_t MountAllHsp(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t MountAllGroup(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath);

    // 沙箱回调函数
    static int32_t ProcessMountPoint(cJSON *mntPoint, MountPointProcessParams &params);

    // debug hap
    static std::string ConvertDebugRealPath(const AppSpawningCtx *appProperty, std::string path);
    static void DoUninstallDebugSandbox(std::vector<std::string> &bundleList, cJSON *mountPoints);
    static int32_t GetPackageList(AppSpawningCtx *property, std::vector<std::string> &bundleList, bool tmp);

    static int32_t DoMountDebugPoints(const AppSpawningCtx *appProperty, cJSON *appConfig);
    static int32_t MountDebugSharefs(const AppSpawningCtx *property, const char *src, const char *target);
};

} // namespace AppSpawn
} // namespace OHOS

#endif // SANDBOX_CORE_H
