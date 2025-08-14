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
#include "appspawn_encaps.h"

#define APP_ENCAPS "encaps"
#define APP_OHOS_ENCAPS_COUNT_KEY "ohos.encaps.count"
#define APP_OHOS_ENCAPS_FORK_KEY "ohos.encaps.fork.count"
#define APP_OHOS_ENCAPS_PERMISSIONS_KEY "permissions"

#define MSG_EXT_NAME_MAX_DECIMAL       10
#define OH_APP_MAX_PIDS_NUM            512
#define OH_ENCAPS_PROC_TYPE_BASE       0x18
#define OH_ENCAPS_PERMISSION_TYPE_BASE 0x1E
#define OH_ENCAPS_MAGIC                'E'
#define OH_PROC_HAP                    4
#define OH_ENCAPS_DEFAULT_FLAG         0
#define OH_ENCAPS_DEFAULT_STR          ""
// permission value max len is 512
#define OH_ENCAPS_VALUE_MAX_LEN 512
// encapsCount max count is 64
#define OH_ENCAPS_MAX_COUNT 64

#define SET_ENCAPS_PROC_TYPE_CMD _IOW(OH_ENCAPS_MAGIC, OH_ENCAPS_PROC_TYPE_BASE, uint32_t)
#define SET_ENCAPS_PERMISSION_TYPE_CMD _IOW(OH_ENCAPS_MAGIC, OH_ENCAPS_PERMISSION_TYPE_BASE, UserEncaps)

APPSPAWN_STATIC int OpenEncapsFile(void)
{
    int fd = 0;
    fd = open("/dev/encaps", O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGW("Failed to open encaps file errno: %{public}d", errno);
    }
    return fd;
}

APPSPAWN_STATIC int WriteEncapsInfo(int fd, AppSpawnEncapsBaseType encapsType, const void *encapsInfo, uint32_t flag)
{
    if (encapsInfo == NULL) {
        return APPSPAWN_ARG_INVALID;
    }

    int ret = 0;
    switch (encapsType) {
        case ENCAPS_PROC_TYPE_MODE:
            ret = ioctl(fd, SET_ENCAPS_PROC_TYPE_CMD, &flag);
            break;
        case ENCAPS_PERMISSION_TYPE_MODE:
            ret = ioctl(fd, SET_ENCAPS_PERMISSION_TYPE_CMD, encapsInfo);
            break;
        default:
            ret = APPSPAWN_ARG_INVALID;
            break;
    }
    if (ret != 0) {
        APPSPAWN_LOGE("Encaps the setup failed ret: %{public}d fd: %{public}d", ret, fd);
        return ret;
    }
    return 0;
}

APPSPAWN_STATIC int EnableEncapsForProc(int encapsFileFd)
{
    uint32_t flag = OH_PROC_HAP;
    return WriteEncapsInfo(encapsFileFd, ENCAPS_PROC_TYPE_MODE, OH_ENCAPS_DEFAULT_STR, flag);
}

APPSPAWN_STATIC uint32_t SpawnGetMaxPids(AppSpawningCtx *property)
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

APPSPAWN_STATIC int AddPermissionStrToValue(const char *valueStr, UserEncap *encap)
{
    APPSPAWN_CHECK(valueStr != NULL, return APPSPAWN_ARG_INVALID, "Invalid string value");
    uint32_t valueLen = strlen(valueStr) + 1;
    APPSPAWN_CHECK(valueLen > 1 && valueLen <= OH_ENCAPS_VALUE_MAX_LEN, return APPSPAWN_ARG_INVALID,
        "String value len is invalied, len: %{public}u", valueLen);
    char *value = (char *)calloc(1, valueLen);
    APPSPAWN_CHECK(value != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to calloc value");

    int ret = strcpy_s(value, valueLen, valueStr);
    APPSPAWN_CHECK(ret == EOK, free(value);
        return APPSPAWN_SYSTEM_ERROR, "Failed to copy string value");

    encap->value.ptrValue = (void *)value;
    encap->valueLen = valueLen;
    encap->type = ENCAPS_CHAR_ARRAY;
    return 0;
}

APPSPAWN_STATIC int AddPermissionIntArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize)
{
    uint32_t valueLen = sizeof(int) * arraySize;
    APPSPAWN_CHECK(valueLen <= OH_ENCAPS_VALUE_MAX_LEN, return APPSPAWN_ARG_INVALID,
        "Int array len too long, len: %{public}u", valueLen);
    int *value = (int *)calloc(1, valueLen);
    APPSPAWN_CHECK(value != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to calloc int array value");

    cJSON *arrayItemTemp = arrayItem;
    for (size_t index = 0; index < arraySize; index++) {
        if (arrayItemTemp == NULL || !cJSON_IsNumber(arrayItemTemp)) {
            free(value);
            APPSPAWN_LOGE("Invalid int array item type");
            return APPSPAWN_ARG_INVALID;
        }
        value[index] = arrayItemTemp->valueint;
        arrayItemTemp = arrayItemTemp->next;
    }
    encap->value.ptrValue = (void *)value;
    encap->valueLen = valueLen;
    encap->type = ENCAPS_INT_ARRAY;
    return 0;
}

APPSPAWN_STATIC int AddPermissionBoolArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize)
{
    uint32_t valueLen = sizeof(bool) * arraySize;
    APPSPAWN_CHECK(valueLen <= OH_ENCAPS_VALUE_MAX_LEN, return APPSPAWN_ARG_INVALID,
        "Bool array len too long, len: %{public}u", valueLen);
    bool *value = (bool *)calloc(1, valueLen);
    APPSPAWN_CHECK(value != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to calloc bool array value");

    cJSON *arrayItemTemp = arrayItem;
    for (size_t index = 0; index < arraySize; index++) {
        if (arrayItemTemp == NULL || !cJSON_IsBool(arrayItemTemp)) {
            free(value);
            APPSPAWN_LOGE("Invalid bool array item type");
            return APPSPAWN_ARG_INVALID;
        }
        value[index] = cJSON_IsTrue(arrayItemTemp) ? true : false;
        arrayItemTemp = arrayItemTemp->next;
    }
    encap->value.ptrValue = (void *)value;
    encap->valueLen = valueLen;
    encap->type = ENCAPS_BOOL_ARRAY;
    return 0;
}

APPSPAWN_STATIC int AddPermissionStrArrayToValue(cJSON *arrayItem, UserEncap *encap)
{
    uint32_t valueLen = 0;
    for (cJSON *arrayItemTemp = arrayItem; arrayItemTemp != NULL; arrayItemTemp = arrayItemTemp->next) {
        if (!cJSON_IsString(arrayItemTemp) || arrayItemTemp->valuestring == NULL) {
            APPSPAWN_LOGE("Invalid string array item type");
            return APPSPAWN_ARG_INVALID;
        }
        uint32_t tempLen = strlen(arrayItemTemp->valuestring);
        APPSPAWN_CHECK(tempLen > 0, return APPSPAWN_ARG_INVALID, "String array value is invalied");
        valueLen += tempLen + 1;
    }

    APPSPAWN_CHECK(valueLen > 0 && valueLen <= OH_ENCAPS_VALUE_MAX_LEN, return APPSPAWN_ARG_INVALID,
        "String array len is invalied, len: %{public}u", valueLen);
    char *value = (char *)calloc(1, valueLen);
    APPSPAWN_CHECK(value != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to calloc string array value");

    char *valuePtr = value;
    for (cJSON *arrayItemTemp = arrayItem; arrayItemTemp != NULL; arrayItemTemp = arrayItemTemp->next) {
        int len = strlen(arrayItemTemp->valuestring) + 1;
        int ret = strcpy_s(valuePtr, len, arrayItemTemp->valuestring);
        APPSPAWN_CHECK(ret == EOK, free(value);
            return APPSPAWN_SYSTEM_ERROR, "Failed to copy string value");
        valuePtr += len;
    }
    encap->value.ptrValue = (void *)value;
    encap->valueLen = valueLen;
    encap->type = ENCAPS_CHAR_ARRAY;
    return 0;
}

APPSPAWN_STATIC int AddPermissionArrayToValue(cJSON *permissionItemArr, UserEncap *encap)
{
    uint32_t arraySize = (uint32_t)cJSON_GetArraySize(permissionItemArr);
    if (arraySize == 0) {
        return APPSPAWN_ARG_INVALID;
    }

    // check first item type
    cJSON *arrayItem = permissionItemArr->child;
    if (cJSON_IsNumber(arrayItem)) {
        if (AddPermissionIntArrayToValue(arrayItem, encap, arraySize) != 0) {
            return APPSPAWN_ARG_INVALID;
        }
    } else if (cJSON_IsString(arrayItem)) {
        if (AddPermissionStrArrayToValue(arrayItem, encap) != 0) {
            return APPSPAWN_ARG_INVALID;
        }
    } else if (cJSON_IsBool(arrayItem)) {
        if (AddPermissionBoolArrayToValue(arrayItem, encap, arraySize) != 0) {
            return APPSPAWN_ARG_INVALID;
        }
    } else {
        APPSPAWN_LOGW("Invalid array item type");
        return APPSPAWN_ARG_INVALID;
    }

    return 0;
}

APPSPAWN_STATIC int AddPermissionItemToEncapsInfo(UserEncap *encap, cJSON *permissionItem)
{
    // copy json key
    char *key = permissionItem->string;
    if (key == NULL || strcpy_s(encap->key, OH_ENCAPS_KEY_MAX_LEN, key) != EOK) {
        APPSPAWN_LOGE("Failed to copy json key");
        return APPSPAWN_SYSTEM_ERROR;
    }

    if (cJSON_IsNumber(permissionItem)) {
        encap->type = ENCAPS_INT;
        encap->value.intValue = (uint64_t)permissionItem->valueint;
        encap->valueLen = sizeof(permissionItem->valueint);
    } else if (cJSON_IsBool(permissionItem)) {
        encap->type = ENCAPS_BOOL;
        bool value = cJSON_IsTrue(permissionItem) ? true : false;
        encap->value.intValue = (uint64_t)value;
        encap->valueLen = sizeof(value);
    } else if (cJSON_IsString(permissionItem)) {
        if (AddPermissionStrToValue(permissionItem->valuestring, encap) != 0) {
            return APPSPAWN_ARG_INVALID;
        }
    } else if (cJSON_IsArray(permissionItem))  {
        if (AddPermissionArrayToValue(permissionItem, encap) != 0) {
            return APPSPAWN_ARG_INVALID;
        }
    } else {
        APPSPAWN_LOGW("Invalid permission item type");
        return APPSPAWN_ARG_INVALID;
    }

    return 0;
}

APPSPAWN_STATIC int AddMembersToEncapsInfo(cJSON *extInfoJson, UserEncaps *encapsInfo)
{
    // Get ohos.encaps.count
    cJSON *countJson = cJSON_GetObjectItem(extInfoJson, APP_OHOS_ENCAPS_COUNT_KEY);
    APPSPAWN_CHECK(countJson != NULL && cJSON_IsNumber(countJson), return APPSPAWN_ARG_INVALID, "Invalid countJson");
    int encapsCount = countJson->valueint;

    // Check input count and permissions size
    cJSON *permissionsJson = cJSON_GetObjectItemCaseSensitive(extInfoJson, APP_OHOS_ENCAPS_PERMISSIONS_KEY);
    APPSPAWN_CHECK(permissionsJson != NULL && cJSON_IsArray(permissionsJson), return APPSPAWN_ARG_INVALID,
        "Invalid permissionsJson");
    int count = cJSON_GetArraySize(permissionsJson);
    APPSPAWN_CHECK(count > 0 && count <= OH_ENCAPS_MAX_COUNT && encapsCount == count, return APPSPAWN_ARG_INVALID,
        "Invalid args, encaps count: %{public}d, permission count: %{public}d", encapsCount, count);

    encapsInfo->encap = (UserEncap *)calloc(count + 1, sizeof(UserEncap));
    APPSPAWN_CHECK(encapsInfo->encap != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to calloc encap");

    for (int i = 0; i < count; i++) {
        cJSON *permission = cJSON_GetArrayItem(permissionsJson, i);
        APPSPAWN_CHECK(permission != NULL, return APPSPAWN_ERROR_UTILS_DECODE_JSON_FAIL,
            "Encaps get single permission failed")

        cJSON *permissionItem = permission->child;
        APPSPAWN_CHECK(permissionItem != NULL, return APPSPAWN_ERROR_UTILS_DECODE_JSON_FAIL,
            "Encaps get permission item failed")

        if (AddPermissionItemToEncapsInfo(&encapsInfo->encap[i], permissionItem) != 0) {
            APPSPAWN_LOGE("Add permission to encap failed");
            return APPSPAWN_ERROR_UTILS_ADD_JSON_FAIL;
        }
        encapsInfo->encapsCount++;
    }

    return 0;
}

APPSPAWN_STATIC void FreeEncapsInfo(UserEncaps *encapsInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(encapsInfo != NULL, return);

    if (encapsInfo->encap != NULL) {
        for (uint32_t i = 0; i < encapsInfo->encapsCount; i++) {
            if (encapsInfo->encap[i].type > ENCAPS_AS_ARRAY) {
                free(encapsInfo->encap[i].value.ptrValue);
                encapsInfo->encap[i].value.ptrValue = NULL;
            }
        }
        free(encapsInfo->encap);
        encapsInfo->encap = NULL;
    }
}

/* set ohos.encaps.fork.count to encaps */
static int SpawnSetMaxPids(AppSpawningCtx *property, UserEncaps *encapsInfo)
{
    uint32_t maxPidCount = 0;
    if (GetAppSpawnMsgType(property) != MSG_SPAWN_NATIVE_PROCESS) {
        maxPidCount = SpawnGetMaxPids(property);
    }

    APPSPAWN_CHECK(maxPidCount > 0 && maxPidCount < OH_APP_MAX_PIDS_NUM, return 0,
        "Don't need to set pid max count %{public}u. Use default pid max", maxPidCount);
    APPSPAWN_CHECK(encapsInfo->encapsCount < OH_ENCAPS_MAX_COUNT,
        return APPSPAWN_ARG_INVALID, "Encaps count is more than 64, cannot set permissions");

    uint32_t count = encapsInfo->encapsCount;
    int ret = strcpy_s(encapsInfo->encap[count].key, OH_ENCAPS_KEY_MAX_LEN, APP_OHOS_ENCAPS_FORK_KEY);
    APPSPAWN_CHECK_ONLY_EXPER(ret == EOK, return APPSPAWN_SYSTEM_ERROR);

    encapsInfo->encap[count].value.intValue = (uint64_t)maxPidCount;
    encapsInfo->encap[count].valueLen = sizeof(maxPidCount);
    encapsInfo->encap[count].type = ENCAPS_INT;
    encapsInfo->encapsCount++;
    return 0;
}

APPSPAWN_STATIC int SpawnSetPermissions(AppSpawningCtx *property, UserEncaps *encapsInfo)
{
    // Get Permissions obejct
    cJSON *extInfoJson = GetJsonObjFromExtInfo(property, MSG_EXT_NAME_JIT_PERMISSIONS);
    if (extInfoJson == NULL) {
        APPSPAWN_LOGV("GetJsonObjFromExtInfo failed");
        return APPSPAWN_ARG_INVALID;
    }

    int ret = AddMembersToEncapsInfo(extInfoJson, encapsInfo);
    if (ret != 0) {
        APPSPAWN_LOGW("Add member to encaps failed, ret: %{public}d", ret);
        cJSON_Delete(extInfoJson);
        return ret;
    }

    ret = SpawnSetMaxPids(property, encapsInfo);
    APPSPAWN_CHECK(ret == 0, cJSON_Delete(extInfoJson);
        return ret, "Set max pids count to encaps failed");

    cJSON_Delete(extInfoJson);
    return 0;
}

APPSPAWN_STATIC int SpawnSetEncapsPermissions(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == NULL || property == NULL) {
        return APPSPAWN_ARG_INVALID;
    }

    // The trustlist is used to control not appspawn or nativespawn
    if (!(IsAppSpawnMode(content) || IsHybridSpawnMode(content) || IsNativeSpawnMode(content))) {
        return 0;
    }

    int encapsFileFd = OpenEncapsFile();
    if (encapsFileFd <= 0) {
        return 0;         // Not support encaps ability
    }

    int ret = EnableEncapsForProc(encapsFileFd);
    if (ret != 0) {
        close(encapsFileFd);
        return 0;         // Can't enable encaps ability
    }

    UserEncaps encapsInfo = {0};
    ret = SpawnSetPermissions(property, &encapsInfo);
    if (ret != 0) {
        close(encapsFileFd);
        FreeEncapsInfo(&encapsInfo);
        APPSPAWN_LOGV("Build encaps info failed, ret: %{public}d", ret);
        return 0;        // Can't set permission encpas ability
    }

    (void)WriteEncapsInfo(encapsFileFd, ENCAPS_PERMISSION_TYPE_MODE, &encapsInfo, OH_ENCAPS_DEFAULT_FLAG);
    APPSPAWN_LOGV("Set encaps info finish");

    FreeEncapsInfo(&encapsInfo);
    close(encapsFileFd);

    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_COMMON, SpawnSetEncapsPermissions);
}