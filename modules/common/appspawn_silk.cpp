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
#include "appspawn_silk.h"
#include "appspawn_server.h"
#include "cJSON.h"
#include <cstring>
#include <set>
#include <string>
#include "json_utils.h"
#include <dlfcn.h>

static const std::string SILK_JSON_BASE_PATH("etc/silk");
static const std::string SILK_JSON_CONFIG_NAME("/silk.json");
static const std::string SILK_JSON_ENABLE_ITEM("enabled_app_list");
static const std::string SILK_JSON_LIBRARY_PATH("/vendor/lib64/chipsetsdk/libsilk.so.0.1");

typedef struct TagParseJsonContext {
    std::set<std::string> appList;
} ParseJsonContext;

static ParseJsonContext *g_silkContext = NULL;

static int ParseSilkConfig(const cJSON *root, ParseJsonContext *context)
{
    APPSPAWN_CHECK(root != NULL && context != NULL, return APPSPAWN_SYSTEM_ERROR, "Invalid json");
    // no config
    cJSON *silkJson = cJSON_GetObjectItemCaseSensitive(root, SILK_JSON_ENABLE_ITEM.c_str());
    if (silkJson == nullptr) {
        return 0;
    }

    uint32_t moduleCount = cJSON_GetArraySize(silkJson);
    for (uint32_t i = 0; i < moduleCount; ++i) {
        const char *appName = cJSON_GetStringValue(cJSON_GetArrayItem(silkJson, i));
        if (appName == nullptr) {
            continue;
        }
        APPSPAWN_LOGV("Enable silk appName %{public}s", appName);
        if (!context->appList.count(appName)) {
            context->appList.insert(appName);
        }
    }
    return 0;
}

void LoadSilkConfig(void)
{
    g_silkContext = new ParseJsonContext();
    if (g_silkContext == NULL) {
        APPSPAWN_LOGE("Alloc space for silk context failed");
        return;
    }
    int ret = ParseJsonConfig(SILK_JSON_BASE_PATH.c_str(), SILK_JSON_CONFIG_NAME.c_str(),
                              ParseSilkConfig, g_silkContext);
    if (ret == APPSPAWN_SANDBOX_NONE) {
        return;
    }
    APPSPAWN_LOGI("Load silk Config ret %{public}d", ret);
    return;
}

void LoadSilkLibrary(const char *packageName)
{
    if (packageName == NULL) {
        return;
    }
    for (std::string appName : g_silkContext->appList) {
        if (appName.compare(packageName) == 0) {
            void *handle = dlopen(SILK_JSON_LIBRARY_PATH.c_str(), RTLD_NOW);
            APPSPAWN_LOGI("Enable Silk AppName %{public}s result:%{public}s",
                appName.c_str(), handle ? "success" : "failed");
        }
    }
    if (g_silkContext != NULL) {
        g_silkContext->appList.clear();
        delete g_silkContext;
        g_silkContext = NULL;
    }
}