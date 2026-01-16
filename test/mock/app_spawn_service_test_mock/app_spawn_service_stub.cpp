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

    if (appInfo == nullptr) {
        printf("AppQueueDestroyProc appInfo is null");
    }

    if (data == nullptr) {
        printf("AppQueueDestroyProc data is null");
    }
}

void StopAppSpawn(void)
{
    printf("StopAppSpawn");
}

void DumpStatus(const char *appName, pid_t pid, int status, int *signal)
{
    if (appName == nullptr) {
        printf("DumpStatus appName is null");
    }

    if (pid == 0) {
        printf("DumpStatus pid is 0");
    }

    if (status == 0) {
        printf("DumpStatus status is 0");
    }

    if (signal == nullptr) {
        printf("DumpStatus signal is null");
    }
}

void WriteSignalInfoToFd(AppSpawnedProcess *appInfo, AppSpawnContent *content, int signal)
{
    if (appInfo == nullptr) {
        printf("WriteSignalInfoToFd appInfo is null");
    }

    if (content == nullptr) {
        printf("WriteSignalInfoToFd content is null");
    }

    if (signal == 0) {
        printf("WriteSignalInfoToFd signal is 0");
    }
}

void HandleDiedPid(pid_t pid, uid_t uid, int status)
{
    if (pid == 0) {
        printf("HandleDiedPid pid is 0");
    }

    if (uid == 0) {
        printf("HandleDiedPid uid is 0");
    }

    if (status == 0) {
        printf("HandleDiedPid status is 0");
    }
}

void ProcessSignal(const struct signalfd_siginfo *siginfo)
{
    if (siginfo == nullptr) {
        printf("ProcessSignal siginfo is null");
    }
}

void AppSpawningCtxOnClose(const AppSpawnMgr *mgr, AppSpawningCtx *ctx, void *data)
{
    if (mgr == nullptr) {
        printf("AppSpawningCtxOnClose mgr is null");
    }

    if (ctx == nullptr) {
        printf("AppSpawningCtxOnClose ctx is null");
    }

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

    if (handle == nullptr) {
        printf("SendMessageComplete handle is null");
    }
}

int SendResponse(const AppSpawnConnection *connection, const AppSpawnMsg *msg, int result, pid_t pid)
{
    if (connection == nullptr) {
        printf("SendResponse connection is null");
    }

    if (msg == nullptr) {
        printf("SendResponse msg is null");
    }

    if (result == 0) {
        printf("SendResponse result is null");
    }

    if (pid == 0) {
        printf("SendResponse pid is null");
    }

    return 0;
}

void WaitMsgCompleteTimeOut(const TimerHandle taskHandle, void *context)
{
    if (taskHandle == nullptr) {
        printf("WaitMsgCompleteTimeOut taskHandle is null");
    }

    if (context == nullptr) {
        printf("WaitMsgCompleteTimeOut context is null");
    }
}

int StartTimerForCheckMsg(AppSpawnConnection *connection)
{
    if (connection == nullptr) {
        printf("StartTimerForCheckMsg connection is null");
    }

    return 0;
}

int HandleRecvMessage(const TaskHandle taskHandle, uint8_t * buffer, int bufferSize, int flags)
{
    if (taskHandle == nullptr) {
        printf("HandleRecvMessage taskHandle is null");
    }

    if (buffer == nullptr) {
        printf("HandleRecvMessage buffer is null");
    }

    if (bufferSize == 0) {
        printf("HandleRecvMessage bufferSize is 0");
    }

    if (flags == 0) {
        printf("HandleRecvMessage flags is 0");
    }

    return 0;
}

bool OnConnectionUserCheck(uid_t uid)
{
    if (uid == 0) {
        printf("OnConnectionUserCheck uid is 0");
    }

    return true;
}

int OnConnection(const LoopHandle loopHandle, const TaskHandle server)
{
    if (loopHandle == nullptr) {
        printf("OnConnection loopHandle is null");
    }

    if (server == nullptr) {
        printf("OnConnection server is null");
    }

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
    if (stream == nullptr) {
        printf("MsgDevicedebugCheck stream is null");
    }
    
    if (message == nullptr) {
        printf("MsgDevicedebugCheck message is null");
    }

    return true;
}

void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen)
{
    if (taskHandle == nullptr) {
        printf("OnReceiveRequest taskHandle is null");
    }

    if (buffer == nullptr) {
        printf("OnReceiveRequest buffer is null");
    }

    if (buffLen == 0) {
        printf("OnReceiveRequest buffLen is 0");
    }
}

char *GetMapMem(uint32_t clientId, const char *processName, uint32_t size, bool readOnly, RunMode runMode)
{
    if (clientId == 0) {
        printf("GetMapMem clientId is 0");
    }

    if (processName == nullptr) {
        printf("GetMapMem processName is null");
    }

    if (size == 0) {
        printf("GetMapMem size is 0");
    }

    if (readOnly == true) {
        printf("GetMapMem readOnly is true");
    }

    if (isNweb == true) {
        printf("GetMapMem isNweb is true");
    }

    return nullptr;
}

int WriteMsgToChild(AppSpawningCtx *property, RunMode mode)
{
    if (property == nullptr) {
        printf("WriteMsgToChild property is null");
    }

    if (isNweb == true) {
        printf("WriteMsgToChild isNweb is true");
    }

    return 0;
}

int InitForkContext(AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("InitForkContext property is null");
    }

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

    if (fd == 0) {
        printf("ProcessChildProcessFd fd is 0");
    }

    if (events == nullptr) {
        printf("ProcessChildProcessFd events is null");
    }

    if (context == nullptr) {
        printf("ProcessChildProcessFd context is null");
    }
}

int OpenPidFd(pid_t pid, unsigned int flags)
{
    if (pid == 0) {
        printf("OpenPidFd pid is null");
    }

    if (flags == 0) {
        printf("OpenPidFd flags is null");
    }

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
    if (property == nullptr) {
        printf("IsChildColdRun property is nulls");
    }

    return 0;
}

int AddChildWatcher(AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("AddChildWatcher property is null");
    }

    return 0;
}

bool IsSupportRunHnp()
{
    return true;
}

void ClearMMAP(int clientId, uint32_t memSize)
{
    if (clientId == 0) {
        printf("ClearMMAP clientId is 0");
    }

    if (memSize == 0) {
        printf("ClearMMAP memSize is 0");
    }
}

void ClearPreforkInfo(AppSpawningCtx *property)
{
    if (property == nullptr) {
        printf("ClearPreforkInfo property is null");
    }
}

int WritePreforkMsg(AppSpawningCtx *property, uint32_t memSize)
{
    if (property == nullptr) {
        printf("WritePreforkMsg property is null");
    }

    if (memSize == 0) {
        printf("WritePreforkMsg memSize is 0");
    }

    return 0;
}

int GetAppSpawnMsg(AppSpawningCtx *property, uint32_t memSize)
{
    if (property == nullptr) {
        printf("GetAppSpawnMsg property is null");
    }

    if (memSize == 0) {
        printf("GetAppSpawnMsg memSize is 0");
    }

    return 0;
}

int SetPreforkProcessName(AppSpawnContent *content)
{
    if (content == nullptr) {
        printf("SetPreforkProcessName content is null");
    }

    return 0;
}

void ClearPipeFd(int pipe[], int length)
{
    if (pipe == nullptr) {
        printf("ClearPipeFd pipe is null");
    }

    if (length == 0) {
        printf("ClearPipeFd length is 0");
    }
}

void ProcessPreFork(AppSpawnContent *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("ProcessPreFork content is null");
    }

    if (property == nullptr) {
        printf("ProcessPreFork property is null");
    }
}

int NormalSpawnChild(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid)
{
    if (content == nullptr) {
        printf("NormalSpawnChild content is null");
    }

    if (client == nullptr) {
        printf("NormalSpawnChild client is null");
    }

    if (childPid == nullptr) {
        printf("NormalSpawnChild childPid is null");
    }

    return 0;
}

int AppSpawnProcessMsgForPrefork(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid)
{
    if (content == nullptr) {
        printf("AppSpawnProcessMsgForPrefork content is null");
    }

    if (client == nullptr) {
        printf("AppSpawnProcessMsgForPrefork client is null");
    }

    if (childPid == nullptr) {
        printf("AppSpawnProcessMsgForPrefork childPid is null");
    }

    return 0;
}

bool IsSupportPrefork(AppSpawnContent *content, AppSpawnClient *client)
{
    if (content == nullptr) {
        printf("IsSupportPrefork content is null");
    }

    if (client == nullptr) {
        printf("IsSupportPrefork client is null");
    }

    return true;
}

bool IsBootFinished(void)
{
    return true;
}

int RunAppSpawnProcessMsg(AppSpawnContent *content, AppSpawnClient *client, pid_t *childPid)
{
    if (content == nullptr) {
        printf("RunAppSpawnProcessMsg content is null");
    }

    if (client == nullptr) {
        printf("RunAppSpawnProcessMsg client is null");
    }

    if (childPid == nullptr) {
        printf("RunAppSpawnProcessMsg childPid is null");
    }

    return 0;
}

void ProcessSpawnReqMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (connection == nullptr) {
        printf("ProcessSpawnReqMsg connection is null");
    }

    if (message == nullptr) {
        printf("ProcessSpawnReqMsg message is null");
    }
}

void WaitChildDied(pid_t pid)
{
    if (pid == 0) {
        printf("WaitChildDied pid is 0");
    }
}

void WaitChildTimeout(const TimerHandle taskHandle, void *context)
{
    if (taskHandle == nullptr) {
        printf("WaitChildTimeout taskHandle is null");
    }

    if (context == nullptr) {
        printf("WaitChildTimeout context is null");
    }
}

int ProcessChildFdCheck(int fd, AppSpawningCtx *property)
{
    if (fd == 0) {
        printf("ProcessChildFdCheck fd is 0");
    }

    if (property == nullptr) {
        printf("ProcessChildFdCheck property is null");
    }

    return 0;
}

void ProcessChildResponse(const WatcherHandle taskHandle, int fd, uint32_t *events, const void *context)
{
    if (taskHandle == nullptr) {
        printf("ProcessChildResponse taskHandle is null");
    }

    if (fd == 0) {
        printf("ProcessChildResponse fd is 0");
    }

    if (events == nullptr) {
        printf("ProcessChildResponse is null");
    }

    if (context == nullptr) {
        printf("ProcessChildResponse is null");
    }
}

void NotifyResToParent(AppSpawnContent *content, AppSpawnClient *client, int result)
{
    if (content == nullptr) {
        printf("NotifyResToParent content is null");
    }

    if (client == nullptr) {
        printf("NotifyResToParent client is null");
    }

    if (result == 0) {
        printf("NotifyResToParent result is 0");
    }
}

int CreateAppSpawnServer(TaskHandle *server, const char *socketName)
{
    if (server == nullptr) {
        printf("CreateAppSpawnServer server is null");
    }

    if (socketName == nullptr) {
        printf("CreateAppSpawnServer socketName is null");
    }

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
    if (content == nullptr) {
        printf("AppSpawnColdStartApp content is null");
    }

    if (client == nullptr) {
        printf("AppSpawnColdStartApp client is null");
    }

    return 0;
}

AppSpawningCtx *GetAppSpawningCtxFromArg(AppSpawnMgr *content, int argc, char *const argv[])
{
    if (content == nullptr) {
        printf("GetAppSpawningCtxFromArg content is null");
    }

    if (argc == 0) {
        printf("GetAppSpawningCtxFromArg argc is 0");
    }

    if (argv == nullptr) {
        printf("GetAppSpawningCtxFromArg argv is null");
    }

    return nullptr;
}

void AppSpawnColdRun(AppSpawnContent *content, int argc, char *const argv[])
{
    if (content == nullptr) {
        printf("AppSpawnColdRun content is null");
    }

    if (argc == 0) {
        printf("AppSpawnColdRun argc is 0");
    }

    if (argv == nullptr) {
        printf("AppSpawnColdRun argv is null");
    }
}

void AppSpawnRun(AppSpawnContent *content, int argc, char *const argv[])
{
    if (content == nullptr) {
        printf("AppSpawnRun content is null");
    }

    if (argc == 0) {
        printf("AppSpawnRun argc is 0");
    }

    if (argv == nullptr) {
        printf("AppSpawnRun argv is null");
    }
}

int AppSpawnClearEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("AppSpawnClearEnv content is null");
    }

    if (property == nullptr) {
        printf("AppSpawnClearEnv property is null");
    }

    return 0;
}

int IsEnablePrefork(void)
{
    return 0;
}

AppSpawnContent *AppSpawnCreateContent(const char *socketName, char *longProcName, uint32_t nameLen, int mode)
{
    if (socketName == nullptr) {
        printf("AppSpawnCreateContent socketName is null");
    }

    if (longProcName == nullptr) {
        printf("AppSpawnCreateContent longProcName is null");
    }

    if (nameLen == 0) {
        printf("AppSpawnCreateContent nameLen is 0");
    }

    if (mode == 0) {
        printf("AppSpawnCreateContent mode is 0");
    }

    return nullptr;
}

AppSpawnMsgNode *ProcessSpawnBegetctlMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (connection == nullptr) {
        printf("ProcessSpawnBegetctlMsg connection is null");
    }

    if (message == nullptr) {
        printf("ProcessSpawnBegetctlMsg message is null");
    }

    return nullptr;
}

void ProcessBegetCmdMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (connection == nullptr) {
        printf("ProcessBegetCmdMsg connection is null");
    }

    if (message == nullptr) {
        printf("ProcessBegetCmdMsg message is null");
    }
}

int ProcessSpawnRemountMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (connection == nullptr) {
        printf("ProcessSpawnRemountMsg connection is null");
    }

    if (message == nullptr) {
        printf("ProcessSpawnRemountMsg message is null");
    }

    return 0;
}

void ProcessSpawnRestartMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (connection == nullptr) {
        printf("ProcessSpawnRestartMsg connection is null");
    }

    if (message == nullptr) {
        printf("ProcessSpawnRestartMsg message is null");
    }
}

void ProcessUninstallDebugHap(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (connection == nullptr) {
        printf("ProcessUninstallDebugHap connection is null");
    }

    if (message == nullptr) {
        printf("ProcessUninstallDebugHap message is null");
    }
}

int AppspawpnDevicedebugKill(int pid, cJSON *args)
{
    if (pid == 0) {
        printf("AppspawpnDevicedebugKill pid is 0");
    }

    if (args == nullptr) {
        printf("AppspawpnDevicedebugKill args is null");
    }

    return 0;
}

int AppspawnDevicedebugDeal(const char* op, int pid, cJSON *args)
{
    if (op == nullptr) {
        printf("AppspawnDevicedebugDeal op is null");
    }

    if (pid == 0) {
        printf("AppspawnDevicedebugDeal pid is 0");
    }

    if (args == nullptr) {
        printf("AppspawnDevicedebugDeal args is null");
    }

    return 0;
}

int ProcessAppSpawnDeviceDebugMsg(AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("ProcessAppSpawnDeviceDebugMsg message is null");
    }

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
    if (connection == nullptr) {
        printf("AppSpawnReqMsgFdGet connection is null");
    }

    if (message == nullptr) {
        printf("AppSpawnReqMsgFdGet message is null");
    }

    if (fdName == nullptr) {
        printf("AppSpawnReqMsgFdGet fdName is null");
    }

    if (fd == nullptr) {
        printf("AppSpawnReqMsgFdGet fd is null");
    }

    return 0;
}

void ProcessObserveProcessSignalMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("ProcessObserveProcessSignalMsg message is null");
    }

    if (connection == nullptr) {
        printf("ProcessObserveProcessSignalMsg connection is null");
    }
}

void ProcessRecvMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("ProcessRecvMsg message is null");
    }

    if (connection == nullptr) {
        printf("ProcessRecvMsg connection is null");
    }
}

#ifdef __cplusplus
}
#endif
