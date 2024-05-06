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

#ifndef APPSPAWN_MODULE_MGR_H
#define APPSPAWN_MODULE_MGR_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "hookmgr.h"
#include "modulemgr.h"
#include "appspawn_hook.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HOOK_STOP_WHEN_ERROR 0x2
#if defined(__aarch64__) || defined(__x86_64__)
#define ASAN_MODULE_PATH "/system/lib64/appspawn/libappspawn_asan"
#else
#define ASAN_MODULE_PATH "/system/lib/appspawn/libappspawn_asan"
#endif

typedef enum {
    MODULE_DEFAULT,
    MODULE_APPSPAWN,
    MODULE_NWEBSPAWN,
    MODULE_COMMON,
    MODULE_MAX
} AppSpawnModuleType;

typedef struct {
    AppSpawnContent *content;
    AppSpawnClient *client;
    struct timespec tmStart;
    struct timespec tmEnd;
} AppSpawnHookArg;

int AppSpawnModuleMgrInstall(const char *moduleName);
int AppSpawnLoadAutoRunModules(int type);
void AppSpawnModuleMgrUnInstall(int type);
void DeleteAppSpawnHookMgr(void);
int ServerStageHookExecute(AppSpawnHookStage stage, AppSpawnContent *content);
int ProcessMgrHookExecute(AppSpawnHookStage stage, const AppSpawnContent *content, const AppSpawnedProcessInfo *appInfo);
int AppSpawnHookExecute(AppSpawnHookStage stage, uint32_t flags, AppSpawnContent *content, AppSpawnClient *client);

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_MODULE_MGR_H
