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

#ifndef APPSPAWN_SERVICE_H
#define APPSPAWN_SERVICE_H

#include <limits.h>
#include <stdbool.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_msg.h"
#include "appspawn_server.h"
#include "appspawn_utils.h"
#include "list.h"
#include "loop_event.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef APPSPAWN_TEST
#define MAX_WAIT_MSG_COMPLETE (5 * 100)  // 500ms
#define WAIT_CHILD_RESPONSE_TIMEOUT 3  //3s
#else
#define MAX_WAIT_MSG_COMPLETE (5 * 1000)  // 5s
#define WAIT_CHILD_RESPONSE_TIMEOUT 3  //3s
#endif

typedef struct TagAppSpawnMsgNode AppSpawnMsgNode;
typedef struct TagAppSpawnMsgReceiverCtx {
    uint32_t nextMsgId;              // 校验消息id
    uint32_t msgRecvLen;             // 已经接收的长度
    TimerHandle timer;               // 测试消息完整
    AppSpawnMsgNode *incompleteMsg;  // 保存不完整的消息，额外保存消息头信息
} AppSpawnMsgReceiverCtx;

typedef struct TagAppSpawnConnection {
    uint32_t connectionId;
    TaskHandle stream;
    AppSpawnMsgReceiverCtx receiverCtx;
} AppSpawnConnection;

typedef struct TagAppSpawnStartArg {
    RunMode mode;
    uint32_t moduleType;
    const char *socketName;
    const char *serviceName;
    uint32_t initArg : 1;
} AppSpawnStartArg;

pid_t NWebSpawnLaunch(void);
void NWebSpawnInit(void);
AppSpawnContent *StartSpawnService(const AppSpawnStartArg *arg, uint32_t argvSize, int argc, char *const argv[]);
void AppSpawnDestroyContent(AppSpawnContent *content);

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_SERVICE_H
