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
#include <pthread.h>
#include <stdlib.h>

#ifndef APPSPAWN_CLIENT
#include "appspawn_sandbox.h"
#endif
#include "appspawn_client.h"
#include "appspawn_mount_permission.h"
#include "appspawn_msg.h"
#include "appspawn_permission.h"
#include "appspawn_utils.h"
#include "json_utils.h"
#include "securec.h"

typedef struct TagParseJsonContext {
    SandboxQueue permissionQueue;
    int32_t maxPermissionIndex;
    uint32_t inited;
    AppSpawnClientType type;
} ParseJsonContext, PermissionManager;

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static PermissionManager g_permissionMgr[CLIENT_MAX] = {};

#ifdef APPSPAWN_SANDBOX_NEW
static int ParseAppSandboxConfig(const cJSON *root, PermissionManager *mgr)
{
    APPSPAWN_LOGE("ParseAppSandboxConfig");
    // conditional
    cJSON *json = cJSON_GetObjectItemCaseSensitive(root, "conditional");
    if (json == NULL) {
        return 0;
    }
    // permission
    cJSON *config = cJSON_GetObjectItemCaseSensitive(json, "permission");
    if (config == NULL || !cJSON_IsArray(config)) {
        return 0;
    }

    uint32_t configSize = cJSON_GetArraySize(config);
    for (uint32_t i = 0; i < configSize; i++) {
        cJSON *json = cJSON_GetArrayItem(config, i);
        if (json == NULL) {
            continue;
        }
        char *name = GetStringFromJsonObj(json, "name");
        APPSPAWN_LOGV("ParsePermissionConfig %{public}s", name);
        if (name == NULL) {
            APPSPAWN_LOGE("No name in permission configs");
            continue;
        }
        int ret = AddSandboxPermissionNode(name, &mgr->permissionQueue);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }
    return 0;
}

static PermissionManager *GetPermissionMgrByType(AppSpawnClientType type)
{
    APPSPAWN_CHECK_ONLY_EXPER(type < CLIENT_MAX, return NULL);
    g_permissionMgr[type].type = type;
    return &g_permissionMgr[type];
}
#else

static int ParsePermissionConfig(const cJSON *permissionConfigs, PermissionManager *mgr)
{
    cJSON *config = NULL;
    cJSON_ArrayForEach(config, permissionConfigs)
    {
        const char *name = config->string;
        APPSPAWN_LOGV("ParsePermissionConfig %{public}s", name);
        int ret = AddSandboxPermissionNode(name, &mgr->permissionQueue);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }
    return 0;
}

static int ParseAppSandboxConfig(const cJSON *appSandboxConfig, PermissionManager *mgr)
{
    cJSON *configs = cJSON_GetObjectItemCaseSensitive(appSandboxConfig, "permission");
    APPSPAWN_CHECK(configs != NULL && cJSON_IsArray(configs), return 0, "No permission in json");

    int ret = 0;
    uint32_t configSize = cJSON_GetArraySize(configs);
    for (uint32_t i = 0; i < configSize; i++) {
        cJSON *json = cJSON_GetArrayItem(configs, i);
        ret = ParsePermissionConfig(json, mgr);
        APPSPAWN_CHECK(ret == 0, return ret, "Parse permission config fail result: %{public}d ", ret);
    }
    return ret;
}

static PermissionManager *GetPermissionMgrByType(AppSpawnClientType type)
{
    APPSPAWN_CHECK_ONLY_EXPER(type < CLIENT_MAX, return NULL);
    g_permissionMgr[0].type = CLIENT_FOR_APPSPAWN;
    return &g_permissionMgr[0];
}
#endif

static int LoadPermissionConfig(PermissionManager *mgr)
{
    APPSPAWN_CHECK(mgr != NULL, return -1, "Invalid permission mgr");
    int ret = ParseJsonConfig("etc/sandbox",
        mgr->type == CLIENT_FOR_APPSPAWN ? APP_SANDBOX_FILE_NAME : WEB_SANDBOX_FILE_NAME, ParseAppSandboxConfig, mgr);
    if (ret == APPSPAWN_SANDBOX_NONE) {
        APPSPAWN_LOGW("No sandbox config");
        ret = 0;
    }
    APPSPAWN_CHECK(ret == 0, return ret, "Load sandbox fail");
    mgr->maxPermissionIndex = PermissionRenumber(&mgr->permissionQueue);
    return 0;
}

static int32_t PMGetPermissionIndex(AppSpawnClientType type, const char *permission)
{
    PermissionManager *mgr = GetPermissionMgrByType(type);
    APPSPAWN_CHECK_ONLY_EXPER(mgr != NULL, return INVALID_PERMISSION_INDEX);
    APPSPAWN_CHECK_ONLY_EXPER(mgr->inited && mgr->maxPermissionIndex >= 0, return INVALID_PERMISSION_INDEX);
    APPSPAWN_CHECK_ONLY_EXPER(permission != NULL, return INVALID_PERMISSION_INDEX);
    return GetPermissionIndexInQueue((SandboxQueue *)&mgr->permissionQueue, permission);
}

static int32_t PMGetMaxPermissionIndex(AppSpawnClientType type)
{
    PermissionManager *mgr = GetPermissionMgrByType(type);
    APPSPAWN_CHECK_ONLY_EXPER(mgr != NULL, return INVALID_PERMISSION_INDEX);
    APPSPAWN_CHECK_ONLY_EXPER(mgr->inited && mgr->maxPermissionIndex >= 0, return INVALID_PERMISSION_INDEX);
    return mgr->maxPermissionIndex;
}

static const char *PMGetPermissionByIndex(AppSpawnClientType type, int32_t index)
{
    PermissionManager *mgr = GetPermissionMgrByType(type);
    APPSPAWN_CHECK_ONLY_EXPER(mgr != NULL, return NULL);
    APPSPAWN_CHECK_ONLY_EXPER(mgr->inited && mgr->maxPermissionIndex >= 0, return NULL);
    if (mgr->maxPermissionIndex <= index) {
        return NULL;
    }
    const SandboxPermissionNode *node = GetPermissionNodeInQueueByIndex((SandboxQueue *)&mgr->permissionQueue, index);
#ifdef APPSPAWN_CLIENT
    return node == NULL ? NULL : node->name;
#else
    return node == NULL ? NULL : node->section.name;
#endif
}

int LoadPermission(AppSpawnClientType type)
{
    APPSPAWN_LOGW("LoadPermission %{public}d", type);
    pthread_mutex_lock(&g_mutex);
    PermissionManager *mgr = GetPermissionMgrByType(type);
    if (mgr == NULL) {
        pthread_mutex_unlock(&g_mutex);
        return APPSPAWN_ARG_INVALID;
    }

    if (mgr->inited) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    mgr->maxPermissionIndex = -1;
    mgr->permissionQueue.type = 0;
    OH_ListInit(&mgr->permissionQueue.front);
    int ret = LoadPermissionConfig(mgr);
    if (ret != 0) {
        pthread_mutex_unlock(&g_mutex);
        return ret;
    }
    mgr->inited = 1;
    pthread_mutex_unlock(&g_mutex);
    return 0;
}

void DeletePermission(AppSpawnClientType type)
{
    APPSPAWN_LOGW("DeletePermission %{public}d", type);
    pthread_mutex_lock(&g_mutex);
    PermissionManager *mgr = GetPermissionMgrByType(type);
    if (mgr == NULL) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    if (!mgr->inited) {
        pthread_mutex_unlock(&g_mutex);
        return;
    }
    DeleteSandboxPermissions(&mgr->permissionQueue);
    mgr->maxPermissionIndex = -1;
    mgr->inited = 0;
    pthread_mutex_unlock(&g_mutex);
}

int32_t GetPermissionMaxCount()
{
    int32_t maxCount = 0;
    for (uint32_t i = 0; i < CLIENT_MAX; i++) {
        int32_t max = PMGetMaxPermissionIndex(i);
        if (max > maxCount) {
            maxCount = max;
        }
    }
    return maxCount;
}

int32_t GetPermissionIndex(AppSpawnClientHandle handle, const char *permission)
{
    APPSPAWN_CHECK(permission != NULL, return INVALID_PERMISSION_INDEX, "Invalid permission name");
    if (handle == NULL) {
        return PMGetPermissionIndex(CLIENT_FOR_APPSPAWN, permission);
    }
    AppSpawnReqMsgMgr *reqMgr = (AppSpawnReqMsgMgr *)handle;
    return PMGetPermissionIndex(reqMgr->type, permission);
}

int32_t GetMaxPermissionIndex(AppSpawnClientHandle handle)
{
    if (handle == NULL) {
        return PMGetMaxPermissionIndex(CLIENT_FOR_APPSPAWN);
    }
    AppSpawnReqMsgMgr *reqMgr = (AppSpawnReqMsgMgr *)handle;
    return PMGetMaxPermissionIndex(reqMgr->type);
}

const char *GetPermissionByIndex(AppSpawnClientHandle handle, int32_t index)
{
    if (handle == NULL) {
        return PMGetPermissionByIndex(CLIENT_FOR_APPSPAWN, index);
    }
    AppSpawnReqMsgMgr *reqMgr = (AppSpawnReqMsgMgr *)handle;
    return PMGetPermissionByIndex(reqMgr->type, index);
}

__attribute__((constructor)) static void LoadPermissionModule(void)
{
    (void)LoadPermission(CLIENT_FOR_APPSPAWN);
    (void)LoadPermission(CLIENT_FOR_NWEBSPAWN);
}

__attribute__((destructor)) static void DeletePermissionModule(void)
{
    DeletePermission(CLIENT_FOR_APPSPAWN);
    DeletePermission(CLIENT_FOR_NWEBSPAWN);
}