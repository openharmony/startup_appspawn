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
#include <sys/stat.h>
#include <vector>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <map>
#include <string>
#include "securec.h"

#include "sandbox_shared_mount.h"
#include "appspawn_mount_permission.h"
#include "appspawn_utils.h"
#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "parameter.h"

#define USER_ID_SIZE 16
#define DIR_MODE 0711
#define LOCK_STATUS_SIZE 16
#define LOCK_STATUS_UNLOCK "0"

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

bool IsValidDataGroupItem(cJSON *item)
{
    // Check if the item contains the specified key and if the value corresponding to the key is a string
    cJSON *datagroupId = cJSON_GetObjectItem(item, GROUPLIST_KEY_DATAGROUPID);
    cJSON *gid = cJSON_GetObjectItem(item, GROUPLIST_KEY_GID);
    cJSON *dir = cJSON_GetObjectItem(item, GROUPLIST_KEY_DIR);
    cJSON *uuid = cJSON_GetObjectItem(item, GROUPLIST_KEY_UUID);

    if (datagroupId && cJSON_IsString(datagroupId) &&
        gid && cJSON_IsString(gid) &&
        dir && cJSON_IsString(dir) &&
        uuid && cJSON_IsString(uuid)) {
        return true;
    }
    return false;
}

#ifndef APPSPAWN_SANDBOX_NEW
// Lock bundle info structure for _locked directory management
// Key is the complete _locked directory path (lockPath)
struct LockBundleInfo {
    uint32_t refCount;        // reference count
    std::string lockPath;     // _locked directory complete path (same as map key)
};

// Global map for managing lock bundle info, key is lockPath (complete _locked directory path)
APPSPAWN_STATIC std::map<std::string, LockBundleInfo> g_lockBundleMap;

/**
 * @brief Build lockPath from pre-built varBundleName (simpler version)
 * @param uid User ID
 * @param varBundleName Pre-built variable bundle name
 * @param msgFlags Message flags (contains APP_FLAGS_ISOLATED_SANDBOX_TYPE etc.)
 * @return Complete _locked directory path
 */
static std::string BuildLockPath(uint32_t uid, const std::string &varBundleName,
    const AppSpawnMsgFlags *msgFlags)
{
    std::string lockSbxPathStamp = "/mnt/sandbox/" + std::to_string(uid / UID_BASE) + "/";
    lockSbxPathStamp += CheckAppSpawnMsgFlagsSet(msgFlags, APP_FLAGS_ISOLATED_SANDBOX_TYPE) ?
        "isolated/" : "";
    lockSbxPathStamp += varBundleName;
    lockSbxPathStamp += "_locked";
    return lockSbxPathStamp;
}

/**
 * @brief Build lockPath from app information (full version for cases without pre-built varBundleName)
 * @param uid User ID
 * @param bundleName Bundle name (original bundle name without clone suffix)
 * @param appIndex Bundle index (0 for non-clone, >0 for clone)
 * @param msgFlags Message flags (contains APP_FLAGS_ISOLATED_SANDBOX_TYPE etc.)
 * @return Complete _locked directory path
 */
static std::string BuildLockPath(uint32_t uid, const std::string &bundleName,
    uint32_t appIndex, const AppSpawnMsgFlags *msgFlags)
{
    // Build varBundleName from bundleName and appIndex
    std::string varBundleName = bundleName;
    APPSPAWN_ONLY_EXPER(CheckAppSpawnMsgFlagsSet(msgFlags, APP_FLAGS_CLONE_ENABLE) && appIndex > 0,
        std::ostringstream oss;
        oss << "+clone-" << appIndex << "+" << bundleName;
        varBundleName = oss.str());

    return BuildLockPath(uid, varBundleName, msgFlags);
}

APPSPAWN_STATIC int AddLockBundleRef(const std::string &lockPath)
{
    APPSPAWN_ONLY_EXPER(lockPath.empty(), return -1);
    auto it = g_lockBundleMap.find(lockPath);
    if (it == g_lockBundleMap.end()) {
        // First time, initialize structure
        LockBundleInfo info;
        info.refCount = 1;
        info.lockPath = lockPath;
        g_lockBundleMap[lockPath] = info;
        APPSPAWN_LOGI("AddLockBundleRef: new lockPath %{public}s, refCount=1", lockPath.c_str());
    } else {
        // Already exists, increase count
        it->second.refCount++;
        APPSPAWN_LOGI("AddLockBundleRef: lockPath %{public}s refCount=%{public}u",
                      lockPath.c_str(), it->second.refCount);
    }
    return 0;
}

APPSPAWN_STATIC void ReleaseLockBundleRef(const std::string &lockPath)
{
    APPSPAWN_ONLY_EXPER(lockPath.empty(), return);

    auto it = g_lockBundleMap.find(lockPath);
    APPSPAWN_CHECK_LOGW(it != g_lockBundleMap.end(), return,
        "ReleaseLockBundleRef: lockPath %{public}s not found", lockPath.c_str());

    // Prevent refCount underflow
    APPSPAWN_CHECK(it->second.refCount > 0, return,
        "ReleaseLockBundleRef: refCount underflow for %{public}s, current refCount=%{public}u",
        lockPath.c_str(), it->second.refCount);

    it->second.refCount--;
    APPSPAWN_LOGI("ReleaseLockBundleRef: lockPath %{public}s refCount=%{public}u",
                  lockPath.c_str(), it->second.refCount);
    APPSPAWN_ONLY_EXPER(it->second.refCount != 0, return);
    // Count is 0, delete the _locked directory
    int ret = rmdir(lockPath.c_str());
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "Remove _locked dir %{public}s failed, errno %{public}d",
        lockPath.c_str(), errno);
    // Remove from map
    g_lockBundleMap.erase(it);
}

APPSPAWN_STATIC bool IsUnlockStatus(uint32_t uid)
{
    const int userIdBase = UID_BASE;
    uid = uid / userIdBase;
    if (uid == 0) {
        return true;
    }
    std::string lockStatusParam = "startup.appspawn.lockstatus_" + std::to_string(uid);
    char userLockStatus[LOCK_STATUS_SIZE] = {0};
    int ret = GetParameter(lockStatusParam.c_str(), "1", userLockStatus, sizeof(userLockStatus));
    APPSPAWN_CHECK_DUMPI(ret > 0 && strcmp(userLockStatus, LOCK_STATUS_UNLOCK) == 0, return false,
        "lockStatus %{public}u %{public}s", uid, userLockStatus);
    return true;
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
    APPSPAWN_DUMPI("mount path %{public}s to shared success", arg->destPath);
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
    APPSPAWN_DUMPI("mount shared map success");
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

static inline cJSON *GetJsonObjFromExtInfo(const AppSpawningCtx *property, const char *name)
{
    uint32_t size = 0;
    char *extInfo = (char *)(GetAppSpawnMsgExtInfo(property->message, name, &size));
    if (size == 0 || extInfo == nullptr) {
        return nullptr;
    }
    APPSPAWN_LOGV("Get json name %{public}s value %{public}s", name, extInfo);
    cJSON *extInfoJson = cJSON_Parse(extInfo);    // need to free
    APPSPAWN_CHECK(extInfoJson != nullptr, return nullptr, "Invalid ext info %{public}s for %{public}s", extInfo, name);
    return extInfoJson;
}

static int ParseDataGroupList(AppSpawnMgr *content, const AppSpawningCtx *property, AppDacInfo *info,
                              const char *varBundleName)
{
    int ret = 0;
    cJSON *dataGroupList = GetJsonObjFromExtInfo(property, DATA_GROUP_SOCKET_TYPE);
    APPSPAWN_CHECK_LOGV(dataGroupList != nullptr, return 0, "dataGroupList is empty");
    APPSPAWN_CHECK(cJSON_IsArray(dataGroupList), cJSON_Delete(dataGroupList);
        return APPSPAWN_ARG_INVALID, "dataGroupList is not Array");

    // Iterate through the array (assuming groups is an array)
    cJSON *item = nullptr;
    cJSON_ArrayForEach(item, dataGroupList) {
        // Check if the item is valid
        APPSPAWN_CHECK((item != nullptr && IsValidDataGroupItem(item)), break,
                       "Element is not a valid data group item");

        cJSON *dirItem = cJSON_GetObjectItemCaseSensitive(item, "dir");
        cJSON *uuidItem = cJSON_GetObjectItemCaseSensitive(item, "uuid");
        if (dirItem == nullptr || !cJSON_IsString(dirItem) || uuidItem == nullptr || !cJSON_IsString(uuidItem)) {
            APPSPAWN_LOGE("Data group element is invalid");
            break;
        }

        const char *srcPath = dirItem->valuestring;
        APPSPAWN_CHECK(!CheckPath(srcPath), break, "src path %{public}s is invalid", srcPath);

        int elxValue = GetElxInfoFromDir(srcPath);
        APPSPAWN_CHECK((elxValue >= EL2 && elxValue < ELX_MAX), break, "Get elx value failed");

        const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(elxValue);
        APPSPAWN_CHECK(templateItem != nullptr, break, "Get data group arg template failed");

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

        ret = AddDataGroupItemToQueue(content, srcPath, sandboxPath, uuidItem->valuestring);
        if (ret != 0) {
            APPSPAWN_LOGE("Add datagroup item to dataGroupCtxQueue failed, el%{public}d", elxValue);
            OH_ListRemoveAll(&content->dataGroupCtxQueue, nullptr);
            break;
        }
    }
    cJSON_Delete(dataGroupList);

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
            APPSPAWN_LOGE("Shared mount %{public}s to %{public}s failed, ret %{public}d", args.srcPath,
                          sandboxPath, ret);
        }
        node = node->next;
    }
    OH_ListRemoveAll(&content->dataGroupCtxQueue, nullptr);
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
    const char *bundleName = GetBundleName(property);
    APPSPAWN_ONLY_EXPER(info == nullptr || varBundleName == "" || bundleName == nullptr,
        APPSPAWN_LOGE("Invalid app dac info or varBundleName");
        return);

    if (IsUnlockStatus(info->uid)) {
        SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_UNLOCKED_STATUS);
        return;
    }

    MountSharedMap(property, info, varBundleName.c_str());
    ParseDataGroupList(content, property, info, varBundleName.c_str());

    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(
        GetAppSpawnMsgInfo(property->message, TLV_MSG_FLAGS));
    std::string lockSbxPathStamp = BuildLockPath(info->uid, varBundleName, msgFlags);
    int ret = MakeDirRec(lockSbxPathStamp.c_str(), DIR_MODE, 1);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir %{public}s failed, errno %{public}d", lockSbxPathStamp.c_str(), errno);
    }
    // Add reference count for _locked directory, use lockPath as key
    AddLockBundleRef(lockSbxPathStamp);
    // Set flag to indicate that AddLockBundleRef has been called (use const_cast to modify)
    const_cast<AppSpawningCtx *>(property)->lockBundleRefAdded = true;
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

#ifndef APPSPAWN_SANDBOX_NEW
static int AppCleanupHook(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo)
{
    APPSPAWN_ONLY_EXPER(appInfo == nullptr || IsNWebSpawnMode(content),
        return 0);
    // Only process when uid > 0 and device is locked
    APPSPAWN_ONLY_EXPER(IsUnlockStatus(appInfo->uid) || !appInfo->lockBundleRefAdded, return 0);

    // Build lockPath from appInfo
    std::string lockPath = BuildLockPath(appInfo->uid, std::string(appInfo->name),
        appInfo->appIndex, appInfo->msgFlags);
    APPSPAWN_LOGI("AppCleanupHook: cleanup lockPath=%{public}s", lockPath.c_str());
    ReleaseLockBundleRef(lockPath);
    // Clear flag to prevent double release (redundant but safe)
    const_cast<AppSpawnedProcessInfo *>(appInfo)->lockBundleRefAdded = false;
    return 0;
}

static int SpawnFailedHook(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_ONLY_EXPER(property == nullptr || IsNWebSpawnMode(content),
        return 0);

    // Get uid, appIndex and msgFlags from property
    AppDacInfo *dacInfo = reinterpret_cast<AppDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    uint32_t uid = dacInfo != nullptr ? dacInfo->uid : 0;

    // Only process when uid > 0 and device is locked
    APPSPAWN_ONLY_EXPER(IsUnlockStatus(uid) || !property->lockBundleRefAdded, return 0);

    // Only process when uid > 0 and device is locked
    const char *bundleName = GetBundleName(property);
    APPSPAWN_CHECK(bundleName != nullptr, return 0,
        "SpawnFailedHook: failed to get bundle name");

    uint32_t appIndex = 0;
    AppSpawnMsgBundleInfo *bundleInfo = reinterpret_cast<AppSpawnMsgBundleInfo *>(
        GetAppProperty(property, TLV_BUNDLE_INFO));
    APPSPAWN_ONLY_EXPER(bundleInfo != nullptr, appIndex = bundleInfo->bundleIndex);

    // Get complete msgFlags
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(
        GetAppSpawnMsgInfo(property->message, TLV_MSG_FLAGS));

    std::string lockPath = BuildLockPath(uid, std::string(bundleName), appIndex, msgFlags);
    APPSPAWN_LOGV("SpawnFailedHook: fork failed, release lock ref for lockPath=%{public}s", lockPath.c_str());
    ReleaseLockBundleRef(lockPath);
    // Clear flag to prevent double release
    property->lockBundleRefAdded = false;
    return 0;
}

#endif

MODULE_CONSTRUCTOR(void)
{
#ifndef APPSPAWN_SANDBOX_NEW
    AddProcessMgrHook(STAGE_SERVER_APP_CLEANUP, HOOK_PRIO_COMMON, AppCleanupHook);
    AddAppSpawnHook(STAGE_SERVER_SPAWN_ABORT, HOOK_PRIO_COMMON, SpawnFailedHook);
    (void)AddServerStageHook(STAGE_SERVER_LOCK, HOOK_PRIO_COMMON, UpdateDataGroupDirs);
    APPSPAWN_LOGI("RegisterLockBundleHooks: hooks registered");
#endif
}

