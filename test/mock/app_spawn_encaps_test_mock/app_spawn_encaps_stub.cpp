/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "app_spawn_stub.h"

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdbool>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>


#ifdef __cplusplus
extern "C" {
#endif

int OpenEncapsFile(void)
{
    return 0;
}

int WriteEncapsInfo(int fd, AppSpawnEncapsBaseType encapsType, const void *encapsInfo, uint32_t flag)
{
    if (fd == 0) {
        printf("WriteEncapsInfo fd is 0");
    }

    if (encapsType == nullptr) {
        printf("WriteEncapsInfo encapsType is null");
    }

    if (encapsInfo == nullptr) {
        printf("WriteEncapsInfo encapsInfo is null");
    }

    if (flag == 0) {
        printf("WriteEncapsInfo flag is 0");
    }

    return 0;
}

int EnableEncapsForProc(int encapsFileFd)
{
    if (encapsFileFd == 0) {
        printf("EnableEncapsForProc encapsFileFd is 0");
    }

    return 0;
}

uint32_t SpawnGetMaxPids(AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("SpawnGetMaxPids property is null");
    }

    return 0;
}

cJSON *GetJsonObjFromExtInfo(const AppSpawningCtx *property, const char *name)
{
    if (property == nullptr) {
        printf("GetJsonObjFromExtInfo property is null");
    }

    if (name == nullptr) {
        printf("GetJsonObjFromExtInfo name is null");
    }

    return nullptr;
}

int AddPermissionStrToValue(const char *valueStr, UserEncap *encap)
{
    if (valueStr == nullptr) {
        printf("AddPermissionStrToValue valueStr is null");
    }

    if (encap == nullptr) {
        printf("AddPermissionStrToValue encap is null");
    }

    return 0;
}

int AddPermissionIntArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize)
{
    if (arrayItem == nullptr) {
        printf("AddPermissionIntArrayToValue arrayItem is null");
    }

    if (encap == nullptr) {
        printf("AddPermissionIntArrayToValue encap is null");
    }

    if (arraySize == 0) {
        printf("AddPermissionIntArrayToValue arraySize is 0");
    }

    return 0;
}

int AddPermissionBoolArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize)
{
    if (arrayItem == nullptr) {
        printf("AddPermissionBoolArrayToValue arrayItem is null");
    }

    if (encap == nullptr) {
        printf("AddPermissionBoolArrayToValue encap is null");
    }

    if (arraySize == 0) {
        printf("AddPermissionBoolArrayToValue arraySize is 0");
    }

    return 0;
}

int AddPermissionStrArrayToValue(cJSON *arrayItem, UserEncap *encap)
{
    if (arrayItem == nullptr) {
        printf("AddPermissionStrArrayToValue arrayItem is null");
    }

    if (encap == nullptr) {
        printf("AddPermissionStrArrayToValue encap is null");
    }

    return 0;
}

int AddPermissionArrayToValue(cJSON *permissionItemArr, UserEncap *encap)
{
    if (permissionItemArr == nullptr) {
        printf("AddPermissionArrayToValue permissionItemArr is null");
    }

    if (encap == nullptr) {
        printf("AddPermissionArrayToValue encap is null");
    }

    return 0;
}

int AddPermissionItemToEncapsInfo(UserEncap *encap, cJSON *permissionItem)
{
    if (encap == nullptr) {
        printf("AddPermissionItemToEncapsInfo encap is null");
    }

    if (permissionItem == nullptr) {
        printf("AddPermissionItemToEncapsInfo permissionItem is null");
    }

    return 0;
}

cJSON *GetEncapsPermissions(cJSON *extInfoJson, int *count)
{
    if (extInfoJson == nullptr) {
        printf("GetEncapsPermissions extInfoJson is null");
    }

    if (count == nullptr) {
        printf("GetEncapsPermissions count is null");
    }

    return nullptr;
}

int AddMembersToEncapsInfo(cJSON *permissionsJson, UserEncaps *encapsInfo, int count)
{
    if (permissionsJson == nullptr) {
        printf("AddMembersToEncapsInfo permissionsJson is null");
    }

    if (encapsInfo == nullptr) {
        printf("AddMembersToEncapsInfo encapsInfo is null");
    }

    if (count == 0) {
        printf("AddMembersToEncapsInfo count is null");
    }

    return 0;
}

void FreeEncapsInfo(UserEncaps *encapsInfo)
{
    if (encapsInfo == nullptr) {
        printf("FreeEncapsInfo encapsInfo is null");
    }
}

int SpawnSetMaxPids(AppSpawningCtx *property, UserEncaps *encapsInfo)
{
    if (property == nullptr) {
        printf("SpawnSetMaxPids property is null");
    }

    if (encapsInfo == nullptr) {
        printf("SpawnSetMaxPids encapsInfo is null");
    }

    return 0;
}

int SpawnSetPermissions(AppSpawningCtx *property, UserEncaps *encapsInfo)
{
    if (property == nullptr) {
        printf("SpawnSetPermissions property is null");
    }

    if (encapsInfo == nullptr) {
        printf("SpawnSetPermissions encapsInfo is null");
    }

    return 0;
}

void SetAllowDumpable(AppSpawningCtx *property, UserEncaps *encapsInfo)
{
    if (property == nullptr) {
        printf("SetAllowDumpable property is null");
    }

    if (encapsInfo == nullptr) {
        printf("SetAllowDumpable encapsInfo is null");
    }
}

#ifdef __cplusplus
}
#endif
