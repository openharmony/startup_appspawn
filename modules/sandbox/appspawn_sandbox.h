/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef APPSPAWN_SANDBOX_H
#define APPSPAWN_SANDBOX_H

#include <limits.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SANDBOX_STAMP_FILE_SUFFIX ".stamp"
#define JSON_FLAGS_INTERNAL "__internal__"
#define SANDBOX_NWEBSPAWN_ROOT_PATH APPSPAWN_BASE_DIR "/mnt/sandbox/com.ohos.render/"
#define OHOS_RENDER "__internal__.com.ohos.render"

#define PHYSICAL_APP_INSTALL_PATH "/data/app/el1/bundle/public/"
#define APL_SYSTEM_CORE "system_core"
#define APL_SYSTEM_BASIC "system_basic"
#define DEFAULT_NWEB_SANDBOX_SEC_PATH "/data/app/el1/bundle/public/com.ohos.nweb"  // persist.nweb.sandbox.src_path

#define PARAMETER_PACKAGE_NAME "<PackageName>"
#define PARAMETER_USER_ID "<currentUserId>"
#define PARAMETER_PACKAGE_INDEX "<PackageName_index>"

#define FILE_MODE 0711
#define MAX_SANDBOX_BUFFER 256
#define FUSE_OPTIONS_MAX_LEN 256
#define DLP_FUSE_FD 1000
#define APP_FLAGS_SECTION 0x80000000
#define BASIC_MOUNT_FLAGS (MS_REC | MS_BIND)
#define INVALID_UID ((uint32_t)-1)

#ifdef APPSPAWN_64
#define APPSPAWN_LIB_NAME "lib64"
#else
#define APPSPAWN_LIB_NAME "lib"
#endif

#define MOUNT_MODE_NONE 0       // "none"
#define MOUNT_MODE_ALWAYS 1     // "always"
#define MOUNT_MODE_NOT_EXIST 2  // "not-exists"

#define MOUNT_PATH_OP_NONE    ((uint32_t)-1)
#define MOUNT_PATH_OP_SYMLINK SANDBOX_TAG_INVALID
#define MOUNT_PATH_OP_UNMOUNT    (SANDBOX_TAG_INVALID + 1)
#define MOUNT_PATH_OP_ONLY_SANDBOX    (SANDBOX_TAG_INVALID + 2)
#define MOUNT_PATH_OP_REPLACE_BY_SANDBOX    (SANDBOX_TAG_INVALID + 3)
#define MOUNT_PATH_OP_REPLACE_BY_SRC    (SANDBOX_TAG_INVALID + 4)
#define FILE_CROSS_APP_MODE "ohos.permission.FILE_CROSS_APP"

typedef enum SandboxTag {
    SANDBOX_TAG_MOUNT_PATH = 0,
    SANDBOX_TAG_MOUNT_FILE,
    SANDBOX_TAG_SYMLINK,
    SANDBOX_TAG_PERMISSION,
    SANDBOX_TAG_PACKAGE_NAME,
    SANDBOX_TAG_SPAWN_FLAGS,
    SANDBOX_TAG_NAME_GROUP,
    SANDBOX_TAG_SYSTEM_CONST,
    SANDBOX_TAG_APP_VARIABLE,
    SANDBOX_TAG_APP_CONST,
    SANDBOX_TAG_REQUIRED,
    SANDBOX_TAG_INVALID
} SandboxNodeType;

typedef struct {
    struct ListNode node;
    uint32_t type;
} SandboxMountNode;

typedef struct TagSandboxQueue {
    struct ListNode front;
    uint32_t type;
} SandboxQueue;

/*
"create-on-demand": {
    "uid": "userId", // 默认使用消息的uid、gid
    "gid":  "groupId",
    "ugo": 750
    }
*/
typedef struct {
    uid_t uid;
    gid_t gid;
    uint32_t mode;
} PathDemandInfo;

typedef struct TagPathMountNode {
    SandboxMountNode sandboxNode;
    char *source;                  // source 目录，一般是全局的fs 目录
    char *target;                  // 沙盒化后的目录
    mode_t destMode;               // "dest-mode": "S_IRUSR | S_IWOTH | S_IRWXU "  默认值：0
    uint32_t mountSharedFlag : 1;  // "mount-shared-flag" : "true", 默认值：false
    uint32_t createDemand : 1;
    uint32_t checkErrorFlag : 1;
    uint32_t category;
    char *appAplName;
    PathDemandInfo demandInfo[0];
} PathMountNode;

typedef struct TagSymbolLinkNode {
    SandboxMountNode sandboxNode;
    char *target;
    char *linkName;
    mode_t destMode;  // "dest-mode": "S_IRUSR | S_IWOTH | S_IRWXU "
    uint32_t checkErrorFlag : 1;
} SymbolLinkNode;

typedef struct TagSandboxSection {
    SandboxMountNode sandboxNode;
    struct ListNode front;  // mount-path
    char *name;
    uint32_t number : 16;
    uint32_t gidCount : 16;
    gid_t *gidTable;             // "gids": [1006, 1008],
    uint32_t sandboxSwitch : 1;  // "sandbox-switch": "ON",
    uint32_t sandboxShared : 1;  // "sandbox-switch": "ON",
    SandboxMountNode **nameGroups;
} SandboxSection;

typedef struct {
    SandboxSection section;
} SandboxPackageNameNode;

typedef struct {
    SandboxSection section;
    uint32_t flagIndex;
} SandboxFlagsNode;

typedef struct TagSandboxGroupNode {
    SandboxSection section;
    uint32_t destType;
    PathMountNode *depNode;
    uint32_t depMode;
    uint32_t depMounted : 1; // 是否执行了挂载
} SandboxNameGroupNode;

typedef struct TagPermissionNode {
    SandboxSection section;
    int32_t permissionIndex;
} SandboxPermissionNode;

typedef struct TagAppSpawnSandboxCfg {
    AppSpawnExtData extData;
    SandboxQueue requiredQueue;
    SandboxQueue permissionQueue;
    SandboxQueue packageNameQueue;  // SandboxSection
    SandboxQueue spawnFlagsQueue;
    SandboxQueue nameGroupsQueue;
    uint32_t depNodeCount;
    SandboxNameGroupNode **depGroupNodes;
    int32_t maxPermissionIndex;
    uint32_t sandboxNsFlags;  // "sandbox-ns-flags": [ "pid", "net" ], // for appspawn and newspawn
    // for comm section
    uint32_t topSandboxSwitch : 1;  // "top-sandbox-switch": "ON",
    uint32_t appFullMountEnable : 1;
    uint32_t pidNamespaceSupport : 1;
    uint32_t mounted : 1;
    char *rootPath;
} AppSpawnSandboxCfg;

enum {
    BUFFER_FOR_SOURCE,
    BUFFER_FOR_TARGET,
    BUFFER_FOR_TMP,
    MAX_BUFFER
};

typedef struct TagSandboxBuffer {
    uint32_t bufferLen;
    uint32_t current;
    char *buffer;
} SandboxBuffer;

typedef struct TagSandboxContext {
    SandboxBuffer buffer[MAX_BUFFER];
    const char *bundleName;
    const AppSpawnMsgNode *message;  // 修改成操作消息
    uint32_t sandboxSwitch : 1;
    uint32_t sandboxShared : 1;
    uint32_t bundleHasWps : 1;
    uint32_t dlpBundle : 1;
    uint32_t dlpUiExtType : 1;
    uint32_t appFullMountEnable : 1;
    uint32_t nwebspawn : 1;
    char *rootPath;
} SandboxContext;

/**
 * @brief AppSpawnSandboxCfg op
 *
 * @return AppSpawnSandboxCfg*
 */
AppSpawnSandboxCfg *CreateAppSpawnSandbox(void);
AppSpawnSandboxCfg *GetAppSpawnSandbox(const AppSpawnMgr *content);
void DeleteAppSpawnSandbox(AppSpawnSandboxCfg *sandbox);
int LoadAppSandboxConfig(AppSpawnSandboxCfg *sandbox, int nwebSpawn);
void DumpAppSpawnSandboxCfg(AppSpawnSandboxCfg *sandbox);

/**
 * @brief SandboxSection op
 *
 */
SandboxSection *CreateSandboxSection(const char *name, uint32_t dataLen, uint32_t type);
SandboxSection *GetSandboxSection(const SandboxQueue *queue, const char *name);
void AddSandboxSection(SandboxSection *node, SandboxQueue *queue);
void DeleteSandboxSection(SandboxSection *node);
__attribute__((always_inline)) inline uint32_t GetSectionType(const SandboxSection *section)
{
    return section != NULL ? section->sandboxNode.type : SANDBOX_TAG_INVALID;
}

/**
 * @brief SandboxMountNode op
 *
 */
SandboxMountNode *CreateSandboxMountNode(uint32_t dataLen, uint32_t type);
SandboxMountNode *GetFirstSandboxMountNode(const SandboxSection *section);
void DeleteSandboxMountNode(SandboxMountNode *mountNode);
void AddSandboxMountNode(SandboxMountNode *node, SandboxSection *section);
PathMountNode *GetPathMountNode(const SandboxSection *section, int type, const char *source, const char *target);
SymbolLinkNode *GetSymbolLinkNode(const SandboxSection *section, const char *target, const char *linkName);

/**
 * @brief sandbox mount interface
 *
 */
int MountSandboxConfigs(const AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn);
int StagedMountSystemConst(const AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn);
int StagedMountPreUnShare(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int StagedMountPostUnshare(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
// 在子进程退出时，由父进程发起unmount操作
int UnmountDepPaths(const AppSpawnSandboxCfg *sandbox, uid_t uid);
int UnmountSandboxConfigs(const AppSpawnSandboxCfg *sandbox, uid_t uid, const char *name);

/**
 * @brief Variable op
 *
 */
typedef struct {
    struct ListNode node;
    ReplaceVarHandler replaceVar;
    char name[0];
} AppSandboxVarNode;

typedef struct TagVarExtraData {
    uint32_t sandboxTag;
    uint32_t operation;
    union {
        PathMountNode *depNode;
    } data;
} VarExtraData;

void ClearVariable(void);
void AddDefaultVariable(void);
const char *GetSandboxRealVar(const SandboxContext *context,
    uint32_t bufferType, const char *source, const char *prefix, const VarExtraData *extraData);

/**
 * @brief expand config
 *
 */
typedef struct {
    struct ListNode node;
    ProcessExpandSandboxCfg cfgHandle;
    int prio;
    char name[0];
} AppSandboxExpandAppCfgNode;
int ProcessExpandAppSandboxConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandBox, const char *name);
void AddDefaultExpandAppSandboxConfigHandle(void);
void ClearExpandAppSandboxConfigHandle(void);

__attribute__((always_inline)) inline void *GetSpawningMsgInfo(const SandboxContext *context, uint32_t type)
{
    APPSPAWN_CHECK(context->message != NULL,
        return NULL, "Invalid property for type %{public}u", type);
    return GetAppSpawnMsgInfo(context->message, type);
}

/**
 * @brief Sandbox Context op
 *
 * @return SandboxContext*
 */
SandboxContext *GetSandboxContext(void);
void DeleteSandboxContext(SandboxContext *context);

/**
 * @brief defineMount Arg Template and operation
 *
 */
enum {
    MOUNT_TMP_DEFAULT,
    MOUNT_TMP_RDONLY,
    MOUNT_TMP_EPFS,
    MOUNT_TMP_DAC_OVERRIDE,
    MOUNT_TMP_FUSE,
    MOUNT_TMP_DLP_FUSE,
    MOUNT_TMP_SHRED,
    MOUNT_TMP_MAX
};

typedef struct {
    char *name;
    uint32_t category;
    const char *fsType;
    unsigned long mountFlags;
    const char *options;
    mode_t mountSharedFlag;
} MountArgTemplate;

typedef struct {
    const char *name;
    unsigned long flags;
} SandboxFlagInfo;

uint32_t GetMountCategory(const char *name);
const MountArgTemplate *GetMountArgTemplate(uint32_t category);
const SandboxFlagInfo *GetSandboxFlagInfo(const char *key, const SandboxFlagInfo *flagsInfos, uint32_t count);
int GetPathMode(const char *name);

void DumpMountPathMountNode(const PathMountNode *pathNode);

typedef struct {
    const char *originPath;
    const char *destinationPath;
    const char *fsType;
    unsigned long mountFlags;
    const char *options;
    mode_t mountSharedFlag;
} MountArg;

int SandboxMountPath(const MountArg *arg);

__attribute__((always_inline)) inline int IsPathEmpty(const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return 1;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_SANDBOX_H
