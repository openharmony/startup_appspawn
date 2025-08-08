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

#ifndef SANDBOX_DEF_H
#define SANDBOX_DEF_H

#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>

namespace OHOS {
namespace AppSpawn {
namespace SandboxCommonDef {
// 全局常量定义
constexpr int32_t OPTIONS_MAX_LEN = 256;
constexpr int32_t FILE_ACCESS_COMMON_DIR_STATUS = 0;
constexpr int32_t FILE_CROSS_APP_STATUS = 1;
constexpr static mode_t FILE_MODE = 0711;
constexpr static mode_t BASIC_MOUNT_FLAGS = MS_REC | MS_BIND;
constexpr int32_t MAX_MOUNT_TIME = 500;  // 500us
constexpr int32_t LOCK_STATUS_SIZE = 16;

// 沙盒配置文件
const std::string APP_JSON_CONFIG = "/appdata-sandbox.json";
const std::string APP_ISOLATED_JSON_CONFIG = "/appdata-sandbox-isolated.json";

/* 沙盒配置文件中关键字 */
// 公共属性
constexpr const char *g_sandboxRootPrefix = "sandbox-root";
constexpr const char *g_sandBoxNsFlags = "sandbox-ns-flags";
constexpr const char *g_topSandBoxSwitchPrefix = "top-sandbox-switch";
constexpr const char *g_sandBoxSwitchPrefix = "sandbox-switch";
const std::string g_ohosGpu = "__internal__.com.ohos.gpu";
const std::string g_ohosRender = "__internal__.com.ohos.render";
constexpr const char *g_commonPrefix = "common";
constexpr const char *g_privatePrefix = "individual";
constexpr const char *g_permissionPrefix = "permission";
constexpr const char *g_appBase = "app-base";
constexpr const char *g_appResources = "app-resources";
constexpr const char *g_flagePoint = "flags-point";
const std::string g_internal = "__internal__";
const std::string g_mntTmpRoot = "/mnt/debugtmp/";
const std::string g_mntShareRoot = "/mnt/debug/";
const std::string g_sandboxRootPathTemplate = "/mnt/sandbox/<currentUserId>/<PackageName>";
const std::string g_originSandboxPath = "/mnt/sandbox/<PackageName>";

// 挂载目录字段
constexpr const char *g_mountPrefix = "mount-paths";
constexpr const char *g_srcPath = "src-path";
constexpr const char *g_sandBoxPath = "sandbox-path";
constexpr const char *g_sandBoxFlags = "sandbox-flags";
constexpr const char *g_fsType = "fs-type";
constexpr const char *g_sandBoxOptions = "options";
constexpr const char *g_actionStatuc = "check-action-status";
constexpr const char *g_destMode = "dest-mode";
constexpr const char *g_flags = "flags";

// 挂载可选属性
constexpr const char *g_sandBoxShared = "sandbox-shared";
constexpr const char *g_mountSharedFlag = "mount-shared-flag";
constexpr const char *g_dacOverrideSensitive = "dac-override-sensitive";
constexpr const char *g_sandBoxFlagsCustomized = "sandbox-flags-customized";
constexpr const char *g_appAplName = "app-apl-name";
constexpr const char *g_sandBoxDecPath = "dec-paths";
constexpr const char *CREATE_SANDBOX_PATH = "create-sandbox-path";

// link目录字段
constexpr const char *g_symlinkPrefix = "symbol-links";
constexpr const char *g_targetName = "target-name";
constexpr const char *g_linkName = "link-name";

constexpr const char *g_gidPrefix = "gids";

// 可变参数
const std::string g_userId = "<currentUserId>";
const std::string g_permissionUserId = "<permissionUserId>";
const std::string g_permissionUser = "<permissionUser>";
const std::string g_packageName = "<PackageName>";
const std::string g_packageNameIndex = "<PackageName_index>";
const std::string g_variablePackageName = "<variablePackageName>";
const std::string g_clonePackageName = "<clonePackageName>";
const std::string g_arkWebPackageName = "<arkWebPackageName>";
const std::string g_hostUserId = "<hostUserId>";
const std::string g_devModel = "<devModel>";

/* HSP */
const std::string HSPLIST_SOCKET_TYPE = "HspList";
const std::string g_hspList_key_bundles = "bundles";
const std::string g_hspList_key_modules = "modules";
const std::string g_hspList_key_versions = "versions";
const std::string g_sandboxHspInstallPath = "/data/storage/el1/bundle/";

/* DataGroup */
const std::string DATA_GROUP_SOCKET_TYPE = "DataGroup";
const std::string g_groupList_key_dataGroupId = "dataGroupId";
const std::string g_groupList_key_gid = "gid";
const std::string g_groupList_key_dir = "dir";
const std::string g_groupList_key_uuid = "uuid";

/* Overlay */
const std::string OVERLAY_SOCKET_TYPE = "Overlay";
const std::string g_overlayPath = "/data/storage/overlay/";

/* system hap */
const std::string APL_SYSTEM_CORE = "system_core";
const std::string APL_SYSTEM_BASIC = "system_basic";
const std::string g_physicalAppInstallPath = "/data/app/el1/bundle/public/";
const std::string g_dataBundles = "/data/bundles/";

/* bundle resource with APP_FLAGS_BUNDLE_RESOURCES */
const std::string g_bundleResourceSrcPath = "/data/service/el1/public/bms/bundle_resources/";
const std::string g_bundleResourceDestPath = "/data/storage/bundle_resources/";

/* 配置文件中value校验值 */
const std::string g_sandBoxRootDir = "/mnt/sandbox/";
const std::string g_sandBoxRootDirNweb = "/mnt/sandbox/com.ohos.render/";
const std::string  DEV_SHM_DIR = "/dev/shm/";
const std::string g_statusCheck = "true";
const std::string g_sbxSwitchCheck = "ON";
const std::string g_dlpBundleName = "com.ohos.dlpmanager";

/* debug hap */
constexpr const char *g_mntTmpSandboxRoot = "/mnt/debugtmp/<currentUserId>/debug_hap/<variablePackageName>";
constexpr const char *g_mntShareSandboxRoot = "/mnt/debug/<currentUserId>/debug_hap/<variablePackageName>";
constexpr const char *g_debughap = "debug";

/* 分割符 */
constexpr const char *g_fileSeparator = "/";
constexpr const char *g_overlayDecollator = "|";

/* 权限名 */
const std::string FILE_CROSS_APP_MODE = "ohos.permission.FILE_CROSS_APP";
const std::string FILE_ACCESS_COMMON_DIR_MODE = "ohos.permission.FILE_ACCESS_COMMON_DIR";
const std::string ACCESS_DLP_FILE_MODE = "ohos.permission.ACCESS_DLP_FILE";
const std::string FILE_ACCESS_MANAGER_MODE = "ohos.permission.FILE_ACCESS_MANAGER";
const std::string READ_WRITE_USER_FILE_MODE = "ohos.permission.READ_WRITE_USER_FILE";
const std::string GET_ALL_PROCESSES_MODE = "ohos.permission.GET_ALL_PROCESSES";
const std::string APP_ALLOW_IOURING = "ohos.permission.ALLOW_IOURING";
const std::string ARK_WEB_PERSIST_PACKAGE_NAME = "persist.arkwebcore.package_name";

/* 系统参数 */
const std::string DEVICE_MODEL_NAME_PARAM = "const.cust.devmodel";

// 枚举类型
enum SandboxConfigType {
    SANDBOX_APP_JSON_CONFIG,
    SANDBOX_ISOLATED_JSON_CONFIG
};

} // namespace SandboxCommonDef
} // namespace AppSpawn
} // namespace OHOS

#endif // SANDBOX_DEF_H