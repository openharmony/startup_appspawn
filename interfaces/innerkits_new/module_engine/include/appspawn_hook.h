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

#ifndef APPSPAWN_HOOK_H
#define APPSPAWN_HOOK_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_msg.h"
#include "hookmgr.h"
#include "list.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct TagAppSpawnMgr AppSpawnMgr;
typedef struct TagAppSpawningCtx AppSpawningCtx;
typedef struct AppSpawnContent AppSpawnContent;
typedef struct AppSpawnClient AppSpawnClient;
typedef struct TagAppSpawnedProcess AppSpawnedProcess;

typedef enum {
    EXT_DATA_SANDBOX
} ExtDataType;

struct TagAppSpawnExtData;
typedef void (*AppSpawnExtDataFree)(struct TagAppSpawnExtData *data);
typedef void (*AppSpawnExtDataDump)(struct TagAppSpawnExtData *data);
typedef struct TagAppSpawnExtData {
    ListNode node;
    uint32_t dataId;
    AppSpawnExtDataFree freeNode;
    AppSpawnExtDataDump dumpNode;
} AppSpawnExtData;

typedef enum TagAppSpawnHookStage {
    // run in init
    HOOK_PRELOAD  = 10,
    // run before fork
    HOOK_SPAWN_PREPARE = 20,
    // run in child process
    HOOK_SPAWN_CLEAR_ENV = 30, // clear env, set token HOOK_SPAWN_CLEAR_ENV
    HOOK_SPAWN_SET_CHILD_PROPERTY,
    HOOK_SPAWN_COMPLETED,
    HOOK_SPAWN_POST = 40,
    // for app change
    HOOK_APP_ADD = 50,
    HOOK_APP_DIED,
} AppSpawnHookStage;

typedef int (*PreloadHook)(AppSpawnMgr *content);
typedef int (*AppSpawnHook)(AppSpawnMgr *content, AppSpawningCtx *property);
typedef int (*ProcessChangeHook)(const AppSpawnMgr *content, const AppSpawnedProcess *appInfo);
int AddPreloadHook(int prio, PreloadHook hook);
int AddAppSpawnHook(AppSpawnHookStage stage, int prio, AppSpawnHook hook);
int AddAppChangeHook(AppSpawnHookStage stage, int prio, ProcessChangeHook hook);

int IsNWebSpawnMode(const AppSpawnMgr *content);
int IsColdRunMode(const AppSpawnMgr *content);

int GetAppSpawnMsgType(const AppSpawningCtx *property);
const char *GetBundleName(const AppSpawningCtx *property);
void *GetAppProperty(const AppSpawningCtx *property, uint32_t type);
const char *GetProcessName(const AppSpawningCtx *property);

/**
 * @brief Get the App Property Ex info
 *
 * @param property app 属性信息
 * @param name 变量名
 * @param len 返回变量长度
 * @return uint8_t* 返回变量值
 */
uint8_t *GetAppPropertyExt(const AppSpawningCtx *property, const char *name, uint32_t *len);

/**
 * @brief 检查app属性参数的flags是否设置
 *
 * @param property app 属性信息
 * @param type TLV_MSG_FLAGS or TLV_PERMISSION
 * @param index flags index
 * @return int
 */
int CheckAppPropertyFlags(const AppSpawningCtx *property, uint32_t type, uint32_t index);
int SetAppPropertyFlags(const AppSpawningCtx *property, uint32_t type, uint32_t index);

__attribute__((always_inline)) inline int TestAppMsgFlagsSet(const AppSpawningCtx *property, uint32_t index)
{
    return CheckAppPropertyFlags(property, TLV_MSG_FLAGS, index);
}
__attribute__((always_inline)) inline int TestAppPermissionFlags(const AppSpawningCtx *property, uint32_t index)
{
    return CheckAppPropertyFlags(property, TLV_PERMISSION, index);
}
__attribute__((always_inline)) inline int SetAppPermissionFlags(const AppSpawningCtx *property, uint32_t index)
{
    return SetAppPropertyFlags(property, TLV_PERMISSION, index);
}

typedef int (*ChildLoop)(AppSpawnContent *content, AppSpawnClient *client);
/**
 * @brief 注册子进程run函数
 *
 * @param content
 * @param loop
 */
void RegChildLooper(AppSpawnContent *content, ChildLoop loop);

/**
 * @brief 按mode创建文件件
 *
 * @param path 路径
 * @param mode mode
 * @param lastPath 是否文件名
 * @return int 结果
 */
int MakeDirRec(const char *path, mode_t mode, int lastPath);
__attribute__((always_inline)) inline int MakeDirRecursive(const char *path, mode_t mode)
{
    return MakeDirRec(path, mode, 1);
}

typedef struct {
    const char *originPath;
    const char *destinationPath;
    const char *fsType;
    unsigned long mountFlags;
    const char *options;
    mode_t mountSharedFlag;
} MountArg;

int SandboxMountPath(const MountArg *arg);
int IsDeveloperModeOn(const AppSpawningCtx *property);

// 扩展变量
typedef struct TagSandboxContext SandboxContext;
typedef int (*ReplaceVarHandler)(const SandboxContext *context,
    const uint8_t *buffer, uint32_t bufferLen, uint32_t *realLen, int permission);
/**
 * @brief 注册变量替换处理函数
 *
 * @param name 变量名
 * @param handler 处理函数
 * @return int
 */
int AddVariableReplaceHandler(const char *name, ReplaceVarHandler handler);

typedef struct TagAppSpawnSandbox AppSpawnSandbox;
typedef int (*ProcessExpandSandboxCfg)(const SandboxContext *context,
    const AppSpawnSandbox *appSandBox, const char *name);
#define EXPAND_CFG_HANDLER_PRIO_START 3

/**
 * @brief 注册扩展属性处理函数
 *
 * @param name 扩展变量名
 * @param handleExpandCfg  处理函数
 * @return int
 */
int RegisterExpandSandboxCfgHandler(const char *name, int prio, ProcessExpandSandboxCfg handleExpandCfg);

#ifndef MODULE_DESTRUCTOR
#define MODULE_CONSTRUCTOR(void) static void _init(void) __attribute__((constructor)); static void _init(void)
#define MODULE_DESTRUCTOR(void) static void _destroy(void) __attribute__((destructor)); static void _destroy(void)
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
