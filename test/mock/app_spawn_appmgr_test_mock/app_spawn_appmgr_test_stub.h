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

#ifndef APPSPAWN_TEST_STUB_H
#define APPSPAWN_TEST_STUB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "cJSON.h"
#include "appspawn_client.h"
#include "appspawn_hook.h"
#include "appspawn_encaps.h"

#ifdef __cplusplus
extern "C" {
#endif

AppSpawnMgr *CreateAppSpawnMgr(int mode);
AppSpawnContent *GetAppSpawnContent(void);
AppSpawnContent *GetAppSpawnContent(void);
void SpawningQueueDestroy(ListNode *node);
void ExtDataDestroy(ListNode *node);
void DeleteAppSpawnMgr(AppSpawnMgr *mgr);
void TraversalSpawnedProcess(AppTraversal traversal, void *data);
int AppInfoPidComparePro(ListNode *node, void *data);
int AppInfoNameComparePro(ListNode *node, void *data);
int AppInfoCompareProc(ListNode *node, ListNode *newNode);
AppSpawnedProcess *AddSpawnedProcess(pid_t pid, const char *processName, uint32_t appIndex, bool isDebuggable);
void TerminateSpawnedProcess(AppSpawnedProcess *node);
AppSpawnedProcess *GetSpawnedProcess(pid_t pid);
AppSpawnedProcess *GetSpawnedProcessByName(const char *name);
int KillAndWaitStatus(pid_t pid, int sig, int *exitStatus);
int GetProcessTerminationStatus(pid_t pid);
AppSpawningCtx *CreateAppSpawningCtx(void);
void DeleteAppSpawningCtx(AppSpawningCtx *property);
int AppPropertyComparePid(ListNode *node, void *data);
AppSpawningCtx *GetAppSpawningCtxByPid(pid_t pid);
void AppSpawningCtxTraversal(ProcessTraversal traversal, void *data);
int DumpAppSpawnQueue(ListNode *node, void *data);
int DumpAppQueue(ListNode *node, void *data);
int DumpExtData(ListNode *node, void *data);
void ProcessAppSpawnDumpMsg(const AppSpawnMsgNode *message);
int ProcessTerminationStatusMsg(const AppSpawnMsgNode *message, AppSpawnResult *result);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
