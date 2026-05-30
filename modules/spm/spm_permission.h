/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
// Permission management interfaces
#ifndef APPSAPWN_SPM_PERMISSION_H
#define APPSAPWN_SPM_PERMISSION_H
#include "perm_setproc_c.h"
#include "appspawn_mount_permission.h"

#ifdef __cplusplus
extern "C" {
#endif

const SandboxPermissionNode *GetPermissionNodeByOpcode(const SandboxQueue *queue, uint32_t opcode);

const char *GetPermissionNameByOpcode(const SandboxQueue *queue, uint32_t opcode);

int32_t GetSpawnFlagIndexesFromPermissionBitmap(const SandboxQueue *queue,
    const uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint32_t *flagIndexes, uint32_t flagCount);

#ifdef __cplusplus
}
#endif

#endif