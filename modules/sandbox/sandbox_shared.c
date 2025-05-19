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

#include "sandbox_shared.h"

#include <sys/mount.h>
#include "securec.h"

#include "appspawn_sandbox.h"
#include "appspawn_permission.h"
#include "appspawn_utils.h"
#include "parameter.h"

#define USER_ID_SIZE                16
#define DIR_MODE                    0711
#define LOCK_STATUS_SIZE            16

#define DATA_GROUP_SOCKET_TYPE      "DataGroup"
#define GROUPLIST_KEY_DATAGROUPID   "dataGroupId"
#define GROUPLIST_KEY_GID           "gid"
#define GROUPLIST_KEY_DIR           "dir"
#define GROUPLIST_KEY_UUID          "uuid"

static const MountSharedTemplate MOUNT_SHARED_MAP[] = {
    {"/data/storage/el2", NULL},
    {"/data/storage/el3", NULL},
    {"/data/storage/el4", NULL},
    {"/data/storage/el5", "ohos.permission.PROTECT_SCREEN_LOCK_DATA"},
};

static const DataGroupSandboxPathTemplate DATA_GROUP_SANDBOX_PATH_MAP[] = {
    {"el2", EL2, "/data/storage/el2/group/", NULL},
    {"el3", EL3, "/data/storage/el3/group/", NULL},
    {"el4", EL4, "/data/storage/el4/group/", NULL},
    {"el5", EL5, "/data/storage/el5/group/", "ohos.permission.PROTECT_SCREEN_LOCK_DATA"},
};

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

int GetElxInfoFromDir(const char *path)
{
    int ret = ELX_MAX;
    if (path == NULL) {
        return ret;
    }
    uint32_t count = ARRAY_LENGTH(DATA_GROUP_SANDBOX_PATH_MAP);
    for (uint32_t i = 0; i < count; ++i) {
        if (strstr(path, DATA_GROUP_SANDBOX_PATH_MAP[i].elxName) != NULL) {
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
        return NULL;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (DATA_GROUP_SANDBOX_PATH_MAP[i].category == category) {
            return &DATA_GROUP_SANDBOX_PATH_MAP[i];
        }
    }
    return NULL;
}

static bool IsUnlockStatus(uint32_t uid)
{
    const int userIdBase = UID_BASE;
    uid = uid / userIdBase;
    if (uid == 0) {
        return true;
    }
    char lockStatusParam[PARAM_BUFFER_SIZE] = {0};
    int ret = snprintf_s(lockStatusParam, PARAM_BUFFER_SIZE, PARAM_BUFFER_SIZE - 1, 
                         "startup.appspawn.lockstatus_%u", uid);
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
                   "Format lock status param failed, errno: %{public}d", errno);

    char userLockStatus[LOCK_STATUS_SIZE] = {0};
    ret = GetParameter(lockStatusParam, "1", userLockStatus, sizeof(userLockStatus));
    APPSPAWN_LOGI("Get param %{public}s %{public}s", lockStatusParam, userLockStatus);
    if (ret > 0 && (strcmp(userLockStatus, "0") == 0)) {   // 0:unlock status 1：lock status
        return true;
    }
    return false;
}

static bool SetSandboxPathShared(const char *sandboxPath)
{
    int ret = mount(NULL, sandboxPath, NULL, MS_SHARED, NULL);
    if (ret != 0) {
        APPSPAWN_LOGW("Need to mount %{public}s to shared, errno %{public}d", sandboxPath, errno);
        return false;
    }
    return true;
}

static int MountWithFileMgr(const AppDacInfo *info)
{
    /* /mnt/user/<currentUserId>/nosharefs/docs */
    char nosharefsDocsDir[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(nosharefsDocsDir, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/user/%u/nosharefs/docs",
                         info->uid / UID_BASE);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf nosharefsDocsDir failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    /* /mnt/sandbox/<currentUser/app-root/storage/Users */
    char storageUserPath[PATH_MAX_LEN] = {0};
    ret = snprintf_s(storageUserPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/sandbox/%u/app-root/storage/Users",
                     info->uid / UID_BASE);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf storageUserPath failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    // Check whether the directory is a shared mount point
    if (SetSandboxPathShared(storageUserPath)) {
        APPSPAWN_LOGV("shared mountpoint is exist");
        return 0;
    }

    ret = CreateSandboxDir(storageUserPath, DIR_MODE);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir %{public}s failed, errno %{public}d", storageUserPath, errno);
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    MountArg arg = {
        .originPath = nosharefsDocsDir,
        .destinationPath = storageUserPath,
        .fsType = NULL,
        .mountFlags = MS_BIND | MS_REC,
        .options = NULL,
        .mountSharedFlag = MS_SHARED
    };
    ret = SandboxMountPath(&arg);
    if (ret != 0) {
        APPSPAWN_LOGE("mount %{public}s shared failed, ret %{public}d", storageUserPath, ret);
    }
    return ret;
}

static int MountWithOther(const AppDacInfo *info)
{
    /* /mnt/user/<currentUserId>/sharefs/docs */
    char sharefsDocsDir[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(sharefsDocsDir, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/user/%u/sharefs/docs",
                         info->uid / UID_BASE);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf sharefsDocsDir failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    /* /mnt/sandbox/<currentUser/app-root/storage/Users */
    char storageUserPath[PATH_MAX_LEN] = {0};
    ret = snprintf_s(storageUserPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/sandbox/%u/app-root/storage/Users",
                     info->uid / UID_BASE);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf storageUserPath failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    // Check whether the directory is a shared mount point
    if (SetSandboxPathShared(storageUserPath)) {
        APPSPAWN_LOGV("shared mountpoint is exist");
        return 0;
    }

    ret = CreateSandboxDir(storageUserPath, DIR_MODE);
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

    MountArg arg = {
        .originPath = sharefsDocsDir,
        .destinationPath = storageUserPath,
        .fsType = "sharefs",
        .mountFlags = MS_NODEV,
        .options = options,
        .mountSharedFlag = MS_SHARED
    };
    ret = SandboxMountPath(&arg);
    if (ret != 0) {
        APPSPAWN_LOGE("mount %{public}s shared failed, ret %{public}d", storageUserPath, ret);
    }
    return ret;
}

static void MountStorageUsers(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const AppDacInfo *info)
{
    int ret = 0;
    int index = GetPermissionIndexInQueue(&sandbox->permissionQueue, FILE_ACCESS_MANAGER_MODE);
    int checkRes = CheckSandboxCtxPermissionFlagSet(context, (uint32_t)index);
    if (checkRes == 0) {
        /* mount /mnt/user/<currentUserId>/sharefs/docs to /mnt/sandbox/<currentUserId>/app-root/storage/Users */
        ret = MountWithOther(info);
    } else {
        /* mount /mnt/user/<currentUserId>/nosharefs/docs to /mnt/sandbox/<currentUserId>/app-root/storage/Users */
        ret = MountWithFileMgr(info);
    }
    if (ret != 0) {
        APPSPAWN_LOGE("Update %{public}s storage dir failed, ret %{public}d",
                      checkRes == 0 ? "sharefs dir" : "no sharefs dir", ret);
    } else {
        APPSPAWN_LOGI("Update %{public}s storage dir success", checkRes == 0 ? "sharefs dir" : "no sharefs dir");
    }
}

static int MountSharedMapItem(const char *bundleNamePath, const char *sandboxPathItem)
{
    /* /mnt/sandbox/<currentUserId>/<bundleName>/data/storage/el<x> */
    char sandboxPath[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(sandboxPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s",
                         bundleNamePath, sandboxPathItem);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf sandboxPath failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    // Check whether the directory is a shared mount point
    if (SetSandboxPathShared(sandboxPath)) {
        APPSPAWN_LOGV("shared mountpoint is exist");
        return 0;
    }

    ret = CreateSandboxDir(sandboxPath, DIR_MODE);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir %{public}s failed, errno %{public}d", sandboxPath, errno);
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    MountArg arg = {
        .originPath = sandboxPath,
        .destinationPath = sandboxPath,
        .fsType = NULL,
        .mountFlags = MS_BIND | MS_REC,
        .options = NULL,
        .mountSharedFlag = MS_SHARED
    };
    ret = SandboxMountPath(&arg);
    if (ret != 0) {
        APPSPAWN_LOGE("mount %{public}s shared failed, ret %{public}d", sandboxPath, ret);
    }
    return ret;
}

static void MountSharedMap(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const char *bundleNamePath)
{
    int length = sizeof(MOUNT_SHARED_MAP) / sizeof(MOUNT_SHARED_MAP[0]);
    for (int i = 0; i < length; i++) {
        if (MOUNT_SHARED_MAP[i].permission == NULL) {
            MountSharedMapItem(bundleNamePath, MOUNT_SHARED_MAP[i].sandboxPath);
        } else {
            int index = GetPermissionIndexInQueue(&sandbox->permissionQueue, MOUNT_SHARED_MAP[i].permission);
            APPSPAWN_LOGV("mount dir on lock mountPermissionFlags %{public}d", index);
            if (CheckSandboxCtxPermissionFlagSet(context, (uint32_t)index)) {
                MountSharedMapItem(bundleNamePath, MOUNT_SHARED_MAP[i].sandboxPath);
            }
        }
    }
    APPSPAWN_LOGI("mount shared map success");
}

static int DataGroupCtxNodeCompare(ListNode *node, void *data)
{
    DataGroupCtx *existingNode = (DataGroupCtx *)ListEntry(node, DataGroupCtx, node);
    DataGroupCtx *newNode = (DataGroupCtx *)data;
    if (existingNode == NULL || newNode == NULL) {
        APPSPAWN_LOGE("Invalid param");
        return APPSPAWN_ARG_INVALID;
    }

    // compare src path and sandbox path
    bool isSrcPathEqual = (strcmp(existingNode->srcPath.path, newNode->srcPath.path) == 0);
    bool isDestPathEqual = (strcmp(existingNode->destPath.path, newNode->destPath.path) == 0);

    return (isSrcPathEqual && isDestPathEqual) ? 0 : 1;
}

static int AddDataGroupItemToQueue(AppSpawnMgr *content, const char *srcPath, const char *destPath)
{
    DataGroupCtx *dataGroupNode = (DataGroupCtx *)calloc(1, sizeof(DataGroupCtx));
    APPSPAWN_CHECK(dataGroupNode != NULL, return APPSPAWN_ERROR_UTILS_MEM_FAIL, "Calloc dataGroupNode failed");
    if (strcpy_s(dataGroupNode->srcPath.path, PATH_MAX_LEN - 1, srcPath) != EOK ||
        strcpy_s(dataGroupNode->destPath.path, PATH_MAX_LEN - 1, destPath) != EOK) {
        APPSPAWN_LOGE("strcpy dataGroupNode path failed");
        free(dataGroupNode);
        dataGroupNode = NULL;
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }
    dataGroupNode->srcPath.pathLen = strlen(dataGroupNode->srcPath.path);
    dataGroupNode->destPath.pathLen = strlen(dataGroupNode->destPath.path);
    ListNode *node = OH_ListFind(&content->dataGroupCtxQueue, (void *)dataGroupNode, DataGroupCtxNodeCompare);
    if (node != NULL) {
        APPSPAWN_LOGI("DataGroupCtxNode %{public}s is exist", dataGroupNode->srcPath.path);
        free(dataGroupNode);
        dataGroupNode = NULL;
        return 0;
    }
    OH_ListInit(&dataGroupNode->node);
    OH_ListAddTail(&content->dataGroupCtxQueue, &dataGroupNode->node);
    return 0;
}

static inline cJSON *GetJsonObjFromExtInfo(const SandboxContext *context, const char *name)
{
    uint32_t size = 0;
    char *extInfo = (char *)(GetAppSpawnMsgExtInfo(context->message, name, &size));
    if (size == 0 || extInfo == NULL) {
        return NULL;
    }
    APPSPAWN_LOGV("Get json name %{public}s value %{public}s", name, extInfo);
    cJSON *extInfoJson = cJSON_Parse(extInfo);    // need to free
    APPSPAWN_CHECK(extInfoJson != NULL, return NULL, "Invalid ext info %{public}s for %{public}s", extInfo, name);
    return extInfoJson;
}

static void DumpDataGroupCtxQueue(const ListNode *front)
{
    if (front == NULL) {
        return;
    }

    uint32_t count = 0;
    ListNode *node = front->next;
    while (node != front && node != NULL) {
        DataGroupCtx *dataGroupNode = (DataGroupCtx *)ListEntry(node, DataGroupCtx, node);
        count++;
        APPSPAWN_LOGV("      ************************************** %{public}d", count);
        APPSPAWN_LOGV("      srcPath: %{public}s", dataGroupNode->srcPath.path);
        APPSPAWN_LOGV("      destPath: %{public}s", dataGroupNode->destPath.path);
        node = node->next;
    }
}

static int ParseDataGroupList(AppSpawnMgr *content, SandboxContext *context, const AppSpawnSandboxCfg *appSandbox,
                              const char *bundleNamePath)
{
    int ret = 0;
    cJSON *dataGroupList = GetJsonObjFromExtInfo(context, DATA_GROUP_SOCKET_TYPE);
    if (dataGroupList == NULL) {
        return APPSPAWN_ARG_INVALID;
    }

    // Iterate through the array (assuming groups is an array)
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, dataGroupList) {
        // Check if the item is valid
        APPSPAWN_CHECK(IsValidDataGroupItem(item), break, "Element is not a valid data group item");

        cJSON *dirItem = cJSON_GetObjectItemCaseSensitive(item, "dir");
        cJSON *uuidItem = cJSON_GetObjectItemCaseSensitive(item, "uuid");
        if (dirItem == NULL || !cJSON_IsString(dirItem) || uuidItem == NULL || !cJSON_IsString(uuidItem)) {
            APPSPAWN_LOGE("Data group element is invalid");
            break;
        }

        const char *srcPath = dirItem->valuestring;
        APPSPAWN_CHECK(!CheckPath(srcPath), break, "src path %{public}s is invalid", srcPath);

        int elxValue = GetElxInfoFromDir(srcPath);
        APPSPAWN_CHECK((elxValue >= EL2 && elxValue < ELX_MAX), break, "Get elx value failed");
        
        const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(elxValue);
        APPSPAWN_CHECK(templateItem != NULL, break, "Get data group arg template failed");

        // If permission isn't null, need check permission flag
        if (templateItem->permission != NULL) {
            int index = GetPermissionIndexInQueue(&appSandbox->permissionQueue, templateItem->permission);
            APPSPAWN_LOGV("mount dir no lock mount permission flag %{public}d", index);
            if (!CheckSandboxCtxPermissionFlagSet(context, (uint32_t)index)) {
                continue;
            }
        }
        // sandboxPath: /mnt/sandbox/<currentUserId>/<bundleName>/data/storage/el<x>/group/<uuid>
        char targetPath[PATH_MAX_LEN] = {0};
        int len = snprintf_s(targetPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s%s",
                             bundleNamePath, templateItem->sandboxPath, uuidItem->valuestring);
        APPSPAWN_CHECK(len > 0, break, "Failed to format targetPath");

        ret = AddDataGroupItemToQueue(content, srcPath, targetPath);
        if (ret != 0) {
            APPSPAWN_LOGE("Add datagroup item to dataGroupCtxQueue failed, el%{public}d", elxValue);
            OH_ListRemoveAll(&content->dataGroupCtxQueue, NULL);
            break;
        }
    }
    cJSON_Delete(dataGroupList);

    DumpDataGroupCtxQueue(&content->dataGroupCtxQueue);
    return ret;
}

int UpdateDataGroupDirs(AppSpawnMgr *content)
{
    if (content == NULL) {
        return APPSPAWN_ARG_INVALID;
    }

    int ret = 0;
    ListNode *node = content->dataGroupCtxQueue.next;
    while (node != &content->dataGroupCtxQueue && node != NULL) {
        DataGroupCtx *dataGroupNode = (DataGroupCtx *)ListEntry(node, DataGroupCtx, node);
        MountArg args = {
            .originPath = dataGroupNode->srcPath.path,
            .destinationPath = dataGroupNode->destPath.path,
            .fsType = NULL,
            .mountFlags = MS_BIND | MS_REC,
            .options = NULL,
            .mountSharedFlag = MS_SHARED
        };
        ret = SandboxMountPath(&args);
        if (ret != 0) {
            APPSPAWN_LOGE("Shared mount %{public}s to %{public}s failed, errno %{public}d", args.originPath,
                          args.destinationPath, ret);
        }
        node = node->next;
    }
    OH_ListRemoveAll(&content->dataGroupCtxQueue, NULL);
    return 0;
}

static int CreateSharedStamp(AppSpawnMsgDacInfo *info, SandboxContext *context)
{
    char lockSbxPathStamp[PATH_MAX_LEN] = {0};
    int ret = 0;
    if (CheckSandboxCtxMsgFlagSet(context, APP_FLAGS_ISOLATED_SANDBOX_TYPE) != 0) {
        ret = snprintf_s(lockSbxPathStamp, PATH_MAX_LEN, PATH_MAX_LEN - 1,
                         "/mnt/sandbox/%d/isolated/%s_locked", info->uid / UID_BASE, context->bundleName);
    } else {
        ret = snprintf_s(lockSbxPathStamp, PATH_MAX_LEN, PATH_MAX_LEN - 1,
                         "/mnt/sandbox/%d/%s_locked", info->uid / UID_BASE, context->bundleName);
    }
    if (ret <= 0) {
        APPSPAWN_LOGE("Failed to format lock sandbox path stamp");
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }
    ret = CreateSandboxDir(lockSbxPathStamp, DIR_MODE);
    if (ret != 0) {
        APPSPAWN_LOGE("Mkdir %{public}s failed, errno: %{public}d", lockSbxPathStamp, errno);
    }
    return ret;
}

int MountDirsToShared(AppSpawnMgr *content, SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (content == NULL || context == NULL || sandbox == NULL) {
        APPSPAWN_LOGE("Input paramters invalid");
        return APPSPAWN_SANDBOX_INVALID;
    }

    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSandboxCtxMsgInfo(context, TLV_DAC_INFO);
    AppSpawnMsgBundleInfo *bundleInfo = (AppSpawnMsgBundleInfo *)GetSandboxCtxMsgInfo(context, TLV_BUNDLE_INFO);
    if (info == NULL || bundleInfo == NULL) {
        APPSPAWN_LOGE("Info or bundleInfo invalid");
        return APPSPAWN_SANDBOX_INVALID;
    }

    if (IsUnlockStatus(info->uid)) {
        return 0;
    }

    /* /mnt/sandbox/<currentUserId>/<bundleName> */
    char bundleNamePath[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(bundleNamePath, PATH_MAX_LEN, PATH_MAX_LEN - 1,
                         "/mnt/sandbox/%u/%s", info->uid / UID_BASE, bundleInfo->bundleName);
    if (ret < 0) {
        APPSPAWN_LOGE("Failed to format lock sandbox path stamp");
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    MountSharedMap(context, sandbox, bundleNamePath);
    MountStorageUsers(context, sandbox, info);
    ParseDataGroupList(content, context, sandbox, bundleNamePath);

    ret = CreateSharedStamp(info, context);
    if (ret != 0) {
        APPSPAWN_LOGE("mkdir lockSbxPathStamp failed, ret: %{public}d", ret);
    }
    return ret;
}

MODULE_CONSTRUCTOR(void)
{
#ifdef APPSPAWN_SANDBOX_NEW
    (void)AddServerStageHook(STAGE_SERVER_LOCK, HOOK_PRIO_COMMON, UpdateDataGroupDirs);
#endif
}
