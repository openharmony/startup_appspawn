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
#include "appspawn_hook.h"
#include "appspawn_encaps.h"
#include "appspawn_server.h"

void SetBoolParamResult(const char *key, bool flag);
#ifdef __cplusplus
extern "C" {
#endif

typedef struct TagMountTestArg {
    const char *originPath;
    const char *destinationPath;
    const char *fsType;
    unsigned long mountFlags;
    const char *options;
    mode_t mountSharedFlag;
} MountTestArg;

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
typedef struct TagPathMountNode PathMountNode;
typedef struct TagMountTestArg MountTestArg;
typedef struct TagVarExtraData VarExtraData;
typedef struct TagSandboxSection SandboxSection;
typedef struct TagAppSpawnNamespace {
    AppSpawnExtData extData;
    int nsSelfPidFd;
    int nsInitPidFd;
} AppSpawnNamespace;
typedef struct TagAppSpawnedProcess AppSpawnedProcessInfo;

int AppSpawnExtDataCompareDataId(ListNode *node, void *data);
AppSpawnNamespace *GetAppSpawnNamespace(const AppSpawnMgr *content);
int SetPidNamespace(int nsPidFd, int nsType);
AppSpawnNamespace *CreateAppSpawnNamespace(void);
void DeleteAppSpawnNamespace(AppSpawnNamespace *ns);
void FreeAppSpawnNamespace(struct TagAppSpawnExtData *data);
int PreForkSetPidNamespace(AppSpawnMgr *content, AppSpawningCtx *property);
int PostForkSetPidNamespace(AppSpawnMgr *content, AppSpawningCtx *property);
int ProcessMgrRemoveApp(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo);
int ProcessMgrAddApp(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo);
void TryCreateSocket(AppSpawnReqMsgMgr *reqMgr);

int MountAllGroup(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox,
                  const cJSON *groups);
int MountAllHsp(const SandboxContext *context, const cJSON *hsps);

void CheckAndCreateSandboxFile(const char *file);
int VarPackageNameReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int ReplaceVariableForDepSandboxPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int ReplaceVariableForDepSrcPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int ReplaceVariableForDepPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int SpawnPrepareSandboxCfg(AppSpawnMgr *content, AppSpawningCtx *property);
unsigned long GetMountModeFromConfig(const cJSON *config, const char *key, unsigned long def);
uint32_t GetFlagIndexFromJson(const cJSON *config);
int ParseMountPathsConfig(AppSpawnSandboxCfg *sandbox,
    const cJSON *mountConfigs, SandboxSection *section, uint32_t type);
int ParseSymbolLinksConfig(AppSpawnSandboxCfg *sandbox, const cJSON *symbolLinkConfigs,
    SandboxSection *section);
int ParseGidTableConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, SandboxSection *section);

int AppSpawnColdStartApp(struct AppSpawnContent *content, AppSpawnClient *client);
void ProcessSignal(const struct signalfd_siginfo *siginfo);
int CreateClientSocket(uint32_t type, int block);
void CloseClientSocket(int socketId);
int ParseAppSandboxConfig(const cJSON *appSandboxConfig, AppSpawnSandboxCfg *sandbox);
AppSpawnSandboxCfg *CreateAppSpawnSandbox(ExtDataType type);
void AddDefaultVariable(void);
bool CheckDirRecursive(const char *path);
void CreateDemandSrc(const SandboxContext *context, const PathMountNode *sandboxNode, const MountTestArg *args);
int CheckSandboxMountNode(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation);
int AppSpawnClearEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int AppSpawnChild(AppSpawnContent *content, AppSpawnClient *client);
char *GetSpawnNameByRunMode(RunMode mode);
int WriteMsgToChild(AppSpawningCtx *property, RunMode mode);
int WriteToFile(const char *path, int truncated, pid_t pids[], uint32_t count);
int GetCgroupPath(const AppSpawnedProcess *appInfo, char *buffer, uint32_t buffLen);
void SetDeveloperMode(bool mode);
int LoadPermission(AppSpawnClientType type);
void DeletePermission(AppSpawnClientType type);
int SetProcessName(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetIsolateDir(const AppSpawningCtx *property);
int SetCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetFdEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int PreLoadEnablePidNs(AppSpawnMgr *content);
int NsInitFunc();
int GetNsPidFd(pid_t pid);
int PreLoadEnablePidNs(AppSpawnMgr *content);
pid_t GetPidByName(const char *name);
int RunBegetctlBootApp(AppSpawnMgr *content, AppSpawningCtx *property);
void SetSystemEnv(void);
void RunAppSandbox(const char *ptyName);
HOOK_MGR *GetAppSpawnHookMgr(void);
int SpawnKickDogStart(AppSpawnMgr *mgrContent);
cJSON *GetEncapsPermissions(cJSON *extInfoJson, int *count);
int AddMembersToEncapsInfo(cJSON *extInfoJson, UserEncaps *encapsInfo, int count);
int SpawnSetPermissions(AppSpawningCtx *property, UserEncaps *encapsInfo);
int AddPermissionItemToEncapsInfo(UserEncap *encap, cJSON *permissionItem);
void FreeEncapsInfo(UserEncaps *encapsInfo);
int SpawnSetEncapsPermissions(AppSpawnMgr *content, AppSpawningCtx *property);
int WriteEncapsInfo(int fd, AppSpawnEncapsBaseType encapsType, const void *encapsInfo, uint32_t flag);
int AddPermissionIntArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize);
int AddPermissionBoolArrayToValue(cJSON *arrayItem, UserEncap *encap, uint32_t arraySize);
int AddPermissionStrArrayToValue(cJSON *arrayItem, UserEncap *encap);
int AddPermissionArrayToValue(cJSON *permissionItemArr, UserEncap *encap);
int SetSchedPriority(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetUidGid(const AppSpawnMgr *content, const AppSpawningCtx *property);
int PrctlStub(int option, ...);
void DumpMsgFlags(const AppSpawnMsgNode *message);
int SetXpmConfig(AppSpawnMgr *content, AppSpawningCtx *property);

#ifdef APPSPAWN_HITRACE_OPTION
int FilterAppSpawnTrace(AppSpawnMgr *content, AppSpawningCtx *property);
#endif

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
