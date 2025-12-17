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

#include "app_spawn_stub.h"

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdbool>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>


#ifdef __cplusplus
extern "C" {
#endif

int VarPackageNameIndexReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("VarPackageNameIndexReplace context is null");
    }

    if (buffer == nullptr) {
        printf("VarPackageNameIndexReplace buffer is null");
    }

    if (bufferLen == 0) {
        printf("VarPackageNameIndexReplace bufferLen is 0");
    }

    if (realLen == nullptr) {
        printf("VarPackageNameIndexReplace realLen is null");
    }

    if (extraData == nullptr) {
        printf("VarPackageNameIndexReplace extraData is null");
    }

    return 0;
}

int VarPackageNameReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("VarPackageNameReplace context is null");
    }

    if (buffer == nullptr) {
        printf("VarPackageNameReplace buffer is null");
    }

    if (bufferLen == 0) {
        printf("VarPackageNameReplace bufferLen is 0");
    }

    if (realLen == nullptr) {
        printf("VarPackageNameReplace realLen is null");
    }

    if (extraData == nullptr) {
        printf("VarPackageNameReplace extraData is null");
    }

    return 0;
}

int VarCurrentUseIdReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("VarCurrentUseIdReplace context is null");
    }

    if (buffer == nullptr) {
        printf("VarCurrentUseIdReplace buffer is null");
    }

    if (bufferLen == 0) {
        printf("VarCurrentUseIdReplace bufferLen is 0");
    }

    if (realLen == nullptr) {
        printf("VarCurrentUseIdReplace realLen is null");
    }

    if (extraData == nullptr) {
        printf("VarCurrentUseIdReplace extraData is null");
    }

    return 0;
}

int VarCurrentHostUserIdReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("VarCurrentHostUserIdReplace context is null");
    }

    if (buffer == nullptr) {
        printf("VarCurrentHostUserIdReplace buffer is null");
    }

    if (bufferLen == 0) {
        printf("VarCurrentHostUserIdReplace bufferLen is 0");
    }

    if (realLen == nullptr) {
        printf("VarCurrentHostUserIdReplace realLen is null");
    }

    if (extraData == nullptr) {
        printf("VarCurrentHostUserIdReplace extraData is null");
    }

    return 0;
}

int VarArkWebPackageNameReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen,
    const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("VarArkWebPackageNameReplace context is null");
    }

    if (buffer == nullptr) {
        printf("VarArkWebPackageNameReplace buffer is null");
    }

    if (bufferLen == 0) {
        printf("VarArkWebPackageNameReplace bufferLen is 0");
    }

    if (realLen == nullptr) {
        printf("VarArkWebPackageNameReplace realLen is null");
    }

    if (extraData == nullptr) {
        printf("VarArkWebPackageNameReplace extraData is null");
    }

    return 0;
}

int VariableNodeCompareName(ListNode *node, void *data)
{
    if (node == nullptr) {
        printf("VariableNodeCompareName node is null");
    }

    if (data == nullptr) {
        printf("VariableNodeCompareName data is null");
    }

    return 0;
}

AppSandboxVarNode *GetAppSandboxVarNode(const char *name)
{
    if (name == nullptr) {
        printf("GetAppSandboxVarNode  name is null");
    }

    return nullptr;
}

int ReplaceVariableByParameter(const char *varData, SandboxBuffer *sandboxBuffer)
{
    if (varData == nullptr) {
        printf("ReplaceVariableByParameter varData is null");
    }

    if (sandboxBuffer == nullptr) {
        printf("ReplaceVariableByParameter sandboxBuffer is null");
    }

    return 0;
}

int ReplaceVariableForDepSandboxPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("ReplaceVariableForDepSandboxPath context is null");
    }

    if (buffer == nullptr) {
        printf("ReplaceVariableForDepSandboxPath buffer is null");
    }

    if (bufferLen == 0) {
        printf("ReplaceVariableForDepSandboxPath bufferLen is 0");
    }

    if (realLen == nullptr) {
        printf("ReplaceVariableForDepSandboxPath realLen is null");
    }

    if (extraData == nullptr) {
        printf("ReplaceVariableForDepSandboxPath extraData is null");
    }

    return 0;
}

int ReplaceVariableForDepSrcPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("ReplaceVariableForDepSrcPath context is null");
    }

    if (buffer == nullptr) {
        printf("ReplaceVariableForDepSrcPath buffer is null");
    }

    if (bufferLen == 0) {
        printf("ReplaceVariableForDepSrcPath bufferLen is 0");
    }

    if (realLen == nullptr) {
        printf("ReplaceVariableForDepSrcPath realLen is null");
    }

    if (extraData == nullptr) {
        printf("ReplaceVariableForDepSrcPath extraData is nul");
    }

    return 0;
}

int ReplaceVariableForDepPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("ReplaceVariableForDepPath context is null");
    }

    if (buffer == nullptr) {
        printf("ReplaceVariableForDepPath buffer is null");
    }

    if (bufferLen == 0) {
        printf("ReplaceVariableForDepPath bufferLen is 0");
    }

    if (realLen == nullptr) {
        printf("ReplaceVariableForDepPath realLen is null");
    }

    if (extraData == nullptr) {
        printf("ReplaceVariableForDepPath extraData is null");
    }

    return 0;
}

int ReplaceVariableForpackageName(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("ReplaceVariableForpackageName context is null");
    }

    if (buffer == nullptr) {
        printf("ReplaceVariableForpackageName buffer is null");
    }

    if (bufferLen == 0) {
        printf("ReplaceVariableForpackageName bufferLen is null");
    }

    if (realLen == nullptr) {
        printf("ReplaceVariableForpackageName realLen is null");
    }

    if (extraData == nullptr) {
        printf("ReplaceVariableForpackageName extraData is null");
    }

    return 0;
}

int GetVariableName(char *varData, uint32_t len, const char *varStart, uint32_t *varLen)
{
    if (varData == nullptr) {
        printf("GetVariableName varData is null");
    }

    if (len == 0) {
        printf("GetVariableName len is 0");
    }

    if (varStart == nullptr) {
        printf("GetVariableName varStart is null");
    }

    if (varLen == nullptr) {
        printf("GetVariableName varLen is null");
    }

    return 0;
}

int ReplaceVariable(const SandboxContext *context,
    const char *varStart, SandboxBuffer *sandboxBuffer, uint32_t *varLen, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("ReplaceVariable context is null");
    }

    if (varStart == nullptr) {
        printf("ReplaceVariable varStart is null");
    }

    if (sandboxBuffer == nullptr) {
        printf("ReplaceVariable sandboxBuffer is null");
    }

    if (varLen == nullptr) {
        printf("ReplaceVariable varLen is null");
    }

    if (extraData == nullptr) {
        printf("ReplaceVariable extraData is null");
    }

    return 0;
}

int HandleVariableReplace(const SandboxContext *context,
    SandboxBuffer *sandboxBuffer, const char *source, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("HandleVariableReplace context is null");
    }

    if (sandboxBuffer == nullptr) {
        printf("HandleVariableReplace sandboxBuffer is null");
    }

    if (source == nullptr) {
        printf("HandleVariableReplace source is null");
    }

    if (extraData == nullptr) {
        printf("HandleVariableReplace extraData is null");
    }

    return 0;
}

char *GetSandboxRealVar(const SandboxContext *context, uint32_t bufferType, const char *source,
    const char *prefix, const VarExtraData *extraData)
{
    if (context == nullptr) {
        printf("GetSandboxRealVar context is null");
    }

    if (bufferType == 0) {
        printf("GetSandboxRealVar bufferType is 0");
    }

    if (source == nullptr) {
        printf("GetSandboxRealVar source is null");
    }

    if (prefix == nullptr) {
        printf("GetSandboxRealVar prefix is null");
    }

    if (extraData == nullptr) {
        printf("GetSandboxRealVar extraData is null");
    }

    return nullptr;
}

int AddVariableReplaceHandler(const char *name, ReplaceVarHandler handler)
{
    if (name == nullptr) {
        printf("AddVariableReplaceHandler name is null");
    }

    if (handler == nullptr) {
        printf("AddVariableReplaceHandler handler is null");
    }

    return 0;
}

void AddDefaultVariable(void)
{
    printf("AddDefaultVariable");
}

#ifdef __cplusplus
}
#endif
