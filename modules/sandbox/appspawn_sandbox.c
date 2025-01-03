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
#include "sandbox_adapter.h"

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
#include "securec.h"

#include "appspawn_msg.h"
#include "appspawn_utils.h"
#ifdef WITH_DLP
#include "dlp_fuse_fd.h"
#endif
#include "init_utils.h"
#include "parameter.h"
#include "appspawn_permission.h"

#ifdef WITH_SELINUX
#ifdef APPSPAWN_MOUNT_TMPSHM
#include "policycoreutils.h"
#endif // APPSPAWN_MOUNT_TMPSHM
#endif // WITH_SELINUX

#define USER_ID_SIZE 16
#define DIR_MODE     0711
#define LOCK_STATUS_PARAM_SIZE     64
#define LOCK_STATUS_SIZE     16
#define DEV_SHM_DIR "/dev/shm/"

static inline void SetMountPathOperation(uint32_t *operation, uint32_t index)
{
    *operation |= (1 << index);
}

static inline bool CheckSpawningMsgFlagSet(const SandboxContext *context, uint32_t index)
{
    APPSPAWN_CHECK(context->message != NULL, return false, "Invalid property for type %{public}u", TLV_MSG_FLAGS);
    return CheckAppSpawnMsgFlag(context->message, TLV_MSG_FLAGS, index);
}

APPSPAWN_STATIC inline bool CheckSpawningPermissionFlagSet(const SandboxContext *context, uint32_t index)
{
    APPSPAWN_CHECK(context != NULL && context->message != NULL,
        return NULL, "Invalid property for type %{public}u", TLV_PERMISSION);
    return CheckAppSpawnMsgFlag(context->message, TLV_PERMISSION, index);
}

APPSPAWN_STATIC bool CheckDirRecursive(const char *path)
{
    char buffer[PATH_MAX] = {0};
    const char slash = '/';
    const char *p = path;
    char *curPos = strchr(path, slash);
    while (curPos != NULL) {
        int len = curPos - p;
        p = curPos + 1;
        if (len == 0) {
            curPos = strchr(p, slash);
            continue;
        }
        int ret = memcpy_s(buffer, PATH_MAX, path, p - path - 1);
        APPSPAWN_CHECK(ret == 0, return false, "Failed to copy path");
        ret = access(buffer, F_OK);
        APPSPAWN_CHECK(ret == 0, return false, "Dir not exit %{public}s errno: %{public}d", buffer, errno);
        curPos = strchr(p, slash);
    }
    int ret = access(path, F_OK);
    APPSPAWN_CHECK(ret == 0, return false, "Dir not exit %{public}s errno: %{public}d", buffer, errno);
    return true;
}

int SandboxMountPath(const MountArg *arg)
{
    APPSPAWN_CHECK(arg != NULL && arg->originPath != NULL && arg->destinationPath != NULL,
        return APPSPAWN_ARG_INVALID, "Invalid arg ");

    APPSPAWN_LOGV("Bind mount arg: '%{public}s' '%{public}s' %{public}x '%{public}s' %{public}s => %{public}s",
        arg->fsType, arg->mountSharedFlag == MS_SHARED ? "MS_SHARED" : "MS_SLAVE",
        (uint32_t)arg->mountFlags, arg->options, arg->originPath, arg->destinationPath);

    int ret = mount(arg->originPath, arg->destinationPath, arg->fsType, arg->mountFlags, arg->options);
    if (ret != 0) {
        APPSPAWN_LOGW("errno is: %{public}d, bind mount %{public}s => %{public}s",
            errno, arg->originPath, arg->destinationPath);
        if (strstr(arg->originPath, "/data/app/el1/") != NULL || strstr(arg->originPath, "/data/app/el2/") != NULL) {
            CheckDirRecursive(arg->originPath);
        }
        return errno;
    }
    ret = mount(NULL, arg->destinationPath, NULL, arg->mountSharedFlag, NULL);
    if (ret != 0) {
        APPSPAWN_LOGW("errno is: %{public}d, bind mount %{public}s => %{public}s",
            errno, arg->originPath, arg->destinationPath);
        return errno;
    }
    return 0;
}

static int BuildRootPath(char *buffer, uint32_t bufferLen, const AppSpawnSandboxCfg *sandbox, uid_t uid)
{
    int ret = 0;
    int len = 0;
    uint32_t currLen = 0;
    uint32_t userIdLen = sizeof(PARAMETER_USER_ID) - 1;
    uint32_t rootLen = strlen(sandbox->rootPath);
    char *rootPath = strstr(sandbox->rootPath, PARAMETER_USER_ID);
    if (rootPath == NULL) {
        len = sprintf_s(buffer, bufferLen, "%s/%d", sandbox->rootPath, uid);
    } else {
        ret = memcpy_s(buffer, bufferLen, sandbox->rootPath, rootPath - sandbox->rootPath);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to copy root path %{public}s", sandbox->rootPath);
        currLen = rootPath - sandbox->rootPath;

        if (rootLen > (currLen + userIdLen)) {
            len = sprintf_s(buffer + currLen, bufferLen - currLen, "%d%s",
                uid, sandbox->rootPath + currLen + userIdLen);
        } else {
            len = sprintf_s(buffer + currLen, bufferLen - currLen, "%d", uid);
        }
        APPSPAWN_CHECK(len > 0 && ((uint32_t)len < (bufferLen - currLen)), return ret,
            "Failed to format root path %{public}s", sandbox->rootPath);
        currLen += (uint32_t)len;
    }
    buffer[currLen] = '\0';
    return 0;
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
        context->sandboxSwitch = 1;
        context->sandboxShared = false;
        context->message = NULL;
        context->rootPath = NULL;
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
    if (context == g_sandboxContext) {
        g_sandboxContext = NULL;
    }
    free(context);
}

static bool NeedNetworkIsolated(SandboxContext *context, const AppSpawningCtx *property)
{
    int developerMode = IsDeveloperModeOpen();
    if (CheckSpawningMsgFlagSet(context, APP_FLAGS_ISOLATED_SANDBOX) && !developerMode) {
        return true;
    }

    if (CheckSpawningMsgFlagSet(context, APP_FLAGS_ISOLATED_NETWORK)) {
        uint32_t len = 0;
        char *extensionType = GetAppPropertyExt(property, MSG_EXT_NAME_EXTENSION_TYPE, &len);
        if (extensionType == NULL || extensionType[0] == '\0' || !developerMode) {
            return true;
        }
    }

    return false;
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
    context->dlpBundle = strcmp(GetProcessName(property), "com.ohos.dlpmanager") == 0;
    context->appFullMountEnable = sandbox->appFullMountEnable;

    context->sandboxSwitch = 1;
    context->sandboxShared = false;
    SandboxPackageNameNode *packageNode = (SandboxPackageNameNode *)GetSandboxSection(
        &sandbox->packageNameQueue, context->bundleName);
    if (packageNode) {
        context->sandboxShared = packageNode->section.sandboxShared;
    }
    context->message = property->message;

    context->sandboxNsFlags = CLONE_NEWNS;

    if (NeedNetworkIsolated(context, property)) {
        context->sandboxNsFlags |= sandbox->sandboxNsFlags & CLONE_NEWNET ? CLONE_NEWNET : 0;
    }

    ListNode *node = sandbox->permissionQueue.front.next;
    while (node != &sandbox->permissionQueue.front) {
        SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
        if (permissionNode == NULL) {
            return -1;
        }
        context->sandboxShared = permissionNode->section.sandboxShared;
        node = node->next;
    }

    // root path
    const char *rootPath = GetSandboxRealVar(context, BUFFER_FOR_SOURCE, sandbox->rootPath, NULL, NULL);
    if (rootPath) {
        context->rootPath = strdup(rootPath);
    }
    if (context->rootPath == NULL) {
        DeleteSandboxContext(context);
        return -1;
    }
    return 0;
}

static VarExtraData *GetVarExtraData(const SandboxContext *context, const SandboxSection *section)
{
    static VarExtraData extraData;
    (void)memset_s(&extraData, sizeof(extraData), 0, sizeof(extraData));
    extraData.sandboxTag = GetSectionType(section);
    if (GetSectionType(section) == SANDBOX_TAG_NAME_GROUP) {
        SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)section;
        extraData.data.depNode = groupNode->depNode;
    }
    return &extraData;
}

static uint32_t GetMountArgs(const SandboxContext *context,
    const PathMountNode *sandboxNode, uint32_t operation, MountArg *args)
{
    uint32_t category = sandboxNode->category;
    const MountArgTemplate *tmp = GetMountArgTemplate(category);
    if (tmp == 0) {
        return MOUNT_TMP_DEFAULT;
    }
    args->fsType = tmp->fsType;
    args->options = tmp->options;
    args->mountFlags = tmp->mountFlags;
    args->mountSharedFlag = (sandboxNode->mountSharedFlag) ? MS_SHARED : tmp->mountSharedFlag;
    return category;
}

APPSPAWN_STATIC int CheckSandboxMountNode(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation)
{
    if (sandboxNode->source == NULL || sandboxNode->target == NULL) {
        APPSPAWN_LOGW("Invalid mount config section %{public}s", section->name);
        return 0;
    }
    // special handle wps and don't use /data/app/xxx/<Package> config
    if (CHECK_FLAGS_BY_INDEX(operation, SANDBOX_TAG_SPAWN_FLAGS)) {  // flags-point
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

static int32_t SandboxMountFusePath(const SandboxContext *context, const MountArg *args)
{
    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSpawningMsgInfo(context, TLV_DAC_INFO);
    APPSPAWN_CHECK(info != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, context->bundleName);

    // umount fuse path, make sure that sandbox path is not a mount point
    umount2(args->destinationPath, MNT_DETACH);

    int fd = open("/dev/fuse", O_RDWR);
    APPSPAWN_CHECK(fd != -1, return -EINVAL,
        "open /dev/fuse failed, errno: %{public}d sandbox path %{public}s", errno, args->destinationPath);

    char options[OPTIONS_MAX_LEN] = {0};
    int ret = sprintf_s(options, sizeof(options), "fd=%d,"
        "rootmode=40000,user_id=%d,group_id=%d,allow_other,"
        "context=\"u:object_r:dlp_fuse_file:s0\","
        "fscontext=u:object_r:dlp_fuse_file:s0", fd, info->uid, info->gid);
    APPSPAWN_CHECK(ret > 0, return APPSPAWN_ERROR_UTILS_MEM_FAIL, "sprintf options fail");

    APPSPAWN_LOGV("Bind mount dlp fuse \n "
        "mount arg: '%{public}s' '%{public}s' %{public}x '%{public}s' %{public}s => %{public}s",
        args->fsType, args->mountSharedFlag == MS_SHARED ? "MS_SHARED" : "MS_SLAVE",
        (uint32_t)args->mountFlags, options, args->originPath, args->destinationPath);

    // To make sure destinationPath exist
    CreateSandboxDir(args->destinationPath, FILE_MODE);
    MountArg mountArg = {args->originPath, args->destinationPath, args->fsType, args->mountFlags, options, MS_SHARED};
    ret = SandboxMountPath(&mountArg);
    if (ret != 0) {
        close(fd);
        return -1;
    }
    /* set DLP_FUSE_FD  */
#ifdef WITH_DLP
    SetDlpFuseFd(fd);
#endif
    return 0;
}

APPSPAWN_STATIC void CheckAndCreateSandboxFile(const char *file)
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

APPSPAWN_STATIC void CreateDemandSrc(const SandboxContext *context, const PathMountNode *sandboxNode,
    const MountArg *args)
{
    if (!sandboxNode->createDemand) {
        return;
    }
    CheckAndCreateSandboxFile(args->originPath);
    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSpawningMsgInfo(context, TLV_DAC_INFO);
    APPSPAWN_CHECK(info != NULL, return,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, context->bundleName);

    // chmod
    uid_t uid = sandboxNode->demandInfo->uid != INVALID_UID ? sandboxNode->demandInfo->uid : info->uid;
    gid_t gid = sandboxNode->demandInfo->gid != INVALID_UID ? sandboxNode->demandInfo->gid : info->gid;
    int ret = chown(args->originPath, uid, gid);
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to chown %{public}s errno: %{public}d", args->originPath, errno);
    }
    if (sandboxNode->demandInfo->mode != INVALID_UID) {
        ret = chmod(args->originPath, sandboxNode->demandInfo->mode);
        if (ret != 0) {
            APPSPAWN_LOGE("Failed to chmod %{public}s errno: %{public}d", args->originPath, errno);
        }
    }
}

APPSPAWN_STATIC const char *GetRealSrcPath(const SandboxContext *context, const char *source, VarExtraData *extraData)
{
    bool hasPackageName = strstr(source, "<variablePackageName>") != NULL;
    extraData->variablePackageName = (char *)context->bundleName;
    const char *originPath = GetSandboxRealVar(context, BUFFER_FOR_SOURCE, source, NULL, extraData);
    if (originPath == NULL) {
        return NULL;
    }
    if (hasPackageName && CheckSpawningMsgFlagSet(context, APP_FLAGS_ATOMIC_SERVICE)) {
        const char *varPackageName = strrchr(originPath, '/') ? strrchr(originPath, '/') + 1 : originPath;
        MakeAtomicServiceDir(context, originPath, varPackageName);
    }
    return originPath;
}

// 设置挂载参数options
static int32_t SetMountArgsOption(const SandboxContext *context, uint32_t category, uint32_t operation, MountArg *args)
{
    if ((category != MOUNT_TMP_DAC_OVERRIDE) && (category != MOUNT_TMP_DAC_OVERRIDE_DELETE)) {
        return 0;
    }

    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSpawningMsgInfo(context, TLV_DAC_INFO);
    if (info == NULL) {
        APPSPAWN_LOGE("Get msg dac info failed");
        return APPSPAWN_ARG_INVALID;
    }

    char options[OPTIONS_MAX_LEN] = {0};
    int len = sprintf_s(options, OPTIONS_MAX_LEN, "%s%s%d", args->options, SHAREFS_OPTION_USER, info->uid / UID_BASE);
    if (len <= 0) {
        APPSPAWN_LOGE("sprintf_s failed");
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }
    args->options = options;
    return 0;
}

// 根据沙盒配置文件中挂载类别进行挂载
static int DoSandboxMountByCategory(const SandboxContext *context, const PathMountNode *sandboxNode,
                                    MountArg *args, uint32_t operation)
{
    int ret = 0;
    uint32_t category = GetMountArgs(context, sandboxNode, operation, args);
    if (CHECK_FLAGS_BY_INDEX(operation, SANDBOX_TAG_PERMISSION) ||
        CHECK_FLAGS_BY_INDEX(operation, SANDBOX_TAG_SPAWN_FLAGS)) {
        ret = SetMountArgsOption(context, category, operation, args);
        if (ret != 0) {
            APPSPAWN_LOGE("set mount arg option fail. ret: %{public}d", ret);
            return ret;
        }
    }
    if (category == MOUNT_TMP_DLP_FUSE || category == MOUNT_TMP_FUSE) {
        ret = SandboxMountFusePath(context, args);
    } else {
        ret = SandboxMountPath(args);
    }
    return ret;
}

static int DoSandboxPathNodeMount(const SandboxContext *context,
    const SandboxSection *section, const PathMountNode *sandboxNode, uint32_t operation)
{
    if (CheckSandboxMountNode(context, section, sandboxNode, operation) == 0) {
        return 0;
    }

    MountArg args = {};
    uint32_t category = GetMountArgs(context, sandboxNode, operation, &args);
    VarExtraData *extraData = GetVarExtraData(context, section);
    args.originPath = GetRealSrcPath(context, sandboxNode->source, extraData);
    // dest
    extraData->operation = operation;  // only destinationPath
    // 对name group的节点，需要对目的沙盒进行特殊处理，不能带root-dir
    if (CHECK_FLAGS_BY_INDEX(operation, SANDBOX_TAG_NAME_GROUP) &&
        CHECK_FLAGS_BY_INDEX(operation, MOUNT_PATH_OP_ONLY_SANDBOX)) {
        args.destinationPath = GetSandboxRealVar(context, BUFFER_FOR_TARGET, sandboxNode->target, NULL, extraData);
    } else {
        args.destinationPath = GetSandboxRealVar(context,
            BUFFER_FOR_TARGET, sandboxNode->target, context->rootPath, extraData);
    }
    APPSPAWN_CHECK(args.originPath != NULL && args.destinationPath != NULL,
        return APPSPAWN_ARG_INVALID, "Invalid path %{public}s %{public}s", args.originPath, args.destinationPath);

    if (sandboxNode->sandboxNode.type == SANDBOX_TAG_MOUNT_FILE) {
        CheckAndCreateSandboxFile(args.destinationPath);
    } else {
        if (access(args.destinationPath, F_OK) != 0) {
            CreateSandboxDir(args.destinationPath, FILE_MODE);
        }
    }

    CreateDemandSrc(context, sandboxNode, &args);

    int ret = 0;
    if (CHECK_FLAGS_BY_INDEX(operation, MOUNT_PATH_OP_UNMOUNT)) {  // unmount this deps
        APPSPAWN_LOGV("umount2 %{public}s", args.destinationPath);
        ret = umount2(args.destinationPath, MNT_DETACH);
        APPSPAWN_CHECK_ONLY_LOG(ret == 0, "Failed to umount %{public}s errno %{public}d", args.destinationPath, errno);
    }

    ret = DoSandboxMountByCategory(context, sandboxNode, &args, operation);
    if (ret != 0 && sandboxNode->checkErrorFlag) {
        APPSPAWN_LOGE("Failed to mount config, section: %{public}s result: %{public}d category: %{public}d",
            section->name, ret, category);
        return ret;
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
    APPSPAWN_LOGV("symlink from %{public}s to %{public}s", target, linkName);
    if (access(linkName, F_OK) == 0) {
        if (rmdir(linkName) != 0) {
            APPSPAWN_LOGW("linkName %{public}s already exist and rmdir failed, errno %{public}d", linkName, errno);
        }
    }
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
    return 0;
}

static int DoSandboxNodeMount(const SandboxContext *context, const SandboxSection *section, uint32_t operation)
{
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
                if (!CHECK_FLAGS_BY_INDEX(operation, MOUNT_PATH_OP_SYMLINK)) {
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

static bool IsUnlockStatus(uint32_t uid)
{
    const int userIdBase = UID_BASE;
    uid = uid / userIdBase;
    if (uid == 0) {  // uid = 0 不涉及加密目录的挂载
        return true;
    }

    char lockStatusParam[LOCK_STATUS_PARAM_SIZE] = {0};
    char userLockStatus[LOCK_STATUS_SIZE] = {0};
    int ret = snprintf_s(lockStatusParam, sizeof(lockStatusParam), sizeof(lockStatusParam) - 1,
        "startup.appspawn.lockstatus_%u", uid);
    APPSPAWN_CHECK(ret > 0, return false, "get lock status param failed, errno %{public}d", errno);
    ret = GetParameter(lockStatusParam, "1", userLockStatus, sizeof(userLockStatus));
    APPSPAWN_LOGI("get param %{public}s %{public}s", lockStatusParam, userLockStatus);
    if (ret > 0 && (strcmp(userLockStatus, "0") == 0)) {     // 0：解密状态  1：加密状态
        return true;
    }
    return false;
}

static void MountDir(AppSpawnMsgDacInfo *info, const char *bundleName, const char *rootPath, const char *targetPath)
{
    if (info == NULL || bundleName == NULL || rootPath == NULL || targetPath == NULL) {
        return;
    }

    const int userIdBase = UID_BASE;
    size_t allPathSize = strlen(rootPath) + strlen(targetPath) + strlen(bundleName) + 2;
    allPathSize += USER_ID_SIZE;
    char *path = (char *)malloc(sizeof(char) * (allPathSize));
    (void)memset_s(path, allPathSize, 0, allPathSize);
    APPSPAWN_CHECK(path != NULL, return, "Failed to malloc path");
    int len = sprintf_s(path, allPathSize, "%s%u/%s%s", rootPath, info->uid / userIdBase, bundleName, targetPath);
    APPSPAWN_CHECK(len > 0 && ((size_t)len < allPathSize), free(path);
        return, "Failed to get sandbox path");

    if (access(path, F_OK) == 0) {
        free(path);
        return;
    }

    MakeDirRec(path, DIR_MODE, 1);
    if (mount(path, path, NULL, MS_BIND | MS_REC, NULL) != 0) {
        APPSPAWN_LOGI("bind mount %{public}s failed, error %{public}d", path, errno);
        free(path);
        return;
    }
    if (mount(NULL, path, NULL, MS_SHARED, NULL) != 0) {
        APPSPAWN_LOGI("mount path %{public}s to shared failed, errno %{public}d", path, errno);
    } else {
        APPSPAWN_LOGI("mount path %{public}s to shared success", path);
    }

    free(path);
}

static const MountSharedTemplate MOUNT_SHARED_MAP[] = {
    {"/data/storage/el2", NULL},
    {"/data/storage/el3", NULL},
    {"/data/storage/el4", NULL},
    {"/data/storage/el5", "ohos.permission.PROTECT_SCREEN_LOCK_DATA"},
};

static int MountInShared(const AppSpawnMsgDacInfo *info, const char *rootPath, const char *src, const char *target)
{
    if (info == NULL) {
        return APPSPAWN_ARG_INVALID;
    }

    char path[MAX_SANDBOX_BUFFER] = {0};
    int ret = snprintf_s(path, MAX_SANDBOX_BUFFER, MAX_SANDBOX_BUFFER - 1, "%s/%u/app-root/%s", rootPath,
                         info->uid / UID_BASE, target);
    if (ret <= 0) {
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    char currentUserPath[MAX_SANDBOX_BUFFER] = {0};
    ret = snprintf_s(currentUserPath, MAX_SANDBOX_BUFFER, MAX_SANDBOX_BUFFER - 1, "%s/currentUser", path);
    if (ret <= 0) {
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    if (access(currentUserPath, F_OK) == 0) {
        return 0;
    }

    ret = MakeDirRec(path, DIR_MODE, 1);
    if (ret != 0) {
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    if (mount(src, path, NULL, MS_BIND | MS_REC, NULL) != 0) {
        APPSPAWN_LOGI("bind mount %{public}s to %{public}s failed, error %{public}d", src, path, errno);
        return APPSPAWN_SANDBOX_ERROR_MOUNT_FAIL;
    }
    if (mount(NULL, path, NULL, MS_SHARED, NULL) != 0) {
        APPSPAWN_LOGI("mount path %{public}s to shared failed, errno %{public}d", path, errno);
        return APPSPAWN_SANDBOX_ERROR_MOUNT_FAIL;
    }

    return 0;
}

static int SharedMountInSharefs(const AppSpawnMsgDacInfo *info, const char *rootPath,
                                const char *src, const char *target)
{
    char currentUserPath[MAX_SANDBOX_BUFFER] = {0};
    int ret = snprintf_s(currentUserPath, MAX_SANDBOX_BUFFER, MAX_SANDBOX_BUFFER - 1, "%s/currentUser", target);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf_s currentUserPath failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    if (access(currentUserPath, F_OK) == 0) {
        return 0;
    }

    ret = MakeDirRec(target, DIR_MODE, 1);
    if (ret != 0) {
        return APPSPAWN_SANDBOX_ERROR_MKDIR_FAIL;
    }

    char options[OPTIONS_MAX_LEN] = {0};
    ret = snprintf_s(options, OPTIONS_MAX_LEN, OPTIONS_MAX_LEN - 1, "override_support_delete,user_id=%d",
                     info->uid / UID_BASE);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf_s options failed, errno %{public}d", errno);
        return APPSPAWN_ERROR_UTILS_MEM_FAIL;
    }

    if (mount(src, target, "sharefs", MS_NODEV, options) != 0) {
        APPSPAWN_LOGE("sharefs mount %{public}s to %{public}s failed, error %{public}d",
                      src, target, errno);
        return APPSPAWN_SANDBOX_ERROR_MOUNT_FAIL;
    }
    if (mount(NULL, target, NULL, MS_SHARED, NULL) != 0) {
        APPSPAWN_LOGE("mount path %{public}s to shared failed, errno %{public}d", target, errno);
        return APPSPAWN_SANDBOX_ERROR_MOUNT_FAIL;
    }

    return 0;
}

static void UpdateStorageDir(const SandboxContext *context, AppSpawnSandboxCfg *sandbox, const AppSpawnMsgDacInfo *info)
{
    const char mntUser[] = "/mnt/user";
    const char nosharefsDocs[] = "nosharefs/docs";
    const char sharefsDocs[] = "sharefs/docs";
    const char rootPath[] = "/mnt/sandbox";
    const char userPath[] = "/storage/Users";

    /* /mnt/user/<currentUserId>/nosharefs/Docs */
    char nosharefsDocsDir[MAX_SANDBOX_BUFFER] = {0};
    int ret = snprintf_s(nosharefsDocsDir, MAX_SANDBOX_BUFFER, MAX_SANDBOX_BUFFER - 1, "%s/%d/%s",
                         mntUser, info->uid / UID_BASE, nosharefsDocs);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf_s nosharefsDocsDir failed, errno %{public}d", errno);
        return;
    }

    /* /mnt/user/<currentUserId>/sharefs/Docs */
    char sharefsDocsDir[MAX_SANDBOX_BUFFER] = {0};
    ret = snprintf_s(sharefsDocsDir, MAX_SANDBOX_BUFFER, MAX_SANDBOX_BUFFER - 1, "%s/%d/%s",
                     mntUser, info->uid / UID_BASE, sharefsDocs);
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf_s sharefsDocsDir failed, errno %{public}d", errno);
        return;
    }

    int index = GetPermissionIndexInQueue(&sandbox->permissionQueue, FILE_ACCESS_MANAGER_MODE);
    int res = CheckSpawningPermissionFlagSet(context, index);
    if (res == 0) {
        char storageUserPath[MAX_SANDBOX_BUFFER] = {0};
        ret = snprintf_s(storageUserPath, MAX_SANDBOX_BUFFER, MAX_SANDBOX_BUFFER - 1, "%s/%d/app-root/%s", rootPath,
                         info->uid / UID_BASE, userPath);
        if (ret <= 0) {
            APPSPAWN_LOGE("snprintf_s storageUserPath failed, errno %{public}d", errno);
            return;
        }
        /* mount /mnt/user/<currentUserId>/sharefs/docs to /mnt/sandbox/<currentUserId>/app-root/storage/Users */
        ret = SharedMountInSharefs(info, rootPath, sharefsDocsDir, storageUserPath);
    } else {
        /* mount /mnt/user/<currentUserId>/nosharefs/docs to /mnt/sandbox/<currentUserId>/app-root/storage/Users */
        ret = MountInShared(info, rootPath, nosharefsDocsDir, userPath);
    }
    if (ret != 0) {
        APPSPAWN_LOGE("Update storage dir, ret %{public}d", ret);
    }
    APPSPAWN_LOGI("Update %{public}s storage dir success", res == 0 ? "sharefs dir" : "no sharefs dir");
}

static void MountDirToShared(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    const char rootPath[] = "/mnt/sandbox/";
    const char nwebPath[] = "/mnt/nweb";
    const char nwebTmpPath[] = "/mnt/nweb/tmp";
    const char appRootName[] = "app-root";
    AppSpawnMsgDacInfo *info = (AppSpawnMsgDacInfo *)GetSpawningMsgInfo(context, TLV_DAC_INFO);
    if (info == NULL || context->bundleName == NULL) {
        return;
    }

    MountDir(info, appRootName, rootPath, nwebPath);
    MountDir(info, appRootName, rootPath, nwebTmpPath);

    if (IsUnlockStatus(info->uid)) {
        return;
    }

    UpdateStorageDir(context, sandbox, info);

    int length = sizeof(MOUNT_SHARED_MAP) / sizeof(MOUNT_SHARED_MAP[0]);
    for (int i = 0; i < length; i++) {
        if (MOUNT_SHARED_MAP[i].permission == NULL) {
            MountDir(info, context->bundleName, rootPath, MOUNT_SHARED_MAP[i].sandboxPath);
        } else {
            int index = GetPermissionIndexInQueue(&sandbox->permissionQueue, MOUNT_SHARED_MAP[i].permission);
            APPSPAWN_LOGV("mount dir on lock mountPermissionFlags %{public}d", index);
            if (CheckSpawningPermissionFlagSet(context, index)) {
                MountDir(info, context->bundleName, rootPath, MOUNT_SHARED_MAP[i].sandboxPath);
            }
        }
    }
    char lockSbxPathStamp[MAX_SANDBOX_BUFFER] = { 0 };
    int ret = 0;
    if (CheckSpawningMsgFlagSet(context, APP_FLAGS_ISOLATED_SANDBOX_TYPE) != 0) {
        ret = snprintf_s(lockSbxPathStamp, MAX_SANDBOX_BUFFER, MAX_SANDBOX_BUFFER - 1, "%s%d/isolated/%s_locked",
                         rootPath, info->uid / UID_BASE, context->bundleName);
    } else {
        ret = snprintf_s(lockSbxPathStamp, MAX_SANDBOX_BUFFER, MAX_SANDBOX_BUFFER - 1, "%s%d/%s_locked",
                         rootPath, info->uid / UID_BASE, context->bundleName);
    }
    if (ret <= 0) {
        APPSPAWN_LOGE("snprintf_s lock sandbox path stamp failed");
        return;
    }

    CreateSandboxDir(lockSbxPathStamp, FILE_MODE);
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

static bool CheckAndCreateDepPath(const SandboxContext *context, const SandboxNameGroupNode *groupNode)
{
    PathMountNode *mountNode = (PathMountNode *)GetFirstSandboxMountNode(&groupNode->section);
    if (mountNode == NULL) {
        return false;
    }

    // 这里可能需要替换deps的数据
    VarExtraData *extraData = GetVarExtraData(context, &groupNode->section);
    const char *srcPath = GetSandboxRealVar(context, BUFFER_FOR_SOURCE, mountNode->source, NULL, extraData);
    if (srcPath == NULL) {
        return false;
    }
    if (access(srcPath, F_OK) == 0) {
        APPSPAWN_LOGV("Src path %{public}s exist, do not mount", srcPath);
        return true;
    }
    // 不存在，则创建并挂载
    APPSPAWN_LOGV("Mount depended source: %{public}s", groupNode->depNode->source);
    CreateSandboxDir(groupNode->depNode->source, FILE_MODE);
    return false;
}

static int MountSandboxConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *sandbox, const SandboxSection *section, uint32_t op)
{
    uint32_t operation = (op != MOUNT_PATH_OP_NONE) ? op : 0;
    SetMountPathOperation(&operation, section->sandboxNode.type);
    // if sandbox switch is off, don't do symlink work again
    if (context->sandboxSwitch && sandbox->topSandboxSwitch) {
        SetMountPathOperation(&operation, MOUNT_PATH_OP_SYMLINK);
    }

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
        if (groupNode->depMounted != 1) {
            SetMountPathOperation(&operation, MOUNT_PATH_OP_REPLACE_BY_SANDBOX);
        }
        SetMountPathOperation(&operation, SANDBOX_TAG_NAME_GROUP);
        ret = DoSandboxNodeMount(context, &groupNode->section, operation);
        APPSPAWN_CHECK(ret == 0, return ret,
            "Mount name group %{public}s fail result: %{public}d", groupNode->section.name, ret);
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
            BUFFER_FOR_TARGET, "/data/bundles/", context->rootPath, NULL);
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
        int ret = MountSandboxConfig(context, sandbox, &sandboxNode->section, MOUNT_PATH_OP_NONE);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }
    return 0;
}

static int SetSandboxSpawnFlagsConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    ListNode *node = sandbox->spawnFlagsQueue.front.next;
    while (node != &sandbox->spawnFlagsQueue.front) {
        SandboxFlagsNode *sandboxNode = (SandboxFlagsNode *)ListEntry(node, SandboxMountNode, node);
        // match flags point
        if (sandboxNode->flagIndex == 0 || !CheckSpawningMsgFlagSet(context, sandboxNode->flagIndex)) {
            node = node->next;
            continue;
        }

        int ret = MountSandboxConfig(context, sandbox, &sandboxNode->section, MOUNT_PATH_OP_NONE);
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

        APPSPAWN_LOGV("SetSandboxPermissionConfig permission %{public}d %{public}s",
            permissionNode->permissionIndex, permissionNode->section.name);
        int ret = MountSandboxConfig(context, sandbox, &permissionNode->section, MOUNT_PATH_OP_NONE);
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
        BUFFER_FOR_TARGET, "/data/storage/bundle_resources/", context->rootPath, NULL);
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
    ret = chdir(context->rootPath);
    APPSPAWN_CHECK(ret == 0, return ret,
        "chdir failed, app: %{public}s, path: %{public}s errno: %{public}d",
        context->bundleName, context->rootPath, errno);

    if (context->sandboxShared) {
        ret = chroot(context->rootPath);
        APPSPAWN_CHECK(ret == 0, return ret,
            "chroot failed, path: %{public}s errno: %{public}d", context->rootPath, errno);
        return ret;
    }
    ret = syscall(SYS_pivot_root, context->rootPath, context->rootPath);
    APPSPAWN_CHECK(ret == 0, return ret,
        "pivot root failed, path: %{public}s errno: %{public}d", context->rootPath, errno);
    ret = umount2(".", MNT_DETACH);
    APPSPAWN_CHECK(ret == 0, return ret,
        "MNT_DETACH failed,  path: %{public}s errno: %{public}d", context->rootPath, errno);
    APPSPAWN_LOGV("ChangeCurrentDir %{public}s ", context->rootPath);
    return ret;
}

static int SandboxRootFolderCreateNoShare(
    const SandboxContext *context, const AppSpawnSandboxCfg *sandbox, bool remountProc)
{
    APPSPAWN_LOGV("SandboxRootFolderCreateNoShare %{public}s ", context->rootPath);
    int ret = mount(NULL, "/", NULL, MS_REC | MS_SLAVE, NULL);
    APPSPAWN_CHECK(ret == 0, return ret,
        "set propagation slave failed, app: %{public}s errno: %{public}d", context->rootPath, errno);

    MountArg arg = {context->rootPath, context->rootPath, NULL, BASIC_MOUNT_FLAGS, NULL, MS_SLAVE};
    ret = SandboxMountPath(&arg);
    APPSPAWN_CHECK(ret == 0, return ret,
        "mount path failed, app: %{public}s errno: %{public}d", context->rootPath, ret);
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
            "set propagation slave failed, app: %{public}s errno: %{public}d", context->rootPath, errno);
        // bind mount "/" to /mnt/sandbox/<packageName> path
        // rootfs: to do more resources bind mount here to get more strict resources constraints
        ret = mount("/", context->rootPath, NULL, BASIC_MOUNT_FLAGS, NULL);
        APPSPAWN_CHECK(ret == 0, return ret,
            "mount bind / failed, app: %{public}s errno: %{public}d", context->rootPath, errno);
    } else if (!context->sandboxShared) {
        bool remountProc = !context->nwebspawn && ((sandbox->sandboxNsFlags & CLONE_NEWPID) == CLONE_NEWPID);
        ret = SandboxRootFolderCreateNoShare(context, sandbox, remountProc);
    }
    return ret;
}

static bool IsSandboxMounted(const AppSpawnSandboxCfg *sandbox, const char *name, const char *rootPath)
{
    char path[PATH_MAX] = {};
    int len = sprintf_s(path, sizeof(path), "%s%s", rootPath, SANDBOX_STAMP_FILE_SUFFIX);
    APPSPAWN_CHECK(len > 0, return false, "Failed to format path");

    FILE *f = fopen(path, "rb");
    if (f != NULL) {
        fclose(f);
#ifndef APPSPAWN_TEST
        return true;
#endif
    }
    return false;
}

static int SetSandboxMounted(const AppSpawnSandboxCfg *sandbox, const char *name, char *rootPath)
{
    APPSPAWN_LOGW("SetSystemConstMounted %{public}s ", rootPath);
    char path[PATH_MAX] = {};
    int len = sprintf_s(path, sizeof(path), "%s%s", rootPath, SANDBOX_STAMP_FILE_SUFFIX);
    APPSPAWN_CHECK(len > 0, return 0, "Failed to format path");

    FILE *f = fopen(path, "wb");
    if (f != NULL) {
        fclose(f);
    }
    return 0;
}

static void UnmountPath(char *rootPath, uint32_t len, const SandboxMountNode *sandboxNode)
{
    if (sandboxNode->type == SANDBOX_TAG_MOUNT_PATH) {
        PathMountNode *pathNode = (PathMountNode *)sandboxNode;
        int ret = strcat_s(rootPath, len, pathNode->target);
        APPSPAWN_CHECK(ret == 0, return, "Failed to format");
        APPSPAWN_LOGV("Unmount sandbox config sandbox path %{public}s ", rootPath);
        ret = umount2(rootPath, MNT_DETACH);
        APPSPAWN_CHECK_ONLY_LOG(ret == 0, "Failed to umount2 %{public}s errno: %{public}d", rootPath, errno);
    }
}

int UnmountDepPaths(const AppSpawnSandboxCfg *sandbox, uid_t uid)
{
    APPSPAWN_CHECK(sandbox != NULL, return -1, "Invalid sandbox or context");
    APPSPAWN_LOGI("Unmount sandbox mount-paths-deps %{public}u ", sandbox->depNodeCount);
    char path[PATH_MAX] = {};
    int ret = BuildRootPath(path, sizeof(path), sandbox, uid / UID_BASE);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return -1);
    uint32_t rootLen = strlen(path);
    for (uint32_t i = 0; i < sandbox->depNodeCount; i++) {
        SandboxNameGroupNode *groupNode = sandbox->depGroupNodes[i];
        if (groupNode == NULL || groupNode->depNode == NULL) {
            continue;
        }
        // unmount this deps
        UnmountPath(path, sizeof(path), &groupNode->depNode->sandboxNode);
        path[rootLen] = '\0';
    }
    return 0;
}

int UnmountSandboxConfigs(const AppSpawnSandboxCfg *sandbox, uid_t uid, const char *name)
{
    APPSPAWN_CHECK(sandbox != NULL, return -1, "Invalid sandbox or context");
    APPSPAWN_CHECK(name != NULL, return -1, "Invalid name");
    char path[PATH_MAX] = {};
    int ret = BuildRootPath(path, sizeof(path), sandbox, uid / UID_BASE);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return -1);
    uint32_t rootLen = strlen(path);
    APPSPAWN_LOGI("Unmount sandbox %{public}s root: %{public}s", name, path);

    if (!IsSandboxMounted(sandbox, name, path)) {
        return 0;
    }

    SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, name);
    if (section == NULL) {
        return 0;
    }
    ListNode *node = section->front.next;
    while (node != &section->front) {
        SandboxMountNode *sandboxNode = (SandboxMountNode *)ListEntry(node, SandboxMountNode, node);
        UnmountPath(path, sizeof(path), sandboxNode);
        path[rootLen] = '\0';
        // get next
        node = node->next;
    }

    // delete stamp file
    ret = strcat_s(path, sizeof(path), SANDBOX_STAMP_FILE_SUFFIX);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return 0);
    APPSPAWN_LOGI("Unmount sandbox %{public}s ", path);
    unlink(path);
    return 0;
}

// Check whether the process incubation contains the ohos.permission.ACCESS_DLP_FILE permission
static bool IsADFPermission(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property)
{
    int index = GetPermissionIndexInQueue(&sandbox->permissionQueue, ACCESS_DLP_FILE_MODE);
    if (index > 0 && CheckAppPermissionFlagSet(property, index)) {
        return true;
    }
    if (GetBundleName(property) != NULL && strstr(GetBundleName(property), "com.ohos.dlpmanager") != NULL) {
        return true;
    }
    return false;
}

int StagedMountSystemConst(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
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
     *          dst = mount-path.sandbox-path --> 存在依赖时，配置<deps-path>、<deps-sandbox-path>、<deps-src-path>
     *      否则：
     *          src = mount-path.src-path
     *          dst = root-dir + mount-path.sandbox-path
     */
    SandboxContext *context = GetSandboxContext();
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return APPSPAWN_SYSTEM_ERROR);
    int ret = InitSandboxContext(context, sandbox, property, nwebspawn);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    if (IsSandboxMounted(sandbox, "system-const", context->rootPath) && IsADFPermission(sandbox, property) != true) {
        APPSPAWN_LOGV("Sandbox system-const %{public}s has been mount", context->rootPath);
        DeleteSandboxContext(context);
        return 0;
    }

    APPSPAWN_LOGV("Set sandbox system-const %{public}s", context->rootPath);
    uint32_t operation = 0;
    SetMountPathOperation(&operation, MOUNT_PATH_OP_REPLACE_BY_SANDBOX); // 首次挂载，使用sandbox替换
    SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "system-const");
    if (section != NULL) {
        ret = MountSandboxConfig(context, sandbox, section, operation);
    }
    SetSandboxMounted(sandbox, "system-const", context->rootPath);
    DeleteSandboxContext(context);
    return ret;
}

static int MountDepGroups(const SandboxContext *context, SandboxNameGroupNode *groupNode)
{
     /**
     * 在unshare前处理mount-paths-deps 处理逻辑
     *   1.判断是否有mount-paths-deps节点，没有直接返回;
     *   2.填充json文件中路径的变量值;
     *   3.校验deps-mode的值是否是not-exists
     *          是not-exist则判断mount-paths.src-path是否存在，若不存在则创建并挂载mount-paths-deps中的目录
     *                                                       若存在则不挂载mount-paths-deps中的目录
     *          是always则创建并挂载mount-paths-deps中的目录;
     *          deps-mode默认值为always;
     *
     */
    int ret = 0;
    if (groupNode == NULL || groupNode->depNode == NULL) {
        return 0;
    }

    ret = UpdateMountPathDepsPath(context, groupNode);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to update deps path name groups %{public}s", groupNode->section.name);

    if (groupNode->depMode == MOUNT_MODE_NOT_EXIST && CheckAndCreateDepPath(context, groupNode)) {
        return 0;
    }

    uint32_t operation = 0;
    SetMountPathOperation(&operation, MOUNT_PATH_OP_UNMOUNT);
    groupNode->depMounted = 1;
    ret = DoSandboxPathNodeMount(context, &groupNode->section, groupNode->depNode, operation);
    if (ret != 0) {
        APPSPAWN_LOGE("Mount deps root fail %{public}s", groupNode->section.name);
    }
    return ret;
}

static int SetSystemConstDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "system-const");
    if (section == NULL || section->nameGroups == NULL) {
        return 0;
    }

    int ret = 0;
    for (uint32_t i = 0; i < section->number; i++) {
        if (section->nameGroups[i] == NULL) {
            continue;
        }
        SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)section->nameGroups[i];
        ret = MountDepGroups(context, groupNode);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to mount deps groups");
    }
    return ret;
}

static int SetAppVariableDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "app-variable");
    if (section == NULL || section->nameGroups == NULL) {
        return 0;
    }

    int ret = 0;
    for (uint32_t i = 0; i < section->number; i++) {
        if (section->nameGroups[i] == NULL) {
            continue;
        }
        SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)section->nameGroups[i];
        ret = MountDepGroups(context, groupNode);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to mount deps groups");
    }
    return ret;
}

static int SetSpawnFlagsDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    ListNode *node = sandbox->spawnFlagsQueue.front.next;
    int ret = 0;
    while (node != &sandbox->spawnFlagsQueue.front) {
        SandboxFlagsNode *sandboxNode = (SandboxFlagsNode *)ListEntry(node, SandboxMountNode, node);
        // match flags point
        if (sandboxNode->flagIndex == 0 || !CheckSpawningMsgFlagSet(context, sandboxNode->flagIndex)) {
            node = node->next;
            continue;
        }

        if (sandboxNode->section.nameGroups == NULL) {
            node = node->next;
            continue;
        }

        for (uint32_t i = 0; i < sandboxNode->section.number; i++) {
            if (sandboxNode->section.nameGroups[i] == NULL) {
                continue;
            }
            SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)sandboxNode->section.nameGroups[i];
            ret = MountDepGroups(context, groupNode);
            APPSPAWN_CHECK(ret == 0, return ret, "Failed to mount deps groups");
        }
        node = node->next;
    }
    return ret;
}

static int SetPackageNameDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    SandboxPackageNameNode *sandboxNode =
        (SandboxPackageNameNode *)GetSandboxSection(&sandbox->packageNameQueue, context->bundleName);
    if (sandboxNode == NULL || sandboxNode->section.nameGroups == NULL) {
        return 0;
    }

    int ret = 0;
    for (uint32_t i = 0; i < sandboxNode->section.number; i++) {
        if (sandboxNode->section.nameGroups[i] == NULL) {
            continue;
        }
        SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)sandboxNode->section.nameGroups[i];
        ret = MountDepGroups(context, groupNode);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to mount deps groups");
    }
    return ret;
}

static int SetPermissionDepGroups(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    ListNode *node = sandbox->permissionQueue.front.next;
    int ret = 0;
    while (node != &sandbox->permissionQueue.front) {
        SandboxPermissionNode *permissionNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
        // match flags point
        if (!CheckSpawningPermissionFlagSet(context, permissionNode->permissionIndex)) {
            node = node->next;
            continue;
        }

        if (permissionNode->section.nameGroups == NULL) {
            node = node->next;
            continue;
        }

        for (uint32_t i = 0; i < permissionNode->section.number; i++) {
            if (permissionNode->section.nameGroups[i] == NULL) {
                continue;
            }
            SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)permissionNode->section.nameGroups[i];
            ret = MountDepGroups(context, groupNode);
            APPSPAWN_CHECK(ret == 0, return ret, "Failed to mount deps groups");
        }
        node = node->next;
    }
    return ret;
}

// The execution of the preunshare phase depends on the mounted mount point
static int StagedDepGroupMounts(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    int ret = SetSystemConstDepGroups(context, sandbox);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to set system const deps groups");

    ret = SetAppVariableDepGroups(context, sandbox);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to set app variable deps groups");

    ret = SetSpawnFlagsDepGroups(context, sandbox);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to set spawn flags deps groups");

    ret = SetPackageNameDepGroups(context, sandbox);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to set package name deps groups");

    ret = SetPermissionDepGroups(context, sandbox);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to set permission deps groups");

    return ret;
}

int StagedMountPreUnShare(const SandboxContext *context, AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_CHECK(sandbox != NULL && context != NULL, return -1, "Invalid sandbox or context");
    APPSPAWN_LOGV("Set sandbox config before unshare group count %{public}d", sandbox->depNodeCount);

    MountDirToShared(context, sandbox);
    int ret = StagedDepGroupMounts(context, sandbox);

    return ret;
}

static int SetAppVariableConfig(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    /**
     * app-variable 处理逻辑
     *   root-dir global.sandbox-root
     *   app-variabl， 处理mount path
     *      src = mount-path.src-path
     *      dst = root-dir + mount-path.src-path
     *   遍历name-groups，处理mount path
     *      如果存在 mount-paths-deps
     *          dst = mount-path.sandbox-path --> 存在依赖时，配置<deps-path>、<deps-sandbox-path>、<deps-src-path>
     *      否则：
     *          src = mount-path.src-path
     *          dst = root-dir + mount-path.sandbox-path
     */
    int ret = 0;
    // 首次挂载，使用sandbox替换
    uint32_t operation = 0;
    SetMountPathOperation(&operation, MOUNT_PATH_OP_REPLACE_BY_SANDBOX);
    SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "app-variable");
    if (section == NULL) {
        return 0;
    }
    ret = MountSandboxConfig(context, sandbox, section, operation);
    APPSPAWN_CHECK(ret == 0, return ret,
        "Set app-variable config fail result: %{public}d, app: %{public}s", ret, context->bundleName);
    return 0;
}

int StagedMountPostUnshare(const SandboxContext *context, const AppSpawnSandboxCfg *sandbox)
{
    APPSPAWN_CHECK(sandbox != NULL && context != NULL, return -1, "Invalid sandbox or context");
    APPSPAWN_LOGV("Set sandbox config after unshare ");

    int ret = SetAppVariableConfig(context, sandbox);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
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

#ifdef APPSPAWN_MOUNT_TMPSHM
static void MountDevShmPath(SandboxContext *context)
{
    char sandboxDevShmPath[PATH_MAX] = {};
    int ret = strcpy_s(sandboxDevShmPath, sizeof(sandboxDevShmPath), context->rootPath);
    APPSPAWN_CHECK(ret == 0, return, "Failed to strcpy rootPath");
    ret = strcat_s(sandboxDevShmPath, sizeof(sandboxDevShmPath), DEV_SHM_DIR);
    APPSPAWN_CHECK(ret == 0, return, "Failed to format devShmPath");
    int result = mount("tmpfs", sandboxDevShmPath, "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV, "size=32M");
    if (result != 0) {
        APPSPAWN_LOGW("Error mounting %{public}s to tmpfs, errno %{public}d", sandboxDevShmPath, errno);
    }
}
#endif

int MountSandboxConfigs(AppSpawnSandboxCfg *sandbox, const AppSpawningCtx *property, int nwebspawn)
{
    APPSPAWN_CHECK_ONLY_EXPER(property != NULL, return -1);
    APPSPAWN_CHECK(sandbox != NULL, return -1, "Failed to get sandbox for %{public}s", GetProcessName(property));

    SandboxContext *context = GetSandboxContext();
    APPSPAWN_CHECK_ONLY_EXPER(context != NULL, return APPSPAWN_SYSTEM_ERROR);
    int ret = InitSandboxContext(context, sandbox, property, nwebspawn);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    APPSPAWN_LOGV("Set sandbox config %{public}s sandboxNsFlags 0x%{public}x",
        context->rootPath, context->sandboxNsFlags);
    do {
        ret = StagedMountPreUnShare(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        CreateSandboxDir(context->rootPath, FILE_MODE);
        // add pid to a new mnt namespace
        ret = unshare(context->sandboxNsFlags);
        APPSPAWN_CHECK(ret == 0, break,
            "unshare failed, app: %{public}s errno: %{public}d", context->bundleName, errno);
        if ((context->sandboxNsFlags & CLONE_NEWNET) == CLONE_NEWNET) {
            ret = EnableNewNetNamespace();
            APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        }

        ret = SandboxRootFolderCreate(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        ret = StagedMountPostUnshare(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        ret = SetOverlayAppSandboxConfig(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = SetBundleResourceSandboxConfig(context, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

#ifdef APPSPAWN_MOUNT_TMPSHM
        MountDevShmPath(context);
#endif
        ret = ChangeCurrentDir(context);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
#if defined(APPSPAWN_MOUNT_TMPSHM) && defined(WITH_SELINUX)
        Restorecon(DEV_SHM_DIR);
#endif
        APPSPAWN_LOGV("Change root dir success %{public}s ", context->rootPath);
    } while (0);
    DeleteSandboxContext(context);
    return ret;
}
