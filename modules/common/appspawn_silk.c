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

#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#include "appspawn_server.h"
#include "appspawn_silk.h"
#include "appspawn_utils.h"
#include "config_policy_utils.h"
#include "cJSON.h"
#include "json_utils.h"
#include "securec.h"

#define SILK_JSON_CONFIG_PATH   "/vendor/etc/silk/silk.json"
#define SILK_JSON_ENABLE_ITEM   "enabled_app_list"
#define SILK_JSON_LIBRARY_PATH  "/vendor/lib64/chipsetsdk/libsilk.so.0.1"

#define SILK_JSON_MAX 128
#define SILK_JSON_NAME_MAX 256

struct SilkConfig {
    char **configItems;
    int configCursor;
};

static struct SilkConfig g_silkConfig = {0};

static void ParseSilkConfig(const cJSON *root, struct SilkConfig *config)
{
    cJSON *silkJson = cJSON_GetObjectItemCaseSensitive(root, SILK_JSON_ENABLE_ITEM);
    if (silkJson == NULL) {
        return;
    }

    uint32_t configCount = cJSON_GetArraySize(silkJson);
    APPSPAWN_CHECK(configCount <= SILK_JSON_MAX, configCount = SILK_JSON_MAX,
                   "config count %{public}u is larger than %{public}d", configCount, SILK_JSON_MAX);
    config->configItems = (char **)malloc(configCount * sizeof(char *));
    APPSPAWN_CHECK(config->configItems != NULL, return, "Alloc for silk config items failed");
    int ret = memset_s(config->configItems, configCount * sizeof(char *), 0, configCount * sizeof(char *));
    APPSPAWN_CHECK(ret == 0, free(config->configItems);
                   config->configItems = NULL; return,
                   "Memset silk config items failed");

    for (uint32_t i = 0; i < configCount; ++i) {
        const char *appName = cJSON_GetStringValue(cJSON_GetArrayItem(silkJson, i));
        APPSPAWN_CHECK(appName != NULL, break, "appName is NULL");
        APPSPAWN_LOGI("Enable silk appName %{public}s", appName);

        int len = strlen(appName) + 1;
        APPSPAWN_CHECK(len <= SILK_JSON_NAME_MAX, continue,
                       "appName %{public}s is larger than the maximum limit", appName);
        char **item = &config->configItems[config->configCursor];
        *item = (char *)malloc(len * sizeof(char));
        APPSPAWN_CHECK(*item != NULL, break, "Alloc for config item failed");

        ret = memset_s(*item, len * sizeof(char), 0, len * sizeof(char));
        APPSPAWN_CHECK(ret == 0, free(*item);
                       *item = NULL; break,
                       "Memset config item %{public}s failed", appName);

        ret = strncpy_s(*item, len, appName, len - 1);
        APPSPAWN_CHECK(ret == 0, free(*item);
                       *item = NULL; break,
                       "Copy config item %{public}s failed", appName);
        config->configCursor++;
    }
}

void LoadSilkConfig(void)
{
    cJSON *root = GetJsonObjFromFile(SILK_JSON_CONFIG_PATH);
    APPSPAWN_CHECK(root != NULL, return, "Failed to load silk config");
    ParseSilkConfig(root, &g_silkConfig);
    cJSON_Delete(root);
}

static void FreeSilkConfigItems(void)
{
    for (int i = 0; i < g_silkConfig.configCursor; i++) {
        if (g_silkConfig.configItems[i] != NULL) {
            free(g_silkConfig.configItems[i]);
            g_silkConfig.configItems[i] = NULL;
        }
    }
}

static void FreeSilkConfig(void)
{
    free(g_silkConfig.configItems);
    g_silkConfig.configItems = NULL;
    g_silkConfig.configCursor = 0;
}

static void FreeAllSilkConfig(void)
{
    FreeSilkConfigItems();
    FreeSilkConfig();
}

void LoadSilkLibrary(const char *packageName)
{
    APPSPAWN_CHECK(g_silkConfig.configItems != NULL, return,
                   "Load silk library failed for configItems is NULL");
    APPSPAWN_CHECK(packageName != NULL, FreeAllSilkConfig(); return,
                   "Load silk library failed for packageName is NULL");
    char **appName = NULL;
    void *handle = NULL;
    for (int i = 0; i < g_silkConfig.configCursor; i++) {
        appName = &g_silkConfig.configItems[i];
        if (*appName == NULL) {
            break;
        }
        if (handle == NULL && strcmp(*appName, packageName) == 0) {
            handle = dlopen(SILK_JSON_LIBRARY_PATH, RTLD_NOW);
            APPSPAWN_LOGI("Enable Silk AppName %{public}s result:%{public}s",
                *appName, handle ? "success" : "failed");
        }
        free(*appName);
        *appName = NULL;
    }
    FreeSilkConfig();
}