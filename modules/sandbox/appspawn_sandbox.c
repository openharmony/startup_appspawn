/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "appspawn_sandbox.h"

#include <dirent.h>

#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "init_utils.h"
#include "parameter.h"
#include "securec.h"

static inline bool CheckSpawningMsgFlagSet(const SandboxContext *context, uint32_t index)
{
    APPSPAWN_CHECK(context->message != NULL, return false, "Invalid property for type %{public}u", TLV_MSG_FLAGS);
    return CheckAppSpawnMsgFlag(context->message, TLV_MSG_FLAGS, index);
}

static inline bool CheckSpawningPermissionFlagSet(const SandboxContext *context, uint32_t index)
{
    APPSPAWN_CHECK(context != NULL && context->message != NULL,
        return NULL, "Invalid property for type %{public}u", TLV_PERMISSION);
    return CheckAppSpawnMsgFlag(context->message, TLV_PERMISSION, index);
}

static SandboxContext *g_sandboxContext = NULL;

SandboxContext *GetSandboxContext(void)
{
    if (g_sandboxContext == NULL) {
        SandboxContext *context = calloc(1, MAX_SANDBOX_BUFFER * MAX_BUFFER + sizeof(SandboxContext));
        APPSPAWN_CHECK(context != NULL, return NULL, "Failed to get mem");
        char *buffer = (char *)(context + 1);
        for (int i = 0; i < MAX_BUFFER; i++) {
            context->buffer[i].bufferLen = MAX_SANDBOX_BUFFER;
            context->buffer[i].current = 0;
            context->buffer[i].buffer = buffer + MAX_SANDBOX_BUFFER * i;
        }
        context->bundleName = NULL;
        context->bundleHasWps = 0;
        context->dlpBundle = 0;
        context->appFullMountEnable = 0;
        context->dlpUiExtType = 0;
        context->sandboxSwitch = 1;
        context->sandboxShared = false;
        context->message = NULL;
        context->rootPath = NULL;
        context->sandboxPackagePath = NULL;

        g_sandboxContext = context;
    }
    return g_sandboxContext;
}

void DeleteSandboxContext(SandboxContext *context)
{
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return);
    if (context->rootPath) {
        free(context->rootPath);
        context->rootPath = NULL;
    }
    if (context->sandboxPackagePath) {
        free(context->sandboxPackagePath);
        context->sandboxPackagePath = NULL;
    }
    if (context == g_sandboxContext) {
        g_sandboxContext = NULL;
    }
    free(context);
}

static int InitSandboxContext(SandboxContext *context,
    const AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
{
    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppProperty(property, TLV_MSG_FLAGS);
    APPSPAWN_CHECK(msgFlags != NULL, return APPSPAWN_TLV_NONE,
        "No msg flags in msg %{public}s", GetProcessName(property));
    context->nwebspawn = nwebspawn;
    context->bundleName = GetBundleName(property);
    context->bundleHasWps = strstr(context->bundleName, "wps") != NULL;
    context->dlpBundle = strstr(context->bundleName, "com.ohos.dlpmanager") != NULL;
    context->appFullMountEnable = sandbox->appFullMountEnable;
    context->dlpUiExtType = strstr(GetProcessName(property), "sys/commonUI") != NULL;

    context->sandboxSwitch = 1;
    context->sandboxShared = false;
    SandboxPackageNameNode *packageNode = (SandboxPackageNameNode *)GetSandboxSection(
        &sandbox->packageNameQueue, context->bundleName);
    if (packageNode) {
        context->sandboxShared = packageNode->section.sandboxShared;
    }
    context->message = property->message;
    return 0;
}

static VarExtraData *GetVarExtraData(const SandboxContext *context, const SandboxSection *section)
{
    static VarExtraData extraData;
    (void)memset_s(&extraData, sizeof(extraData), 0, sizeof(extraData));
    extraData.sandboxTag = GetSectionType(section);
    if (extraData.sandboxTag == SANDBOX_TAG_NAME_GROUP) {
        SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)section;
        extraData.data.depNode = groupNode->depNode;
    }
    return &extraData;
}

static int BuildPackagePath(SandboxContext *sandboxContext, const AppSpawnSandboxCfg *sandbox)
{
    const char *packagePath = GetSandboxRealVar(sandboxContext, BUFFER_FOR_SOURCE, sandbox->rootPath, NULL, NULL);
    if (packagePath) {
        sandboxContext->sandboxPackagePath = strdup(packagePath);
    }
    APPSPAWN_CHECK(sandboxContext->sandboxPackagePath != NULL,
        return APPSPAWN_SYSTEM_ERROR, "Failed to format path app: %{public}s", sandboxContext->bundleName);
    return 0;
}

static int BuildRootPath(SandboxContext *context, const AppSpawnSandboxCfg *sandbox, const SandboxSection *section)
{
    const char *rootPath = GetSandboxRealVar(context, BUFFER_FOR_SOURCE, sandbox->rootPath, NULL, NULL);
    if (rootPath) {
        context->rootPath = strdup(rootPath);
    }
    APPSPAWN_CHECK(context->rootPath != NULL, return -1,
        "Failed to build root path app: %{public}s", context->bundleName);
    return 0;
}

static uint32_t GetMountArgs(const SandboxContext *context, const PathMountNode *sandboxNode, MountArg *args)
{
    uint32_t category = sandboxNode->category;
    if ((category == MOUNT_TMP_DLP_FUSE) && !context->appFullMountEnable) {  // use default
        category = MOUNT_TMP_DEFAULT;
    }
    const MountArgTemplate *tmp = GetMountArgTemplate(category);
    if (tmp == 0) {
        return MOUNT_TMP_DEFAULT;
    }
    args->mountFlags = tmp->mountFlags;
    args->fsType = tmp->fsType;
    args->options = tmp->options;
    args->mountSharedFlag = (sandboxNode->mountSharedFlag) ? MS_SHARED : tmp->mountSharedFlag;
    return category;
}

static int CheckSandboxMountNode(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode)
{
    if (sandboxNode->source == NULL || sandboxNode->target == NULL) {
        APPSPAWN_LOGW("Invalid mount config section %{public}s", section->name);
        return 0;
    }
    // special handle wps and don't use /data/app/xxx/<Package> config
    if (GetSectionType(section) == SANDBOX_TAG_SPAWN_FLAGS) {  // flags-point
        if (context->bundleHasWps &&
            (strstr(sandboxNode->source, "/data/app") != NULL) &&
            (strstr(sandboxNode->source, "/base") != NULL || strstr(sandboxNode->source, "/database") != NULL) &&
            (strstr(sandboxNode->source, PARAMETER_PACKAGE_NAME) != NULL)) {
            APPSPAWN_LOGW("Invalid mount source %{public}s section %{public}s",
                sandboxNode->source, section->name);
            return 0;
        }
    }
    // check apl
    AppSpawnMsgDomainInfo *msgDomainInfo = (AppSpawnMsgDomainInfo *)GetSpawningMsgInfo(context, TLV_DOMAIN_INFO);
    if (msgDomainInfo != NULL && sandboxNode->appAplName != NULL) {
        if (!strcmp(sandboxNode->appAplName, msgDomainInfo->apl)) {
            APPSPAWN_LOGW("Invalid mount app apl %{public}s %{public}s section %{public}s",
                sandboxNode->appAplName, msgDomainInfo->apl, section->name);
            return 0;
        }
    }
    return 1;
}

static int32_t DoDlpAppMountStrategy(const SandboxContext *context, const MountArg *args)
{
    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSpawningMsgInfo(context, TLV_DAC_INFO);
    APPSPAWN_CHECK(info != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, context->bundleName);

    // umount fuse path, make sure that sandbox path is not a mount point
    umount2(args->destinationPath, MNT_DETACH);

    int fd = open("/dev/fuse", O_RDWR);
    APPSPAWN_CHECK(fd != -1, return -EINVAL,
        "open /dev/fuse failed, errno: %{public}d sandbox path %{public}s", errno, args->destinationPath);

    char options[FUSE_OPTIONS_MAX_LEN];
    (void)sprintf_s(options, sizeof(options), "fd=%d,"
        "rootmode=40000,user_id=%d,group_id=%d,allow_other,"
        "context=\"u:object_r:dlp_fuse_file:s0\","
        "fscontext=u:object_r:dlp_fuse_file:s0", fd, info->uid, info->gid);

    // To make sure destinationPath exist
    CreateSandboxDir(args->destinationPath, FILE_MODE);
    MountArg mountArg = {args->originPath, args->destinationPath, args->fsType, args->mountFlags, options, MS_SHARED};
    int ret = SandboxMountPath(&mountArg);
    if (ret != 0) {
        close(fd);
        return -1;
    }
    /* close DLP_FUSE_FD and dup FD to it */
    close(DLP_FUSE_FD);
    ret = dup2(fd, DLP_FUSE_FD);
    APPSPAWN_CHECK_ONLY_LOG(ret != -1, "dup fuse fd %{public}d failed, errno: %{public}d", fd, errno);
    return 0;
}

static void CheckAndCreateSandboxFile(const char *file)
{
    if (access(file, F_OK) == 0) {
        APPSPAWN_LOGI("file %{public}s already exist", file);
        return;
    }
    MakeDirRec(file, FILE_MODE, 0);
    int fd = open(file, O_CREAT, FILE_MODE);
    if (fd < 0) {
        APPSPAWN_LOGW("failed create %{public}s, err=%{public}d", file, errno);
    } else {
        close(fd);
    }
    return;
}

static int DoSandboxPathNodeMount(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, int operation)
{
    if (CheckSandboxMountNode(context, section, sandboxNode) == 0) {
        return 0;
    }

    MountArg args = {};
    uint32_t category = GetMountArgs(context, sandboxNode, &args);

    args.originPath = GetSandboxRealVar(context, BUFFER_FOR_SOURCE, sandboxNode->source, NULL, NULL);
    // only destinationPath
    VarExtraData *extraData = GetVarExtraData(context, section);
    args.destinationPath = GetSandboxRealVar(context,
        BUFFER_FOR_TARGET, sandboxNode->target, context->rootPath, extraData);
    APPSPAWN_CHECK(args.originPath != NULL && args.destinationPath != NULL,
        return APPSPAWN_ARG_INVALID, "Invalid path %{public}s %{public}s", args.originPath, args.destinationPath);

    if (sandboxNode->sandboxNode.type == SANDBOX_TAG_MOUNT_FILE) {
        CheckAndCreateSandboxFile(args.destinationPath);
    } else {
        CreateSandboxDir(args.destinationPath, FILE_MODE);
    }
    APPSPAWN_LOGV("Bind mount category %{public}u op %{public}x \n "
        "mount arg: %{public}s %{public}s %{public}x %{public}s \n %{public}s => %{public}s",
        category, operation, args.fsType, args.mountSharedFlag == MS_SHARED ? "MS_SHARED" : "MS_SLAVE",
        (uint32_t)args.mountFlags, args.options, args.originPath, args.destinationPath);

    if ((operation & MOUNT_PATH_OP_UNMOUNT) == MOUNT_PATH_OP_UNMOUNT) {
        // unmount this deps
        APPSPAWN_LOGV("DoSandboxPathNodeMount umount2 %{public}s", args.destinationPath);
        umount2(args.destinationPath, MNT_DETACH);
    }
    int ret = 0;
    if (category == MOUNT_TMP_DLP_FUSE || category == MOUNT_TMP_DLP_FUSE) {
        ret = DoDlpAppMountStrategy(context, &args);
    } else {
        ret = SandboxMountPath(&args);
    }
    if (ret != 0 && sandboxNode->checkErrorFlag) {
        APPSPAWN_LOGE("Failed to mount config, section: %{public}s result: %{public}d category: %{public}d",
            section->name, ret, category);
        return ret;
    }
    if (sandboxNode->destMode != 0) {
        chmod(context->rootPath, sandboxNode->destMode);
    }
    return 0;
}

static int DoSandboxPathSymLink(const SandboxContext *context,
    const SandboxSection *section, const SymbolLinkNode *sandboxNode)
{
    // Check the validity of the symlink configuration
    if (sandboxNode->linkName == NULL || sandboxNode->target == NULL) {
        APPSPAWN_LOGW("Invalid symlink config, section %{public}s", section->name);
        return 0;
    }

    const char *target = GetSandboxRealVar(context, BUFFER_FOR_SOURCE, sandboxNode->target, NULL, NULL);
    const char *linkName = GetSandboxRealVar(context, BUFFER_FOR_TARGET,
        sandboxNode->linkName, context->rootPath, NULL);
    APPSPAWN_LOGV("symlink, from %{public}s to %{public}s", target, linkName);
    int ret = symlink(target, linkName);
    if (ret && errno != EEXIST) {
        if (sandboxNode->checkErrorFlag) {
            APPSPAWN_LOGE("symlink failed, errno: %{public}d link info %{public}s %{public}s",
                errno, sandboxNode->target, sandboxNode->linkName);
            return errno;
        }
        APPSPAWN_LOGV("symlink failed, errno: %{public}d link info %{public}s %{public}s",
            errno, sandboxNode->target, sandboxNode->linkName);
    }
    if (sandboxNode->destMode != 0) {
        chmod(context->rootPath, sandboxNode->destMode);
    }
    return 0;
}

static int DoSandboxNodeMount(const SandboxContext *context, const SandboxSection *section, int operation)
{
    APPSPAWN_LOGV("DoSandboxNodeMount section %{public}s operation %{public}x", section->name, operation);
    ListNode *node = section->front.next;
    while (node != &section->front) {
        int ret = 0;
        SandboxMountNode *sandboxNode = (SandboxMountNode *)ListEntry(node, SandboxMountNode, node);
        switch (sandboxNode->type) {
            case SANDBOX_TAG_MOUNT_PATH:
            case SANDBOX_TAG_MOUNT_FILE:
                ret = DoSandboxPathNodeMount(context, section, (PathMountNode *)sandboxNode, operation);
                break;
            case SANDBOX_TAG_SYMLINK:
                if ((operation & MOUNT_PATH_OP_NO_SYMLINK) == MOUNT_PATH_OP_NO_SYMLINK) {
                    break;
                }
                ret = DoSandboxPathSymLink(context, section, (SymbolLinkNode *)sandboxNode);
                break;
            default:
                break;
        }
        if (ret != 0) {
            return ret;
        }
        node = node->next;
    }
    return 0;
}

static int UpdateMountPathDepsPath(const SandboxContext *context, SandboxNameGroupNode *groupNode)
{
    PathMountNode *depNode = groupNode->depNode;
    const char *srcPath = GetSandboxRealVar(context, BUFFER_FOR_SOURCE, depNode->source, NULL, NULL);
    const char *sandboxPath = GetSandboxRealVar(context, BUFFER_FOR_TARGET, depNode->target, NULL, NULL);
    if (srcPath == NULL || sandboxPath == NULL) {
        APPSPAWN_LOGE("Failed to get real path %{public}s ", groupNode->section.name);
        return APPSPAWN_SANDBOX_MOUNT_FAIL;
    }
    free(depNode->source);
    depNode->source = strdup(srcPath);
    free(depNode->target);
    depNode->target = strdup(sandboxPath);
    if (depNode->source == NULL || depNode->target == NULL) {
        APPSPAWN_LOGE("Failed to get real path %{public}s ", groupNode->section.name);
        if (depNode->source) {
            free(depNode->source);
            depNode->source = NULL;
        }
        if (depNode->target) {
            free(depNode->target);
            depNode->target = NULL;
        }
        return APPSPAWN_SANDBOX_MOUNT_FAIL;
    }
    return 0;
}

static bool CheckAndCreateDepPath(const SandboxContext *context, const SandboxNameGroupNode *sandboxNode)
{
    if (sandboxNode->mountMode != MOUNT_MODE_NOT_EXIST) {
        return false;
    }

    PathMountNode *mountNode = (PathMountNode *)GetFirstSandboxMountNode(&sandboxNode->section);
    if (mountNode == NULL) {
        return false;
    }

    const char *srcPath = GetSandboxRealVar(context, BUFFER_FOR_SOURCE, mountNode->source, NULL, NULL);
    if (srcPath == NULL) {
        return false;
    }
    APPSPAWN_LOGV("Mount depended to check src: %{public}s", srcPath);
    if (access(srcPath, F_OK) == 0) {
        return true;
    }
    // 这里已经是实际路径了，不需要转化
    APPSPAWN_LOGV("Mount depended source: %{public}s", sandboxNode->depNode->source);
    CreateSandboxDir(sandboxNode->depNode->source, FILE_MODE);
    return false;
}

static int MountNameGroupPath(const SandboxContext *context,
    const AppSpawnSandboxCfg *sandbox, const SandboxNameGroupNode *sandboxNode)
{
    int ret = 0;
    APPSPAWN_LOGV("Set name-group: '%{public}s' root path: '%{public}s' global %{public}s",
        sandboxNode->section.name, context->rootPath, sandbox->rootPath);

    ret = DoSandboxNodeMount(context, &sandboxNode->section, 0);
    APPSPAWN_CHECK(ret == 0, return ret,
        "Mount name group %{public}s fail result: %{public}d", sandboxNode->section.name, ret);
    return ret;
}

static int MountSandboxConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *sandbox, const SandboxSection *section, int operation)
{
    // if sandbox switch is off, don't do symlink work again
    operation |= !(context->sandboxSwitch && sandbox->topSandboxSwitch) ? MOUNT_PATH_OP_NO_SYMLINK : 0;
    APPSPAWN_LOGV("Set config section: %{public}s root path: '%{public}s' symlinkDo: %{public}x",
        section->name, context->rootPath, operation);

    int ret = DoSandboxNodeMount(context, section, operation);
    APPSPAWN_CHECK(ret == 0, return ret,
        "Mount sandbox config fail result: %{public}d, app: %{public}s", ret, context->bundleName);

    if (section->nameGroups == NULL) {
        return 0;
    }

    for (uint32_t i = 0; i < section->number; i++) {
        if (section->nameGroups[i] == NULL) {
            continue;
        }
        SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)section->nameGroups[i];
        MountNameGroupPath(context, sandbox, groupNode);
    }
    return 0;
}

static int SetExpandSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    int ret = ProcessExpandAppSandboxConfig(context, sandbox, "HspList");
    APPSPAWN_CHECK(ret == 0, return ret,
        "Set HspList config fail result: %{public}d, app: %{public}s", ret, context->bundleName);
    ret = ProcessExpandAppSandboxConfig(context, sandbox, "DataGroup");
    APPSPAWN_CHECK(ret == 0, return ret,
        "Set DataGroup config fail result: %{public}d, app: %{public}s", ret, context->bundleName);

    bool mountDestBundlePath = false;
    AppSpawnMsgDomainInfo *msgDomainInfo = (AppSpawnMsgDomainInfo *)GetSpawningMsgInfo(context, TLV_DOMAIN_INFO);
    if (msgDomainInfo != NULL) {
        mountDestBundlePath = (strcmp(msgDomainInfo->apl, APL_SYSTEM_BASIC) == 0) ||
            (strcmp(msgDomainInfo->apl, APL_SYSTEM_CORE) == 0);
    }
    if (mountDestBundlePath || (CheckSpawningMsgFlagSet(context, APP_FLAGS_ACCESS_BUNDLE_DIR) != 0)) {
        // need permission check for system app here
        const char *destBundlesPath = GetSandboxRealVar(context,
            BUFFER_FOR_TARGET, "/data/bundles/", context->sandboxPackagePath, NULL);
        CreateSandboxDir(destBundlesPath, FILE_MODE);
        MountArg mountArg = {PHYSICAL_APP_INSTALL_PATH, destBundlesPath, NULL, MS_REC | MS_BIND, NULL, MS_SLAVE};
        ret = SandboxMountPath(&mountArg);
        APPSPAWN_CHECK(ret == 0, return ret, "mount library failed %{public}d", ret);
    }
    return 0;
}

static int SetSandboxPackageNameConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    SandboxPackageNameNode *sandboxNode =
        (SandboxPackageNameNode *)GetSandboxSection(&sandbox->packageNameQueue, context->bundleName);
    if (sandboxNode != NULL) {
        int ret = MountSandboxConfig(context, sandbox, &sandboxNode->section, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }
    return 0;
}

static int SetSandboxSpawnFlagsConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_LOGV("Set spawning flags config");
    ListNode *node = sandbox->spawnFlagsQueue.front.next;
    while (node != &sandbox->spawnFlagsQueue.front) {
        SandboxFlagsNode *sandboxNode = (SandboxFlagsNode *)ListEntry(node, SandboxMountNode, node);
        if (!CheckSpawningMsgFlagSet(context, sandboxNode->flagIndex)) {  // match flags point
            node = node->next;
            continue;
        }

        int ret = MountSandboxConfig(context, sandbox, &sandboxNode->section, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
        node = node->next;
    }
    return 0;
}

static int SetSandboxPermissionConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_LOGV("Set permission config");
    ListNode *node = sandbox->permissionQueue.front.next;
    while (node != &sandbox->permissionQueue.front) {
        SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
        if (!CheckSpawningPermissionFlagSet(context, permissionNode->permissionIndex)) {
            node = node->next;
            continue;
        }

        APPSPAWN_LOGV("SetSandboxPermissionConfig permission %{public}s", permissionNode->section.name);
        int ret = MountSandboxConfig(context, sandbox, &permissionNode->section, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
        node = node->next;
    }
    return 0;
}

static int SetOverlayAppSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (!CheckSpawningMsgFlagSet(context, APP_FLAGS_OVERLAY)) {
        return 0;
    }
    int ret = ProcessExpandAppSandboxConfig(context, sandbox, "Overlay");
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    return 0;
}

static int SetBundleResourceSandboxConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    if (!CheckSpawningMsgFlagSet(context, APP_FLAGS_BUNDLE_RESOURCES)) {
        return 0;
    }
    const char *destPath = GetSandboxRealVar(context,
        BUFFER_FOR_TARGET, "/data/storage/bundle_resources/", context->sandboxPackagePath, NULL);
    CreateSandboxDir(destPath, FILE_MODE);
    MountArg mountArg = {
        "/data/service/el1/public/bms/bundle_resources/", destPath, NULL, MS_REC | MS_BIND, NULL, MS_SLAVE
    };
    int ret = SandboxMountPath(&mountArg);
    return ret;
}

static int32_t ChangeCurrentDir(const SandboxContext *context)
{
    int32_t ret = 0;
    ret = chdir(context->sandboxPackagePath);
    APPSPAWN_CHECK(ret == 0, return ret,
        "chdir failed, app: %{public}s, path: %{public}s errno: %{public}d",
        context->bundleName, context->sandboxPackagePath, errno);

    if (context->sandboxShared) {
        ret = chroot(context->sandboxPackagePath);
        APPSPAWN_CHECK(ret == 0, return ret,
            "chroot failed, path: %{public}s errno: %{public}d", context->sandboxPackagePath, errno);
        return ret;
    }
    ret = syscall(SYS_pivot_root, context->sandboxPackagePath, context->sandboxPackagePath);
    APPSPAWN_CHECK(ret == 0, return ret,
        "pivot root failed, path: %{public}s errno: %{public}d", context->sandboxPackagePath, errno);
    ret = umount2(".", MNT_DETACH);
    APPSPAWN_CHECK(ret == 0, return ret,
        "MNT_DETACH failed,  path: %{public}s errno: %{public}d", context->sandboxPackagePath, errno);
    APPSPAWN_LOGV("ChangeCurrentDir %{public}s ", context->sandboxPackagePath);
    return ret;
}

static int SandboxRootFolderCreateNoShare(
    const SandboxContext *context, const AppSpawnSandboxCfg *sandbox, bool remountProc)
{
    APPSPAWN_LOGV("SandboxRootFolderCreateNoShare %{public}s ", context->sandboxPackagePath);
    int ret = mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL);
    APPSPAWN_CHECK(ret == 0, return ret,
        "set propagation slave failed, app: %{public}s errno: %{public}d", context->sandboxPackagePath, errno);

    MountArg arg = {context->sandboxPackagePath, context->sandboxPackagePath, NULL, BASIC_MOUNT_FLAGS, NULL, MS_SLAVE};
    ret = SandboxMountPath(&arg);
    APPSPAWN_CHECK(ret == 0, return ret,
        "mount path failed, app: %{public}s errno: %{public}d", context->sandboxPackagePath, ret);
    return ret;
}

static int SandboxRootFolderCreate(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_LOGV("topSandboxSwitch %{public}d sandboxSwitch: %{public}d sandboxShared: %{public}d \n",
        sandbox->topSandboxSwitch, context->sandboxSwitch, context->sandboxShared);

    int ret = 0;
    if (sandbox->topSandboxSwitch == 0 || context->sandboxSwitch == 0) {
        ret = mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL);
        APPSPAWN_CHECK(ret == 0, return ret,
            "set propagation slave failed, app: %{public}s errno: %{public}d", context->sandboxPackagePath, errno);
        // bind mount "/" to /mnt/sandbox/<packageName> path
        // rootfs: to do more resources bind mount here to get more strict resources constraints
        ret = mount("/", context->sandboxPackagePath, NULL, BASIC_MOUNT_FLAGS, NULL);
        APPSPAWN_CHECK(ret == 0, return ret,
            "mount bind / failed, app: %{public}s errno: %{public}d", context->sandboxPackagePath, errno);
    } else if (!context->sandboxShared) {
        bool remountProc = !context->nwebspawn && ((sandbox->sandboxNsFlags & CLONE_NEWPID) == CLONE_NEWPID);
        ret = SandboxRootFolderCreateNoShare(context, sandbox, remountProc);
    }
    return ret;
}

int DumpCurrentDir(SandboxContext *context, const char *dirPath)
{
    DIR *pDir = opendir(dirPath);
    APPSPAWN_CHECK(pDir != NULL, return -1, "Read dir :%{public}s failed.%{public}d", dirPath, errno);

    struct dirent *dp;
    while ((dp = readdir(pDir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        if (dp->d_type == DT_DIR) {
            APPSPAWN_LOGV(" Current path %{public}s/%{public}s ", dirPath, dp->d_name);
            APPSPAWN_CHECK_ONLY_EXPER(snprintf_s(context->buffer[1].buffer, context->buffer[1].bufferLen, context->buffer[1].bufferLen - 1,
                "%s/%s", dirPath, dp->d_name) != -1, return -1);
            char *path = strdup(context->buffer[1].buffer);
            DumpCurrentDir(context, path);
            free(path);
        }
    }
    closedir(pDir);
    return 0;
}

int MountSandboxConfigs(const AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
{
    APPSPAWN_CHECK_ONLY_EXPER(property != NULL, return -1);
    APPSPAWN_CHECK(sandbox != NULL, return -1, "Failed to get sandbox for %{public}s", GetProcessName(property));

    SandboxContext *context = GetSandboxContext();
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return APPSPAWN_SYSTEM_ERROR);
    int ret = InitSandboxContext(context, sandbox, property, nwebspawn);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    do {
        ret = BuildRootPath((SandboxContext *)context, sandbox, NULL);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = BuildPackagePath(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        APPSPAWN_LOGV("Set sandbox config %{public}s", context->sandboxPackagePath);

        ret = StagedMountPreUnShare(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        CreateSandboxDir(context->sandboxPackagePath, FILE_MODE);
        // add pid to a new mnt namespace
        ret = unshare(CLONE_NEWNS);
        APPSPAWN_CHECK(ret == 0, break,
            "unshare failed, app: %{public}s errno: %{public}d", context->bundleName, errno);

        ret = SandboxRootFolderCreate(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        ret = StagedMountPostUnshare(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        ret = SetOverlayAppSandboxConfig(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = SetBundleResourceSandboxConfig(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        ret = ChangeCurrentDir(context);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        APPSPAWN_LOGV("Change root dir success %{public}s ", context->sandboxPackagePath);
    } while (0);

    if (getcwd(context->buffer[0].buffer, context->buffer[0].bufferLen) != NULL) {
        APPSPAWN_LOGE("current_path %{public}s\n", context->buffer[0].buffer);
    }
    APPSPAWN_LOGV("current_path %{public}s \n", context->buffer[0].buffer);
    DumpCurrentDir(context, "/data");
    DeleteSandboxContext(context);
    return ret;
}

int StagedMountSystemConst(const AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
{
    APPSPAWN_CHECK(sandbox != NULL, return -1, "Failed to get sandbox for %{public}s", GetProcessName(property));
    /**
     * system-const 处理逻辑
     *   root-dir "/mnt/sandbox/app-root/<currentUserId>"
     *   遍历system-const， 处理mount path   -- 可以清除配置
     *      src = mount-path.src-path
     *      dst = root-dir + mount-path.sandbox-path
     *
     *   遍历name-groups，处理mount path
     *      检查type，必须是 system-const
     *      如果存在 mount-paths-deps
     *          src = mount-path.src-path
     *          dst = root-dir + mount-paths-deps.sandbox-path + mount-path.sandbox-path
     *      否则：
     *          src = mount-path.src-path
     *          dst = root-dir + mount-path.sandbox-path
     */
    SandboxContext *context = GetSandboxContext();
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return APPSPAWN_SYSTEM_ERROR);
    int ret = InitSandboxContext(context, sandbox, property, nwebspawn);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    BuildRootPath((SandboxContext *)context, sandbox, NULL);

    SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "system-const");
    if (section != NULL) {
        ret = MountSandboxConfig(context, sandbox, section, 0);
    }
    DeleteSandboxContext(context);
    return ret;
}

int StagedMountPreUnShare(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_CHECK(sandbox != NULL && context != NULL, return -1, "Invalid sandbox or context");
    APPSPAWN_LOGV("Set sandbox config before unshare group count %{public}d", sandbox->depNodeCount);

    /**
     * 在unshare前处理mount-paths-deps 处理逻辑
     *   root-dir global.sandbox-root
     *   src-dir "/mnt/sandbox/app-common/<currentUserId>"
     *   遍历mount-paths-deps，处理mount-paths-deps
     *      src = mount-paths-deps.src-path
     *      dst = mount-paths-deps.sandbox-path
     *      如果设置no-exist，检查mount-paths 的src（实际路径） 是否不存在，
                则安mount-paths-deps.src-path 创建。 按shared方式挂载mount-paths-deps
     *      如果是 always，按shared方式挂载mount-paths-deps
     *      不配置 按always 处理
     *
     */
    int ret = 0;
    for (uint32_t i = 0; i < sandbox->depNodeCount; i++) {
        SandboxNameGroupNode *groupNode = sandbox->depGroupNodes[i];
        if (groupNode == NULL || groupNode->depNode == NULL) {
            continue;
        }
        APPSPAWN_LOGV("Set sandbox deps config %{public}s ", groupNode->section.name);
        // change source and target to real path
        ret = UpdateMountPathDepsPath(context, groupNode);
        APPSPAWN_CHECK(ret == 0, return ret,
            "Failed to update deps path name group %{public}s", groupNode->section.name);

        if (CheckAndCreateDepPath(context, groupNode)) {
            continue;
        }

        ret = DoSandboxPathNodeMount(context, &groupNode->section, groupNode->depNode, 0);
        if (ret != 0) {
            APPSPAWN_LOGE("Mount deps root fail %{public}s", groupNode->section.name);
            return ret;
        }

        if (groupNode->destType == SANDBOX_TAG_SYSTEM_CONST) {
            ret = MountSandboxConfig(context, sandbox, &groupNode->section, MOUNT_PATH_OP_UNMOUNT);
            APPSPAWN_CHECK(ret == 0, return ret,
                "Set system-const config fail result: %{public}d, app: %{public}s", ret, context->bundleName);
        }
    }
    return 0;
}

int StagedMountPostUnshare(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_CHECK(sandbox != NULL && context != NULL, return -1, "Invalid sandbox or context");
    APPSPAWN_LOGV("Set sandbox config after unshare ");
    /**
     * app-variable 处理逻辑
     *   root-dir global.sandbox-root
     *   app-variabl， 处理mount path
     *      src = mount-path.src-path
     *      dst = root-dir + mount-path.sandbox-path
     *   遍历name-groups，处理mount path
     *      如果存在 mount-paths-deps
     *          src = mount-path.src-path
     *          dst = root-dir + mount-paths-deps.sandbox-path + mount-path.sandbox-path
     *      否则：
     *          src = mount-path.src-path
     *          dst = root-dir + mount-path.sandbox-path
     */
    int ret = 0;
    SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "app-variable");
    if (section != NULL) {
        ret = MountSandboxConfig(context, sandbox, section, 0);
        APPSPAWN_CHECK(ret == 0, return ret,
            "Set app-variable config fail result: %{public}d, app: %{public}s", ret, context->bundleName);
    }
    if (!context->nwebspawn) {
        ret = SetExpandSandboxConfig(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }

    ret = SetSandboxSpawnFlagsConfig(context, sandbox);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = SetSandboxPackageNameConfig(context, sandbox);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = SetSandboxPermissionConfig(context, sandbox);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    return ret;
}

int UnmountSandboxConfigs(const AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_CHECK(sandbox != NULL, return -1, "Invalid sandbox or context");
    for (uint32_t i = 0; i < sandbox->depNodeCount; i++) {
        SandboxNameGroupNode *groupNode = sandbox->depGroupNodes[i];
        if (groupNode == NULL || groupNode->depNode == NULL) {
            continue;
        }
        APPSPAWN_LOGV("Unmount sandbox config sandbox path %{public}s ", groupNode->depNode->target);
        // unmount this deps
        umount2(groupNode->depNode->target, MNT_DETACH);
    }
    return 0;
}
