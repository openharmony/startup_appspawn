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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AppSpawnContent AppSpawnContent;
typedef struct AppSpawnClient AppSpawnClient;
typedef struct TagAppSpawnReqMsgNode AppSpawnReqMsgNode;
typedef void *AppSpawnClientHandle;
typedef struct TagAppSpawnReqMsgMgr AppSpawnReqMsgMgr;
typedef struct TagAppSpawningCtx AppSpawningCtx;
typedef struct TagAppSpawnMsg AppSpawnMsg;
typedef struct TagAppSpawnSandboxCfg  AppSpawnSandboxCfg;
typedef struct TagAppSpawnExtData AppSpawnExtData;
typedef struct TagSandboxContext SandboxContext;
typedef struct TagAppSpawnedProcess AppSpawnedProcess;
typedef struct TagAppSpawnForkArg AppSpawnForkArg;
typedef struct TagAppSpawnMsgNode AppSpawnMsgNode;
typedef struct TagAppSpawnMgr AppSpawnMgr;

void SetHapDomainSetcontextResult(int result);

void ProcessSignal(const struct signalfd_siginfo *siginfo);

int CreateClientSocket(uint32_t type, int block);
void CloseClientSocket(int socketId);

int ParseAppSandboxConfig(const cJSON *appSandboxConfig, AppSpawnSandboxCfg *sandbox);
AppSpawnSandboxCfg *CreateAppSpawnSandbox(void);
void AddDefaultVariable(void);

int AppSpawnClearEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int AppSpawnChild(AppSpawnContent *content, AppSpawnClient *client);
int WriteMsgToChild(AppSpawningCtx *property, bool isNweb);

int WriteToFile(const char *path, int truncated, pid_t pids[], uint32_t count);
int GetCgroupPath(const AppSpawnedProcess *appInfo, char *buffer, uint32_t buffLen);

void SetDeveloperMode(bool mode);

int LoadPermission(AppSpawnClientType type);
void DeletePermission(AppSpawnClientType type);

#define STUB_NEED_CHECK 0x01
typedef int (*ExecvFunc)(const char *pathname, char *const argv[]);
enum {
    STUB_MOUNT,
    STUB_EXECV,
    STUB_MAX,
};

typedef struct {
    uint16_t type;
    uint16_t flags;
    int result;
    void *arg;
} StubNode;
StubNode *GetStubNode(int type);
#ifdef __cplusplus
}
#endif
#endif // APPSPAWN_TEST_STUB_H
