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

#ifndef SANDBOX_COMMON_H
#define SANDBOX_COMMON_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include "sandbox_def.h"
#include "appspawn_msg.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "sandbox_shared_mount.h"
#include "json_utils.h"

namespace OHOS {
namespace AppSpawn {

// 挂载选项
typedef struct SandboxMountConfig {
    unsigned long mountFlags;
    std::string optionsPoint;
    std::string fsType;
    std::string sandboxPath;
    std::vector<std::string> decPaths;
} SandboxMountConfig;

typedef struct MountPointProcessParams {
    const AppSpawningCtx *appProperty;  // 引用属性
    bool checkFlag;                     // 检查标志
    std::string section;                // 分区名称
    std::string sandboxRoot;            // 沙箱根路径
    std::string bundleName;             // 包名
} MountPointProcessParams;

using ArrayItemProcessor = std::function<int32_t(cJSON*)>;

class SandboxCommon {
public:
    // 加载配置文件
    static int LoadAppSandboxConfigCJson(AppSpawnMgr *content);
    static void FreeAppSandboxConfigCJson(AppSpawnMgr *content);
    static void StoreJsonConfig(cJSON *appSandboxConfig, SandboxCommonDef::SandboxConfigType type);
    static std::vector<cJSON *> &GetCJsonConfig(SandboxCommonDef::SandboxConfigType type);

    static int32_t HandleArrayForeach(cJSON *arrayJson, ArrayItemProcessor processor);

    // 获取应用信息
    static std::string GetExtraInfoByType(const AppSpawningCtx *appProperty, const std::string &type);
    static std::string GetSandboxRootPath(const AppSpawningCtx *appProperty, cJSON *config); // GetSbxPathByConfig

    // 文件操作
    static int CreateDirRecursive(const std::string &path, mode_t mode); // MakeDirRecursive
    static void CreateDirRecursiveWithClock(const std::string &path, mode_t mode); // MakeDirRecursiveWithClock
    static void SetSandboxPathChmod(cJSON *jsonConfig, std::string &sandboxRoot); // DoSandboxChmod

    // 获取挂载配置参数信息
    static uint32_t ConvertFlagStr(const std::string &flagStr);
    static unsigned long GetMountFlags(cJSON *config); // GetSandboxMountFlags
    static bool IsCreateSandboxPathEnabled(cJSON *json, std::string srcPath); // GetCreateSandboxPath
    static bool IsTotalSandboxEnabled(const AppSpawningCtx *appProperty); // CheckTotalSandboxSwitchStatus
    static bool IsAppSandboxEnabled(const AppSpawningCtx *appProperty); // CheckAppSandboxSwitchStatus
    static void GetSandboxMountConfig(const AppSpawningCtx *appProperty, const std::string &section,
                                      cJSON *mntPoint, SandboxMountConfig &mountConfig);

    // 校验操作
    static bool HasPrivateInBundleName(const std::string &bundleName); // CheckBundleNameForPrivate
    static bool IsMountSuccessful(cJSON *mntPoint); // GetCheckStatus
    static int CheckBundleName(const std::string &bundleName);
    static bool IsValidMountConfig(cJSON *mntPoint, const AppSpawningCtx *appProperty,
                                   bool checkFlag); // CheckMountConfig
    static bool IsPrivateSharedStatus(const std::string &bundleName,
                                      AppSpawningCtx *appProperty); // GetSandboxPrivateSharedStatus
    static int32_t CheckAppFullMountEnable();

    // 路径处理
    static std::vector<std::string> SplitString(std::string &str, const std::string &delimiter); // split
    static std::string ReplaceAllVariables(std::string str, const std::string& from,
                                           const std::string& to); // replace_all
    static std::string ConvertToRealPath(const AppSpawningCtx *appProperty, std::string path);
    static std::string ConvertToRealPathWithPermission(const AppSpawningCtx *appProperty, std::string path);

    // 挂载操作
    static int32_t DoAppSandboxMountOnce(const AppSpawningCtx *appProperty, const SharedMountArgs *arg);

private:
    // 加载配置文件
    static uint32_t GetSandboxNsFlags(bool isNweb);
    static bool AppSandboxPidNsIsSupport(void);
    static void StoreCJsonConfig(cJSON *root, SandboxCommonDef::SandboxConfigType type);

    // 文件操作
    static bool VerifyDirRecursive(const std::string &path); // CheckDirRecursive
    static void CreateFileIfNotExist(const char *file); // CheckAndCreatFile

    // 获取挂载配置参数信息
    static bool GetSwitchStatus(cJSON *config); // GetSbxSwitchStatusByConfig
    static unsigned long GetMountFlagsFromConfig(const std::vector<std::string> &vec);
    static bool IsDacOverrideEnabled(cJSON *config); // GetSandboxDacOverrideEnable
    static std::string GetFsType(cJSON *config); // GetSandboxFsType
    static std::string GetOptions(const AppSpawningCtx *appProperty, cJSON *config); // GetSandboxOptions
    static std::vector<std::string> GetDecPath(const AppSpawningCtx *appProperty, cJSON *config); // GetSandboxDecPath

    // 校验操作
    static bool IsNeededCheckPathStatus(const AppSpawningCtx *appProperty, const char *path);
    static void CheckMountStatus(const std::string &path);

    // 路径处理
    static std::string ReplaceVariablePackageName(const AppSpawningCtx *appProperty, const std::string &path);
    static void MakeAtomicServiceDir(const AppSpawningCtx *appProperty, std::string path,
                                     std::string variablePackageName);
    static std::string ReplaceHostUserId(const AppSpawningCtx *appProperty, const std::string &path);
    static std::string ReplaceClonePackageName(const AppSpawningCtx *appProperty, const std::string &path);
    static const std::string &GetArkWebPackageName(void);

private:
    static int32_t deviceTypeEnable_;
    static std::map<SandboxCommonDef::SandboxConfigType, std::vector<cJSON *>> appSandboxCJsonConfig_; // sandboxManager
    typedef enum {
        SANDBOX_PACKAGENAME_DEFAULT = 0,
        SANDBOX_PACKAGENAME_CLONE,
        SANDBOX_PACKAGENAME_EXTENSION,
        SANDBOX_PACKAGENAME_CLONE_AND_EXTENSION,
        SANDBOX_PACKAGENAME_ATOMIC_SERVICE,
    } SandboxVarPackageNameType;
};

} // namespace AppSpawn
} // namespace OHOS

#endif // SANDBOX_COMMON_H