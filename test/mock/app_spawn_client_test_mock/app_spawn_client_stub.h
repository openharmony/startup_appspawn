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

uint32_t GetDefaultTimeout(uint32_t def);
InitClientInstance(AppSpawnClientType type);
CloseClientSocket(int socketId);
int CreateClientSocket(uint32_t type, uint32_t timeout);
int UpdateSocketTimeout(uint32_t timeout, int socketFd);
int ReadMessage(int socketFd, uint8_t *buf, int len, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result);
int WriteMessage(int socketFd, const uint8_t *buf, ssize_t len, int *fds, int *fdCount);
int HandleMsgSend(AppSpawnReqMsgMgr *reqMgr, int socketId, AppSpawnReqMsgNode *reqNode);
void TryCreateSocket(AppSpawnReqMsgMgr *reqMgr);
void SendSpawnListenMsg(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode);
int ClientSendMsg(AppSpawnReqMsgMgr *reqMgr, AppSpawnReqMsgNode *reqNode, AppSpawnResult *result);
int SpawnListenBase(AppSpawnReqMsgMgr *reqMgr, const char *processName, int fd, bool startFlag);
void SpawnListen(AppSpawnReqMsgMgr *reqMgr, const char *processName);
int AppSpawnClientInit(const char *serviceName, AppSpawnClientHandle *handle);
int AppSpawnClientDestroy(AppSpawnClientHandle handle);
int AppSpawnClientSendMsg(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle, AppSpawnResult *result);
int AppSpawnClientSendUserLockStatus(uint32_t userId, bool isLocked);
int SpawnListenFdSet(int fd);
int NativeSpawnListenFdSet(int fd);
int SpawnListenCloseSet(void);
int NativeSpawnListenCloseSet(void);
#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
