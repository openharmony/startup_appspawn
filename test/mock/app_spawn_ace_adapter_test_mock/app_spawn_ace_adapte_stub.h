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

void GetNames(const cJSON *root, std::set<std::string> &names, std::string key);
int GetNameSet(const cJSON *root, ParseJsonContext *context);
void PreloadModule(bool isHybrid);
void LoadExtendLib(bool isHybrid);
void PreloadCJLibs(void);
void LoadExtendCJLib(void);
int BuildFdInfoMap(const AppSpawnMsgNode *message, std::map<std::string, int> &fdMap, int isColdRun);
void ClearEnvAndReturnSuccess(AppSpawnContent *content, AppSpawnClient *client);
int RunChildThread(const AppSpawnMgr *content, const AppSpawningCtx *property);
int RunChildByRenderCmd(const AppSpawnMgr *content, const AppSpawningCtx *property);
int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client);
int PreLoadAppSpawn(AppSpawnMgr *content);
int DoDlopenLibs(const cJSON *root, ParseJsonContext *context);
int DlopenAppSpawn(AppSpawnMgr *content);
int ProcessSpawnDlopenMsg(AppSpawnMgr *content);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
