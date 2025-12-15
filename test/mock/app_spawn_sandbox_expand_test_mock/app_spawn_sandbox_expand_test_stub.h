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
int MountAllHsp(const SandboxContext *context, const cJSON *hsps);
int MountAllGroup(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox,
    const cJSON *groups);
int SetOverlayAppPath(const char *hapPath, void *context);
int SetOverlayAppSandboxConfig(const SandboxContext *context, const char *overlayInfo);
cJSON *GetJsonObjFromProperty(const SandboxContext *context, const char *name);
int ProcessHSPListConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name);
int ProcessDataGroupConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name);
int ProcessOverlayAppConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandbox, const char *name);
int AppSandboxExpandAppCfgCompareName(ListNode *node, void *data);
int AppSandboxExpandAppCfgComparePrio(ListNode *node1, ListNode *node2);
AppSandboxExpandAppCfgNode *GetAppSandboxExpandAppCfg(const char *name);
int RegisterExpandSandboxCfgHandler(const char *name, int prio, ProcessExpandSandboxCfg handleExpandCfg);
void AddDefaultExpandAppSandboxConfigHandle(void);
void ClearExpandAppSandboxConfigHandle(void);
#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
