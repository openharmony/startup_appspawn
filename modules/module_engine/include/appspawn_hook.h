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
typedef struct TagAppSpawnedProcess AppSpawnedProcessInfo;

typedef enum {
    EXT_DATA_SANDBOX,
    EXT_DATA_NAMESPACE,
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
    // 服务状态处理
    STAGE_SERVER_PRELOAD  = 10,
    STAGE_SERVER_EXIT,
    // 应用状态处理
    STAGE_SERVER_APP_ADD,
    STAGE_SERVER_APP_DIED,
    // run before fork
    STAGE_PARENT_PRE_FORK = 20,
    STAGE_PARENT_POST_FORK = 21,
    STAGE_PARENT_PRE_RELY = 22,
    STAGE_PARENT_POST_RELY = 23,

    // run in child process
    STAGE_CHILD_PRE_COLDBOOT = 30, // clear env, set token before cold boot
    STAGE_CHILD_EXECUTE,
    STAGE_CHILD_PRE_RELY,
    STAGE_CHILD_POST_RELY,
    STAGE_CHILD_PRE_RUN,
    STAGE_MAX
} AppSpawnHookStage;

typedef enum TagAppSpawnHookPrio {
    HOOK_PRIO_HIGHEST = 1000,
    HOOK_PRIO_COMMON = 2000,
    HOOK_PRIO_SANDBOX = 3000,
    HOOK_PRIO_PROPERTY = 4000,
    HOOK_PRIO_LOWEST = 5000,
} AppSpawnHookPrio;

/**
 * @brief 预加载处理函数
 *
 * @param content appspawn appspawn管理数据
 * @return int
 */
typedef int (*ServerStageHook)(AppSpawnMgr *content);

/**
 * @brief 应用孵化各阶段注册函数
 *
 * @param content appspawn appspawn管理数据
 * @param property 业务孵化数据
 * @return int
 */
typedef int (*AppSpawnHook)(AppSpawnMgr *content, AppSpawningCtx *property);

/**
 * @brief 业务进程变化注册函数
 *
 * @param content appspawn appspawn管理数据
 * @param appInfo 业务进程信息
 * @return int
 */
typedef int (*ProcessChangeHook)(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo);

/**
 * @brief 添加服务阶段的处理函数
 *
 * @param stage 阶段信息
 * @param prio 优先级
 * @param hook 预加载处理函数
 * @return int
 */
int AddServerStageHook(AppSpawnHookStage stage, int prio, ServerStageHook hook);

/**
 * @brief 添加预加载处理函数
 *
 * @param prio 优先级
 * @param hook 预加载处理函数
 * @return int
 */
__attribute__((always_inline)) inline int AddPreloadHook(int prio, ServerStageHook hook)
{
    return AddServerStageHook(STAGE_SERVER_PRELOAD, prio, hook);
}

/**
 * @brief 按阶段添加应用孵化处理函数
 *
 * @param stage 阶段信息
 * @param prio 优先级
 * @param hook 应用孵化阶段处理函数
 * @return int
 */
int AddAppSpawnHook(AppSpawnHookStage stage, int prio, AppSpawnHook hook);

/**
 * @brief 添加业务进程处理函数
 *
 * @param stage 阶段信息
 * @param prio 优先级
 * @param hook 业务进程变化处理函数
 * @return int
 */
int AddProcessMgrHook(AppSpawnHookStage stage, int prio, ProcessChangeHook hook);

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
__attribute__((always_inline)) inline int CreateSandboxDir(const char *path, mode_t mode)
{
    return MakeDirRec(path, mode, 1);
}

// 扩展变量
typedef struct TagSandboxContext SandboxContext;
typedef struct TagVarExtraData VarExtraData;
typedef int (*ReplaceVarHandler)(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
/**
 * @brief 注册变量替换处理函数
 *
 * @param name 变量名
 * @param handler 处理函数
 * @return int
 */
int AddVariableReplaceHandler(const char *name, ReplaceVarHandler handler);

typedef struct TagAppSpawnSandboxCfg AppSpawnSandboxCfg;
typedef int (*ProcessExpandSandboxCfg)(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandBox, const char *name);
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
