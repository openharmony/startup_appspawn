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
    return 0;
}

int InitClientInstance(AppSpawnClientType type)
{
    return 0;
}

void CloseClientSocket(int socketId)
{
    printf("CloseClientSocket %d" socketId);
}

int CreateClientSocket(uint32_t type, uint32_t timeout)
{
    return 0;
}

int UpdateSocketTimeout(uint32_t timeout, int socketFd)
{
    return 0;
}

int ReadMessage(int socketFd, uint8_t *buf, int len, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result)
{
    return 0;
}

int WriteMessage(int socketFd, const uint8_t *buf, ssize_t len, int *fds, int *fdCount)
{
    return 0;
}

int HandleMsgSend(AppSpawnReqMsgMgr *reqMgr, int socketId, AppSpawnReqMsgNode *reqNode)
{
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
}

int ClientSendMsg(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result)
{
    return 0;
}

int SpawnListenBase(AppSpawnReqMsgMgr *reqMgr, const char *processName, int fd, bool startFlag)
{
    return 0;
}

void SpawnListen(AppSpawnReqMsgMgr *reqMgr, const char *processName)
{
    if (reqMgr != nullptr) {
        printf("SendSpawnListenMsg socketId %d", reqMgr->socketId);
    }
}

int AppSpawnClientInit(const char *serviceName, AppSpawnClientHandle *handle)
{
    return 0;
}

int AppSpawnClientDestroy(AppSpawnClientHandle handle)
{
    return 0;
}

int AppSpawnClientSendMsg(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle, AppSpawnResult *result)
{
    return 0;
}

int AppSpawnClientSendUserLockStatus(uint32_t userId, bool isLocked)
{
    return 0;
}

int SpawnListenFdSet(int fd)
{
    return 0;
}

int NativeSpawnListenFdSet(int fd)
{
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
