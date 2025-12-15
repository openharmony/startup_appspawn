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

#ifndef APPSPAWN_TEST_STUB_H
#define APPSPAWN_TEST_STUB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "cJSON.h"
#include "appspawn_client.h"
#include "appspawn_hook.h"
#include "appspawn_encaps.h"

#ifdef __cplusplus
extern "C" {
#endif

int OpenEncapsFile(void);
int WriteEncapsInfo(int fd, AppSpawnEncapsBaseType encapsType, const void *encapsInfo, uint32_t flag);
int EnableEncapsForProc(int encapsFileFd);
uint32_t SpawnGetMaxPids(AppSpawningCtx *property);
cJSON *GetJsonObjFromExtInfo(const AppSpawningCtx *property, const char *name);
int AddPermissionStrToValue(const char *valueStr, UserEncap *encap);
int AddPermissionIntArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize);
int AddPermissionBoolArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize);
int AddPermissionStrArrayToValue(cJSON *arrayItem, UserEncap *encap);
int AddPermissionArrayToValue(cJSON *permissionItemArr, UserEncap *encap);
int AddPermissionItemToEncapsInfo(UserEncap *encap, cJSON *permissionItem);
cJSON *GetEncapsPermissions(cJSON *extInfoJson, int *count);
int AddMembersToEncapsInfo(cJSON *permissionsJson, UserEncaps *encapsInfo, int count);
void FreeEncapsInfo(UserEncaps *encapsInfo);
int SpawnSetMaxPids(AppSpawningCtx *property, UserEncaps *encapsInfo);
int SpawnSetPermissions(AppSpawningCtx *property, UserEncaps *encapsInfo);
void SetAllowDumpable(AppSpawningCtx *property, UserEncaps *encapsInfo);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
