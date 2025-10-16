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

#include "app_spawn_stub.h"

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdbool>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>

#include <linux/capability.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "access_token.h"
#include "hilog/log.h"
#include "securec.h"
#include "token_setproc.h"
#include "tokenid_kit.h"

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif
#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#include <sys/prctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

static AppSpawnMgr *g_appSpawnMgr = nullptr;

AppSpawnMgr *CreateAppSpawnMgr(int mode)
{
    return nullptr;
}

AppSpawnMgr *GetAppSpawnMgr(void)
{
    return g_appSpawnMgr;
}

AppSpawnContent *GetAppSpawnContent(void)
{
    return g_appSpawnMgr == nullptr ? nullptr : &g_appSpawnMgr->content;
}

void SpawningQueueDestroy(ListNode *node)
{
    if (node == nullptr) {
        printf("SpawningQueueDestroy node is null");
    }
}

void ExtDataDestroy(ListNode *node)
{
    if (node == nullptr) {
        printf("ExtDataDestroy node is null");
    }
}

void DeleteAppSpawnMgr(AppSpawnMgr *mgr)
{
    if (mgr == nullptr) {
        printf("DeleteAppSpawnMgr mgr is null");
    }
}

void TraversalSpawnedProcess(AppTraversal traversal, void *data)
{
    if (data == nullptr) {
        printf("TraversalSpawnedProcess data is null");
    }
}

int AppInfoPidComparePro(ListNode *node, void *data)
{
    return 0;
}

int AppInfoNameComparePro(ListNode *node, void *data)
{
    return 0;
}

int AppInfoCompareProc(ListNode *node, ListNode *newNode)
{
    return 0;
}

AppSpawnedProcess *AddSpawnedProcess(pid_t pid, const char *processName, uint32_t appIndex, bool isDebuggable)
{
    return nullptr;
}

void TerminateSpawnedProcess(AppSpawnedProcess *node)
{
    if (node == nullptr) {
        printf("ExtDataDestroy node is null");
    }
}

AppSpawnedProcess *GetSpawnedProcess(pid_t pid)
{
    return nullptr;
}

AppSpawnedProcess *GetSpawnedProcessByName(const char *name)
{
    return nullptr;
}

void DumpProcessSpawnStack(pid_t pid)
{
    if (pid == nullptr) {
        printf("TDumpProcessSpawnStack pid is null");
    }
}

int KillAndWaitStatus(pid_t pid, int sig, int *exitStatus)
{
    return 0;
}

int GetProcessTerminationStatus(pid_t pid)
{
    return 0;
}

AppSpawningCtx *CreateAppSpawningCtx(void)
{
    return nullptr;
}

void DeleteAppSpawningCtx(AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("DeleteAppSpawningCtx pid is null");
    }
}

int AppPropertyComparePid(ListNode *node, void *data)
{
    return 0;
}

AppSpawningCtx *GetAppSpawningCtxByPid(pid_t pid)
{
    return nullptr;
}

void AppSpawningCtxTraversal(ProcessTraversal traversal, void *data)
{
    if (data == nullptr) {
        printf("AppSpawningCtxTraversal data is null");
    }
}

int DumpAppSpawnQueue(ListNode *node, void *data)
{
    return 0;
}

int DumpAppQueue(ListNode *node, void *data)
{
    return 0;
}

int DumpExtData(ListNode *node, void *data)
{
    return 0;
}

void ProcessAppSpawnDumpMsg(const AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("AppSpawningCtxTraversal data is null");
    }
}

int ProcessTerminationStatusMsg(const AppSpawnMsgNode *message, AppSpawnResult *result)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
