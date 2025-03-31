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

#include "securec.h"
#include "appspawn_msg.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "json_utils.h"
#include "appspawn_permission.h"
#include "sandbox_shared.h"

#define SANDBOX_INSTALL_PATH "/data/storage/el1/bundle/"
#define SANDBOX_OVERLAY_PATH "/data/storage/overlay/"

APPSPAWN_STATIC int MountAllHsp(const SandboxContext *context, const cJSON *hsps)
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

APPSPAWN_STATIC int MountAllGroup(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox,
                                  const cJSON *groups)
{
    APPSPAWN_CHECK(context != NULL && groups != NULL && appSandbox != NULL, return APPSPAWN_SANDBOX_INVALID,
                   "Invalid context or group");
    mode_t mountFlags = MS_REC | MS_BIND;
    mode_t mountSharedFlag = MS_SLAVE;
    if (CheckAppSpawnMsgFlag(context->message, TLV_MSG_FLAGS, APP_FLAGS_ISOLATED_SANDBOX)) {
        APPSPAWN_LOGV("Data group flags is isolated");
        mountSharedFlag |= MS_REMOUNT | MS_NODEV | MS_RDONLY | MS_BIND;
    }
    int ret = 0;
    // Iterate through the array (assuming groups is an array)
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, groups) {
        // Check if the item is valid
        APPSPAWN_CHECK(IsValidDataGroupItem(item), return APPSPAWN_ARG_INVALID,
                       "Element is not a valid data group item");

        cJSON *dirItem = cJSON_GetObjectItemCaseSensitive(item, "dir");
        cJSON *uuidItem = cJSON_GetObjectItemCaseSensitive(item, "uuid");
        if (dirItem == NULL || !cJSON_IsString(dirItem) || uuidItem == NULL || !cJSON_IsString(uuidItem)) {
            APPSPAWN_LOGE("Data group element is invalid");
            return APPSPAWN_ARG_INVALID;
        }

        const char *srcPath = dirItem->valuestring;
        APPSPAWN_CHECK(!CheckPath(srcPath), return APPSPAWN_ARG_INVALID, "src path %{public}s is invalid", srcPath);

        int elxValue = GetElxInfoFromDir(srcPath);
        APPSPAWN_CHECK((elxValue >= EL2 && elxValue < ELX_MAX), return APPSPAWN_ARG_INVALID, "Get elx value failed");
        
        const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(elxValue);
        APPSPAWN_CHECK(templateItem != NULL, return APPSPAWN_ARG_INVALID, "Get data group arg template failed");

        // If permission isn't null, need check permission flag
        if (templateItem->permission != NULL) {
            int index = GetPermissionIndexInQueue(&appSandbox->permissionQueue, templateItem->permission);
            APPSPAWN_LOGV("mount dir no lock mount permission flag %{public}d", index);
            if (!CheckSandboxCtxPermissionFlagSet(context, (uint32_t)index)) {
                continue;
            }
        }
        (void)memset_s(context->buffer[0].buffer, context->buffer[0].bufferLen, 0, context->buffer[0].bufferLen);
        int len = snprintf_s(context->buffer[0].buffer, context->buffer[0].bufferLen, context->buffer[0].bufferLen - 1,
                             "%s%s%s", context->rootPath, templateItem->sandboxPath, uuidItem->valuestring);
        APPSPAWN_CHECK(len > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL, "Get data group arg template failed");
        ret = CreateSandboxDir(context->buffer[0].buffer, FILE_MODE);
        APPSPAWN_CHECK(ret == 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL, "Mkdir sandbox dir failed");
        MountArg mountArg = {srcPath, context->buffer[0].buffer, NULL, mountFlags, NULL, mountSharedFlag};
        ret = SandboxMountPath(&mountArg);
        APPSPAWN_CHECK_ONLY_LOG(ret == 0, "mount datagroup failed");
    }
    return 0;
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

    ret = MakeDirRec(sandboxContext->buffer[1].buffer, FILE_MODE, 1);
    APPSPAWN_CHECK(ret == 0, return ret, "Fail to mkdir dir %{public}s, ret: %{public}d",
                   sandboxContext->buffer[1].buffer, ret);
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

static int ProcessHSPListConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    cJSON *root = GetJsonObjFromProperty(context, name);
    APPSPAWN_CHECK_ONLY_EXPER(root != NULL, return 0);
    int ret = MountAllHsp(context, root);
    cJSON_Delete(root);
    return ret;
}

static int ProcessDataGroupConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    cJSON *root = GetJsonObjFromProperty(context, name);
    APPSPAWN_CHECK_ONLY_EXPER(root != NULL, return 0);
    int ret = MountAllGroup(context, appSandbox, root);
    cJSON_Delete(root);
    return ret;
}

static int ProcessOverlayAppConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandbox, const char *name)
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

int ProcessExpandAppSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *appSandbox, const char *name)
{
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL && appSandbox != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_LOGV("ProcessExpandAppSandboxConfig %{public}s.", name);
    const AppSandboxExpandAppCfgNode *node = GetAppSandboxExpandAppCfg(name);
    if (node != NULL && node->cfgHandle != NULL) {
        return node->cfgHandle(context, appSandbox, name);
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
