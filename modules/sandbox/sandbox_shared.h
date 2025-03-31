/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SANDBOX_SHARED_H
#define SANDBOX_SHARED_H

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "json_utils.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    EL2 = 0,
    EL3,
    EL4,
    EL5,
    ELX_MAX
};

typedef struct DataGroupSandboxPathTemplate {
    const char *elxName;
    uint32_t category;
    const char *sandboxPath;
    const char *permission;
} DataGroupSandboxPathTemplate;

/**
 * @brief DataGroup expand info
 */
bool IsValidDataGroupItem(cJSON *item);
int GetElxInfoFromDir(const char *path);
const DataGroupSandboxPathTemplate *GetDataGroupArgTemplate(uint32_t category);

/**
 * @brief Mount the dirs as shared before device unlocking
 */
int MountDirsToShared(AppSpawnMgr *content, SandboxContext *context, AppSpawnSandboxCfg *sandbox);

#ifdef __cplusplus
}
#endif
#endif  // SANDBOX_SHARED_H
