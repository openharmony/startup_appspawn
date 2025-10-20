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

bool IsValidDataGroupItem(cJSON *item);
int GetElxInfoFromDir(const char *path);
DataGroupSandboxPathTemplate *GetDataGroupArgTemplate(uint32_t category);
bool IsUnlockStatus(uint32_t uid);
bool SetSandboxPathShared(const char *sandboxPath);
int MountWithFileMgr(const AppDacInfo *info);
int MountWithOther(const AppDacInfo *info);
void MountStorageUsers(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const AppDacInfo *info);
int MountSharedMapItem(const char *bundleNamePath, const char *sandboxPathItem);
void MountSharedMap(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const char *bundleNamePath);
int DataGroupCtxNodeCompare(ListNode *node, void *data);
int AddDataGroupItemToQueue(AppSpawnMgr *content, const char *srcPath, const char *destPath);
cJSON *GetJsonObjFromExtInfo(const SandboxContext *context, const char *name);
void DumpDataGroupCtxQueue(const ListNode *front);
int ParseDataGroupList(AppSpawnMgr *content, SandboxContext *context, const AppSpawnSandboxCfg *appSandbox,
    const char *bundleNamePath);
int UpdateDataGroupDirs(AppSpawnMgr *content);
int CreateSharedStamp(AppSpawnMsgDacInfo *info, SandboxContext *context);
int MountDirsToShared(AppSpawnMgr *content, SandboxContext *context, AppSpawnSandboxCfg *sandbox);


#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
