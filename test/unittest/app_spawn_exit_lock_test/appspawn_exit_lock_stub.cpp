/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "securec.h"
#include "appspawn_exit_lock_stub.h"
#include "appspawn.h"

#ifdef __cplusplus
extern "C" {
#endif

int AddServerStageHook(AppSpawnHookStage stage, int prio, ServerStageHook hook)
{
    return 0;
}

int GetParameter(const char *key, const char *def, char *value, uint32_t len)
{
    if (strcmp(key, "startup.appspawn.lockstatus_100") == 0) {
        return strcpy_s(value, len, "0") == 0 ? strlen("0") : -1;
    }
    if (strcmp(key, "startup.appspawn.lockstatus_101") == 0) {
        return strcpy_s(value, len, "1") == 0 ? strlen("1") : -1;
    }
    return -1;
}

void RegChildLooper(AppSpawnContent *content, ChildLoop loop)
{
    if (content != nullptr) {
        printf("RegChildLooper is not nullptr");
    }
}

void AppSpawnEnvClear(AppSpawnContent *content, AppSpawnClient *client)
{
    if (content != nullptr) {
        printf("AppSpawnEnvClear is not nullptr");
    }
}

ssize_t WriteStub(int fd, const void *buf, size_t count)
{
    return 0;
}

int CloseStub(int fd)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
