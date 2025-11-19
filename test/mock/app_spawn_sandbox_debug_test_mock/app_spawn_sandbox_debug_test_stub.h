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

int InitDebugSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
    const AppSpawningCtx *property, int nwebspawn);
void UmountAndRmdirDir(const char *targetPath);
int RemoveDebugBaseConfig(SandboxSection *section, const char *debugRootPath);
int RemoveDebugAppVarConfig(const AppSpawnSandboxCfg *sandboxCfg, const char *debugRootPath);
int RemoveDebugPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandboxCfg,
    const char *debugRootPath);
int ConvertUserIdPath(const AppSpawningCtx *property, char *debugRootPath, char *debugTmpRootPath);
int UninstallPrivateDirs(const AppSpawnMgr *content, const AppSpawningCtx *property,
    RemoveDebugDirInfo *removeDebugDirInfo);
int UninstallAllDirs(const AppSpawnMgr *content, const AppSpawningCtx *property,
    RemoveDebugDirInfo *removeDebugDirInfo);
int UninstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property);
int SetDebugAppVarConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int SetDebugPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int SetDebugAutomicTmpRootPath(SandboxContext *context, const AppSpawningCtx *property);
int InitDebugSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
    const AppSpawningCtx *property, int nwebspawn);
int MountDebugTmpConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int MountDebugDirBySharefs(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox);
int InstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
