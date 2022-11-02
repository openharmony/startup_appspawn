/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#include <string>
#include <vector>
#include <sys/types.h>
#include "nlohmann/json.hpp"
#include "client_socket.h"

namespace OHOS {
namespace AppSpawn {
class SandboxUtils {
public:
    static void StoreNamespaceJsonConfig(nlohmann::json &appNamespaceConfig);
    static nlohmann::json GetNamespaceJsonConfig(void);
    static void StoreJsonConfig(nlohmann::json &appSandboxConfig);
    static nlohmann::json GetJsonConfig();
    static void StoreProductJsonConfig(nlohmann::json &productSandboxConfig);
    static nlohmann::json GetProductJsonConfig();
    static int32_t SetAppSandboxProperty(const ClientSocket::AppProperty *appProperty);
    static uint32_t GetNamespaceFlagsFromConfig(const char *bundleName);

private:
    static int32_t DoAppSandboxMountOnce(const char *originPath, const char *destinationPath,
                                         const char *fsType, unsigned long mountFlags,
                                         const char *options);
    static int32_t DoSandboxFileCommonBind(const ClientSocket::AppProperty *appProperty, nlohmann::json &wholeConfig);
    static int32_t DoSandboxFileCommonSymlink(const ClientSocket::AppProperty *appProperty,
                                              nlohmann::json &wholeConfig);
    static int32_t DoSandboxFilePrivateBind(const ClientSocket::AppProperty *appProperty, nlohmann::json &wholeConfig);
    static int32_t DoSandboxFilePrivateSymlink(const ClientSocket::AppProperty *appProperty,
                                               nlohmann::json &wholeConfig);
    static int32_t DoSandboxFilePrivateFlagsPointHandle(const ClientSocket::AppProperty *appProperty,
                                                        nlohmann::json &wholeConfig);
    static int32_t DoSandboxFileCommonFlagsPointHandle(const ClientSocket::AppProperty *appProperty,
                                                       nlohmann::json &wholeConfig);
    static int32_t HandleFlagsPoint(const ClientSocket::AppProperty *appProperty,
                                           nlohmann::json &wholeConfig);
    static int32_t SetPrivateAppSandboxProperty(const ClientSocket::AppProperty *appProperty);
    static int32_t SetCommonAppSandboxProperty(const ClientSocket::AppProperty *appProperty,
                                                      std::string &sandboxPackagePath);
    static int32_t DoSandboxRootFolderCreateAdapt(std::string &sandboxPackagePath);
    static int32_t DoSandboxRootFolderCreate(const ClientSocket::AppProperty *appProperty,
                                             std::string &sandboxPackagePath);
    static void DoSandboxChmod(nlohmann::json jsonConfig, std::string &sandboxRoot);
    static int DoAllMntPointsMount(const ClientSocket::AppProperty *appProperty, nlohmann::json &appConfig);
    static int DoAllSymlinkPointslink(const ClientSocket::AppProperty *appProperty, nlohmann::json &appConfig);
    static std::string ConvertToRealPath(const ClientSocket::AppProperty *appProperty, std::string sandboxRoot);
    static std::string GetSbxPathByConfig(const ClientSocket::AppProperty *appProperty, nlohmann::json &config);
    static void CheckAndPrepareSrcPath(const ClientSocket::AppProperty *appProperty, const std::string &srcPath);
    static bool CheckTotalSandboxSwitchStatus(const ClientSocket::AppProperty *appProperty);
    static bool CheckAppSandboxSwitchStatus(const ClientSocket::AppProperty *appProperty);
    static bool CheckBundleNameForPrivate(const std::string &bundleName);
    static bool GetSbxSwitchStatusByConfig(nlohmann::json &config);
    static unsigned long GetMountFlagsFromConfig(const std::vector<std::string> &vec);
    static int32_t SetCommonAppSandboxProperty_(const ClientSocket::AppProperty *appProperty,
                                         nlohmann::json &config);
    static int32_t SetPrivateAppSandboxProperty_(const ClientSocket::AppProperty *appProperty,
                                          nlohmann::json &config);
    static int32_t SetRenderSandboxProperty(const ClientSocket::AppProperty *appProperty,
                                            std::string &sandboxPackagePath);

private:
    static nlohmann::json appNamespaceConfig_;
    static nlohmann::json appSandboxConfig_;
    static nlohmann::json productSandboxConfig_;
};
} // namespace AppSpawn
} // namespace OHOS
#endif  // SANDBOX_UTILS_H
