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

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static AppSpawnReqMsgMgr *g_clientInstance[CLIENT_MAX] = {NULL};

static uint32_t GetDefaultTimeout(uint32_t def)
{
    uint32_t value = def;
    char data[32] = {};  // 32 length
    int ret = GetParameter("persist.appspawn.reqMgr.timeout", "0", data, sizeof(data));
    if (ret > 0 && strcmp(data, "0") != 0) {
        errno = 0;
        value = atoi(data);
        return (errno != 0) ? def : value;
    }
    return value;
}

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
    clientInstance->timeout = GetDefaultTimeout(TIMEOUT_DEF);
    clientInstance->maxRetryCount = MAX_RETRY_SEND_COUNT;
    clientInstance->socketId = -1;
    pthread_mutex_init(&clientInstance->mutex, NULL);
    // init recvBlock
    OH_ListInit(&clientInstance->recvBlock.node);
    clientInstance->recvBlock.blockSize = RECV_BLOCK_LEN;
    clientInstance->recvBlock.currentIndex = 0;
    g_clientInstance[type] = clientInstance;
    pthread_mutex_unlock(&g_mutex);
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

APPSPAWN_STATIC int CreateClientSocket(uint32_t type, uint32_t timeout)
{
    const char *socketName = type == CLIENT_FOR_APPSPAWN ? APPSPAWN_SOCKET_NAME : NWEBSPAWN_SOCKET_NAME;
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
        socklen_t socketAddrLen = offsetof(struct sockaddr_un, sun_path) + pathLen + 1;
        ret = connect(socketFd, (struct sockaddr *)(&addr), socketAddrLen);
        APPSPAWN_CHECK(ret == 0, break,
            "Failed to connect %{public}s error: %{public}d", addr.sun_path, errno);
        APPSPAWN_LOGI("Create socket success %{public}s socketFd: %{public}d", addr.sun_path, socketFd);
        return socketFd;
    } while (0);
    CloseClientSocket(socketFd);
    return -1;
}

static int ReadMessage(int socketFd, uint32_t sendMsgId, uint8_t *buf, int len, AppSpawnResult *result)
{
    ssize_t rLen = TEMP_FAILURE_RETRY(read(socketFd, buf, len));
    APPSPAWN_CHECK(rLen >= 0, return APPSPAWN_TIMEOUT,
        "Read message from fd %{public}d rLen %{public}zd errno: %{public}d", socketFd, rLen, errno);
    if ((size_t)rLen >= sizeof(AppSpawnResponseMsg)) {
        AppSpawnResponseMsg *msg = (AppSpawnResponseMsg *)(buf);
        APPSPAWN_CHECK_ONLY_LOG(sendMsgId == msg->msgHdr.msgId,
            "Invalid msg recvd %{public}u %{public}u", sendMsgId, msg->msgHdr.msgId);
        return memcpy_s(result, sizeof(AppSpawnResult), &msg->result, sizeof(msg->result));
    }
    return APPSPAWN_TIMEOUT;
}

static int WriteMessage(int socketFd, const uint8_t *buf, ssize_t len)
{
    ssize_t written = 0;
    ssize_t remain = len;
    const uint8_t *offset = buf;
    for (ssize_t wLen = 0; remain > 0; offset += wLen, remain -= wLen, written += wLen) {
        wLen = send(socketFd, offset, remain, MSG_NOSIGNAL);
        APPSPAWN_LOGV("Write msg errno: %{public}d %{public}zd", errno, wLen);
        APPSPAWN_CHECK((wLen > 0) || (errno == EINTR), return -errno,
            "Failed to write message to fd %{public}d, wLen %{public}zd errno: %{public}d", socketFd, wLen, errno);
    }
    return written == len ? 0 : -EFAULT;
}

static int HandleMsgSend(AppSpawnReqMsgMgr *reqMgr, int socketId, AppSpawnReqMsgNode *reqNode)
{
    APPSPAWN_LOGV("HandleMsgSend reqId: %{public}u msgId: %{public}d", reqNode->reqId, reqNode->msg->msgId);
    ListNode *sendNode = reqNode->msgBlocks.next;
    uint32_t currentIndex = 0;
    while (sendNode != NULL && sendNode != &reqNode->msgBlocks) {
        AppSpawnMsgBlock *sendBlock = (AppSpawnMsgBlock *)ListEntry(sendNode, AppSpawnMsgBlock, node);
        int ret = WriteMessage(socketId, sendBlock->buffer, sendBlock->currentIndex);
        currentIndex += sendBlock->currentIndex;
        APPSPAWN_LOGV("Write msg ret: %{public}d msgId: %{public}u %{public}u %{public}u",
            ret, reqNode->msg->msgId, reqNode->msg->msgLen, currentIndex);
        if (ret == 0) {
            sendNode = sendNode->next;
            continue;
        }
        APPSPAWN_LOGE("Send msg fail reqId: %{public}u msgId: %{public}d ret: %{public}d",
            reqNode->reqId, reqNode->msg->msgId, ret);
        return ret;
    }
    return 0;
}

static void TryCreateSocket(AppSpawnReqMsgMgr *reqMgr)
{
    uint32_t retryCount = 1;
    while (retryCount <= reqMgr->maxRetryCount) {
        if (reqMgr->socketId < 0) {
            reqMgr->socketId = CreateClientSocket(reqMgr->type, reqMgr->timeout);
        }
        if (reqMgr->socketId < 0) {
            APPSPAWN_LOGV("Failed to create socket, try again");
            usleep(RETRY_TIME);
            retryCount++;
            continue;
        }
        break;
    }
}

static int ClientSendMsg(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result)
{
    uint32_t retryCount = 1;
    while (retryCount <= reqMgr->maxRetryCount) {
        if (reqMgr->socketId < 0) { // try create socket
            TryCreateSocket(reqMgr);
            if (reqMgr->socketId < 0) {
                usleep(RETRY_TIME);
                retryCount++;
                continue;
            }
        }

        if (reqNode->msg->msgId == 0) {
            reqNode->msg->msgId = reqMgr->msgNextId++;
        }
        int ret = HandleMsgSend(reqMgr, reqMgr->socketId, reqNode);
        if (ret == 0) {
            ret = ReadMessage(reqMgr->socketId, reqNode->msg->msgId,
                reqMgr->recvBlock.buffer, reqMgr->recvBlock.blockSize, result);
        }
        if (ret == 0) {
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

int AppSpawnClientInit(const char *serviceName, AppSpawnClientHandle *handle)
{
    APPSPAWN_CHECK(serviceName != NULL, return APPSPAWN_ARG_INVALID, "Invalid service name");
    APPSPAWN_CHECK(handle != NULL, return APPSPAWN_ARG_INVALID, "Invalid handle for %{public}s", serviceName);
    APPSPAWN_LOGV("AppSpawnClientInit serviceName %{public}s", serviceName);
    AppSpawnClientType type = CLIENT_FOR_APPSPAWN;
    if (strcmp(serviceName, NWEBSPAWN_SERVER_NAME) == 0 || strstr(serviceName, NWEBSPAWN_SOCKET_NAME) != NULL) {
        type = CLIENT_FOR_NWEBSPAWN;
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

    APPSPAWN_LOGI("AppSpawnClientSendMsg reqId: %{public}u msgLen: %{public}u %{public}s",
        reqNode->reqId, reqNode->msg->msgLen, reqNode->msg->processName);
    pthread_mutex_lock(&reqMgr->mutex);
    int ret = ClientSendMsg(reqMgr, reqNode, result);
    if (ret != 0) {
        result->result = ret;
    }
    pthread_mutex_unlock(&reqMgr->mutex);
    APPSPAWN_LOGI("AppSpawnClientSendMsg reqId: %{public}u end result: 0x%{public}x pid: %{public}d",
        reqNode->reqId, result->result, result->pid);
    AppSpawnReqMsgFree(reqHandle);
    return ret;
}
