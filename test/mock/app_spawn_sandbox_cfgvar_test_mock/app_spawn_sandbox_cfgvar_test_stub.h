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
int VarPackageNameIndexReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int VarPackageNameReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int VarCurrentUseIdReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int VarCurrentHostUserIdReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int VarArkWebPackageNameReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen,
    const VarExtraData *extraData);
int VariableNodeCompareName(ListNode *node, void *data);
AppSandboxVarNode *GetAppSandboxVarNode(const char *name);
int ReplaceVariableByParameter(const char *varData, SandboxBuffer *sandboxBuffer);
int ReplaceVariableForDepSandboxPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int ReplaceVariableForDepSrcPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int ReplaceVariableForDepPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int ReplaceVariableForpackageName(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData);
int GetVariableName(char *varData, uint32_t len, const char *varStart, uint32_t *varLen);
int ReplaceVariable(const SandboxContext *context,
    const char *varStart, SandboxBuffer *sandboxBuffer, uint32_t *varLen, const VarExtraData *extraData);
int HandleVariableReplace(const SandboxContext *context,
    SandboxBuffer *sandboxBuffer, const char *source, const VarExtraData *extraData);
char *GetSandboxRealVar(const SandboxContext *context, uint32_t bufferType, const char *source,
    const char *prefix, const VarExtraData *extraData);
int AddVariableReplaceHandler(const char *name, ReplaceVarHandler handler);
void AddDefaultVariable(void);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
