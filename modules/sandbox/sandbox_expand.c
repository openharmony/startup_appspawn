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

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "appspawn_msg.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "json_utils.h"
#include "securec.h"

#define SANDBOX_GROUP_PATH "/data/storage/el2/group/"
#define SANDBOX_INSTALL_PATH "/data/storage/el2/group/"
#define SANDBOX_OVERLAY_PATH "/data/storage/overlay/"

static inline bool CheckPath(const char *name)
{
    return name != NULL && strcmp(name, ".") != 0 && strcmp(name, "..") != 0 && strstr(name, "/") == NULL;
}

static int MountAllHsp(const SandboxContext *context, const cJSON *hsps)
{
    APPSPAWN_CHECK(context != NULL && hsps != NULL, return -1, "Invalid context or hsps");

    int ret = 0;
    cJSON *bundles = cJSON_GetObjectItemCaseSensitive(hsps, "bundles");
    cJSON *modules = cJSON_GetObjectItemCaseSensitive(hsps, "modules");
    cJSON *versions = cJSON_GetObjectItemCaseSensitive(hsps, "versions");
    APPSPAWN_CHECK(bundles != NULL && cJSON_IsArray(bundles), return -1, "MountAllHsp: invalid bundles");
    APPSPAWN_CHECK(modules != NULL && cJSON_IsArray(modules), return -1, "MountAllHsp: invalid modules");
    APPSPAWN_CHECK(versions != NULL && cJSON_IsArray(versions), return -1, "MountAllHsp: invalid versions");
    int count = cJSON_GetArraySize(bundles);
    APPSPAWN_CHECK(count == cJSON_GetArraySize(modules), return -1, "MountAllHsp: sizes are not same");
    APPSPAWN_CHECK(count == cJSON_GetArraySize(versions), return -1, "MountAllHsp: sizes are not same");

    APPSPAWN_LOGI("MountAllHsp app: %{public}s, count: %{public}d", context->bundleName, count);
    for (int i = 0; i < count; i++) {
        char *libBundleName = cJSON_GetStringValue(cJSON_GetArrayItem(bundles, i));
        char *libModuleName = cJSON_GetStringValue(cJSON_GetArrayItem(modules, i));
        char *libVersion = cJSON_GetStringValue(cJSON_GetArrayItem(versions, i));
        APPSPAWN_CHECK(CheckPath(libBundleName) && CheckPath(libModuleName) && CheckPath(libVersion),
            return -1, "MountAllHsp: path error");

        // src path
        int len = sprintf_s(context->buffer[0].buffer, context->buffer[0].bufferLen, "%s%s/%s/%s",
            PHYSICAL_APP_INSTALL_PATH, libBundleName, libVersion, libModuleName);
        APPSPAWN_CHECK(len > 0, return -1, "Failed to format install path");
        // sandbox path
        len = sprintf_s(context->buffer[1].buffer, context->buffer[1].bufferLen, "%s%s%s/%s",
            context->rootPath, SANDBOX_INSTALL_PATH, libBundleName, libModuleName);
        APPSPAWN_CHECK(len > 0, return -1, "Failed to format install path");

        CreateSandboxDir(context->buffer[1].buffer, FILE_MODE);
        MountArg mountArg = {
            context->buffer[0].buffer, context->buffer[1].buffer, NULL, MS_REC | MS_BIND, NULL, MS_SLAVE
        };
        ret = SandboxMountPath(&mountArg);
        APPSPAWN_CHECK(ret == 0, return ret, "mount library failed %{public}d", ret);
    }
    return ret;
}

static inline char *GetLastPath(const char *libPhysicalPath)
{
    char *tmp = GetLastStr(libPhysicalPath, "/");
    return tmp + 1;
}

static int MountAllGroup(const SandboxContext *context, const cJSON *groups)
{
    APPSPAWN_CHECK(context != NULL && groups != NULL, return -1, "Invalid context or group");
    int ret = 0;
    cJSON *dataGroupIds = cJSON_GetObjectItemCaseSensitive(groups, "dataGroupId");
    cJSON *gids = cJSON_GetObjectItemCaseSensitive(groups, "gid");
    cJSON *dirs = cJSON_GetObjectItemCaseSensitive(groups, "dir");
    APPSPAWN_CHECK(dataGroupIds != NULL && cJSON_IsArray(dataGroupIds),
        return -1, "MountAllGroup: invalid dataGroupIds");
    APPSPAWN_CHECK(gids != NULL && cJSON_IsArray(gids), return -1, "MountAllGroup: invalid gids");
    APPSPAWN_CHECK(dirs != NULL && cJSON_IsArray(dirs), return -1, "MountAllGroup: invalid dirs");
    int count = cJSON_GetArraySize(dataGroupIds);
    APPSPAWN_CHECK(count == cJSON_GetArraySize(gids), return -1, "MountAllGroup: sizes are not same");
    APPSPAWN_CHECK(count == cJSON_GetArraySize(dirs), return -1, "MountAllGroup: sizes are not same");

    APPSPAWN_LOGI("MountAllGroup: app: %{public}s, count: %{public}d", context->bundleName, count);
    for (int i = 0; i < count; i++) {
        cJSON *dirJson = cJSON_GetArrayItem(dirs, i);
        APPSPAWN_CHECK(dirJson != NULL && cJSON_IsString(dirJson), return -1, "MountAllGroup: invalid dirJson");
        const char *libPhysicalPath = cJSON_GetStringValue(dirJson);
        APPSPAWN_CHECK(!CheckPath(libPhysicalPath), return -1, "MountAllGroup: path error");

        char *dataGroupUuid = GetLastPath(libPhysicalPath);
        int len = sprintf_s(context->buffer[0].buffer, context->buffer[0].bufferLen, "%s%s%s",
            context->rootPath, SANDBOX_GROUP_PATH, dataGroupUuid);
        APPSPAWN_CHECK(len > 0, return -1, "Failed to format install path");
        APPSPAWN_LOGV("MountAllGroup src: '%{public}s' =>'%{public}s'", libPhysicalPath, context->buffer[0].buffer);

        CreateSandboxDir(context->buffer[0].buffer, FILE_MODE);
        MountArg mountArg = {libPhysicalPath, context->buffer[0].buffer, NULL, MS_REC | MS_BIND, NULL, MS_SLAVE};
        ret = SandboxMountPath(&mountArg);
        APPSPAWN_CHECK(ret == 0, return ret, "mount library failed %{public}d", ret);
    }
    return ret;
}

typedef struct {
    const SandboxContext *sandboxContext;
    uint32_t srcSetLen;
    char *mountedSrcSet;
} OverlayContext;

static int SetOverlayAppPath(const char *hapPath, void *context)
{
    APPSPAWN_LOGV("SetOverlayAppPath '%{public}s'", hapPath);
    OverlayContext *overlayContext = (OverlayContext *)context;
    const SandboxContext *sandboxContext = overlayContext->sandboxContext;

    // src path
    char *tmp = GetLastStr(hapPath, "/");
    if (tmp == NULL) {
        return 0;
    }
    int ret = strncpy_s(sandboxContext->buffer[0].buffer,
        sandboxContext->buffer[0].bufferLen, hapPath, tmp - (char *)hapPath);
    APPSPAWN_CHECK(ret == 0, return ret, "mount library failed %{public}d", ret);

    if (strstr(overlayContext->mountedSrcSet, sandboxContext->buffer[0].buffer) != NULL) {
        APPSPAWN_LOGV("%{public}s have mounted before, no need to mount twice.", sandboxContext->buffer[0].buffer);
        return 0;
    }
    ret = strcat_s(overlayContext->mountedSrcSet, overlayContext->srcSetLen, "|");
    APPSPAWN_CHECK(ret == 0, return ret, "Fail to add src path to set %{public}s", "|");
    ret = strcat_s(overlayContext->mountedSrcSet, overlayContext->srcSetLen, sandboxContext->buffer[0].buffer);
    APPSPAWN_CHECK(ret == 0, return ret, "Fail to add src path to set %{public}s", sandboxContext->buffer[0].buffer);

    // sandbox path
    tmp = GetLastStr(sandboxContext->buffer[0].buffer, "/");
    if (tmp == NULL) {
        return 0;
    }
    int len = sprintf_s(sandboxContext->buffer[1].buffer, sandboxContext->buffer[1].bufferLen, "%s%s",
        sandboxContext->rootPath, SANDBOX_OVERLAY_PATH);
    APPSPAWN_CHECK(len > 0, return -1, "Failed to format install path");
    ret = strcat_s(sandboxContext->buffer[1].buffer, sandboxContext->buffer[1].bufferLen - len, tmp + 1);
    APPSPAWN_CHECK(ret == 0, return ret, "mount library failed %{public}d", ret);
    APPSPAWN_LOGV("SetOverlayAppPath path: '%{public}s' => '%{public}s'",
        sandboxContext->buffer[0].buffer, sandboxContext->buffer[1].buffer);

    MountArg mountArg = {
        sandboxContext->buffer[0].buffer, sandboxContext->buffer[1].buffer, NULL, MS_REC | MS_BIND, NULL, MS_SHARED
    };
    int retMount = SandboxMountPath(&mountArg);
    if (retMount != 0) {
        APPSPAWN_LOGE("Fail to mount overlay path, src is %{public}s.", hapPath);
        ret = retMount;
    }
    return ret;
}

static int SetOverlayAppSandboxConfig(const SandboxContext *context, const char *overlayInfo)
{
    APPSPAWN_CHECK(context != NULL && overlayInfo != NULL, return -1, "Invalid context or overlayInfo");
    OverlayContext overlayContext;
    overlayContext.sandboxContext = context;
    overlayContext.srcSetLen = strlen(overlayInfo);
    overlayContext.mountedSrcSet = (char *)calloc(1, overlayContext.srcSetLen + 1);
    APPSPAWN_CHECK(overlayContext.mountedSrcSet != NULL, return -1, "Failed to create mountedSrcSet");
    *(overlayContext.mountedSrcSet + overlayContext.srcSetLen) = '\0';
    int ret = StringSplit(overlayInfo, "|", (void *)&overlayContext, SetOverlayAppPath);
    APPSPAWN_LOGV("overlayContext->mountedSrcSet: '%{public}s'", overlayContext.mountedSrcSet);
    free(overlayContext.mountedSrcSet);
    return ret;
}

static inline cJSON *GetJsonObjFromProperty(const SandboxContext *context, const char *name)
{
    uint32_t size = 0;
    char *extInfo = (char *)(GetAppSpawnMsgExtInfo(context->message, name, &size));
    if (size == 0 || extInfo == NULL) {
        return NULL;
    }
    APPSPAWN_LOGV("Get json name %{public}s value %{public}s", name, extInfo);
    cJSON *root = cJSON_Parse(extInfo);
    APPSPAWN_CHECK(root != NULL, return NULL, "Invalid ext info %{public}s for %{public}s", extInfo, name);
    return root;
}

static int ProcessHSPListConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandBox, const char *name)
{
    cJSON *root = GetJsonObjFromProperty(context, name);
    APPSPAWN_CHECK_ONLY_EXPER(root != NULL, return 0);
    int ret = MountAllHsp(context, root);
    cJSON_Delete(root);
    return ret;
}

static int ProcessDataGroupConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandBox, const char *name)
{
    cJSON *root = GetJsonObjFromProperty(context, name);
    APPSPAWN_CHECK_ONLY_EXPER(root != NULL, return 0);
    int ret = MountAllGroup(context, root);
    cJSON_Delete(root);
    return ret;
}

static int ProcessOverlayAppConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandBox, const char *name)
{
    uint32_t size = 0;
    char *extInfo = (char *)GetAppSpawnMsgExtInfo(context->message, name, &size);
    if (size == 0 || extInfo == NULL) {
        return 0;
    }
    APPSPAWN_LOGV("ProcessOverlayAppConfig name %{public}s value %{public}s", name, extInfo);
    return SetOverlayAppSandboxConfig(context, extInfo);
}

struct ListNode g_sandboxExpandCfgList = {&g_sandboxExpandCfgList, &g_sandboxExpandCfgList};
static int AppSandboxExpandAppCfgCompareName(ListNode *node, void *data)
{
    AppSandboxExpandAppCfgNode *varNode = ListEntry(node, AppSandboxExpandAppCfgNode, node);
    return strncmp((char *)data, varNode->name, strlen(varNode->name));
}

static int AppSandboxExpandAppCfgComparePrio(ListNode *node1, ListNode *node2)
{
    AppSandboxExpandAppCfgNode *varNode1 = ListEntry(node1, AppSandboxExpandAppCfgNode, node);
    AppSandboxExpandAppCfgNode *varNode2 = ListEntry(node2, AppSandboxExpandAppCfgNode, node);
    return varNode1->prio - varNode2->prio;
}

static const AppSandboxExpandAppCfgNode *GetAppSandboxExpandAppCfg(const char *name)
{
    ListNode *node = OH_ListFind(&g_sandboxExpandCfgList, (void *)name, AppSandboxExpandAppCfgCompareName);
    if (node == NULL) {
        return NULL;
    }
    return ListEntry(node, AppSandboxExpandAppCfgNode, node);
}

int RegisterExpandSandboxCfgHandler(const char *name, int prio, ProcessExpandSandboxCfg handleExpandCfg)
{
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL && handleExpandCfg != NULL, return APPSPAWN_ARG_INVALID);
    if (GetAppSandboxExpandAppCfg(name) != NULL) {
        return APPSPAWN_NODE_EXIST;
    }

    size_t len = APPSPAWN_ALIGN(strlen(name) + 1);
    AppSandboxExpandAppCfgNode *node = (AppSandboxExpandAppCfgNode *)(malloc(sizeof(AppSandboxExpandAppCfgNode) + len));
    APPSPAWN_CHECK(node != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to create sandbox");
    // ext data init
    OH_ListInit(&node->node);
    node->cfgHandle = handleExpandCfg;
    node->prio = prio;
    int ret = strcpy_s(node->name, len, name);
    APPSPAWN_CHECK(ret == 0, free(node);
        return -1, "Failed to copy name %{public}s", name);
    OH_ListAddWithOrder(&g_sandboxExpandCfgList, &node->node, AppSandboxExpandAppCfgComparePrio);
    return 0;
}

int ProcessExpandAppSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandBox, const char *name)
{
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL && appSandBox != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_LOGV("ProcessExpandAppSandboxConfig %{public}s.", name);
    const AppSandboxExpandAppCfgNode *node = GetAppSandboxExpandAppCfg(name);
    if (node != NULL && node->cfgHandle != NULL) {
        return node->cfgHandle(context, appSandBox, name);
    }
    return 0;
}

void AddDefaultExpandAppSandboxConfigHandle(void)
{
    RegisterExpandSandboxCfgHandler("HspList", 0, ProcessHSPListConfig);
    RegisterExpandSandboxCfgHandler("DataGroup", 1, ProcessDataGroupConfig);
    RegisterExpandSandboxCfgHandler("Overlay", 2, ProcessOverlayAppConfig);  // 2 priority
}

void ClearExpandAppSandboxConfigHandle(void)
{
    OH_ListRemoveAll(&g_sandboxExpandCfgList, NULL);
}
