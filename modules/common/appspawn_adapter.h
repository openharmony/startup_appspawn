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

#include "appspawn_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

int SetAppAccessToken(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetSelinuxCon(const AppSpawnMgr *content, const AppSpawningCtx *property);

int SetUidGidFilter(const AppSpawnMgr *content);
int SetSeccompFilter(const AppSpawnMgr *content, const AppSpawningCtx *property);
int SetInternetPermission(const AppSpawningCtx *property);
int32_t SetEnvInfo(const AppSpawnMgr *content, const AppSpawningCtx *property);
int32_t LoadSeLinuxConfig(void);

// stub for extend func
void DisallowInternet(void);
void DumpSpawnStack(pid_t pid);

// Process kill reason reporting
#define REASON_APPSPAWN_STOP 3053
#define REASON_KILL_CGROUP 8
#define DEV_SYSLOAD "/dev/sysload"

void SetKillReason(const AppSpawnMgr *mgr, pid_t pid, uid_t uid, int reason);

#ifdef __cplusplus
}
#endif
#endif
