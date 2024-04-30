/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef SANDBOX_UTILS_H
#define SANDBOX_UTILS_H

#include <set>
#include <string>
#include <sys/mount.h>
#include <sys/types.h>
#include <vector>

#include "nlohmann/json.hpp"
#include "appspawn_server.h"
#include "appspawn_manager.h"

namespace OHOS {
namespace AppSpawn {
class SandboxUtils {
public:
    static void StoreJsonConfig(nlohmann::json &appSandboxConfig);
    static std::vector<nlohmann::json> &GetJsonConfig();
    static int32_t SetAppSandboxProperty(AppSpawningCtx *client);
    static int32_t SetAppSandboxPropertyNweb(AppSpawningCtx *client);
    static uint32_t GetSandboxNsFlags(bool isNweb);
    static std::set<std::string> GetMountPermissionNames();
    static std::string GetExtraInfoByType(const AppSpawningCtx *appProperty, const std::string &type);
    typedef struct {
        unsigned long mountFlags;
        std::string optionsPoint;
        std::string fsType;
        std::string sandboxPath;
    } SandboxMountConfig;

#ifndef APPSPAWN_TEST
private:
#endif
    static int32_t DoAppSandboxMountOnce(const char *originPath, const char *destinationPath,
                                         const char *fsType, unsigned long mountFlags,
                                         const char *options, mode_t mountSharedFlag = MS_SLAVE);
    static int32_t DoSandboxFileCommonBind(const AppSpawningCtx *appProperty, nlohmann::json &wholeConfig);
    static int32_t DoSandboxFileCommonSymlink(const AppSpawningCtx *appProperty,
                                              nlohmann::json &wholeConfig);
    static int32_t DoSandboxFilePrivateBind(const AppSpawningCtx *appProperty, nlohmann::json &wholeConfig);
    static int32_t DoSandboxFilePrivateSymlink(const AppSpawningCtx *appProperty,
                                               nlohmann::json &wholeConfig);
    static int32_t DoSandboxFilePrivateFlagsPointHandle(const AppSpawningCtx *appProperty,
                                                        nlohmann::json &wholeConfig);
    static int32_t DoSandboxFileCommonFlagsPointHandle(const AppSpawningCtx *appProperty,
                                                       nlohmann::json &wholeConfig);
    static int32_t HandleFlagsPoint(const AppSpawningCtx *appProperty,
                                           nlohmann::json &wholeConfig);
    static int32_t SetPrivateAppSandboxProperty(const AppSpawningCtx *appProperty);
    static int32_t SetCommonAppSandboxProperty(const AppSpawningCtx *appProperty,
                                                      std::string &sandboxPackagePath);
    static int32_t MountAllHsp(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t MountAllGroup(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t DoSandboxRootFolderCreateAdapt(std::string &sandboxPackagePath);
    static int32_t DoSandboxRootFolderCreate(const AppSpawningCtx *appProperty,
                                             std::string &sandboxPackagePath);
    static void DoSandboxChmod(nlohmann::json jsonConfig, std::string &sandboxRoot);
    static int DoAllMntPointsMount(const AppSpawningCtx *appProperty,
        nlohmann::json &appConfig, const std::string &section = "app-base");
    static int DoAllSymlinkPointslink(const AppSpawningCtx *appProperty, nlohmann::json &appConfig);
    static std::string ConvertToRealPath(const AppSpawningCtx *appProperty, std::string path);
    static std::string ConvertToRealPathWithPermission(const AppSpawningCtx *appProperty, std::string path);
    static std::string GetSbxPathByConfig(const AppSpawningCtx *appProperty, nlohmann::json &config);
    static bool CheckTotalSandboxSwitchStatus(const AppSpawningCtx *appProperty);
    static bool CheckAppSandboxSwitchStatus(const AppSpawningCtx *appProperty);
    static bool CheckBundleNameForPrivate(const std::string &bundleName);
    static bool GetSbxSwitchStatusByConfig(nlohmann::json &config);
    static unsigned long GetMountFlagsFromConfig(const std::vector<std::string> &vec);
    static int32_t SetCommonAppSandboxProperty_(const AppSpawningCtx *appProperty,
                                                nlohmann::json &config);
    static int32_t SetPrivateAppSandboxProperty_(const AppSpawningCtx *appProperty,
                                                 nlohmann::json &config);
    static int32_t SetRenderSandboxProperty(const AppSpawningCtx *appProperty,
                                            std::string &sandboxPackagePath);
    static int32_t SetRenderSandboxPropertyNweb(const AppSpawningCtx *appProperty,
                                                std::string &sandboxPackagePath);
    static int32_t SetOverlayAppSandboxProperty(const AppSpawningCtx *appProperty,
                                                std::string &sandboxPackagePath);
    static int32_t SetBundleResourceAppSandboxProperty(const AppSpawningCtx *appProperty,
                                                       std::string &sandboxPackagePath);
    static int32_t DoSandboxFilePermissionBind(AppSpawningCtx *appProperty,
                                               nlohmann::json &wholeConfig);
    static int32_t SetPermissionAppSandboxProperty_(AppSpawningCtx *appProperty,
                                                    nlohmann::json &config);
    static int32_t SetPermissionAppSandboxProperty(AppSpawningCtx *appProperty);
    static int32_t DoAddGid(AppSpawningCtx *appProperty, nlohmann::json &appConfig,
                            const char* permissionName, const std::string &section);
    static bool CheckAppFullMountEnable();
    static int32_t SetSandboxProperty(AppSpawningCtx *appProperty, std::string &sandboxPackagePath);
    static int32_t ChangeCurrentDir(std::string &sandboxPackagePath, const std::string &bundleName,
                                    bool sandboxSharedStatus);
    static int32_t GetMountPermissionFlags(const std::string permissionName);
    static bool GetSandboxDacOverrideEnable(nlohmann::json &config);
    static unsigned long GetSandboxMountFlags(nlohmann::json &config);
    static std::string GetSandboxFsType(nlohmann::json &config);
    static std::string GetSandboxOptions(nlohmann::json &config);
    static std::string GetSandboxPath(const AppSpawningCtx *appProperty, nlohmann::json &mntPoint,
                                      const std::string &section, std::string sandboxRoot);
    static void GetSandboxMountConfig(const std::string &section, nlohmann::json &mntPoint,
                                         SandboxMountConfig &mountConfig);
    static std::vector<nlohmann::json> appSandboxConfig_;
    static bool deviceTypeEnable_;
};
class JsonUtils {
public:
    static bool GetJsonObjFromJson(nlohmann::json &jsonObj, const std::string &jsonPath);
    static bool GetStringFromJson(const nlohmann::json &json, const std::string &key, std::string &value);
};
} // namespace AppSpawn
} // namespace OHOS

int LoadAppSandboxConfig(AppSpawnMgr *content);
#endif  // SANDBOX_UTILS_H
