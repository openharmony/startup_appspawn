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

#include "sandbox_core.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <set>
#include "securec.h"
#include "appspawn_manager.h"
#include "appspawn_trace.h"
#include "appspawn_utils.h"
#include "sandbox_dec.h"
#include "sandbox_def.h"
#ifdef APPSPAWN_HISYSEVENT
#include "hisysevent_adapter.h"
#endif

#ifdef WITH_DLP
#include "dlp_fuse_fd.h"
#endif

namespace OHOS {
namespace AppSpawn {

bool SandboxCore::NeedNetworkIsolated(AppSpawningCtx *property)
{
    int developerMode = IsDeveloperModeOpen();
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX) && !developerMode) {
        return true;
    }

    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_NETWORK)) {
        std::string extensionType = SandboxCommon::GetExtraInfoByType(property, MSG_EXT_NAME_EXTENSION_TYPE);
        if (extensionType.length() == 0 || !developerMode) {
            return true;
        }
    }

    return false;
}

int SandboxCore::EnableSandboxNamespace(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags)
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
    APPSPAWN_CHECK(rc == 0, return rc, "unshare %{public}s failed, err %{public}d", GetBundleName(appProperty), errno);

    if ((sandboxNsFlags & CLONE_NEWNET) == CLONE_NEWNET) {
        rc = EnableNewNetNamespace();
        APPSPAWN_CHECK(rc == 0, return rc, "Set %{public}s new netnamespace failed", GetBundleName(appProperty));
    }
    return 0;
}

uint32_t SandboxCore::GetAppMsgFlags(const AppSpawningCtx *property)
{
    APPSPAWN_CHECK(property != nullptr && property->message != nullptr,
        return 0, "Invalid property for name %{public}u", TLV_MSG_FLAGS);
    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppSpawnMsgInfo(property->message, TLV_MSG_FLAGS);
    APPSPAWN_CHECK(msgFlags != nullptr,
        return 0, "No TLV_MSG_FLAGS in msg %{public}s", property->message->msgHeader.processName);
    return msgFlags->flags[0];
}

bool SandboxCore::CheckMountFlag(const AppSpawningCtx *appProperty, const std::string bundleName,
                                 cJSON *appConfig)
{
    const char *flagChr = GetStringFromJsonObj(appConfig, SandboxCommonDef::g_flags);
    if (flagChr == nullptr) {
        return false;
    }
    std::string flagStr(flagChr);
    uint32_t flag = SandboxCommon::ConvertFlagStr(flagStr);
    if ((CheckAppMsgFlagsSet(appProperty, flag) != 0) &&
        bundleName.find("wps") != std::string::npos) {
        return true;
    }
    return false;
}

void SandboxCore::UpdateMsgFlagsWithPermission(AppSpawningCtx *appProperty, const std::string &permissionMode,
                                               uint32_t flag)
{
    int32_t processIndex = GetPermissionIndex(nullptr, permissionMode.c_str());
    if ((CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(processIndex)) == 0)) {
        APPSPAWN_LOGV("Don't need set %{public}s flag", permissionMode.c_str());
        return;
    }

    int ret = SetAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, flag);
    if (ret != 0) {
        APPSPAWN_LOGV("Set %{public}s flag failed", permissionMode.c_str());
    }
}

int32_t SandboxCore::UpdatePointFlags(AppSpawningCtx *appProperty)
{
    uint32_t index = 0;
#ifdef APPSPAWN_SUPPORT_NOSHAREFS
    index = APP_FLAGS_FILE_CROSS_APP;
#else
    index = APP_FLAGS_FILE_ACCESS_COMMON_DIR;
#endif
    int32_t fileMgrIndex = GetPermissionIndex(nullptr, SandboxCommonDef::FILE_ACCESS_MANAGER_MODE.c_str());
    if ((CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(fileMgrIndex)) == 0)) {
        return SetAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, index);
    }
    return 0;
}

std::string SandboxCore::GetSandboxPath(const AppSpawningCtx *appProperty, cJSON *mntPoint,
    const std::string &section, std::string sandboxRoot)
{
    std::string sandboxPath = "";
    const char *tmpSandboxPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_sandBoxPath);
    if (tmpSandboxPathChr == nullptr) {
        return "";
    }
    std::string tmpSandboxPath(tmpSandboxPathChr);
    if (section.compare(SandboxCommonDef::g_permissionPrefix) == 0) {
        sandboxPath = sandboxRoot + SandboxCommon::ConvertToRealPathWithPermission(appProperty, tmpSandboxPath);
    } else {
        sandboxPath = sandboxRoot + SandboxCommon::ConvertToRealPath(appProperty, tmpSandboxPath);
    }
    return sandboxPath;
}

int32_t SandboxCore::DoDlpAppMountStrategy(const AppSpawningCtx *appProperty, const std::string &srcPath,
        const std::string &sandboxPath, const std::string &fsType, unsigned long mountFlags)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return -1;
    }

    // umount fuse path, make sure that sandbox path is not a mount point
    umount2(sandboxPath.c_str(), MNT_DETACH);

    int fd = open("/dev/fuse", O_RDWR);
    APPSPAWN_CHECK(fd != -1, return -EINVAL, "open /dev/fuse failed, errno is %{public}d", errno);

    char options[SandboxCommonDef::OPTIONS_MAX_LEN];
    (void)sprintf_s(options, sizeof(options), "fd=%d,"
        "rootmode=40000,user_id=%u,group_id=%u,allow_other,"
        "context=\"u:object_r:dlp_fuse_file:s0\","
        "fscontext=u:object_r:dlp_fuse_file:s0",
        fd, dacInfo->uid, dacInfo->gid);

    // To make sure destinationPath exist
    (void)SandboxCommon::CreateDirRecursive(sandboxPath, SandboxCommonDef::FILE_MODE);

#ifndef APPSPAWN_TEST
    APPSPAWN_LOGV("Bind mount %{public}s to %{public}s '%{public}s' '%{public}lu' '%{public}s'",
        srcPath.c_str(), sandboxPath.c_str(), fsType.c_str(), mountFlags, options);
    int ret = mount(srcPath.c_str(), sandboxPath.c_str(), fsType.c_str(), mountFlags, options);
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
    return fd;
}

int32_t SandboxCore::HandleSpecialAppMount(const AppSpawningCtx *appProperty, const std::string &srcPath,
        const std::string &sandboxPath, const std::string &fsType, unsigned long mountFlags)
{
    std::string bundleName = GetBundleName(appProperty);
    std::string processName = GetProcessName(appProperty);
    /* dlp application mount strategy */
    /* dlp is an example, we should change to real bundle name later */
    if (bundleName.find(SandboxCommonDef::g_dlpBundleName) != std::string::npos &&
        processName.compare(SandboxCommonDef::g_dlpBundleName) == 0) {
        if (!fsType.empty()) {
            return DoDlpAppMountStrategy(appProperty, srcPath, sandboxPath, fsType, mountFlags);
        }
    }
    return -1;
}

cJSON *SandboxCore::GetPrivateJsonInfo(const AppSpawningCtx *appProperty, cJSON *wholeConfig)
{
    cJSON *firstPrivate = GetFirstCommonConfig(wholeConfig, SandboxCommonDef::g_privatePrefix);
    if (!firstPrivate) {
        return nullptr;
    }

    const char *bundleName = GetBundleName(appProperty);
    return GetFirstSubConfig(firstPrivate, bundleName);
}

int32_t SandboxCore::DoSandboxFilePrivateBind(const AppSpawningCtx *appProperty, cJSON *wholeConfig)
{
    cJSON *bundleNameInfo = GetPrivateJsonInfo(appProperty, wholeConfig);
    if (bundleNameInfo == nullptr) {
        return 0;
    }
    (void)DoAddGid((AppSpawningCtx *)appProperty, bundleNameInfo, "", SandboxCommonDef::g_privatePrefix);
    return DoAllMntPointsMount(appProperty, bundleNameInfo, nullptr, SandboxCommonDef::g_privatePrefix);
}

int32_t SandboxCore::DoSandboxFilePrivateSymlink(const AppSpawningCtx *appProperty, cJSON *wholeConfig)
{
    cJSON *bundleNameInfo = GetPrivateJsonInfo(appProperty, wholeConfig);
    if (bundleNameInfo == nullptr) {
        return 0;
    }
    return DoAllSymlinkPointslink(appProperty, bundleNameInfo);
}

int32_t SandboxCore::DoSandboxFilePrivateFlagsPointHandle(const AppSpawningCtx *appProperty, cJSON *wholeConfig)
{
    cJSON *bundleNameInfo = GetPrivateJsonInfo(appProperty, wholeConfig);
    if (bundleNameInfo == nullptr) {
        return 0;
    }
    return HandleFlagsPoint(appProperty, bundleNameInfo);
}

int32_t SandboxCore::SetPrivateAppSandboxProperty_(const AppSpawningCtx *appProperty, cJSON *config)
{
    int ret = DoSandboxFilePrivateBind(appProperty, config);
    APPSPAWN_CHECK(ret == 0, return ret, "DoSandboxFilePrivateBind failed");

    ret = DoSandboxFilePrivateSymlink(appProperty, config);
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "DoSandboxFilePrivateSymlink failed");

    ret = DoSandboxFilePrivateFlagsPointHandle(appProperty, config);
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "DoSandboxFilePrivateFlagsPointHandle failed");

    return ret;
}

int32_t SandboxCore::DoSandboxFilePermissionBind(AppSpawningCtx *appProperty, cJSON *wholeConfig)
{
    cJSON *permission = cJSON_GetObjectItemCaseSensitive(wholeConfig, SandboxCommonDef::g_permissionPrefix);
    if (!permission || !cJSON_IsArray(permission)) {
        return 0;
    }

    auto processor = [&appProperty](cJSON *item) {
        cJSON *permissionChild = item->child;
        while (permissionChild != nullptr) {
            int index = GetPermissionIndex(nullptr, permissionChild->string);
            if (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(index)) == 0) {
                permissionChild = permissionChild->next;
                continue;
            }
            cJSON *permissionMountPaths = cJSON_GetArrayItem(permissionChild, 0);
            if (!permissionMountPaths) {
                permissionChild = permissionChild->next;
                continue;
            }
            APPSPAWN_LOGV("DoSandboxFilePermissionBind %{public}s index %{public}d", permissionChild->string, index);
            DoAddGid(appProperty, permissionMountPaths, permissionChild->string, SandboxCommonDef::g_permissionPrefix);
            DoAllMntPointsMount(appProperty, permissionMountPaths, permissionChild->string,
                                SandboxCommonDef::g_permissionPrefix);

            permissionChild = permissionChild->next;
        }
        return 0;
    };

    return SandboxCommon::HandleArrayForeach(permission, processor);
}

int32_t SandboxCore::SetPermissionAppSandboxProperty_(AppSpawningCtx *appProperty, cJSON *config)
{
    int ret = DoSandboxFilePermissionBind(appProperty, config);
    APPSPAWN_CHECK(ret == 0, return ret, "DoSandboxFilePermissionBind failed");
    return ret;
}

cJSON *SandboxCore::GetFirstCommonConfig(cJSON *wholeConfig, const char *prefix)
{
    cJSON *commonConfig = cJSON_GetObjectItemCaseSensitive(wholeConfig, prefix);
    if (!commonConfig || !cJSON_IsArray(commonConfig)) {
        return nullptr;
    }

    return cJSON_GetArrayItem(commonConfig, 0);
}

cJSON *SandboxCore::GetFirstSubConfig(cJSON *parent, const char *key)
{
    cJSON *config = cJSON_GetObjectItemCaseSensitive(parent, key);
    if (!config || !cJSON_IsArray(config)) {
        return nullptr;
    }

    return cJSON_GetArrayItem(config, 0);
}

int32_t SandboxCore::DoSandboxFileCommonBind(const AppSpawningCtx *appProperty, cJSON *wholeConfig)
{
    cJSON *firstCommon = GetFirstCommonConfig(wholeConfig, SandboxCommonDef::g_commonPrefix);
    if (!firstCommon) {
        return 0;
    }

    cJSON *appBaseConfig = GetFirstSubConfig(firstCommon, SandboxCommonDef::g_appBase);
    if (!appBaseConfig) {
        return 0;
    }

    int ret = DoAllMntPointsMount(appProperty, appBaseConfig, nullptr, SandboxCommonDef::g_appBase);
    if (ret) {
        return ret;
    }

    cJSON *appResourcesConfig = GetFirstSubConfig(firstCommon, SandboxCommonDef::g_appResources);
    if (!appResourcesConfig) {
        return 0;
    }
    ret = DoAllMntPointsMount(appProperty, appResourcesConfig, nullptr, SandboxCommonDef::g_appResources);
    return ret;
}

int32_t SandboxCore::DoSandboxFileCommonSymlink(const AppSpawningCtx *appProperty, cJSON *wholeConfig)
{
    cJSON *firstCommon = GetFirstCommonConfig(wholeConfig, SandboxCommonDef::g_commonPrefix);
    if (!firstCommon) {
        return 0;
    }

    cJSON *appBaseConfig = GetFirstSubConfig(firstCommon, SandboxCommonDef::g_appBase);
    if (!appBaseConfig) {
        return 0;
    }

    int ret = DoAllSymlinkPointslink(appProperty, appBaseConfig);
    if (ret) {
        return ret;
    }

    cJSON *appResourcesConfig = GetFirstSubConfig(firstCommon, SandboxCommonDef::g_appResources);
    if (!appResourcesConfig) {
        return 0;
    }
    ret = DoAllSymlinkPointslink(appProperty, appResourcesConfig);
    return ret;
}

int32_t SandboxCore::DoSandboxFileCommonFlagsPointHandle(const AppSpawningCtx *appProperty, cJSON *wholeConfig)
{
    cJSON *firstCommon = GetFirstCommonConfig(wholeConfig, SandboxCommonDef::g_commonPrefix);
    if (!firstCommon) {
        return 0;
    }

    cJSON *appResourcesConfig = GetFirstSubConfig(firstCommon, SandboxCommonDef::g_appResources);
    if (!appResourcesConfig) {
        return 0;
    }
    return HandleFlagsPoint(appProperty, appResourcesConfig);
}

int32_t SandboxCore::SetCommonAppSandboxProperty_(const AppSpawningCtx *appProperty, cJSON *config)
{
    int rc = 0;
    rc = DoSandboxFileCommonBind(appProperty, config);
    APPSPAWN_CHECK(rc == 0, return rc, "DoSandboxFileCommonBind failed, %{public}s", GetBundleName(appProperty));

    // if sandbox switch is off, don't do symlink work again
    if (SandboxCommon::IsAppSandboxEnabled(appProperty) == true &&
        (SandboxCommon::IsTotalSandboxEnabled(appProperty) == true)) {
        rc = DoSandboxFileCommonSymlink(appProperty, config);
        APPSPAWN_CHECK(rc == 0, return rc, "DoSandboxFileCommonSymlink failed, %{public}s", GetBundleName(appProperty));
    }

    rc = DoSandboxFileCommonFlagsPointHandle(appProperty, config);
    APPSPAWN_CHECK_ONLY_LOG(rc == 0, "DoSandboxFilePrivateFlagsPointHandle failed");

    return rc;
}

static inline bool CheckPath(const std::string& name)
{
    return !name.empty() && name != "." && name != ".." && name.find("/") == std::string::npos;
}

static inline cJSON *GetJsonObjFromProperty(const AppSpawningCtx *appProperty, const char *name)
{
    uint32_t size = 0;
    const char *extInfo = (char *)(GetAppSpawnMsgExtInfo(appProperty->message, name, &size));
    if (size == 0 || extInfo == nullptr) {
        return nullptr;
    }
    APPSPAWN_LOGV("Get json name %{public}s value %{public}s", name, extInfo);
    cJSON *root = cJSON_Parse(extInfo);
    APPSPAWN_CHECK(root != nullptr, return nullptr, "Invalid ext info %{public}s for %{public}s", extInfo, name);
    return root;
}

int32_t SandboxCore::MountAllHsp(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath, cJSON *hspRoot)
{
    if (appProperty == nullptr || sandboxPackagePath == "") {
        return 0;
    }
    int ret = 0;
    APPSPAWN_CHECK_ONLY_EXPER(hspRoot != nullptr && cJSON_IsObject(hspRoot), return 0);

    cJSON *bundles = cJSON_GetObjectItemCaseSensitive(hspRoot, "bundles");
    cJSON *modules = cJSON_GetObjectItemCaseSensitive(hspRoot, "modules");
    cJSON *versions = cJSON_GetObjectItemCaseSensitive(hspRoot, "versions");
    APPSPAWN_CHECK(bundles != nullptr && cJSON_IsArray(bundles), return -1, "MountAllHsp: invalid bundles");
    APPSPAWN_CHECK(modules != nullptr && cJSON_IsArray(modules), return -1, "MountAllHsp: invalid modules");
    APPSPAWN_CHECK(versions != nullptr && cJSON_IsArray(versions), return -1, "MountAllHsp: invalid versions");
    int count = cJSON_GetArraySize(bundles);
    APPSPAWN_CHECK(count == cJSON_GetArraySize(modules), return -1, "MountAllHsp: sizes are not same");
    APPSPAWN_CHECK(count == cJSON_GetArraySize(versions), return -1, "MountAllHsp: sizes are not same");

    APPSPAWN_LOGI("MountAllHsp app: %{public}s, count: %{public}d", GetBundleName(appProperty), count);
    for (int i = 0; i < count; i++) {
        if (!(cJSON_IsString(cJSON_GetArrayItem(bundles, i)) && cJSON_IsString(cJSON_GetArrayItem(modules, i)) &&
            cJSON_IsString(cJSON_GetArrayItem(versions, i)))) {
            return -1;
        }
        const char *libBundleName = cJSON_GetStringValue(cJSON_GetArrayItem(bundles, i));
        const char *libModuleName = cJSON_GetStringValue(cJSON_GetArrayItem(modules, i));
        const char *libVersion = cJSON_GetStringValue(cJSON_GetArrayItem(versions, i));
        APPSPAWN_CHECK(libBundleName != nullptr && libModuleName != nullptr && libVersion != nullptr,
            return -1, "MountAllHsp: config error");
        APPSPAWN_CHECK(CheckPath(libBundleName) && CheckPath(libModuleName) && CheckPath(libVersion),
            return -1, "MountAllHsp: path error");

        std::string libPhysicalPath = SandboxCommonDef::g_physicalAppInstallPath + libBundleName + "/" +
                                      libVersion + "/" + libModuleName;
        std::string mntPath =
            sandboxPackagePath + SandboxCommonDef::g_sandboxHspInstallPath + libBundleName + "/" + libModuleName;
        SharedMountArgs arg = {
            .srcPath = libPhysicalPath.c_str(),
            .destPath = mntPath.c_str()
        };
        ret = SandboxCommon::DoAppSandboxMountOnce(appProperty, &arg);
        APPSPAWN_CHECK(ret == 0, return 0, "mount library failed %{public}d", ret);
    }
    return 0;
}

int32_t SandboxCore::MountAllGroup(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
    if (appProperty == nullptr || sandboxPackagePath == "") {
        return 0;
    }
    cJSON *dataGroupRoot = GetJsonObjFromProperty(appProperty, SandboxCommonDef::DATA_GROUP_SOCKET_TYPE.c_str());
    APPSPAWN_CHECK_ONLY_EXPER(dataGroupRoot != nullptr, return 0);

    mode_t mountSharedFlag = MS_SLAVE;
    if (CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX)) {
        mountSharedFlag |= MS_REMOUNT | MS_NODEV | MS_RDONLY | MS_BIND;
    }

    auto processor = [&appProperty, &sandboxPackagePath, &mountSharedFlag](cJSON *item) {
        APPSPAWN_CHECK(IsValidDataGroupItem(item), return -1, "MountAllGroup: data group item error");
        const char *srcPathChr = GetStringFromJsonObj(item, SandboxCommonDef::g_groupList_key_dir.c_str());
        if (srcPathChr == nullptr) {
            return 0;
        }
        std::string srcPath(srcPathChr);
        APPSPAWN_CHECK(!CheckPath(srcPath), return -1, "MountAllGroup: path error");
        const char *uuidChr = GetStringFromJsonObj(item, SandboxCommonDef::g_groupList_key_uuid.c_str());
        if (uuidChr == nullptr) {
            return 0;
        }
        std::string uuidStr(uuidChr);

        int elxValue = GetElxInfoFromDir(srcPath.c_str());
        APPSPAWN_CHECK((elxValue >= EL2 && elxValue < ELX_MAX), return -1, "Get elx value failed");

        const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(elxValue);
        APPSPAWN_CHECK(templateItem != nullptr, return -1, "Get data group arg template failed");

        // If permission isn't null, need check permission flag
        if (templateItem->permission != nullptr) {
            int index = GetPermissionIndex(nullptr, templateItem->permission);
            APPSPAWN_LOGV("mount dir no lock mount permission flag %{public}d", index);
            if (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(index)) == 0) {
                return 0;
            }
        }

        std::string mntPath = sandboxPackagePath + templateItem->sandboxPath + uuidStr;
        SharedMountArgs arg = {
            .srcPath = srcPath.c_str(),
            .destPath = mntPath.c_str(),
            .mountSharedFlag = mountSharedFlag
        };
        int result = SandboxCommon::DoAppSandboxMountOnce(appProperty, &arg);
        if (result != 0) {
            APPSPAWN_LOGE("mount el%{public}d datagroup failed", elxValue);
        }
        return 0;
    };
    int ret = SandboxCommon::HandleArrayForeach(dataGroupRoot, processor);
    cJSON_Delete(dataGroupRoot);
    return ret;
}

int32_t SandboxCore::ProcessMountPoint(cJSON *mntPoint, MountPointProcessParams &params)
{
    APPSPAWN_CHECK_ONLY_EXPER(SandboxCommon::IsValidMountConfig(mntPoint, params.appProperty, params.checkFlag),
                              return 0);
    const char *srcPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_srcPath);
    const char *sandboxPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_sandBoxPath);
    if (srcPathChr == nullptr || sandboxPathChr == nullptr) {
        return 0;
    }
    std::string srcPath(srcPathChr);
    std::string sandboxPath(sandboxPathChr);
    srcPath = SandboxCommon::ConvertToRealPath(params.appProperty, srcPath);
    APPSPAWN_CHECK_ONLY_EXPER(SandboxCommon::IsCreateSandboxPathEnabled(mntPoint, srcPath), return 0);
    sandboxPath = GetSandboxPath(params.appProperty, mntPoint, params.section, params.sandboxRoot);
    SandboxMountConfig mountConfig = {0};
    SandboxCommon::GetSandboxMountConfig(params.appProperty, params.section, mntPoint, mountConfig);
    SharedMountArgs arg = {
        .srcPath = srcPath.c_str(),
        .destPath = sandboxPath.c_str(),
        .fsType = mountConfig.fsType.c_str(),
        .mountFlags = SandboxCommon::GetMountFlags(mntPoint),
        .options = mountConfig.optionsPoint.c_str(),
        .mountSharedFlag =
            GetBoolValueFromJsonObj(mntPoint, SandboxCommonDef::g_mountSharedFlag, false) ? MS_SHARED : MS_SLAVE
    };

    /* if app mount failed for special strategy, we need deal with common mount config */
    int ret = HandleSpecialAppMount(params.appProperty, arg.srcPath, arg.destPath, arg.fsType, arg.mountFlags);
    if (ret < 0) {
        ret = SandboxCommon::DoAppSandboxMountOnce(params.appProperty, &arg);
    }
    APPSPAWN_CHECK(ret == 0 || !SandboxCommon::IsMountSuccessful(mntPoint),
#ifdef APPSPAWN_HISYSEVENT
        ReportMountFail(params.bundleName.c_str(), arg.srcPath, arg.destPath, errno);
        ret = APPSPAWN_SANDBOX_MOUNT_FAIL;
#endif
        return ret,
        "DoAppSandboxMountOnce section %{public}s failed, %{public}s", params.section.c_str(), arg.destPath);
    SetDecPolicyWithPermission(params.appProperty, mountConfig);
    SandboxCommon::SetSandboxPathChmod(mntPoint, params.sandboxRoot);
    return 0;
}

int32_t SandboxCore::DoAllMntPointsMount(const AppSpawningCtx *appProperty, cJSON *appConfig,
                                         const char *typeName, const std::string &section)
{
    std::string bundleName = GetBundleName(appProperty);
    cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(appConfig, SandboxCommonDef::g_mountPrefix);
    if (mountPoints == nullptr || !cJSON_IsArray(mountPoints)) {
        APPSPAWN_LOGI("mount config is not found in %{public}s, app name is %{public}s",
            section.c_str(), bundleName.c_str());
        return 0;
    }

    std::string sandboxRoot = SandboxCommon::GetSandboxRootPath(appProperty, appConfig);
    bool checkFlag = CheckMountFlag(appProperty, bundleName, appConfig);
    MountPointProcessParams mountPointParams = {
        .appProperty = appProperty,
        .checkFlag = checkFlag,
        .section = section,
        .sandboxRoot = sandboxRoot,
        .bundleName = bundleName
    };

    auto processor = [&mountPointParams](cJSON *mntPoint) {
        return ProcessMountPoint(mntPoint, mountPointParams);
    };

    return SandboxCommon::HandleArrayForeach(mountPoints, processor);
}

int32_t SandboxCore::DoAddGid(AppSpawningCtx *appProperty, cJSON *appConfig,
                              const char *permissionName, const std::string &section)
{
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (dacInfo == nullptr) {
        return 0;
    }

    cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(appConfig, SandboxCommonDef::g_gidPrefix);
    if (mountPoints == nullptr || !cJSON_IsArray(mountPoints)) {
        return 0;
    }

    std::string bundleName = GetBundleName(appProperty);
    auto processor = [&dacInfo](cJSON *item) {
        gid_t gid = 0;
        if (cJSON_IsNumber(item)) {
            gid = (gid_t)cJSON_GetNumberValue(item);
        }
        if (gid <= 0) {
            return 0;
        }
        if (dacInfo->gidCount < APP_MAX_GIDS) {
            dacInfo->gidTable[dacInfo->gidCount++] = gid;
        }
        return 0;
    };

    return SandboxCommon::HandleArrayForeach(mountPoints, processor);
}

int32_t SandboxCore::DoAllSymlinkPointslink(const AppSpawningCtx *appProperty, cJSON *appConfig)
{
    cJSON *symlinkPoints = cJSON_GetObjectItemCaseSensitive(appConfig, SandboxCommonDef::g_symlinkPrefix);
    if (symlinkPoints == nullptr || !cJSON_IsArray(symlinkPoints)) {
        APPSPAWN_LOGV("symlink config is not found");
        return 0;
    }

    std::string sandboxRoot = SandboxCommon::GetSandboxRootPath(appProperty, appConfig);
    auto processor = [&appProperty, &sandboxRoot](cJSON *item) {
        const char *targetNameChr = GetStringFromJsonObj(item, SandboxCommonDef::g_targetName);
        const char *linkNameChr = GetStringFromJsonObj(item, SandboxCommonDef::g_linkName);
        if (targetNameChr == nullptr || linkNameChr == nullptr) {
            return 0;
        }
        std::string targetName(targetNameChr);
        std::string linkName(linkNameChr);
        targetName = SandboxCommon::ConvertToRealPath(appProperty, targetName);
        linkName = sandboxRoot + SandboxCommon::ConvertToRealPath(appProperty, linkName);
        int ret = symlink(targetName.c_str(), linkName.c_str());
        if (ret && errno != EEXIST && SandboxCommon::IsMountSuccessful(item)) {
            APPSPAWN_LOGE("errno is %{public}d, symlink failed, %{public}s", errno, linkName.c_str());
            return -1;
        }
        SandboxCommon::SetSandboxPathChmod(item, sandboxRoot);
        return 0;
    };

    return SandboxCommon::HandleArrayForeach(symlinkPoints, processor);
}

int32_t SandboxCore::DoSandboxRootFolderCreateAdapt(std::string &sandboxPackagePath)
{
#ifndef APPSPAWN_TEST
    int rc = mount(nullptr, "/", nullptr, MS_REC | MS_SLAVE, nullptr);
    APPSPAWN_CHECK(rc == 0, return rc, "set propagation slave failed");
#endif
    (void)SandboxCommon::CreateDirRecursive(sandboxPackagePath, SandboxCommonDef::FILE_MODE);

    // bind mount "/" to /mnt/sandbox/<currentUserId>/<packageName> path
    // rootfs: to do more resources bind mount here to get more strict resources constraints
#ifndef APPSPAWN_TEST
    rc = mount("/", sandboxPackagePath.c_str(), nullptr, SandboxCommonDef::BASIC_MOUNT_FLAGS, nullptr);
    APPSPAWN_CHECK(rc == 0, return rc, "mount bind / failed, %{public}d", errno);
#endif
    return 0;
}

int32_t SandboxCore::DoSandboxRootFolderCreate(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
#ifndef APPSPAWN_TEST
    int rc = mount(nullptr, "/", nullptr, MS_REC | MS_SLAVE, nullptr);
    if (rc) {
        return rc;
    }
#endif
    SharedMountArgs arg = {
        .srcPath = sandboxPackagePath.c_str(),
        .destPath = sandboxPackagePath.c_str()
    };
    SandboxCommon::DoAppSandboxMountOnce(appProperty, &arg);

    return 0;
}

void SandboxCore::GetSpecialMountCondition(bool &isPreInstalled, bool &isHaveSandBoxPermission,
                                           const AppSpawningCtx *appProperty)
{
    const std::string preInstallFlag = "PREINSTALLED_HAP";
    const std::string customSandBoxFlag = "CUSTOM_SANDBOX_HAP";
    isPreInstalled = CheckAppMsgFlagsSet(appProperty, SandboxCommon::ConvertFlagStr(preInstallFlag)) != 0;
    isHaveSandBoxPermission = CheckAppMsgFlagsSet(appProperty, SandboxCommon::ConvertFlagStr(customSandBoxFlag)) != 0;
}

int32_t SandboxCore::MountNonShellPreInstallHap(const AppSpawningCtx *appProperty, cJSON *item)
{
    bool isPreInstalled = false;
    bool isHaveSandBoxPermission = false;
    GetSpecialMountCondition(isPreInstalled, isHaveSandBoxPermission, appProperty);
    bool preInstallMount = (isPreInstalled && !isHaveSandBoxPermission);
    if (preInstallMount) {
        return DoAllMntPointsMount(appProperty, item, nullptr, SandboxCommonDef::g_flagePoint);
    }
    return 0;
}

int32_t SandboxCore::MountShellPreInstallHap(const AppSpawningCtx *appProperty, cJSON *item)
{
    bool isPreInstalled = false;
    bool isHaveSandBoxPermission = false;
    GetSpecialMountCondition(isPreInstalled, isHaveSandBoxPermission, appProperty);
    bool preInstallShellMount = (isPreInstalled && isHaveSandBoxPermission);
    if (preInstallShellMount) {
        return DoAllMntPointsMount(appProperty, item, nullptr, SandboxCommonDef::g_flagePoint);
    }
    return 0;
}

int32_t SandboxCore::HandleFlagsPoint(const AppSpawningCtx *appProperty, cJSON *appConfig)
{
    cJSON *flagsPoints = cJSON_GetObjectItemCaseSensitive(appConfig, SandboxCommonDef::g_flagePoint);
    if (flagsPoints == nullptr || !cJSON_IsArray(flagsPoints)) {
        APPSPAWN_LOGV("flag points config is not found");
        return 0;
    }

    auto processor = [&appProperty](cJSON *item) {
        const char *flagsChr = GetStringFromJsonObj(item, SandboxCommonDef::g_flags);
        if (flagsChr == nullptr) {
            return 0;
        }
        std::string flagsStr(flagsChr);

        const std::string preInstallFlag = "PREINSTALLED_HAP";
        const std::string preInstallShellFlag = "PREINSTALLED_SHELL_HAP";

        if (flagsStr == preInstallFlag) {
            return MountNonShellPreInstallHap(appProperty, item);
        }

        if (flagsStr == preInstallShellFlag) {
            return MountShellPreInstallHap(appProperty, item);
        }

        uint32_t flag = SandboxCommon::ConvertFlagStr(flagsStr);
        if (CheckAppMsgFlagsSet(appProperty, flag) == 0) {
            return 0;
        }
        return DoAllMntPointsMount(appProperty, item, nullptr, SandboxCommonDef::g_flagePoint);
    };
    return SandboxCommon::HandleArrayForeach(flagsPoints, processor);
}

int32_t SandboxCore::SetOverlayAppSandboxProperty(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
    int ret = 0;
    if (!CheckAppMsgFlagsSet(appProperty, APP_FLAGS_OVERLAY)) {
        return ret;
    }

    std::string overlayInfo = SandboxCommon::GetExtraInfoByType(appProperty, SandboxCommonDef::OVERLAY_SOCKET_TYPE);
    std::set<std::string> mountedSrcSet;
    std::vector<std::string> splits = SandboxCommon::SplitString(overlayInfo, SandboxCommonDef::g_overlayDecollator);
    std::string sandboxOverlayPath = sandboxPackagePath + SandboxCommonDef::g_overlayPath;
    for (auto hapPath : splits) {
        size_t pathIndex = hapPath.find_last_of(SandboxCommonDef::g_fileSeparator);
        if (pathIndex == std::string::npos) {
            continue;
        }
        std::string srcPath = hapPath.substr(0, pathIndex);
        if (mountedSrcSet.find(srcPath) != mountedSrcSet.end()) {
            APPSPAWN_LOGV("%{public}s have mounted before, no need to mount twice.", srcPath.c_str());
            continue;
        }

        auto bundleNameIndex = srcPath.find_last_of(SandboxCommonDef::g_fileSeparator);
        std::string destPath = sandboxOverlayPath + srcPath.substr(bundleNameIndex + 1, srcPath.length());
        SharedMountArgs arg = {
            .srcPath = srcPath.c_str(),
            .destPath = destPath.c_str()
        };
        int32_t retMount = SandboxCommon::DoAppSandboxMountOnce(appProperty, &arg);
        if (retMount != 0) {
            APPSPAWN_LOGE("fail to mount overlay path, src is %{public}s.", hapPath.c_str());
            ret = retMount;
        }

        mountedSrcSet.emplace(srcPath);
    }
    return ret;
}

int32_t SandboxCore::SetBundleResourceAppSandboxProperty(const AppSpawningCtx *appProperty,
                                                         std::string &sandboxPackagePath)
{
    if (!CheckAppMsgFlagsSet(appProperty, APP_FLAGS_BUNDLE_RESOURCES)) {
        return 0;
    }

    std::string destPath = sandboxPackagePath + SandboxCommonDef::g_bundleResourceDestPath;
    SharedMountArgs arg = {
        .srcPath = SandboxCommonDef::g_bundleResourceSrcPath.c_str(),
        .destPath = destPath.c_str()
    };
    return SandboxCommon::DoAppSandboxMountOnce(appProperty, &arg);
}

int32_t SandboxCore::SetCommonAppSandboxProperty(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
    int ret = 0;
    SandboxCommonDef::SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;

    for (auto& jsonConfig : SandboxCommon::GetCJsonConfig(type)) {
        ret = SetCommonAppSandboxProperty_(appProperty, jsonConfig);
        APPSPAWN_CHECK(ret == 0, return ret,
            "parse appdata config for common failed, %{public}s", sandboxPackagePath.c_str());
    }
    cJSON *hspRoot = GetJsonObjFromProperty(appProperty, SandboxCommonDef::HSPLIST_SOCKET_TYPE.c_str());
    ret = MountAllHsp(appProperty, sandboxPackagePath, hspRoot);
    cJSON_Delete(hspRoot);
    APPSPAWN_CHECK(ret == 0, return ret, "mount extraInfo failed, %{public}s", sandboxPackagePath.c_str());

    ret = MountAllGroup(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(ret == 0, return ret, "mount groupList failed, %{public}s", sandboxPackagePath.c_str());

    AppSpawnMsgDomainInfo *info =
        reinterpret_cast<AppSpawnMsgDomainInfo *>(GetAppProperty(appProperty, TLV_DOMAIN_INFO));
    APPSPAWN_CHECK(info != nullptr, return -1, "No domain info %{public}s", sandboxPackagePath.c_str());
    if (strcmp(info->apl, SandboxCommonDef::APL_SYSTEM_BASIC.data()) == 0 ||
        strcmp(info->apl, SandboxCommonDef::APL_SYSTEM_CORE.data()) == 0 ||
        CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ACCESS_BUNDLE_DIR)) {
        // need permission check for system app here
        std::string destbundlesPath = sandboxPackagePath + SandboxCommonDef::g_dataBundles;
        SharedMountArgs arg = {
            .srcPath = SandboxCommonDef::g_physicalAppInstallPath.c_str(),
            .destPath = destbundlesPath.c_str()
        };
        SandboxCommon::DoAppSandboxMountOnce(appProperty, &arg);
    }

    return 0;
}

int32_t SandboxCore::SetPrivateAppSandboxProperty(const AppSpawningCtx *appProperty)
{
    int ret = 0;
    SandboxCommonDef::SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;

    for (auto& config : SandboxCommon::GetCJsonConfig(type)) {
        ret = SetPrivateAppSandboxProperty_(appProperty, config);
        APPSPAWN_CHECK(ret == 0, return ret, "parse adddata-sandbox config failed");
    }
    return ret;
}

int32_t SandboxCore::SetPermissionAppSandboxProperty(AppSpawningCtx *appProperty)
{
    int ret = 0;
    SandboxCommonDef::SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;

    for (auto& config : SandboxCommon::GetCJsonConfig(type)) {
        ret = SetPermissionAppSandboxProperty_(appProperty, config);
        APPSPAWN_CHECK(ret == 0, return ret, "parse permission config failed");
    }
    return ret;
}

int32_t SandboxCore::SetSandboxProperty(AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
    int32_t ret = 0;
    const std::string bundleName = GetBundleName(appProperty);
    StartAppspawnTrace("SetCommonAppSandboxProperty");
    ret = SetCommonAppSandboxProperty(appProperty, sandboxPackagePath);
    FinishAppspawnTrace();
    APPSPAWN_CHECK(ret == 0, return ret, "SetCommonAppSandboxProperty failed, packagename is %{public}s",
                   bundleName.c_str());
    if (SandboxCommon::HasPrivateInBundleName(bundleName)) {
        ret = SetPrivateAppSandboxProperty(appProperty);
        APPSPAWN_CHECK(ret == 0, return ret, "SetPrivateAppSandboxProperty failed, packagename is %{public}s",
                       bundleName.c_str());
    }
    StartAppspawnTrace("SetPermissionAppSandboxProperty");
    ret = SetPermissionAppSandboxProperty(appProperty);
    FinishAppspawnTrace();
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

int32_t SandboxCore::SetAppSandboxProperty(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags)
{
    APPSPAWN_CHECK(appProperty != nullptr, return -1, "Invalid appspawn client");
    if (SandboxCommon::CheckBundleName(GetBundleName(appProperty)) != 0) {
        return -1;
    }
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    APPSPAWN_CHECK(dacInfo != nullptr, return -1, "No dac info in msg app property");

    const std::string bundleName = GetBundleName(appProperty);
    cJSON *tmpJson = nullptr;
    std::string sandboxPackagePath = SandboxCommon::GetSandboxRootPath(appProperty, tmpJson);
    SandboxCommon::CreateDirRecursiveWithClock(sandboxPackagePath.c_str(), SandboxCommonDef::FILE_MODE);
    bool sandboxSharedStatus = SandboxCommon::IsPrivateSharedStatus(bundleName, appProperty) ||
        (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(GetPermissionIndex(nullptr,
        SandboxCommonDef::ACCESS_DLP_FILE_MODE.c_str()))) != 0);

    // add pid to a new mnt namespace
    StartAppspawnTrace("EnableSandboxNamespace");
    int rc = EnableSandboxNamespace(appProperty, sandboxNsFlags);
    FinishAppspawnTrace();
    APPSPAWN_CHECK(rc == 0, return rc, "unshare failed, packagename is %{public}s", bundleName.c_str());
    APPSPAWN_CHECK(UpdatePointFlags(appProperty) == 0, return -1, "Set app permission flag fail.");

    UpdateMsgFlagsWithPermission(appProperty, SandboxCommonDef::GET_ALL_PROCESSES_MODE, APP_FLAGS_GET_ALL_PROCESSES);
    UpdateMsgFlagsWithPermission(appProperty, SandboxCommonDef::APP_ALLOW_IOURING, APP_FLAGS_ALLOW_IOURING);
    // check app sandbox switch
    if (!SandboxCommon::IsTotalSandboxEnabled(appProperty) || !SandboxCommon::IsAppSandboxEnabled(appProperty)) {
        rc = DoSandboxRootFolderCreateAdapt(sandboxPackagePath);
    } else if (!sandboxSharedStatus) {
        rc = DoSandboxRootFolderCreate(appProperty, sandboxPackagePath);
    }
    APPSPAWN_CHECK(rc == 0, return rc, "DoSandboxRootFolderCreate failed, %{public}s", bundleName.c_str());
    rc = SetSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(rc == 0, return rc, "SetSandboxProperty failed, %{public}s", bundleName.c_str());

#ifdef APPSPAWN_MOUNT_TMPSHM
    MountDevShmPath(sandboxPackagePath);
#endif

#ifndef APPSPAWN_TEST
    StartAppspawnTrace("ChangeCurrentDir");
    rc = ChangeCurrentDir(sandboxPackagePath, bundleName, sandboxSharedStatus);
    FinishAppspawnTrace();
    APPSPAWN_CHECK(rc == 0, return rc, "change current dir failed");
    APPSPAWN_LOGV("Change root dir success");
#endif
    SetDecWithDir(appProperty, dacInfo->uid / UID_BASE);
    SetDecDenyWithDir(appProperty);
    SetDecPolicy();
#if defined(APPSPAWN_MOUNT_TMPSHM) && defined(WITH_SELINUX)
    Restorecon(DEV_SHM_DIR);
#endif
    return 0;
}

int32_t SandboxCore::SetRenderSandboxPropertyNweb(const AppSpawningCtx *appProperty, std::string &sandboxPackagePath)
{
    char *processType = (char *)(GetAppSpawnMsgExtInfo(appProperty->message, MSG_EXT_NAME_PROCESS_TYPE, nullptr));
    APPSPAWN_CHECK(processType != nullptr, return -1, "Invalid processType data");
    SandboxCommonDef::SandboxConfigType type = CheckAppMsgFlagsSet(appProperty, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;

    int ret = 0;
    const char *bundleName = GetBundleName(appProperty);
    for (auto& config : SandboxCommon::GetCJsonConfig(type)) {
        cJSON *firstIndividual = GetFirstCommonConfig(config, SandboxCommonDef::g_privatePrefix);
        if (!firstIndividual) {
            continue;
        }
        if (strcmp(processType, "render") == 0) {
            cJSON *renderInfo = GetFirstSubConfig(firstIndividual, SandboxCommonDef::g_ohosRender.c_str());
            if (!renderInfo) {
                continue;
            }
            ret = DoAllMntPointsMount(appProperty, renderInfo, nullptr, SandboxCommonDef::g_ohosRender);
            APPSPAWN_CHECK(ret == 0, return ret, "DoAllMntPointsMount failed, %{public}s", bundleName);

            ret = DoAllSymlinkPointslink(appProperty, renderInfo);
            APPSPAWN_CHECK(ret == 0, return ret, "DoAllSymlinkPointslink failed, %{public}s", bundleName);

            ret = HandleFlagsPoint(appProperty, renderInfo);
            APPSPAWN_CHECK_ONLY_LOG(ret == 0, "HandleFlagsPoint for render-sandbox failed, %{public}s", bundleName);
        } else if (strcmp(processType, "gpu") == 0) {
            cJSON *gpuInfo = GetFirstSubConfig(firstIndividual, SandboxCommonDef::g_ohosGpu.c_str());
            if (!gpuInfo) {
                continue;
            }
            ret = DoAllMntPointsMount(appProperty, gpuInfo, nullptr, SandboxCommonDef::g_ohosGpu);
            APPSPAWN_CHECK(ret == 0, return ret, "DoAllMntPointsMount failed, %{public}s", bundleName);

            ret = DoAllSymlinkPointslink(appProperty, gpuInfo);
            APPSPAWN_CHECK(ret == 0, return ret, "DoAllSymlinkPointslink failed, %{public}s", bundleName);

            ret = HandleFlagsPoint(appProperty, gpuInfo);
            APPSPAWN_CHECK_ONLY_LOG(ret == 0, "HandleFlagsPoint for gpu-sandbox failed, %{public}s", bundleName);
        }
    }

    return 0;
}

int32_t SandboxCore::SetAppSandboxPropertyNweb(AppSpawningCtx *appProperty, uint32_t sandboxNsFlags)
{
    APPSPAWN_CHECK(appProperty != nullptr, return -1, "Invalid appspawn client");
    if (SandboxCommon::CheckBundleName(GetBundleName(appProperty)) != 0) {
        return -1;
    }
    std::string sandboxPackagePath = SandboxCommonDef::g_sandBoxRootDirNweb;
    const std::string bundleName = GetBundleName(appProperty);
    bool sandboxSharedStatus = SandboxCommon::IsPrivateSharedStatus(bundleName, appProperty);
    sandboxPackagePath += bundleName;
    SandboxCommon::CreateDirRecursiveWithClock(sandboxPackagePath.c_str(), SandboxCommonDef::FILE_MODE);

    // add pid to a new mnt namespace
    StartAppspawnTrace("EnableSandboxNamespace");
    int rc = EnableSandboxNamespace(appProperty, sandboxNsFlags);
    FinishAppspawnTrace();
    APPSPAWN_CHECK(rc == 0, return rc, "unshare failed, packagename is %{public}s", bundleName.c_str());

    // check app sandbox switch
    if ((SandboxCommon::IsTotalSandboxEnabled(appProperty) == false) ||
        (SandboxCommon::IsAppSandboxEnabled(appProperty) == false)) {
        rc = DoSandboxRootFolderCreateAdapt(sandboxPackagePath);
    } else if (!sandboxSharedStatus) {
        rc = DoSandboxRootFolderCreate(appProperty, sandboxPackagePath);
    }
    APPSPAWN_CHECK(rc == 0, return rc, "DoSandboxRootFolderCreate failed, %{public}s", bundleName.c_str());

    // rendering process can be created by different apps, and the bundle names of these apps are different,
    // so we can't use the method SetPrivateAppSandboxProperty which mount dirs by using bundle name.
    rc = SetRenderSandboxPropertyNweb(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(rc == 0, return rc, "SetRenderSandboxPropertyNweb for %{public}s failed", bundleName.c_str());

    rc = SetOverlayAppSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(rc == 0, return rc, "SetOverlayAppSandboxProperty for %{public}s failed", bundleName.c_str());

    rc = SetBundleResourceAppSandboxProperty(appProperty, sandboxPackagePath);
    APPSPAWN_CHECK(rc == 0, return rc, "SetBundleResourceAppSandboxProperty for %{public}s failed", bundleName.c_str());

#ifndef APPSPAWN_TEST
    rc = chdir(sandboxPackagePath.c_str());
    APPSPAWN_CHECK(rc == 0, return rc, "chdir %{public}s failed, err %{public}d", sandboxPackagePath.c_str(), errno);

    if (sandboxSharedStatus) {
        rc = chroot(sandboxPackagePath.c_str());
        APPSPAWN_CHECK(rc == 0, return rc, "chroot %{public}s failed, err %{public}d",
            sandboxPackagePath.c_str(), errno);
        return 0;
    }

    rc = syscall(SYS_pivot_root, sandboxPackagePath.c_str(), sandboxPackagePath.c_str());
    APPSPAWN_CHECK(rc == 0, return rc, "pivot %{public}s failed, err %{public}d", sandboxPackagePath.c_str(), errno);

    rc = umount2(".", MNT_DETACH);
    APPSPAWN_CHECK(rc == 0, return rc, "MNT_DETACH failed, packagename is %{public}s", bundleName.c_str());
#endif
    return 0;
}

int32_t SandboxCore::ChangeCurrentDir(std::string &sandboxPackagePath, const std::string &bundleName,
                                    bool sandboxSharedStatus)
{
    int32_t ret = 0;
    ret = chdir(sandboxPackagePath.c_str());
    APPSPAWN_CHECK(ret == 0, return ret, "chdir %{public}s failed, err %{public}d", sandboxPackagePath.c_str(), errno);

    if (sandboxSharedStatus) {
        ret = chroot(sandboxPackagePath.c_str());
        APPSPAWN_CHECK(ret == 0, return ret, "chroot %{public}s failed, err %{public}d",
            sandboxPackagePath.c_str(), errno);
        return ret;
    }

    ret = syscall(SYS_pivot_root, sandboxPackagePath.c_str(), sandboxPackagePath.c_str());
    APPSPAWN_CHECK(ret == 0, return ret, "pivot %{public}s failed, err %{public}d", sandboxPackagePath.c_str(), errno);

    ret = umount2(".", MNT_DETACH);
    APPSPAWN_CHECK(ret == 0, return ret, "MNT_DETACH failed, packagename is %{public}s", bundleName.c_str());
    return ret;
}

static const DecDenyPathTemplate DEC_DENY_PATH_MAP[] = {
    {"ohos.permission.READ_WRITE_DOWNLOAD_DIRECTORY", "/storage/Users/currentUser/Download"},
    {"ohos.permission.READ_WRITE_DESKTOP_DIRECTORY", "/storage/Users/currentUser/Desktop"},
    {"ohos.permission.READ_WRITE_DOCUMENTS_DIRECTORY", "/storage/Users/currentUser/Documents"},
};

int32_t SandboxCore::SetDecWithDir(const AppSpawningCtx *appProperty, uint32_t userId)
{
    AppSpawnMsgAccessToken *tokenInfo =
        reinterpret_cast<AppSpawnMsgAccessToken *>(GetAppProperty(appProperty, TLV_ACCESS_TOKEN_INFO));
    APPSPAWN_CHECK(tokenInfo != nullptr, return APPSPAWN_MSG_INVALID, "Get token id failed.");

    AppSpawnMsgBundleInfo *bundleInfo =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    APPSPAWN_CHECK(bundleInfo != nullptr, return APPSPAWN_MSG_INVALID, "No bundle info in msg %{public}s",
        GetBundleName(appProperty));

    uint32_t flags = CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_ATOMIC_SERVICE) ? 0x4 : 0;
    if (flags == 0) {
        flags = (CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_CLONE_ENABLE) &&
            bundleInfo->bundleIndex > 0) ? 0x1 : 0;
    }
    std::ostringstream clonePackageName;
    if (flags == 1) {
        clonePackageName << "+clone-" << bundleInfo->bundleIndex << "+" << bundleInfo->bundleName;
    } else {
        clonePackageName << bundleInfo->bundleName;
    }
    std::string dir = "/storage/Users/currentUser/Download/" + clonePackageName.str();
    DecPolicyInfo decPolicyInfo = {0};
    decPolicyInfo.pathNum = 1;
    PathInfo pathInfo = {0};
    pathInfo.path = strdup(dir.c_str());
    if (pathInfo.path == nullptr) {
        APPSPAWN_LOGE("strdup %{public}s failed, err %{public}d", dir.c_str(), errno);
        return APPSPAWN_MSG_INVALID;
    }
    pathInfo.pathLen = static_cast<uint32_t>(strlen(pathInfo.path));
    pathInfo.mode = SANDBOX_MODE_WRITE | SANDBOX_MODE_READ;
    decPolicyInfo.path[0] = pathInfo;
    decPolicyInfo.tokenId = tokenInfo->accessTokenIdEx;
    decPolicyInfo.flag = true;
    SetDecPolicyInfos(&decPolicyInfo);

    if (decPolicyInfo.path[0].path) {
        free(decPolicyInfo.path[0].path);
        decPolicyInfo.path[0].path = nullptr;
    }
    return 0;
}

int32_t SandboxCore::SetDecPolicyWithPermission(const AppSpawningCtx *appProperty, SandboxMountConfig &mountConfig)
{
    if (mountConfig.decPaths.size() == 0) {
        return 0;
    }
    AppSpawnMsgAccessToken *tokenInfo =
        reinterpret_cast<AppSpawnMsgAccessToken *>(GetAppProperty(appProperty, TLV_ACCESS_TOKEN_INFO));
    APPSPAWN_CHECK(tokenInfo != nullptr, return APPSPAWN_MSG_INVALID, "Get token id failed.");

    DecPolicyInfo decPolicyInfo = {0};
    decPolicyInfo.pathNum = mountConfig.decPaths.size();
    int ret = 0;
    for (uint32_t i = 0; i < decPolicyInfo.pathNum; i++) {
        PathInfo pathInfo = {0};
        pathInfo.path = strdup(mountConfig.decPaths[i].c_str());
        if (pathInfo.path == nullptr) {
            APPSPAWN_LOGE("strdup %{public}s failed, err %{public}d", mountConfig.decPaths[i].c_str(), errno);
            ret = APPSPAWN_ERROR_UTILS_MEM_FAIL;
            goto EXIT;
        }
        pathInfo.pathLen = static_cast<uint32_t>(strlen(pathInfo.path));
        pathInfo.mode = SANDBOX_MODE_WRITE | SANDBOX_MODE_READ;
        decPolicyInfo.path[i] = pathInfo;
    }
    decPolicyInfo.tokenId = tokenInfo->accessTokenIdEx;
    decPolicyInfo.flag = true;
    SetDecPolicyInfos(&decPolicyInfo);
EXIT:
    for (uint32_t i = 0; i < decPolicyInfo.pathNum; i++) {
        if (decPolicyInfo.path[i].path) {
            free(decPolicyInfo.path[i].path);
            decPolicyInfo.path[i].path = nullptr;
        }
    }
    return ret;
}

void SandboxCore::SetDecDenyWithDir(const AppSpawningCtx *appProperty)
{
    int32_t userFileIndex = GetPermissionIndex(nullptr, SandboxCommonDef::READ_WRITE_USER_FILE_MODE.c_str());
    APPSPAWN_CHECK_LOGV(userFileIndex != -1, return, "userFileIndex Invalid index");
    if (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(userFileIndex)) == 0) {
        APPSPAWN_LOGV("The app doesn't have %{public}s, no need to set deny rules",
                      SandboxCommonDef::READ_WRITE_USER_FILE_MODE.c_str());
        return;
    }

    AppSpawnMsgAccessToken *tokenInfo =
        reinterpret_cast<AppSpawnMsgAccessToken *>(GetAppProperty(appProperty, TLV_ACCESS_TOKEN_INFO));
    APPSPAWN_CHECK(tokenInfo != nullptr, return, "Get token id failed");

    DecPolicyInfo decPolicyInfo = {0};
    decPolicyInfo.pathNum = 0;
    uint32_t count = ARRAY_LENGTH(DEC_DENY_PATH_MAP);
    for (uint32_t i = 0, j = 0; i < count; i++) {
        int32_t index = GetPermissionIndex(nullptr, DEC_DENY_PATH_MAP[i].permission);
        if (CheckAppPermissionFlagSet(appProperty, static_cast<uint32_t>(index))) {
            continue;
        }
        PathInfo pathInfo = {0};
        pathInfo.path = const_cast<char *>(DEC_DENY_PATH_MAP[i].decPath);
        pathInfo.pathLen = static_cast<uint32_t>(strlen(pathInfo.path));
        pathInfo.mode = DEC_MODE_DENY_INHERIT;
        decPolicyInfo.path[j++] = pathInfo;
        decPolicyInfo.pathNum += 1;
    }
    decPolicyInfo.tokenId = tokenInfo->accessTokenIdEx;
    decPolicyInfo.flag = true;
    SetDecPolicyInfos(&decPolicyInfo);
}

std::string SandboxCore::ConvertDebugRealPath(const AppSpawningCtx *appProperty, std::string path)
{
    AppSpawnMsgBundleInfo *info =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(appProperty, TLV_BUNDLE_INFO));
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(appProperty, TLV_DAC_INFO));
    if (info == nullptr || dacInfo == nullptr) {
        return "";
    }
    if (CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_ATOMIC_SERVICE)) {
        path = SandboxCommon::ReplaceAllVariables(path, SandboxCommonDef::g_variablePackageName, info->bundleName);
    }
    return SandboxCommon::ConvertToRealPath(appProperty, path);
}

void SandboxCore::DoUninstallDebugSandbox(std::vector<std::string> &bundleList, cJSON *config)
{
    cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(config, SandboxCommonDef::g_mountPrefix);
    if (mountPoints == nullptr || !cJSON_IsArray(mountPoints)) {
        APPSPAWN_LOGI("Invalid mountPoints");
        return;
    }

    auto processor = [&bundleList](cJSON *mntPoint) {
        const char *srcPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_srcPath);
        const char *sandboxPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_sandBoxPath);
        if (srcPathChr == nullptr || sandboxPathChr == nullptr) {
            return 0;
        }
        std::string tmpSandboxPath = sandboxPathChr;
        for (const auto& currentBundle : bundleList) {
            std::string sandboxPath = currentBundle + tmpSandboxPath;
            APPSPAWN_LOGV("DoUninstallDebugSandbox with path %{public}s", sandboxPath.c_str());
            APPSPAWN_CHECK(access(sandboxPath.c_str(), F_OK) == 0, continue,
                "Invalid path %{public}s", sandboxPath.c_str());
            int ret = umount2(sandboxPath.c_str(), MNT_DETACH);
            APPSPAWN_CHECK_ONLY_LOG(ret == 0, "umount failed %{public}d %{public}d", ret, errno);
            ret = rmdir(sandboxPath.c_str());
            APPSPAWN_CHECK_ONLY_LOG(ret == 0, "rmdir failed %{public}d %{public}d", ret, errno);
        }
        return 0;
    };

    (void)SandboxCommon::HandleArrayForeach(mountPoints, processor);
}

int32_t SandboxCore::GetPackageList(AppSpawningCtx *property, std::vector<std::string> &bundleList, bool tmp)
{
    APPSPAWN_CHECK(property != nullptr, return APPSPAWN_ARG_INVALID, "Invalid property");
    AppDacInfo *info = reinterpret_cast<AppDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    if (GetBundleName(property) == nullptr || SandboxCommon::CheckBundleName(GetBundleName(property)) != 0 ||
        info == nullptr) {
        std::string uid;
        char *userId = (char *)GetAppSpawnMsgExtInfo(property->message, MSG_EXT_NAME_USERID, nullptr);
        if (userId != nullptr) {
            uid = std::string(userId);
        } else {
            APPSPAWN_CHECK(info != nullptr, return APPSPAWN_ARG_INVALID, "Invalid dacInfo");
            uid = std::to_string(info->uid / UID_BASE);
        }
        std::string defaultSandboxRoot = (tmp ? SandboxCommonDef::g_mntTmpRoot : SandboxCommonDef::g_mntShareRoot) +
                                          uid + "/debug_hap";
        APPSPAWN_CHECK(access(defaultSandboxRoot.c_str(), F_OK) == 0, return APPSPAWN_ARG_INVALID,
            "Failed to access %{public}s, err %{public}d", defaultSandboxRoot.c_str(), errno);

        DIR *dir = opendir(defaultSandboxRoot.c_str());
        if (dir == nullptr) {
            APPSPAWN_LOGE("Failed to open %{public}s, err %{public}d", defaultSandboxRoot.c_str(), errno);
            return APPSPAWN_SYSTEM_ERROR;
        }
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_name[0] == '.') {
                continue;
            }
            std::string packagePath = defaultSandboxRoot + "/" + std::string(entry->d_name);
            APPSPAWN_LOGV("GetPackageList %{public}s", packagePath.c_str());
            bundleList.push_back(packagePath);
        }
        closedir(dir);
    } else {
        bundleList.push_back(ConvertDebugRealPath(property, tmp ? SandboxCommonDef::g_mntTmpSandboxRoot :
                             SandboxCommonDef::g_mntShareSandboxRoot));
    }
    return 0;
}

int32_t SandboxCore::UninstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK(property != nullptr && content != nullptr, return APPSPAWN_ARG_INVALID, "Invalid property");

    std::vector<std::string> bundleList;
    int ret = GetPackageList(property, bundleList, true);
    APPSPAWN_CHECK(ret == 0, return -1, "GetPackageList failed");
    SandboxCommonDef::SandboxConfigType type = CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;
    for (auto& wholeConfig : SandboxCommon::GetCJsonConfig(type)) {
        cJSON *debugJson = GetFirstCommonConfig(wholeConfig, SandboxCommonDef::g_debughap);
        if (!debugJson) {
            continue;
        }
        cJSON *debugCommonConfig = GetFirstSubConfig(debugJson, SandboxCommonDef::g_commonPrefix);
        if (!debugCommonConfig) {
            continue;
        }
        DoUninstallDebugSandbox(bundleList, debugCommonConfig);

        cJSON *debugPermissionConfig = GetFirstSubConfig(debugJson, SandboxCommonDef::g_permissionPrefix);
        if (!debugPermissionConfig) {
            continue;
        }

        cJSON *permissionChild = debugPermissionConfig->child;
        while (permissionChild != nullptr) {
            cJSON *permissionMountPaths = cJSON_GetArrayItem(permissionChild, 0);
            if (!permissionMountPaths) {
                permissionChild = permissionChild->next;
                continue;
            }
            DoUninstallDebugSandbox(bundleList, permissionMountPaths);
            permissionChild = permissionChild->next;
        }
    }
    bundleList.clear();
    ret = GetPackageList(property, bundleList, false);
    APPSPAWN_CHECK(ret == 0, return -1, "GetPackageList failed");
    for (const auto& currentBundle : bundleList) {
        std::string sandboxPath = currentBundle;
        APPSPAWN_LOGV("UninstallDebugSandbox with path %{public}s", sandboxPath.c_str());
            APPSPAWN_CHECK(access(sandboxPath.c_str(), F_OK) == 0, continue,
                "Invalid path %{public}s", sandboxPath.c_str());
        ret = umount2(sandboxPath.c_str(), MNT_DETACH);
        APPSPAWN_CHECK_ONLY_LOG(ret == 0, "umount failed %{public}d %{public}d", ret, errno);
        ret = rmdir(sandboxPath.c_str());
        APPSPAWN_CHECK_ONLY_LOG(ret == 0, "rmdir failed %{public}d %{public}d", ret, errno);
    }

    return 0;
}

int32_t SandboxCore::DoMountDebugPoints(const AppSpawningCtx *appProperty, cJSON *appConfig)
{
    std::string bundleName = GetBundleName(appProperty);
    cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(appConfig, SandboxCommonDef::g_mountPrefix);
    if (mountPoints == nullptr || !cJSON_IsArray(mountPoints)) {
        APPSPAWN_LOGI("mount config is not found, app name is %{public}s", bundleName.c_str());
        return 0;
    }

    std::string sandboxRoot = ConvertDebugRealPath(appProperty, SandboxCommonDef::g_mntTmpSandboxRoot);
    int atomicService = CheckAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_ATOMIC_SERVICE);

    auto processor = [&sandboxRoot, &atomicService, &appProperty](cJSON *mntPoint) {
        const char *srcPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_srcPath);
        const char *sandboxPathChr = GetStringFromJsonObj(mntPoint, SandboxCommonDef::g_sandBoxPath);
        if (srcPathChr == nullptr || sandboxPathChr == nullptr) {
            return 0;
        }
        std::string srcPath(srcPathChr);
        std::string sandboxPath(sandboxPathChr);
        srcPath = SandboxCommon::ConvertToRealPath(appProperty, srcPath);
        sandboxPath = GetSandboxPath(appProperty, mntPoint, SandboxCommonDef::g_debughap, sandboxRoot);
        if (access(sandboxPath.c_str(), F_OK) == 0) {
            APPSPAWN_CHECK(atomicService, return 0, "sandbox path already exist");
            APPSPAWN_CHECK(umount2(sandboxPath.c_str(), MNT_DETACH) == 0, return 0,
                "umount sandbox path failed, errno is %{public}d %{public}s", errno, sandboxPath.c_str());
        }
        SandboxMountConfig mountConfig = {0};
        SandboxCommon::GetSandboxMountConfig(appProperty, SandboxCommonDef::g_debughap, mntPoint, mountConfig);
        SharedMountArgs arg = {
            .srcPath = srcPath.c_str(),
            .destPath = sandboxPath.c_str(),
            .fsType = mountConfig.fsType.c_str(),
            .mountFlags = SandboxCommon::GetMountFlags(mntPoint),
            .options = mountConfig.optionsPoint.c_str(),
            .mountSharedFlag =
                GetBoolValueFromJsonObj(mntPoint, SandboxCommonDef::g_mountSharedFlag, false) ? MS_SHARED : MS_SLAVE
        };
        int ret = SandboxCommon::DoAppSandboxMountOnce(appProperty, &arg);
        APPSPAWN_CHECK(ret == 0 || !SandboxCommon::IsMountSuccessful(mntPoint), return ret,
            "DoMountDebugPoints %{public}s failed", arg.destPath);
        return ret;
    };

    (void)SandboxCommon::HandleArrayForeach(mountPoints, processor);
    return 0;
}

int32_t SandboxCore::MountDebugSharefs(const AppSpawningCtx *property, const char *src, const char *target)
{
    char dataPath[SandboxCommonDef::OPTIONS_MAX_LEN] = {0};
    int ret = snprintf_s(dataPath, SandboxCommonDef::OPTIONS_MAX_LEN, SandboxCommonDef::OPTIONS_MAX_LEN - 1,
                         "%s/data", target);
    if (ret >= 0 && access(dataPath, F_OK) == 0) {
        return 0;
    }

    ret = MakeDirRec(target, SandboxCommonDef::FILE_MODE, 1);
    if (ret != 0) {
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    AppDacInfo *info = reinterpret_cast<AppDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    if (info == nullptr) {
        return APPSPAWN_ARG_INVALID;
    }
    char options[SandboxCommonDef::OPTIONS_MAX_LEN] = {0};
    ret = snprintf_s(options, SandboxCommonDef::OPTIONS_MAX_LEN, SandboxCommonDef::OPTIONS_MAX_LEN - 1,
                     "override_support_delete,user_id=%u", info->uid / UID_BASE);
    if (ret <= 0) {
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    if (mount(src, target, "sharefs", MS_NODEV, options) != 0) {
        APPSPAWN_LOGE("sharefs mount %{public}s to %{public}s failed, errno %{public}d", src, target, errno);
        return APPSPAWN_SANDBOX_ERROR_MOUNT_FAIL;
    }
    if (mount(nullptr, target, nullptr, MS_SHARED, nullptr) != 0) {
        APPSPAWN_LOGE("mount path %{public}s to shared failed, errno %{public}d", target, errno);
        return APPSPAWN_SANDBOX_ERROR_MOUNT_FAIL;
    }

    return 0;
}

int32_t SandboxCore::InstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (!IsDeveloperModeOn(property)) {
        return 0;
    }

    uint32_t len = 0;
    char *provisionType = reinterpret_cast<char *>(GetAppPropertyExt(property,
        MSG_EXT_NAME_PROVISION_TYPE, &len));
    if (provisionType == nullptr || len == 0 || strcmp(provisionType, "debug") != 0) {
        return 0;
    }

    SandboxCommonDef::SandboxConfigType type = CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG : SandboxCommonDef::SANDBOX_APP_JSON_CONFIG;

    for (auto& wholeConfig : SandboxCommon::GetCJsonConfig(type)) {
        cJSON *debugJson = GetFirstCommonConfig(wholeConfig, SandboxCommonDef::g_debughap);
        if (!debugJson) {
            continue;
        }
        cJSON *debugCommonConfig = GetFirstSubConfig(debugJson, SandboxCommonDef::g_commonPrefix);
        if (!debugCommonConfig) {
            continue;
        }
        DoMountDebugPoints(property, debugCommonConfig);

        cJSON *debugPermissionConfig = GetFirstSubConfig(debugJson, SandboxCommonDef::g_permissionPrefix);
        if (!debugPermissionConfig) {
            continue;
        }

        cJSON *permissionChild = debugPermissionConfig->child;
        while (permissionChild != nullptr) {
            int index = GetPermissionIndex(nullptr, permissionChild->string);
            if (CheckAppPermissionFlagSet(property, static_cast<uint32_t>(index)) == 0) {
                permissionChild = permissionChild->next;
                continue;
            }
            cJSON *permissionMountPaths = cJSON_GetArrayItem(permissionChild, 0);
            if (!permissionMountPaths) {
                permissionChild = permissionChild->next;
                continue;
            }
            DoMountDebugPoints(property, permissionMountPaths);

            permissionChild = permissionChild->next;
        }
    }

    MountDebugSharefs(property, ConvertDebugRealPath(property, SandboxCommonDef::g_mntTmpSandboxRoot).c_str(),
        ConvertDebugRealPath(property, SandboxCommonDef::g_mntShareSandboxRoot).c_str());
    return 0;
}
} // namespace AppSpawn
} // namespace OHOS