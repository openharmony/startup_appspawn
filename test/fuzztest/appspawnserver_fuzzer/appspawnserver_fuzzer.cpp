/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "appspawnserver_fuzzer.h"
#include <string>
#include "appspawn_server.h"

namespace OHOS {

    // Fuzz测试: AppSpawnExecuteClearEnvHook
    int FuzzerAppSpawnExecuteClearEnvHook(const uint8_t *data, size_t size)
    {
        if (size < sizeof(AppSpawnContent*) + sizeof(AppSpawnClient*)) {
            return 0;
        }

        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(const_cast<uint8_t*>(data));
        AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(const_cast<uint8_t*>(data +
            sizeof(AppSpawnContent*)));

        // 调用待测试函数
        AppSpawnExecuteClearEnvHook(content, client);
        return 0;
    }

    // Fuzz测试: AppSpawnExecuteSpawningHook
    int FuzzerAppSpawnExecuteSpawningHook(const uint8_t *data, size_t size)
    {
        if (size < sizeof(AppSpawnContent*) + sizeof(AppSpawnClient*)) {
            return 0;
        }
        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(const_cast<uint8_t*>(data));
        AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(const_cast<uint8_t*>(data +
            sizeof(AppSpawnContent*)));

        // 调用待测试函数
        AppSpawnExecuteSpawningHook(content, client);
        return 0;
    }

    // Fuzz测试: AppSpawnExecutePreReplyHook
    int FuzzerAppSpawnExecutePreReplyHook(const uint8_t *data, size_t size)
    {
        if (size < sizeof(AppSpawnContent*) + sizeof(AppSpawnClient*)) {
            return 0;
        }
        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(const_cast<uint8_t*>(data));
        AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(const_cast<uint8_t*>(data +
            sizeof(AppSpawnContent*)));

        // 调用待测试函数
        AppSpawnExecutePreReplyHook(content, client);
        return 0;
    }

    // Fuzz测试: AppSpawnExecutePostReplyHook
    int FuzzerAppSpawnExecutePostReplyHook(const uint8_t *data, size_t size)
    {
        if (size < sizeof(AppSpawnContent*) + sizeof(AppSpawnClient*)) {
            return 0;
        }
        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(const_cast<uint8_t*>(data));
        AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(const_cast<uint8_t*>(data +
            sizeof(AppSpawnContent*)));

        // 调用待测试函数
        AppSpawnExecutePostReplyHook(content, client);
        return 0;
    }

    // Fuzz测试: AppSpawnEnvClear
    int FuzzerAppSpawnEnvClear(const uint8_t *data, size_t size)
    {
        if (size < sizeof(AppSpawnContent*) + sizeof(AppSpawnClient*)) {
            return 0;
        }
        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(const_cast<uint8_t*>(data));
        AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(const_cast<uint8_t*>(data +
            sizeof(AppSpawnContent*)));

        // 调用待测试函数
        AppSpawnEnvClear(content, client);
        return 0;
    }

    // Fuzz测试: AppSpawnProcessMsg
    int FuzzerAppSpawnProcessMsg(const uint8_t *data, size_t size)
    {
        if (size < sizeof(AppSpawnContent*) + sizeof(AppSpawnClient*) + sizeof(pid_t)) {
            return 0;
        }

        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(const_cast<uint8_t*>(data));
        AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(const_cast<uint8_t*>(data +
            sizeof(AppSpawnContent*)));
        pid_t *childPid = reinterpret_cast<pid_t *>(const_cast<uint8_t*>(data +
            sizeof(AppSpawnContent*) + sizeof(AppSpawnClient*)));

        // 调用待测试函数
        AppSpawnProcessMsg(content, client, childPid);
        return 0;
    }

    // Fuzz测试: ProcessExit
    int FuzzerProcessExit(const uint8_t *data, size_t size)
    {
        if (size < sizeof(int)) {
            return 0;
        }
        int code = *reinterpret_cast<const int *>(data);

        // 调用待测试函数
        ProcessExit(code);
        return 0;
    }

    // Fuzz测试: AppSpawnChild
    int FuzzerAppSpawnChild(const uint8_t *data, size_t size)
    {
        if (size < sizeof(AppSpawnContent*) + sizeof(AppSpawnClient*)) {
            return 0;
        }
        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(const_cast<uint8_t*>(data));
        AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(const_cast<uint8_t*>(data +
            sizeof(AppSpawnContent*)));

        // 调用待测试函数
        AppSpawnChild(content, client);
        return 0;
    }
}
/* Fuzzer entry point */
int FuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzerAppSpawnExecuteClearEnvHook(data, size);
    OHOS::FuzzerAppSpawnExecuteSpawningHook(data, size);
    OHOS::FuzzerAppSpawnExecutePreReplyHook(data, size);
    OHOS::FuzzerAppSpawnExecutePostReplyHook(data, size);
    OHOS::FuzzerAppSpawnEnvClear(data, size);
    OHOS::FuzzerAppSpawnProcessMsg(data, size);
    OHOS::FuzzerProcessExit(data, size);
    OHOS::FuzzerAppSpawnChild(data, size);
    return 0;
}