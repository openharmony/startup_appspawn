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
#include "config_policy_utils.h"
#include "json_utils.h"
#include "sandbox_utils.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::AppSpawn;

namespace {
    const std::string MODULE_TEST_BUNDLE_NAME("moduleTestProcessName");
    const std::string NAMESPACE_JSON_CONFIG("/system/etc/sandbox/sandbox-config.json");
    const std::string APP_JSON_CONFIG("/appdata-sandbox.json");
}

void LoadAppSandboxConfig(void)
{
    bool rc = true;
    // load sandbox config
    nlohmann::json appSandboxConfig;
    CfgFiles *files = GetCfgFiles("etc/sandbox");
    for (int i = 0; (files != nullptr) && (i < MAX_CFG_POLICY_DIRS_CNT); ++i) {
        if (files->paths[i] == nullptr) {
            continue;
        }
        std::string path = files->paths[i];
        path += APP_JSON_CONFIG;
        APPSPAWN_LOGI("LoadAppSandboxConfig %{public}s", path.c_str());
        rc = JsonUtils::GetJsonObjFromJson(appSandboxConfig, path);
        APPSPAWN_CHECK(rc, continue, "Failed to load app data sandbox config %{public}s", path.c_str());
        SandboxUtils::StoreJsonConfig(appSandboxConfig);
    }
    FreeCfgFiles(files);

    nlohmann::json appNamespaceConfig;
    rc = JsonUtils::GetJsonObjFromJson(appNamespaceConfig, NAMESPACE_JSON_CONFIG);
    APPSPAWN_CHECK_ONLY_LOG(rc, "AppSpawnServer::Failed to load app sandbox namespace config");
    SandboxUtils::StoreNamespaceJsonConfig(appNamespaceConfig);
}

int32_t SetAppSandboxProperty(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    APPSPAWN_CHECK(client != NULL, return -1, "Invalid appspwn client");
    AppSpawnClientExt *clientExt = reinterpret_cast<AppSpawnClientExt *>(client);
    // no sandbox
    if (clientExt->property.flags & APP_NO_SANDBOX) {
        return 0;
    }
    // no news
    if ((client->cloneFlags & CLONE_NEWNS) != CLONE_NEWNS) {
        return 0;
    }
    int ret = SandboxUtils::SetAppSandboxProperty(client);
    // free HspList
    if (clientExt->property.hspList.data != nullptr) {
        free(clientExt->property.hspList.data);
        clientExt->property.hspList = {};
    }
    // for module test do not create sandbox
    if (strncmp(clientExt->property.bundleName,
        MODULE_TEST_BUNDLE_NAME.c_str(), MODULE_TEST_BUNDLE_NAME.size()) == 0) {
        return 0;
    }
    return ret;
}

uint32_t GetAppNamespaceFlags(const char *bundleName)
{
    return SandboxUtils::GetNamespaceFlagsFromConfig(bundleName);
}

