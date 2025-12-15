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
    return 0;
}

int EnableEncapsForProc(int encapsFileFd)
{
    return 0;
}

uint32_t SpawnGetMaxPids(AppSpawningCtx *property)
{
    return 0;
}

cJSON *GetJsonObjFromExtInfo(const AppSpawningCtx *property, const char *name)
{
    return nullptr;
}

int AddPermissionStrToValue(const char *valueStr, UserEncap *encap)
{
    return 0;
}

int AddPermissionIntArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize)
{
    return 0;
}

int AddPermissionBoolArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize)
{
    return 0;
}

int AddPermissionStrArrayToValue(cJSON *arrayItem, UserEncap *encap)
{
    return 0;
}

int AddPermissionArrayToValue(cJSON *permissionItemArr, UserEncap *encap)
{
    return 0;
}

int AddPermissionItemToEncapsInfo(UserEncap *encap, cJSON *permissionItem)
{
    return 0;
}

cJSON *GetEncapsPermissions(cJSON *extInfoJson, int *count)
{
    return nullptr;
}

int AddMembersToEncapsInfo(cJSON *permissionsJson, UserEncaps *encapsInfo, int count)
{
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
    return 0;
}

int SpawnSetPermissions(AppSpawningCtx *property, UserEncaps *encapsInfo)
{
    return 0;
}

void SetAllowDumpable(AppSpawningCtx *property, UserEncaps *encapsInfo)
{
    if (encapsInfo == nullptr) {
        printf("SetAllowDumpable encapsInfo is null");
    }
}

#ifdef __cplusplus
}
#endif
