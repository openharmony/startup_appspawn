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
    if (path == nullptr) {
        printf("GetElxInfoFromDir path is null");
    }

    return 0;
}

DataGroupSandboxPathTemplate *GetDataGroupArgTemplate(uint32_t category)
{
    if (category == 0) {
        printf("GetDataGroupArgTemplate category is 0");
    }

    return nullptr;
}

bool IsUnlockStatus(uint32_t uid)
{
    if (uid == 0) {
        printf("IsUnlockStatus uid is 0");
    }

    return true;
}

bool SetSandboxPathShared(const char *sandboxPath)
{
    if (sandboxPath == nullptr) {
        printf("SetSandboxPathShared sandboxPath is null");
    }

    return true;
}

int MountWithFileMgr(const AppDacInfo *info)
{
    if (info == nullptr) {
        printf("MountWithFileMgr info is null");
    }

    return 0;
}

int MountWithOther(const AppDacInfo *info)
{
    if (info == nullptr) {
        printf("MountWithOther info is null");
    }

    return 0;
}

void MountStorageUsers(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const AppDacInfo *info)
{
    if (context == nullptr) {
        printf("MountStorageUsers context is nullptr");
    }

    if (sandbox == nullptr) {
        printf("MountStorageUsers sandbox is nullptr");
    }

    if (info == nullptr) {
        printf("MountStorageUsers info is nullptr");
    }
}

int MountSharedMapItem(const char *bundleNamePath, const char *sandboxPathItem)
{
    if (bundleNamePath == nullptr) {
        printf("MountSharedMapItem bundleNamePath is null");
    }

    if (sandboxPathItem == nullptr) {
        printf("MountSharedMapItem sandboxPathItem is null");
    }

    return 0;
}

void MountSharedMap(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const char *bundleNamePath)
{
    if (context == nullptr) {
        printf("MountSharedMap context is nullptr");
    }

    if (sandbox == nullptr) {
        printf("MountSharedMap sandbox is nullptr");
    }

    if (bundleNamePath == nullptr) {
        printf("MountSharedMap bundleNamePath is nullptr");
    }
}

int DataGroupCtxNodeCompare(ListNode *node, void *data)
{
    if (node == nullptr) {
        printf("DataGroupCtxNodeCompare node is null");
    }

    if (data == nullptr) {
        printf("DataGroupCtxNodeCompare data is null");
    }

    return 0;
}

int AddDataGroupItemToQueue(AppSpawnMgr *content, const char *srcPath, const char *destPath)
{
    if (content == nullptr) {
        printf("AddDataGroupItemToQueue content is null");
    }

    if (srcPath == nullptr) {
        printf("AddDataGroupItemToQueue srcPath is null");
    }

    if (destPath == nullptr) {
        printf("AddDataGroupItemToQueue destPath is null");
    }

    return 0;
}

cJSON *GetJsonObjFromExtInfo(const SandboxContext *context, const char *name)
{
    if (context == nullptr) {
        printf("GetJsonObjFromExtInfo context is null");
    }

    if (name == nullptr) {
        printf("GetJsonObjFromExtInfo name is null");
    }

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
    if (content == nullptr) {
        printf("ParseDataGroupList content is null");
    }

    if (context == nullptr) {
        printf("ParseDataGroupList context is null");
    }

    if (appSandbox == nullptr) {
        printf("ParseDataGroupList appSandbox is null");
    }

    if (bundleNamePath == nullptr) {
        printf("ParseDataGroupList bundleNamePath is null");
    }

    return 0;
}

int UpdateDataGroupDirs(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("UpdateDataGroupDirs content is null");
    }

    return 0;
}

int CreateSharedStamp(AppSpawnMsgDacInfo *info, SandboxContext *context)
{
    if (info == nullptr) {
        printf("CreateSharedStamp info is null");
    }

    if (context == nullptr) {
        printf("CreateSharedStamp context is null");
    }

    return 0;
}

int MountDirsToShared(AppSpawnMgr *content, SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    if (content == nullptr) {
        printf("MountDirsToShared content is null");
    }

    if (context == nullptr) {
        printf("MountDirsToShared context is null");
    }

    if (sandbox == nullptr) {
        printf("MountDirsToShared sandbox is null");
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
