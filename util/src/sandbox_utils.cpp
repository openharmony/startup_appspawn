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

#include "sandbox_utils.h"
#include "json_utils.h"
#include "hilog/log.h"
#include "securec.h"

#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>
#include <cerrno>
#include <dirent.h>
#include <dlfcn.h>
#include <fstream>
#include <sstream>

using namespace std;
using namespace OHOS;
using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0, "AppSpawn_SandboxUtil"};

namespace OHOS {
namespace AppSpawn {
namespace {
    constexpr int32_t UID_BASE = 200000;
    constexpr int32_t FUSE_OPTIONS_MAX_LEN = 128;
    constexpr int32_t DLP_FUSE_FD = 1000;
    constexpr static mode_t FILE_MODE = 0711;
    constexpr static mode_t BASIC_MOUNT_FLAGS = MS_REC | MS_BIND;
    constexpr std::string_view APL_SYSTEM_CORE("system_core");
    constexpr std::string_view APL_SYSTEM_BASIC("system_basic");
    const std::string PHYSICAL_APP_INSTALL_PATH = "/data/app/el1/bundle/public/";
    const std::string SANDBOX_APP_INSTALL_PATH = "/data/accounts/account_0/applications/";
    const std::string DATA_BUNDLES = "/data/bundles/";
    const std::string USERID = "<currentUserId>";
    const std::string PACKAGE_NAME = "<PackageName>";
    const std::string SANDBOX_DIR = "/mnt/sandbox/";
    const std::string STATUS_CHECK = "true";
    const std::string SBX_SWITCH_CHECK = "ON";
    const char *ACTION_STATUS = "check-action-status";
    const char *APP_BASE = "app-base";
    const char *APP_RESOURCES = "app-resources";
    const char *APP_APL_NAME = "app-apl-name";
    const char *COMMON_PREFIX = "common";
    const char *DEST_MODE = "dest-mode";
    const char *FS_TYPE = "fs-type";
    const char *LINK_NAME = "link-name";
    const char *MOUNT_PREFIX = "mount-paths";
    const char *PRIVATE_PREFIX = "individual";
    const char *SRC_PATH = "src-path";
    const char *SANDBOX_PATH = "sandbox-path";
    const char *SANDBOX_FLAGS = "sandbox-flags";
    const char *SANDBOX_SWITCH_PREFIX = "sandbox-switch";
    const char *SYMLINK_PREFIX = "symbol-links";
    const char *SANDBOX_ROOT_PREFIX = "sandbox-root";
    const char *TOP_SANDBOX_SWITCH_PREFIX = "top-sandbox-switch";
    const char *TARGET_NAME = "target-name";
    const char *WARGNAR_DEVICE_PATH = "/3rdmodem";
}

nlohmann::json SandboxUtils::appSandboxConfig_;
nlohmann::json SandboxUtils::productSandboxConfig_;

void SandboxUtils::StoreJsonConfig(nlohmann::json &appSandboxConfig)
{
    SandboxUtils::appSandboxConfig_ = appSandboxConfig;
}

nlohmann::json SandboxUtils::GetJsonConfig()
{
    return SandboxUtils::appSandboxConfig_;
}

void SandboxUtils::StoreProductJsonConfig(nlohmann::json &productSandboxConfig)
{
    SandboxUtils::productSandboxConfig_ = productSandboxConfig;
}

nlohmann::json SandboxUtils::GetProductJsonConfig()
{
    return SandboxUtils::productSandboxConfig_;
}

void SandboxUtils::MakeDirRecursive(const std::string path, mode_t mode)
{
    size_t size = path.size();
    if (size == 0)
        return;

    size_t index = 0;
    do {
        size_t pathIndex = path.find_first_of('/', index);
        index = pathIndex == std::string::npos ? size : pathIndex + 1;
        std::string dir = path.substr(0, index);
        if (access(dir.c_str(), F_OK) < 0 && mkdir(dir.c_str(), mode) < 0) {
            HiLog::Error(LABEL, "mkdir %{public}s error", dir.c_str());
            return;
        }
    } while (index < size);
}

int32_t SandboxUtils::DoAppSandboxMountOnce(const char *originPath, const char *destinationPath,
                                            const char *fsType, unsigned long mountFlags,
                                            const char *options)
{
    int ret = 0;

    // To make sure destinationPath exist
    MakeDirRecursive(destinationPath, FILE_MODE);

    // to mount fs and bind mount files or directory
    ret = mount(originPath, destinationPath, fsType, mountFlags, options);
    if (ret) {
        HiLog::Error(LABEL, "bind mount %{public}s to %{public}s failed %{public}d", originPath,
            destinationPath, errno);
        return ret;
    }

    ret = mount(NULL, destinationPath, NULL, MS_PRIVATE, NULL);
    if (ret) {
        HiLog::Error(LABEL, "private mount to %{public}s failed %{public}d", destinationPath, errno);
        return ret;
    }

    return 0;
}

static std::string& replace_all(std::string& str, const std::string& old_value, const std::string& new_value)
{
    while (true) {
        std::string::size_type pos(0);
        if ((pos = str.find(old_value)) != std::string::npos)
            str.replace(pos, old_value.length(), new_value);
        else break;
    }
    return str;
}

static std::vector<std::string> split(std::string &str, const std::string &pattern)
{
    std::string::size_type pos;
    std::vector<std::string> result;
    str += pattern;
    size_t size = str.size();

    for (unsigned int i = 0; i < size; i++) {
        pos = str.find(pattern, i);
        if (pos < size) {
            std::string s = str.substr(i, pos - i);
            result.push_back(s);
            i = pos + pattern.size() - 1;
        }
    }

    return result;
}

void SandboxUtils::DoSandboxChmod(nlohmann::json jsonConfig, std::string &sandboxRoot)
{
    const std::map<std::string, mode_t> modeMap = {{"S_IRUSR", S_IRUSR}, {"S_IWUSR", S_IWUSR}, {"S_IXUSR", S_IXUSR},
                                                   {"S_IRGRP", S_IRGRP}, {"S_IWGRP", S_IWGRP}, {"S_IXGRP", S_IXGRP},
                                                   {"S_IROTH", S_IROTH}, {"S_IWOTH", S_IWOTH}, {"S_IXOTH", S_IXOTH},
                                                   {"S_IRWXU", S_IRWXU}, {"S_IRWXG", S_IRWXG}, {"S_IRWXO", S_IRWXO}};
    std::string fileModeStr;
    mode_t mode = 0;

    bool rc = JsonUtils::GetStringFromJson(jsonConfig, DEST_MODE, fileModeStr);
    if (rc == false) {
        return;
    }

    std::vector<std::string> modeVec = split(fileModeStr, "|");
    for (unsigned int i = 0; i < modeVec.size(); i++) {
        if (modeMap.count(modeVec[i])) {
            mode |= modeMap.at(modeVec[i]);
        }
    }

    chmod(sandboxRoot.c_str(), mode);
}

unsigned long SandboxUtils::GetMountFlagsFromConfig(const std::vector<std::string> &vec)
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

string SandboxUtils::ConvertToRealPath(const ClientSocket::AppProperty *appProperty, std::string path)
{
    if (path.find(PACKAGE_NAME) != -1) {
        path = replace_all(path, PACKAGE_NAME, appProperty->bundleName);
    }

    if (path.find(USERID) != -1) {
        path = replace_all(path, USERID, std::to_string(appProperty->uid / UID_BASE));
    }

    return path;
}

std::string SandboxUtils::GetSbxPathByConfig(const ClientSocket::AppProperty *appProperty, nlohmann::json &config)
{
    std::string sandboxRoot = "";
    if (config.find(SANDBOX_ROOT_PREFIX) != config.end()) {
        sandboxRoot = config[SANDBOX_ROOT_PREFIX].get<std::string>();
        sandboxRoot = ConvertToRealPath(appProperty, sandboxRoot);
    } else {
        sandboxRoot = SANDBOX_DIR + appProperty->bundleName;
        HiLog::Error(LABEL, "read sandbox-root config failed, set sandbox-root to default root"
            "app name is %{public}s", appProperty->bundleName);
    }

    return sandboxRoot;
}

bool SandboxUtils::GetSbxSwitchStatusByConfig(nlohmann::json &config)
{
    if (config.find(SANDBOX_SWITCH_PREFIX) != config.end()) {
        std::string switchStatus = config[SANDBOX_SWITCH_PREFIX].get<std::string>();
        if (switchStatus == SBX_SWITCH_CHECK) {
            return true;
        } else {
            return false;
        }
    }

    // if not find sandbox-switch node, default switch status is true
    return true;
}

static bool CheckMountConfig(nlohmann::json &mntPoint, const ClientSocket::AppProperty *appProperty)
{
    if (mntPoint.find(SRC_PATH) == mntPoint.end() || mntPoint.find(SANDBOX_PATH) == mntPoint.end()
            || mntPoint.find(SANDBOX_FLAGS) == mntPoint.end()) {
        HiLog::Error(LABEL, "read mount config failed, app name is %{public}s", appProperty->bundleName);
        return false;
    }

    if (mntPoint[APP_APL_NAME] != nullptr) {
        std::string  app_apl_name = mntPoint[APP_APL_NAME];
        const char *p_app_apl = nullptr;
        p_app_apl = app_apl_name.c_str();
        if (!strcmp(p_app_apl, appProperty->apl)) {
            return false;
        }
    }

    return true;
}

static int32_t DoDlpAppMountStrategy(const std::string &srcPath, const std::string &sandboxPath,
                                     const std::string &fsType, unsigned long mountFlags)
{
    int fd = open("/dev/fuse", O_RDWR);
    if (fd == -1) {
        HiLog::Error(LABEL, "open /dev/fuse failed, errno is %{public}d", errno);
        return -EINVAL;
    }

    char options[FUSE_OPTIONS_MAX_LEN];
    (void)sprintf_s(options, sizeof(options), "fd=%d,rootmode=40000,user_id=0,group_id=0", fd);

    int ret = mount(srcPath.c_str(), sandboxPath.c_str(), fsType.c_str(), mountFlags, options);
    if (ret) {
        HiLog::Error(LABEL, "DoDlpAppMountStrategy failed, bind mount %{public}s to %{public}s"
                     "failed %{public}d", srcPath.c_str(), sandboxPath.c_str(), errno);
        return ret;
    }

    /* close DLP_FUSE_FD and dup FD to it */
    close(DLP_FUSE_FD);
    ret = dup2(fd, DLP_FUSE_FD);
    if (ret) {
        HiLog::Error(LABEL, "dup fuse fd %{public}d failed, errno is %{public}d",
                     fd, errno);
    }

    return ret;
}

static int32_t HandleSpecialAppMount(const std::string &srcPath, const std::string &sandboxPath,
                                     const std::string &fsType, unsigned long mountFlags,
                                     const std::string &bundleName)
{
    /* dlp application mount strategy */
    /* dlp is an example, we should change to real bundle name later */
    if (bundleName.find("dlp") != -1) {
        if (fsType.empty()) {
            return -1;
        } else {
            return DoDlpAppMountStrategy(srcPath, sandboxPath, fsType, mountFlags);
        }
    }

    return -1;
}

int SandboxUtils::DoAllMntPointsMount(const ClientSocket::AppProperty *appProperty, nlohmann::json &appConfig)
{
    if (appConfig.find(MOUNT_PREFIX) == appConfig.end()) {
        HiLog::Debug(LABEL, "mount config is not found, maybe reuslt sandbox launch failed"
            "app name is %{public}s", appProperty->bundleName);
        return 0;
    }

    nlohmann::json mountPoints = appConfig[MOUNT_PREFIX];
    std::string sandboxRoot = GetSbxPathByConfig(appProperty, appConfig);
    unsigned int mountPointSize = mountPoints.size();

    for (unsigned int i = 0; i < mountPointSize; i++) {
        nlohmann::json mntPoint = mountPoints[i];

        if (CheckMountConfig(mntPoint, appProperty) == false) {
            continue;
        }

        std::string srcPath = ConvertToRealPath(appProperty, mntPoint[SRC_PATH].get<std::string>());
        std::string sandboxPath = sandboxRoot + ConvertToRealPath(appProperty,
                                                                  mntPoint[SANDBOX_PATH].get<std::string>());
        unsigned long mountFlags = GetMountFlagsFromConfig(mntPoint[SANDBOX_FLAGS].get<std::vector<std::string>>());
        std::string fsType = "";
        if (mntPoint.find(FS_TYPE) != mntPoint.end()) {
            fsType = mntPoint[FS_TYPE].get<std::string>();
        }

        int ret = 0;
        /* if app mount failed for special strategy, we need deal with common mount config */
        ret = HandleSpecialAppMount(srcPath, sandboxPath, fsType, mountFlags, appProperty->bundleName);
        if (ret) {
            if (fsType.empty()) {
                ret = DoAppSandboxMountOnce(srcPath.c_str(), sandboxPath.c_str(), nullptr, mountFlags, nullptr);
            } else {
                ret = DoAppSandboxMountOnce(srcPath.c_str(), sandboxPath.c_str(), fsType.c_str(), mountFlags, nullptr);
            }
        }
        if (ret) {
            HiLog::Error(LABEL, "DoAppSandboxMountOnce failed, %{public}s", sandboxPath.c_str());

            std::string actionStatus = STATUS_CHECK;
            (void)JsonUtils::GetStringFromJson(mntPoint, ACTION_STATUS, actionStatus);
            if (actionStatus == STATUS_CHECK) {
                return ret;
            }
        }

        DoSandboxChmod(mntPoint, sandboxRoot);
    }

    return 0;
}

int SandboxUtils::DoAllSymlinkPointslink(const ClientSocket::AppProperty *appProperty, nlohmann::json &appConfig)
{
    if (appConfig.find(SYMLINK_PREFIX) == appConfig.end()) {
        HiLog::Debug(LABEL, "symlink config is not found, maybe reuslt sandbox launch failed"
            "app name is %{public}s", appProperty->bundleName);
        return 0;
    }

    nlohmann::json symlinkPoints = appConfig[SYMLINK_PREFIX];
    std::string sandboxRoot = GetSbxPathByConfig(appProperty, appConfig);
    unsigned int symlinkPointSize = symlinkPoints.size();

    for (unsigned int i = 0; i < symlinkPointSize; i++) {
        nlohmann::json symPoint = symlinkPoints[i];

        // Check the validity of the symlink configuration
        if (symPoint.find(TARGET_NAME) == symPoint.end() || symPoint.find(LINK_NAME) == symPoint.end()) {
            HiLog::Error(LABEL, "read symlink config failed, app name is %{public}s", appProperty->bundleName);
            continue;
        }

        std::string targetName = ConvertToRealPath(appProperty, symPoint[TARGET_NAME].get<std::string>());
        std::string linkName = sandboxRoot + ConvertToRealPath(appProperty, symPoint[LINK_NAME].get<std::string>());
        HiLog::Debug(LABEL, "symlink, from %{public}s to %{public}s", targetName.c_str(), linkName.c_str());

        int ret = symlink(targetName.c_str(), linkName.c_str());
        if (ret && errno != EEXIST) {
            HiLog::Error(LABEL, "symlink failed, %{public}s, errno is %{public}d", linkName.c_str(), errno);

            std::string actionStatus = STATUS_CHECK;
            (void)JsonUtils::GetStringFromJson(symPoint, ACTION_STATUS, actionStatus);
            if (actionStatus == STATUS_CHECK) {
                return ret;
            }
        }

        DoSandboxChmod(symPoint, sandboxRoot);
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxFilePrivateBind(const ClientSocket::AppProperty *appProperty,
                                               nlohmann::json &wholeConfig)
{
    nlohmann::json privateAppConfig = wholeConfig[PRIVATE_PREFIX][0];
    if (privateAppConfig.find(appProperty->bundleName) != privateAppConfig.end()) {
        return DoAllMntPointsMount(appProperty, privateAppConfig[appProperty->bundleName][0]);
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxFilePrivateSymlink(const ClientSocket::AppProperty *appProperty,
                                                  nlohmann::json &wholeConfig)
{
    nlohmann::json privateAppConfig = wholeConfig[PRIVATE_PREFIX][0];
    if (privateAppConfig.find(appProperty->bundleName) != privateAppConfig.end()) {
        return DoAllSymlinkPointslink(appProperty, privateAppConfig[appProperty->bundleName][0]);
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxFileCommonBind(const ClientSocket::AppProperty *appProperty, nlohmann::json &wholeConfig)
{
    nlohmann::json commonConfig = wholeConfig[COMMON_PREFIX][0];
    int ret = 0;

    if (commonConfig.find(APP_BASE) != commonConfig.end()) {
        ret = DoAllMntPointsMount(appProperty, commonConfig[APP_BASE][0]);
        if (ret) {
            return ret;
        }
    }

    if (commonConfig.find(APP_RESOURCES) != commonConfig.end()) {
        ret = DoAllMntPointsMount(appProperty, commonConfig[APP_RESOURCES][0]);
    }

    return ret;
}

int32_t SandboxUtils::DoSandboxFileCommonSymlink(const ClientSocket::AppProperty *appProperty,
                                                 nlohmann::json &wholeConfig)
{
    nlohmann::json commonConfig = wholeConfig[COMMON_PREFIX][0];
    int ret = 0;

    if (commonConfig.find(APP_BASE) != commonConfig.end()) {
        ret = DoAllSymlinkPointslink(appProperty, commonConfig[APP_BASE][0]);
        if (ret) {
            return ret;
        }
    }

    if (commonConfig.find(APP_RESOURCES) != commonConfig.end()) {
        ret = DoAllSymlinkPointslink(appProperty, commonConfig[APP_RESOURCES][0]);
    }

    return ret;
}

int32_t SandboxUtils::SetPrivateAppSandboxProperty_(const ClientSocket::AppProperty *appProperty,
                                                    nlohmann::json &config)
{
    int ret = 0;

    ret = DoSandboxFilePrivateBind(appProperty, config);
    if (ret) {
        HiLog::Error(LABEL, "DoSandboxFilePrivateBind failed");
        return ret;
    }

    ret = DoSandboxFilePrivateSymlink(appProperty, config);
    if (ret) {
        HiLog::Error(LABEL, "DoSandboxFilePrivateSymlink failed");
    }

    return ret;
}

int32_t SandboxUtils::SetPrivateAppSandboxProperty(const ClientSocket::AppProperty *appProperty)
{
    nlohmann::json productConfig = SandboxUtils::GetProductJsonConfig();
    nlohmann::json config = SandboxUtils::GetJsonConfig();
    int ret = 0;

    ret = SetPrivateAppSandboxProperty_(appProperty, config);
    if (ret) {
        HiLog::Error(LABEL, "parse adddata-sandbox config failed");
        return ret;
    }

    ret = SetPrivateAppSandboxProperty_(appProperty, productConfig);
    if (ret) {
        HiLog::Error(LABEL, "parse product-sandbox config failed");
    }

    return ret;
}

int32_t SandboxUtils::SetCommonAppSandboxProperty_(const ClientSocket::AppProperty *appProperty,
                                                   nlohmann::json &config)
{
    int rc = 0;

    rc = DoSandboxFileCommonBind(appProperty, config);
    if (rc) {
        HiLog::Error(LABEL, "DoSandboxFileCommonBind failed, %{public}s", appProperty->bundleName);
        return rc;
    }

    // if sandbox switch is off, don't do symlink work again
    if (CheckAppSandboxSwitchStatus(appProperty) == true && (CheckTotalSandboxSwitchStatus(appProperty) == true)) {
        rc = DoSandboxFileCommonSymlink(appProperty, config);
        if (rc) {
            HiLog::Error(LABEL, "DoSandboxFileCommonSymlink failed, %{public}s", appProperty->bundleName);
        }
    }

    return rc;
}

int32_t SandboxUtils::SetCommonAppSandboxProperty(const ClientSocket::AppProperty *appProperty,
                                                  std::string &sandboxPackagePath)
{
    nlohmann::json jsonConfig = SandboxUtils::GetJsonConfig();
    nlohmann::json productConfig = SandboxUtils::GetProductJsonConfig();
    int ret = 0;

    ret = SetCommonAppSandboxProperty_(appProperty, jsonConfig);
    if (ret) {
        HiLog::Error(LABEL, "parse appdata config for common failed, %{public}s", sandboxPackagePath.c_str());
        return ret;
    }

    ret = SetCommonAppSandboxProperty_(appProperty, productConfig);
    if (ret) {
        HiLog::Error(LABEL, "parse product config for common failed, %{public}s", sandboxPackagePath.c_str());
        return ret;
    }

    if (strcmp(appProperty->apl, APL_SYSTEM_BASIC.data()) == 0 ||
        strcmp(appProperty->apl, APL_SYSTEM_CORE.data()) == 0) {
        // need permission check for system app here
        std::string destbundlesPath = sandboxPackagePath + DATA_BUNDLES;
        DoAppSandboxMountOnce(PHYSICAL_APP_INSTALL_PATH.c_str(), destbundlesPath.c_str(), "", BASIC_MOUNT_FLAGS,
                              nullptr);
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxRootFolderCreateAdapt(std::string &sandboxPackagePath)
{
    int rc = mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL);
    if (rc) {
        HiLog::Error(LABEL, "set propagation slave failed");
        return rc;
    }

    MakeDirRecursive(sandboxPackagePath, FILE_MODE);

    // bind mount "/" to /mnt/sandbox/<packageName> path
    // rootfs: to do more resources bind mount here to get more strict resources constraints
    rc = mount("/", sandboxPackagePath.c_str(), NULL, BASIC_MOUNT_FLAGS, NULL);
    if (rc) {
        HiLog::Error(LABEL, "mount bind / failed, %{public}d", errno);
        return rc;
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxRootFolderCreate(const ClientSocket::AppProperty *appProperty,
                                                std::string &sandboxPackagePath)
{
    int rc = mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL);
    if (rc) {
        return rc;
    }

    DoAppSandboxMountOnce(sandboxPackagePath.c_str(), sandboxPackagePath.c_str(), "",
                          BASIC_MOUNT_FLAGS, nullptr);

    return 0;
}

bool SandboxUtils::CheckTotalSandboxSwitchStatus(const ClientSocket::AppProperty *appProperty)
{
    nlohmann::json wholeConfig = SandboxUtils::GetJsonConfig();

    nlohmann::json commonAppConfig = wholeConfig[COMMON_PREFIX][0];
    if (commonAppConfig.find(TOP_SANDBOX_SWITCH_PREFIX) != commonAppConfig.end()) {
        std::string switchStatus = commonAppConfig[TOP_SANDBOX_SWITCH_PREFIX].get<std::string>();
        if (switchStatus == SBX_SWITCH_CHECK) {
            return true;
        } else {
            return false;
        }
    }

    // default sandbox switch is on
    return true;
}

bool SandboxUtils::CheckAppSandboxSwitchStatus(const ClientSocket::AppProperty *appProperty)
{
    nlohmann::json wholeConfig = SandboxUtils::GetJsonConfig();
    bool rc = true;

    nlohmann::json privateAppConfig = wholeConfig[PRIVATE_PREFIX][0];
    if (privateAppConfig.find(appProperty->bundleName) != privateAppConfig.end()) {
        nlohmann::json appConfig = privateAppConfig[appProperty->bundleName][0];
        rc = GetSbxSwitchStatusByConfig(appConfig);
        HiLog::Error(LABEL, "CheckAppSandboxSwitchStatus middle, %{public}d", rc);
    }

    // default sandbox switch is on
    return rc;
}

int32_t SandboxUtils::SetAppSandboxProperty(const ClientSocket::AppProperty *appProperty)
{
    std::string sandboxPackagePath = "/mnt/sandbox/";
    mkdir(sandboxPackagePath.c_str(), FILE_MODE);
    const std::string bundleName = appProperty->bundleName;
    sandboxPackagePath += bundleName;
    mkdir(sandboxPackagePath.c_str(), FILE_MODE);

    // add pid to a new mnt namespace
    int rc = unshare(CLONE_NEWNS);
    if (rc) {
        HiLog::Error(LABEL, "unshare failed, packagename is %{public}s", bundleName.c_str());
        return rc;
    }

    // to make wargnar work and check app sandbox switch
    if (access(WARGNAR_DEVICE_PATH, F_OK) == 0 || (CheckTotalSandboxSwitchStatus(appProperty) == false) ||
        (CheckAppSandboxSwitchStatus(appProperty) == false)) {
        rc = DoSandboxRootFolderCreateAdapt(sandboxPackagePath);
    } else {
        rc = DoSandboxRootFolderCreate(appProperty, sandboxPackagePath);
    }
    if (rc) {
        HiLog::Error(LABEL, "DoSandboxRootFolderCreate failed, %{public}s", bundleName.c_str());
        return rc;
    }

    rc = SetCommonAppSandboxProperty(appProperty, sandboxPackagePath);
    if (rc) {
        HiLog::Error(LABEL, "SetCommonAppSandboxProperty failed, packagename is %{public}s", bundleName.c_str());
        return rc;
    }

    rc = SetPrivateAppSandboxProperty(appProperty);
    if (rc) {
        HiLog::Error(LABEL, "SetPrivateAppSandboxProperty failed, packagename is %{public}s", bundleName.c_str());
        return rc;
    }

    rc = chdir(sandboxPackagePath.c_str());
    if (rc) {
        HiLog::Error(LABEL, "chdir failed, packagename is %{public}s, path is %{public}s", \
            bundleName.c_str(), sandboxPackagePath.c_str());
        return rc;
    }

    rc = syscall(SYS_pivot_root, sandboxPackagePath.c_str(), sandboxPackagePath.c_str());
    if (rc) {
        HiLog::Error(LABEL, "pivot root failed, packagename is %{public}s, errno is %{public}d", \
            bundleName.c_str(), errno);
        return rc;
    }

    rc = umount2(".", MNT_DETACH);
    if (rc) {
        HiLog::Error(LABEL, "MNT_DETACH failed, packagename is %{public}s", bundleName.c_str());
        return rc;
    }

    return 0;
}
} // namespace AppSpawn
} // namespace OHOS
