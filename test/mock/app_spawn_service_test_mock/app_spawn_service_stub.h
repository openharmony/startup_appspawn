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

void SetFdCtrl(int fd, int opt);
void AppQueueDestroyProc(const AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data);
void StopAppSpawn(void);
void DumpStatus(const char *appName, pid_t pid, int status, int *signal);
void WriteSignalInfoToFd(AppSpawnedProcess *appInfo, AppSpawnContent *content, int signal);
void HandleDiedPid(pid_t pid, uid_t uid, int status);
void ProcessSignal(const struct signalfd_siginfo *siginfo);
void OnClose(const TaskHandle taskHandle);
void OnDisConnect(const TaskHandle taskHandle);
void SendMessageComplete(const TaskHandle taskHandle, BufferHandle handle);
int SendResponse(const AppSpawnConnection *connection, const AppSpawnMsg *msg, int result, pid_t pid);
void WaitMsgCompleteTimeOut(const TimerHandle taskHandle, void *context);
int StartTimerForCheckMsg(AppSpawnConnection *connection);
int HandleRecvMessage(const TaskHandle taskHandle, uint8_t *buffer, int bufferSize, int flags);
bool OnConnectionUserCheck(uid_t uid);
int OnConnection(const LoopHandle loopHandle, const TaskHandle server);
void OnListenFdClose(const TaskHandle taskHandle);
bool MsgDevicedebugCheck(TaskHandle stream, AppSpawnMsgNode *message);
void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen);
char *GetMapMem(uint32_t clientId, const char *processName, uint32_t size, bool readOnly, bool isNweb);
int WriteMsgToChild(AppSpawningCtx *property, bool isNweb);
int InitForkContext(AppSpawningCtx *property);
void ClosePidfdWatcher(const TaskHandle taskHandle);
void ProcessChildProcessFd(const WatcherHandle taskHandle, int fd, uint32_t *events, const void *context);
int OpenPidFd(pid_t pid, unsigned int flags);
void WatchChildProcessFd(AppSpawningCtx *property);
int IsChildColdRun(AppSpawningCtx *property);
int AddChildWatcher(AppSpawningCtx *property);
bool IsSupportRunHnp();
void ClearMMAP(int clientId, uint32_t memSize);
void ClearPreforkInfo(AppSpawningCtx *property);
int WritePreforkMsg(AppSpawningCtx *property, uint32_t memSize);
int GetAppSpawnMsg(AppSpawningCtx *property, uint32_t memSize);
int SetPreforkProcessName(AppSpawnContent *content);
void ClearPipeFd(int pipe[], int length);
void ProcessPreFork(AppSpawnContent *content, AppSpawningCtx *property);
int NormalSpawnChild(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid);
int AppSpawnProcessMsgForPrefork(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid);
bool IsSupportPrefork(AppSpawnContent *content, AppSpawnClient *client);
bool IsBootFinished(void);
int RunAppSpawnProcessMsg(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid);
void ProcessSpawnReqMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);
void WaitChildDied(pid_t pid);
void WaitChildTimeout(const TimerHandle taskHandle, void *context);
int ProcessChildFdCheck(int fd, AppSpawningCtx *property);
void ProcessChildResponse(const WatcherHandle taskHandle, int fd, uint32_t *events, const void *context);
int CreateAppSpawnServer(TaskHandle *server, const char *socketName);
void AppSpawnDestroyContent(AppSpawnContent *content);
int AppSpawnColdStartApp(struct AppSpawnContent *content, AppSpawnClient *client);
AppSpawningCtx *GetAppSpawningCtxFromArg(AppSpawnMgr *content, int argc, char *const argv[]);
void AppSpawnColdRun(AppSpawnContent *content, int argc, char *const argv[]);
void AppSpawnRun(AppSpawnContent *content, int argc, char *const argv[]);
int AppSpawnClearEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int IsEnablePrefork(void);
AppSpawnContent *AppSpawnCreateContent(const char *socketName, char *longProcName, uint32_t nameLen, int mode);
AppSpawnMsgNode *ProcessSpawnBegetctlMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);
void ProcessBegetCmdMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);
int ProcessSpawnRemountMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);
void ProcessSpawnRestartMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);
void ProcessUninstallDebugHap(AppSpawnConnection *connection, AppSpawnMsgNode *message);
int AppspawpnDevicedebugKill(int pid, cJSON *args);
int AppspawnDevicedebugDeal(const char* op, int pid, cJSON *args);
int ProcessAppSpawnDeviceDebugMsg(AppSpawnMsgNode *message);
void ProcessAppSpawnLockStatusMsg(AppSpawnMsgNode *message);
int AppSpawnReqMsgFdGet(AppSpawnConnection *connection, AppSpawnMsgNode *message,
    const char *fdName, int *fd);
void ProcessObserveProcessSignalMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
