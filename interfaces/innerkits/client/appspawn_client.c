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

#include "appspawn_client.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <linux/in.h>
#include <linux/socket.h>
#include <linux/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>

#include "appspawn_mount_permission.h"
#include "appspawn_hook.h"
#include "appspawn_utils.h"
#include "parameter.h"
#include "securec.h"
#include "appspawndf_utils.h"

#define USER_LOCK_STATUS_SIZE 8
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static AppSpawnReqMsgMgr *g_clientInstance[CLIENT_MAX] = {NULL};
static pthread_mutex_t g_spawnListenMutex = PTHREAD_MUTEX_INITIALIZER;
static int g_spawnListenFd = 0;
static bool g_spawnListenStart = false;
static bool g_spawndfListenStart = false;
static pthread_mutex_t g_nativeSpawnListenMutex = PTHREAD_MUTEX_INITIALIZER;
static int g_nativeSpawnListenFd = 0;
static bool g_nativeSpawnListenStart = false;
static pthread_mutex_t g_hybridSpawnListenMutex = PTHREAD_MUTEX_INITIALIZER;
static int g_hybridSpawnListenFd = 0;
static bool g_hybridSpawnListenStart = false;

APPSPAWN_STATIC void SpawnListen(AppSpawnReqMsgMgr *reqMgr, const char *processName);

static int InitClientInstance(AppSpawnClientType type)
{
    pthread_mutex_lock(&g_mutex);
    if (g_clientInstance[type] != NULL) {
        pthread_mutex_unlock(&g_mutex);
        return 0;
    }
    AppSpawnReqMsgMgr *clientInstance = malloc(sizeof(AppSpawnReqMsgMgr) + RECV_BLOCK_LEN);
    if (clientInstance == NULL) {
        pthread_mutex_unlock(&g_mutex);
        return APPSPAWN_SYSTEM_ERROR;
    }
    // init
    clientInstance->type = type;
    clientInstance->msgNextId = 1;
    // Since cjappspawn is non-resident, set client timeout larger than default.
    if (type == CLIENT_FOR_CJAPPSPAWN) {
        clientInstance->timeout = CJAPPSPAWN_CLIENT_TIMEOUT;
    } else {
        clientInstance->timeout = GetSpawnTimeout(TIMEOUT_DEF, false);
    }
    clientInstance->maxRetryCount = MAX_RETRY_SEND_COUNT;
    clientInstance->socketId = -1;
    pthread_mutex_init(&clientInstance->mutex, NULL);
    // init recvBlock
    OH_ListInit(&clientInstance->recvBlock.node);
    clientInstance->recvBlock.blockSize = RECV_BLOCK_LEN;
    clientInstance->recvBlock.currentIndex = 0;
    g_clientInstance[type] = clientInstance;
    pthread_mutex_unlock(&g_mutex);
    (void)AppSpawndfIsServiceEnabled(AppSpawnClientInit); // preload for dflist
    return 0;
}

APPSPAWN_STATIC void CloseClientSocket(int socketId)
{
    APPSPAWN_LOGV("Closed socket with fd %{public}d", socketId);
    if (socketId >= 0) {
        int flag = 0;
        setsockopt(socketId, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
        close(socketId);
    }
}

APPSPAWN_STATIC const char *GetSocketName(uint32_t type)
{
    const char *socketName = NULL;
    switch (type) {
        case CLIENT_FOR_APPSPAWN:
            socketName = APPSPAWN_SOCKET_NAME;
            break;
        case CLIENT_FOR_CJAPPSPAWN:
            socketName = CJAPPSPAWN_SOCKET_NAME;
            break;
        case CLIENT_FOR_NATIVESPAWN:
            socketName = NATIVESPAWN_SOCKET_NAME;
            break;
        case CLIENT_FOR_HYBRIDSPAWN:
            socketName = HYBRIDSPAWN_SOCKET_NAME;
            break;
        case CLIENT_FOR_APPSPAWNDF:
            socketName = APPSPAWNDF_SOCKET_NAME;
            break;
        default:
            socketName = NWEBSPAWN_SOCKET_NAME;
            break;
    }
    return socketName;
}

APPSPAWN_STATIC int CreateClientSocket(uint32_t type, uint32_t timeout)
{
    const char *socketName = GetSocketName(type);
    int socketFd = socket(AF_UNIX, SOCK_STREAM, 0);  // SOCK_SEQPACKET
    APPSPAWN_CHECK(socketFd >= 0, return -1,
        "Socket socket fd: %{public}s error: %{public}d", socketName, errno);
    int ret = 0;
    do {
        int flag = 1;
        ret = setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
        flag = 1;
        ret = setsockopt(socketFd, SOL_SOCKET, SO_PASSCRED, &flag, sizeof(flag));
        APPSPAWN_CHECK(ret == 0, break, "Set opt SO_PASSCRED name: %{public}s error: %{public}d", socketName, errno);

        struct timeval timeoutVal = {timeout, 0};
        ret = setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, &timeoutVal, sizeof(timeoutVal));
        APPSPAWN_CHECK(ret == 0, break, "Set opt SO_SNDTIMEO name: %{public}s error: %{public}d", socketName, errno);
        ret = setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeoutVal, sizeof(timeoutVal));
        APPSPAWN_CHECK(ret == 0, break, "Set opt SO_RCVTIMEO name: %{public}s error: %{public}d", socketName, errno);

        ret = APPSPAWN_SYSTEM_ERROR;
        struct sockaddr_un addr;
        socklen_t pathSize = sizeof(addr.sun_path);
        int pathLen = snprintf_s(addr.sun_path, pathSize, (pathSize - 1), "%s%s", APPSPAWN_SOCKET_DIR, socketName);
        APPSPAWN_CHECK(pathLen > 0, break, "Format path %{public}s error: %{public}d", socketName, errno);
        addr.sun_family = AF_LOCAL;
        socklen_t socketAddrLen = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + (socklen_t)pathLen + 1);
        ret = connect(socketFd, (struct sockaddr *)(&addr), socketAddrLen);
        APPSPAWN_CHECK(ret == 0, break,
            "Failed to connect %{public}s error: %{public}d", addr.sun_path, errno);
        APPSPAWN_LOGI("Create socket success %{public}s socketFd: %{public}d", addr.sun_path, socketFd);
        return socketFd;
    } while (0);
    CloseClientSocket(socketFd);
    return -1;
}

APPSPAWN_STATIC int UpdateSocketTimeout(uint32_t timeout, int socketFd)
{
    struct timeval timeoutVal = {timeout, 0};
    int ret = setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, &timeoutVal, sizeof(timeoutVal));
    APPSPAWN_CHECK(ret == 0, return ret, "Set opt SO_SNDTIMEO error: %{public}d", errno);
    ret = setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &timeoutVal, sizeof(timeoutVal));
    APPSPAWN_CHECK(ret == 0, return ret, "Set opt SO_RCVTIMEO error: %{public}d", errno);
    return ret;
}

static int ReadMessage(int socketFd, uint8_t *buf, int len, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result)
{
    struct timespec readStart = { 0 };
    clock_gettime(CLOCK_MONOTONIC, &readStart);
    ssize_t rLen = TEMP_FAILURE_RETRY(read(socketFd, buf, len));
    if (rLen == -1 && errno == EAGAIN) {
        struct timespec readEnd = { 0 };
        clock_gettime(CLOCK_MONOTONIC, &readEnd);
        uint64_t diff = DiffTime(&readStart, &readEnd);
        uint64_t timeout = reqNode->isAsan ? COLDRUN_READ_RETRY_TIME : NORMAL_READ_RETRY_TIME;
        // If difftime is greater than timeout, it is considered that system hibernation or a time jump has occurred
        if (diff > timeout) {
            APPSPAWN_LOGW("Read message again from fd %{public}d, difftime %{public}" PRId64 " us", socketFd, diff);
            rLen = TEMP_FAILURE_RETRY(read(socketFd, buf, len));
        }
    }
    APPSPAWN_CHECK(rLen >= 0, return APPSPAWN_TIMEOUT,
        "Read message from fd %{public}d rLen %{public}zd errno: %{public}d", socketFd, rLen, errno);
    if ((size_t)rLen >= sizeof(AppSpawnResponseMsg)) {
        AppSpawnResponseMsg *msg = (AppSpawnResponseMsg *)(buf);
        APPSPAWN_CHECK_ONLY_LOG(reqNode->msg->msgId == msg->msgHdr.msgId,
            "Invalid msg recvd %{public}u %{public}u", reqNode->msg->msgId, msg->msgHdr.msgId);
        return memcpy_s(result, sizeof(AppSpawnResult), &msg->result, sizeof(msg->result));
    }
    return APPSPAWN_TIMEOUT;
}

static int WriteMessage(int socketFd, const uint8_t *buf, ssize_t len, int *fds, int *fdCount)
{
    ssize_t written = 0;
    ssize_t remain = len;
    const uint8_t *offset = buf;
    struct iovec iov = {
        .iov_base = (void *) offset,
        .iov_len = len,
    };
    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };
    char *ctrlBuffer = NULL;
    if (fdCount != NULL && fds != NULL && *fdCount > 0) {
        msg.msg_controllen = CMSG_SPACE(*fdCount * sizeof(int));
        ctrlBuffer = (char *) malloc(msg.msg_controllen);
        APPSPAWN_CHECK(ctrlBuffer != NULL, return -1,
            "WriteMessage fail to alloc memory for msg_control %{public}d %{public}d", msg.msg_controllen, errno);
        msg.msg_control = ctrlBuffer;
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        APPSPAWN_CHECK(cmsg != NULL, free(ctrlBuffer);
            return -1, "WriteMessage fail to get CMSG_FIRSTHDR %{public}d", errno);
        cmsg->cmsg_len = CMSG_LEN(*fdCount * sizeof(int));
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_level = SOL_SOCKET;
        int ret = memcpy_s(CMSG_DATA(cmsg), cmsg->cmsg_len, fds, *fdCount * sizeof(int));
        APPSPAWN_CHECK(ret == 0, free(ctrlBuffer);
            return -1, "WriteMessage fail to memcpy_s fd %{public}d", errno);
        APPSPAWN_LOGV("build fd info count %{public}d", *fdCount);
    }
    for (ssize_t wLen = 0; remain > 0; offset += wLen, remain -= wLen, written += wLen) {
        errno = 0;
        wLen = sendmsg(socketFd, &msg, MSG_NOSIGNAL);
        APPSPAWN_LOGV("Write msg errno: %{public}d %{public}zd", errno, wLen);
        APPSPAWN_CHECK((wLen > 0) || (errno == EINTR), free(ctrlBuffer);
            return -errno,
            "Failed to write message to fd %{public}d, wLen %{public}zd errno: %{public}d", socketFd, wLen, errno);
    }
    free(ctrlBuffer);
    return written == len ? 0 : -EFAULT;
}

static int HandleMsgSend(AppSpawnReqMsgMgr *reqMgr, int socketId, AppSpawnReqMsgNode *reqNode)
{
    APPSPAWN_LOGV("HandleMsgSend reqId: %{public}u msgId: %{public}d", reqNode->reqId, reqNode->msg->msgId);
    ListNode *sendNode = reqNode->msgBlocks.next;
    uint32_t currentIndex = 0;
    bool sendFd = true;
    while (sendNode != NULL && sendNode != &reqNode->msgBlocks) {
        AppSpawnMsgBlock *sendBlock = (AppSpawnMsgBlock *)ListEntry(sendNode, AppSpawnMsgBlock, node);
        int ret = WriteMessage(socketId, sendBlock->buffer, sendBlock->currentIndex,
            sendFd ? reqNode->fds : NULL,
            sendFd ? &reqNode->fdCount : NULL);
        currentIndex += sendBlock->currentIndex;
        APPSPAWN_LOGV("Write msg ret: %{public}d msgId: %{public}u %{public}u %{public}u",
            ret, reqNode->msg->msgId, reqNode->msg->msgLen, currentIndex);
        if (ret == 0) {
            sendFd = false;
            sendNode = sendNode->next;
            continue;
        }
        APPSPAWN_LOGE("Send msg fail reqId: %{public}u msgId: %{public}d ret: %{public}d",
            reqNode->reqId, reqNode->msg->msgId, ret);
        return ret;
    }
    return 0;
}

APPSPAWN_STATIC inline void SleepRetryTime(bool needSleep)
{
    if (needSleep) {
        usleep(RETRY_TIME);
    }
}

APPSPAWN_STATIC void TryCreateSocket(AppSpawnReqMsgMgr *reqMgr, bool needSleep)
{
    uint32_t retryCount = 1;
    while (retryCount <= reqMgr->maxRetryCount) {
        if (reqMgr->socketId < 0) {
            reqMgr->socketId = CreateClientSocket(reqMgr->type, reqMgr->timeout);
        }
        if (reqMgr->socketId < 0) {
            APPSPAWN_LOGV("Failed to create socket, try again");
            SleepRetryTime(needSleep);
            retryCount++;
            continue;
        }
        break;
    }
}

static void SendSpawnListenMsg(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode)
{
    if (reqMgr->type == CLIENT_FOR_APPSPAWN && reqNode->msg->msgType != MSG_OBSERVE_PROCESS_SIGNAL_STATUS) {
        pthread_mutex_lock(&g_spawnListenMutex);
        SpawnListen(reqMgr, reqNode->msg->processName);
        pthread_mutex_unlock(&g_spawnListenMutex);
    }

    if (reqMgr->type == CLIENT_FOR_APPSPAWNDF && reqNode->msg->msgType != MSG_OBSERVE_PROCESS_SIGNAL_STATUS) {
        pthread_mutex_lock(&g_spawnListenMutex);
        SpawnListen(reqMgr, reqNode->msg->processName);
        pthread_mutex_unlock(&g_spawnListenMutex);
    }

    if (reqMgr->type == CLIENT_FOR_NATIVESPAWN && reqNode->msg->msgType != MSG_OBSERVE_PROCESS_SIGNAL_STATUS) {
        pthread_mutex_lock(&g_nativeSpawnListenMutex);
        SpawnListen(reqMgr, reqNode->msg->processName);
        pthread_mutex_unlock(&g_nativeSpawnListenMutex);
    }

    if (reqMgr->type == CLIENT_FOR_HYBRIDSPAWN && reqNode->msg->msgType != MSG_OBSERVE_PROCESS_SIGNAL_STATUS) {
        pthread_mutex_lock(&g_hybridSpawnListenMutex);
        SpawnListen(reqMgr, reqNode->msg->processName);
        pthread_mutex_unlock(&g_hybridSpawnListenMutex);
    }
}

static int ClientSendMsg(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result,
    bool needSleep)
{
    uint32_t retryCount = 1;
    int isColdRun = reqNode->isAsan;
    while (retryCount <= reqMgr->maxRetryCount) {
        if (reqMgr->socketId < 0) { // try create socket
            TryCreateSocket(reqMgr, needSleep);
            if (reqMgr->socketId < 0) {
                SleepRetryTime(needSleep);
                retryCount++;
                continue;
            }
        }
        SendSpawnListenMsg(reqMgr, reqNode);

        if (isColdRun && reqMgr->timeout < ASAN_TIMEOUT) {
            UpdateSocketTimeout(ASAN_TIMEOUT, reqMgr->socketId);
        }

        if (reqNode->msg->msgId == 0) {
            reqNode->msg->msgId = reqMgr->msgNextId++;
        }
        int ret = HandleMsgSend(reqMgr, reqMgr->socketId, reqNode);
        if (ret == 0) {
            ret = ReadMessage(reqMgr->socketId, reqMgr->recvBlock.buffer, reqMgr->recvBlock.blockSize, reqNode, result);
        }
        if (ret == 0) {
            if (isColdRun && reqMgr->timeout < ASAN_TIMEOUT) {
                UpdateSocketTimeout(reqMgr->timeout, reqMgr->socketId);
            }
            return 0;
        }
        // retry
        CloseClientSocket(reqMgr->socketId);
        reqMgr->socketId = -1;
        reqMgr->msgNextId = 1;
        reqNode->msg->msgId = 0;
        usleep(RETRY_TIME);
        retryCount++;
    }
    return APPSPAWN_TIMEOUT;
}

APPSPAWN_STATIC int SpawnListenBase(AppSpawnReqMsgMgr *reqMgr, const char *processName, int fd, bool startFlag)
{
    if (fd <= 0 || startFlag) {
        APPSPAWN_LOGV("Spawn Listen fail, fd:%{public}d, startFlag:%{public}d", fd, startFlag);
        return -1;
    }
    AppSpawnClientType type = reqMgr->type;
    APPSPAWN_LOGI("Spawn Listen start type:%{public}d, fd:%{public}d", type, fd);

    AppSpawnReqMsgHandle reqHandle;
    int ret = AppSpawnReqMsgCreate(MSG_OBSERVE_PROCESS_SIGNAL_STATUS, processName, &reqHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to create type:%{public}d req msg, ret %{public}d", type, ret);

    ret = AppSpawnReqMsgAddFd(reqHandle, SPAWN_LISTEN_FD_NAME, fd);
    APPSPAWN_CHECK(ret == 0, AppSpawnReqMsgFree(reqHandle);
        return ret, "Failed to add fd info to msg, ret %{public}d", ret);

    AppSpawnResult result = {0};
    ret = ClientSendMsg(reqMgr, (AppSpawnReqMsgNode *)reqHandle, &result, true);
    APPSPAWN_CHECK(ret == 0, return ret, "Send msg to type:%{public}d fail, ret %{public}d", type, ret);
    APPSPAWN_CHECK(result.result == 0, return result.result,
                   "Spawn failed to handle listen msg, result %{public}d", result.result);

    APPSPAWN_LOGI("Spawn Listen client type:%{public}d send fd:%{public}d success", type, fd);
    return 0;
}

APPSPAWN_STATIC void SpawnListen(AppSpawnReqMsgMgr *reqMgr, const char *processName)
{
    APPSPAWN_CHECK(reqMgr != NULL, return, "Invalid reqMgr");
    APPSPAWN_CHECK(processName != NULL, return, "Invalid process name");

    int ret = 0;
    if (reqMgr->type == CLIENT_FOR_APPSPAWN) {
        ret = SpawnListenBase(reqMgr, processName, g_spawnListenFd, g_spawnListenStart);
        APPSPAWN_ONLY_EXPER(ret == 0, g_spawnListenStart = true);
    }

    if (reqMgr->type == CLIENT_FOR_NATIVESPAWN) {
        ret = SpawnListenBase(reqMgr, processName, g_nativeSpawnListenFd, g_nativeSpawnListenStart);
        APPSPAWN_ONLY_EXPER(ret == 0, g_nativeSpawnListenStart = true);
    }

    if (reqMgr->type == CLIENT_FOR_HYBRIDSPAWN) {
        ret = SpawnListenBase(reqMgr, processName, g_hybridSpawnListenFd, g_hybridSpawnListenStart);
        APPSPAWN_ONLY_EXPER(ret == 0, g_hybridSpawnListenStart = true);
    }

    if (AppSpawndfIsServiceEnabled(AppSpawnClientInit) && reqMgr->type == CLIENT_FOR_APPSPAWNDF) {
        ret = SpawnListenBase(reqMgr, processName, g_spawnListenFd, g_spawndfListenStart);
        APPSPAWN_ONLY_EXPER(ret == 0, g_spawndfListenStart = true);
    }
}

int AppSpawnClientInit(const char *serviceName, AppSpawnClientHandle *handle)
{
    APPSPAWN_CHECK(serviceName != NULL, return APPSPAWN_ARG_INVALID, "Invalid service name");
    APPSPAWN_CHECK(handle != NULL, return APPSPAWN_ARG_INVALID, "Invalid handle for %{public}s", serviceName);
    APPSPAWN_LOGV("AppSpawnClientInit serviceName %{public}s", serviceName);
    AppSpawnClientType type = CLIENT_FOR_APPSPAWN;
    if (strcmp(serviceName, CJAPPSPAWN_SERVER_NAME) == 0) {
        type = CLIENT_FOR_CJAPPSPAWN;
    } else if (strcmp(serviceName, NWEBSPAWN_SERVER_NAME) == 0 || strstr(serviceName, NWEBSPAWN_SOCKET_NAME) != NULL) {
        type = CLIENT_FOR_NWEBSPAWN;
    } else if (strcmp(serviceName, NATIVESPAWN_SERVER_NAME) == 0 ||
        strstr(serviceName, NATIVESPAWN_SOCKET_NAME) != NULL) {
        type = CLIENT_FOR_NATIVESPAWN;
    } else if (strcmp(serviceName, HYBRIDSPAWN_SERVER_NAME) == 0) {
        type = CLIENT_FOR_HYBRIDSPAWN;
    } else if (strcmp(serviceName, APPSPAWNDF_SERVER_NAME) == 0) {
        type = CLIENT_FOR_APPSPAWNDF;
    }
    int ret = InitClientInstance(type);
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_SYSTEM_ERROR, "Failed to create reqMgr");
    *handle = (AppSpawnClientHandle)g_clientInstance[type];
    return 0;
}

int AppSpawnClientDestroy(AppSpawnClientHandle handle)
{
    AppSpawnReqMsgMgr *reqMgr = (AppSpawnReqMsgMgr *)handle;
    APPSPAWN_CHECK(reqMgr != NULL, return APPSPAWN_SYSTEM_ERROR, "Invalid reqMgr");
    pthread_mutex_lock(&g_mutex);
    if (reqMgr->type < sizeof(g_clientInstance) / sizeof(g_clientInstance[0])) {
        g_clientInstance[reqMgr->type] = NULL;
    }
    pthread_mutex_unlock(&g_mutex);
    pthread_mutex_destroy(&reqMgr->mutex);
    if (reqMgr->socketId >= 0) {
        CloseClientSocket(reqMgr->socketId);
        reqMgr->socketId = -1;
    }
    free(reqMgr);
    return 0;
}

int ClientSendMsgLocked(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result,
    bool needSleep)
{
    APPSPAWN_CHECK(reqMgr != NULL && reqNode != NULL, return APPSPAWN_ARG_INVALID, "Invalid req context");
    pthread_mutex_lock(&reqMgr->mutex);
    int ret = ClientSendMsg(reqMgr, reqNode, result, needSleep);
    pthread_mutex_unlock(&reqMgr->mutex);
    return ret;
}

int AppSpawnClientSendMsgDfControl(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode,
    AppSpawnResult *result)
{
    uint32_t msgType = reqNode->msg->msgType;
    AppSpawnReqMsgMgr *dfMgr = (AppSpawnReqMsgMgr *)AppSpawndfGetHandle();

    if (msgType == MSG_APP_SPAWN) {
        // The ASAN needs to be optimized
        bool targetDfSpawn = AppSpawndfIsAppInWhiteList(reqNode->msg->processName)
            && !AppSpawndfIsAppEnableGWPASan(reqNode->msg->processName, reqNode->msgFlags);
        if (targetDfSpawn && dfMgr != NULL) {
            int ret = ClientSendMsgLocked(dfMgr, reqNode, result, false);
            if (ret == 0) {
                return 0;
            }
            APPSPAWN_LOGI("Failback: client %{public}s spawn via appspawndf fail, ret=%{public}d",
                reqNode->msg->processName, ret);
        }
        int ret = ClientSendMsgLocked(reqMgr, reqNode, result, true);
        return ret;
    }

    if (AppSpawndfIsBroadcastMsg(msgType)) {
        AppSpawnResult dfRes = {0};
        int ret = ClientSendMsgLocked(reqMgr, reqNode, result, true);
        if (dfMgr != NULL) {
            (void)ClientSendMsgLocked(dfMgr, reqNode, &dfRes, true);
            AppSpawndfMergeBroadcastResult(msgType, result, &dfRes);
        }
        return ret;
    }

    return ClientSendMsgLocked(reqMgr, reqNode, result, true);
}

int AppSpawnClientSendMsg(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle, AppSpawnResult *result)
{
    APPSPAWN_CHECK(result != NULL, AppSpawnReqMsgFree(reqHandle);
        return APPSPAWN_ARG_INVALID, "Invalid result");
    result->result = APPSPAWN_ARG_INVALID;
    result->pid = 0;
    AppSpawnReqMsgMgr *reqMgr = (AppSpawnReqMsgMgr *)handle;
    APPSPAWN_CHECK(reqMgr != NULL, AppSpawnReqMsgFree(reqHandle);
        return APPSPAWN_ARG_INVALID, "Invalid reqMgr");
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK(reqNode != NULL && reqNode->msg != NULL, AppSpawnReqMsgFree(reqHandle);
        return APPSPAWN_ARG_INVALID, "Invalid msgReq");

    APPSPAWN_DUMPI("AppSpawnClientSendMsg reqId:%{public}u msgLen:%{public}u fd:%{public}d %{public}s",
        reqNode->reqId, reqNode->msg->msgLen, reqMgr->socketId, reqNode->msg->processName);

    int ret;
    if (AppSpawndfIsServiceEnabled(AppSpawnClientInit) && reqMgr->type == CLIENT_FOR_APPSPAWN) {
        ret = AppSpawnClientSendMsgDfControl(reqMgr, reqNode, result);
    } else {
        ret = ClientSendMsgLocked(reqMgr, reqNode, result, true);
    }

    if (ret != 0) {
        result->result = ret;
    }
    APPSPAWN_DUMPI("AppSpawnClientSendMsg reqId:%{public}u fd:%{public}d end result:0x%{public}x pid:%{public}d",
        reqNode->reqId, reqMgr->socketId, result->result, result->pid);
    AppSpawnReqMsgFree(reqHandle);
    return ret;
}

int AppSpawnClientSendUserLockStatus(uint32_t userId, bool isLocked)
{
    char lockstatus[USER_LOCK_STATUS_SIZE] = {0};
    int ret = snprintf_s(lockstatus, USER_LOCK_STATUS_SIZE, USER_LOCK_STATUS_SIZE - 1, "%u:%d", userId, isLocked);
    APPSPAWN_CHECK(ret > 0, return ret, "Failed to build lockstatus req msg, ret = %{public}d", ret);
    APPSPAWN_LOGI("Send lockstatus msg to appspawn %{public}s", lockstatus);

    AppSpawnReqMsgHandle reqHandle;
    ret = AppSpawnReqMsgCreate(MSG_LOCK_STATUS, "storage_manager", &reqHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to create appspawn req msg, ret = %{public}d", ret);

    ret = AppSpawnReqMsgAddStringInfo(reqHandle, "lockstatus", lockstatus);
    APPSPAWN_CHECK(ret == 0, AppSpawnReqMsgFree(reqHandle);
        return ret, "Failed to add lockstatus message, ret=%{public}d", ret);

    AppSpawnClientHandle clientHandle;
    ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, AppSpawnReqMsgFree(reqHandle);
        return ret, "Appspawn client failed to init, ret=%{public}d", ret);

    AppSpawnResult result = {0};
    ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    AppSpawnClientDestroy(clientHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Send msg to appspawn failed, ret=%{public}d", ret);

    if (result.result != 0) {
        APPSPAWN_LOGE("Appspawn failed to handle message, result=%{public}d", result.result);
        return result.result;
    }
    APPSPAWN_LOGI("Send lockstatus msg to appspawn success");
    return 0;
}

int SpawnListenFdSet(int fd)
{
    if (fd <= 0) {
        APPSPAWN_LOGE("Spawn Listen fd set[%{public}d] failed", fd);
        return APPSPAWN_ARG_INVALID;
    }
    g_spawnListenFd = fd;
    APPSPAWN_LOGI("Spawn Listen fd set[%{public}d] success", fd);
    return 0;
}

int NativeSpawnListenFdSet(int fd)
{
    if (fd <= 0) {
        APPSPAWN_LOGE("NativeSpawn Listen fd set[%{public}d] failed", fd);
        return APPSPAWN_ARG_INVALID;
    }
    g_nativeSpawnListenFd = fd;
    APPSPAWN_LOGI("NativeSpawn Listen fd set[%{public}d] success", fd);
    return 0;
}

int HybridSpawnListenFdSet(int fd)
{
    if (fd <= 0) {
        APPSPAWN_LOGE("HybridSpawn Listen fd set[%{public}d] failed", fd);
        return APPSPAWN_ARG_INVALID;
    }
    g_hybridSpawnListenFd = fd;
    APPSPAWN_LOGI("HybridSpawn Listen fd set[%{public}d] success", fd);
    return 0;
}

int SpawnListenCloseSet(void)
{
    pthread_mutex_lock(&g_spawnListenMutex);
    g_spawnListenStart = false;
    g_spawndfListenStart = false;
    pthread_mutex_unlock(&g_spawnListenMutex);
    APPSPAWN_LOGI("Spawn Listen close set success");
    return 0;
}

int NativeSpawnListenCloseSet(void)
{
    pthread_mutex_lock(&g_nativeSpawnListenMutex);
    g_nativeSpawnListenStart = false;
    pthread_mutex_unlock(&g_nativeSpawnListenMutex);
    APPSPAWN_LOGI("NativeSpawn Listen close set success");
    return 0;
}

int HybridSpawnListenCloseSet(void)
{
    pthread_mutex_lock(&g_hybridSpawnListenMutex);
    g_hybridSpawnListenStart = false;
    pthread_mutex_unlock(&g_hybridSpawnListenMutex);
    APPSPAWN_LOGI("HybridSpawn Listen close set success");
    return 0;
}
