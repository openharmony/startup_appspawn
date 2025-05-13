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

#undef _GNU_SOURCE
#define _GNU_SOURCE

#include "appspawn_sandbox.h"

#include <sys/mount.h>
#include <dirent.h>
#include "securec.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "modulemgr.h"

#define DEBUG_MNT_TMP_ROOT     "/mnt/debugtmp/"
#define DEBUG_MNT_SHAREFS_ROOT "/mnt/debug/"
#define DEBUG_HAP_DIR          "debug_hap"

typedef struct TagRemoveDebugDirInfo {
    char *debugTmpPath;
    char *debugPath;
    AppSpawnSandboxCfg *sandboxCfg;
    SandboxContext *context;
} RemoveDebugDirInfo;

static int InitDebugSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
                                   const AppSpawningCtx *property, int nwebspawn);

static void UmountAndRmdirDir(const char *targetPath)
{
    if (access(targetPath, F_OK) != 0) {
        APPSPAWN_LOGE("targetPath %{public}s is not exist", targetPath);
        return;
    }

    int ret = umount2(targetPath, MNT_DETACH);
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "umount failed %{public}s errno: %{public}d", targetPath, errno);
    ret = rmdir(targetPath);
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "rmdir failed %{public}s errno: %{public}d", targetPath, errno);
    APPSPAWN_LOGI("rmdir targetPath: %{public}s", targetPath);
}

static int RemoveDebugBaseConfig(SandboxSection *section, const char *debugRootPath)
{
    ListNode *node = section->front.next;
    int ret = 0;
    while (node != &section->front && node != NULL) {
        SandboxMountNode *sandboxNode = (SandboxMountNode *)ListEntry(node, SandboxMountNode, node);
        APPSPAWN_CHECK(sandboxNode != NULL, return APPSPAWN_SANDBOX_INVALID, "Get sandbox mount node failed");
        char targetPath[PATH_MAX_LEN] = {0};
        ret = snprintf_s(targetPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s", debugRootPath,
                         ((PathMountNode *)sandboxNode)->target);
        APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
                       "snprintf_s targetPath failed, errno: %{public}d", errno);
        UmountAndRmdirDir(targetPath);
        node = node->next;
    }
    return 0;
}

static int RemoveDebugAppVarConfig(const AppSpawnSandboxCfg *sandboxCfg, const char *debugRootPath)
{
    SandboxSection *section = GetSandboxSection(&sandboxCfg->requiredQueue, "app-variable");
    if (section == NULL) {
        return 0;
    }

    return RemoveDebugBaseConfig(section, debugRootPath);
}

static int RemoveDebugPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandboxCfg,
                                       const char *debugRootPath)
{
    ListNode *node = sandboxCfg->permissionQueue.front.next;
    int ret = 0;
    while (node != &sandboxCfg->permissionQueue.front && node != NULL) {
        SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
        APPSPAWN_CHECK(permissionNode != NULL, return APPSPAWN_SANDBOX_INVALID, "Get sandbox permission node failed");
        APPSPAWN_LOGV("CheckSandboxCtxPermissionFlagSet permission %{public}d %{public}s",
                      permissionNode->permissionIndex, permissionNode->section.name);
        ret = RemoveDebugBaseConfig(&permissionNode->section, debugRootPath);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to remove debug permission config");
        node = node->next;
    }
    return ret;
}

// 获取userId信息，若消息请求中携带userId拓展字段则使用userId否则使用info信息中的uid
static int ConvertUserIdPath(const AppSpawningCtx *property, char *debugRootPath, char *debugTmpRootPath)
{
    int ret = 0;
    char *userId = (char *)GetAppSpawnMsgExtInfo(property->message, MSG_EXT_NAME_USERID, NULL);
    if (userId == NULL) {
        AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
        APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_TLV_NONE, "No tlv %{public}d in msg", TLV_DAC_INFO);
        ret = snprintf_s(debugRootPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%d/", DEBUG_MNT_SHAREFS_ROOT,
                         dacInfo->uid / UID_BASE);
        APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
                       "snprintf_s debugRootPath failed, errno: %{public}d", errno);
        ret = snprintf_s(debugTmpRootPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%d/", DEBUG_MNT_TMP_ROOT,
                         dacInfo->uid / UID_BASE);
        APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
                       "snprintf_s debugTmpRootPath failed, errno: %{public}d", errno);
    } else {
        ret = snprintf_s(debugRootPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s/", DEBUG_MNT_SHAREFS_ROOT, userId);
        APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
                       "snprintf_s debugRootPath failed, errno: %{public}d", errno);
        ret = snprintf_s(debugTmpRootPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s/", DEBUG_MNT_TMP_ROOT, userId);
        APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
                       "snprintf_s debugTmpRootPath failed, errno: %{public}d", errno);
    }
    return 0;
}

static int UninstallPrivateDirs(const AppSpawnMgr *content, const AppSpawningCtx *property,
                                RemoveDebugDirInfo *removeDebugDirInfo)
{
    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_TLV_NONE, "No tlv %{public}d in msg", TLV_DAC_INFO);

    char uidPath[PATH_MAX_LEN] = {0};
    /* snprintf_s /mnt/debugtmp/<uid> */
    int ret = snprintf_s(uidPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%d/", DEBUG_MNT_TMP_ROOT, dacInfo->uid / UID_BASE);
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
                   "snprintf_s /mnt/debugtmp/<uid> failed, errno: %{public}d", errno);

    /* snprintf_s /mnt/debugtmp/<userId>/debug_hap/<variablePackageName> */
    ret = snprintf_s(removeDebugDirInfo->debugTmpPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s",
                     removeDebugDirInfo->debugTmpPath, removeDebugDirInfo->context->rootPath + strlen(uidPath));
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
        "snprintf_s /mnt/debugtmp/<userId>/debug_hap/<variablePackageName> failed, errno: %{public}d", errno);

    ret = RemoveDebugAppVarConfig(removeDebugDirInfo->sandboxCfg, removeDebugDirInfo->debugTmpPath);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to remove debug app variable config");

    ret = RemoveDebugPermissionConfig(removeDebugDirInfo->context, removeDebugDirInfo->sandboxCfg,
                                      removeDebugDirInfo->debugTmpPath);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to remove debug permission config");

    /* umount and remove dir /mnt/debug/<userId>/debug_hap/<variablePackageName> */
    ret = snprintf_s(removeDebugDirInfo->debugPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s/",
        removeDebugDirInfo->debugPath, removeDebugDirInfo->context->rootPath + strlen(uidPath));
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
        "snprintf_s /mnt/debug/<userId>/debug_hap/<variablePackageName> failed, errno: %{public}d", errno);
    UmountAndRmdirDir(removeDebugDirInfo->debugPath);

    return 0;
}

static int UninstallAllDirs(const AppSpawnMgr *content, const AppSpawningCtx *property,
                            RemoveDebugDirInfo *removeDebugDirInfo)
{
    /* snprintf_s /mnt/debugtmp/<uid>/debug_hap */
    int ret = snprintf_s(removeDebugDirInfo->debugTmpPath + strlen(removeDebugDirInfo->debugTmpPath), PATH_MAX_LEN,
                         PATH_MAX_LEN - 1, "%s/", "debug_hap");
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL,
                   "snprintf_s /mnt/debugtmp/<uid>/debug_hap failed, errno: %{public}d", errno);

    char debugTmpPackagePath[PATH_MAX_LEN] = {0};
    char debugPackagePath[PATH_MAX_LEN] = {0};
    DIR *dir = opendir(removeDebugDirInfo->debugTmpPath);
    APPSPAWN_CHECK(dir != NULL, return APPSPAWN_SYSTEM_ERROR,
        "Failed to open %{public}s, errno: %{public}d", removeDebugDirInfo->debugTmpPath, errno);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        (void)memset_s(debugPackagePath, PATH_MAX_LEN, 0, PATH_MAX_LEN);
        (void)memset_s(debugTmpPackagePath, PATH_MAX_LEN, 0, PATH_MAX_LEN);

        /* snprintf_s /mnt/debugtmp/<uid>/debug_hap/<variablePackageName> */
        ret = snprintf_s(debugTmpPackagePath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s",
                         removeDebugDirInfo->debugTmpPath, entry->d_name);
        APPSPAWN_CHECK(ret > 0, closedir(dir); return APPSPAWN_ERROR_UTILS_MEM_FAIL,
            "snprintf_s /mnt/debugtmp/<uid>/debug_hap/<variablePackageName> failed, errno: %{public}d", errno);

        ret = RemoveDebugAppVarConfig(removeDebugDirInfo->sandboxCfg, debugTmpPackagePath);
        APPSPAWN_CHECK(ret == 0, closedir(dir);
                                 return ret, "Failed to remove app variable config");

        ret = RemoveDebugPermissionConfig(removeDebugDirInfo->context, removeDebugDirInfo->sandboxCfg,
                                          debugTmpPackagePath);
        APPSPAWN_CHECK(ret == 0, closedir(dir);
                                 return ret, "Failed to remove debug permission config");

        /**
         * umount and remove dir /mnt/debug/<uid>/debug_hap/<variablePackageName>
         * /mnt/debug + <uid>/debug_hap/<variablePackageName>(debugTmpPackagePath)
         */
        ret = snprintf_s(debugPackagePath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s", DEBUG_MNT_SHAREFS_ROOT,
                         debugTmpPackagePath + strlen(DEBUG_MNT_TMP_ROOT));
        APPSPAWN_CHECK(ret > 0, closedir(dir); return APPSPAWN_ERROR_UTILS_MEM_FAIL,
            "snprintf_s /mnt/debug/<uid>/debug_hap/<variablePackageName> failed, errno: %{public}d", errno);
        UmountAndRmdirDir(debugPackagePath);
    }
    closedir(dir);
    return 0;
}

static int UninstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK(content != NULL && property != NULL, return APPSPAWN_ARG_INVALID,
        "Invalid appspawn client or property");
    char debugRootPath[PATH_MAX_LEN] = {0};
    char debugTmpRootPath[PATH_MAX_LEN] = {0};

    int ret = ConvertUserIdPath(property, debugRootPath, debugTmpRootPath);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to convert userid path");

    AppSpawnSandboxCfg *sandboxCfg = GetAppSpawnSandbox(content, EXT_DATA_DEBUG_HAP_SANDBOX);
    APPSPAWN_CHECK(sandboxCfg != NULL, return APPSPAWN_SANDBOX_INVALID,
                              "Failed to get sandbox for %{public}s", GetProcessName(property));

    SandboxContext *context = GetSandboxContext();   // Need free after mounting each time
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return APPSPAWN_ERROR_UTILS_MEM_FAIL);
    ret = InitDebugSandboxContext(context, sandboxCfg, property, IsNWebSpawnMode(content));
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, DeleteSandboxContext(context);
                                        return ret);

    RemoveDebugDirInfo removeDebugDirInfo = {
        .debugPath = debugRootPath,
        .debugTmpPath = debugTmpRootPath,
        .context = context,
        .sandboxCfg = sandboxCfg
    };
    // If the message request carries package name information, it is necessary to obtain the actual package name
    if (GetBundleName(property) != NULL) {
        ret = UninstallPrivateDirs(content, property, &removeDebugDirInfo);
    } else {  // Traverse directories from debugTmpRootPath directory
        ret = UninstallAllDirs(content, property, &removeDebugDirInfo);
    }
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "Failed to uninstall debug hap dir, ret: %{public}d", ret);

    DeleteSandboxContext(context);
    return 0;
}

// Mount the point of the app-variable attribute in the debug attribute.
static int SetDebugAppVarConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "app-variable");
    if (section == NULL) {
        return 0;
    }

    uint32_t operation = 0;
    if (CheckSandboxCtxMsgFlagSet(context, APP_FLAGS_ATOMIC_SERVICE)) {
        SetMountPathOperation(&operation, MOUNT_PATH_OP_UNMOUNT);
    }
    int ret = MountSandboxConfig(context, sandbox, section, operation);
    APPSPAWN_CHECK(ret == 0, return ret, "Set debug app-variable config fail result: %{public}d, app: %{public}s",
                   ret, context->bundleName);
    return 0;
}

static int SetDebugPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    ListNode *node = sandbox->permissionQueue.front.next;
    while (node != &sandbox->permissionQueue.front && node != NULL) {
        SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
        if (!CheckSandboxCtxPermissionFlagSet(context, permissionNode->permissionIndex)) {
            node = node->next;
            continue;
        }

        APPSPAWN_LOGV("SetSandboxPermissionConfig permission %{public}d %{public}s",
                      permissionNode->permissionIndex, permissionNode->section.name);
        uint32_t operation = MOUNT_PATH_OP_NONE;
        if (CheckSandboxCtxMsgFlagSet(context, APP_FLAGS_ATOMIC_SERVICE)) {
            SetMountPathOperation(&operation, MOUNT_PATH_OP_UNMOUNT);
        }
        int ret = MountSandboxConfig(context, sandbox, &permissionNode->section, operation);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
        node = node->next;
    }
    return 0;
}

static int SetDebugAutomicTmpRootPath(SandboxContext *context, const AppSpawningCtx *property)
{
    context->bundleName = GetBundleName(property);
    context->message = property->message;
    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSandboxCtxMsgInfo(context, TLV_DAC_INFO);
    APPSPAWN_CHECK(info != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, context->bundleName);

    char debugAutomicRootPath[PATH_MAX_LEN] = {0};
    int ret = snprintf_s(debugAutomicRootPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "/mnt/debugtmp/%d/debug_hap/%s",
                         info->uid / UID_BASE, context->bundleName);
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL, "snprintf_s debugAutomicRootPath failed");
    context->rootPath = strdup(debugAutomicRootPath);
    if (context->rootPath == NULL) {
        DeleteSandboxContext(context);
        return APPSPAWN_SYSTEM_ERROR;
    }
    APPSPAWN_LOGI("Set automic sandbox root: %{public}s", context->rootPath);
    return 0;
}

static int InitDebugSandboxContext(SandboxContext *context, const AppSpawnSandboxCfg *sandbox,
                                   const AppSpawningCtx *property, int nwebspawn)
{
    if (GetBundleName(property) == NULL) {
        APPSPAWN_LOGI("No need init sandbox context");
        return 0;
    }
    if (!CheckAppMsgFlagsSet(property, APP_FLAGS_ATOMIC_SERVICE)) {
        return InitSandboxContext(context, sandbox, property, nwebspawn);
    }

    return SetDebugAutomicTmpRootPath(context, property);
}

static int MountDebugTmpConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    int ret = SetDebugAppVarConfig(context, sandbox);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = SetDebugPermissionConfig(context, sandbox);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    return ret;
}

static int MountDebugDirBySharefs(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (context->rootPath == NULL) {
        APPSPAWN_LOGE("sandbox root is null");
        return APPSPAWN_SANDBOX_INVALID;
    }

    const char *srcPath = context->rootPath;
    char dstPath[PATH_MAX_LEN] = {0};
    size_t mntTmpRootLen = strlen(DEBUG_MNT_TMP_ROOT);
    int ret = snprintf_s(dstPath, PATH_MAX_LEN, PATH_MAX_LEN - 1, "%s%s",
                         DEBUG_MNT_SHAREFS_ROOT, context->rootPath + mntTmpRootLen);
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL, "Failed to snprintf_s dstPath");

    // If mount dstPath to MS_SHARED, the mount point already exists.
    ret = mount(NULL, dstPath, NULL, MS_SHARED, NULL);
    if (ret == 0) {
        return 0;
    }

    ret = MakeDirRec(dstPath, FILE_MODE, 1);
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL, "Failed to mkdir dstPath: %{public}s", dstPath);

    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSandboxCtxMsgInfo(context, TLV_DAC_INFO);
    APPSPAWN_CHECK(info != NULL, return APPSPAWN_TLV_NONE,
                   "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, context->bundleName);

    char options[OPTIONS_MAX_LEN] = {0};
    ret = snprintf_s(options, OPTIONS_MAX_LEN, OPTIONS_MAX_LEN - 1, "override_support_delete,user_id=%u",
                     info->uid / UID_BASE);
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL, "Failed to snprintf_s options");

    const MountArgTemplate *tmp = GetMountArgTemplate(MOUNT_TMP_DAC_OVERRIDE_DELETE);
    APPSPAWN_CHECK(tmp != NULL, return APPSPAWN_SANDBOX_INVALID, "Failed to get mount args");

    MountArg args = {
        .originPath = srcPath,
        .destinationPath = dstPath,
        .fsType = tmp->fsType,
        .options = options,
        .mountFlags = tmp->mountFlags,
        .mountSharedFlag = MS_SLAVE
    };
    ret = SandboxMountPath(&args);
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_SYSTEM_ERROR, "Failed to mount points");

    return 0;
}

static int InstallDebugSandbox(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK(property != NULL && content != NULL, return APPSPAWN_ARG_INVALID,
                   "Invalid appspawn client or property");
    
    if (!IsDeveloperModeOn(property)) {
        return 0;
    }

    uint32_t size = 0;
    char *provisionType = GetAppSpawnMsgExtInfo(property->message, MSG_EXT_NAME_PROVISION_TYPE, &size);
    if (provisionType == NULL || size == 0 || strcmp(provisionType, "debug") != 0) {
        return 0;
    }

    APPSPAWN_LOGI("Install %{public}s debug sandbox", GetProcessName(property));
    AppSpawnSandboxCfg *sandboxCfg = GetAppSpawnSandbox(content, EXT_DATA_DEBUG_HAP_SANDBOX);
    APPSPAWN_CHECK(sandboxCfg != NULL, return APPSPAWN_SANDBOX_INVALID,
                   "Failed to get sandbox for %{public}s", GetProcessName(property));

    SandboxContext *context = GetSandboxContext();   // Need free after mounting each time
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return APPSPAWN_SYSTEM_ERROR);
    int ret = InitDebugSandboxContext(context, sandboxCfg, property, IsNWebSpawnMode(content));
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, DeleteSandboxContext(context);
                                        return ret);

    do {
        ret = MountDebugTmpConfig(context, sandboxCfg);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        /**
         * /mnt/debugtmp/<currentUserId>/debug_hap/<variablePackageName>/ =>
         * /mnt/debug/<currentUserId>/debug_hap/<variablePackageName>/
         */
        ret = MountDebugDirBySharefs(context, sandboxCfg);
    } while (0);

    DeleteSandboxContext(context);
    return 0;
}

#ifdef APPSPAWN_SANDBOX_NEW
MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load debug hap module ...");
    (void)AddAppSpawnHook(STAGE_PARENT_UNINSTALL, HOOK_PRIO_SANDBOX, UninstallDebugSandbox);
    (void)AddAppSpawnHook(STAGE_PARENT_POST_RELY, HOOK_PRIO_SANDBOX, InstallDebugSandbox);
}
#endif