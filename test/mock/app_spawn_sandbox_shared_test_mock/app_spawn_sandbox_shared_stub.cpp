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

bool IsValidDataGroupItem(cJSON *item)
{
    return true;
}

int GetElxInfoFromDir(const char *path)
{
    return 0;
}

DataGroupSandboxPathTemplate *GetDataGroupArgTemplate(uint32_t category)
{
    return nullptr;
}

bool IsUnlockStatus(uint32_t uid)
{
    return true;
}

bool SetSandboxPathShared(const char *sandboxPath)
{
    return true;
}

int MountWithFileMgr(const AppDacInfo *info)
{
    return 0;
}

int MountWithOther(const AppDacInfo *info)
{
    return 0;
}

void MountStorageUsers(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const AppDacInfo *info)
{
    if (context == nullptr) {
        printf("MountStorageUsers context is nullptr");
    }
}

int MountSharedMapItem(const char *bundleNamePath, const char *sandboxPathItem)
{
    return 0;
}

void MountSharedMap(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const char *bundleNamePath)
{
    if (context == nullptr) {
        printf("MountSharedMap context is nullptr");
    }
}

int DataGroupCtxNodeCompare(ListNode *node, void *data)
{
    return 0;
}

int AddDataGroupItemToQueue(AppSpawnMgr *content, const char *srcPath, const char *destPath)
{
    return 0;
}

cJSON *GetJsonObjFromExtInfo(const SandboxContext *context, const char *name)
{
    return nullptr;
}

void DumpDataGroupCtxQueue(const ListNode *front)
{
    if (front == nullptr) {
        printf("DumpDataGroupCtxQueue front is nullptr");
    }
}

int ParseDataGroupList(AppSpawnMgr *content, SandboxContext *context, const AppSpawnSandboxCfg *appSandbox,
    const char *bundleNamePath)
{
    return 0;
}

int UpdateDataGroupDirs(AppSpawnMgr *content)
{
    return 0;
}

int CreateSharedStamp(AppSpawnMsgDacInfo *info, SandboxContext *context)
{
    return 0;
}

int MountDirsToShared(AppSpawnMgr *content, SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
