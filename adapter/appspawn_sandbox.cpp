/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "appspawn_adapter.h"

#include <unistd.h>

#include <string>
#include <vector>

#include "hilog/log.h"
#include "json_utils.h"
#include "sandbox_utils.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::HiviewDFX;
using namespace OHOS::AppSpawn;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0, "AppSpawn_SandboxUtil"};

namespace {
#ifdef __aarch64__
    const std::string APP_JSON_CONFIG("/system/etc/sandbox/appdata-sandbox64.json");
#else
    const std::string APP_JSON_CONFIG("/system/etc/sandbox/appdata-sandbox.json");
#endif
    const std::string PRODUCT_JSON_CONFIG("/system/etc/sandbox/product-sandbox.json");
}

void LoadAppSandboxConfig(void)
{
    // load sandbox config
    nlohmann::json appSandboxConfig;

    bool rc = JsonUtils::GetJsonObjFromJson(appSandboxConfig, APP_JSON_CONFIG);
    if (!rc) {
        HiLog::Error(LABEL, "AppSpawnServer::Failed to load app private sandbox config");
    }
    SandboxUtils::StoreJsonConfig(appSandboxConfig);

    rc = JsonUtils::GetJsonObjFromJson(appSandboxConfig, PRODUCT_JSON_CONFIG);
    if (!rc) {
        HiLog::Error(LABEL, "AppSpawnServer::Failed to load app product sandbox config");
    }
    SandboxUtils::StoreProductJsonConfig(appSandboxConfig);
}

int32_t SetAppSandboxProperty(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    APPSPAWN_CHECK(client != NULL, return -1, "Invalid appspwn client");

    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    return SandboxUtils::SetAppSandboxProperty(&appProperty->property);
}
