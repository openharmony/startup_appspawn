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

#ifndef APPSPAWN_SANDBOX_PERMISSION_H
#define APPSPAWN_SANDBOX_PERMISSION_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_SANDBOX_FILE_NAME "/appdata-sandbox.json"
#define WEB_SANDBOX_FILE_NAME "/appdata-sandbox-nweb.json"

typedef struct TagSandboxQueue SandboxQueue;
typedef struct TagPermissionNode SandboxPermissionNode;

int32_t AddSandboxPermissionNode(const char *name, SandboxQueue *queue);
int32_t DeleteSandboxPermissions(SandboxQueue *queue);
int32_t GetPermissionIndexInQueue(SandboxQueue *queue, const char *permission);
const SandboxPermissionNode *GetPermissionNodeInQueue(SandboxQueue *queue, const char *permission);
const SandboxPermissionNode *GetPermissionNodeInQueueByIndex(SandboxQueue *queue, int32_t index);
int32_t PermissionRenumber(SandboxQueue *queue);

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_SANDBOX_PERMISSION_H
