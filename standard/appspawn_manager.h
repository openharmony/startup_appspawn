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

#ifndef APPSPAWN_MANAGER_H
#define APPSPAWN_MANAGER_H

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

#define MODE_ID_INDEX 1
#define MODE_VALUE_INDEX 2
#define FD_ID_INDEX 3
#define FD_VALUE_INDEX 4
#define FLAGS_VALUE_INDEX 5
#define SHM_SIZE_INDEX 6
#define PARAM_ID_INDEX 7
#define PARAM_VALUE_INDEX 8
#define CLIENT_ID_INDEX 9
#define ARG_NULL 10

#define MAX_DIED_PROCESS_COUNT 5

#define INVALID_OFFSET 0xffffffff

#define APP_STATE_IDLE 1
#define APP_STATE_SPAWNING 2

#define APPSPAWN_INLINE __attribute__((always_inline)) inline

typedef struct AppSpawnContent AppSpawnContent;
typedef struct AppSpawnClient AppSpawnClient;
typedef struct TagAppSpawnConnection AppSpawnConnection;

typedef struct TagAppSpawnMsgNode {
    AppSpawnConnection *connection;
    AppSpawnMsg msgHeader;
    uint32_t tlvCount;
    uint32_t *tlvOffset;  // 记录属性的在msg中的偏移，不完全拷贝试消息完整
    uint8_t *buffer;
} AppSpawnMsgNode;

typedef struct {
    int32_t fd[2];  // 2 fd count
    WatcherHandle watcherHandle;
    TimerHandle timer;
    char *childMsg;
    uint32_t msgSize;
    char *coldRunPath;
} AppSpawnForkCtx;

typedef struct TagAppSpawningCtx {
    AppSpawnClient client;
    struct ListNode node;
    AppSpawnForkCtx forkCtx;
    AppSpawnMsgNode *message;
    pid_t pid;
    int state;
    struct timespec spawnStart;
} AppSpawningCtx;

typedef struct TagAppSpawnedProcess {
    struct ListNode node;
    uid_t uid;
    pid_t pid;
    uint32_t max;
    int exitStatus;
    struct timespec spawnStart;
    struct timespec spawnEnd;
#ifdef DEBUG_BEGETCTL_BOOT
    AppSpawnMsgNode *message;
#endif
    char name[0];
} AppSpawnedProcess;

typedef struct TagAppSpawnMgr {
    AppSpawnContent content;
    TaskHandle server;
    SignalHandle sigHandler;
    pid_t servicePid;
    struct ListNode appQueue;  // save app pid and name
    uint32_t diedAppCount;
    struct ListNode diedQueue;      // save app pid and name
    struct ListNode appSpawnQueue;  // save app pid and name
    struct timespec perLoadStart;
    struct timespec perLoadEnd;
    struct ListNode extData;
} AppSpawnMgr;

/**
 * @brief App Spawn Mgr object op
 *
 */
AppSpawnMgr *CreateAppSpawnMgr(int mode);
AppSpawnMgr *GetAppSpawnMgr(void);
void DeleteAppSpawnMgr(AppSpawnMgr *mgr);
AppSpawnContent *GetAppSpawnContent(void);

/**
 * @brief 孵化成功后进程或者app实例的操作
 *
 */
typedef void (*AppTraversal)(const AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data);
void TraversalSpawnedProcess(AppTraversal traversal, void *data);
AppSpawnedProcess *AddSpawnedProcess(pid_t pid, const char *processName);
AppSpawnedProcess *GetSpawnedProcess(pid_t pid);
AppSpawnedProcess *GetSpawnedProcessByName(const char *name);
void TerminateSpawnedProcess(AppSpawnedProcess *node);

/**
 * @brief 孵化过程中的ctx对象的操作
 *
 */
typedef void (*ProcessTraversal)(const AppSpawnMgr *mgr, AppSpawningCtx *ctx, void *data);
void AppSpawningCtxTraversal(ProcessTraversal traversal, void *data);
AppSpawningCtx *GetAppSpawningCtxByPid(pid_t pid);
AppSpawningCtx *CreateAppSpawningCtx();
void DeleteAppSpawningCtx(AppSpawningCtx *property);
int KillAndWaitStatus(pid_t pid, int sig, int *exitStatus);

/**
 * @brief 消息解析、处理
 *
 */
void ProcessAppSpawnDumpMsg(const AppSpawnMsgNode *message);
int ProcessTerminationStatusMsg(const AppSpawnMsgNode *message, AppSpawnResult *result);

AppSpawnMsgNode *CreateAppSpawnMsg(void);
void DeleteAppSpawnMsg(AppSpawnMsgNode *msgNode);
int CheckAppSpawnMsg(const AppSpawnMsgNode *message);
int DecodeAppSpawnMsg(AppSpawnMsgNode *message);
int GetAppSpawnMsgFromBuffer(const uint8_t *buffer, uint32_t bufferLen,
    AppSpawnMsgNode **outMsg, uint32_t *msgRecvLen, uint32_t *reminder);
AppSpawnMsgNode *RebuildAppSpawnMsgNode(AppSpawnMsgNode *message, AppSpawnedProcess *appInfo);

/**
 * @brief 消息内容操作接口
 *
 */
void DumpAppSpawnMsg(const AppSpawnMsgNode *message);
void *GetAppSpawnMsgInfo(const AppSpawnMsgNode *message, int type);
void *GetAppSpawnMsgExtInfo(const AppSpawnMsgNode *message, const char *name, uint32_t *len);
int CheckAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index);
int SetAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index);

APPSPAWN_INLINE int IsSpawnServer(const AppSpawnMgr *content)
{
    return (content != NULL) && (content->servicePid == getpid());
}

APPSPAWN_INLINE int IsNWebSpawnMode(const AppSpawnMgr *content)
{
    return (content != NULL) &&
        (content->content.mode == MODE_FOR_NWEB_SPAWN || content->content.mode == MODE_FOR_NWEB_COLD_RUN);
}

APPSPAWN_INLINE int IsColdRunMode(const AppSpawnMgr *content)
{
    return (content != NULL) &&
        (content->content.mode == MODE_FOR_APP_COLD_RUN || content->content.mode == MODE_FOR_NWEB_COLD_RUN);
}

APPSPAWN_INLINE int IsDeveloperModeOn(const AppSpawningCtx *property)
{
    return (property != NULL && ((property->client.flags & APP_DEVELOPER_MODE) == APP_DEVELOPER_MODE));
}

APPSPAWN_INLINE int GetAppSpawnMsgType(const AppSpawningCtx *appProperty)
{
    return (appProperty != NULL && appProperty->message != NULL) ?
        appProperty->message->msgHeader.msgType : MAX_TYPE_INVALID;
}

APPSPAWN_INLINE const char *GetProcessName(const AppSpawningCtx *property)
{
    if (property == NULL || property->message == NULL) {
        return NULL;
    }
    return property->message->msgHeader.processName;
}

APPSPAWN_INLINE const char *GetBundleName(const AppSpawningCtx *property)
{
    if (property == NULL || property->message == NULL) {
        return NULL;
    }
    AppSpawnMsgBundleInfo *info = (AppSpawnMsgBundleInfo *)GetAppSpawnMsgInfo(property->message, TLV_BUNDLE_INFO);
    if (info != NULL) {
        return info->bundleName;
    }
    return NULL;
}

APPSPAWN_INLINE void *GetAppProperty(const AppSpawningCtx *property, uint32_t type)
{
    APPSPAWN_CHECK(property != NULL && property->message != NULL,
        return NULL, "Invalid property for type %{public}u", type);
    return GetAppSpawnMsgInfo(property->message, type);
}

APPSPAWN_INLINE void *GetAppPropertyExt(const AppSpawningCtx *property, const char *name, uint32_t *len)
{
    APPSPAWN_CHECK(name != NULL, return NULL, "Invalid name ");
    APPSPAWN_CHECK(property != NULL && property->message != NULL,
        return NULL, "Invalid property for name %{public}s", name);
    return GetAppSpawnMsgExtInfo(property->message, name, len);
}

APPSPAWN_INLINE int CheckAppMsgFlagsSet(const AppSpawningCtx *property, uint32_t index)
{
    APPSPAWN_CHECK(property != NULL && property->message != NULL,
        return 0, "Invalid property for name %{public}u", TLV_MSG_FLAGS);
    return CheckAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, index);
}

APPSPAWN_INLINE int CheckAppPermissionFlagSet(const AppSpawningCtx *property, uint32_t index)
{
    APPSPAWN_CHECK(property != NULL && property->message != NULL,
        return 0, "Invalid property for name %{public}u", TLV_PERMISSION);
    return CheckAppSpawnMsgFlag(property->message, TLV_PERMISSION, index);
}

APPSPAWN_INLINE int SetAppPermissionFlags(const AppSpawningCtx *property, uint32_t index)
{
    APPSPAWN_CHECK(property != NULL && property->message != NULL,
        return -1, "Invalid property for name %{public}u", TLV_PERMISSION);
    return SetAppSpawnMsgFlag(property->message, TLV_PERMISSION, index);
}

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_MANAGER_H
