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

#include "appspawn_manager.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "modulemgr.h"
#include "parameter.h"
#include "securec.h"

struct ListNode g_sandboxVarList = {&g_sandboxVarList, &g_sandboxVarList};

static int VarPackageNameIndexReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    AppSpawnMsgBundleInfo *bundleInfo = (
        AppSpawnMsgBundleInfo *)GetSpawningMsgInfo(context, TLV_BUNDLE_INFO);
    APPSPAWN_CHECK(bundleInfo != NULL, return APPSPAWN_TLV_NONE,
        "No bundle info in msg %{public}s", context->bundleName);
    int len = 0;
    if (bundleInfo->bundleIndex > 0) {
        len = sprintf_s((char *)buffer, bufferLen, "%s_%d", bundleInfo->bundleName, bundleInfo->bundleIndex);
    } else {
        len = sprintf_s((char *)buffer, bufferLen, "%s", bundleInfo->bundleName);
    }
    APPSPAWN_CHECK(len > 0 && ((uint32_t)len < bufferLen),
        return -1, "Failed to format path app: %{public}s", context->bundleName);
    *realLen = (uint32_t)len;
    return 0;
}

static int VarPackageNameReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    int len = sprintf_s((char *)buffer, bufferLen, "%s", context->bundleName);
    APPSPAWN_CHECK(len > 0 && ((uint32_t)len < bufferLen),
        return -1, "Failed to format path app: %{public}s", context->bundleName);
    *realLen = (uint32_t)len;
    return 0;
}

static int VarCurrentUseIdReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSpawningMsgInfo(context, TLV_DAC_INFO);
    APPSPAWN_CHECK(info != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, context->bundleName);
    int len = 0;
    if (extraData == NULL || !CHECK_FLAGS_BY_INDEX(extraData->operation, SANDBOX_TAG_PERMISSION)) {
        len = sprintf_s((char *)buffer, bufferLen, "%u", info->uid / UID_BASE);
    } else if (context->appFullMountEnable && strlen(info->userName) > 0) {
        len = sprintf_s((char *)buffer, bufferLen, "%s", "currentUser");
    } else {
        len = sprintf_s((char *)buffer, bufferLen, "%s", "currentUser");
    }
    APPSPAWN_CHECK(len > 0 && ((uint32_t)len < bufferLen),
        return -1, "Failed to format path app: %{public}s", context->bundleName);
    *realLen = (uint32_t)len;
    return 0;
}

static int VariableNodeCompareName(ListNode *node, void *data)
{
    AppSandboxVarNode *varNode = (AppSandboxVarNode *)ListEntry(node, AppSandboxVarNode, node);
    return strcmp((char *)data, varNode->name);
}

static AppSandboxVarNode *GetAppSandboxVarNode(const char *name)
{
    ListNode *node = OH_ListFind(&g_sandboxVarList, (void *)name, VariableNodeCompareName);
    if (node == NULL) {
        return NULL;
    }
    return (AppSandboxVarNode *)ListEntry(node, AppSandboxVarNode, node);
}

static int ReplaceVariableByParameter(const char *varData, SandboxBuffer *sandboxBuffer)
{
    // "<param:persist.nweb.sandbox.src_path>"
    int len = GetParameter(varData + sizeof("<param:") - 1,
        DEFAULT_NWEB_SANDBOX_SEC_PATH, sandboxBuffer->buffer + sandboxBuffer->current,
        sandboxBuffer->bufferLen - sandboxBuffer->current - 1);
    APPSPAWN_CHECK(len > 0, return -1, "Failed to get param for var %{public}s", varData);
    sandboxBuffer->current += len;
    return 0;
}

static int ReplaceVariableForDepSandboxPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    APPSPAWN_CHECK(extraData != NULL, return -1, "Invalid extra data ");
    uint32_t len = strlen(extraData->data.depNode->target);
    int ret = memcpy_s((char *)buffer, bufferLen, extraData->data.depNode->target, len);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to copy real data");
    *realLen = len;
    return 0;
}

static int ReplaceVariableForDepSrcPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    APPSPAWN_CHECK(extraData != NULL, return -1, "Invalid extra data ");
    uint32_t len = strlen(extraData->data.depNode->source);
    int ret = memcpy_s((char *)buffer, bufferLen, extraData->data.depNode->source, len);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to copy real data");
    *realLen = len;
    return 0;
}

static int ReplaceVariableForDepPath(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    APPSPAWN_CHECK(extraData != NULL, return -1, "Invalid extra data ");
    char *path = extraData->data.depNode->source;
    if (CHECK_FLAGS_BY_INDEX(extraData->operation, MOUNT_PATH_OP_REPLACE_BY_SANDBOX)) {
        path = extraData->data.depNode->target;
    } else if (CHECK_FLAGS_BY_INDEX(extraData->operation, MOUNT_PATH_OP_REPLACE_BY_SRC) && IsPathEmpty(path)) {
        path = extraData->data.depNode->target;
    }
    APPSPAWN_CHECK(path != NULL, return -1, "Invalid path %{public}x ", extraData->operation);
    uint32_t len = strlen(path);
    int ret = memcpy_s((char *)buffer, bufferLen, path, len);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to copy real data");
    *realLen = len;
    return 0;
}

static int GetVariableName(char *varData, uint32_t len, const char *varStart, uint32_t *varLen)
{
    uint32_t i = 0;
    uint32_t sourceLen = strlen(varStart);
    for (; i < sourceLen; i++) {
        if (i > len) {
            return -1;
        }
        varData[i] = *(varStart + i);
        if (varData[i] == '>') {
            break;
        }
    }
    varData[i + 1] = '\0';
    *varLen = i + 1;
    return 0;
}

static int ReplaceVariable(const SandboxContext *context,
    const char *varStart, SandboxBuffer *sandboxBuffer, uint32_t *varLen, const VarExtraData *extraData)
{
    char varName[128] = {0};  // 128 max len for var
    int ret = GetVariableName(varName, sizeof(varName), varStart, varLen);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get variable name");

    uint32_t valueLen = 0;
    AppSandboxVarNode *node = GetAppSandboxVarNode(varName);
    if (node != NULL) {
        ret = node->replaceVar(context, sandboxBuffer->buffer + sandboxBuffer->current,
            sandboxBuffer->bufferLen - sandboxBuffer->current - 1, &valueLen, extraData);
        APPSPAWN_CHECK(ret == 0 && valueLen < (sandboxBuffer->bufferLen - sandboxBuffer->current),
            return -1, "Failed to fill real data");
        sandboxBuffer->current += valueLen;
        return 0;
    }
    // "<param:persist.nweb.sandbox.src_path>"
    if (strncmp(varName, "<param:", sizeof("<param:") - 1) == 0) {  // retry param:
        varName[*varLen - 1] = '\0';                                // erase last >
        return ReplaceVariableByParameter(varName, sandboxBuffer);
    }
    if (strncmp(varName, "<lib>", sizeof("<lib>") - 1) == 0) {  // retry lib
        ret = memcpy_s(sandboxBuffer->buffer + sandboxBuffer->current,
            sandboxBuffer->bufferLen - sandboxBuffer->current, APPSPAWN_LIB_NAME, strlen(APPSPAWN_LIB_NAME));
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to copy real data");
        sandboxBuffer->current += strlen(APPSPAWN_LIB_NAME);
        return 0;
    }
    // no match revered origin data
    APPSPAWN_LOGE("ReplaceVariable var '%{public}s' no match variable", varName);
    ret = memcpy_s(sandboxBuffer->buffer + sandboxBuffer->current,
        sandboxBuffer->bufferLen - sandboxBuffer->current, varName, *varLen);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to copy real data");
    sandboxBuffer->current += *varLen;
    return 0;
}

static int HandleVariableReplace(const SandboxContext *context,
    SandboxBuffer *sandboxBuffer, const char *source, const VarExtraData *extraData)
{
    size_t sourceLen = strlen(source);
    for (size_t i = 0; i < sourceLen; i++) {
        if ((sandboxBuffer->current + 1) >= sandboxBuffer->bufferLen) {
            return -1;
        }
        if (*(source + i) != '<') {  // copy source
            *(sandboxBuffer->buffer + sandboxBuffer->current) = *(source + i);
            sandboxBuffer->current++;
            continue;
        }
        uint32_t varLen = 0;
        int ret = ReplaceVariable(context, source + i, sandboxBuffer, &varLen, extraData);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to fill real data");
        i += (varLen - 1);
    }
    return 0;
}

const char *GetSandboxRealVar(const SandboxContext *context,
    uint32_t bufferType, const char *source, const char *prefix, const VarExtraData *extraData)
{
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return NULL);
    APPSPAWN_CHECK(bufferType < ARRAY_LENGTH(context->buffer), return NULL, "Invalid index for buffer");
    SandboxBuffer *sandboxBuffer = &((SandboxContext *)context)->buffer[bufferType];
    APPSPAWN_CHECK_ONLY_EXPER(sandboxBuffer != NULL && sandboxBuffer->buffer != NULL, return NULL);
    const char *tmp = source;
    int ret = 0;
    if (!IsPathEmpty(prefix)) {  // copy prefix data
        ret = HandleVariableReplace(context, sandboxBuffer, prefix, extraData);
        APPSPAWN_CHECK(ret == 0, return NULL, "Failed to replace source %{public}s ", prefix);

        if (tmp != NULL && sandboxBuffer->buffer[sandboxBuffer->current - 1] == '/' && *tmp == '/') {
            tmp = source + 1;
        }
    }
    if (!IsPathEmpty(tmp)) {  // copy source data
        ret = HandleVariableReplace(context, sandboxBuffer, tmp, extraData);
        APPSPAWN_CHECK(ret == 0, return NULL, "Failed to replace source %{public}s ", source);
    }
    sandboxBuffer->buffer[sandboxBuffer->current] = '\0';
    // restore buffer
    sandboxBuffer->current = 0;

    // For the depNode scenario, if there are variables in the deps path, a secondary replacement is required
    if (extraData != NULL && extraData->sandboxTag == SANDBOX_TAG_NAME_GROUP && extraData->data.depNode != NULL) {
        if (strstr(sandboxBuffer->buffer, "<") != NULL) {
            SandboxBuffer *tmpBuffer = &((SandboxContext *)context)->buffer[BUFFER_FOR_TMP];
            ret = HandleVariableReplace(context, tmpBuffer, sandboxBuffer->buffer, extraData);
            APPSPAWN_CHECK(ret == 0, return NULL, "Failed to replace source %{public}s ", sandboxBuffer->buffer);
            tmpBuffer->buffer[tmpBuffer->current] = '\0';
            ret = strcpy_s(sandboxBuffer->buffer, sandboxBuffer->bufferLen, tmpBuffer->buffer);
            APPSPAWN_CHECK(ret == 0, return NULL, "Failed to copy source %{public}s ", sandboxBuffer->buffer);
        }
    }
    return sandboxBuffer->buffer;
}

int AddVariableReplaceHandler(const char *name, ReplaceVarHandler handler)
{
    APPSPAWN_CHECK(name != NULL && handler != NULL, return APPSPAWN_ARG_INVALID, "Invalid arg ");
    if (GetAppSandboxVarNode(name) != NULL) {
        return APPSPAWN_NODE_EXIST;
    }

    size_t len = APPSPAWN_ALIGN(strlen(name) + 1);
    AppSandboxVarNode *node = (AppSandboxVarNode *)malloc(sizeof(AppSandboxVarNode) + len);
    APPSPAWN_CHECK(node != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to create sandbox");
    OH_ListInit(&node->node);
    node->replaceVar = handler;
    int ret = strcpy_s(node->name, len, name);
    APPSPAWN_CHECK(ret == 0, free(node);
        return -1, "Failed to copy name %{public}s", name);
    OH_ListAddTail(&g_sandboxVarList, &node->node);
    return 0;
}

void AddDefaultVariable(void)
{
    AddVariableReplaceHandler(PARAMETER_PACKAGE_NAME, VarPackageNameReplace);
    AddVariableReplaceHandler(PARAMETER_USER_ID, VarCurrentUseIdReplace);
    AddVariableReplaceHandler(PARAMETER_PACKAGE_INDEX, VarPackageNameIndexReplace);
    /*
        deps-path路径变量的含义：
        1）首次挂载时，表示mount-paths-deps->sandbox-path  【STAGE_GLOBAL或者应用孵化时的挂载】
        使用 MOUNT_PATH_OP_REPLACE_BY_SANDBOX 标记
        2）二次挂载时，表示mount-paths-deps->src-path；
            如果mount-paths-deps->src-path为空，则使用mount-paths-deps->sandbox-path
        使用 MOUNT_PATH_OP_ONLY_SANDBOX + MOUNT_PATH_OP_REPLACE_BY_SRC，只使用源目录，不添加root-dir
        【RemountByName时，如el2解锁或nweb更新时】
    */
    AddVariableReplaceHandler("<deps-sandbox-path>", ReplaceVariableForDepSandboxPath);
    AddVariableReplaceHandler("<deps-src-path>", ReplaceVariableForDepSrcPath);
    AddVariableReplaceHandler("<deps-path>", ReplaceVariableForDepPath);
}

void ClearVariable(void)
{
    OH_ListRemoveAll(&g_sandboxVarList, NULL);
}
