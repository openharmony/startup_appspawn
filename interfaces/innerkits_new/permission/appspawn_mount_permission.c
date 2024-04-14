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
#include <stdlib.h>
#include <pthread.h>

#ifdef APPSPAWN_CLIENT
#include "appspawn_mount_permission.h"
#else
#include "appspawn_sandbox.h"
#endif
#include "appspawn_msg.h"
#include "appspawn_permission.h"
#include "appspawn_utils.h"
#include "json_utils.h"
#include "securec.h"

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static int32_t g_maxPermissionIndex = -1;
static SandboxQueue g_permissionQueue = {0};

#ifdef APPSPAWN_SANDBOX_NEW
static int ParseAppSandboxConfig(const cJSON *root, ParseJsonContext *context)
{
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
        if (name == NULL) {
            APPSPAWN_LOGE("No name in permission configs");
            continue;
        }
        int ret = AddSandboxPermissionNode(name, &g_permissionQueue);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }
    return 0;
}
#else

static int ParsePermissionConfig(const cJSON *permissionConfigs)
{
    cJSON *config = NULL;
    cJSON_ArrayForEach(config, permissionConfigs)
    {
        const char *name = config->string;
        APPSPAWN_LOGV("ParsePermissionConfig %{public}s", name);
        int ret = AddSandboxPermissionNode(name, &g_permissionQueue);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }
    return 0;
}

static int ParseAppSandboxConfig(const cJSON *appSandboxConfig, ParseJsonContext *context)
{
    cJSON *configs = cJSON_GetObjectItemCaseSensitive(appSandboxConfig, "permission");
    APPSPAWN_CHECK(configs != NULL && cJSON_IsArray(configs), return 0, "No permission in json");

    int ret = 0;
    uint32_t configSize = cJSON_GetArraySize(configs);
    for (uint32_t i = 0; i < configSize; i++) {
        cJSON *json = cJSON_GetArrayItem(configs, i);
        ret = ParsePermissionConfig(json);
        APPSPAWN_CHECK(ret == 0, return ret, "Parse permission config fail result: %{public}d ", ret);
    }
    return ret;
}
#endif

static int LoadPermissionConfig(void)
{
    int ret = ParseJsonConfig("etc/sandbox", APP_SANDBOX_FILE_NAME, ParseAppSandboxConfig, NULL);
    if (ret == APPSPAWN_SANDBOX_NONE) {
        APPSPAWN_LOGW("No sandbox config");
        ret = 0;
    }
    APPSPAWN_CHECK(ret == 0, return ret, "Load sandbox fail");
    g_maxPermissionIndex = PermissionRenumber(&g_permissionQueue);
    return 0;
}

int32_t GetPermissionIndex(const char *permission)
{
    APPSPAWN_CHECK_ONLY_EXPER(permission != NULL && g_maxPermissionIndex > 0, return INVALID_PERMISSION_INDEX);
    return GetPermissionIndexInQueue(&g_permissionQueue, permission);
}

int32_t GetMaxPermissionIndex(void)
{
    return g_maxPermissionIndex;
}

const char *GetPermissionByIndex(int32_t index)
{
    if (g_maxPermissionIndex <= index) {
        return NULL;
    }
    const SandboxPermissionNode *node = GetPermissionNodeInQueueByIndex(&g_permissionQueue, index);
#ifdef APPSPAWN_CLIENT
    return node == NULL ? NULL : node->name;
#else
    return node == NULL ? NULL : node->section.name;
#endif
}

static void LoadPermission(void)
{
    pthread_mutex_lock(&g_mutex);
    if (g_maxPermissionIndex == -1) {
        OH_ListInit(&g_permissionQueue.front);
        LoadPermissionConfig();
    }
    pthread_mutex_unlock(&g_mutex);
}

static void DeletePermissions(void)
{
    pthread_mutex_lock(&g_mutex);
    OH_ListRemoveAll(&g_permissionQueue.front, NULL);
    g_maxPermissionIndex = -1;
    pthread_mutex_unlock(&g_mutex);
}

__attribute__((constructor)) static void LoadPermissionModule(void)
{
    LoadPermission();
}

__attribute__((destructor)) static void DeletePermissionModule(void)
{
    DeletePermissions();
}