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

void SetFdCtrl(int fd, int opt)
{
    printf("SetFdCtrl %d %d", fd, opt);
}

void AppQueueDestroyProc(const AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data)
{
    if (mgr == nullptr) {
        printf("AppQueueDestroyProc mgr is null");
    }
}

void StopAppSpawn(void)
{
    printf("StopAppSpawn");
}

void DumpStatus(const char *appName, pid_t pid, int status, int *signal)
{
    printf("DumpStatus %d", status);
}

void WriteSignalInfoToFd(AppSpawnedProcess *appInfo, AppSpawnContent *content, int signal)
{
    printf("WriteSignalInfoToFd %d", signal);
}

void HandleDiedPid(pid_t pid, uid_t uid, int status)
{
    printf("HandleDiedPid %d", status);
}

void ProcessSignal(const struct signalfd_siginfo *siginfo)
{
    if (siginfo == nullptr) {
        printf("ProcessSignal siginfo is null");
    }
}

void AppSpawningCtxOnClose(const AppSpawnMgr *mgr, AppSpawningCtx *ctx, void *data)
{
    if (data == nullptr) {
        printf("AppSpawningCtxOnClose data is null");
    }
}

void OnClose(const TaskHandle taskHandle)
{
    if (taskHandle == nullptr) {
        printf("OnClose taskHandle is null");
    }
}

void OnDisConnect(const TaskHandle taskHandle)
{
    if (taskHandle == nullptr) {
        printf("OnDisConnect taskHandle is null");
    }
}

void SendMessageComplete(const TaskHandle taskHandle, BufferHandle handle)
{
    if (taskHandle == nullptr) {
        printf("SendMessageComplete taskHandle is null");
    }
}

int SendResponse(const AppSpawnConnection *connection, const AppSpawnMsg *msg, int result, pid_t pid)
{
    return 0;
}

void WaitMsgCompleteTimeOut(const TimerHandle taskHandle, void *context)
{
    if (taskHandle == nullptr) {
        printf("WaitMsgCompleteTimeOut taskHandle is null");
    }
}

int StartTimerForCheckMsg(AppSpawnConnection *connection)
{
    return 0;
}

int HandleRecvMessage(const TaskHandle taskHandle, uint8_t * buffer, int bufferSize, int flags)
{
    return 0;
}

bool OnConnectionUserCheck(uid_t uid)
{
    return true;
}

int OnConnection(const LoopHandle loopHandle, const TaskHandle server)
{
    return 0;
}

void OnListenFdClose(const TaskHandle taskHandle)
{
    if (taskHandle == nullptr) {
        printf("OnListenFdClose taskHandle is null");
    }
}

bool MsgDevicedebugCheck(TaskHandle stream, AppSpawnMsgNode *message)
{
    return true;
}

void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen)
{
    if (taskHandle == nullptr) {
        printf("OnReceiveRequest taskHandle is null");
    }
}

char *GetMapMem(uint32_t clientId, const char *processName, uint32_t size, bool readOnly, bool isNweb)
{
    return nullptr;
}

int WriteMsgToChild(AppSpawningCtx *property, bool isNweb)
{
    return 0;
}

int InitForkContext(AppSpawningCtx *property)
{
    return 0;
}

void ClosePidfdWatcher(const TaskHandle taskHandle)
{
    if (taskHandle == nullptr) {
        printf("ClosePidfdWatcher taskHandle is null");
    }
}

void ProcessChildProcessFd(const WatcherHandle taskHandle, int fd, uint32_t *events, const void *context)
{
    if (taskHandle == nullptr) {
        printf("ProcessChildProcessFd taskHandle is null");
    }
}

int OpenPidFd(pid_t pid, unsigned int flags)
{
    return 0;
}

void WatchChildProcessFd(AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("AppSpawningCtx property is null");
    }
}

int IsChildColdRun(AppSpawningCtx *property)
{
    return 0;
}

int AddChildWatcher(AppSpawningCtx *property)
{
    return 0;
}

bool IsSupportRunHnp()
{
    return true;
}

void ClearMMAP(int clientId, uint32_t memSize)
{
    printf("ClearMMAP %d %d", clientId, memSize);
}

void ClearPreforkInfo(AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("ClearPreforkInfo property is null");
    }
}

int WritePreforkMsg(AppSpawningCtx *property, uint32_t memSize)
{
    return 0;
}

int GetAppSpawnMsg(AppSpawningCtx *property, uint32_t memSize)
{
    return 0;
}

int SetPreforkProcessName(AppSpawnContent *content)
{
    return 0;
}

void ClearPipeFd(int pipe[], int length)
{
    printf("ClearPipeFd %d", length);
}

void ProcessPreFork(AppSpawnContent *content, AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("ProcessPreFork property is null");
    }
}

int NormalSpawnChild(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid)
{
    return 0;
}

int AppSpawnProcessMsgForPrefork(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid)
{
    return 0;
}

bool IsSupportPrefork(AppSpawnContent *content, AppSpawnClient *client)
{
    return true;
}

bool IsBootFinished(void)
{
    return true;
}

int RunAppSpawnProcessMsg(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid)
{
    return 0;
}

void ProcessSpawnReqMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (connection == nullptr) {
        printf("ProcessSpawnReqMsg connection is null");
    }
}

void WaitChildDied(pid_t pid)
{
    printf("WaitChildDied %d", pid);
}

void WaitChildTimeout(const TimerHandle taskHandle, void *context)
{
    if (taskHandle == nullptr) {
        printf("WaitChildTimeout taskHandle is null");
    }
}

int ProcessChildFdCheck(int fd, AppSpawningCtx *property)
{
    return 0;
}

void ProcessChildResponse(const WatcherHandle taskHandle, int fd, uint32_t *events, const void *context)
{
    if (taskHandle == nullptr) {
        printf("ProcessChildResponse taskHandle is null");
    }
}

void NotifyResToParent(AppSpawnContent *content, AppSpawnClient *client, int result)
{
    if (content == nullptr) {
        printf("NotifyResToParent content is null");
    }
}

int CreateAppSpawnServer(TaskHandle *server, const char *socketName)
{
    return 0;
}

void AppSpawnDestroyContent(AppSpawnContent *content)
{
    if (content == nullptr) {
        printf("AppSpawnDestroyContent content is null");
    }
}

int AppSpawnColdStartApp(struct AppSpawnContent *content, AppSpawnClient *client)
{
    return 0;
}

AppSpawningCtx *GetAppSpawningCtxFromArg(AppSpawnMgr *content, int argc, char *const argv[])
{
    return nullptr;
}

void AppSpawnColdRun(AppSpawnContent *content, int argc, char *const argv[])
{
    if (content == nullptr) {
        printf("AppSpawnColdRun content is null");
    }
}

void AppSpawnRun(AppSpawnContent *content, int argc, char *const argv[])
{
    if (content == nullptr) {
        printf("AppSpawnRun content is null");
    }
}

int AppSpawnClearEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

int IsEnablePrefork(void)
{
    return 0;
}

AppSpawnContent *AppSpawnCreateContent(const char *socketName, char *longProcName, uint32_t nameLen, int mode)
{
    return nullptr;
}

AppSpawnMsgNode *ProcessSpawnBegetctlMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    return nullptr;
}

void ProcessBegetCmdMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (connection == nullptr) {
        printf("ProcessBegetCmdMsg connection is null");
    }
}

int ProcessSpawnRemountMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    printf("ProcessSpawnRemountMsg, do not handle it");
    return 0;
}

void ProcessSpawnRestartMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    printf("ProcessSpawnRestartMsg, do not handle it");
}

void ProcessUninstallDebugHap(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    printf("ProcessUninstallDebugHap start");
}

int AppspawpnDevicedebugKill(int pid, cJSON *args)
{
    return 0;
}

int AppspawnDevicedebugDeal(const char* op, int pid, cJSON *args)
{
    return 0;
}

int ProcessAppSpawnDeviceDebugMsg(AppSpawnMsgNode *message)
{
    return 0;
}

void ProcessAppSpawnLockStatusMsg(AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("ProcessAppSpawnLockStatusMsg message is null");
    }
}

int AppSpawnReqMsgFdGet(AppSpawnConnection *connection, AppSpawnMsgNode *message,
    const char *fdName, int *fd)
{
    return 0;
}

void ProcessObserveProcessSignalMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("ProcessObserveProcessSignalMsg message is null");
    }
}

void ProcessRecvMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("ProcessRecvMsg message is null");
    }
}

#ifdef __cplusplus
}
#endif
