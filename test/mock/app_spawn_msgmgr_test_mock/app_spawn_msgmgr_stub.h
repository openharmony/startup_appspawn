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

void *GetAppSpawnMsgInfo(const AppSpawnMsgNode *message, int type);
void *GetAppSpawnMsgExtInfo(const AppSpawnMsgNode *message, const char *name, uint32_t *len);
int CheckAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index);
int SetSpawnMsgFlags(AppSpawnMsgFlags *msgFlags, uint32_t index);
int SetAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index);
AppSpawnMsgNode *CreateAppSpawnMsg(void);
int CheckRecvMsg(const AppSpawnMsg *msg);
int AppSpawnMsgRebuild(AppSpawnMsgNode *message, const AppSpawnMsg *msg);
AppSpawnMsgNode *RebuildAppSpawnMsgNode(AppSpawnMsgNode *message, AppSpawnedProcess *appInfo);
int CheckAppSpawnMsg(const AppSpawnMsgNode *message);
int CheckExtTlvInfo(const AppSpawnTlv *tlv, uint32_t remainLen);
int CheckMsgTlv(const AppSpawnTlv *tlv, uint32_t remainLen);
int DecodeAppSpawnMsg(AppSpawnMsgNode *message);
int GetAppSpawnMsgFromBuffer(const uint8_t *buffer, uint32_t bufferLen,
    AppSpawnMsgNode **outMsg, uint32_t *msgRecvLen, uint32_t *reminder);
void DumpMsgFlags(const char *processName, const char *info, const AppSpawnMsgFlags *msgFlags);
void DumpMsgExtInfo(const AppSpawnTlv *tlv);
void DumpAppSpawnMsg(const AppSpawnMsgNode *message);
#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
