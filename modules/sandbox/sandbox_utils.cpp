/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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

#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <vector>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "appspawn_hook.h"
#include "appspawn_mount_permission.h"
#include "appspawn_msg.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "appspawn_utils.h"
#include "config_policy_utils.h"
#include "sandbox_shared_mount.h"
#ifdef WITH_DLP
#include "dlp_fuse_fd.h"
#endif
#include "init_param.h"
#include "init_utils.h"
#include "parameter.h"
#include "parameters.h"
#include "securec.h"
#include "appspawn_trace.h"
#ifdef APPSPAWN_HISYSEVENT
#include "hisysevent_adapter.h"
#endif

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif

#define MAX_MOUNT_TIME 500  // 500us
#define LOCK_STATUS_SIZE 16

using namespace std;
using namespace OHOS;

namespace OHOS {
namespace AppSpawn {
namespace {
    constexpr int32_t OPTIONS_MAX_LEN = 256;
    constexpr int32_t FILE_ACCESS_COMMON_DIR_STATUS = 0;
    constexpr int32_t FILE_CROSS_APP_STATUS = 1;
    constexpr static mode_t FILE_MODE = 0711;
    constexpr static mode_t BASIC_MOUNT_FLAGS = MS_REC | MS_BIND;
    constexpr std::string_view APL_SYSTEM_CORE("system_core");
    constexpr std::string_view APL_SYSTEM_BASIC("system_basic");
    const std::string APP_JSON_CONFIG("/appdata-sandbox.json");
    const std::string APP_ISOLATED_JSON_CONFIG("/appdata-sandbox-isolated.json");
    const std::string g_physicalAppInstallPath = "/data/app/el1/bundle/public/";
    const std::string g_sandboxGroupPath = "/data/storage/el2/group/";
    const std::string g_sandboxHspInstallPath = "/data/storage/el1/bundle/";
    const std::string g_sandBoxAppInstallPath = "/data/accounts/account_0/applications/";
    const std::string g_bundleResourceSrcPath = "/data/service/el1/public/bms/bundle_resources/";
    const std::string g_bundleResourceDestPath = "/data/storage/bundle_resources/";
    const std::string g_dataBundles = "/data/bundles/";
    const std::string g_userId = "<currentUserId>";
    const std::string g_packageName = "<PackageName>";
    const std::string g_packageNameIndex = "<PackageName_index>";
    const std::string g_variablePackageName = "<variablePackageName>";
    const std::string g_arkWebPackageName = "<arkWebPackageName>";
    const std::string g_sandBoxDir = "/mnt/sandbox/";
    const std::string g_statusCheck = "true";
    const std::string g_sbxSwitchCheck = "ON";
    const std::string g_dlpBundleName = "com.ohos.dlpmanager";
    const std::string g_internal = "__internal__";
    const std::string g_hspList_key_bundles = "bundles";
    const std::string g_hspList_key_modules = "modules";
    const std::string g_hspList_key_versions = "versions";
    const std::string g_overlayPath = "/data/storage/overlay/";
    const std::string g_groupList_key_dataGroupId = "dataGroupId";
    const std::string g_groupList_key_gid = "gid";
    const std::string g_groupList_key_dir = "dir";
    const std::string g_groupList_key_uuid = "uuid";
    const std::string HSPLIST_SOCKET_TYPE = "HspList";
    const std::string OVERLAY_SOCKET_TYPE = "Overlay";
    const std::string DATA_GROUP_SOCKET_TYPE = "DataGroup";
    const char *g_actionStatuc = "check-action-status";
    const char *g_appBase = "app-base";
    const char *g_appResources = "app-resources";
    const char *g_appAplName = "app-apl-name";
    const char *g_commonPrefix = "common";
    const char *g_destMode = "dest-mode";
    const char *g_fsType = "fs-type";
    const char *g_linkName = "link-name";
    const char *g_mountPrefix = "mount-paths";
    const char *g_gidPrefix = "gids";
    const char *g_privatePrefix = "individual";
    const char *g_permissionPrefix = "permission";
    const char *g_srcPath = "src-path";
    const char *g_sandBoxPath = "sandbox-path";
    const char *g_sandBoxFlags = "sandbox-flags";
    const char *g_sandBoxFlagsCustomized = "sandbox-flags-customized";
    const char *g_sandBoxOptions = "options";
    const char *g_dacOverrideSensitive = "dac-override-sensitive";
    const char *g_sandBoxShared = "sandbox-shared";
    const char *g_sandBoxSwitchPrefix = "sandbox-switch";
    const char *g_symlinkPrefix = "symbol-links";
    const char *g_sandboxRootPrefix = "sandbox-root";
    const char *g_topSandBoxSwitchPrefix = "top-sandbox-switch";
    const char *g_targetName = "target-name";
    const char *g_flagePoint = "flags-point";
    const char *g_mountSharedFlag = "mount-shared-flag";
    const char *g_flags = "flags";
    const char *g_sandBoxNsFlags = "sandbox-ns-flags";
    const char* g_fileSeparator = "/";
    const char* g_overlayDecollator = "|";
    const std::string g_sandBoxRootDir = "/mnt/sandbox/";
    const std::string g_ohosRender = "__internal__.com.ohos.render";
    const std::string g_sandBoxRootDirNweb = "/mnt/sandbox/com.ohos.render/";
    const std::string FILE_CROSS_APP_MODE = "ohos.permission.FILE_CROSS_APP";
    const std::string FILE_ACCESS_COMMON_DIR_MODE = "ohos.permission.FILE_ACCESS_COMMON_DIR";
    const std::string ACCESS_DLP_FILE_MODE = "ohos.permission.ACCESS_DLP_FILE";
    const std::string FILE_ACCESS_MANAGER_MODE = "ohos.permission.FILE_ACCESS_MANAGER";
    const std::string ARK_WEB_PERSIST_PACKAGE_NAME = "persist.arkwebcore.package_name";

    const std::string& getArkWebPackageName()
    {
        static std::string arkWebPackageName;
        if (arkWebPackageName.empty()) {
            arkWebPackageName = system::GetParameter(ARK_WEB_PERSIST_PACKAGE_NAME, "");
        }
        return arkWebPackageName;
    }
}

static uint32_t GetAppMsgFlags(const AppSpawningCtx *property)
{
    APPSPAWN_CHECK(property != nullptr && property->message != nullptr,
        return 0, "Invalid property for name %{public}u", TLV_MSG_FLAGS);
    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppSpawnMsgInfo(property->message, TLV_MSG_FLAGS);
    APPSPAWN_CHECK(msgFlags != nullptr,
        return 0, "No TLV_MSG_FLAGS in msg %{public}s", property->message->msgHeader.processName);
    return msgFlags->flags[0];
}

bool JsonUtils::GetJsonObjFromJson(nlohmann::json &jsonObj, const std::string &jsonPath)
{
    APPSPAWN_CHECK(jsonPath.length() <= PATH_MAX, return false, "jsonPath is too long");
    std::ifstream jsonFileStream;
    jsonFileStream.open(jsonPath.c_str(), std::ios::in);
    APPSPAWN_CHECK_ONLY_EXPER(jsonFileStream.is_open(), return false);
    std::ostringstream buf;
    char ch;
    while (buf && jsonFileStream.get(ch)) {
        buf.put(ch);
    }
    jsonFileStream.close();
    jsonObj = nlohmann::json::parse(buf.str(), nullptr, false);
    APPSPAWN_CHECK(!jsonObj.is_discarded() && jsonObj.is_structured(), return false, "Parse json file failed");
    return true;
}

bool JsonUtils::GetStringFromJson(const nlohmann::json &json, const std::string &key, std::string &value)
{
    APPSPAWN_CHECK(json.is_object(), return false, "json is not object.");
    bool isRet = json.find(key) != json.end() && json.at(key).is_string();
    if (isRet) {
        value = json.at(key).get<std::string>();
        APPSPAWN_LOGV("Find key[%{public}s] : %{public}s successful.", key.c_str(), value.c_str());
        return true;
    } else {
        return false;
    }
}

std::map<SandboxConfigType, std::vector<nlohmann::json>> SandboxUtils::appSandboxConfig_ = {};
int32_t SandboxUtils::deviceTypeEnable_ = -1;

void SandboxUtils::StoreJsonConfig(nlohmann::json &appSandboxConfig, SandboxConfigType type)
{
    SandboxUtils::appSandboxConfig_[type].push_back(appSandboxConfig);
}

std::vector<nlohmann::json> &SandboxUtils::GetJsonConfig(SandboxConfigType type)
{
    return SandboxUtils::appSandboxConfig_[type];
}

static void MakeDirRecursive(const std::string &path, mode_t mode)
{
    size_t size = path.size();
    if (size == 0) {
        return;
    }

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
}

static void MakeDirRecursiveWithClock(const std::string &path, mode_t mode)
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

static bool CheckDirRecursive(const std::string &path)
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
        APPSPAWN_CHECK(access(dir.c_str(), F_OK) == 0,
            return false, "check dir %{public}s failed, strerror: %{public}s", dir.c_str(), strerror(errno));
#endif
    } while (index < size);
    return true;
}

static void CheckAndCreatFile(const char *file)
{
    if (access(file, F_OK) == 0) {
        APPSPAWN_LOGI("file %{public}s already exist", file);
        return;
    }
    std::string path = file;
    auto pos = path.find_last_of('/');
    APPSPAWN_CHECK(pos != std::string::npos, return, "file %{public}s error", file);
    std::string dir = path.substr(0, pos);
    MakeDirRecursive(dir, FILE_MODE);
    int fd = open(file, O_CREAT, FILE_MODE);
    if (fd < 0) {
        APPSPAWN_LOGW("failed create %{public}s, err=%{public}d", file, errno);
    } else {
        close(fd);
    }
    return;
}

int32_t SandboxUtils::DoAppSandboxMountOnce(const char *originPath, const char *destinationPath,
                                            const char *fsType, unsigned long mountFlags,
                                            const char *options, mode_t mountSharedFlag)
{
    if (originPath == nullptr || destinationPath == nullptr || originPath[0] == '\0' || destinationPath[0] == '\0') {
        return 0;
    }
    if (strstr(originPath, "system/etc/hosts") != nullptr) {
        CheckAndCreatFile(destinationPath);
    } else {
        MakeDirRecursive(destinationPath, FILE_MODE);
    }

    int ret = 0;
    // to mount fs and bind mount files or directory
    struct timespec mountStart = {0};
    clock_gettime(CLOCK_MONOTONIC, &mountStart);
    APPSPAWN_LOGV("Bind mount %{public}s to %{public}s '%{public}s' '%{public}lu' '%{public}s' '%{public}u'",
        originPath, destinationPath, fsType, mountFlags, options, mountSharedFlag);
    ret = mount(originPath, destinationPath, fsType, mountFlags, options);
    struct timespec mountEnd = {0};
    clock_gettime(CLOCK_MONOTONIC, &mountEnd);
    uint64_t diff = DiffTime(&mountStart, &mountEnd);
    APPSPAWN_CHECK_ONLY_LOG(diff < MAX_MOUNT_TIME, "mount %{public}s time %{public}" PRId64 " us", originPath, diff);
#ifdef APPSPAWN_HISYSEVENT
    APPSPAWN_CHECK_ONLY_EXPER(diff < FUNC_REPORT_DURATION, ReportAbnormalDuration("MOUNT", diff));
#endif
    if (ret != 0) {
        APPSPAWN_LOGI("errno is: %{public}d, bind mount %{public}s to %{public}s", errno, originPath, destinationPath);
        std::string originPathStr = originPath == nullptr ? "" : originPath;
        if (originPathStr.find("data/app/el1/") != std::string::npos ||
            originPathStr.find("data/app/el2/") != std::string::npos) {
            CheckDirRecursive(originPathStr);
        }
        return ret;
    }

    ret = mount(nullptr, destinationPath, nullptr, mountSharedFlag, nullptr);
    APPSPAWN_CHECK(ret == 0, return ret, "errno is: %{public}d, private mount to %{public}s '%{public}u' failed",
        errno, destinationPath, mountSharedFlag);
    return 0;
}

static std::string& replace_all(std::string& str, const std::string& old_value, const std::string& new_value)
{
    while (true) {
        std::string::size_type pos(0);
        if ((pos = str.find(old_value)) != std::string::npos) {
            str.replace(pos, old_value.length(), new_value);
        } else {
            break;
        }
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

    bool rc = JsonUtils::GetStringFromJson(jsonConfig, g_destMode, fileModeStr);
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

static void MakeAtomicServiceDir(const AppSpawningCtx *appProperty, std::string path, std::string variablePackageName)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    APPSPAWN_CHECK(dacInfo != NULL, return, "No dac info in msg app property");
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
    APPSPAWN_CHECK(msgDomainInfo != NULL, return, "No domain info for %{public}s", GetProcessName(appProperty));
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

static std::string ReplaceVariablePackageName(const AppSpawningCtx *appProperty, const std::string &path)
{
    std::string tmpSandboxPath = path;
    AppSpawnMsgBundleInfo *bundleInfo =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    APPSPAWN_CHECK(bundleInfo != NULL, return "", "No bundle info in msg %{public}s", GetBundleName(appProperty));

    char *extension;
    uint32_t flags = CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_ATOMIC_SERVICE) ? 0x4 : 0;
    if (flags == 0) {
        flags = (CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_CLONE_ENABLE) &&
            bundleInfo->bundleIndex > 0) ? 0x1 : 0;
        flags |= CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_EXTENSION_SANDBOX) ? 0x2 : 0;
        extension = reinterpret_cast<char *>(
            GetAppSpawnMsgExtInfo(appProperty->message, MSG_EXT_NAME_APP_EXTENSION, NULL));
    }
    std::ostringstream variablePackageName;
    switch (flags) {
        case 0:    // 0 default
            variablePackageName << bundleInfo->bundleName;
            break;
        case 1:    // 1 +clone-bundleIndex+packageName
            variablePackageName << "+clone-" << bundleInfo->bundleIndex << "+" << bundleInfo->bundleName;
            break;
        case 2: {  // 2 +extension-<extensionType>+packageName
            APPSPAWN_CHECK(extension != NULL, return "", "Invalid extension data ");
            variablePackageName << "+extension-" << extension << "+" << bundleInfo->bundleName;
            break;
        }
        case 3: {  // 3 +clone-bundleIndex+extension-<extensionType>+packageName
            APPSPAWN_CHECK(extension != NULL, return "", "Invalid extension data ");
            variablePackageName << "+clone-" << bundleInfo->bundleIndex << "+extension" << "-" <<
                extension << "+" << bundleInfo->bundleName;
            break;
        }
        case 4: {  // 4 +auid-<accountId>+packageName
            std::string accountId = SandboxUtils::GetExtraInfoByType(appProperty, MSG_EXT_NAME_ACCOUNT_ID);
            variablePackageName << "+auid-" << accountId << "+" << bundleInfo->bundleName;
            std::string atomicServicePath = path;
            atomicServicePath = replace_all(atomicServicePath, g_variablePackageName, variablePackageName.str());
            MakeAtomicServiceDir(appProperty, atomicServicePath, variablePackageName.str());
            break;
        }
        default:
            variablePackageName << bundleInfo->bundleName;
            break;
    }
    tmpSandboxPath = replace_all(tmpSandboxPath, g_variablePackageName, variablePackageName.str());
    APPSPAWN_LOGV("tmpSandboxPath %{public}s", tmpSandboxPath.c_str());
    return tmpSandboxPath;
}

string SandboxUtils::ConvertToRealPath(const AppSpawningCtx *appProperty, std::string path)
{
    AppSpawnMsgBundleInfo *info =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (info == nullptr || dacInfo == nullptr) {
        return "";
    }
    if (path.find(g_packageNameIndex) != std::string::npos) {
        std::string bundleNameWithIndex = info->bundleName;
        if (info->bundleIndex != 0) {
            bundleNameWithIndex = std::to_string(info->bundleIndex) + "_" + bundleNameWithIndex;
        }
        path = replace_all(path, g_packageNameIndex, bundleNameWithIndex);
    }

    if (path.find(g_packageName) != std::string::npos) {
        path = replace_all(path, g_packageName, info->bundleName);
    }

    if (path.find(g_userId) != std::string::npos) {
        path = replace_all(path, g_userId, std::to_string(dacInfo->uid / UID_BASE));
    }

    if (path.find(g_variablePackageName) != std::string::npos) {
        path = ReplaceVariablePackageName(appProperty, path);
    }

    if (path.find(g_arkWebPackageName) != std::string::npos) {
        path = replace_all(path, g_arkWebPackageName, getArkWebPackageName());
        APPSPAWN_LOGV(
            "arkWeb sandbox, path %{public}s, package:%{public}s",
            path.c_str(), getArkWebPackageName().c_str());
    }

    return path;
}

std::string SandboxUtils::ConvertToRealPathWithPermission(const AppSpawningCtx *appProperty,
                                                          std::string path)
{
    AppSpawnMsgBundleInfo *info =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    if (info == nullptr) {
        return "";
    }
    if (path.find(g_packageNameIndex) != std::string::npos) {
        std::string bundleNameWithIndex = info->bundleName;
        if (info->bundleIndex != 0) {
            bundleNameWithIndex = std::to_string(info->bundleIndex) + "_" + bundleNameWithIndex;
        }
        path = replace_all(path, g_packageNameIndex, bundleNameWithIndex);
    }

    if (path.find(g_packageName) != std::string::npos) {
        path = replace_all(path, g_packageName, info->bundleName);
    }

    if (path.find(g_userId) != std::string::npos) {
        if (deviceTypeEnable_ == FILE_CROSS_APP_STATUS) {
            path = replace_all(path, g_userId, "currentUser");
        } else if (deviceTypeEnable_ == FILE_ACCESS_COMMON_DIR_STATUS) {
            path = replace_all(path, g_userId, "currentUser");
        } else {
            return "";
        }
    }
    return path;
}

bool SandboxUtils::GetSandboxDacOverrideEnable(nlohmann::json &config)
{
    std::string dacOverrideSensitive = "";
    if (config.find(g_dacOverrideSensitive) == config.end()) {
        return false;
    }
    dacOverrideSensitive = config[g_dacOverrideSensitive].get<std::string>();
    if (dacOverrideSensitive.compare("true") == 0) {
        return true;
    }
    return false;
}

std::string SandboxUtils::GetSbxPathByConfig(const AppSpawningCtx *appProperty, nlohmann::json &config)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return "";
    }

    std::string sandboxRoot = "";
    const std::string originSandboxPath = "/mnt/sandbox/<PackageName>";
    std::string isolatedFlagText = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ? "isolated/" : "";
    const std::string defaultSandboxRoot = g_sandBoxDir + to_string(dacInfo->uid / UID_BASE) +
        "/" + isolatedFlagText.c_str() + GetBundleName(appProperty);
    if (config.find(g_sandboxRootPrefix) != config.end()) {
        sandboxRoot = config[g_sandboxRootPrefix].get<std::string>();
        if (sandboxRoot == originSandboxPath) {
            sandboxRoot = defaultSandboxRoot;
        } else {
            sandboxRoot = ConvertToRealPath(appProperty, sandboxRoot);
            APPSPAWN_LOGV("set sandbox-root name is %{public}s", sandboxRoot.c_str());
        }
    } else {
        sandboxRoot = defaultSandboxRoot;
        APPSPAWN_LOGV("set sandbox-root to default rootapp name is %{public}s", GetBundleName(appProperty));
    }

    return sandboxRoot;
}

bool SandboxUtils::GetSbxSwitchStatusByConfig(nlohmann::json &config)
{
    if (config.find(g_sandBoxSwitchPrefix) != config.end()) {
        std::string switchStatus = config[g_sandBoxSwitchPrefix].get<std::string>();
        if (switchStatus == g_sbxSwitchCheck) {
            return true;
        } else {
            return false;
        }
    }

    // if not find sandbox-switch node, default switch status is true
    return true;
}

static bool CheckMountConfig(nlohmann::json &mntPoint, const AppSpawningCtx *appProperty,
                             bool checkFlag)
{
    bool istrue = mntPoint.find(g_srcPath) == mntPoint.end() || (!mntPoint[g_srcPath].is_string()) ||
                  mntPoint.find(g_sandBoxPath) == mntPoint.end() || (!mntPoint[g_sandBoxPath].is_string()) ||
                  ((mntPoint.find(g_sandBoxFlags) == mntPoint.end()) &&
                  (mntPoint.find(g_sandBoxFlagsCustomized) == mntPoint.end()));
    APPSPAWN_CHECK(!istrue, return false,
        "read mount config failed, app name is %{public}s", GetBundleName(appProperty));

    AppSpawnMsgDomainInfo *info =
        reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(appProperty, TLV_DOMAIN_INFO));
    APPSPAWN_CHECK(info != nullptr, return false, "Filed to get domain info %{public}s", GetBundleName(appProperty));

    if (mntPoint[g_appAplName] != nullptr) {
        std::string app_apl_name = mntPoint[g_appAplName].get<std::string>();
        const char *p_app_apl = nullptr;
        p_app_apl = app_apl_name.c_str();
        if (!strcmp(p_app_apl, info->apl)) {
            return false;
        }
    }

    const std::string configSrcPath = mntPoint[g_srcPath].get<std::string>();
    // special handle wps and don't use /data/app/xxx/<Package> config
    if (checkFlag && (configSrcPath.find("/data/app") != std::string::npos &&
        (configSrcPath.find("/base") != std::string::npos ||
         configSrcPath.find("/database") != std::string::npos
        ) && configSrcPath.find(g_packageName) != std::string::npos)) {
        return false;
    }

    return true;
}

static int32_t DoDlpAppMountStrategy(const AppSpawningCtx *appProperty,
                                     const std::string &srcPath, const std::string &sandboxPath,
                                     const std::string &fsType, unsigned long mountFlags)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return -1;
    }

    // umount fuse path, make sure that sandbox path is not a mount point
    umount2(sandboxPath.c_str(), MNT_DETACH);

    int fd = open("/dev/fuse", O_RDWR);
    APPSPAWN_CHECK(fd != -1, return -EINVAL, "open /dev/fuse failed, errno is %{public}d", errno);

    char options[OPTIONS_MAX_LEN];
    (void)sprintf_s(options, sizeof(options), "fd=%d,"
        "rootmode=40000,user_id=%u,group_id=%u,allow_other,"
        "context=\"u:object_r:dlp_fuse_file:s0\","
        "fscontext=u:object_r:dlp_fuse_file:s0",
        fd, dacInfo->uid, dacInfo->gid);

    // To make sure destinationPath exist
    MakeDirRecursive(sandboxPath, FILE_MODE);

    int ret = 0;
#ifndef APPSPAWN_TEST
    APPSPAWN_LOGV("Bind mount %{public}s to %{public}s '%{public}s' '%{public}lu' '%{public}s'",
        srcPath.c_str(), sandboxPath.c_str(), fsType.c_str(), mountFlags, options);
    ret = mount(srcPath.c_str(), sandboxPath.c_str(), fsType.c_str(), mountFlags, options);
    APPSPAWN_CHECK(ret == 0, close(fd);
        return ret, "DoDlpAppMountStrategy failed, bind mount %{public}s to %{public}s failed %{public}d",
        srcPath.c_str(), sandboxPath.c_str(), errno);

    ret = mount(nullptr, sandboxPath.c_str(), nullptr, MS_SHARED, nullptr);
    APPSPAWN_CHECK(ret == 0, close(fd);
        return ret, "errno is: %{public}d, private mount to %{public}s failed", errno, sandboxPath.c_str());
#endif
    /* set DLP_FUSE_FD  */
#ifdef WITH_DLP
    SetDlpFuseFd(fd);
#endif
    ret = fd;
    return ret;
}

static int32_t HandleSpecialAppMount(const AppSpawningCtx *appProperty,
    const std::string &srcPath, const std::string &sandboxPath, const std::string &fsType, unsigned long mountFlags)
{
    std::string bundleName = GetBundleName(appProperty);
    std::string processName = GetProcessName(appProperty);
    /* dlp application mount strategy */
    /* dlp is an example, we should change to real bundle name later */
    if (bundleName.find(g_dlpBundleName) != std::string::npos &&
        processName.compare(g_dlpBundleName) == 0) {
        if (fsType.empty()) {
            return -1;
        } else {
            return DoDlpAppMountStrategy(appProperty, srcPath, sandboxPath, fsType, mountFlags);
        }
    }
    return -1;
}

static uint32_t ConvertFlagStr(const std::string &flagStr)
{
    const std::map<std::string, int> flagsMap = {{"0", 0}, {"START_FLAGS_BACKUP", 1},
                                                 {"DLP_MANAGER", 2},
                                                 {"DEVELOPER_MODE", 17}};

    if (flagsMap.count(flagStr)) {
        return 1 << flagsMap.at(flagStr);
    }

    return 0;
}

unsigned long SandboxUtils::GetSandboxMountFlags(nlohmann::json &config)
{
    unsigned long mountFlags = BASIC_MOUNT_FLAGS;
    if (GetSandboxDacOverrideEnable(config) && (config.find(g_sandBoxFlagsCustomized) != config.end())) {
        mountFlags = GetMountFlagsFromConfig(config[g_sandBoxFlagsCustomized].get<std::vector<std::string>>());
    } else if (config.find(g_sandBoxFlags) != config.end()) {
        mountFlags = GetMountFlagsFromConfig(config[g_sandBoxFlags].get<std::vector<std::string>>());
    }
    return mountFlags;
}

std::string SandboxUtils::GetSandboxFsType(nlohmann::json &config)
{
    std::string fsType;
    if (GetSandboxDacOverrideEnable(config) && (config.find(g_fsType) != config.end())) {
        fsType = config[g_fsType].get<std::string>();
    } else {
        fsType = "";
    }
    return fsType;
}

std::string SandboxUtils::GetSandboxOptions(const AppSpawningCtx *appProperty, nlohmann::json &config)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return "";
    }

    std::string options;
    const int userIdBase = UID_BASE;
    if (GetSandboxDacOverrideEnable(config) && (config.find(g_sandBoxOptions) != config.end())) {
        options = config[g_sandBoxOptions].get<std::string>() + ",user_id=";
        options += std::to_string(dacInfo->uid / userIdBase);
    } else {
        options = "";
    }
    return options;
}

void SandboxUtils::GetSandboxMountConfig(const AppSpawningCtx *appProperty, const std::string &section,
                                         nlohmann::json &mntPoint, SandboxMountConfig &mountConfig)
{
    if (section.compare(g_permissionPrefix) == 0) {
        mountConfig.optionsPoint = GetSandboxOptions(appProperty, mntPoint);
        mountConfig.fsType = GetSandboxFsType(mntPoint);
    } else {
        mountConfig.fsType = (mntPoint.find(g_fsType) != mntPoint.end()) ? mntPoint[g_fsType].get<std::string>() : "";
        mountConfig.optionsPoint = "";
    }
    return;
}

std::string SandboxUtils::GetSandboxPath(const AppSpawningCtx *appProperty, nlohmann::json &mntPoint,
    const std::string &section, std::string sandboxRoot)
{
    std::string sandboxPath = "";
    std::string tmpSandboxPath = mntPoint[g_sandBoxPath].get<std::string>();
    if (section.compare(g_permissionPrefix) == 0) {
        sandboxPath = sandboxRoot + ConvertToRealPathWithPermission(appProperty, tmpSandboxPath);
    } else {
        sandboxPath = sandboxRoot + ConvertToRealPath(appProperty, tmpSandboxPath);
    }
    return sandboxPath;
}

static bool CheckMountFlag(const AppSpawningCtx *appProperty, const std::string bundleName, nlohmann::json &appConfig)
{
    if (appConfig.find(g_flags) != appConfig.end()) {
        if (((ConvertFlagStr(appConfig[g_flags].get<std::string>()) & GetAppMsgFlags(appProperty)) != 0) &&
            bundleName.find("wps") != std::string::npos) {
            return true;
        }
    }
    return false;
}

int SandboxUtils::DoAllMntPointsMount(const AppSpawningCtx *appProperty,
                                      nlohmann::json &appConfig, const char *typeName, const std::string &section)
{
    std::string bundleName = GetBundleName(appProperty);
    if (appConfig.find(g_mountPrefix) == appConfig.end()) {
        APPSPAWN_LOGV("mount config is not found in %{public}s, app name is %{public}s",
            section.c_str(), bundleName.c_str());
        return 0;
    }

    std::string sandboxRoot = GetSbxPathByConfig(appProperty, appConfig);
    bool checkFlag = CheckMountFlag(appProperty, bundleName, appConfig);

    nlohmann::json& mountPoints = appConfig[g_mountPrefix];
    unsigned int mountPointSize = mountPoints.size();
    for (unsigned int i = 0; i < mountPointSize; i++) {
        nlohmann::json& mntPoint = mountPoints[i];
        APPSPAWN_CHECK_ONLY_EXPER(CheckMountConfig(mntPoint, appProperty, checkFlag), continue);

        std::string srcPath = ConvertToRealPath(appProperty, mntPoint[g_srcPath].get<std::string>());
        std::string sandboxPath = GetSandboxPath(appProperty, mntPoint, section, sandboxRoot);
        SandboxMountConfig mountConfig = {0};
        GetSandboxMountConfig(appProperty, section, mntPoint, mountConfig);
        unsigned long mountFlags = GetSandboxMountFlags(mntPoint);
        mode_t mountSharedFlag = (mntPoint.find(g_mountSharedFlag) != mntPoint.end()) ? MS_SHARED : MS_SLAVE;

        /* if app mount failed for special strategy, we need deal with common mount config */
        int ret = HandleSpecialAppMount(appProperty, srcPath, sandboxPath, mountConfig.fsType, mountFlags);
        if (ret < 0) {
            ret = DoAppSandboxMountOnce(srcPath.c_str(), sandboxPath.c_str(), mountConfig.fsType.c_str(),
                                        mountFlags, mountConfig.optionsPoint.c_str(), mountSharedFlag);
        }
        if (ret) {
            std::string actionStatus = g_statusCheck;
            (void)JsonUtils::GetStringFromJson(mntPoint, g_actionStatuc, actionStatus);
            if (actionStatus == g_statusCheck) {
                APPSPAWN_LOGE("DoAppSandboxMountOnce section %{public}s failed, %{public}s",
                    section.c_str(), sandboxPath.c_str());
#ifdef APPSPAWN_HISYSEVENT
                ReportMountFail(bundleName.c_str(), srcPath.c_str(), sandboxPath.c_str(), errno);
                ret = APPSPAWN_SANDBOX_MOUNT_FAIL;
#endif
                return ret;
            }
        }

        DoSandboxChmod(mntPoint, sandboxRoot);
    }

    return 0;
}

int32_t SandboxUtils::DoAddGid(AppSpawningCtx *appProperty, nlohmann::json &appConfig,
                               const char* permissionName, const std::string &section)
{
    std::string bundleName = GetBundleName(appProperty);
    if (appConfig.find(g_gidPrefix) == appConfig.end()) {
        return 0;
    }
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return 0;
    }

    nlohmann::json& gids = appConfig[g_gidPrefix];
    unsigned int gidSize = gids.size();
    for (unsigned int i = 0; i < gidSize; i++) {
        if (dacInfo->gidCount < APP_MAX_GIDS) {
            APPSPAWN_LOGI("add gid to gitTable in %{public}s, permission is %{public}s, gid:%{public}u",
                bundleName.c_str(), permissionName, gids[i].get<uint32_t>());
            dacInfo->gidTable[dacInfo->gidCount++] = gids[i].get<uint32_t>();
        }
    }
    return 0;
}

int SandboxUtils::DoAllSymlinkPointslink(const AppSpawningCtx *appProperty, nlohmann::json &appConfig)
{
    APPSPAWN_CHECK(appConfig.find(g_symlinkPrefix) != appConfig.end(), return 0, "symlink config is not found,"
        "maybe result sandbox launch failed app name is %{public}s", GetBundleName(appProperty));

    nlohmann::json& symlinkPoints = appConfig[g_symlinkPrefix];
    std::string sandboxRoot = GetSbxPathByConfig(appProperty, appConfig);
    unsigned int symlinkPointSize = symlinkPoints.size();

    for (unsigned int i = 0; i < symlinkPointSize; i++) {
        nlohmann::json& symPoint = symlinkPoints[i];

        // Check the validity of the symlink configuration
        if (symPoint.find(g_targetName) == symPoint.end() || (!symPoint[g_targetName].is_string()) ||
            symPoint.find(g_linkName) == symPoint.end() || (!symPoint[g_linkName].is_string())) {
            APPSPAWN_LOGE("read symlink config failed, app name is %{public}s", GetBundleName(appProperty));
            continue;
        }

        std::string targetName = ConvertToRealPath(appProperty, symPoint[g_targetName].get<std::string>());
        std::string linkName = sandboxRoot + ConvertToRealPath(appProperty, symPoint[g_linkName].get<std::string>());
        APPSPAWN_LOGV("symlink, from %{public}s to %{public}s", targetName.c_str(), linkName.c_str());

        int ret = symlink(targetName.c_str(), linkName.c_str());
        if (ret && errno != EEXIST) {
            APPSPAWN_LOGE("errno is %{public}d, symlink failed, %{public}s", errno, linkName.c_str());

            std::string actionStatus = g_statusCheck;
            (void)JsonUtils::GetStringFromJson(symPoint, g_actionStatuc, actionStatus);
            if (actionStatus == g_statusCheck) {
                return ret;
            }
        }

        DoSandboxChmod(symPoint, sandboxRoot);
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxFilePrivateBind(const AppSpawningCtx *appProperty,
                                               nlohmann::json &wholeConfig)
{
    const char *bundleName = GetBundleName(appProperty);
    nlohmann::json& privateAppConfig = wholeConfig[g_privatePrefix][0];
    if (privateAppConfig.find(bundleName) != privateAppConfig.end()) {
        APPSPAWN_LOGV("DoSandboxFilePrivateBind %{public}s", bundleName);
        DoAddGid((AppSpawningCtx *)appProperty, privateAppConfig[bundleName][0], "", g_privatePrefix);
        return DoAllMntPointsMount(appProperty, privateAppConfig[bundleName][0], nullptr, g_privatePrefix);
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxFilePermissionBind(AppSpawningCtx *appProperty,
                                                  nlohmann::json &wholeConfig)
{
    if (wholeConfig.find(g_permissionPrefix) == wholeConfig.end()) {
        APPSPAWN_LOGV("DoSandboxFilePermissionBind not found permission information in config file");
        return 0;
    }
    nlohmann::json& permissionAppConfig = wholeConfig[g_permissionPrefix][0];
    for (nlohmann::json::iterator it = permissionAppConfig.begin(); it != permissionAppConfig.end(); ++it) {
        const std::string permission = it.key();
        int index = GetPermissionIndex(nullptr, permission.c_str());
        APPSPAWN_LOGV("DoSandboxFilePermissionBind mountPermissionFlags %{public}d", index);
        if (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(index))) {
            DoAddGid(appProperty, permissionAppConfig[permission][0], permission.c_str(), g_permissionPrefix);
            DoAllMntPointsMount(appProperty, permissionAppConfig[permission][0], permission.c_str(),
                                g_permissionPrefix);
        } else {
            APPSPAWN_LOGV("DoSandboxFilePermissionBind false %{public}s permission %{public}s",
                GetBundleName(appProperty), permission.c_str());
        }
    }
    return 0;
}

std::set<std::string> SandboxUtils::GetMountPermissionNames()
{
    std::set<std::string> permissionSet;
    for (auto& config : SandboxUtils::GetJsonConfig(SANBOX_APP_JSON_CONFIG)) {
        if (config.find(g_permissionPrefix) == config.end()) {
            continue;
        }
        nlohmann::json& permissionAppConfig = config[g_permissionPrefix][0];
        for (auto it = permissionAppConfig.begin(); it != permissionAppConfig.end(); it++) {
            permissionSet.insert(it.key());
        }
    }
    APPSPAWN_LOGI("GetMountPermissionNames size: %{public}lu", static_cast<unsigned long>(permissionSet.size()));
    return permissionSet;
}

int32_t SandboxUtils::DoSandboxFilePrivateSymlink(const AppSpawningCtx *appProperty,
                                                  nlohmann::json &wholeConfig)
{
    const char *bundleName = GetBundleName(appProperty);
    nlohmann::json& privateAppConfig = wholeConfig[g_privatePrefix][0];
    if (privateAppConfig.find(bundleName) != privateAppConfig.end()) {
        return DoAllSymlinkPointslink(appProperty, privateAppConfig[bundleName][0]);
    }

    return 0;
}

int32_t SandboxUtils::HandleFlagsPoint(const AppSpawningCtx *appProperty,
                                       nlohmann::json &appConfig)
{
    if (appConfig.find(g_flagePoint) == appConfig.end()) {
        return 0;
    }

    nlohmann::json& flagsPoints = appConfig[g_flagePoint];
    unsigned int flagsPointSize = flagsPoints.size();

    for (unsigned int i = 0; i < flagsPointSize; i++) {
        nlohmann::json& flagPoint = flagsPoints[i];

        if (flagPoint.find(g_flags) != flagPoint.end() && flagPoint[g_flags].is_string()) {
            std::string flagsStr = flagPoint[g_flags].get<std::string>();
            uint32_t flag = ConvertFlagStr(flagsStr);
            if ((GetAppMsgFlags(appProperty) & flag) != 0) {
                return DoAllMntPointsMount(appProperty, flagPoint, nullptr, g_flagePoint);
            }
        } else {
            APPSPAWN_LOGE("read flags config failed, app name is %{public}s", GetBundleName(appProperty));
        }
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(const AppSpawningCtx *appProperty,
                                                           nlohmann::json &wholeConfig)
{
    const char *bundleName = GetBundleName(appProperty);
    nlohmann::json& privateAppConfig = wholeConfig[g_privatePrefix][0];
    if (privateAppConfig.find(bundleName) != privateAppConfig.end()) {
        return HandleFlagsPoint(appProperty, privateAppConfig[bundleName][0]);
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxFileCommonFlagsPointHandle(const AppSpawningCtx *appProperty,
                                                          nlohmann::json &wholeConfig)
{
    nlohmann::json& commonConfig = wholeConfig[g_commonPrefix][0];
    if (commonConfig.find(g_appResources) != commonConfig.end()) {
        return HandleFlagsPoint(appProperty, commonConfig[g_appResources][0]);
    }

    return 0;
}

int32_t SandboxUtils::DoSandboxFileCommonBind(const AppSpawningCtx *appProperty, nlohmann::json &wholeConfig)
{
    nlohmann::json& commonConfig = wholeConfig[g_commonPrefix][0];
    int ret = 0;

    if (commonConfig.find(g_appBase) != commonConfig.end()) {
        ret = DoAllMntPointsMount(appProperty, commonConfig[g_appBase][0], nullptr, g_appBase);
        if (ret) {
            return ret;
        }
    }

    if (commonConfig.find(g_appResources) != commonConfig.end()) {
        ret = DoAllMntPointsMount(appProperty, commonConfig[g_appResources][0], nullptr, g_appResources);
    }

    return ret;
}

int32_t SandboxUtils::DoSandboxFileCommonSymlink(const AppSpawningCtx *appProperty,
                                                 nlohmann::json &wholeConfig)
{
    nlohmann::json& commonConfig = wholeConfig[g_commonPrefix][0];
    int ret = 0;

    if (commonConfig.find(g_appBase) != commonConfig.end()) {
        ret = DoAllSymlinkPointslink(appProperty, commonConfig[g_appBase][0]);
        if (ret) {
            return ret;
        }
    }

    if (commonConfig.find(g_appResources) != commonConfig.end()) {
        ret = DoAllSymlinkPointslink(appProperty, commonConfig[g_appResources][0]);
    }

    return ret;
}

int32_t SandboxUtils::SetPrivateAppSandboxProperty_(const AppSpawningCtx *appProperty,
                                                    nlohmann::json &config)
{
    int ret = DoSandboxFilePrivateBind(appProperty, config);
    APPSPAWN_CHECK(ret == 0, return ret, "DoSandboxFilePrivateBind failed");

    ret = DoSandboxFilePrivateSymlink(appProperty, config);
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "DoSandboxFilePrivateSymlink failed");

    ret = DoSandboxFilePrivateFlagsPointHandle(appProperty, config);
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "DoSandboxFilePrivateFlagsPointHandle failed");

    return ret;
}

int32_t SandboxUtils::SetPermissionAppSandboxProperty_(AppSpawningCtx *appProperty,
                                                       nlohmann::json &config)
{
    int ret = DoSandboxFilePermissionBind(appProperty, config);
    APPSPAWN_CHECK(ret == 0, return ret, "DoSandboxFilePermissionBind failed");
    return ret;
}


int32_t SandboxUtils::SetRenderSandboxProperty(const AppSpawningCtx *appProperty,
                                               std::string &sandboxPackagePath)
{
    return 0;
}

int32_t SandboxUtils::SetRenderSandboxPropertyNweb(const AppSpawningCtx *appProperty,
                                                   std::string &sandboxPackagePath)
{
    SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SANBOX_ISOLATED_JSON_CONFIG : SANBOX_APP_JSON_CONFIG;

    for (auto& config : SandboxUtils::GetJsonConfig(type)) {
        nlohmann::json& privateAppConfig = config[g_privatePrefix][0];
        if (privateAppConfig.find(g_ohosRender) != privateAppConfig.end()) {
            int ret = DoAllMntPointsMount(appProperty, privateAppConfig[g_ohosRender][0], nullptr, g_ohosRender);
            APPSPAWN_CHECK(ret == 0, return ret, "DoAllMntPointsMount failed, %{public}s",
                GetBundleName(appProperty));
            ret = DoAllSymlinkPointslink(appProperty, privateAppConfig[g_ohosRender][0]);
            APPSPAWN_CHECK(ret == 0, return ret, "DoAllSymlinkPointslink failed, %{public}s",
                GetBundleName(appProperty));
            ret = HandleFlagsPoint(appProperty, privateAppConfig[g_ohosRender][0]);
            APPSPAWN_CHECK_ONLY_LOG(ret == 0, "HandleFlagsPoint for render-sandbox failed, %{public}s",
                GetBundleName(appProperty));
        }
    }
    return 0;
}

int32_t SandboxUtils::SetPrivateAppSandboxProperty(const AppSpawningCtx *appProperty)
{
    int ret = 0;
    SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SANBOX_ISOLATED_JSON_CONFIG : SANBOX_APP_JSON_CONFIG;

    for (auto& config : SandboxUtils::GetJsonConfig(type)) {
        ret = SetPrivateAppSandboxProperty_(appProperty, config);
        APPSPAWN_CHECK(ret == 0, return ret, "parse adddata-sandbox config failed");
    }
    return ret;
}

static bool GetSandboxPrivateSharedStatus(const string &bundleName, AppSpawningCtx *appProperty)
{
    bool result = false;
    SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SANBOX_ISOLATED_JSON_CONFIG : SANBOX_APP_JSON_CONFIG;

    for (auto& config : SandboxUtils::GetJsonConfig(type)) {
        nlohmann::json& privateAppConfig = config[g_privatePrefix][0];
        if (privateAppConfig.find(bundleName) != privateAppConfig.end() &&
            privateAppConfig[bundleName][0].find(g_sandBoxShared) !=
            privateAppConfig[bundleName][0].end()) {
            string sandboxSharedStatus =
                privateAppConfig[bundleName][0][g_sandBoxShared].get<std::string>();
            if (sandboxSharedStatus == g_statusCheck) {
                result = true;
            }
        }
    }
    return result;
}

int32_t SandboxUtils::SetPermissionAppSandboxProperty(AppSpawningCtx *appProperty)
{
    int ret = 0;
    SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SANBOX_ISOLATED_JSON_CONFIG : SANBOX_APP_JSON_CONFIG;

    for (auto& config : SandboxUtils::GetJsonConfig(type)) {
        ret = SetPermissionAppSandboxProperty_(appProperty, config);
        APPSPAWN_CHECK(ret == 0, return ret, "parse adddata-sandbox config failed");
    }
    return ret;
}


int32_t SandboxUtils::SetCommonAppSandboxProperty_(const AppSpawningCtx *appProperty,
                                                   nlohmann::json &config)
{
    int rc = 0;

    rc = DoSandboxFileCommonBind(appProperty, config);
    APPSPAWN_CHECK(rc == 0, return rc, "DoSandboxFileCommonBind failed, %{public}s", GetBundleName(appProperty));

    // if sandbox switch is off, don't do symlink work again
    if (CheckAppSandboxSwitchStatus(appProperty) == true && (CheckTotalSandboxSwitchStatus(appProperty) == true)) {
        rc = DoSandboxFileCommonSymlink(appProperty, config);
        APPSPAWN_CHECK(rc == 0, return rc, "DoSandboxFileCommonSymlink failed, %{public}s", GetBundleName(appProperty));
    }

    rc = DoSandboxFileCommonFlagsPointHandle(appProperty, config);
    APPSPAWN_CHECK_ONLY_LOG(rc == 0, "DoSandboxFilePrivateFlagsPointHandle failed");

    return rc;
}

int32_t SandboxUtils::SetCommonAppSandboxProperty(const AppSpawningCtx *appProperty,
                                                  std::string &sandboxPackagePath)
{
    int ret = 0;
    SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SANBOX_ISOLATED_JSON_CONFIG : SANBOX_APP_JSON_CONFIG;

    for (auto& jsonConfig : SandboxUtils::GetJsonConfig(type)) {
        ret = SetCommonAppSandboxProperty_(appProperty, jsonConfig);
        APPSPAWN_CHECK(ret == 0, return ret,
            "parse appdata config for common failed, %{public}s", sandboxPackagePath.c_str());
    }

    ret = MountAllHsp(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(ret == 0, return ret, "mount extraInfo failed, %{public}s", sandboxPackagePath.c_str());

    ret = MountAllGroup(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(ret == 0, return ret, "mount groupList failed, %{public}s", sandboxPackagePath.c_str());

    AppSpawnMsgDomainInfo *info =
        reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(appProperty, TLV_DOMAIN_INFO));
    APPSPAWN_CHECK(info != nullptr, return -1, "No domain info %{public}s", sandboxPackagePath.c_str());
    if (strcmp(info->apl, APL_SYSTEM_BASIC.data()) == 0 ||
        strcmp(info->apl, APL_SYSTEM_CORE.data()) == 0 ||
        CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ACCESS_BUNDLE_DIR)) {
        // need permission check for system app here
        std::string destbundlesPath = sandboxPackagePath + g_dataBundles;
        DoAppSandboxMountOnce(g_physicalAppInstallPath.c_str(), destbundlesPath.c_str(), "", BASIC_MOUNT_FLAGS,
                              nullptr);
    }

    return 0;
}

static inline bool CheckPath(const std::string& name)
{
    return !name.empty() && name != "." && name != ".." && name.find("/") == std::string::npos;
}

std::string SandboxUtils::GetExtraInfoByType(const AppSpawningCtx *appProperty, const std::string &type)
{
    uint32_t len = 0;
    char *info = reinterpret_cast<char *>(GetAppPropertyExt(appProperty, type.c_str(), &len));
    if (info == nullptr) {
        return "";
    }
    return std::string(info, len);
}

int32_t SandboxUtils::MountAllHsp(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
    int ret = 0;
    string hspListInfo = GetExtraInfoByType(appProperty, HSPLIST_SOCKET_TYPE);
    if (hspListInfo.length() == 0) {
        return ret;
    }

    nlohmann::json hsps = nlohmann::json::parse(hspListInfo.c_str(), nullptr, false);
    APPSPAWN_CHECK(!hsps.is_discarded() && hsps.contains(g_hspList_key_bundles) && hsps.contains(g_hspList_key_modules)
        && hsps.contains(g_hspList_key_versions), return -1, "MountAllHsp: json parse failed");

    nlohmann::json& bundles = hsps[g_hspList_key_bundles];
    nlohmann::json& modules = hsps[g_hspList_key_modules];
    nlohmann::json& versions = hsps[g_hspList_key_versions];
    APPSPAWN_CHECK(bundles.is_array() && modules.is_array() && versions.is_array() && bundles.size() == modules.size()
        && bundles.size() == versions.size(), return -1, "MountAllHsp: value is not arrary or sizes are not same");

    APPSPAWN_LOGI("MountAllHsp: app = %{public}s, cnt = %{public}lu",
        GetBundleName(appProperty), static_cast<unsigned long>(bundles.size()));
    for (uint32_t i = 0; i < bundles.size(); i++) {
        // elements in json arrary can be different type
        APPSPAWN_CHECK(bundles[i].is_string() && modules[i].is_string() && versions[i].is_string(),
            return -1, "MountAllHsp: element type error");

        std::string libBundleName = bundles[i];
        std::string libModuleName = modules[i];
        std::string libVersion = versions[i];
        APPSPAWN_CHECK(CheckPath(libBundleName) && CheckPath(libModuleName) && CheckPath(libVersion),
            return -1, "MountAllHsp: path error");

        std::string libPhysicalPath = g_physicalAppInstallPath + libBundleName + "/" + libVersion + "/" + libModuleName;
        std::string mntPath =  sandboxPackagePath + g_sandboxHspInstallPath + libBundleName + "/" + libModuleName;
        ret = DoAppSandboxMountOnce(libPhysicalPath.c_str(), mntPath.c_str(), "", BASIC_MOUNT_FLAGS, nullptr);
        APPSPAWN_CHECK(ret == 0, return ret, "mount library failed %{public}d", ret);
    }
    return ret;
}

int32_t SandboxUtils::DoSandboxRootFolderCreateAdapt(std::string &sandboxPackagePath)
{
#ifndef APPSPAWN_TEST
    int rc = mount(nullptr, "/", nullptr, MS_REC | MS_SLAVE, nullptr);
    APPSPAWN_CHECK(rc == 0, return rc, "set propagation slave failed");
#endif
    MakeDirRecursive(sandboxPackagePath, FILE_MODE);

    // bind mount "/" to /mnt/sandbox/<currentUserId>/<packageName> path
    // rootfs: to do more resources bind mount here to get more strict resources constraints
#ifndef APPSPAWN_TEST
    rc = mount("/", sandboxPackagePath.c_str(), nullptr, BASIC_MOUNT_FLAGS, nullptr);
    APPSPAWN_CHECK(rc == 0, return rc, "mount bind / failed, %{public}d", errno);
#endif
    return 0;
}

int32_t SandboxUtils::MountAllGroup(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
    int ret = 0;
    string dataGroupInfo = GetExtraInfoByType(appProperty, DATA_GROUP_SOCKET_TYPE);
    if (dataGroupInfo.length() == 0) {
        return ret;
    }

    nlohmann::json groups = nlohmann::json::parse(dataGroupInfo.c_str(), nullptr, false);
    APPSPAWN_CHECK(!groups.is_discarded() && groups.contains(g_groupList_key_dataGroupId)
        && groups.contains(g_groupList_key_gid) && groups.contains(g_groupList_key_dir), return -1,
            "MountAllGroup: json parse failed");

    nlohmann::json& dataGroupIds = groups[g_groupList_key_dataGroupId];
    nlohmann::json& gids = groups[g_groupList_key_gid];
    nlohmann::json& dirs = groups[g_groupList_key_dir];
    APPSPAWN_CHECK(dataGroupIds.is_array() && gids.is_array() && dirs.is_array() && dataGroupIds.size() == gids.size()
        && dataGroupIds.size() == dirs.size(), return -1, "MountAllGroup: value is not arrary or sizes are not same");
    APPSPAWN_LOGI("MountAllGroup: app = %{public}s, cnt = %{public}lu",
        GetBundleName(appProperty), static_cast<unsigned long>(dataGroupIds.size()));
    for (uint32_t i = 0; i < dataGroupIds.size(); i++) {
        // elements in json arrary can be different type
        APPSPAWN_CHECK(dataGroupIds[i].is_string() && gids[i].is_string() && dirs[i].is_string(),
            return -1, "MountAllGroup: element type error");

        std::string srcPath = dirs[i];
        APPSPAWN_CHECK(!CheckPath(srcPath), return -1, "MountAllGroup: path error");

        size_t lastPathSplitPos = srcPath.find_last_of(g_fileSeparator);
        APPSPAWN_CHECK(lastPathSplitPos != std::string::npos, return -1, "MountAllGroup: path error");
        std::string dataGroupUuid = srcPath.substr(lastPathSplitPos + 1);

        uint32_t elxValue = GetElxInfoFromDir(srcPath.c_str());
        APPSPAWN_CHECK((elxValue >= EL2 && elxValue < ELX_MAX), return -1, "Get elx value failed");

        const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(elxValue);
        APPSPAWN_CHECK(templateItem != nullptr, return -1, "Get data group arg template failed");

        // If permission isn't null, need check permission flag
        if (templateItem->permission != nullptr) {
            int index = GetPermissionIndex(nullptr, templateItem->permission);
            APPSPAWN_LOGV("mount dir no lock mount permission flag %{public}d", index);
            if (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(index)) == 0) {
                continue;
            }
        }

        std::string mntPath = sandboxPackagePath + templateItem->sandboxPath + dataGroupUuid;
        mode_t mountFlags = MS_REC | MS_BIND;
        mode_t mountSharedFlag = MS_SLAVE;
        if (CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX)) {
            mountSharedFlag |= MS_REMOUNT | MS_NODEV | MS_RDONLY | MS_BIND;
        }
        ret = DoAppSandboxMountOnce(srcPath.c_str(), mntPath.c_str(), "", mountFlags, nullptr, mountSharedFlag);
        if (ret != 0) {
            APPSPAWN_LOGE("mount el%{public}d datagroup failed", elxValue);
        }
    }
    return 0;
}

int32_t SandboxUtils::DoSandboxRootFolderCreate(const AppSpawningCtx *appProperty,
                                                std::string &sandboxPackagePath)
{
#ifndef APPSPAWN_TEST
    int rc = mount(nullptr, "/", nullptr, MS_REC | MS_SLAVE, nullptr);
    if (rc) {
        return rc;
    }
#endif
    DoAppSandboxMountOnce(sandboxPackagePath.c_str(), sandboxPackagePath.c_str(), "",
                          BASIC_MOUNT_FLAGS, nullptr);

    return 0;
}

uint32_t SandboxUtils::GetSandboxNsFlags(bool isNweb)
{
    uint32_t nsFlags = 0;
    nlohmann::json appConfig;
    const std::map<std::string, uint32_t> NamespaceFlagsMap = { {"pid", CLONE_NEWPID},
                                                                {"net", CLONE_NEWNET} };

    if (!CheckTotalSandboxSwitchStatus(nullptr)) {
        return nsFlags;
    }

    for (auto& config : SandboxUtils::GetJsonConfig(SANBOX_APP_JSON_CONFIG)) {
        if (isNweb) {
            nlohmann::json& privateAppConfig = config[g_privatePrefix][0];
            if (privateAppConfig.find(g_ohosRender) == privateAppConfig.end()) {
                continue;
            }
            appConfig = privateAppConfig[g_ohosRender][0];
        } else {
            nlohmann::json& baseConfig = config[g_commonPrefix][0];
            if (baseConfig.find(g_appBase) == baseConfig.end()) {
                continue;
            }
            appConfig = baseConfig[g_appBase][0];
        }
        if (appConfig.find(g_sandBoxNsFlags) == appConfig.end()) {
            continue;
        }
        const auto vec = appConfig[g_sandBoxNsFlags].get<std::vector<std::string>>();
        for (unsigned int j = 0; j < vec.size(); j++) {
            if (NamespaceFlagsMap.count(vec[j])) {
                nsFlags |= NamespaceFlagsMap.at(vec[j]);
            }
        }
    }

    if (!nsFlags) {
        APPSPAWN_LOGE("config is not found %{public}s ns config", isNweb ? "Nweb" : "App");
    }
    return nsFlags;
}

bool SandboxUtils::CheckBundleNameForPrivate(const std::string &bundleName)
{
    if (bundleName.find(g_internal) != std::string::npos) {
        return false;
    }
    return true;
}

bool SandboxUtils::CheckTotalSandboxSwitchStatus(const AppSpawningCtx *appProperty)
{
    SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SANBOX_ISOLATED_JSON_CONFIG : SANBOX_APP_JSON_CONFIG;

    for (auto& wholeConfig : SandboxUtils::GetJsonConfig(type)) {
        if (wholeConfig.find(g_commonPrefix) == wholeConfig.end()) {
            continue;
        }
        nlohmann::json& commonAppConfig = wholeConfig[g_commonPrefix][0];
        if (commonAppConfig.find(g_topSandBoxSwitchPrefix) != commonAppConfig.end()) {
            std::string switchStatus = commonAppConfig[g_topSandBoxSwitchPrefix].get<std::string>();
            if (switchStatus == g_sbxSwitchCheck) {
                return true;
            } else {
                return false;
            }
        }
    }
    // default sandbox switch is on
    return true;
}

bool SandboxUtils::CheckAppSandboxSwitchStatus(const AppSpawningCtx *appProperty)
{
    bool rc = true;
    SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SANBOX_ISOLATED_JSON_CONFIG : SANBOX_APP_JSON_CONFIG;

    for (auto& wholeConfig : SandboxUtils::GetJsonConfig(type)) {
        if (wholeConfig.find(g_privatePrefix) == wholeConfig.end()) {
            continue;
        }
        nlohmann::json& privateAppConfig = wholeConfig[g_privatePrefix][0];
        if (privateAppConfig.find(GetBundleName(appProperty)) != privateAppConfig.end()) {
            nlohmann::json& appConfig = privateAppConfig[GetBundleName(appProperty)][0];
            rc = GetSbxSwitchStatusByConfig(appConfig);
            if (rc) {
                break;
            }
        }
    }
    // default sandbox switch is on
    return rc;
}

static int CheckBundleName(const std::string &bundleName)
{
    if (bundleName.empty() || bundleName.size() > APP_LEN_BUNDLE_NAME) {
        return -1;
    }
    if (bundleName.find('\\') != std::string::npos || bundleName.find('/') != std::string::npos) {
        return -1;
    }
    return 0;
}

int32_t SandboxUtils::SetOverlayAppSandboxProperty(const AppSpawningCtx *appProperty,
                                                   string &sandboxPackagePath)
{
    int ret = 0;
    if (!CheckAppMsgFlagsSet(appProperty, APP_FLAGS_OVERLAY)) {
        return ret;
    }

    string overlayInfo = GetExtraInfoByType(appProperty, OVERLAY_SOCKET_TYPE);
    set<string> mountedSrcSet;
    vector<string> splits = split(overlayInfo, g_overlayDecollator);
    string sandboxOverlayPath = sandboxPackagePath + g_overlayPath;
    for (auto hapPath : splits) {
        size_t pathIndex = hapPath.find_last_of(g_fileSeparator);
        if (pathIndex == string::npos) {
            continue;
        }
        std::string srcPath = hapPath.substr(0, pathIndex);
        if (mountedSrcSet.find(srcPath) != mountedSrcSet.end()) {
            APPSPAWN_LOGV("%{public}s have mounted before, no need to mount twice.", srcPath.c_str());
            continue;
        }

        auto bundleNameIndex = srcPath.find_last_of(g_fileSeparator);
        string destPath = sandboxOverlayPath + srcPath.substr(bundleNameIndex + 1, srcPath.length());
        int32_t retMount = DoAppSandboxMountOnce(srcPath.c_str(), destPath.c_str(),
                                                 nullptr, BASIC_MOUNT_FLAGS, nullptr);
        if (retMount != 0) {
            APPSPAWN_LOGE("fail to mount overlay path, src is %{public}s.", hapPath.c_str());
            ret = retMount;
        }

        mountedSrcSet.emplace(srcPath);
    }
    return ret;
}

int32_t SandboxUtils::SetBundleResourceAppSandboxProperty(const AppSpawningCtx *appProperty,
                                                          string &sandboxPackagePath)
{
    int ret = 0;
    if (!CheckAppMsgFlagsSet(appProperty, APP_FLAGS_BUNDLE_RESOURCES)) {
        return ret;
    }

    string srcPath = g_bundleResourceSrcPath;
    string destPath = sandboxPackagePath + g_bundleResourceDestPath;
    ret = DoAppSandboxMountOnce(
        srcPath.c_str(), destPath.c_str(), nullptr, BASIC_MOUNT_FLAGS, nullptr);
    return ret;
}

int32_t SandboxUtils::CheckAppFullMountEnable()
{
    if (deviceTypeEnable_ != -1) {
        return deviceTypeEnable_;
    }

    char value[] = "false";
    int32_t ret = GetParameter("const.filemanager.full_mount.enable", "false", value, sizeof(value));
    if (ret > 0 && (strcmp(value, "true")) == 0) {
        deviceTypeEnable_ = FILE_CROSS_APP_STATUS;
    } else if (ret > 0 && (strcmp(value, "false")) == 0) {
        deviceTypeEnable_ = FILE_ACCESS_COMMON_DIR_STATUS;
    } else {
        deviceTypeEnable_ = -1;
    }

    return deviceTypeEnable_;
}

int32_t SandboxUtils::SetSandboxProperty(AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
    int32_t ret = 0;
    const std::string bundleName = GetBundleName(appProperty);
    ret = SetCommonAppSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(ret == 0, return ret, "SetCommonAppSandboxProperty failed, packagename is %{public}s",
                   bundleName.c_str());
    if (CheckBundleNameForPrivate(bundleName)) {
        ret = SetPrivateAppSandboxProperty(appProperty);
        APPSPAWN_CHECK(ret == 0, return ret, "SetPrivateAppSandboxProperty failed, packagename is %{public}s",
                       bundleName.c_str());
    }
    ret = SetPermissionAppSandboxProperty(appProperty);
    APPSPAWN_CHECK(ret == 0, return ret, "SetPermissionAppSandboxProperty failed, packagename is %{public}s",
                   bundleName.c_str());

    ret = SetOverlayAppSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(ret == 0, return ret, "SetOverlayAppSandboxProperty failed, packagename is %{public}s",
                   bundleName.c_str());

    ret = SetBundleResourceAppSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(ret == 0, return ret, "SetBundleResourceAppSandboxProperty failed, packagename is %{public}s",
                   bundleName.c_str());
    APPSPAWN_LOGV("Set appsandbox property success");
    return ret;
}

int32_t SandboxUtils::ChangeCurrentDir(std::string &sandboxPackagePath, const std::string &bundleName,
                                       bool sandboxSharedStatus)
{
    int32_t ret = 0;
    ret = chdir(sandboxPackagePath.c_str());
    APPSPAWN_CHECK(ret == 0, return ret, "chdir failed, packagename is %{public}s, path is %{public}s",
        bundleName.c_str(), sandboxPackagePath.c_str());

    if (sandboxSharedStatus) {
        ret = chroot(sandboxPackagePath.c_str());
        APPSPAWN_CHECK(ret == 0, return ret, "chroot failed, path is %{public}s errno is %{public}d",
            sandboxPackagePath.c_str(), errno);
        return ret;
    }

    ret = syscall(SYS_pivot_root, sandboxPackagePath.c_str(), sandboxPackagePath.c_str());
    APPSPAWN_CHECK(ret == 0, return ret, "errno is %{public}d, pivot root failed, packagename is %{public}s",
        errno, bundleName.c_str());

    ret = umount2(".", MNT_DETACH);
    APPSPAWN_CHECK(ret == 0, return ret, "MNT_DETACH failed, packagename is %{public}s", bundleName.c_str());
    return ret;
}

static int EnableSandboxNamespace(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags)
{
#ifdef APPSPAWN_HISYSEVENT
    struct timespec startClock = {0};
    clock_gettime(CLOCK_MONOTONIC, &startClock);
#endif
    int rc = unshare(sandboxNsFlags);
#ifdef APPSPAWN_HISYSEVENT
    struct timespec endClock = {0};
    clock_gettime(CLOCK_MONOTONIC, &endClock);
    uint64_t diff = DiffTime(&startClock, &endClock);
    APPSPAWN_CHECK_ONLY_EXPER(diff < FUNC_REPORT_DURATION, ReportAbnormalDuration("unshare", diff));
#endif
    APPSPAWN_CHECK(rc == 0, return rc, "unshare failed, packagename is %{public}s", GetBundleName(appProperty));

    if ((sandboxNsFlags & CLONE_NEWNET) == CLONE_NEWNET) {
        rc = EnableNewNetNamespace();
        APPSPAWN_CHECK(rc == 0, return rc, "Set new netnamespace failed %{public}s", GetBundleName(appProperty));
    }
    return 0;
}

int32_t SandboxUtils::SetPermissionWithParam(AppSpawningCtx *appProperty)
{
    int32_t index = 0;
    int32_t appFullMountStatus = CheckAppFullMountEnable();
    if (appFullMountStatus == FILE_CROSS_APP_STATUS) {
        index = GetPermissionIndex(nullptr, FILE_CROSS_APP_MODE.c_str());
    } else if (appFullMountStatus == FILE_ACCESS_COMMON_DIR_STATUS) {
        index = GetPermissionIndex(nullptr, FILE_ACCESS_COMMON_DIR_MODE.c_str());
    }

    int32_t fileMgrIndex = GetPermissionIndex(nullptr, FILE_ACCESS_MANAGER_MODE.c_str());
    if (index > 0 && fileMgrIndex > 0 &&
        (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(fileMgrIndex)) == 0)) {
        return SetAppPermissionFlags(appProperty, index);
    }
    return -1;
}

int32_t SandboxUtils::SetAppSandboxProperty(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags)
{
    APPSPAWN_CHECK(appProperty != nullptr, return -1, "Invalid appspwn client");
    if (CheckBundleName(GetBundleName(appProperty)) != 0) {
        return -1;
    }
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return -1;
    }

    std::string sandboxPackagePath = g_sandBoxRootDir + to_string(dacInfo->uid / UID_BASE) + "/";
    const std::string bundleName = GetBundleName(appProperty);
    bool sandboxSharedStatus = GetSandboxPrivateSharedStatus(bundleName, appProperty) ||
        (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(GetPermissionIndex(nullptr,
        ACCESS_DLP_FILE_MODE.c_str()))) != 0);
    sandboxPackagePath += CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ? "isolated/" : "";
    sandboxPackagePath += bundleName;
    MakeDirRecursiveWithClock(sandboxPackagePath.c_str(), FILE_MODE);

    // add pid to a new mnt namespace
    int rc = EnableSandboxNamespace(appProperty, sandboxNsFlags);
    APPSPAWN_CHECK(rc == 0, return rc, "unshare failed, packagename is %{public}s", bundleName.c_str());
    if (SetPermissionWithParam(appProperty) != 0) {
        APPSPAWN_LOGW("Set app permission flag fail.");
    }
    // check app sandbox switch
    if ((CheckTotalSandboxSwitchStatus(appProperty) == false) ||
        (CheckAppSandboxSwitchStatus(appProperty) == false)) {
        rc = DoSandboxRootFolderCreateAdapt(sandboxPackagePath);
    } else if (!sandboxSharedStatus) {
        rc = DoSandboxRootFolderCreate(appProperty, sandboxPackagePath);
    }
    APPSPAWN_CHECK(rc == 0, return rc, "DoSandboxRootFolderCreate failed, %{public}s", bundleName.c_str());
    rc = SetSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(rc == 0, return rc, "SetSandboxProperty failed, %{public}s", bundleName.c_str());

#ifndef APPSPAWN_TEST
    rc = ChangeCurrentDir(sandboxPackagePath, bundleName, sandboxSharedStatus);
    APPSPAWN_CHECK(rc == 0, return rc, "change current dir failed");
    APPSPAWN_LOGV("Change root dir success");
#endif
    return 0;
}

int32_t SandboxUtils::SetAppSandboxPropertyNweb(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags)
{
    APPSPAWN_CHECK(appProperty != nullptr, return -1, "Invalid appspwn client");
    if (CheckBundleName(GetBundleName(appProperty)) != 0) {
        return -1;
    }
    std::string sandboxPackagePath = g_sandBoxRootDirNweb;
    const std::string bundleName = GetBundleName(appProperty);
    bool sandboxSharedStatus = GetSandboxPrivateSharedStatus(bundleName, appProperty);
    sandboxPackagePath += bundleName;
    MakeDirRecursiveWithClock(sandboxPackagePath.c_str(), FILE_MODE);

    // add pid to a new mnt namespace
    int rc = EnableSandboxNamespace(appProperty, sandboxNsFlags);
    APPSPAWN_CHECK(rc == 0, return rc, "unshare failed, packagename is %{public}s", bundleName.c_str());

    // check app sandbox switch
    if ((CheckTotalSandboxSwitchStatus(appProperty) == false) ||
        (CheckAppSandboxSwitchStatus(appProperty) == false)) {
        rc = DoSandboxRootFolderCreateAdapt(sandboxPackagePath);
    } else if (!sandboxSharedStatus) {
        rc = DoSandboxRootFolderCreate(appProperty, sandboxPackagePath);
    }
    APPSPAWN_CHECK(rc == 0, return rc, "DoSandboxRootFolderCreate failed, %{public}s", bundleName.c_str());
    // rendering process can be created by different apps,
    // and the bundle names of these apps are different,
    // so we can't use the method SetPrivateAppSandboxProperty
    // which mount dirs by using bundle name.
    rc = SetRenderSandboxPropertyNweb(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(rc == 0, return rc, "SetRenderSandboxPropertyNweb failed, packagename is %{public}s",
        sandboxPackagePath.c_str());

    rc = SetOverlayAppSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(rc == 0, return rc, "SetOverlayAppSandboxProperty failed, packagename is %{public}s",
        bundleName.c_str());

    rc = SetBundleResourceAppSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(rc == 0, return rc, "SetBundleResourceAppSandboxProperty failed, packagename is %{public}s",
        bundleName.c_str());

#ifndef APPSPAWN_TEST
    rc = chdir(sandboxPackagePath.c_str());
    APPSPAWN_CHECK(rc == 0, return rc, "chdir failed, packagename is %{public}s, path is %{public}s",
        bundleName.c_str(), sandboxPackagePath.c_str());

    if (sandboxSharedStatus) {
        rc = chroot(sandboxPackagePath.c_str());
        APPSPAWN_CHECK(rc == 0, return rc, "chroot failed, path is %{public}s errno is %{public}d",
            sandboxPackagePath.c_str(), errno);
        return 0;
    }

    rc = syscall(SYS_pivot_root, sandboxPackagePath.c_str(), sandboxPackagePath.c_str());
    APPSPAWN_CHECK(rc == 0, return rc, "errno is %{public}d, pivot root failed, packagename is %{public}s",
        errno, bundleName.c_str());

    rc = umount2(".", MNT_DETACH);
    APPSPAWN_CHECK(rc == 0, return rc, "MNT_DETACH failed, packagename is %{public}s", bundleName.c_str());
#endif
    return 0;
}
} // namespace AppSpawn
} // namespace OHOS

static bool AppSandboxPidNsIsSupport(void)
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

int LoadAppSandboxConfig(AppSpawnMgr *content)
{
    bool rc = true;
    // load sandbox config
    nlohmann::json appSandboxConfig;
    CfgFiles *files = GetCfgFiles("etc/sandbox");
    for (int i = 0; (files != nullptr) && (i < MAX_CFG_POLICY_DIRS_CNT); ++i) {
        if (files->paths[i] == nullptr) {
            continue;
        }
        std::string path = files->paths[i];
        std::string appPath = path + OHOS::AppSpawn::APP_JSON_CONFIG;
        APPSPAWN_LOGI("LoadAppSandboxConfig %{public}s", appPath.c_str());
        rc = OHOS::AppSpawn::JsonUtils::GetJsonObjFromJson(appSandboxConfig, appPath);
        APPSPAWN_CHECK(rc, continue, "Failed to load app data sandbox config %{public}s", appPath.c_str());
        OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(appSandboxConfig, SANBOX_APP_JSON_CONFIG);

        std::string isolatedPath = path + OHOS::AppSpawn::APP_ISOLATED_JSON_CONFIG;
        APPSPAWN_LOGI("LoadAppSandboxConfig %{public}s", isolatedPath.c_str());
        rc = OHOS::AppSpawn::JsonUtils::GetJsonObjFromJson(appSandboxConfig, isolatedPath);
        APPSPAWN_CHECK(rc, continue, "Failed to load app data sandbox config %{public}s", isolatedPath.c_str());
        OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(appSandboxConfig, SANBOX_ISOLATED_JSON_CONFIG);
    }
    FreeCfgFiles(files);
    bool isNweb = IsNWebSpawnMode(content);
    if (!isNweb && !AppSandboxPidNsIsSupport()) {
        return 0;
    }
    content->content.sandboxNsFlags = OHOS::AppSpawn::SandboxUtils::GetSandboxNsFlags(isNweb);
    return 0;
}

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
    if ((CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX) && !IsDeveloperModeOpen()) ||
        CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_NETWORK)) {
        sandboxNsFlags |= content->content.sandboxNsFlags & CLONE_NEWNET ? CLONE_NEWNET : 0;
    }
    APPSPAWN_LOGV("SetAppSandboxProperty sandboxNsFlags 0x%{public}x", sandboxNsFlags);
    StartAppspawnTrace("SetAppSandboxProperty");
    if (IsNWebSpawnMode(content)) {
        ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxPropertyNweb(property, sandboxNsFlags);
    } else {
        ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(property, sandboxNsFlags);
    }
    FinishAppspawnTrace();
    // for module test do not create sandbox, use APP_FLAGS_IGNORE_SANDBOX to ignore sandbox result
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_IGNORE_SANDBOX)) {
        APPSPAWN_LOGW("Do not care sandbox result %{public}d", ret);
        return 0;
    }
    return ret;
}

#define USER_ID_SIZE 16
#define DIR_MODE 0711

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

static void UmountDir(const char *rootPath, const char *targetPath, const AppSpawnedProcessInfo *appInfo)
{
    size_t allPathSize = strlen(rootPath) + USER_ID_SIZE + strlen(appInfo->name) + strlen(targetPath) + 2;
    char *path = reinterpret_cast<char *>(malloc(sizeof(char) * (allPathSize)));
    APPSPAWN_CHECK(path != NULL, return, "Failed to malloc path");

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
    APPSPAWN_CHECK(content != NULL && appInfo != NULL && appInfo->name != NULL,
        return -1, "Invalid content or appInfo");
    if (IsNWebSpawnMode(content)) {
        return 0;
    }
    APPSPAWN_LOGV("UmountSandboxPath name %{public}s pid %{public}d", appInfo->name, appInfo->pid);
    const char rootPath[] = "/mnt/sandbox/";
    const char el1Path[] = "/data/storage/el1/bundle";

    uint32_t userId = appInfo->uid / UID_BASE;
    std::string key = std::to_string(userId) + "-" + std::string(appInfo->name);
    map<string, int> *el1BundleCountMap = static_cast<std::map<std::string, int>*>(GetEl1BundleMountCount());
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
    (void)AddServerStageHook(STAGE_SERVER_PRELOAD, HOOK_PRIO_SANDBOX, LoadAppSandboxConfig);
    (void)AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_COMMON, SpawnMountDirToShared);
    (void)AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_SANDBOX, SetAppSandboxProperty);
    (void)AddProcessMgrHook(STAGE_SERVER_APP_UMOUNT, HOOK_PRIO_SANDBOX, UmountSandboxPath);
}
#endif
