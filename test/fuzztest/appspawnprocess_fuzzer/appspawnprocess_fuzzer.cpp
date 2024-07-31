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

#include "appspawnprocess_fuzzer.h"
#include <string>
#include "appspawn_manager.h"
/**
 * @brief 孵化成功后进程或者app实例的操作
 *
 */
namespace OHOS {

    void AppTraversal(const AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data)
    {
        return;
    }

    void TraversalSpawnedProcess(AppTraversal traversal, void *data)
    {
        AppSpawnMgr *mgr = GetAppSpawnMgr();
        if (mgr == NULL || traversal == NULL) {
            return;
        }
        ListNode *node = g_appSpawnMgr->appQueue.next;
        while (node != &g_appSpawnMgr->appQueue) {
            ListNode *next = node->next;
            AppSpawnedProcess *appInfo = ListEntry(node, AppSpawnedProcess, node);
            traversal(g_appSpawnMgr, appInfo, data);
            node = next;
        }
    }

    int FuzzTraversalSpawnedProcess(const uint8_t *data, size_t size)
    {
        TraversalSpawnedProcess(AppTraversal, (void *)data);
        return 0;
    }

    int FuzzAddSpawnedProcess(const uint8_t *data, size_t size)
    {
        pid_t pid = static_cast<pid_t>(size);
        std::string processName(reinterpret_cast<const char*>(data), size);
        AddSpawnedProcess(pid, processName.c_str());
        return 0;
    }

    int FuzzGetSpawnedProcess(const uint8_t *data, size_t size)
    {
        pid_t pid = static_cast<pid_t>(size);
        AppSpawnedProcess *process = GetSpawnedProcess(pid);
        if (!process) {
            return -1;
        }
        return 0;
    }

    int FuzzGetSpawnedProcessByName(const uint8_t *data, size_t size)
    {
        std::string processName(reinterpret_cast<const char*>(data), size);
        AppSpawnedProcess *process = GetSpawnedProcessByName(processName.c_str());
        if (!process) {
            return -1;
        }
        return 0;
    }

    int FuzzTerminateSpawnedProcess(const uint8_t *data, size_t size)
    {
        pid_t pid = static_cast<pid_t>(size);
        AppSpawnedProcess *process = GetSpawnedProcess(pid);
        if (!process) {
            return -1;
        }
        TerminateSpawnedProcess(process);
        return 0;
    }

}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTraversalSpawnedProcess(data, size);
    OHOS::FuzzAddSpawnedProcess(data, size);
    OHOS::FuzzGetSpawnedProcess(data, size);
    OHOS::FuzzGetSpawnedProcessByName(data, size);
    OHOS::FuzzTerminateSpawnedProcess(data, size);
    return 0;
}
