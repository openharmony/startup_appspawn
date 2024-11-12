/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "securec.h"

#include "cJSON.h"
#include "appspawn_adapter.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"

#define APP_ENCAPS "encaps"
#define APP_OHOS_ENCAPS_COUNT_KEY "ohos.encaps.count"
#define APP_OHOS_ENCAPS_FORK_KEY "ohos.encaps.fork.count"

#define MSG_EXT_NAME_MAX_DECIMAL       10
#define OH_APP_MAX_PIDS_NUM            512
#define OH_ENCAPS_PROC_TYPE_BASE       0x18
#define OH_ENCAPS_PERMISSION_TYPE_BASE 0x1A
#define OH_ENCAPS_MAGIC                'E'
#define OH_PROC_HAP                    4
#define OH_ENCAPS_DEFAULT_FLAG         0
#define OH_ENCAPS_DEFAULT_STR          ""

#define SET_ENCAPS_PROC_TYPE_CMD _IOW(OH_ENCAPS_MAGIC, OH_ENCAPS_PROC_TYPE_BASE, uint32_t)
#define SET_ENCAPS_PERMISSION_TYPE_CMD _IOW(OH_ENCAPS_MAGIC, OH_ENCAPS_PERMISSION_TYPE_BASE, char *)

typedef enum {
    ENCAPS_PROC_TYPE_MODE,        // enable the encaps attribute of a process
    ENCAPS_PERMISSION_TYPE_MODE,  // set the encaps permission of a process
    ENCAPS_MAX_TYPE_MODE
} AppSpawnEncapsBaseType;

static int OpenEncapsFile(void)
{
    int fd = 0;
    fd = open("dev/encaps", O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGW("Failed to open encaps file errno: %{public}d", errno);
    }
    return fd;
}

static int WriteEncapsInfo(int fd, AppSpawnEncapsBaseType encapsType, const char *encapsInfo, uint32_t flag)
{
    if (encapsInfo == NULL) {
        return APPSPAWN_ARG_INVALID;
    }
    APPSPAWN_LOGV("root object: %{public}s", encapsInfo);

    int ret = 0;
    switch (encapsType) {
        case ENCAPS_PROC_TYPE_MODE:
            ret = ioctl(fd, SET_ENCAPS_PROC_TYPE_CMD, &flag);
            break;
        case ENCAPS_PERMISSION_TYPE_MODE:
            ret = ioctl(fd, SET_ENCAPS_PERMISSION_TYPE_CMD, encapsInfo);
            break;
        default:
            break;
    }
    if (ret != 0) {
        APPSPAWN_LOGE("Encaps the setup failed ret: %{public}d fd: %{public}d maxPid: %{public}s", ret, fd, encapsInfo);
        return ret;
    }
    return 0;
}

APPSPAWN_STATIC int EnableEncapsForProc(int encapsFileFd)
{
    uint32_t flag = OH_PROC_HAP;
    return WriteEncapsInfo(encapsFileFd, ENCAPS_PROC_TYPE_MODE, OH_ENCAPS_DEFAULT_STR, flag);
}

static uint32_t SpawnGetMaxPids(AppSpawnMgr *content, AppSpawningCtx *property)
{
    uint32_t len = 0;
    char *pidMaxStr = GetAppPropertyExt(property, MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX, &len);
    APPSPAWN_CHECK_ONLY_EXPER(pidMaxStr != NULL, return 0);
    uint32_t maxNum = 0;
    // string convert to value
    if (len != 0) {
        char *endPtr = NULL;
        maxNum = strtoul(pidMaxStr, &endPtr, MSG_EXT_NAME_MAX_DECIMAL);
        if (endPtr == pidMaxStr || *endPtr != '\0') {
            APPSPAWN_LOGW("Failed to convert a character string to a value.(ignore), endPtr: %{public}s", endPtr);
            return 0;
        }
        return maxNum;
    }
    return 0;
}

/* set ohos.encaps.fork.count to encaps */
static int SpawnSetMaxPids(AppSpawnMgr *content, AppSpawningCtx *property, cJSON *encaps)
{
    uint32_t maxPidCount = SpawnGetMaxPids(content, property);
    if (maxPidCount == 0 || maxPidCount > OH_APP_MAX_PIDS_NUM) {
        APPSPAWN_LOGV("Don't need to set pid max count. Use default pid max");
        return APPSPAWN_PIDMGR_DEFAULT_PID_MAX;
    }

    if (cJSON_AddNumberToObject(encaps, APP_OHOS_ENCAPS_FORK_KEY, maxPidCount) == NULL) {
        APPSPAWN_LOGV("Add number to object failed.(ignore)");
        return APPSPAWN_PIDMGR_DEFAULT_PID_MAX;
    }

    return 0;
}

static inline cJSON *GetJsonObjFromExtInfo(const AppSpawningCtx *property, const char *name)
{
    uint32_t size = 0;
    char *extInfo = (char *)(GetAppSpawnMsgExtInfo(property->message, name, &size));
    if (size == 0 || extInfo == NULL) {
        return NULL;
    }
    APPSPAWN_LOGV("Get json name %{public}s value %{public}s", name, extInfo);
    cJSON *extInfoJson = cJSON_Parse(extInfo);    // need to free
    APPSPAWN_CHECK(extInfoJson != NULL, return NULL, "Invalid ext info %{public}s for %{public}s", extInfo, name);
    return extInfoJson;
}

static int AddJITPermissionToEncaps(cJSON *extInfoJson, cJSON *encaps, uint32_t *permissionCount)
{
    // Get ohos.encaps.count
    cJSON *countJson = cJSON_GetObjectItem(extInfoJson, "ohos.encaps.count");
    int encapsCount = 0;
    if (cJSON_IsNumber(countJson)) {
        encapsCount = countJson->valueint;
    }

    // Check input count and permissions size
    cJSON *permissions = cJSON_GetObjectItemCaseSensitive(extInfoJson, "permissions");
    int count = cJSON_GetArraySize(permissions);
    if (encapsCount != count) {
        APPSPAWN_LOGE("Invalid args, encaps count: %{public}d, permission count: %{public}d", encapsCount, count);
        return APPSPAWN_ARG_INVALID;
    }

    // If permissionName is obtained, it needs to be written in the format of ["permissionName: "true""] in the encaps
    for (int i = 0; i < count; i++) {
        char *permissionName = cJSON_GetStringValue(cJSON_GetArrayItem(permissions, i));
        if (cJSON_AddStringToObject(encaps, permissionName, "true") == NULL) {
            APPSPAWN_LOGV("Add permission to object failed.(ignore)");
            return APPSPAWN_ERROR_UTILS_ADD_JSON_FAIL;
        }
    }
    *permissionCount += count;

    return 0;
}

static int SpawnSetJITPermissions(AppSpawnMgr *content, AppSpawningCtx *property, cJSON *encaps, uint32_t *count)
{
    cJSON *extInfoJson = GetJsonObjFromExtInfo(property, MSG_EXT_NAME_JIT_PERMISSIONS);
    if (extInfoJson == NULL) {
        return APPSPAWN_ARG_INVALID;
    }

    int ret = AddJITPermissionToEncaps(extInfoJson, encaps, count);
    if (ret != 0) {
        APPSPAWN_LOGW("Add permission to object failed.(ignore), ret: %{public}d", ret);
    }

    cJSON_Delete(extInfoJson);
    return ret;
}

static int AddMembersToEncaps(AppSpawnMgr *content, AppSpawningCtx *property, cJSON *encaps)
{
    uint32_t encapsPermissionCount = 0;
    // need set ohos.encaps.count to encaps firstly
    if (cJSON_AddNumberToObject(encaps, APP_OHOS_ENCAPS_COUNT_KEY, encapsPermissionCount) == NULL) {
        APPSPAWN_LOGV("Set ohos.encaps.count to object failed.(ignore)");
        return APPSPAWN_ERROR_UTILS_ADD_JSON_FAIL;
    }

    int ret = SpawnSetMaxPids(content, property, encaps);
    if (ret != 0) {
        APPSPAWN_LOGV("Can't set max pids to encaps object.(ignore), ret: %{public}d", ret);
    } else {
        encapsPermissionCount += 1;
    }

    uint32_t count = 0;
    ret = SpawnSetJITPermissions(content, property, encaps, &count);
    if (ret != 0) {
        APPSPAWN_LOGV("Can't set JIT permission to encaps object.(ignore), ret: %{public}d", ret);
    } else {
        encapsPermissionCount += count;
    }

    if (encapsPermissionCount == 0) {
        return APPSPAWN_ERROR_UTILS_ADD_JSON_FAIL; // Don't need set permission
    }

    cJSON *encapsCountItem = cJSON_GetObjectItem(encaps, APP_OHOS_ENCAPS_COUNT_KEY);
    if (encapsCountItem != NULL) {
        cJSON_SetNumberValue(encapsCountItem, encapsPermissionCount);
    }

    return 0;
}

static int SpawnBuildEncaps(AppSpawnMgr *content, AppSpawningCtx *property, char **encapsInfoStr)
{
    // Create root object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return APPSPAWN_ERROR_UTILS_CREATE_JSON_FAIL;
    }

    // Create encaps object
    cJSON *encaps = cJSON_CreateObject();
    if (encaps == NULL) {
        cJSON_Delete(root);
        return APPSPAWN_ERROR_UTILS_CREATE_JSON_FAIL;
    }

    int ret = AddMembersToEncaps(content, property, encaps);
    if (ret != 0) {
        APPSPAWN_LOGW("Add members to encaps object failed.(ignore), ret: %{public}d", ret);
        cJSON_Delete(root);
        cJSON_Delete(encaps);
        return ret;
    }

    if (cJSON_AddItemToObject(root, APP_ENCAPS, encaps) != true) {   // add encaps object to root
        cJSON_Delete(root);
        cJSON_Delete(encaps);
        APPSPAWN_LOGW("Add encaps object to root failed.(ignore)");
        return APPSPAWN_ERROR_UTILS_ADD_JSON_FAIL;
    }

    *encapsInfoStr = cJSON_PrintUnformatted(root);       // need to  free
    if (*encapsInfoStr == NULL) {
        cJSON_Delete(root);
        return APPSPAWN_ERROR_UTILS_DECODE_JSON_FAIL;
    }

    cJSON_Delete(root);
    return 0;
}

APPSPAWN_STATIC int SpawnSetEncapsPermissions(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == NULL || property == NULL) {
        return APPSPAWN_ARG_INVALID;
    }

    // The trustlist is used to control not appspawn
    if (!IsAppSpawnMode(content)) {
        return 0;
    }

    int encapsFileFd = OpenEncapsFile();
    if (encapsFileFd <= 0) {
        return 0;         // Not support encaps ability
    }

    int ret = EnableEncapsForProc(encapsFileFd);
    if (ret != 0) {
        return 0;         // Can't enable encaps ability
    }

    char *encapsInfoStr = NULL;
    ret = SpawnBuildEncaps(content, property, &encapsInfoStr);
    if (ret != 0) {
        APPSPAWN_LOGW("Build encaps object failed, ret: %{public}d", ret);
        return 0;        // Can't set permission encpas ability
    }

    (void)WriteEncapsInfo(encapsFileFd, ENCAPS_PERMISSION_TYPE_MODE, encapsInfoStr, OH_ENCAPS_DEFAULT_FLAG);

    if (encapsInfoStr != NULL) {
        free(encapsInfoStr);
    }
    close(encapsFileFd);

    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_COMMON, SpawnSetEncapsPermissions);
}