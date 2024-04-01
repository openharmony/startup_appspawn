/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "init_param.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::AppSpawn;

namespace {
    const std::string MODULE_TEST_BUNDLE_NAME("moduleTestProcessName");
    const std::string APP_JSON_CONFIG("/appdata-sandbox.json");
}

static bool AppSandboxPidNsIsSupport(void)
{
    char buffer[10] = {0};
    uint32_t buffSize = sizeof(buffer);

    if (SystemGetParameter("const.sandbox.pidns.support", buffer, &buffSize) != 0) {
        return true;
    }
    if (!strcmp(buffer, "false")) {
        return false;
    }
    return true;
}

void LoadAppSandboxConfig(AppSpawnContent *content)
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

    if (!content->isNweb && !AppSandboxPidNsIsSupport()) {
        return;
    }
    content->sandboxNsFlags = SandboxUtils::GetSandboxNsFlags(content->isNweb);
}

int32_t SetAppSandboxProperty(struct AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_CHECK(client != NULL, return -1, "Invalid appspwn client");
    AppSpawnClientExt *clientExt = reinterpret_cast<AppSpawnClientExt *>(client);
    // no sandbox
    if (clientExt->property.flags & APP_NO_SANDBOX) {
        return 0;
    }

    int ret = 0;
    if (client->cloneFlags & CLONE_NEWPID) {
        ret = getprocpid();
        if (ret < 0) {
            return ret;
        }
    }
    if (content->isNweb) {
        ret = SandboxUtils::SetAppSandboxPropertyNweb(client);
    } else {
        ret = SandboxUtils::SetAppSandboxProperty(client);
    }

    // free ExtraInfo
    if (clientExt->property.extraInfo.data != nullptr) {
        free(clientExt->property.extraInfo.data);
        clientExt->property.extraInfo = {};
    }

    // for module test do not create sandbox
    if (strncmp(clientExt->property.bundleName,
        MODULE_TEST_BUNDLE_NAME.c_str(), MODULE_TEST_BUNDLE_NAME.size()) == 0) {
        return 0;
    }
    return ret;
}
