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

#include <string>
#include "appspawn_service.h"
#include "json_utils.h"
#include "sandbox_utils.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::AppSpawn;

namespace {
    const std::string MODULE_TEST_BUNDLE_NAME("moduleTestProcessName");
    const std::string NAMESPACE_JSON_CONFIG("/system/etc/sandbox/sandbox-config.json");
#if defined (__aarch64__) || defined (__x86_64__)
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
    APPSPAWN_CHECK_ONLY_LOG(rc, "AppSpawnServer::Failed to load app private sandbox config");
    SandboxUtils::StoreJsonConfig(appSandboxConfig);

    rc = JsonUtils::GetJsonObjFromJson(appSandboxConfig, PRODUCT_JSON_CONFIG);
    APPSPAWN_CHECK_ONLY_LOG(rc, "AppSpawnServer::Failed to load app product sandbox config");
    SandboxUtils::StoreProductJsonConfig(appSandboxConfig);

    nlohmann::json appNamespaceConfig;
    rc = JsonUtils::GetJsonObjFromJson(appNamespaceConfig, NAMESPACE_JSON_CONFIG);
    APPSPAWN_CHECK_ONLY_LOG(rc, "AppSpawnServer::Failed to load app sandbox namespace config");
    SandboxUtils::StoreNamespaceJsonConfig(appNamespaceConfig);
}

int32_t SetAppSandboxProperty(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    APPSPAWN_CHECK(client != NULL, return -1, "Invalid appspwn client");
    AppSpawnClientExt *appProperty = reinterpret_cast<AppSpawnClientExt *>(client);
    appProperty->property.cloneFlags = client->cloneFlags;
    int ret = SandboxUtils::SetAppSandboxProperty(&appProperty->property);
    // for module test do not create sandbox
    if (strncmp(appProperty->property.bundleName,
        MODULE_TEST_BUNDLE_NAME.c_str(), MODULE_TEST_BUNDLE_NAME.size()) == 0) {
        return 0;
    }
    return ret;
}

uint32_t GetAppNamespaceFlags(const char *bundleName)
{
    return SandboxUtils::GetNamespaceFlagsFromConfig(bundleName);
}

