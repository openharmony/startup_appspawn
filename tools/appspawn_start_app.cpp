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
#include <cstring>

#include "appspawn_server.h"
#include "client_socket.h"
#include "hilog/log.h"
#include "securec.h"

using namespace OHOS;
using namespace OHOS::HiviewDFX;
static constexpr HiLogLabel LABEL = {LOG_CORE, 0, "AppSpawnServer"};
static const int DECIMAL = 10;

int main(int argc, char *const argv[])
{
    if (argc <= 11) { // 11 min argc
        HiLog::Error(LABEL, "Invalid argc %{public}d", argc);
        return -1;
    }
    HiLog::Debug(LABEL, "AppSpawnServer argc %{public}d app:%{public}s", argc, argv[4]); // 4 name index
    // calculate child process long name size
    uintptr_t start = reinterpret_cast<uintptr_t>(argv[0]);
    uintptr_t end = reinterpret_cast<uintptr_t>(strchr(argv[argc - 1], 0));
    if (end == 0) {
        return -1;
    }
    uintptr_t argvSize = end - start;

    auto appProperty = std::make_unique<OHOS::AppSpawn::ClientSocket::AppProperty>();
    if (appProperty == nullptr) {
        HiLog::Error(LABEL, "Failed to create app property %{public}s", argv[4]); // 4 name index
        return -1;
    }
    int index = 1;
    int fd = strtoul(argv[index++], nullptr, DECIMAL);
    appProperty->uid = strtoul(argv[index++], nullptr, DECIMAL);
    appProperty->gid = strtoul(argv[index++], nullptr, DECIMAL);
    (void)strcpy_s(appProperty->processName, sizeof(appProperty->processName), argv[index++]);
    (void)strcpy_s(appProperty->bundleName, sizeof(appProperty->bundleName), argv[index++]);
    (void)strcpy_s(appProperty->soPath, sizeof(appProperty->soPath), argv[index++]);
    appProperty->accessTokenId = strtoul(argv[index++], nullptr, DECIMAL);
    (void)strcpy_s(appProperty->apl, sizeof(appProperty->apl), argv[index++]);
    (void)strcpy_s(appProperty->renderCmd, sizeof(appProperty->renderCmd), argv[index++]);
    appProperty->flags = strtoul(argv[index++], nullptr, DECIMAL);
    appProperty->gidCount = strtoul(argv[index++], nullptr, DECIMAL);
    uint32_t i = 0;
    while ((i < appProperty->gidCount) && (i < sizeof(appProperty->gidTable) / sizeof(appProperty->gidTable[0]))) {
        if (index >= argc) {
            HiLog::Error(LABEL, "Invalid arg %{public}d %{public}d", index, argc);
            return -1;
        }
        appProperty->gidTable[i++] = strtoul(argv[index++], nullptr, DECIMAL);
    }
    auto appspawnServer = std::make_shared<OHOS::AppSpawn::AppSpawnServer>("AppSpawn");
    if (appspawnServer != nullptr) {
        int ret = appspawnServer->AppColdStart(argv[0], argvSize, appProperty.get(), fd);
        if (ret != 0) {
            HiLog::Error(LABEL, "Cold start %{public}s fail.", appProperty->bundleName);
        }
    }
    return 0;
}
