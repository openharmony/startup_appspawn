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

#include "appspawnctx_fuzzer.h"
#include <string>
#include "appspawn_manager.h"
/**
 * @brief 孵化过程中的ctx对象的操作
 *
 */
namespace OHOS {

    void ProcessTraversal(const AppSpawnMgr *mgr, AppSpawningCtx *ctx, void *data)
    {
        return;
    }

    void AppSpawningCtxTraversal(ProcessTraversal traversal, void *data)
    {
        AppSpawnMgr *mgr = GetAppSpawnMgr();
        if (mgr == NULL || traversal == NULL) {
            return
        }
        ListNode *node = g_appSpawnMgr->appQueue.next;
        while (node != &g_appSpawnMgr->appQueue) {
            ListNode *next = node->next;
            AppSpawnedProcess *appInfo = ListEntry(node, AppSpawnedProcess, node);
            traversal(g_appSpawnMgr, appInfo, data);
            node = next;
        }
    }

    int FuzzAppSpawningCtxTraversal(const uint8_t *data, size_t size)
    {
        pid_t pid = static_cast<pid_t>(size);
        AppSpawningCtxTraversal(ProcessTraversal, (void *)data);
        return 0;
    }

    int FuzzGetAppSpawningCtxByPid(const uint8_t *data, size_t size)
    {
        pid_t pid = static_cast<pid_t>(size);
        AppSpawningCtx *ctx = GetAppSpawningCtxByPid(pid);
        if (ctx) {
            return 0;
        }
        return -1;
    }

    int FuzzCreateAppSpawningCtx(const uint8_t *data, size_t size)
    {
        AppSpawningCtx *ctx = CreateAppSpawningCtx();
        if (ctx) {
            return 0;
        }
        return -1;
    }

    int FuzzDeleteAppSpawningCtx(const uint8_t *data, size_t size)
    {
        AppSpawningCtx *ctx = CreateAppSpawningCtx();
        if (ctx) {
            DeleteAppSpawningCtx(ctx);
            return 0;
        }
        return -1;
    }

    int FuzzKillAndWaitStatus(const uint8_t *data, size_t size)
    {
        pid_t pid = static_cast<pid_t>(size);
        int status = 0;
        int ret = KillAndWaitStatus(pid, pid, &status);
        if (ret) {
            return -1;
        }
        return 0;
    }

}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzAppSpawningCtxTraversal(data, size);
    OHOS::FuzzGetAppSpawningCtxByPid(data, size);
    OHOS::FuzzCreateAppSpawningCtx(data, size);
    OHOS::FuzzDeleteAppSpawningCtx(data, size);
    OHOS::FuzzKillAndWaitStatus(data, size);
    return 0;
}
