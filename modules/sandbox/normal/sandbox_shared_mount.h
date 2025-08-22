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

#ifndef SANDBOX_SHARED_MOUNT_H
#define SANDBOX_SHARED_MOUNT_H

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "list.h"
#include "json_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MountSharedTemplate {
    const char *sandboxPath;
    const char *permission;
} MountSharedTemplate;

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

struct SharedMountArgs {
    const char *srcPath;
    const char *destPath;
    const char *fsType = "";
    unsigned long mountFlags = MS_REC | MS_BIND;
    const char *options = "";
    mode_t mountSharedFlag = MS_SLAVE;
};

bool IsValidDataGroupItem(cJSON *item);
int GetElxInfoFromDir(const char *path);
const DataGroupSandboxPathTemplate *GetDataGroupArgTemplate(uint32_t category);
int MountToShared(AppSpawnMgr *content, const AppSpawningCtx *property);

#ifdef __cplusplus
}
#endif
#endif  // SANDBOX_SHARED_MOUNT_H
