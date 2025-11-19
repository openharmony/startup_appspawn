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
    return 0;
}

int VarPackageNameReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}

int VarCurrentUseIdReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}

int VarCurrentHostUserIdReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}

int VarArkWebPackageNameReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen,
    const VarExtraData *extraData)
{
    return 0;
}

int VariableNodeCompareName(ListNode *node, void *data)
{
    return 0;
}

AppSandboxVarNode *GetAppSandboxVarNode(const char *name)
{
    return nullptr;
}

int ReplaceVariableByParameter(const char *varData, SandboxBuffer *sandboxBuffer)
{
    return 0;
}

int ReplaceVariableForDepSandboxPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}

int ReplaceVariableForDepSrcPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}

int ReplaceVariableForDepPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}

int ReplaceVariableForpackageName(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}

int GetVariableName(char *varData, uint32_t len, const char *varStart, uint32_t *varLen)
{
    return 0;
}

int ReplaceVariable(const SandboxContext *context,
    const char *varStart, SandboxBuffer *sandboxBuffer, uint32_t *varLen, const VarExtraData *extraData)
{
    return 0;
}

int HandleVariableReplace(const SandboxContext *context,
    SandboxBuffer *sandboxBuffer, const char *source, const VarExtraData *extraData)
{
    return 0;
}

char *GetSandboxRealVar(const SandboxContext *context, uint32_t bufferType, const char *source,
    const char *prefix, const VarExtraData *extraData)
{
    return nullptr;
}

int AddVariableReplaceHandler(const char *name, ReplaceVarHandler handler)
{
    return 0;
}

void AddDefaultVariable(void)
{
    printf("AddDefaultVariable");
}

#ifdef __cplusplus
}
#endif
