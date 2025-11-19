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
int SetProcessName(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetKeepCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetAmbientCapability(int cap);
int SetCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property);
void InitDebugParams(const AppSpawnMgr *content, const AppSpawningCtx *property);
void ClearEnvironment(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetXpmConfig(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetUidGid(const AppSpawnMgr *content, const AppSpawningCtx *property);
int32_t SetFileDescriptors(const AppSpawnMgr *content, const AppSpawningCtx *property);
int32_t CheckTraceStatus(void);
int32_t WaitForDebugger(const AppSpawningCtx *property);
int SpawnSetPreInstalledFlag(AppSpawningCtx *property);
int SpawnInitSpawningEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int SpawnSetAppEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int SpawnEnableCache(AppSpawnMgr *content, AppSpawningCtx *property);
int FilterAppSpawnTrace(AppSpawnMgr *content, AppSpawningCtx *property);
int SpawnSetProperties(AppSpawnMgr *content, AppSpawningCtx *property);
int PreLoadSetSeccompFilter(AppSpawnMgr *content);
int CheckEnabled(const char *param, const char *value);
int SpawnGetSpawningFlag(AppSpawnMgr *content, AppSpawningCtx *property);
int SpawnLoadConfig(AppSpawnMgr *content);
int SpawnLoadSeLinuxConfig(AppSpawnMgr *content);
int CloseFdArgs(AppSpawnMgr *content, AppSpawningCtx *property);
int SetFdEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int RecordStartTime(AppSpawnMgr *content, AppSpawningCtx *property);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
