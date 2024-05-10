/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "appspawn_service.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_modulemgr.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_server.h"
#include "appspawn_utils.h"
#include "init_socket.h"
#include "init_utils.h"
#include "parameter.h"
#include "securec.h"

static void WaitChildTimeout(const TimerHandle taskHandle, void *context);
static void ProcessChildResponse(const WatcherHandle taskHandle, int fd, uint32_t *events, const void *context);
static void WaitChildDied(pid_t pid);
static void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen);
static void ProcessRecvMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);

// FD_CLOEXEC
static inline void SetFdCtrl(int fd, int opt)
{
    int option = fcntl(fd, F_GETFD);
    int ret = fcntl(fd, F_SETFD, option | opt);
    if (ret < 0) {
        APPSPAWN_LOGI("Set fd %{public}d option %{public}d %{public}d result: %{public}d", fd, option, opt, errno);
    }
}

static void AppQueueDestroyProc(const AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data)
{
    pid_t pid = appInfo->pid;
    APPSPAWN_LOGI("kill %{public}s pid: %{public}d", appInfo->name, appInfo->pid);
    // notify child proess died,clean sandbox info
    ProcessMgrHookExecute(STAGE_SERVER_APP_DIED, GetAppSpawnContent(), appInfo);
    OH_ListRemove(&appInfo->node);
    OH_ListInit(&appInfo->node);
    free(appInfo);
    if (pid > 0 && kill(pid, SIGKILL) != 0) {
        APPSPAWN_LOGE("unable to kill process, pid: %{public}d errno: %{public}d", pid, errno);
    }
}

static void StopAppSpawn(void)
{
    // delete nwespawn, and wait exit. Otherwise, the process of nwebspawn spawning will become zombie
    AppSpawnedProcess *appInfo = GetSpawnedProcessByName(NWEBSPAWN_SERVER_NAME);
    if (appInfo != NULL) {
        APPSPAWN_LOGI("kill %{public}s pid: %{public}d", appInfo->name, appInfo->pid);
        int exitStatus = 0;
        KillAndWaitStatus(appInfo->pid, SIGTERM, &exitStatus);
        OH_ListRemove(&appInfo->node);
        OH_ListInit(&appInfo->node);
        free(appInfo);
    }
    TraversalSpawnedProcess(AppQueueDestroyProc, NULL);
    APPSPAWN_LOGI("StopAppSpawn ");
    LE_StopLoop(LE_GetDefaultLoop());
}

static inline void DumpStatus(const char *appName, pid_t pid, int status)
{
    if (WIFSIGNALED(status)) {
        APPSPAWN_LOGW("%{public}s with pid %{public}d exit with signal:%{public}d", appName, pid, WTERMSIG(status));
    }
    if (WIFEXITED(status)) {
        APPSPAWN_LOGW("%{public}s with pid %{public}d exit with code:%{public}d", appName, pid, WEXITSTATUS(status));
    }
}

static void HandleDiedPid(pid_t pid, uid_t uid, int status)
{
    AppSpawnedProcess *appInfo = GetSpawnedProcess(pid);
    if (appInfo == NULL) { // If an exception occurs during app spawning, kill pid, return failed
        WaitChildDied(pid);
        DumpStatus("unknown", pid, status);
        return;
    }

    appInfo->exitStatus = status;
    APPSPAWN_CHECK_ONLY_LOG(appInfo->uid == uid, "Invalid uid %{public}u %{public}u", appInfo->uid, uid);
    DumpStatus(appInfo->name, pid, status);
    ProcessMgrHookExecute(STAGE_SERVER_APP_DIED, GetAppSpawnContent(), appInfo);

    // if current process of death is nwebspawn, restart appspawn
    if (strcmp(appInfo->name, NWEBSPAWN_SERVER_NAME) == 0) {
        OH_ListRemove(&appInfo->node);
        free(appInfo);
        APPSPAWN_LOGW("Current process of death is nwebspawn, pid = %{public}d, restart appspawn", pid);
        StopAppSpawn();
        return;
    }
    // move app info to died queue in NWEBSPAWN, or delete appinfo
    TerminateSpawnedProcess(appInfo);
}

APPSPAWN_STATIC void ProcessSignal(const struct signalfd_siginfo *siginfo)
{
    APPSPAWN_LOGI("ProcessSignal signum %{public}d", siginfo->ssi_signo);
    switch (siginfo->ssi_signo) {
        case SIGCHLD: { // delete pid from app map
            pid_t pid;
            int status;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                HandleDiedPid(pid, siginfo->ssi_uid, status);
            }
            break;
        }
        case SIGTERM: { // appswapn killed, use kill without parameter
            StopAppSpawn();
            break;
        }
        default:
            APPSPAWN_LOGI("SigHandler, unsupported signal %{public}d.", siginfo->ssi_signo);
            break;
    }
}

static void AppSpawningCtxOnClose(const AppSpawnMgr *mgr, AppSpawningCtx *ctx, void *data)
{
    if (ctx->message == NULL || ctx->message->connection != data) {
        return;
    }
    APPSPAWN_LOGI("Kill process, pid: %{public}d app: %{public}s", ctx->pid, GetProcessName(ctx));
    if (ctx->pid > 0 && kill(ctx->pid, SIGKILL) != 0) {
        APPSPAWN_LOGE("unable to kill process, pid: %{public}d errno: %{public}d", ctx->pid, errno);
    }
    DeleteAppSpawningCtx(ctx);
}

static void OnClose(const TaskHandle taskHandle)
{
    if (!IsSpawnServer(GetAppSpawnMgr())) {
        return;
    }
    AppSpawnConnection *connection = (AppSpawnConnection *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(connection != NULL, return, "Invalid connection");
    if (connection->receiverCtx.timer) {
        LE_StopTimer(LE_GetDefaultLoop(), connection->receiverCtx.timer);
        connection->receiverCtx.timer = NULL;
    }
    APPSPAWN_LOGI("OnClose connectionId: %{public}u socket %{public}d",
        connection->connectionId, LE_GetSocketFd(taskHandle));
    DeleteAppSpawnMsg(connection->receiverCtx.incompleteMsg);
    connection->receiverCtx.incompleteMsg = NULL;
    // connect close, to close spawning app
    AppSpawningCtxTraversal(AppSpawningCtxOnClose, connection);
}

static void OnDisConnect(const TaskHandle taskHandle)
{
    AppSpawnConnection *connection = (AppSpawnConnection *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(connection != NULL, return, "Invalid connection");
    APPSPAWN_LOGI("OnDisConnect connectionId: %{public}u socket %{public}d",
        connection->connectionId, LE_GetSocketFd(taskHandle));
    OnClose(taskHandle);
}

static void SendMessageComplete(const TaskHandle taskHandle, BufferHandle handle)
{
    AppSpawnConnection *connection = (AppSpawnConnection *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(connection != NULL, return, "Invalid connection");
    uint32_t bufferSize = sizeof(AppSpawnResponseMsg);
    AppSpawnResponseMsg *msg = (AppSpawnResponseMsg *)LE_GetBufferInfo(handle, NULL, &bufferSize);
    if (msg == NULL) {
        return;
    }
    AppSpawnedProcess *appInfo = GetSpawnedProcess(msg->result.pid);
    if (appInfo == NULL) {
        return;
    }
    APPSPAWN_LOGI("SendMessageComplete connectionId: %{public}u result %{public}d app %{public}s pid %{public}d",
        connection->connectionId, LE_GetSendResult(handle), appInfo->name, msg->result.pid);
    if (LE_GetSendResult(handle) != 0 && msg->result.pid > 0) {
        kill(msg->result.pid, SIGKILL);
    }
}

static int SendResponse(const AppSpawnConnection *connection, const AppSpawnMsg *msg, int result, pid_t pid)
{
    APPSPAWN_LOGV("SendResponse connectionId: %{public}u result: 0x%{public}x pid: %{public}d",
        connection->connectionId, result, pid);
    uint32_t bufferSize = sizeof(AppSpawnResponseMsg);
    BufferHandle handle = LE_CreateBuffer(LE_GetDefaultLoop(), bufferSize);
    AppSpawnResponseMsg *buffer = (AppSpawnResponseMsg *)LE_GetBufferInfo(handle, NULL, &bufferSize);
    int ret = memcpy_s(buffer, bufferSize, msg, sizeof(AppSpawnMsg));
    APPSPAWN_CHECK(ret == 0, LE_FreeBuffer(LE_GetDefaultLoop(), NULL, handle);
        return -1, "Failed to memcpy_s bufferSize");
    buffer->result.result = result;
    buffer->result.pid = pid;
    return LE_Send(LE_GetDefaultLoop(), connection->stream, handle, bufferSize);
}

static void WaitMsgCompleteTimeOut(const TimerHandle taskHandle, void *context)
{
    AppSpawnConnection *connection = (AppSpawnConnection *)context;
    APPSPAWN_LOGE("Long time no msg complete so close connectionId: %{public}u", connection->connectionId);
    DeleteAppSpawnMsg(connection->receiverCtx.incompleteMsg);
    connection->receiverCtx.incompleteMsg = NULL;
    LE_CloseStreamTask(LE_GetDefaultLoop(), connection->stream);
}

static inline int StartTimerForCheckMsg(AppSpawnConnection *connection)
{
    if (connection->receiverCtx.timer != NULL) {
        return 0;
    }
    int ret = LE_CreateTimer(LE_GetDefaultLoop(), &connection->receiverCtx.timer, WaitMsgCompleteTimeOut, connection);
    if (ret == 0) {
        ret = LE_StartTimer(LE_GetDefaultLoop(), connection->receiverCtx.timer, MAX_WAIT_MSG_COMPLETE, 1);
    }
    return ret;
}

static int OnConnection(const LoopHandle loopHandle, const TaskHandle server)
{
    APPSPAWN_CHECK(server != NULL && loopHandle != NULL, return -1, "Error server");
    static uint32_t connectionId = 0;
    TaskHandle stream;
    LE_StreamInfo info = {};
    info.baseInfo.flags = TASK_STREAM | TASK_PIPE | TASK_CONNECT;
    info.baseInfo.close = OnClose;
    info.baseInfo.userDataSize = sizeof(AppSpawnConnection);
    info.disConnectComplete = OnDisConnect;
    info.sendMessageComplete = SendMessageComplete;
    info.recvMessage = OnReceiveRequest;
    LE_STATUS ret = LE_AcceptStreamClient(loopHandle, server, &stream, &info);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to alloc stream");

    AppSpawnConnection *connection = (AppSpawnConnection *)LE_GetUserData(stream);
    APPSPAWN_CHECK(connection != NULL, return -1, "Failed to alloc stream");
    struct ucred cred = {-1, -1, -1};
    socklen_t credSize = sizeof(struct ucred);
    if ((getsockopt(LE_GetSocketFd(stream), SOL_SOCKET, SO_PEERCRED, &cred, &credSize) < 0) ||
        (cred.uid != DecodeUid("foundation") && cred.uid != DecodeUid("root"))) {
        APPSPAWN_LOGE("Invalid uid %{public}d from client", cred.uid);
        LE_CloseStreamTask(LE_GetDefaultLoop(), stream);
        return -1;
    }
    SetFdCtrl(LE_GetSocketFd(stream), FD_CLOEXEC);

    connection->connectionId = ++connectionId;
    connection->stream = stream;
    connection->receiverCtx.incompleteMsg = NULL;
    connection->receiverCtx.timer = NULL;
    connection->receiverCtx.msgRecvLen = 0;
    connection->receiverCtx.nextMsgId = 1;
    APPSPAWN_LOGI("OnConnection connectionId: %{public}u fd %{public}d ",
        connection->connectionId, LE_GetSocketFd(stream));
    return 0;
}

static void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen)
{
    AppSpawnConnection *connection = (AppSpawnConnection *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(connection != NULL, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
        return, "Failed to get client form socket");
    APPSPAWN_CHECK(buffLen < MAX_MSG_TOTAL_LENGTH, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
        return, "Message too long %{public}u", buffLen);

    uint32_t reminder = 0;
    uint32_t currLen = 0;
    AppSpawnMsgNode *message = connection->receiverCtx.incompleteMsg; // incomplete msg
    connection->receiverCtx.incompleteMsg = NULL;
    int ret = 0;
    do {
        APPSPAWN_LOGI("OnReceiveRequest connectionId: %{public}u start: 0x%{public}x buffLen %{public}d",
            connection->connectionId, *(uint32_t *)(buffer + currLen), buffLen - currLen);

        ret = GetAppSpawnMsgFromBuffer(buffer + currLen, buffLen - currLen,
            &message, &connection->receiverCtx.msgRecvLen, &reminder);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        if (connection->receiverCtx.msgRecvLen != message->msgHeader.msgLen) {  // recv complete msg
            connection->receiverCtx.incompleteMsg = message;
            message = NULL;
            break;
        }
        connection->receiverCtx.msgRecvLen = 0;
        if (connection->receiverCtx.timer) {
            LE_StopTimer(LE_GetDefaultLoop(), connection->receiverCtx.timer);
            connection->receiverCtx.timer = NULL;
        }
        // decode msg
        ret = DecodeAppSpawnMsg(message);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        (void)ProcessRecvMsg(connection, message);
        message = NULL;
        currLen += buffLen - reminder;
    } while (reminder > 0);

    if (message) {
        DeleteAppSpawnMsg(message);
    }
    if (ret != 0) {
        LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
        return;
    }
    if (connection->receiverCtx.incompleteMsg != NULL) { // Start the detection timer
        ret = StartTimerForCheckMsg(connection);
        APPSPAWN_CHECK(ret == 0, LE_CloseStreamTask(LE_GetDefaultLoop(), taskHandle);
            return, "Failed to create time for connection");
    }
    return;
}

static char *GetMapMem(uint32_t clientId, const char *processName, uint32_t size, bool readOnly, bool isNweb)
{
    char path[PATH_MAX] = {};
    int len = sprintf_s(path, sizeof(path), APPSPAWN_MSG_DIR "%s/%s_%d",
        isNweb ? "nwebspawn" : "appspawn", processName, clientId);
    APPSPAWN_CHECK(len > 0, return NULL, "Failed to format path %{public}s", processName);
    APPSPAWN_LOGV("GetMapMem for child %{public}s memSize %{public}u", path, size);
    int prot = PROT_READ;
    int mode = O_RDONLY;
    if (!readOnly) {
        mode = O_CREAT | O_RDWR | O_TRUNC;
        prot = PROT_READ | PROT_WRITE;
    }
    int fd = open(path, mode, S_IRWXU);
    APPSPAWN_CHECK(fd >= 0, return NULL, "Failed to open errno %{public}d path %{public}s", errno, path);
    if (!readOnly) {
        ftruncate(fd, size);
    }
    void *areaAddr = (void *)mmap(NULL, size, prot, MAP_SHARED, fd, 0);
    close(fd);
    APPSPAWN_CHECK(areaAddr != MAP_FAILED && areaAddr != NULL,
        return NULL, "Failed to map memory error %{public}d fileName %{public}s ", errno, path);
    return (char *)areaAddr;
}

APPSPAWN_STATIC int WriteMsgToChild(AppSpawningCtx *property, bool isNweb)
{
    const uint32_t memSize = (property->message->msgHeader.msgLen / 4096 + 1) * 4096; // 4096 4K
    char *buffer = GetMapMem(property->client.id, GetProcessName(property), memSize, false, isNweb);
    APPSPAWN_CHECK(buffer != NULL, return APPSPAWN_SYSTEM_ERROR,
        "Failed to map memory error %{public}d fileName %{public}s ", errno, GetProcessName(property));
    // copy msg header
    int ret = memcpy_s(buffer, memSize, &property->message->msgHeader, sizeof(AppSpawnMsg));
    if (ret == 0) {
        ret = memcpy_s((char *)buffer + sizeof(AppSpawnMsg), memSize - sizeof(AppSpawnMsg),
            property->message->buffer, property->message->msgHeader.msgLen - sizeof(AppSpawnMsg));
    }
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to copy msg fileName %{public}s ", GetProcessName(property));
        munmap((char *)buffer, memSize);
        return APPSPAWN_SYSTEM_ERROR;
    }
    property->forkCtx.msgSize = memSize;
    property->forkCtx.childMsg = buffer;
    APPSPAWN_LOGV("Write msg to child: %{public}u success", property->client.id);
    return 0;
}

static int InitForkContext(AppSpawningCtx *property)
{
    if (pipe(property->forkCtx.fd) == -1) {
        APPSPAWN_LOGE("create pipe fail, errno: %{public}d", errno);
        return errno;
    }
    int option = fcntl(property->forkCtx.fd[0], F_GETFD);
    if (option > 0) {
        (void)fcntl(property->forkCtx.fd[0], F_SETFD, option | O_NONBLOCK);
    }
    return 0;
}

static int AddChildWatcher(AppSpawningCtx *property)
{
    uint32_t timeout = GetSpawnTimeout(WAIT_CHILD_RESPONSE_TIMEOUT);
    LE_WatchInfo watchInfo = {};
    watchInfo.fd = property->forkCtx.fd[0];
    watchInfo.flags = WATCHER_ONCE;
    watchInfo.events = EVENT_READ;
    watchInfo.processEvent = ProcessChildResponse;
    LE_STATUS status = LE_StartWatcher(LE_GetDefaultLoop(), &property->forkCtx.watcherHandle, &watchInfo, property);
    APPSPAWN_CHECK(status == LE_SUCCESS,
        return APPSPAWN_SYSTEM_ERROR, "Failed to watch child %{public}d", property->pid);
    status = LE_CreateTimer(LE_GetDefaultLoop(), &property->forkCtx.timer, WaitChildTimeout, property);
    if (status == LE_SUCCESS) {
        status = LE_StartTimer(LE_GetDefaultLoop(), property->forkCtx.timer, timeout * 1000, 0); // 1000 1s
    }
    if (status != LE_SUCCESS) {
        if (property->forkCtx.timer != NULL) {
            LE_StopTimer(LE_GetDefaultLoop(), property->forkCtx.timer);
        }
        property->forkCtx.timer = NULL;
        LE_RemoveWatcher(LE_GetDefaultLoop(), property->forkCtx.watcherHandle);
        property->forkCtx.watcherHandle = NULL;
        APPSPAWN_LOGE("Failed to watch child %{public}d", property->pid);
        return APPSPAWN_SYSTEM_ERROR;
    }
    return 0;
}

static void ProcessSpawnReqMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    int ret = CheckAppSpawnMsg(message);
    if (ret != 0) {
        SendResponse(connection, &message->msgHeader, ret, 0);
        DeleteAppSpawnMsg(message);
        return;
    }

    if (IsDeveloperModeOpen()) {
        SetAppSpawnMsgFlag(message, TLV_MSG_FLAGS, APP_FLAGS_DEVELOPER_MODE);
    }

    AppSpawningCtx *property = CreateAppSpawningCtx();
    if (property == NULL) {
        SendResponse(connection, &message->msgHeader, APPSPAWN_SYSTEM_ERROR, 0);
        DeleteAppSpawnMsg(message);
        return;
    }

    property->state = APP_STATE_SPAWNING;
    property->message = message;
    message->connection = connection;
    // mount el2 dir
    // getWrapBundleNameValue
    AppSpawnHookExecute(STAGE_PARENT_PRE_FORK, 0, GetAppSpawnContent(), &property->client);
    if (IsDeveloperModeOn(property)) {
        DumpAppSpawnMsg(property->message);
    }

    if (InitForkContext(property) != 0) {
        SendResponse(connection, &message->msgHeader, APPSPAWN_SYSTEM_ERROR, 0);
        DeleteAppSpawningCtx(property);
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &property->spawnStart);
    ret = AppSpawnProcessMsg(GetAppSpawnContent(), &property->client, &property->pid);
    AppSpawnHookExecute(STAGE_PARENT_POST_FORK, 0, GetAppSpawnContent(), &property->client);
    if (ret != 0) { // wait child process result
        SendResponse(connection, &message->msgHeader, ret, 0);
        DeleteAppSpawningCtx(property);
        return;
    }
    if (AddChildWatcher(property) != 0) { // wait child process result
        kill(property->pid, SIGKILL);
        SendResponse(connection, &message->msgHeader, ret, 0);
        DeleteAppSpawningCtx(property);
        return;
    }
}

static void WaitChildDied(pid_t pid)
{
    AppSpawningCtx *property = GetAppSpawningCtxByPid(pid);
    if (property != NULL && property->state == APP_STATE_SPAWNING) {
        APPSPAWN_LOGI("Child process %{public}s fail \'child crash \'pid %{public}d appId: %{public}d",
            GetProcessName(property), property->pid, property->client.id);
        SendResponse(property->message->connection, &property->message->msgHeader, APPSPAWN_CHILD_CRASH, 0);
        DeleteAppSpawningCtx(property);
    }
}

static void WaitChildTimeout(const TimerHandle taskHandle, void *context)
{
    AppSpawningCtx *property = (AppSpawningCtx *)context;
    APPSPAWN_LOGI("Child process %{public}s fail \'wait child timeout \'pid %{public}d appId: %{public}d",
        GetProcessName(property), property->pid, property->client.id);
    if (property->pid > 0) {
        kill(property->pid, SIGKILL);
    }
    SendResponse(property->message->connection, &property->message->msgHeader, APPSPAWN_SPAWN_TIMEOUT, 0);
    DeleteAppSpawningCtx(property);
}

static void ProcessChildResponse(const WatcherHandle taskHandle, int fd, uint32_t *events, const void *context)
{
    AppSpawningCtx *property = (AppSpawningCtx *)context;
    property->forkCtx.watcherHandle = NULL;  // delete watcher
    LE_RemoveWatcher(LE_GetDefaultLoop(), (WatcherHandle)taskHandle);

    int result = 0;
    (void)read(fd, &result, sizeof(result));
    APPSPAWN_LOGI("Child process %{public}s success pid %{public}d appId: %{public}d result: %{public}d",
        GetProcessName(property), property->pid, property->client.id, result);
    APPSPAWN_CHECK(property->message != NULL, return, "Invalid message in ctx %{public}d", property->client.id);

    if (result != 0) {
        SendResponse(property->message->connection, &property->message->msgHeader, result, property->pid);
        DeleteAppSpawningCtx(property);
        return;
    }
    // success
    AppSpawnedProcess *appInfo = AddSpawnedProcess(property->pid, GetBundleName(property));
    if (appInfo) {
        AppSpawnMsgDacInfo *dacInfo = GetAppProperty(property, TLV_DAC_INFO);
        appInfo->uid = dacInfo != NULL ? dacInfo->uid : 0;
        appInfo->spawnStart.tv_sec = property->spawnStart.tv_sec;
        appInfo->spawnStart.tv_nsec = property->spawnStart.tv_nsec;
#ifdef DEBUG_BEGETCTL_BOOT
        if (IsDeveloperModeOpen()) {
            appInfo->message = property->message;
        }
#endif
        clock_gettime(CLOCK_MONOTONIC, &appInfo->spawnEnd);
        // add max info
    }
    ProcessMgrHookExecute(STAGE_SERVER_APP_ADD, GetAppSpawnContent(), appInfo);
    // response
    AppSpawnHookExecute(STAGE_PARENT_PRE_RELY, 0, GetAppSpawnContent(), &property->client);
    SendResponse(property->message->connection, &property->message->msgHeader, result, property->pid);
    AppSpawnHookExecute(STAGE_PARENT_POST_RELY, 0, GetAppSpawnContent(), &property->client);
#ifdef DEBUG_BEGETCTL_BOOT
    if (IsDeveloperModeOpen()) {
        property->message = NULL;
    }
#endif
    DeleteAppSpawningCtx(property);
}

static void NotifyResToParent(AppSpawnContent *content, AppSpawnClient *client, int result)
{
    AppSpawningCtx *property = (AppSpawningCtx *)client;
    int fd = property->forkCtx.fd[1];
    if (fd >= 0) {
        (void)write(fd, &result, sizeof(result));
        (void)close(fd);
        property->forkCtx.fd[1] = -1;
    }
    APPSPAWN_LOGV("NotifyResToParent client id: %{public}u result: 0x%{public}x", client->id, result);
}

static int CreateAppSpawnServer(TaskHandle *server, const char *socketName)
{
    char path[128] = {0};  // 128 max path
    int ret = snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s%s", APPSPAWN_SOCKET_DIR, socketName);
    APPSPAWN_CHECK(ret >= 0, return -1, "Failed to snprintf_s %{public}d", ret);
    int socketId = GetControlSocket(socketName);
    APPSPAWN_LOGI("get socket form env %{public}s socketId %{public}d", socketName, socketId);

    LE_StreamServerInfo info = {};
    info.baseInfo.flags = TASK_STREAM | TASK_PIPE | TASK_SERVER;
    info.socketId = socketId;
    info.server = path;
    info.baseInfo.close = NULL;
    info.incommingConnect = OnConnection;

    MakeDirRec(path, 0711, 0);  // 0711 default mask
    ret = LE_CreateStreamServer(LE_GetDefaultLoop(), server, &info);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to create socket for %{public}s errno: %{public}d", path, errno);
    SetFdCtrl(LE_GetSocketFd(*server), FD_CLOEXEC);

    APPSPAWN_LOGI("CreateAppSpawnServer path %{public}s fd %{public}d", path, LE_GetSocketFd(*server));
    return 0;
}

void AppSpawnDestroyContent(AppSpawnContent *content)
{
    if (content == NULL) {
        return;
    }
    AppSpawnMgr *appSpawnContent = (AppSpawnMgr *)content;
    if (appSpawnContent->sigHandler != NULL && appSpawnContent->servicePid == getpid()) {
        LE_CloseSignalTask(LE_GetDefaultLoop(), appSpawnContent->sigHandler);
    }

    if (appSpawnContent->server != NULL && appSpawnContent->servicePid == getpid()) { // childProcess can't deal socket
        LE_CloseStreamTask(LE_GetDefaultLoop(), appSpawnContent->server);
        appSpawnContent->server = NULL;
    }
    LE_StopLoop(LE_GetDefaultLoop());
    LE_CloseLoop(LE_GetDefaultLoop());
    DeleteAppSpawnMgr(appSpawnContent);
}

static int AppSpawnColdStartApp(struct AppSpawnContent *content, AppSpawnClient *client)
{
    AppSpawningCtx *property = (AppSpawningCtx *)client;
    char *path = property->forkCtx.coldRunPath != NULL ? property->forkCtx.coldRunPath : "/system/bin/appspawn";
    APPSPAWN_LOGI("ColdStartApp::processName: %{public}s path: %{public}s", GetProcessName(property), path);

    // for cold run, use shared memory to exchange message
    APPSPAWN_LOGV("Write msg to child %{public}s", GetProcessName(property));
    int ret = WriteMsgToChild(property, IsNWebSpawnMode((AppSpawnMgr *)content));
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_SYSTEM_ERROR, "Failed to write msg to child");

    char buffer[4][32] = {0};  // 4 32 buffer for fd
    char *mode = IsNWebSpawnMode((AppSpawnMgr *)content) ? "nweb_cold" : "app_cold";
    int len = sprintf_s(buffer[0], sizeof(buffer[0]), " %d ", property->forkCtx.fd[1]);
    APPSPAWN_CHECK(len > 0, return APPSPAWN_SYSTEM_ERROR, "Invalid to format fd");
    len = sprintf_s(buffer[1], sizeof(buffer[1]), " %u ", property->client.flags);
    APPSPAWN_CHECK(len > 0, return APPSPAWN_SYSTEM_ERROR, "Invalid to format flags");
    len = sprintf_s(buffer[2], sizeof(buffer[2]), " %d ", property->forkCtx.msgSize); // 2 2 index for dest path
    APPSPAWN_CHECK(len > 0, return APPSPAWN_SYSTEM_ERROR, "Invalid to format shmId ");
    len = sprintf_s(buffer[3], sizeof(buffer[3]), " %d ", property->client.id); // 3 3 index for client id
    APPSPAWN_CHECK(len > 0, return APPSPAWN_SYSTEM_ERROR, "Invalid to format shmId ");

    // 2 2 index for dest path
    const char *const formatCmds[] = {
        path, "-mode", mode, "-fd", buffer[0], buffer[1], buffer[2],
        "-param", GetProcessName(property), buffer[3], NULL
    };

    ret = execv(path, (char **)formatCmds);
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to execv, errno: %{public}d", errno);
    }
    APPSPAWN_LOGV("ColdStartApp::processName: %{public}s end", GetProcessName(property));
    return 0;
}

static AppSpawningCtx *GetAppSpawningCtxFromArg(AppSpawnMgr *content, int argc, char *const argv[])
{
    AppSpawningCtx *property = CreateAppSpawningCtx();
    APPSPAWN_CHECK(property != NULL, return NULL, "Create app spawning ctx fail");
    property->forkCtx.fd[1] = atoi(argv[FD_VALUE_INDEX]);
    property->client.flags = atoi(argv[FLAGS_VALUE_INDEX]);
    property->client.flags &= ~APP_COLD_START;

    uint32_t size = atoi(argv[SHM_SIZE_INDEX]);
    property->client.id = atoi(argv[CLIENT_ID_INDEX]);
    uint8_t *buffer = (uint8_t *)GetMapMem(property->client.id,
        argv[PARAM_VALUE_INDEX], size, true, IsNWebSpawnMode(content));
    if (buffer == NULL) {
        APPSPAWN_LOGE("Failed to map errno %{public}d %{public}s", property->client.id, argv[PARAM_VALUE_INDEX]);
        NotifyResToParent(&content->content, &property->client, APPSPAWN_SYSTEM_ERROR);
        DeleteAppSpawningCtx(property);
        return NULL;
    }

    uint32_t msgRecvLen = 0;
    uint32_t remainLen = 0;
    AppSpawnMsgNode *message = NULL;
    int ret = GetAppSpawnMsgFromBuffer(buffer, ((AppSpawnMsg *)buffer)->msgLen, &message, &msgRecvLen, &remainLen);
    // release map
    munmap((char *)buffer, size);

    if (ret == 0 && DecodeAppSpawnMsg(message) == 0 && CheckAppSpawnMsg(message) == 0) {
        property->message = message;
        message = NULL;
        return property;
    }
    NotifyResToParent(&content->content, &property->client, APPSPAWN_MSG_INVALID);
    DeleteAppSpawnMsg(message);
    DeleteAppSpawningCtx(property);
    return NULL;
}

static void AppSpawnColdRun(AppSpawnContent *content, int argc, char *const argv[])
{
    APPSPAWN_CHECK(argc >= ARG_NULL, return, "Invalid arg for cold start %{public}d", argc);
    AppSpawnMgr *appSpawnContent = (AppSpawnMgr *)content;
    APPSPAWN_CHECK(appSpawnContent != NULL, return, "Invalid appspawn content");

    AppSpawningCtx *property = GetAppSpawningCtxFromArg(appSpawnContent, argc, argv);
    if (property == NULL) {
        APPSPAWN_LOGE("Failed to get property from arg");
        return;
    }
    if (IsDeveloperModeOn(property)) {
        DumpAppSpawnMsg(property->message);
    }

    int ret = AppSpawnExecuteSpawningHook(content, &property->client);
    if (ret == 0) {
        ret = AppSpawnExecutePreReplyHook(content, &property->client);
        // success
        NotifyResToParent(content, &property->client, ret);

        (void)AppSpawnExecutePostReplyHook(content, &property->client);

        ret = APPSPAWN_SYSTEM_ERROR;
        if (content->runChildProcessor != NULL) {
            ret = content->runChildProcessor(content, &property->client);
        }
    } else {
        NotifyResToParent(content, &property->client, ret);
    }

    if (ret != 0) {
        AppSpawnEnvClear(content, &property->client);
    }
    APPSPAWN_LOGI("AppSpawnColdRun exit %{public}d.", getpid());
}

static void AppSpawnRun(AppSpawnContent *content, int argc, char *const argv[])
{
    APPSPAWN_LOGI("AppSpawnRun");
    AppSpawnMgr *appSpawnContent = (AppSpawnMgr *)content;
    APPSPAWN_CHECK(appSpawnContent != NULL, return, "Invalid appspawn content");

    LE_STATUS status = LE_CreateSignalTask(LE_GetDefaultLoop(), &appSpawnContent->sigHandler, ProcessSignal);
    if (status == 0) {
        (void)LE_AddSignal(LE_GetDefaultLoop(), appSpawnContent->sigHandler, SIGCHLD);
        (void)LE_AddSignal(LE_GetDefaultLoop(), appSpawnContent->sigHandler, SIGTERM);
    }

    LE_RunLoop(LE_GetDefaultLoop());
    APPSPAWN_LOGI("AppSpawnRun exit mode: %{public}d ", content->mode);

    (void)ServerStageHookExecute(STAGE_SERVER_EXIT, content); // service exit,plugin can deal task
    AppSpawnDestroyContent(content);
}

APPSPAWN_STATIC int AppSpawnClearEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK(content != NULL, return 0, "Invalid appspawn content");
    bool isNweb = IsNWebSpawnMode(content);
    APPSPAWN_LOGV("Clear %{public}s context in child %{public}d process", !isNweb ? "appspawn" : "nwebspawn", getpid());

    DeleteAppSpawningCtx(property);
    AppSpawnDestroyContent(&content->content);

    AppSpawnModuleMgrUnInstall(MODULE_DEFAULT);
    AppSpawnModuleMgrUnInstall(isNweb ? MODULE_APPSPAWN : MODULE_NWEBSPAWN);
    AppSpawnModuleMgrUnInstall(MODULE_COMMON);
    APPSPAWN_LOGV("clear %{public}d end", getpid());
    return 0;
}

AppSpawnContent *AppSpawnCreateContent(const char *socketName, char *longProcName, uint32_t nameLen, int mode)
{
    APPSPAWN_CHECK(socketName != NULL && longProcName != NULL, return NULL, "Invalid name");
    APPSPAWN_LOGI("AppSpawnCreateContent %{public}s %{public}u mode %{public}d", socketName, nameLen, mode);

    AppSpawnMgr *appSpawnContent = CreateAppSpawnMgr(mode);
    APPSPAWN_CHECK(appSpawnContent != NULL, return NULL, "Failed to alloc memory for appspawn");
    appSpawnContent->content.longProcName = longProcName;
    appSpawnContent->content.longProcNameLen = nameLen;
    appSpawnContent->content.notifyResToParent = NotifyResToParent;
    if (IsColdRunMode(appSpawnContent)) {
        appSpawnContent->content.runAppSpawn = AppSpawnColdRun;
    } else {
        appSpawnContent->content.runAppSpawn = AppSpawnRun;
        appSpawnContent->content.coldStartApp = AppSpawnColdStartApp;

        int ret = CreateAppSpawnServer(&appSpawnContent->server, socketName);
        APPSPAWN_CHECK(ret == 0, AppSpawnDestroyContent(&appSpawnContent->content);
            return NULL, "Failed to create server");
    }
    return &appSpawnContent->content;
}

AppSpawnContent *StartSpawnService(const AppSpawnStartArg *startArg, uint32_t argvSize, int argc, char *const argv[])
{
    APPSPAWN_CHECK(startArg != NULL && argv != NULL, return NULL, "Invalid start arg");
    pid_t pid = 0;
    AppSpawnStartArg *arg = (AppSpawnStartArg *)startArg;
    APPSPAWN_LOGV("Start appspawn argvSize %{public}d mode %{public}d service %{public}s",
        argvSize, arg->mode, arg->serviceName);
    if (arg->mode == MODE_FOR_APP_SPAWN) {
#ifndef APPSPAWN_TEST
        pid = NWebSpawnLaunch();
        if (pid == 0) {
            arg->socketName = NWEBSPAWN_SOCKET_NAME;
            arg->serviceName = NWEBSPAWN_SERVER_NAME;
            arg->moduleType = MODULE_NWEBSPAWN;
            arg->mode = MODE_FOR_NWEB_SPAWN;
            arg->initArg = 1;
        }
#endif
    } else if (arg->mode == MODE_FOR_NWEB_SPAWN) {
        NWebSpawnInit();
    }
    if (arg->initArg) {
        int ret = memset_s(argv[0], argvSize, 0, (size_t)argvSize);
        APPSPAWN_CHECK(ret == EOK, return NULL, "Failed to memset argv[0]");
        ret = strncpy_s(argv[0], argvSize, arg->serviceName, strlen(arg->serviceName));
        APPSPAWN_CHECK(ret == EOK, return NULL, "Failed to copy service name %{public}s", arg->serviceName);
    }

    // load module appspawn/common
    AppSpawnLoadAutoRunModules(MODULE_COMMON);
    AppSpawnModuleMgrInstall(ASAN_MODULE_PATH);

    APPSPAWN_CHECK(LE_GetDefaultLoop() != NULL, return NULL, "Invalid default loop");
    AppSpawnContent *content = AppSpawnCreateContent(arg->socketName, argv[0], argvSize, arg->mode);
    APPSPAWN_CHECK(content != NULL, return NULL, "Failed to create content for %{public}s", arg->socketName);

    AppSpawnLoadAutoRunModules(arg->moduleType);  // load corresponding plugin according to startup mode
    int ret = ServerStageHookExecute(STAGE_SERVER_PRELOAD, content);   // Preload, prase the sandbox
    APPSPAWN_CHECK(ret == 0, AppSpawnDestroyContent(content);
        return NULL, "Failed to prepare load %{public}s result: %{public}d", arg->serviceName, ret);
#ifndef APPSPAWN_TEST
    APPSPAWN_CHECK(content->runChildProcessor != NULL, AppSpawnDestroyContent(content);
        return NULL, "No child processor %{public}s result: %{public}d", arg->serviceName, ret);
#endif
    AddAppSpawnHook(STAGE_CHILD_PRE_RUN, HOOK_PRIO_LOWEST, AppSpawnClearEnv);
    if (arg->mode == MODE_FOR_APP_SPAWN) {
        AddSpawnedProcess(pid, NWEBSPAWN_SERVER_NAME);
        SetParameter("bootevent.appspawn.started", "true");
    }
    return content;
}

static AppSpawnMsgNode *ProcessSpawnBegetctlMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    uint32_t len = 0;
    const char *cmdMsg = (const char *)GetAppSpawnMsgExtInfo(message, MSG_EXT_NAME_BEGET_PID, &len);
    APPSPAWN_CHECK(cmdMsg != NULL, return NULL, "Failed to get extInfo");
    AppSpawnedProcess *appInfo = GetSpawnedProcess(atoi(cmdMsg));
    APPSPAWN_CHECK(appInfo != NULL, return NULL, "Failed to get app info");
    AppSpawnMsgNode *msgNode = RebuildAppSpawnMsgNode(message, appInfo);
    APPSPAWN_CHECK(msgNode != NULL, return NULL, "Failed to rebuild app message node");
    int ret = DecodeAppSpawnMsg(msgNode);
    if (ret != 0) {
        DeleteAppSpawnMsg(msgNode);
        return NULL;
    }
    return msgNode;
}

static void ProcessRecvMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message)
{
    AppSpawnMsg *msg = &message->msgHeader;
    APPSPAWN_LOGI("Recv message header magic 0x%{public}x type %{public}u id %{public}u len %{public}u %{public}s",
        msg->magic, msg->msgType, msg->msgId, msg->msgLen, msg->processName);
    APPSPAWN_CHECK_ONLY_LOG(connection->receiverCtx.nextMsgId == msg->msgId,
        "Invalid msg id %{public}u %{public}u", connection->receiverCtx.nextMsgId, msg->msgId);
    connection->receiverCtx.nextMsgId++;

    switch (msg->msgType) {
        case MSG_GET_RENDER_TERMINATION_STATUS: {  // get status
            AppSpawnResult result = {0};
            int ret = ProcessTerminationStatusMsg(message, &result);
            SendResponse(connection, msg, ret == 0 ? result.result : ret, result.pid);
            DeleteAppSpawnMsg(message);
            break;
        }
        case MSG_SPAWN_NATIVE_PROCESS:  // spawn msg
        case MSG_APP_SPAWN: {
            ProcessSpawnReqMsg(connection, message);
            break;
        }
        case MSG_DUMP:
            ProcessAppSpawnDumpMsg(message);
            SendResponse(connection, msg, 0, 0);
            DeleteAppSpawnMsg(message);
            break;
        case MSG_BEGET_CMD: {
            if (!IsDeveloperModeOpen()) {
                SendResponse(connection, msg, APPSPAWN_DEBUG_MODE_NOT_SUPPORT, 0);
                DeleteAppSpawnMsg(message);
                break;
            }
            AppSpawnMsgNode *msgNode = ProcessSpawnBegetctlMsg(connection, message);
            if (msgNode == NULL) {
                SendResponse(connection, msg, APPSPAWN_DEBUG_MODE_NOT_SUPPORT, 0);
                DeleteAppSpawnMsg(message);
                break;
            }
            ProcessSpawnReqMsg(connection, msgNode);
            DeleteAppSpawnMsg(message);
            DeleteAppSpawnMsg(msgNode);
            break;
        }
        default:
            SendResponse(connection, msg, APPSPAWN_MSG_INVALID, 0);
            DeleteAppSpawnMsg(message);
            break;
    }
}