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

#include <cerrno>
#include <sys/mount.h>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <map>
#include "securec.h"

#include "sandbox_shared_mount.h"
#include "appspawn_mount_permission.h"
#include "appspawn_utils.h"
#include "parameter.h"

#define USER_ID_SIZE 16
#define DIR_MODE 0711
#define LOCK_STATUS_SIZE 16

#define DATA_GROUP_SOCKET_TYPE    "DataGroup"
#define GROUPLIST_KEY_DATAGROUPID "dataGroupId"
#define GROUPLIST_KEY_GID         "gid"
#define GROUPLIST_KEY_DIR         "dir"
#define GROUPLIST_KEY_UUID        "uuid"

static const MountSharedTemplate MOUNT_SHARED_MAP[] = {
    {"/data/storage/el2", nullptr},
    {"/data/storage/el3", nullptr},
    {"/data/storage/el4", nullptr},
    {"/data/storage/el5", "ohos.permission.PROTECT_SCREEN_LOCK_DATA"},
};

static const DataGroupSandboxPathTemplate DATA_GROUP_SANDBOX_PATH_MAP[] = {
    {"el2", EL2, "/data/storage/el2/group/", nullptr},
    {"el3", EL3, "/data/storage/el3/group/", nullptr},
    {"el4", EL4, "/data/storage/el4/group/", nullptr},
    {"el5", EL5, "/data/storage/el5/group/", "ohos.permission.PROTECT_SCREEN_LOCK_DATA"},
};

static std::map<std::string, int> g_mountInfoMap;

int GetElxInfoFromDir(const char *path)
{
    int ret = ELX_MAX;
    if (path == nullptr) {
        return ret;
    }
    uint32_t count = ARRAY_LENGTH(DATA_GROUP_SANDBOX_PATH_MAP);
    for (uint32_t i = 0; i < count; ++i) {
        if (strstr(path, DATA_GROUP_SANDBOX_PATH_MAP[i].elxName) != nullptr) {
            return DATA_GROUP_SANDBOX_PATH_MAP[i].category;
        }
    }
    APPSPAWN_LOGE("Get elx info from dir failed, path %{public}s", path);
    return ret;
}

const DataGroupSandboxPathTemplate *GetDataGroupArgTemplate(uint32_t category)
{
    uint32_t count = ARRAY_LENGTH(DATA_GROUP_SANDBOX_PATH_MAP);
    if (category > count) {
        APPSPAWN_LOGE("category %{public}d is out of range", category);
        return nullptr;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (DATA_GROUP_SANDBOX_PATH_MAP[i].category == category) {
            return &DATA_GROUP_SANDBOX_PATH_MAP[i];
        }
    }
    return nullptr;
}

bool IsValidDataGroupItem(nlohmann::json &item)
{
    // Check if the item contains the specified key and if the value corresponding to the key is a string
    if (item.contains(GROUPLIST_KEY_DATAGROUPID) && item[GROUPLIST_KEY_DATAGROUPID].is_string() &&
        item.contains(GROUPLIST_KEY_GID) && item[GROUPLIST_KEY_GID].is_string() &&
        item.contains(GROUPLIST_KEY_DIR) && item[GROUPLIST_KEY_DIR].is_string() &&
        item.contains(GROUPLIST_KEY_UUID) && item[GROUPLIST_KEY_UUID].is_string()) {
        return true;
    }
    return false;
}

void *GetEl1BundleMountCount(void)
{
    return static_cast<void*>(&g_mountInfoMap);
}

#ifndef APPSPAWN_SANDBOX_NEW
static bool IsUnlockStatus(uint32_t uid)
{
    const int userIdBase = UID_BASE;
    uid = uid / userIdBase;
    if (uid == 0) {
        return true;
    }
    std::string lockStatusParam = "startup.appspawn.lockstatus_" + std::to_string(uid);
    char userLockStatus[LOCK_STATUS_SIZE] = {0};
    int ret = GetParameter(lockStatusParam.c_str(), "1", userLockStatus, sizeof(userLockStatus));
    APPSPAWN_LOGI("get param %{public}s %{public}s", lockStatusParam.c_str(), userLockStatus);
    if (ret > 0 && (strcmp(userLockStatus, "0") == 0)) {   // 0:unlock status 1：lock status
        return true;
    }
    return false;
}

static int DoSharedMount(const SharedMountArgs *arg)
{
    if (arg == nullptr || arg->srcPath == nullptr || arg->destPath == nullptr) {
        APPSPAWN_LOGE("Invalid arg");
        return APPSPAWN_ARG_INVALID;
    }

    APPSPAWN_LOGV("Mount arg: '%{public}s' '%{public}s' %{public}lu '%{public}s' %{public}s => %{public}s",
                  arg->fsType, arg->mountSharedFlag == MS_SHARED ? "MS_SHARED" : "MS_SLAVE",
                  arg->mountFlags, arg->options, arg->srcPath, arg->destPath);

    int ret = mount(arg->srcPath, arg->destPath, arg->fsType, arg->mountFlags, arg->options);
    if (ret != 0) {
        APPSPAWN_LOGE("mount %{public}s to %{public}s failed, errno %{public}d",
                      arg->srcPath, arg->destPath, errno);
        return ret;
    }
    ret = mount(nullptr, arg->destPath, nullptr, arg->mountSharedFlag, nullptr);
    if (ret != 0) {
        APPSPAWN_LOGE("mount path %{public}s to shared failed, errno %{public}d", arg->destPath, errno);
        return ret;
    }
    APPSPAWN_LOGI("mount path %{public}s to shared success", arg->destPath);
    return 0;
}

static bool SetSandboxPathShared(const std::string &sandboxPath)
{
    int ret = mount(nullptr, sandboxPath.c_str(), nullptr, MS_SHARED, nullptr);
    if (ret != 0) {
        APPSPAWN_LOGW("Need to mount %{public}s to shared, errno %{public}d", sandboxPath.c_str(), errno);
        return false;
    }
    return true;
}

static int MountEl1Bundle(const AppSpawningCtx *property, const AppDacInfo *info, const char *varBundleName)
{
    /* /data/app/el1/bundle/public/<bundleName> */
    AppSpawnMsgBundleInfo *bundleInfo =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(property, TLV_BUNDLE_INFO));
    if (bundleInfo == nullptr) {
        return APPSPAWN_SANDBOX_INVALID;
    }
    char sourcePath[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(sourcePath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/data/app/el1/bundle/public/%s",
                         bundleInfo->bundleName);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf data/app/el1/bundle/public/%{public}s failed, errno %{public}d",
                      bundleInfo->bundleName, errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    /* /mnt/sandbox/<currentUserId>/<varBundleName>/data/storage/el1/bundle */
    char targetPath[PATH_MAX_LEN] = {0};
    ret = snprintf_s(targetPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/sandbox/%u/%s/data/storage/el1/bundle",
                     info->uid/ UID_BASE, varBundleName);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf el1 bundle sandbox path failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    ret = MakeDirRec(targetPath, DIR_MODE, 1);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir %{public}s failed, errno %{public}d", targetPath, errno);
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    ret = umount2(targetPath, MNT_DETACH);
    if (ret != 0) {
        APPSPAWN_LOGE("umount2 %{public}s failed, errno %{public}d", targetPath, errno);
    }

    SharedMountArgs arg = {
        .srcPath = sourcePath,
        .destPath = targetPath,
        .fsType = nullptr,
        .mountFlags = MS_BIND | MS_REC,
        .options = nullptr,
        .mountSharedFlag = MS_SHARED
    };
    ret = DoSharedMount(&arg);
    if (ret != 0) {
        APPSPAWN_LOGE("mount %{public}s shared failed, ret %{public}d", targetPath, ret);
    }
    std::string key = std::to_string(info->uid / UID_BASE) + "-" + std::string(varBundleName);
    g_mountInfoMap[key]++;
    return ret;
}

static int MountWithFileMgr(const AppSpawningCtx *property, const AppDacInfo *info, const char *varBundleName)
{
    /* /mnt/user/<currentUserId>/nosharefs/docs */
    char nosharefsDocsDir[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(nosharefsDocsDir, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/user/%u/nosharefs/docs",
                         info->uid / UID_BASE);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf nosharefsDocsDir failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    /* /mnt/sandbox/<currentUser/<varBundleName>/storage/Users */
    char storageUserPath[PATH_MAX_LEN] = {0};
    ret = snprintf_s(storageUserPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/sandbox/%u/%s/storage/Users",
                     info->uid / UID_BASE, varBundleName);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf storageUserPath failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    // Check whether the directory is a shared mount point
    if (SetSandboxPathShared(storageUserPath)) {
        APPSPAWN_LOGV("shared mountpoint is exist");
        return 0;
    }

    ret = MakeDirRec(storageUserPath, DIR_MODE, 1);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir %{public}s failed, errno %{public}d", storageUserPath, errno);
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    SharedMountArgs arg = {
        .srcPath = nosharefsDocsDir,
        .destPath = storageUserPath,
        .fsType = nullptr,
        .mountFlags = MS_BIND | MS_REC,
        .options = nullptr,
        .mountSharedFlag = MS_SHARED
    };
    ret = DoSharedMount(&arg);
    if (ret != 0) {
        APPSPAWN_LOGE("mount %{public}s shared failed, ret %{public}d", storageUserPath, ret);
    }
    return ret;
}

static int MountWithOther(const AppSpawningCtx *property, const AppDacInfo *info, const char *varBundleName)
{
    /* /mnt/user/<currentUserId>/sharefs/docs */
    char sharefsDocsDir[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(sharefsDocsDir, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/user/%u/sharefs/docs",
                         info->uid / UID_BASE);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf sharefsDocsDir failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    /* /mnt/sandbox/<currentUser/<varBundleName>/storage/Users */
    char storageUserPath[PATH_MAX_LEN] = {0};
    ret = snprintf_s(storageUserPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/sandbox/%u/%s/storage/Users",
                     info->uid / UID_BASE, varBundleName);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf storageUserPath failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    // Check whether the directory is a shared mount point
    if (SetSandboxPathShared(storageUserPath)) {
        APPSPAWN_LOGV("shared mountpoint is exist");
        return 0;
    }

    ret = MakeDirRec(storageUserPath, DIR_MODE, 1);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir %{public}s failed, errno %{public}d", storageUserPath, errno);
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    char options[PATH_MAX_LEN] = {0};
    ret = snprintf_s(options, PATH_MAX_LEN, PATH_MAX_LEN - 1, "override_support_delete,user_id=%u",
                     info->uid / UID_BASE);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf options failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    SharedMountArgs arg = {
        .srcPath = sharefsDocsDir,
        .destPath = storageUserPath,
        .fsType = "sharefs",
        .mountFlags = MS_NODEV,
        .options = options,
        .mountSharedFlag = MS_SHARED
    };
    ret = DoSharedMount(&arg);
    if (ret != 0) {
        APPSPAWN_LOGE("mount %{public}s shared failed, ret %{public}d", storageUserPath, ret);
    }
    return ret;
}

static void MountStorageUsers(const AppSpawningCtx *property, const AppDacInfo *info, const char *varBundleName)
{
    int ret = 0;
    int index = GetPermissionIndex(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    int checkRes = CheckAppPermissionFlagSet(property, static_cast<uint32_t>(index));
    if (checkRes == 0) {
        /* mount /mnt/user/<currentUserId>/sharefs/docs to /mnt/sandbox/<currentUserId>/<varBundleName>/storage/Users */
        ret = MountWithOther(property, info, varBundleName);
    } else {
        /* mount /mnt/user/<currentUserId>/nosharefs/docs to /mnt/sandbox/<currentUserId>/<varBundleName>/storage/Users
         */
        ret = MountWithFileMgr(property, info, varBundleName);
    }
    if (ret != 0) {
        APPSPAWN_LOGE("Update %{public}s storage dir failed, ret %{public}d",
                      checkRes == 0 ? "sharefs dir" : "no sharefs dir", ret);
    } else {
        APPSPAWN_LOGI("Update %{public}s storage dir success", checkRes == 0 ? "sharefs dir" : "no sharefs dir");
    }
}

static int MountSharedMapItem(const AppSpawningCtx *property, const AppDacInfo *info, const char *varBundleName,
                              const char *sandboxPathItem)
{
    /* /mnt/sandbox/<currentUserId>/<varBundleName>/data/storage/el<x> */
    char sandboxPath[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(sandboxPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/sandbox/%u/%s%s",
                         info->uid / UID_BASE, varBundleName, sandboxPathItem);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf sandboxPath failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    // Check whether the directory is a shared mount point
    if (SetSandboxPathShared(sandboxPath)) {
        APPSPAWN_LOGV("shared mountpoint is exist");
        return 0;
    }

    ret = MakeDirRec(sandboxPath, DIR_MODE, 1);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir %{public}s failed, errno %{public}d", sandboxPath, errno);
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    SharedMountArgs arg = {
        .srcPath = sandboxPath,
        .destPath = sandboxPath,
        .fsType = nullptr,
        .mountFlags = MS_BIND | MS_REC,
        .options = nullptr,
        .mountSharedFlag = MS_SHARED
    };
    ret = DoSharedMount(&arg);
    if (ret != 0) {
        APPSPAWN_LOGE("mount %{public}s shared failed, ret %{public}d", sandboxPath, ret);
    }
    return ret;
}

static void MountSharedMap(const AppSpawningCtx *property, const AppDacInfo *info, const char *varBundleName)
{
    int length = sizeof(MOUNT_SHARED_MAP) / sizeof(MOUNT_SHARED_MAP[0]);
    for (int i = 0; i < length; i++) {
        if (MOUNT_SHARED_MAP[i].permission == nullptr) {
            MountSharedMapItem(property, info, varBundleName, MOUNT_SHARED_MAP[i].sandboxPath);
        } else {
            int index = GetPermissionIndex(nullptr, MOUNT_SHARED_MAP[i].permission);
            APPSPAWN_LOGV("mount dir on lock mountPermissionFlags %{public}d", index);
            if (CheckAppPermissionFlagSet(property, static_cast<uint32_t>(index))) {
                MountSharedMapItem(property, info, varBundleName, MOUNT_SHARED_MAP[i].sandboxPath);
            }
        }
    }
    APPSPAWN_LOGI("mount shared map success");
}

static inline bool CheckPath(const std::string& name)
{
    return !name.empty() && name != "." && name != ".." && name.find("/") == std::string::npos;
}

static int DataGroupCtxNodeCompare(ListNode *node, void *data)
{
    DataGroupCtx *existingNode = (DataGroupCtx *)ListEntry(node, DataGroupCtx, node);
    DataGroupCtx *newNode = (DataGroupCtx *)data;
    if (existingNode == nullptr || newNode == nullptr) {
        APPSPAWN_LOGE("Invalid param");
        return APPSPAWN_ARG_INVALID;
    }

    // compare src path and sandbox path
    bool isSrcPathEqual = (strcmp(existingNode->srcPath.path, newNode->srcPath.path) == 0);
    bool isDestPathEqual = (strcmp(existingNode->destPath.path, newNode->destPath.path) == 0);

    return (isSrcPathEqual && isDestPathEqual) ? 0 : 1;
}

static int AddDataGroupItemToQueue(AppSpawnMgr *content, const std::string &srcPath, const std::string &destPath,
                                   const std::string &dataGroupUuid)
{
    DataGroupCtx *dataGroupNode = (DataGroupCtx *)calloc(1, sizeof(DataGroupCtx));
    APPSPAWN_CHECK(dataGroupNode != nullptr, return APPSPAWN_ERROR_UTILS_MEM_FAIL, "Calloc dataGroupNode failed");
    if (strcpy_s(dataGroupNode->srcPath.path, PATH_MAX_LEN - 1, srcPath.c_str()) != EOK ||
        strcpy_s(dataGroupNode->destPath.path, PATH_MAX_LEN - 1, destPath.c_str()) != EOK ||
        strcpy_s(dataGroupNode->dataGroupUuid, UUID_MAX_LEN, dataGroupUuid.c_str()) != EOK) {
        APPSPAWN_LOGE("strcpy dataGroupNode->srcPath failed");
        free(dataGroupNode);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }
    dataGroupNode->srcPath.pathLen = strlen(dataGroupNode->srcPath.path);
    dataGroupNode->destPath.pathLen = strlen(dataGroupNode->destPath.path);
    ListNode *node = OH_ListFind(&content->dataGroupCtxQueue, (void *)dataGroupNode, DataGroupCtxNodeCompare);
    if (node != nullptr) {
        APPSPAWN_LOGI("DataGroupCtxNode %{public}s is exist", dataGroupNode->srcPath.path);
        free(dataGroupNode);
        dataGroupNode = nullptr;
        return 0;
    }
    OH_ListInit(&dataGroupNode->node);
    OH_ListAddTail(&content->dataGroupCtxQueue, &dataGroupNode->node);
    return 0;
}

static std::string GetExtraInfoByType(const AppSpawningCtx *appPropery, const std::string &type)
{
    uint32_t len = 0;
    char *info = reinterpret_cast<char *>(GetAppPropertyExt(appPropery, type.c_str(), &len));
    if (info == nullptr) {
        return "";
    }
    return std::string(info, len);
}

static void DumpDataGroupCtxQueue(const ListNode *front)
{
    if (front == nullptr) {
        return;
    }

    uint32_t count = 0;
    ListNode *node = front->next;
    while (node != front) {
        DataGroupCtx *dataGroupNode = (DataGroupCtx *)ListEntry(node, DataGroupCtx, node);
        count++;
        APPSPAWN_LOGV("      ************************************** %{public}d", count);
        APPSPAWN_LOGV("      srcPath: %{public}s", dataGroupNode->srcPath.path);
        APPSPAWN_LOGV("      destPath: %{public}s", dataGroupNode->destPath.path);
        APPSPAWN_LOGV("      uuid: %{public}s", dataGroupNode->dataGroupUuid);
        node = node->next;
    }
}

static int ParseDataGroupList(AppSpawnMgr *content, const AppSpawningCtx *property, AppDacInfo *info,
                              const char *varBundleName)
{
    int ret = 0;
    std::string dataGroupList = GetExtraInfoByType(property, DATA_GROUP_SOCKET_TYPE);
    if (dataGroupList.length() == 0) {
        APPSPAWN_LOGE("dataGroupList is empty");
        return APPSPAWN_ARG_INVALID;
    }

    nlohmann::json dataGroupJson = nlohmann::json::parse(dataGroupList.c_str(), nullptr, false);
    if (dataGroupJson.is_discarded() && dataGroupJson.contains(GROUPLIST_KEY_DATAGROUPID) &&
        dataGroupJson.contains(GROUPLIST_KEY_GID) && dataGroupJson.contains(GROUPLIST_KEY_DIR) &&
        dataGroupJson.contains(GROUPLIST_KEY_UUID)) {
        APPSPAWN_LOGE("dataGroupJson is discarded");
        return APPSPAWN_ARG_INVALID;
    }
    for (auto &item : dataGroupJson) {
        APPSPAWN_CHECK(IsValidDataGroupItem(item), return -1, "Element is not a valid data group item");
        std::string srcPath = item[GROUPLIST_KEY_DIR];
        APPSPAWN_CHECK(!CheckPath(srcPath), return -1, "src path %{public}s is invalid", srcPath.c_str());

        int elxValue = GetElxInfoFromDir(srcPath.c_str());
        APPSPAWN_CHECK((elxValue >= EL2 && elxValue < ELX_MAX), return -1, "Get elx value failed");
        
        const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(elxValue);
        APPSPAWN_CHECK(templateItem != nullptr, return -1, "Get data group arg template failed");

        // If permission isn't null, need check permission flag
        if (templateItem->permission != nullptr) {
            int index = GetPermissionIndex(nullptr, templateItem->permission);
            APPSPAWN_LOGV("mount dir no lock mount permission flag %{public}d", index);
            if (CheckAppPermissionFlagSet(property, static_cast<uint32_t>(index)) == 0) {
                continue;
            }
        }

        // sandboxPath: /mnt/sandbox/<currentUserId>/<varBundleName>/data/storage/el<x>/group
        std::string sandboxPath = "/mnt/sandbox/" + std::to_string(info->uid / UID_BASE) + "/" + varBundleName
                                 + templateItem->sandboxPath;

        ret = AddDataGroupItemToQueue(content, srcPath, sandboxPath, item[GROUPLIST_KEY_UUID]);
        if (ret != 0) {
            APPSPAWN_LOGE("Add datagroup item to dataGroupCtxQueue failed, el%{public}d", elxValue);
            OH_ListRemoveAll(&content->dataGroupCtxQueue, nullptr);
            return -1;
        }
    }

    DumpDataGroupCtxQueue(&content->dataGroupCtxQueue);
    return ret;
}

int UpdateDataGroupDirs(AppSpawnMgr *content)
{
    if (content == nullptr) {
        return APPSPAWN_ARG_INVALID;
    }

    ListNode *node = content->dataGroupCtxQueue.next;
    while (node != &content->dataGroupCtxQueue) {
        DataGroupCtx *dataGroupNode = (DataGroupCtx *)ListEntry(node, DataGroupCtx, node);
        char sandboxPath[PATH_MAX_LEN] = {0};
        int ret = snprintf_s(sandboxPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s", dataGroupNode->destPath.path,
                             dataGroupNode->dataGroupUuid);
        if (ret <= 0) {
            APPSPAWN_LOGE("snprintf_s sandboxPath: %{public}s failed, errno %{public}d",
                          dataGroupNode->destPath.path, errno);
            return APPSPAWN_ERROR_UTILS_MEM_FAIL;
        }

        SharedMountArgs args = {
            .srcPath = dataGroupNode->srcPath.path,
            .destPath = sandboxPath,
            .fsType = nullptr,
            .mountFlags = MS_BIND | MS_REC,
            .options = nullptr,
            .mountSharedFlag = MS_SHARED
        };
        ret = DoSharedMount(&args);
        if (ret != 0) {
            APPSPAWN_LOGE("Shared mount %{public}s to %{public}s failed, errno %{public}d", args.srcPath,
                          sandboxPath, ret);
        }
        node = node->next;
    }
    OH_ListRemoveAll(&content->dataGroupCtxQueue, NULL);
    return 0;
}

static std::string ReplaceVarBundleName(const AppSpawningCtx *property)
{
    AppSpawnMsgBundleInfo *bundleInfo =
        reinterpret_cast<AppSpawnMsgBundleInfo *>(GetAppProperty(property, TLV_BUNDLE_INFO));
    if (bundleInfo == nullptr) {
        return "";
    }

    std::string tmpBundlePath = bundleInfo->bundleName;
    std::ostringstream variablePackageName;
    if (CheckAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_CLONE_ENABLE)) {
        variablePackageName << "+clone-" << bundleInfo->bundleIndex << "+" << bundleInfo->bundleName;
        tmpBundlePath = variablePackageName.str();
    }
    return tmpBundlePath;
}

static void MountDirToShared(AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (property == nullptr) {
        return;
    }

    AppDacInfo *info = reinterpret_cast<AppDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    std::string varBundleName = ReplaceVarBundleName(property);
    if (info == NULL || varBundleName == "") {
        APPSPAWN_LOGE("Invalid app dac info or varBundleName");
        return;
    }

    MountEl1Bundle(property, info, varBundleName.c_str());

    if (IsUnlockStatus(info->uid)) {
        SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_UNLOCKED_STATUS);
        return;
    }

    MountSharedMap(property, info, varBundleName.c_str());
    MountStorageUsers(property, info, varBundleName.c_str());
    ParseDataGroupList(content, property, info, varBundleName.c_str());

    std::string lockSbxPathStamp = "/mnt/sandbox/" + std::to_string(info->uid / UID_BASE) + "/";
    lockSbxPathStamp += CheckAppMsgFlagsSet(property, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ? "isolated/" : "";
    lockSbxPathStamp += varBundleName;
    lockSbxPathStamp += "_locked";
    int ret = MakeDirRec(lockSbxPathStamp.c_str(), DIR_MODE, 1);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir %{public}s failed, errno %{public}d", lockSbxPathStamp.c_str(), errno);
    }
}
#endif

int MountToShared(AppSpawnMgr *content, const AppSpawningCtx *property)
{
#ifndef APPSPAWN_SANDBOX_NEW
    // mount dynamic directory to shared
    MountDirToShared(content, property);
#endif
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
#ifndef APPSPAWN_SANDBOX_NEW
    (void)AddServerStageHook(STAGE_SERVER_LOCK, HOOK_PRIO_COMMON, UpdateDataGroupDirs);
#endif
}
