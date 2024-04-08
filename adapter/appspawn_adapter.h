/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef APPSPAWN_ADPATER_CPP
#define APPSPAWN_ADPATER_CPP

#include "appspawn_server.h"

#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AppSpawnContentExt AppSpawnContentExt;
typedef struct AppInfo AppSpawnAppInfo;

int32_t SetAppSandboxProperty(struct AppSpawnContent *content, AppSpawnClient *client);
int SetAppAccessToken(struct AppSpawnContent *content, AppSpawnClient *client);
int SetSelinuxCon(struct AppSpawnContent *content, AppSpawnClient *client);
void LoadExtendLib(AppSpawnContent *content);
void LoadExtendLibNweb(AppSpawnContent *content);
void RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client);
void RunChildProcessorNweb(AppSpawnContent *content, AppSpawnClient *client);
void LoadAppSandboxConfig(AppSpawnContent *content);
void SetUidGidFilter(struct AppSpawnContent *content);
int SetSeccompFilter(struct AppSpawnContent *content, AppSpawnClient *client);
void HandleInternetPermission(const AppSpawnClient *client);

void DisallowInternet(void);

void RecordRenderProcessExitedStatus(pid_t pid, int status);
int GetProcessTerminationStatus(AppSpawnClient *client);

int ProcessAppDied(const AppSpawnContentExt *content, const AppSpawnAppInfo *appInfo);
int ProcessAppAdd(const AppSpawnContentExt *content, const AppSpawnAppInfo *appInfo);

#ifdef __cplusplus
}
#endif
#endif
