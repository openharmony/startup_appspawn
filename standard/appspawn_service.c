/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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
#include "appspawn_adapter.h"
#include "appspawn_server.h"

#include <fcntl.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include <linux/limits.h>

#include "init_hashmap.h"
#include "init_socket.h"
#include "init_utils.h"
#include "parameter.h"
#include "securec.h"
#include "nwebspawn_lancher.h"

#ifdef REPORT_EVENT
#include "event_reporter.h"
#endif
#ifndef APPSPAWN_TEST
#define TV_SEC 60
#define APPSPAWN_EXIT_TIME 60000
#else
#define TV_SEC 2
#define APPSPAWN_EXIT_TIME 500
#endif

static AppSpawnContentExt *g_appSpawnContent = NULL;
static const uint32_t EXTRAINFO_TOTAL_LENGTH_MAX = 32 * 1024;

static int AppInfoHashNodeCompare(const HashNode *node1, const HashNode *node2)
{
    AppInfo *testNode1 = HASHMAP_ENTRY(node1, AppInfo, node);
    AppInfo *testNode2 = HASHMAP_ENTRY(node2, AppInfo, node);
    return testNode1->pid - testNode2->pid;
}

static int TestHashKeyCompare(const HashNode *node1, const void *key)
{
    AppInfo *testNode1 = HASHMAP_ENTRY(node1, AppInfo, node);
    return testNode1->pid - *(pid_t *)key;
}

static int AppInfoHashNodeFunction(const HashNode *node)
{
    AppInfo *testNode = HASHMAP_ENTRY(node, AppInfo, node);
    return testNode->pid % APP_HASH_BUTT;
}

static int AppInfoHashKeyFunction(const void *key)
{
    pid_t code = *(pid_t *)key;
    return code % APP_HASH_BUTT;
}

static void AppInfoHashNodeFree(const HashNode *node, void *context)
{
    (void)context;
    AppInfo *testNode = HASHMAP_ENTRY(node, AppInfo, node);
    APPSPAWN_LOGI("AppInfoHashNodeFree %{public}s\n", testNode->name);
    free(testNode);
}

APPSPAWN_STATIC void AddAppInfo(pid_t pid, const char *processName)
{
    size_t len = strlen(processName) + 1;
    AppInfo *node = (AppInfo *)malloc(sizeof(AppInfo) + len + 1);
    APPSPAWN_CHECK(node != NULL, return, "Failed to malloc for appinfo");

    node->pid = pid;
    int ret = strcpy_s(node->name, len, processName);
    APPSPAWN_CHECK(ret == 0, free(node);
        return, "Failed to strcpy process name");
    HASHMAPInitNode(&node->node);
    ret = OH_HashMapAdd(g_appSpawnContent->appMap, &node->node);
    APPSPAWN_CHECK(ret == 0, free(node);
        return, "Failed to add appinfo to hash");
    APPSPAWN_LOGI("Add %{public}s, pid=%{public}d success", processName, pid);
}

void AddNwebInfo(pid_t pid, const char *processName)
{
    AddAppInfo(pid, processName);
}

static AppInfo *GetAppInfo(pid_t pid)
{
    HashNode *node = OH_HashMapGet(g_appSpawnContent->appMap, (const void *)&pid);
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return NULL);
    return HASHMAP_ENTRY(node, AppInfo, node);
}

static void RemoveAppInfo(pid_t pid)
{
    AppInfo *appInfo = GetAppInfo(pid);
    APPSPAWN_CHECK(appInfo != NULL, return, "Can not find app info for %{public}d", pid);
    OH_HashMapRemove(g_appSpawnContent->appMap, (const void *)&pid);
    free(appInfo);
    if ((g_appSpawnContent->flags & FLAGS_ON_DEMAND) != FLAGS_ON_DEMAND) {
        return;
    }
}

static void KillProcess(const HashNode *node, const void *context)
{
    AppInfo *hashNode = (AppInfo *)node;
    kill(hashNode->pid, SIGKILL);
    APPSPAWN_LOGI("kill app, pid = %{public}d, processName = %{public}s", hashNode->pid, hashNode->name);
}

static void OnClose(const TaskHandle taskHandle)
{
    AppSpawnClientExt *client = (AppSpawnClientExt *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(client != NULL, return, "Invalid client");
    APPSPAWN_LOGI("OnClose %{public}d processName = %{public}s",
        client->client.id, client->property.processName);
    if (client->property.extraInfo.data != NULL) {
        free(client->property.extraInfo.data);
        client->property.extraInfo.totalLength = 0;
        client->property.extraInfo.savedLength = 0;
        client->property.extraInfo.data = NULL;
    }
}

static void SendMessageComplete(const TaskHandle taskHandle, BufferHandle handle)
{
    AppSpawnClientExt *client = (AppSpawnClientExt *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(client != NULL, return, "Failed to get client");
    APPSPAWN_LOGI("SendMessageComplete client.id %{public}d result %{public}d pid %{public}d",
        client->client.id, LE_GetSendResult(handle), client->pid);
    if (LE_GetSendResult(handle) != 0 && client->pid > 0) {
        kill(client->pid, SIGKILL);
        APPSPAWN_LOGI("Send message fail err:%{public}d kill app [ %{public}d %{public}s]",
            LE_GetSendResult(handle), client->pid, client->property.bundleName);
    }
}

static int SendResponse(AppSpawnClientExt *client, const char *buff, size_t buffSize)
{
    uint32_t bufferSize = buffSize;
    BufferHandle handle = LE_CreateBuffer(LE_GetDefaultLoop(), bufferSize);
    char *buffer = (char *)LE_GetBufferInfo(handle, NULL, &bufferSize);
    int ret = memcpy_s(buffer, bufferSize, buff, buffSize);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    return LE_Send(LE_GetDefaultLoop(), client->stream, handle, buffSize);
}

static void KillProcessesByCGroup(uid_t uid, const AppInfo *appInfo)
{
    if (appInfo == NULL) {
        return;
    }

    int userId = uid / 200000;
    char buf[PATH_MAX];

    snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "/dev/memcg/%d/%s/cgroup.procs", userId, appInfo->name);
    FILE *file = fopen(buf, "r");
    APPSPAWN_CHECK(file != NULL, return, "%{public}s not exists.", buf);
    pid_t pid;
    while (fscanf_s(file, "%d\n", &pid) == 1 && pid > 0) {
        // If it is the app process itself, no need to kill again
        if (pid == appInfo->pid) {
            continue;
        }

        // If it is another process spawned by appspawn with the same package name, just ignore
        AppInfo *newApp = GetAppInfo(pid);
        if (newApp != NULL) {
            APPSPAWN_LOGI("Got app %{public}s in same group for pid %{public}d.", newApp->name, pid);
            continue;
        }
        APPSPAWN_LOGI("Kill app pid %{public}d now ...", pid);
        kill(pid, SIGKILL);
    }
    fclose(file);
}

static void HandleDiedPid(pid_t pid, uid_t uid, int status)
{
    AppInfo *appInfo = GetAppInfo(pid);
    KillProcessesByCGroup(uid, appInfo);
    APPSPAWN_CHECK(appInfo != NULL, return, "Can not find app info for %{public}d", pid);
    if (WIFSIGNALED(status)) {
        APPSPAWN_LOGW("%{public}s with pid %{public}d exit with signal:%{public}d",
            appInfo->name, pid, WTERMSIG(status));
    }
    if (WIFEXITED(status)) {
        APPSPAWN_LOGW("%{public}s with pid %{public}d exit with code:%{public}d",
            appInfo->name, pid, WEXITSTATUS(status));
    }

#ifdef REPORT_EVENT
    ReportProcessExitInfo(appInfo->name, pid, uid, status);
#endif

    // delete app info
    RemoveAppInfo(pid);

    // if current process of death is nwebspawn, restart appspawn
    if (pid == g_nwebspawnpid) {
        APPSPAWN_LOGW("Current process of death is nwebspawn, pid = %{public}d, restart appspawn", pid);
        OH_HashMapTraverse(g_appSpawnContent->appMap, KillProcess, NULL);
        LE_StopLoop(LE_GetDefaultLoop());
    }
}

static void HandleDiedPidNweb(pid_t pid, uid_t uid, int status)
{
    AppInfo *appInfo = GetAppInfo(pid);
    APPSPAWN_CHECK(appInfo != NULL, return, "Can not find app info for %{public}d", pid);
    if (WIFSIGNALED(status)) {
        APPSPAWN_LOGW("%{public}s with pid %{public}d exit with signal:%{public}d",
            appInfo->name, pid, WTERMSIG(status));
    }
    if (WIFEXITED(status)) {
        APPSPAWN_LOGW("%{public}s with pid %{public}d exit with code:%{public}d",
            appInfo->name, pid, WEXITSTATUS(status));
    }

#ifdef REPORT_EVENT
    ReportProcessExitInfo(appInfo->name, pid, uid, status);
#endif

    // nwebspawn will invoke waitpid and remove appinfo at GetProcessTerminationStatusInner when
    // GetProcessTerminationStatusInner is called before the parent process receives the SIGCHLD signal.
    RecordRenderProcessExitedStatus(pid, status);

    // delete app info
    RemoveAppInfo(pid);
}

APPSPAWN_STATIC void SignalHandler(const struct signalfd_siginfo *siginfo)
{
    APPSPAWN_LOGI("SignalHandler signum %{public}d", siginfo->ssi_signo);
    switch (siginfo->ssi_signo) {
        case SIGCHLD: {  // delete pid from app map
            pid_t pid;
            int status;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                HandleDiedPid(pid, siginfo->ssi_uid, status);
            }
            break;
        }
        case SIGTERM: {  // appswapn killed, use kill without parameter
            OH_HashMapTraverse(g_appSpawnContent->appMap, KillProcess, NULL);
            LE_StopLoop(LE_GetDefaultLoop());
            break;
        }
        default:
            APPSPAWN_LOGI("SigHandler, unsupported signal %{public}d.", siginfo->ssi_signo);
            break;
    }
}

APPSPAWN_STATIC void SignalHandlerNweb(const struct signalfd_siginfo *siginfo)
{
    APPSPAWN_LOGI("SignalHandler signum %{public}d", siginfo->ssi_signo);
    switch (siginfo->ssi_signo) {
        case SIGCHLD: {  // delete pid from app map
            pid_t pid;
            int status;
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                HandleDiedPidNweb(pid, siginfo->ssi_uid, status);
            }
            break;
        }
        case SIGTERM: {  // appswapn killed, use kill without parameter
            OH_HashMapTraverse(g_appSpawnContent->appMap, KillProcess, NULL);
            LE_StopLoop(LE_GetDefaultLoop());
            break;
        }
        default:
            APPSPAWN_LOGI("SigHandler, unsupported signal %{public}d.", siginfo->ssi_signo);
            break;
    }
}

static void HandleSpecial(AppSpawnClientExt *appProperty)
{
    // special handle bundle name medialibrary and scanner
    const char *specialBundleNames[] = {
        "com.ohos.medialibrary.medialibrarydata"
    };

    for (size_t i = 0; i < sizeof(specialBundleNames) / sizeof(specialBundleNames[0]); i++) {
        if (strcmp(appProperty->property.bundleName, specialBundleNames[i]) == 0) {
            if (appProperty->property.gidCount < APP_MAX_GIDS) {
                appProperty->property.gidTable[appProperty->property.gidCount++] = GID_USER_DATA_RW;
                appProperty->property.gidTable[appProperty->property.gidCount++] = GID_FILE_ACCESS;
            }
            break;
        }
    }
}

static int WaitChild(int fd, int pid, const AppSpawnClientExt *appProperty)
{
    int result = 0;
    fd_set rd;
    struct timeval tv;
    FD_ZERO(&rd);
    FD_SET(fd, &rd);
    tv.tv_sec = TV_SEC;
    tv.tv_usec = 0;

    int ret = select(fd + 1, &rd, NULL, NULL, &tv);
    if (ret == 0) {  // timeout
        APPSPAWN_LOGI("Time out for child %{public}s %{public}d fd %{public}d",
            appProperty->property.processName, pid, fd);
        result = 0;
    } else if (ret == -1) {
        APPSPAWN_LOGI("Error for child %{public}s %{public}d", appProperty->property.processName, pid);
        result = 0;
    } else {
        (void)read(fd, &result, sizeof(result));
    }

    return result;
}

static void CheckColdAppEnabled(AppSpawnClientExt *appProperty)
{
    if ((appProperty->property.flags & 0x01) != 0) {
        char cold[10] = {0};  // 10 cold
        (void)GetParameter("startup.appspawn.cold.boot", "0", cold, sizeof(cold));
        APPSPAWN_LOGV("appspawn.cold.boot %{public}s", cold);
        if (strcmp(cold, "1") == 0) {
            appProperty->client.flags |= APP_COLD_START;
        }
    }
}

static bool ReceiveRequestDataToExtraInfo(const TaskHandle taskHandle, AppSpawnClientExt *client,
    const uint8_t *buffer, uint32_t buffLen)
{
    if (client->property.extraInfo.totalLength) {
        ExtraInfo *extraInfo = &client->property.extraInfo;
        if (extraInfo->savedLength == 0) {
            extraInfo->data = (char *)malloc(extraInfo->totalLength);
            APPSPAWN_CHECK(extraInfo->data != NULL, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
                return false, "ReceiveRequestData: malloc extraInfo failed %{public}u", extraInfo->totalLength);
        }

        uint32_t saved = extraInfo->savedLength;
        uint32_t total = extraInfo->totalLength;
        char *data = extraInfo->data;

        APPSPAWN_LOGI("Receiving extraInfo: (%{public}u saved + %{public}u incoming) / %{public}u total",
            saved, buffLen, total);

        APPSPAWN_CHECK((total - saved) >= buffLen, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "ReceiveRequestData: too many data for extraInfo %{public}u ", buffLen);

        int ret = memcpy_s(data + saved, buffLen, buffer, buffLen);
        APPSPAWN_CHECK(ret == 0, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "ReceiveRequestData: memcpy extraInfo failed");

        extraInfo->savedLength += buffLen;
        if (extraInfo->savedLength < extraInfo->totalLength) {
            return false;
        }
        extraInfo->data[extraInfo->totalLength - 1] = 0;
        return true;
    }
    return true;
}

static int CheckRequestMsgValid(AppSpawnClientExt *client)
{
    if (client->property.extraInfo.totalLength >= EXTRAINFO_TOTAL_LENGTH_MAX) {
         APPSPAWN_LOGE("extrainfo total length invalid,len: %{public}d", client->property.extraInfo.totalLength);
         return -1;
    }    
    for (int i = 0; i < APP_LEN_PROC_NAME; i++) {
        if (client->property.processName[i] == '\0') {
            return 0;
        }
    }

    APPSPAWN_LOGE("processname invalid");
    return -1;
}

APPSPAWN_STATIC bool ReceiveRequestData(const TaskHandle taskHandle, AppSpawnClientExt *client,
    const uint8_t *buffer, uint32_t buffLen)
{
    APPSPAWN_CHECK(buffer != NULL && buffLen > 0, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
        return false, "ReceiveRequestData: Invalid buff, bufferLen:%{public}d", buffLen);

    // 1. receive AppParamter
    if (client->property.extraInfo.totalLength == 0) {
        APPSPAWN_CHECK(buffLen >= sizeof(client->property), LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "ReceiveRequestData: Invalid buffLen %{public}u", buffLen);

        int ret = memcpy_s(&client->property, sizeof(client->property), buffer, sizeof(client->property));
        APPSPAWN_CHECK(ret == 0, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "ReceiveRequestData: memcpy failed %{public}d:%{public}u", ret, buffLen);

        // reset extraInfo
        client->property.extraInfo.savedLength = 0;
        client->property.extraInfo.data = NULL;

        // update buffer
        buffer += sizeof(client->property);
        buffLen -= sizeof(client->property);
        ret = CheckRequestMsgValid(client);
        APPSPAWN_CHECK(ret == 0, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "Invalid request msg");
    }

    // 2. check whether extraInfo exist
    if (client->property.extraInfo.totalLength == 0) { // no extraInfo
        APPSPAWN_LOGV("ReceiveRequestData: no extraInfo");
        return true;
    } else if (buffLen == 0) {
        APPSPAWN_LOGV("ReceiveRequestData: waiting for extraInfo");
        return false;
    }
    return ReceiveRequestDataToExtraInfo(taskHandle, client, buffer, buffLen);
}

static int HandleMessage(AppSpawnClientExt *appProperty)
{
    // create pipe
    if (pipe(appProperty->fd) == -1) {
        APPSPAWN_LOGE("create pipe fail, errno = %{public}d", errno);
        return -1;
    }
    fcntl(appProperty->fd[0], F_SETFL, O_NONBLOCK);
    /* Clone support only one parameter, so need to package application parameters */
    AppSandboxArg sandboxArg = { 0 };
    sandboxArg.content = &g_appSpawnContent->content;
    sandboxArg.client = &appProperty->client;
    sandboxArg.client->cloneFlags = sandboxArg.content->sandboxNsFlags;

    SHOW_CLIENT("Receive client message ", appProperty);
    int result = AppSpawnProcessMsg(&sandboxArg, &appProperty->pid);
    if (result == 0) {  // wait child process result
        result = WaitChild(appProperty->fd[0], appProperty->pid, appProperty);
    }
    close(appProperty->fd[0]);
    close(appProperty->fd[1]);
    APPSPAWN_LOGI("child process %{public}s %{public}s pid %{public}d",
        appProperty->property.processName, (result == 0) ? "success" : "fail", appProperty->pid);
    if (result == 0) {
        AddAppInfo(appProperty->pid, appProperty->property.processName);
        SendResponse(appProperty, (char *)&appProperty->pid, sizeof(appProperty->pid));
    } else {
        SendResponse(appProperty, (char *)&result, sizeof(result));
    }
    return 0;
}

static void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)LE_GetUserData(taskHandle);
    APPSPAWN_CHECK(appProperty != NULL, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
        return, "alloc client Failed");

    if (!ReceiveRequestData(taskHandle, appProperty, buffer, buffLen)) {
        return;
    }

    if (g_appSpawnContent->content.isNweb) {
        // get render process termination status, only nwebspawn need this logic.
        if (appProperty->property.code == GET_RENDER_TERMINATION_STATUS) {
            int ret = GetProcessTerminationStatus(&appProperty->client);
            RemoveAppInfo(appProperty->property.pid);
            SendResponse(appProperty, (char *)&ret, sizeof(ret));
            return;
        }
    }
    APPSPAWN_CHECK(appProperty->property.gidCount <= APP_MAX_GIDS && strlen(appProperty->property.processName) > 0,
        LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
        return, "Invalid property %{public}u", appProperty->property.gidCount);

    // special handle bundle name medialibrary and scanner
    HandleSpecial(appProperty);
    if (g_appSpawnContent->timer != NULL) {
        LE_StopTimer(LE_GetDefaultLoop(), g_appSpawnContent->timer);
        g_appSpawnContent->timer = NULL;
    }
    appProperty->pid = 0;
    CheckColdAppEnabled(appProperty);
    int ret = HandleMessage(appProperty);
    if (ret != 0) {
        LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
    }
}

APPSPAWN_STATIC TaskHandle AcceptClient(const LoopHandle loopHandle, const TaskHandle server, uint32_t flags)
{
    static uint32_t clientId = 0;
    TaskHandle stream;
    LE_StreamInfo info = {};
    info.baseInfo.flags = TASK_STREAM | TASK_PIPE | TASK_CONNECT;
    info.baseInfo.flags |= flags;
    info.baseInfo.close = OnClose;
    info.baseInfo.userDataSize = sizeof(AppSpawnClientExt);
    info.disConnectComplete = NULL;
    info.sendMessageComplete = SendMessageComplete;
    info.recvMessage = OnReceiveRequest;

    LE_STATUS ret = LE_AcceptStreamClient(loopHandle, server, &stream, &info);
    APPSPAWN_CHECK(ret == 0, return NULL, "Failed to alloc stream");
    AppSpawnClientExt *client = (AppSpawnClientExt *)LE_GetUserData(stream);
    APPSPAWN_CHECK(client != NULL, return NULL, "Failed to alloc stream");
    struct ucred cred = {-1, -1, -1};
    socklen_t credSize  = sizeof(struct ucred);
    if ((getsockopt(LE_GetSocketFd(stream), SOL_SOCKET, SO_PEERCRED, &cred, &credSize) < 0) ||
        (cred.uid != DecodeUid("foundation")  && cred.uid != DecodeUid("root"))) {
        APPSPAWN_LOGE("Failed to check uid %{public}d", cred.uid);
        LE_CloseStreamTask(LE_GetDefaultLoop(), stream);
        return NULL;
    }

    client->stream = stream;
    client->client.id = ++clientId;
    client->client.flags = 0;
    client->property.extraInfo.totalLength = 0;
    client->property.extraInfo.savedLength = 0;
    client->property.extraInfo.data = NULL;
    APPSPAWN_LOGI("OnConnection client fd %{public}d Id %{public}d", LE_GetSocketFd(stream), client->client.id);
    return stream;
}

static int OnConnection(const LoopHandle loopHandle, const TaskHandle server)
{
    APPSPAWN_CHECK(server != NULL && loopHandle != NULL, return -1, "Error server");
    (void)AcceptClient(loopHandle, server, 0);
    return 0;
}

static void NotifyResToParent(struct AppSpawnContent_ *content, AppSpawnClient *client, int result)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    int fd = appProperty->fd[1];
    APPSPAWN_LOGI("NotifyResToParent %{public}s fd %{public}d result %{public}d",
        appProperty->property.processName, fd, result);
    write(appProperty->fd[1], &result, sizeof(result));
    // close write
    close(fd);
}

static int AppSpawnInit(AppSpawnContent *content)
{
    AppSpawnContentExt *appSpawnContent = (AppSpawnContentExt *)content;
    APPSPAWN_CHECK(appSpawnContent != NULL, return -1, "Invalid appspawn content");

    APPSPAWN_LOGI("AppSpawnInit");
    if (content->loadExtendLib) {
        content->loadExtendLib(content);
    }

    content->notifyResToParent = NotifyResToParent;
    // set private function
    SetContentFunction(content);

    // set uid gid filetr
    if (content->setUidGidFilter) {
        content->setUidGidFilter(content);
    }

    // load app sandbox config
    LoadAppSandboxConfig(content);

    // enable pid namespace
    if (content->enablePidNs) {
        return content->enablePidNs(content);
    }
    return 0;
}

void AppSpawnColdRun(AppSpawnContent *content, int argc, char *const argv[])
{
    AppSpawnContentExt *appSpawnContent = (AppSpawnContentExt *)content;
    APPSPAWN_CHECK(appSpawnContent != NULL, return, "Invalid appspawn content");

    AppSpawnClientExt *client = (AppSpawnClientExt *)malloc(sizeof(AppSpawnClientExt));
    APPSPAWN_CHECK(client != NULL, return, "Failed to alloc memory for client");
    (void)memset_s(client, sizeof(AppSpawnClientExt), 0, sizeof(AppSpawnClientExt));
    int ret = GetAppSpawnClientFromArg(argc, argv, client);
    APPSPAWN_CHECK(ret == 0, free(client);
        return, "Failed to get client from arg");
    client->client.flags &= ~ APP_COLD_START;
    SHOW_CLIENT("Cold running", client);
    ret = DoStartApp(content, &client->client, content->longProcName, content->longProcNameLen);
    if (ret == 0 && content->runChildProcessor != NULL) {
        content->runChildProcessor(content, &client->client);
    }

    APPSPAWN_LOGI("App exit %{public}d.", getpid());
    free(client);
    free(appSpawnContent);
    g_appSpawnContent = NULL;
}

static void AppSpawnRun(AppSpawnContent *content, int argc, char *const argv[])
{
    APPSPAWN_LOGI("AppSpawnRun");
    AppSpawnContentExt *appSpawnContent = (AppSpawnContentExt *)content;
    APPSPAWN_CHECK(appSpawnContent != NULL, return, "Invalid appspawn content");

    LE_STATUS status;

    if (content->isNweb) {
        status = LE_CreateSignalTask(LE_GetDefaultLoop(), &appSpawnContent->sigHandler, SignalHandlerNweb);
    } else {
        status = LE_CreateSignalTask(LE_GetDefaultLoop(), &appSpawnContent->sigHandler, SignalHandler);
    }

    if (status == 0) {
        (void)LE_AddSignal(LE_GetDefaultLoop(), appSpawnContent->sigHandler, SIGCHLD);
        (void)LE_AddSignal(LE_GetDefaultLoop(), appSpawnContent->sigHandler, SIGTERM);
    }

    LE_RunLoop(LE_GetDefaultLoop());
    APPSPAWN_LOGI("AppSpawnRun exit ");
    if (appSpawnContent->timer != NULL) {
        LE_StopTimer(LE_GetDefaultLoop(), appSpawnContent->timer);
        appSpawnContent->timer = NULL;
    }
    LE_CloseSignalTask(LE_GetDefaultLoop(), appSpawnContent->sigHandler);
    // release resource
    OH_HashMapDestory(appSpawnContent->appMap, NULL);
    LE_CloseStreamTask(LE_GetDefaultLoop(), appSpawnContent->server);
    LE_CloseLoop(LE_GetDefaultLoop());
    free(content);
    g_appSpawnContent = NULL;
}

static int CreateHashForApp(AppSpawnContentExt *appSpawnContent)
{
    HashInfo hashInfo = {
        AppInfoHashNodeCompare,
        TestHashKeyCompare,
        AppInfoHashNodeFunction,
        AppInfoHashKeyFunction,
        AppInfoHashNodeFree,
        APP_HASH_BUTT
    };
    int ret = OH_HashMapCreate(&appSpawnContent->appMap, &hashInfo);
    APPSPAWN_CHECK(ret == 0, free(appSpawnContent); return -1, "Failed to create hash for app");
    return 0;
}

static int CreateAppSpawnServer(AppSpawnContentExt *appSpawnContent, const char *socketName)
{
    char path[128] = {0};  // 128 max path
    int ret = snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s%s", SOCKET_DIR, socketName);
    APPSPAWN_CHECK(ret >= 0, return -1, "Failed to snprintf_s %{public}d", ret);
    int socketId = GetControlSocket(socketName);
    APPSPAWN_LOGI("get socket form env %{public}s socketId %{public}d", socketName, socketId);
    APPSPAWN_CHECK_ONLY_EXPER(socketId <= 0, appSpawnContent->flags |= FLAGS_ON_DEMAND);

    LE_StreamServerInfo info = {};
    info.baseInfo.flags = TASK_STREAM | TASK_PIPE | TASK_SERVER;
    info.socketId = socketId;
    info.server = path;
    info.baseInfo.close = NULL;
    info.incommingConnect = OnConnection;

    ret = LE_CreateStreamServer(LE_GetDefaultLoop(), &appSpawnContent->server, &info);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to create socket for %{public}s", path);
    // create socket

    APPSPAWN_CHECK(ret == 0, return -1, "Failed to lchown %{public}s, err %{public}d. ", path, errno);
    APPSPAWN_LOGI("CreateAppSpawnServer path %{public}s fd %{public}d",
        path, LE_GetSocketFd(appSpawnContent->server));
    return 0;
}

AppSpawnContent *AppSpawnCreateContent(const char *socketName, char *longProcName, uint32_t longProcNameLen, int mode)
{
    APPSPAWN_CHECK(LE_GetDefaultLoop() != NULL, return NULL, "Invalid default loop");
    APPSPAWN_CHECK(socketName != NULL && longProcName != NULL, return NULL, "Invalid name");
    APPSPAWN_LOGI("AppSpawnCreateContent %{public}s %{public}u mode %{public}d", socketName, longProcNameLen, mode);

    AppSpawnContentExt *appSpawnContent = (AppSpawnContentExt *)malloc(sizeof(AppSpawnContentExt));
    APPSPAWN_CHECK(appSpawnContent != NULL, return NULL, "Failed to alloc memory for appspawn");
    (void)memset_s(&appSpawnContent->content, sizeof(appSpawnContent->content), 0, sizeof(appSpawnContent->content));
    appSpawnContent->content.longProcName = longProcName;
    appSpawnContent->content.longProcNameLen = longProcNameLen;
    if (strcmp(longProcName, NWEBSPAWN_SERVER_NAME) == 0) {
        appSpawnContent->content.isNweb = true;
    } else {
        appSpawnContent->content.isNweb = false;
    }
    appSpawnContent->timer = NULL;
    appSpawnContent->flags = 0;
    appSpawnContent->server = NULL;
    appSpawnContent->sigHandler = NULL;
    appSpawnContent->content.initAppSpawn = AppSpawnInit;

    if (mode) {
        appSpawnContent->flags |= FLAGS_MODE_COLD;
        appSpawnContent->content.runAppSpawn = AppSpawnColdRun;
    } else {
        appSpawnContent->content.runAppSpawn = AppSpawnRun;

        // create hash for app
        int ret = CreateHashForApp(appSpawnContent);
        APPSPAWN_CHECK(ret == 0, free(appSpawnContent); return NULL, "Failed to create app");
        ret = CreateAppSpawnServer(appSpawnContent, socketName);
        APPSPAWN_CHECK(ret == 0, free(appSpawnContent); return NULL, "Failed to create server");
    }
    g_appSpawnContent = appSpawnContent;
    return &g_appSpawnContent->content;
}
