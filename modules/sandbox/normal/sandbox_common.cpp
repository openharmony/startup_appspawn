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

#include "sandbox_common.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <fcntl.h>
#include <sstream>
#include <cerrno>
#include <vector>
#include <regex>

#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "sandbox_def.h"
#include "securec.h"
#include "parameters.h"
#include "init_param.h"
#include "init_utils.h"
#include "parameter.h"
#include "config_policy_utils.h"

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif // WITH_SELINUX

namespace OHOS {
namespace AppSpawn {

int32_t SandboxCommon::deviceTypeEnable_ = -1;
int32_t SandboxCommon::mountFailedCount_ = 0;

std::map<SandboxCommonDef::SandboxConfigType, std::vector<cJSON *>> SandboxCommon::appSandboxCJsonConfig_ = {};

// 加载配置文件
uint32_t SandboxCommon::GetSandboxNsFlags(bool isNweb)
{
    uint32_t nsFlags = 0;
    if (!IsTotalSandboxEnabled(nullptr)) {
        return nsFlags;
    }

    const std::map<std::string, uint32_t> NamespaceFlagsMap = { {"pid", CLONE_NEWPID},
                                                                {"net", CLONE_NEWNET} };
    const char *prefixStr =
        isNweb ? SandboxCommonDef::g_privatePrefix : SandboxCommonDef::g_commonPrefix;
    const char *baseStr = isNweb ? SandboxCommonDef::g_ohosRender.c_str() : SandboxCommonDef::g_appBase;

    auto processor = [&baseStr, &NamespaceFlagsMap, &nsFlags](cJSON *item) {
        cJSON *internal = cJSON_GetObjectItemCaseSensitive(item, baseStr);
        if (!internal || !cJSON_IsArray(internal)) {
            return 0;
        }

        // 获取数组中第一个元素
        cJSON *firstElem = cJSON_GetArrayItem(internal, 0);
        if (!firstElem) {
            return 0;
        }

        // 获取 "sandbox-ns-flags" 数组
        cJSON *nsFlagsJson = cJSON_GetObjectItemCaseSensitive(firstElem, SandboxCommonDef::g_sandBoxNsFlags);
        if (!nsFlagsJson || !cJSON_IsArray(nsFlagsJson)) {
            return 0;
        }
        cJSON *nsIte = nullptr;
        cJSON_ArrayForEach(nsIte, nsFlagsJson) {
            const char *sandboxNs = cJSON_GetStringValue(nsIte);
            if (sandboxNs == nullptr) {
                continue;
            }
            std::string sandboxNsStr = sandboxNs;
            if (!NamespaceFlagsMap.count(sandboxNsStr)) {
                continue;
            }
            nsFlags |= NamespaceFlagsMap.at(sandboxNsStr);
        }
        return 0;
    };

    for (auto& config : GetCJsonConfig(SandboxCommonDef::SANDBOX_APP_JSON_CONFIG)) {
        // 获取 "individual" 数组
        cJSON *individual = cJSON_GetObjectItemCaseSensitive(config, prefixStr);
        if (!individual || !cJSON_IsArray(individual)) {
            return nsFlags;
        }
        (void)HandleArrayForeach(individual, processor);
    }

    if (!nsFlags) {
        APPSPAWN_LOGE("config is not found %{public}s ns config", isNweb ? "Nweb" : "App");
    }
    return nsFlags;
}

bool SandboxCommon::AppSandboxPidNsIsSupport(void)
{
    char buffer[10] = {0};
    uint32_t buffSize = sizeof(buffer);

    if (SystemGetParameter("const.sandbox.pidns.support", buffer, &buffSize) != 0) {
        return true;
    }
    if (!strcmp(buffer, "false")) {
        return false;
    }
    return true;
}

void SandboxCommon::StoreCJsonConfig(cJSON *root, SandboxCommonDef::SandboxConfigType type)
{
    appSandboxCJsonConfig_[type].push_back(root);
}

int32_t SandboxCommon::HandleArrayForeach(cJSON *arrayJson, ArrayItemProcessor processor)
{
    if (!arrayJson || !cJSON_IsArray(arrayJson) || !processor) {
        return APPSPAWN_ERROR_UTILS_DECODE_JSON_FAIL;
    }

    int ret = 0;
    cJSON *item;
    cJSON_ArrayForEach(item, arrayJson) {
        ret = processor(item);
        if (ret != 0) {
            return ret;
        }
    }
    return ret;
}

int SandboxCommon::LoadAppSandboxConfigCJson(AppSpawnMgr *content)
{
    // load sandbox config
    cJSON *sandboxCJsonRoot;
    CfgFiles *files = GetCfgFiles("etc/sandbox");
    for (int i = 0; (files != nullptr) && (i < MAX_CFG_POLICY_DIRS_CNT); ++i) {
        if (files->paths[i] == nullptr) {
            continue;
        }
        std::string path = files->paths[i];
        std::string appPath = path + SandboxCommonDef::APP_JSON_CONFIG;
        APPSPAWN_LOGI("LoadAppSandboxConfig %{public}s", appPath.c_str());
        sandboxCJsonRoot = GetJsonObjFromFile(appPath.c_str());
        APPSPAWN_CHECK((sandboxCJsonRoot != nullptr && cJSON_IsObject(sandboxCJsonRoot)), continue,
                       "Failed to load app data sandbox config %{public}s", appPath.c_str());
        StoreCJsonConfig(sandboxCJsonRoot, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

        std::string isolatedPath = path + SandboxCommonDef::APP_ISOLATED_JSON_CONFIG;
        APPSPAWN_LOGI("LoadAppSandboxConfig %{public}s", isolatedPath.c_str());
        sandboxCJsonRoot = GetJsonObjFromFile(isolatedPath.c_str());
        APPSPAWN_CHECK_LOGW((sandboxCJsonRoot != nullptr && cJSON_IsObject(sandboxCJsonRoot)), continue,
                       "Failed to load app data sandbox config %{public}s", isolatedPath.c_str());
        StoreCJsonConfig(sandboxCJsonRoot, SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG);
    }
    FreeCfgFiles(files);

    bool isNweb = IsNWebSpawnMode(content);
    if (!isNweb && !AppSandboxPidNsIsSupport()) {
        return 0;
    }
    content->content.sandboxNsFlags = GetSandboxNsFlags(isNweb);
    return 0;
}

int SandboxCommon::FreeAppSandboxConfigCJson(AppSpawnMgr *content)
{
    UNUSED(content);
    std::vector<cJSON *> &normalJsonVec = GetCJsonConfig(SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);
    for (auto& normal : normalJsonVec) {
        if (normal == nullptr) {
            continue;
        }
        cJSON_Delete(normal);
        normal = nullptr;
    }
    normalJsonVec.clear();

    std::vector<cJSON *> &isolatedJsonVec = GetCJsonConfig(SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG);
    for (auto& isolated : isolatedJsonVec) {
        if (isolated == nullptr) {
            continue;
        }
        cJSON_Delete(isolated);
        isolated = nullptr;
    }
    isolatedJsonVec.clear();
    return 0;
}

std::vector<cJSON *> &SandboxCommon::GetCJsonConfig(SandboxCommonDef::SandboxConfigType type)
{
    return appSandboxCJsonConfig_[type];
}

// 获取应用信息
std::string SandboxCommon::GetExtraInfoByType(const AppSpawningCtx *appProperty, const std::string &type)
{
    uint32_t len = 0;
    char *info = reinterpret_cast<char *>(GetAppPropertyExt(appProperty, type.c_str(), &len));
    if (info == nullptr) {
        return "";
    }
    return std::string(info, len);
}

std::string SandboxCommon::GetSandboxRootPath(const AppSpawningCtx *appProperty, cJSON *config)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return "";
    }

    std::string sandboxRoot = "";
    std::string isolatedFlagText = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ? "isolated/" : "";
    AppSpawnMsgBundleInfo *bundleInfo =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    if (bundleInfo == nullptr) {
        return "";
    }
    std::string tmpBundlePath = bundleInfo->bundleName;
    std::ostringstream variablePackageName;
    if (CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_CLONE_ENABLE)) {
        variablePackageName << "+clone-" << bundleInfo->bundleIndex << "+" << bundleInfo->bundleName;
        tmpBundlePath = variablePackageName.str();
    }
    const std::string variableSandboxRoot = SandboxCommonDef::g_sandBoxRootDir +
        std::to_string(dacInfo->uid / UID_BASE) + "/" + isolatedFlagText.c_str() + tmpBundlePath;

    APPSPAWN_CHECK_ONLY_EXPER(config != nullptr, sandboxRoot = variableSandboxRoot;
        return sandboxRoot);
    const char *sandboxRootChr = GetStringFromJsonObj(config, SandboxCommonDef::g_sandboxRootPrefix);
    if (sandboxRootChr != nullptr) {
        sandboxRoot = sandboxRootChr;
        if (sandboxRoot == SandboxCommonDef::g_originSandboxPath ||
            sandboxRoot == SandboxCommonDef::g_sandboxRootPathTemplate) {
            sandboxRoot = variableSandboxRoot;
        } else {
            sandboxRoot = ConvertToRealPath(appProperty, sandboxRoot);
            APPSPAWN_LOGV("set sandbox-root name is %{public}s", sandboxRoot.c_str());
        }
    } else {
        sandboxRoot = variableSandboxRoot;
        APPSPAWN_LOGV("set sandbox-root to default rootapp name is %{public}s", GetBundleName(appProperty));
    }

    return sandboxRoot;
}

int SandboxCommon::CreateDirRecursive(const std::string &path, mode_t mode)
{
    return MakeDirRec(path.c_str(), mode, 1);
}

void SandboxCommon::CreateDirRecursiveWithClock(const std::string &path, mode_t mode)
{
    size_t size = path.size();
    if (size == 0) {
        return;
    }
#ifdef APPSPAWN_HISYSEVENT
    struct timespec startClock = {0};
    clock_gettime(CLOCK_MONOTONIC, &startClock);
#endif
    size_t index = 0;
    do {
        size_t pathIndex = path.find_first_of('/', index);
        index = pathIndex == std::string::npos ? size : pathIndex + 1;
        std::string dir = path.substr(0, index);
#ifndef APPSPAWN_TEST
        APPSPAWN_CHECK(!(access(dir.c_str(), F_OK) < 0 && mkdir(dir.c_str(), mode) < 0),
                       return, "errno is %{public}d, mkdir %{public}s failed", errno, dir.c_str());
#endif
    } while (index < size);

#ifdef APPSPAWN_HISYSEVENT
    struct timespec endClock = {0};
    clock_gettime(CLOCK_MONOTONIC, &endClock);
    uint64_t diff = DiffTime(&startClock, &endClock);

    APPSPAWN_CHECK_ONLY_EXPER(diff < FUNC_REPORT_DURATION,
                              ReportAbnormalDuration("MakeDirRecursive", diff));
#endif
}

bool SandboxCommon::VerifyDirRecursive(const std::string &path)
{
    size_t size = path.size();
    if (size == 0) {
        return false;
    }
    size_t index = 0;
    do {
        size_t pathIndex = path.find_first_of('/', index);
        index = pathIndex == std::string::npos ? size : pathIndex + 1;
        std::string dir = path.substr(0, index);
#ifndef APPSPAWN_TEST
        APPSPAWN_CHECK_DUMPW(access(dir.c_str(), F_OK) == 0,
            return false, "check dir %{public}s failed,strerror:%{public}s", dir.c_str(), strerror(errno));
#endif
    } while (index < size);
    return true;
}

void SandboxCommon::CreateFileIfNotExist(const char *file)
{
    if (access(file, F_OK) == 0) {
        APPSPAWN_LOGV("file %{public}s already exist", file);
        return;
    }
    std::string path = file;
    auto pos = path.find_last_of('/');
    APPSPAWN_CHECK(pos != std::string::npos, return, "file %{public}s error", file);
    std::string dir = path.substr(0, pos);
    (void)CreateDirRecursive(dir, SandboxCommonDef::FILE_MODE);
    int fd = open(file, O_CREAT, SandboxCommonDef::FILE_MODE);
    if (fd < 0) {
        APPSPAWN_LOGW("failed create %{public}s, err=%{public}d", file, errno);
    } else {
        close(fd);
    }
    return;
}

void SandboxCommon::SetSandboxPathChmod(cJSON *jsonConfig, std::string &sandboxRoot)
{
    const std::map<std::string, mode_t> modeMap = {{"S_IRUSR", S_IRUSR}, {"S_IWUSR", S_IWUSR}, {"S_IXUSR", S_IXUSR},
                                                   {"S_IRGRP", S_IRGRP}, {"S_IWGRP", S_IWGRP}, {"S_IXGRP", S_IXGRP},
                                                   {"S_IROTH", S_IROTH}, {"S_IWOTH", S_IWOTH}, {"S_IXOTH", S_IXOTH},
                                                   {"S_IRWXU", S_IRWXU}, {"S_IRWXG", S_IRWXG}, {"S_IRWXO", S_IRWXO}};
    const char *fileMode = GetStringFromJsonObj(jsonConfig, SandboxCommonDef::g_destMode);
    if (fileMode == nullptr) {
        return;
    }

    mode_t mode = 0;
    std::string fileModeStr = fileMode;
    std::vector<std::string> modeVec = SplitString(fileModeStr, "|");
    for (unsigned int i = 0; i < modeVec.size(); i++) {
        if (modeMap.count(modeVec[i])) {
            mode |= modeMap.at(modeVec[i]);
        }
    }

    chmod(sandboxRoot.c_str(), mode);
}

// 获取挂载配置参数信息
unsigned long SandboxCommon::GetMountFlagsFromConfig(const std::vector<std::string> &vec)
{
    const std::map<std::string, mode_t> MountFlagsMap = { {"rec", MS_REC}, {"MS_REC", MS_REC},
                                                          {"bind", MS_BIND}, {"MS_BIND", MS_BIND},
                                                          {"move", MS_MOVE}, {"MS_MOVE", MS_MOVE},
                                                          {"slave", MS_SLAVE}, {"MS_SLAVE", MS_SLAVE},
                                                          {"rdonly", MS_RDONLY}, {"MS_RDONLY", MS_RDONLY},
                                                          {"shared", MS_SHARED}, {"MS_SHARED", MS_SHARED},
                                                          {"unbindable", MS_UNBINDABLE},
                                                          {"MS_UNBINDABLE", MS_UNBINDABLE},
                                                          {"remount", MS_REMOUNT}, {"MS_REMOUNT", MS_REMOUNT},
                                                          {"nosuid", MS_NOSUID}, {"MS_NOSUID", MS_NOSUID},
                                                          {"nodev", MS_NODEV}, {"MS_NODEV", MS_NODEV},
                                                          {"noexec", MS_NOEXEC}, {"MS_NOEXEC", MS_NOEXEC},
                                                          {"noatime", MS_NOATIME}, {"MS_NOATIME", MS_NOATIME},
                                                          {"lazytime", MS_LAZYTIME}, {"MS_LAZYTIME", MS_LAZYTIME}};

    unsigned long mountFlags = 0;
    for (unsigned int i = 0; i < vec.size(); i++) {
        if (MountFlagsMap.count(vec[i])) {
            mountFlags |= MountFlagsMap.at(vec[i]);
        }
    }
    return mountFlags;
}

bool SandboxCommon::IsDacOverrideEnabled(cJSON *config) // GetSandboxDacOverrideEnable
{
    return GetBoolValueFromJsonObj(config, SandboxCommonDef::g_dacOverrideSensitive, false);
}

bool SandboxCommon::GetSwitchStatus(cJSON *config) // GetSbxSwitchStatusByConfig
{
    // if not find sandbox-switch node, default switch status is true
    return GetBoolValueFromJsonObj(config, SandboxCommonDef::g_sandBoxSwitchPrefix, true);
}

uint32_t SandboxCommon::ConvertFlagStr(const std::string &flagStr)
{
    const std::map<std::string, int> flagsMap = {{"START_FLAGS_BACKUP", APP_FLAGS_BACKUP_EXTENSION},
                                                 {"DLP_MANAGER_FULL_CONTROL", APP_FLAGS_DLP_MANAGER_FULL_CONTROL},
                                                 {"DLP_MANAGER_READ_ONLY", APP_FLAGS_DLP_MANAGER_READ_ONLY},
                                                 {"DEVELOPER_MODE", APP_FLAGS_DEVELOPER_MODE},
                                                 {"PREINSTALLED_HAP", APP_FLAGS_PRE_INSTALLED_HAP},
                                                 {"CUSTOM_SANDBOX_HAP", APP_FLAGS_CUSTOM_SANDBOX},
                                                 {"FILE_CROSS_APP", APP_FLAGS_FILE_CROSS_APP},
                                                 {"FILE_ACCESS_COMMON_DIR", APP_FLAGS_FILE_ACCESS_COMMON_DIR},
                                                 {"CLOUD_FILE_SYNC_ENABLED", APP_FLAGS_CLOUD_FILE_SYNC_ENABLED}};

    if (flagsMap.count(flagStr)) {
        return flagsMap.at(flagStr);
    }
    return 0;
}

unsigned long SandboxCommon::GetMountFlags(cJSON *config) // GetSandboxMountFlags
{
    unsigned long mountFlags = SandboxCommonDef::BASIC_MOUNT_FLAGS;
    std::vector<std::string> vec;
    cJSON *customizedFlags = IsDacOverrideEnabled(config) ?
        cJSON_GetObjectItemCaseSensitive(config, SandboxCommonDef::g_sandBoxFlagsCustomized) :
        cJSON_GetObjectItemCaseSensitive(config, SandboxCommonDef::g_sandBoxFlags);

    if (!customizedFlags) {
        customizedFlags = cJSON_GetObjectItemCaseSensitive(config, SandboxCommonDef::g_sandBoxFlags);
    }
    if (customizedFlags == nullptr || !cJSON_IsArray(customizedFlags)) {
        return mountFlags;
    }

    auto processor = [&vec](cJSON *item) {
        const char *strItem = cJSON_GetStringValue(item);
        if (strItem == nullptr) {
            return -1;
        }
        vec.emplace_back(strItem);
        return 0;
    };

    if (HandleArrayForeach(customizedFlags, processor) != 0) {
        return mountFlags;
    }
    return GetMountFlagsFromConfig(vec);
}

std::string SandboxCommon::GetFsType(cJSON *config) // GetSandboxFsType
{
    std::string fsType = "";
    const char *fsTypeChr = GetStringFromJsonObj(config, SandboxCommonDef::g_fsType);
    if (fsTypeChr == nullptr) {
        return fsType;
    }
    fsType = fsTypeChr;
    return fsType;
}

std::string SandboxCommon::GetOptions(const AppSpawningCtx *appProperty, cJSON *config) // GetSandboxOptions
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return "";
    }

    std::string options = "";
    const char *optionsChr = GetStringFromJsonObj(config, SandboxCommonDef::g_sandBoxOptions);
    if (optionsChr == nullptr) {
        return options;
    }
    options = optionsChr;
    options += ",user_id=" + std::to_string(dacInfo->uid / UID_BASE);
    return options;
}

std::vector<std::string> SandboxCommon::GetDecPath(const AppSpawningCtx *appProperty, cJSON *config)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return {};
    }

    std::vector<std::string> decPaths = {};
    cJSON *decPathJson = cJSON_GetObjectItemCaseSensitive(config, SandboxCommonDef::g_sandBoxDecPath);
    if (decPathJson == nullptr || !cJSON_IsArray(decPathJson)) {
        return {};
    }

    auto processor = [&appProperty, &decPaths](cJSON *item) {
        const char *strItem = cJSON_GetStringValue(item);
        if (strItem == nullptr) {
            return -1;
        }
        std::string decPath = ConvertToRealPathWithPermission(appProperty, strItem);
        decPaths.emplace_back(std::move(decPath));
        return 0;
    };
    if (HandleArrayForeach(decPathJson, processor) != 0) {
        return {};
    }
    return decPaths;
}

bool SandboxCommon::IsCreateSandboxPathEnabled(cJSON *json, std::string srcPath) // GetCreateSandboxPath
{
    bool isRet = GetBoolValueFromJsonObj(json, SandboxCommonDef::CREATE_SANDBOX_PATH, false);
    if (isRet && access(srcPath.c_str(), F_OK) != 0) {
        return false;
    }
    return true;
}

bool SandboxCommon::IsTotalSandboxEnabled(const AppSpawningCtx *appProperty) // CheckTotalSandboxSwitchStatus
{
    SandboxCommonDef::SandboxConfigType type;
    if (appProperty == nullptr) {
        type = SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;
    } else {
        type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
            SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;
    }

    for (auto& wholeConfig : GetCJsonConfig(type)) {
        // 获取 "common" 数组
        cJSON *common = cJSON_GetObjectItemCaseSensitive(wholeConfig, SandboxCommonDef::g_commonPrefix);
        if (!common || !cJSON_IsArray(common)) {
            continue;
        }
        // 获取第一个字段
        cJSON *firstCommon = cJSON_GetArrayItem(common, 0);
        if (!firstCommon) {
            continue;
        }
        return GetBoolValueFromJsonObj(firstCommon, SandboxCommonDef::g_topSandBoxSwitchPrefix, true);
    }
    // default sandbox switch is on
    return true;
}

bool SandboxCommon::IsAppSandboxEnabled(const AppSpawningCtx *appProperty) // CheckAppSandboxSwitchStatus
{
    SandboxCommonDef::SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;

    bool ret = true;
    for (auto& wholeConfig : GetCJsonConfig(type)) {
        // 获取 "individual" 数组
        cJSON *individual = cJSON_GetObjectItemCaseSensitive(wholeConfig, SandboxCommonDef::g_privatePrefix);
        if (!individual || !cJSON_IsArray(individual)) {
            continue;
        }
        cJSON *bundleNameInfo = cJSON_GetObjectItemCaseSensitive(individual, GetBundleName(appProperty));
        if (!bundleNameInfo || !cJSON_IsArray(bundleNameInfo)) {
            continue;
        }
        // 获取第一个字段
        cJSON *firstCommon = cJSON_GetArrayItem(bundleNameInfo, 0);
        if (!firstCommon) {
            continue;
        }
        ret = GetSwitchStatus(firstCommon);
        if (ret) {
            break;
        }
    }
    // default sandbox switch is on
    return ret;
}

void SandboxCommon::GetSandboxMountConfig(const AppSpawningCtx *appProperty, const std::string &section,
                                          cJSON *mntPoint, SandboxMountConfig &mountConfig)
{
    if (section.compare(SandboxCommonDef::g_permissionPrefix) == 0 ||
        section.compare(SandboxCommonDef::g_flagsPoint) == 0 ||
        section.compare(SandboxCommonDef::g_debughap) == 0) {
        mountConfig.optionsPoint = GetOptions(appProperty, mntPoint);
        mountConfig.fsType = GetFsType(mntPoint);
        mountConfig.decPaths = GetDecPath(appProperty, mntPoint);
    } else {
        mountConfig.fsType = GetFsType(mntPoint);
        mountConfig.optionsPoint = "";
#ifdef APPSPAWN_SUPPORT_NOSHAREFS
        mountConfig.decPaths = GetDecPath(appProperty, mntPoint);
#else
        mountConfig.decPaths = {};
#endif
    }
    return;
}

// 校验操作
bool SandboxCommon::IsNeededCheckPathStatus(const AppSpawningCtx *appProperty, const char *path)
{
    if (strstr(path, "data/app/el1/") || strstr(path, "data/app/el2/")) {
        return true;
    }
    if ((strstr(path, "data/app/el3/") || strstr(path, "data/app/el4/") || strstr(path, "data/app/el5/")) &&
        CheckAppMsgFlagsSet(appProperty, APP_FLAGS_UNLOCKED_STATUS)) {
        return true;
    }
    return false;
}

void SandboxCommon::CheckMountStatus(const std::string &path)
{
    std::ifstream file("/proc/self/mountinfo");
    if (!file.is_open()) {
        APPSPAWN_LOGE("Failed to open /proc/self/mountinfo errno %{public}d", errno);
        return;
    }

    bool flag = false;
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(path) != std::string::npos) {
            flag = true;
            APPSPAWN_LOGI("Current mountinfo %{public}s", line.c_str());
        }
    }
    file.close();
    APPSPAWN_CHECK_ONLY_LOG(flag, "Mountinfo not contains %{public}s", path.c_str());
}

bool SandboxCommon::HasPrivateInBundleName(const std::string &bundleName) // CheckBundleNameForPrivate
{
    if (bundleName.find(SandboxCommonDef::g_internal) != std::string::npos) {
        return false;
    }
    return true;
}

bool SandboxCommon::IsMountSuccessful(cJSON *mntPoint) // GetCheckStatus
{
    // default false
    return GetBoolValueFromJsonObj(mntPoint, SandboxCommonDef::g_actionStatuc, false);
}

int SandboxCommon::CheckBundleName(const std::string &bundleName)
{
    if (bundleName.empty() || bundleName.size() > APP_LEN_BUNDLE_NAME) {
        return -1;
    }
    if (bundleName.find('\\') != std::string::npos || bundleName.find('/') != std::string::npos) {
        return -1;
    }
    return 0;
}

int32_t SandboxCommon::CheckAppFullMountEnable()
{
    if (deviceTypeEnable_ != -1) {
        return deviceTypeEnable_;
    }

    char value[] = "false";
    int32_t ret = GetParameter("const.filemanager.full_mount.enable", "false", value, sizeof(value));
    if (ret > 0 && (strcmp(value, "true")) == 0) {
        deviceTypeEnable_ = SandboxCommonDef::FILE_CROSS_APP_STATUS;
    } else if (ret > 0 && (strcmp(value, "false")) == 0) {
        deviceTypeEnable_ = SandboxCommonDef::FILE_ACCESS_COMMON_DIR_STATUS;
    } else {
        deviceTypeEnable_ = -1;
    }

    return deviceTypeEnable_;
}

bool SandboxCommon::IsPrivateSharedStatus(const std::string &bundleName, AppSpawningCtx *appProperty)
{
    bool result = false;
    SandboxCommonDef::SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;

    for (auto& config : GetCJsonConfig(type)) {
        // 获取 "individual" 数组
        cJSON *individual = cJSON_GetObjectItemCaseSensitive(config, SandboxCommonDef::g_privatePrefix);
        if (!individual || !cJSON_IsArray(individual)) {
            return result;
        }
        cJSON *bundleNameInfo = cJSON_GetObjectItemCaseSensitive(individual, bundleName.c_str());
        if (!bundleNameInfo || !cJSON_IsArray(bundleNameInfo)) {
            return result;
        }
        result = GetBoolValueFromJsonObj(bundleNameInfo, SandboxCommonDef::g_sandBoxShared, false);
    }
    return result;
}

bool SandboxCommon::IsValidMountConfig(cJSON *mntPoint, const AppSpawningCtx *appProperty, bool checkFlag)
{
    const char *srcPath = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_srcPath);
    const char *sandboxPath = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_sandBoxPath);
    const char *srcParamPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_paramPath);
    cJSON *customizedFlags = cJSON_GetObjectItemCaseSensitive(mntPoint, SandboxCommonDef::g_sandBoxFlagsCustomized);
    cJSON *flags = cJSON_GetObjectItemCaseSensitive(mntPoint, SandboxCommonDef::g_sandBoxFlags);
    if ((srcPath == nullptr && srcParamPathChr == nullptr) || sandboxPath == nullptr ||
        (customizedFlags == nullptr && flags == nullptr)) {
        APPSPAWN_LOGE("read mount config failed, app name is %{public}s", GetBundleName(appProperty));
        return false;
    }

    AppSpawnMsgDomainInfo *info =
        reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(appProperty, TLV_DOMAIN_INFO));
    APPSPAWN_CHECK(info != nullptr, return false, "Filed to get domain info %{public}s", GetBundleName(appProperty));
    const char *appAplName = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_appAplName);
    if (appAplName != nullptr) {
        if (!strcmp(appAplName, info->apl)) {
            return false;
        }
    }

    const std::string configSrcPath = srcPath == nullptr ? "" : srcPath;
    // special handle wps and don't use /data/app/xxx/<Package> config
    if (checkFlag && (configSrcPath.find("/data/app") != std::string::npos &&
        (configSrcPath.find("/base") != std::string::npos ||
         configSrcPath.find("/database") != std::string::npos
        ) && configSrcPath.find(SandboxCommonDef::g_packageName) != std::string::npos)) {
        return false;
    }

    return true;
}

// 路径处理
std::string SandboxCommon::ReplaceAllVariables(std::string str, const std::string& from, const std::string& to)
{
    while (true) {
        std::string::size_type pos(0);
        if ((pos = str.find(from)) != std::string::npos) {
            str.replace(pos, from.length(), to);
        } else {
            break;
        }
    }
    return str;
}

std::vector<std::string> SandboxCommon::SplitString(std::string &str, const std::string &delimiter)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    str += delimiter;
    size_t size = str.size();
    for (unsigned int i = 0; i < size; i++) {
        pos = str.find(delimiter, i);
        if (pos < size) {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + delimiter.size() - 1;
        }
    }

    return result;
}

void SandboxCommon::MakeAtomicServiceDir(const AppSpawningCtx *appProperty, std::string path,
                                         std::string variablePackageName)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    APPSPAWN_CHECK(dacInfo != nullptr, return, "No dac info in msg app property");
    if (path.find("/mnt/share") != std::string::npos) {
        path = "/data/service/el2/" + std::to_string(dacInfo->uid / UID_BASE) + "/share/" + variablePackageName;
    }
    struct stat st = {};
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        return;
    }

    int ret = mkdir(path.c_str(), S_IRWXU);
    APPSPAWN_CHECK(ret == 0, return, "mkdir %{public}s failed, errno %{public}d", path.c_str(), errno);

    if (path.find("/database") != std::string::npos || path.find("/data/service/el2") != std::string::npos) {
        ret = chmod(path.c_str(), S_IRWXU | S_IRWXG | S_ISGID);
    } else if (path.find("/log") != std::string::npos) {
        ret = chmod(path.c_str(), S_IRWXU | S_IRWXG);
    }
    APPSPAWN_CHECK(ret == 0, return, "chmod %{public}s failed, errno %{public}d", path.c_str(), errno);

#ifdef WITH_SELINUX
    AppSpawnMsgDomainInfo *msgDomainInfo =
        reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(appProperty, TLV_DOMAIN_INFO));
    APPSPAWN_CHECK(msgDomainInfo != nullptr, return, "No domain info for %{public}s", GetProcessName(appProperty));
    HapContext hapContext;
    HapFileInfo hapFileInfo;
    hapFileInfo.pathNameOrig.push_back(path);
    hapFileInfo.apl = msgDomainInfo->apl;
    hapFileInfo.packageName = GetBundleName(appProperty);
    hapFileInfo.hapFlags = msgDomainInfo->hapFlags;
    if (CheckAppMsgFlagsSet(appProperty, APP_FLAGS_DEBUGGABLE)) {
        hapFileInfo.hapFlags |= SELINUX_HAP_DEBUGGABLE;
    }
    if ((path.find("/base") != std::string::npos) || (path.find("/database") != std::string::npos)) {
        ret = hapContext.HapFileRestorecon(hapFileInfo);
        APPSPAWN_CHECK(ret == 0, return, "set dir %{public}s selinuxLabel failed, apl %{public}s, ret %{public}d",
            path.c_str(), hapFileInfo.apl.c_str(), ret);
    }
#endif
    if (path.find("/base") != std::string::npos || path.find("/data/service/el2") != std::string::npos) {
        ret = chown(path.c_str(), dacInfo->uid, dacInfo->gid);
    } else if (path.find("/database") != std::string::npos) {
        ret = chown(path.c_str(), dacInfo->uid, DecodeGid("ddms"));
    } else if (path.find("/log") != std::string::npos) {
        ret = chown(path.c_str(), dacInfo->uid, DecodeGid("log"));
    }
    APPSPAWN_CHECK(ret == 0, return, "chown %{public}s failed, errno %{public}d", path.c_str(), errno);
    return;
}

std::string SandboxCommon::ReplaceVariablePackageName(const AppSpawningCtx *appProperty, const std::string &path)
{
    std::string tmpSandboxPath = path;
    AppSpawnMsgBundleInfo *bundleInfo =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    APPSPAWN_CHECK(bundleInfo != nullptr, return "", "No bundle info in msg %{public}s", GetBundleName(appProperty));

    char *extension;
    uint32_t flags = CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_ATOMIC_SERVICE) ? 0x4 : 0;
    if (flags == 0) {
        flags = (CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_CLONE_ENABLE) &&
            bundleInfo->bundleIndex > 0) ? 0x1 : 0;
        flags |= CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_EXTENSION_SANDBOX) ? 0x2 : 0;
        extension = reinterpret_cast<char *>(
            GetAppSpawnMsgExtInfo(appProperty->message, MSG_EXT_NAME_APP_EXTENSION, nullptr));
    }
    std::ostringstream variablePackageName;
    switch (flags) {
        case SANDBOX_PACKAGENAME_DEFAULT:    // 0 default
            variablePackageName << bundleInfo->bundleName;
            break;
        case SANDBOX_PACKAGENAME_CLONE:    // 1 +clone-bundleIndex+packageName
            variablePackageName << "+clone-" << bundleInfo->bundleIndex << "+" << bundleInfo->bundleName;
            break;
        case SANDBOX_PACKAGENAME_EXTENSION: {  // 2 +extension-<extensionType>+packageName
            APPSPAWN_CHECK(extension != nullptr, return "", "Invalid extension data ");
            variablePackageName << "+extension-" << extension << "+" << bundleInfo->bundleName;
            break;
        }
        case SANDBOX_PACKAGENAME_CLONE_AND_EXTENSION: {  // 3 +clone-bundleIndex+extension-<extensionType>+packageName
            APPSPAWN_CHECK(extension != nullptr, return "", "Invalid extension data ");
            variablePackageName << "+clone-" << bundleInfo->bundleIndex << "+extension" << "-" <<
                extension << "+" << bundleInfo->bundleName;
            break;
        }
        case SANDBOX_PACKAGENAME_ATOMIC_SERVICE: {  // 4 +auid-<accountId>+packageName
            std::string accountId = GetExtraInfoByType(appProperty, MSG_EXT_NAME_ACCOUNT_ID);
            variablePackageName << "+auid-" << accountId << "+" << bundleInfo->bundleName;
            std::string atomicServicePath = path;
            atomicServicePath = ReplaceAllVariables(atomicServicePath, SandboxCommonDef::g_variablePackageName,
                                                    variablePackageName.str());
            MakeAtomicServiceDir(appProperty, atomicServicePath, variablePackageName.str());
            break;
        }
        default:
            variablePackageName << bundleInfo->bundleName;
            break;
    }
    tmpSandboxPath = ReplaceAllVariables(tmpSandboxPath, SandboxCommonDef::g_variablePackageName,
                                         variablePackageName.str());
    return tmpSandboxPath;
}

std::string SandboxCommon::ReplaceHostUserId(const AppSpawningCtx *appProperty, const std::string &path)
{
    std::string tmpSandboxPath = path;
    int32_t uid = 0;
    const char *userId =
        (const char *)(GetAppSpawnMsgExtInfo(appProperty->message, MSG_EXT_NAME_PARENT_UID, nullptr));
    if (userId != nullptr) {
        uid = atoi(userId);
    }
    tmpSandboxPath = ReplaceAllVariables(tmpSandboxPath, SandboxCommonDef::g_hostUserId,
                                                        std::to_string(uid / UID_BASE));
    APPSPAWN_LOGV("tmpSandboxPath %{public}s", tmpSandboxPath.c_str());
    return tmpSandboxPath;
}

std::string SandboxCommon::ReplaceClonePackageName(const AppSpawningCtx *appProperty, const std::string &path)
{
    std::string tmpSandboxPath = path;
    AppSpawnMsgBundleInfo *bundleInfo =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    APPSPAWN_CHECK(bundleInfo != nullptr, return "", "No bundle info in msg %{public}s", GetBundleName(appProperty));

    std::string tmpBundlePath = bundleInfo->bundleName;
    std::ostringstream variablePackageName;
    if (CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_CLONE_ENABLE)) {
        variablePackageName << "+clone-" << bundleInfo->bundleIndex << "+" << bundleInfo->bundleName;
        tmpBundlePath = variablePackageName.str();
    }

    tmpSandboxPath = ReplaceAllVariables(tmpSandboxPath, SandboxCommonDef::g_clonePackageName, tmpBundlePath);
    APPSPAWN_LOGV("tmpSandboxPath %{public}s", tmpSandboxPath.c_str());
    return tmpSandboxPath;
}

const std::string& SandboxCommon::GetArkWebPackageName(void)
{
    static std::string arkWebPackageName;
    if (arkWebPackageName.empty()) {
        arkWebPackageName = system::GetParameter(SandboxCommonDef::ARK_WEB_PERSIST_PACKAGE_NAME, "");
    }
    return arkWebPackageName;
}

const std::string& SandboxCommon::GetDevModel(void)
{
    static std::string devModel;
    if (devModel.empty()) {
        devModel = system::GetParameter(SandboxCommonDef::DEVICE_MODEL_NAME_PARAM, "");
    }
    return devModel;
}

// Parsing parameters in <param> format
std::string SandboxCommon::ParseParamTemplate(const std::string &templateStr)
{
    if (templateStr.empty()) {
        return templateStr;
    }

    std::string tmpStr = templateStr;
    if (templateStr.size() > SandboxCommonDef::MIN_PARAM_SRC_PATH_LEN && templateStr.front() == '<' &&
        templateStr.back() == '>') {
        tmpStr = templateStr.substr(1, templateStr.size() - SandboxCommonDef::MIN_PARAM_SRC_PATH_LEN);
    }

    std::string paramValue = system::GetParameter(tmpStr, "");
    if (paramValue.empty()) {
        APPSPAWN_LOGE("GetParameter %{public}s failed.", tmpStr.c_str());
        paramValue = templateStr;
    }

    return paramValue;
}

// Path concatenation function (handling path separators)
std::string SandboxCommon::JoinParamPaths(const std::vector<std::string> &paths)
{
    std::string result = "";
    for (const auto &path : paths) {
        if (path.empty()) {
            continue;
        }
        // Prevent path traversal
        if (path.find("..") != std::string::npos) {
            APPSPAWN_LOGE("Param src path invalid");
            return "";
        }

        if (!result.empty() && result.back() != '/' && !path.empty() && path.front() != '/') {
            APPSPAWN_LOGV("Param src path need add slash");
            result += '/';
        }

        // Remove duplicate path separators
        std::string cleanPath = path;
        if (cleanPath.front() == '/' && !result.empty() && result.back() == '/') {
            cleanPath = cleanPath.substr(1);
        }

        result += cleanPath;
    }

    return result;
}

std::string SandboxCommon::ConvertToRealPathWithPermission(const AppSpawningCtx *appProperty, std::string path)
{
    AppSpawnMsgBundleInfo *info =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    APPSPAWN_CHECK((info != nullptr && dacInfo != nullptr), return "", "Invalid params");

    if (path.find(SandboxCommonDef::g_packageNameIndex) != std::string::npos) {
        std::string bundleNameWithIndex = info->bundleName;
        if (info->bundleIndex != 0) {
            bundleNameWithIndex = std::to_string(info->bundleIndex) + "_" + bundleNameWithIndex;
        }
        path = ReplaceAllVariables(path, SandboxCommonDef::g_packageNameIndex, bundleNameWithIndex);
    }

    if (path.find(SandboxCommonDef::g_packageName) != std::string::npos) {
        path = ReplaceAllVariables(path, SandboxCommonDef::g_packageName, info->bundleName);
    }

    if (path.find(SandboxCommonDef::g_userId) != std::string::npos) {
        path = ReplaceAllVariables(path, SandboxCommonDef::g_userId, "currentUser");
    }

    if (path.find(SandboxCommonDef::g_permissionUserId) != std::string::npos) {
        path = ReplaceAllVariables(path, SandboxCommonDef::g_permissionUserId, std::to_string(dacInfo->uid / UID_BASE));
    }
    return path;
}

std::string SandboxCommon::ConvertToRealPath(const AppSpawningCtx *appProperty, std::string path)
{
    AppSpawnMsgBundleInfo *info =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (info == nullptr || dacInfo == nullptr) {
        return "";
    }
    if (path.find(SandboxCommonDef::g_packageNameIndex) != std::string::npos) {
        std::string bundleNameWithIndex = info->bundleName;
        if (info->bundleIndex != 0) {
            bundleNameWithIndex = std::to_string(info->bundleIndex) + "_" + bundleNameWithIndex;
        }
        path = ReplaceAllVariables(path, SandboxCommonDef::g_packageNameIndex, bundleNameWithIndex);
    }

    if (path.find(SandboxCommonDef::g_packageName) != std::string::npos) {
        path = ReplaceAllVariables(path, SandboxCommonDef::g_packageName, info->bundleName);
    }

    if (path.find(SandboxCommonDef::g_userId) != std::string::npos) {
        path = ReplaceAllVariables(path, SandboxCommonDef::g_userId, std::to_string(dacInfo->uid / UID_BASE));
    }

    if (path.find(SandboxCommonDef::g_hostUserId) != std::string::npos) {
        path = ReplaceHostUserId(appProperty, path);
    }

    if (path.find(SandboxCommonDef::g_variablePackageName) != std::string::npos) {
        path = ReplaceVariablePackageName(appProperty, path);
    }

    if (path.find(SandboxCommonDef::g_clonePackageName) != std::string::npos) {
        path = ReplaceClonePackageName(appProperty, path);
    }

    if (path.find(SandboxCommonDef::g_arkWebPackageName) != std::string::npos) {
        path = ReplaceAllVariables(path, SandboxCommonDef::g_arkWebPackageName, GetArkWebPackageName());
        APPSPAWN_LOGV("arkWeb sandbox, path %{public}s, package:%{public}s",
                      path.c_str(), GetArkWebPackageName().c_str());
    }

    if (path.find(SandboxCommonDef::g_devModel) != std::string::npos) {
        path = ReplaceAllVariables(path, SandboxCommonDef::g_devModel, GetDevModel());
    }

    return path;
}

// 挂载操作
int32_t SandboxCommon::DoAppSandboxMountOnce(const AppSpawningCtx *appProperty, const SharedMountArgs *arg)
{
    if (!(arg && arg->srcPath && arg->destPath && arg->srcPath[0] != '\0' && arg->destPath[0] != '\0')) {
        return 0;
    }
    if (strstr(arg->srcPath, "system/etc/hosts") != nullptr ||
        strstr(arg->srcPath, "system/etc/profile") != nullptr ||
        strstr(arg->srcPath, "system/etc/sudoers") != nullptr) {
        CreateFileIfNotExist(arg->destPath);
    } else {
        (void)CreateDirRecursive(arg->destPath, SandboxCommonDef::FILE_MODE);
    }

    int ret = 0;
    // to mount fs and bind mount files or directory
    struct timespec mountStart = {0};
    clock_gettime(CLOCK_MONOTONIC_COARSE, &mountStart);
    APPSPAWN_LOGV("Bind mount %{public}s to %{public}s '%{public}s' '%{public}lu' '%{public}s' '%{public}u'",
        arg->srcPath, arg->destPath, arg->fsType, arg->mountFlags, arg->options, arg->mountSharedFlag);
    ret = mount(arg->srcPath, arg->destPath, arg->fsType, arg->mountFlags, arg->options);
    struct timespec mountEnd = {0};
    clock_gettime(CLOCK_MONOTONIC_COARSE, &mountEnd);
    uint64_t diff = DiffTime(&mountStart, &mountEnd);
    APPSPAWN_CHECK_ONLY_DUMPW(diff < SandboxCommonDef::MAX_MOUNT_TIME, "mount %{public}s time %{public}" PRId64 " us",
                            arg->srcPath, diff);
#ifdef APPSPAWN_HISYSEVENT
    APPSPAWN_CHECK_ONLY_EXPER(diff < FUNC_REPORT_DURATION, ReportAbnormalDuration(arg->srcPath, diff));
#endif
    if (ret != 0) {
        APPSPAWN_DUMPW("errno:%{public}d bind mount %{public}s to %{public}s", errno, arg->srcPath, arg->destPath);
#ifdef APPSPAWN_HISYSEVENT
        if (errno == EINVAL && ++mountFailedCount_ == SandboxCommonDef::MAX_MOUNT_INVALID_COUNT) {
            ReportMountFullHisysevent(APPSPAWN_SANDBOX_MOUNT_FULL);
        }
#endif
        if (errno == ENOENT && IsNeededCheckPathStatus(appProperty, arg->srcPath)) {
            VerifyDirRecursive(arg->srcPath);
        }
        return ret;
    }

    ret = mount(nullptr, arg->destPath, nullptr, arg->mountSharedFlag, nullptr);
    if (ret != 0) {
        APPSPAWN_DUMPW("errno:%{public}d private mount to %{public}s '%{public}u' failed",
            errno, arg->destPath, arg->mountSharedFlag);
        if (errno == EINVAL) {
            CheckMountStatus(arg->destPath);
        }
        return ret;
    }
    return 0;
}

int32_t SandboxCommon::DoAppSandboxMountOnceNocheck(const AppSpawningCtx *appProperty, const SharedMountArgs *arg)
{
    if (!(arg && arg->srcPath && arg->destPath && arg->srcPath[0] != '\0' && arg->destPath[0] != '\0')) {
        return 0;
    }
    if ((strncmp(arg->srcPath, SandboxCommonDef::g_hostsPrefix, strlen(SandboxCommonDef::g_hostsPrefix)) == 0) ||
        (strncmp(arg->srcPath, SandboxCommonDef::g_profilePrefix, strlen(SandboxCommonDef::g_profilePrefix)) == 0) ||
        (strncmp(arg->srcPath, SandboxCommonDef::SUDOERS_PREFIX, strlen(SandboxCommonDef::SUDOERS_PREFIX)) == 0)) {
        CreateFileIfNotExist(arg->destPath);
    } else {
        (void)CreateDirRecursive(arg->destPath, SandboxCommonDef::FILE_MODE);
    }

    int ret = 0;
    APPSPAWN_LOGV("Bind mount %{public}s to %{public}s '%{public}s' '%{public}lu' '%{public}s' '%{public}u'",
        arg->srcPath, arg->destPath, arg->fsType, arg->mountFlags, arg->options, arg->mountSharedFlag);
    ret = mount(arg->srcPath, arg->destPath, arg->fsType, arg->mountFlags, arg->options);
    if (ret != 0) {
        APPSPAWN_LOGV("errno is: %{public}d, bind mount %{public}s to %{public}s", errno, arg->srcPath, arg->destPath);
        APPSPAWN_ONLY_EXPER(errno == ENOENT && IsNeededCheckPathStatus(appProperty, arg->srcPath),
            VerifyDirRecursive(arg->srcPath));
        return ret;
    }
    ret = mount(nullptr, arg->destPath, nullptr, arg->mountSharedFlag, nullptr);
    APPSPAWN_CHECK_LOGV(ret == 0, return ret, "errno is:%{public}d,private mount to %{public}s '%{public}u'failed",
        errno, arg->destPath, arg->mountSharedFlag);
    return 0;
}

// Construct the full param-src-path
std::string SandboxCommon::BuildFullParamSrcPath(cJSON *mntPoint)
{
    const char *srcParamPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_paramPath);
    if (srcParamPathChr == nullptr) {
        return "";
    }

    const char *srcPreParamPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_preParamPath);
    const char *srcPostParamPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_postParamPath);

    std::string srcParamPath = ParseParamTemplate(srcParamPathChr);
    std::string srcPreParamPath = srcPreParamPathChr == nullptr ? "" : srcPreParamPathChr;
    std::string srcPostParmPath = srcPostParamPathChr == nullptr ? "" : srcPostParamPathChr;

    std::vector<std::string> pathComponents;
    if (!srcPreParamPath.empty()) {
        pathComponents.push_back(srcPreParamPath);
    }
    if (!srcParamPath.empty()) {
        pathComponents.push_back(srcParamPath);
    }
    if (!srcPostParmPath.empty()) {
        pathComponents.push_back(srcPostParmPath);
    }

    return JoinParamPaths(pathComponents);
}
} // namespace AppSpawn
} // namespace OHOS