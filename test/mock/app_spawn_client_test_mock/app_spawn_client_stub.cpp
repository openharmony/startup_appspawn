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

uint32_t GetDefaultTimeout(uint32_t def)
{
    if (def == 0) {
        printf("GetDefaultTimeout def is 0");
    }

    return 0;
}

int InitClientInstance(AppSpawnClientType type)
{
    if (type == nullptr) {
        printf("InitClientInstance type is null");
    }

    return 0;
}

void CloseClientSocket(int socketId)
{
    printf("CloseClientSocket %d" socketId);
}

int CreateClientSocket(uint32_t type, uint32_t timeout)
{
    if (type == 0) {
        printf("CreateClientSocket type is 0");
    }

    if (timeout == 0) {
        printf("CreateClientSocket timeout is 0");
    }

    return 0;
}

int UpdateSocketTimeout(uint32_t timeout, int socketFd)
{
    if (timeout == 0) {
        printf("UpdateSocketTimeout timeout is 0");
    }

    if (socketFd == 0) {
        printf("UpdateSocketTimeout socketFd is 0");
    }

    return 0;
}

int ReadMessage(int socketFd, uint8_t *buf, int len, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result)
{
    if (socketFd == 0) {
        printf("ReadMessage socketFd is 0");
    }

    if (buf == nullptr) {
        printf("ReadMessage buf is null");
    }

    if (len == 0) {
        printf("ReadMessage len is 0");
    }

    if (reqNode == nullptr) {
        printf("ReadMessage reqNode is null");
    }

    if (result == nullptr) {
        printf("ReadMessage result is null");
    }

    return 0;
}

int WriteMessage(int socketFd, const uint8_t *buf, ssize_t len, int *fds, int *fdCount)
{
    if (socketFd == 0) {
        printf("WriteMessage socketFd is 0");
    }

    if (buf == nullptr) {
        printf("WriteMessage buf is null");
    }

    if (len == 0) {
        printf("WriteMessage len is 0");
    }

    if (fds == nullptr) {
        printf("WriteMessage fds is null");
    }

    if (fdCount == nullptr) {
        printf("WriteMessage fdCount is null");
    }

    return 0;
}

int HandleMsgSend(AppSpawnReqMsgMgr *reqMgr, int socketId, AppSpawnReqMsgNode *reqNode)
{
    if (reqMgr == nullptr) {
        printf("HandleMsgSend reqMgr is null");
    }

    if (socketId == 0) {
        printf("HandleMsgSend socketId is 0");
    }

    if (reqNode == nullptr) {
        printf("HandleMsgSend reqNode is null");
    }

    return 0;
}

void TryCreateSocket(AppSpawnReqMsgMgr *reqMgr)
{
    if (reqMgr != nullptr) {
        printf("TryCreateSocket socketId %d", reqMgr->socketId);
    }
}

void SendSpawnListenMsg(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode)
{
    if (reqMgr != nullptr) {
        printf("SendSpawnListenMsg socketId %d", reqMgr->socketId);
    }

    if (reqNode == nullptr) {
        printf("SendSpawnListenMsg reqNode is null");
    }
}

int ClientSendMsg(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result)
{
    if (reqMgr == nullptr) {
        printf("ClientSendMsg reqMgr is null");
    }

    if (reqNode == nullptr) {
        printf("ClientSendMsg reqNode is null");
    }

    if (result == nullptr) {
        printf("ClientSendMsg result is null");
    }

    return 0;
}

int SpawnListenBase(AppSpawnReqMsgMgr *reqMgr, const char *processName, int fd, bool startFlag)
{
    if (reqMgr == nullptr) {
        printf("SpawnListenBase reqMgr is null");
    }

    if (processName == nullptr) {
        printf("SpawnListenBase processName is null");
    }

    if (fd == 0) {
        printf("SpawnListenBase fd is 0");
    }

    if (startFlag == true) {
        printf("SpawnListenBase startFlag is true");
    }

    return 0;
}

void SpawnListen(AppSpawnReqMsgMgr *reqMgr, const char *processName)
{
    if (reqMgr != nullptr) {
        printf("SendSpawnListenMsg socketId %d", reqMgr->socketId);
    }

    if (processName == nullptr) {
        printf("SpawnListen processName is null");
    }
}

int AppSpawnClientInit(const char *serviceName, AppSpawnClientHandle *handle)
{
    if (serviceName == nullptr) {
        printf("AppSpawnClientInit serviceName is null");
    }

    if (handle == nullptr) {
        printf("AppSpawnClientInit handle is null");
    }

    return 0;
}

int AppSpawnClientDestroy(AppSpawnClientHandle handle)
{
    if (handle == nullptr) {
        printf("AppSpawnClientDestroy handle is null");
    }

    return 0;
}

int AppSpawnClientSendMsg(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle, AppSpawnResult *result)
{
    if (handle == nullptr) {
        printf("AppSpawnClientSendMsg handle is null");
    }

    if (reqHandle == nullptr) {
        printf("AppSpawnClientSendMsg reqHandle is null");
    }

    if (result == nullptr) {
        printf("AppSpawnClientSendMsg result is null");
    }

    return 0;
}

int AppSpawnClientSendUserLockStatus(uint32_t userId, bool isLocked)
{
    if (userId == 0) {
        printf("AppSpawnClientSendUserLockStatus userId is 0");
    }

    if (isLocked == true) {
        printf("AppSpawnClientSendUserLockStatus isLocked is true");
    }

    return 0;
}

int SpawnListenFdSet(int fd)
{
    if (fd == 0) {
        printf("SpawnListenFdSet fd is 0");
    }

    return 0;
}

int NativeSpawnListenFdSet(int fd)
{
    if (fd == 0) {
        printf("NativeSpawnListenFdSet fd is 0");
    }

    return 0;
}

int SpawnListenCloseSet(void)
{
    return 0;
}

int NativeSpawnListenCloseSet(void)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
