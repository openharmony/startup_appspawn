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

#ifndef APPSPAWN_CLIENT_H
#define APPSPAWN_CLIENT_H
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "appspawn_msg.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ASAN_DETECTOR
#define TIMEOUT_DEF 60
#define ASAN_TIMEOUT 60
#elif APPSPAWN_TEST
#define TIMEOUT_DEF 2
#define ASAN_TIMEOUT 11
#else
#define TIMEOUT_DEF 2
#define ASAN_TIMEOUT 10
#endif

#define CJAPPSPAWN_CLIENT_TIMEOUT 10
#define RETRY_TIME (200 * 1000)     // 200 * 1000 wait 200ms CONNECT_RETRY_DELAY = 200 * 1000
#define MAX_RETRY_SEND_COUNT 2      // 2 max retry count CONNECT_RETRY_MAX_TIMES = 2;

#define NORMAL_READ_RETRY_TIME (3 * 1000 * 1000 + 1000 * 1000)   // 4s, Exceed WAIT_CHILD_RESPONSE_TIMEOUT by 1s
// Exceed COLD_CHILD_RESPONSE_TIMEOUT by 0.5s
#define COLDRUN_READ_RETRY_TIME (ASAN_TIMEOUT * 1000 * 1000 + 500 * 1000)

// only used for ExternalFileManager.hap
#define GID_FILE_ACCESS 1006
#define GID_USER_DATA_RW 1008

#define MAX_DATA_IN_TLV 2

struct TagAppSpawnReqMsgNode;
typedef enum {
    CLIENT_FOR_APPSPAWN,
    CLIENT_FOR_NWEBSPAWN,
    CLIENT_FOR_CJAPPSPAWN,
    CLIENT_FOR_NATIVESPAWN,
    CLIENT_FOR_HYBRIDSPAWN,
    CLIENT_MAX
} AppSpawnClientType;

typedef struct {
    struct ListNode node;
    uint32_t blockSize;     // block 的大小
    uint32_t currentIndex;  // 当前已经填充的位置
    uint8_t buffer[0];
} AppSpawnMsgBlock;

typedef struct TagAppSpawnReqMsgMgr {
    AppSpawnClientType type;
    uint32_t maxRetryCount;
    uint32_t timeout;
    uint32_t msgNextId;
    int socketId;
    pthread_mutex_t mutex;
    AppSpawnMsgBlock recvBlock;  // 消息接收缓存
} AppSpawnReqMsgMgr;

typedef struct TagAppSpawnReqMsgNode {
    struct ListNode node;
    uint32_t reqId;
    uint32_t retryCount;
    int fdCount;
    int fds[APP_MAX_FD_COUNT];
    int isAsan;
    AppSpawnMsgFlags *msgFlags;
    AppSpawnMsgFlags *permissionFlags;
    AppSpawnMsg *msg;
    struct ListNode msgBlocks;  // 保存实际的消息数据
} AppSpawnReqMsgNode;

typedef struct {
    uint8_t *data;
    uint16_t dataLen;
    uint16_t dataType;
} AppSpawnAppData;

int32_t GetPermissionMaxCount();

#ifdef __cplusplus
}
#endif
#endif
