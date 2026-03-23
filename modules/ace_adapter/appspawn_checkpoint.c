/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "securec.h"
#include "appspawn_utils.h"
#include "appspawn_msg.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"

#define CHECKPOINT_DEVICE_PATH "/proc/checkpoint/checkpoint"
#define BASE_NUM               10
#define CHECKPOINT_IOCTL_CHECKPOINT_ALL    _IOR(0xE0, 0x0, int)  // Create image process
#define CHECKPOINT_IOCTL_RESTORE_ALL       _IOR(0XE0, 0x1, int)  // Create worker process
#define CHECKPOINT_IOCTL_KILL_ALL          _IOR(0xE0, 0x4, int)  // Kill image process
#define CHECKPOINT_NAME_LEN    64

/**
 * @brief checkpoint ioctl params struct
 */
struct IoctlCheckPointArgs {
    pid_t inputPid;
    char name[CHECKPOINT_NAME_LEN];
    uint64_t checkPointId;
    pid_t resultPid;
};

/**
 * @brief Kill image: kill the process in the checkpoint tree.
 */
struct IoctlKillCheckPointArgs {
    uint64_t checkPointId;
};

typedef void (*CheckPointProcessTraversal)(const AppSpawnMgr *mgr,
                                           AppSpawnedCheckPointProcesses *checkPointInfo, void *data);

static int SpawningFdComparePro(ListNode *node, void *data)
{
    AppSpawnFds *node1 = ListEntry(node, AppSpawnFds, node);
    SpawningFdType type = *(SpawningFdType *)data;
    APPSPAWN_LOGV("current fd node type %{public}d fd[0]: %{public}d", node1->type, node1->fds[0]);
    return node1->type - type;
}

static int SpawningCheckPointIdComparePro(ListNode *node, ListNode *newNode)
{
    AppSpawnedCheckPointProcesses *node1 = ListEntry(node, AppSpawnedCheckPointProcesses, node);
    AppSpawnedCheckPointProcesses *node2 = ListEntry(newNode, AppSpawnedCheckPointProcesses, node);
    return node1->checkPointId - node2->checkPointId;
}

static int ImgInfoNameComparePro(ListNode *node, void *data)
{
    AppSpawnedCheckPointProcesses *node1 = ListEntry(node, AppSpawnedCheckPointProcesses, node);
    return strcmp(node1->name, (char *)data);
}

APPSPAWN_STATIC AppSpawnFds *GetSpawningFd(AppSpawnMgr *content, SpawningFdType fdType)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return NULL);
    ListNode *node = OH_ListFind(&content->spawningFdsQueue, &fdType, SpawningFdComparePro);
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL, return NULL);
    return ListEntry(node, AppSpawnFds, node);
}

APPSPAWN_STATIC int32_t AddSpawningFds(AppSpawnMgr *content, SpawningFdType type, uint32_t count, int *fds)
{
    APPSPAWN_CHECK((type >= TYPE_FOR_DEC) && (type < TYPE_INVALID) && count > 0 && fds != NULL,
                    return APPSPAWN_ARG_INVALID, "Invalid fds info");
    AppSpawnFds *node = (AppSpawnFds *)calloc(1, sizeof(AppSpawnFds) + count * sizeof(int));
    APPSPAWN_CHECK(node != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to malloc for appspawn fd node");

    node->type = type;
    node->count = count;
    for (uint32_t i = 0; i < count; i++) {
        node->fds[i] = fds[i];
    }

    OH_ListInit(&node->node);
    OH_ListAddWithOrder(&content->spawningFdsQueue, &node->node, SpawningFdComparePro);
    APPSPAWN_DUMPI("Add checkpoint fd %{public}d success, currentCnt %{public}d",
                   type, OH_ListGetCnt(&content->spawningFdsQueue));
    return 0;
}

APPSPAWN_STATIC AppSpawnedCheckPointProcesses *AddSpawningCheckPointInfo(
    AppSpawnMgr *content, uint64_t checkPointId, const char *processName, uint32_t appIndex, uint32_t uid)
{
    APPSPAWN_CHECK(checkPointId > 0, return NULL, "Invalid check point id for %{public}s", processName);
    size_t len = strlen(processName) + 1;
    APPSPAWN_CHECK(len > 1, return NULL, "Invalid processName for %{public}s", processName);
    AppSpawnedCheckPointProcesses *node = (AppSpawnedCheckPointProcesses *)calloc(
        1, sizeof(AppSpawnedCheckPointProcesses) + len + 1);
    APPSPAWN_CHECK(node != NULL, return NULL, "Failed to malloc for appinfo");

    node->checkPointId = checkPointId;
    node->uid = uid;
    node->exitStatus = 0;
    node->appIndex = appIndex;
    int ret = strcpy_s(node->name, len, processName);
    APPSPAWN_CHECK(ret == 0, free(node);
        return NULL, "Failed to strcpy process name");

    OH_ListInit(&node->node);
    OH_ListAddWithOrder(&content->checkPointIdQueue, &node->node, SpawningCheckPointIdComparePro);
    APPSPAWN_DUMPI("Add checkpointId %{public}s, checkpointId=%{public}" PRId64" success", processName, checkPointId);
    return node;
}

APPSPAWN_STATIC AppSpawnedCheckPointProcesses *GetSpawningImgInfoByName(AppSpawnMgr *content, const char *name)
{
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL, return NULL);
    ListNode *node = OH_ListFind(&content->checkPointIdQueue, (void *)name, ImgInfoNameComparePro);
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return NULL);
    return ListEntry(node, AppSpawnedCheckPointProcesses, node);
}

static void SetWorkerProcesssForkDenied(const char *bundleName, AppSpawningCtx *property)
{
    AppDacInfo *appInfo = (AppDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK_ONLY_EXPER(appInfo != NULL, return);

    const int userId = appInfo->uid / UID_BASE;
    char pathForkDenied[PATH_MAX] = {};
    int ret = snprintf_s(pathForkDenied, PATH_MAX, PATH_MAX - 1, "/dev/pids/%d/%s/app_%d",
                         userId, bundleName, property->pid);
    APPSPAWN_CHECK(ret > 0, return, "Failed to snprintf_s errno: %{public}d", errno);

    ret = strcat_s(pathForkDenied, sizeof(pathForkDenied), "pid.fork_denied");
    APPSPAWN_CHECK(ret > 0, return, "Failed to strcat_s path");

    int fd = open(pathForkDenied, O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGW("Set worker process %{public}d open failed", property->pid);
        return;
    }

    ret = write(fd, "1", 1);
    if (ret >= 0) {
        fsync(fd);
        APPSPAWN_LOGI("SetForkDenied success, cgroup's owner:%{public}d", appInfo->uid);
    }
    close(fd);
}

/**
 * @brief 执行 checkpoint 进程创建的公共函数
 * @param content appspawn 管理器
 * @param property 孵化上下文
 * @param fd checkpoint 设备 fd
 * @param ioctlCmd ioctl 命令 (CHECKPOINT_IOCTL_CHECKPOINT_ALL 或 CHECKPOINT_IOCTL_RESTORE_ALL)
 * @param needForkDenied 是否需要设置 fork denied (仅工作进程需要)
 * @return 成功返回 0，失败返回错误码
 */
APPSPAWN_STATIC int32_t DoCheckpointProcess(
    AppSpawnMgr *content,
    AppSpawningCtx *property,
    int fd,
    unsigned long ioctlCmd,
    bool needForkDenied)
{
    if (content == NULL || property == NULL) {
        return APPSPAWN_ARG_INVALID;
    }

    // 1. 获取 checkpoint 信息
    AppSpawnCheckpointInfo *info = (AppSpawnCheckpointInfo *)GetAppSpawnMsgInfo(property->message,
                                                                                TLV_CHECK_POINT_INFO);
    APPSPAWN_CHECK(info != NULL, return APPSPAWN_ARG_INVALID, "Invalid checkpoint info");

    // 2. 准备 ioctl 参数
    struct IoctlCheckPointArgs args = {0};
    args.inputPid = info->imgPid;
    args.checkPointId = info->checkPointId;

    const char *bundleName = GetBundleName(property);
    if (bundleName == NULL || strcpy_s(args.name, CHECKPOINT_NAME_LEN, bundleName) != 0) {
        APPSPAWN_LOGE("Failed to copy bundle name");
        return APPSPAWN_SYSTEM_ERROR;
    }
    args.name[CHECKPOINT_NAME_LEN - 1] = '\0';

    // 3. 执行 ioctl 调用
    APPSPAWN_LOGI("DoCheckpointProcess: ioctl cmd=%{public}lu, inputPid=%{public}d, checkPointId=%{public}" PRId64"",
                  ioctlCmd, args.inputPid, args.checkPointId);

    int32_t err = ioctl(fd, ioctlCmd, &args);
    if (err < 0) {
        APPSPAWN_LOGE("ioctl failed, cmd=%{public}lu, errno=%{public}d (%{public}s)",
                      ioctlCmd, errno, strerror(errno));
        return errno;
    }

    // 4. 更新进程信息
    property->pid = args.resultPid;
    property->checkPointId = args.checkPointId;

    // 5. 如果是工作进程，设置 fork denied
    if (needForkDenied) {
        SetWorkerProcesssForkDenied(args.name, property);
    }

    // 6. 更新或添加 checkpoint 信息
    AppSpawnedCheckPointProcesses *app = GetSpawningImgInfoByName(content, GetProcessName(property));
    if (app != NULL && app->checkPointId != args.checkPointId) {
        app->checkPointId = args.checkPointId;
        APPSPAWN_LOGV("Update checkpoint id: %{public}" PRId64"", app->checkPointId);
    } else if (app == NULL) {
        (void)AddSpawningCheckPointInfo(content, args.checkPointId, GetProcessName(property), 0, 0);
    }

    APPSPAWN_LOGI("DoCheckpointProcess success, pid=%{public}d, checkPointId=%{public}" PRId64"",
                  property->pid, property->checkPointId);
    return 0;
}

/**
 * @brief 创建镜像进程
 * 优化后：只需调用公共函数，传递特定参数
 */
APPSPAWN_STATIC int32_t CreateImageProcess(AppSpawnMgr *content, AppSpawningCtx *property, int fd)
{
    APPSPAWN_LOGI("Creating image process");
    return DoCheckpointProcess(content, property, fd, CHECKPOINT_IOCTL_CHECKPOINT_ALL, false);
}

/**
 * @brief 创建工作进程
 * 优化后：只需调用公共函数，传递特定参数
 */
APPSPAWN_STATIC int32_t CreateWorkerProcess(AppSpawnMgr *content, AppSpawningCtx *property, int fd)
{
    APPSPAWN_LOGI("Creating worker process");
    return DoCheckpointProcess(content, property, fd, CHECKPOINT_IOCTL_RESTORE_ALL, true);
}

/**
 * @brief Hook 1: 准备 checkpoint fd
 */
APPSPAWN_STATIC int32_t SpawnPrepareCheckpointFd(const AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == NULL || property == NULL || (content->content.mode != MODE_FOR_APP_SPAWN)) {
        return APPSPAWN_ARG_INVALID;
    }

    // 检查是否为 checkpoint 请求
    AppSpawnCheckpointInfo *info = (AppSpawnCheckpointInfo *)GetAppSpawnMsgInfo(property->message,
                                                                                TLV_CHECK_POINT_INFO);
    if (info == NULL) {
        return 0;  // 不是 checkpoint 请求
    }

    // 获取或创建 fd
    AppSpawnFds *fdNode = GetSpawningFd(content, TYPE_FOR_FORK_ALL);
    if (fdNode != NULL && fdNode->count > 0) {
        APPSPAWN_LOGV("Checkpoint fd already cached: %{public}d", fdNode->fds[0]);
        return 0;
    }

    // 打开设备并缓存
    int fd = open(CHECKPOINT_DEVICE_PATH, O_RDWR);
    APPSPAWN_CHECK(fd >= 0, return APPSPAWN_SYSTEM_ERROR,
                    "Open checkpoint device failed, errno: %{public}s", strerror(errno));

    int ret = AddSpawningFds(content, TYPE_FOR_FORK_ALL, 1, &fd);
    APPSPAWN_CHECK(ret == 0, close(fd);
                   return ret, "Failed to cache checkpoint fd");

    APPSPAWN_LOGI("Checkpoint fd prepared: %{public}d", fd);
    return 0;
}

/**
 * @brief Hook 2: 创建镜像进程
 */
APPSPAWN_STATIC int32_t CreateImageProcessHook(const AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == NULL || property == NULL || (content->content.mode != MODE_FOR_APP_SPAWN)) {
        return APPSPAWN_ARG_INVALID;
    }

    if (!CheckAppMsgFlagsSet(property, APP_FLAGS_SPAWN_IMAGE_PROCESS)) {
        return 0;  // 不是镜像进程
    }

    AppSpawnFds *fdNode = GetSpawningFd(content, TYPE_FOR_FORK_ALL);
    APPSPAWN_CHECK(fdNode != NULL && fdNode->count > 0,
        return APPSPAWN_SYSTEM_ERROR, "Checkpoint fd not prepared");

    return CreateImageProcess(content, property, fdNode->fds[0]);
}

/**
 * @brief Hook 3: 创建工作进程
 */
APPSPAWN_STATIC int32_t CreateWorkerProcessHook(const AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (content == NULL || property == NULL || (content->content.mode != MODE_FOR_APP_SPAWN)) {
        return APPSPAWN_ARG_INVALID;
    }

    if (CheckAppMsgFlagsSet(property, APP_FLAGS_SPAWN_IMAGE_PROCESS)) {
        return 0;  // 是镜像进程，跳过
    }

    AppSpawnCheckpointInfo *info = (AppSpawnCheckpointInfo *)GetAppSpawnMsgInfo(property->message,
                                                                                TLV_CHECK_POINT_INFO);
    if (info == NULL) {
        return 0;  // 不是 checkpoint 请求
    }

    AppSpawnFds *fdNode = GetSpawningFd(content, TYPE_FOR_FORK_ALL);
    APPSPAWN_CHECK(fdNode != NULL && fdNode->count > 0,
        return APPSPAWN_SYSTEM_ERROR, "Checkpoint fd not prepared");

    return CreateWorkerProcess(content, property, fdNode->fds[0]);
}

/**
 * @brief 销毁单个 checkpoint 节点的回调函数
 */
static void ImgQueueDestroyProc(const AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *checkPointInfo, void *data)
{
    uint64_t checkPointId = checkPointInfo->checkPointId;
    int checkPointFd = *(int *)data;

    APPSPAWN_LOGI("Destroying checkpoint: name=%{public}s, checkPointId=%{public}" PRId64"",
                  checkPointInfo->name, checkPointId);

    if (checkPointId > 0 && ioctl(checkPointFd, CHECKPOINT_IOCTL_KILL_ALL, &checkPointId) != 0) {
        APPSPAWN_LOGE("Failed to kill checkpoint, id=%{public}" PRId64", errno=%{public}d (%{public}s)",
                      checkPointId, errno, strerror(errno));
    }

    OH_ListRemove(&checkPointInfo->node);
    OH_ListInit(&checkPointInfo->node);
    free(checkPointInfo);
}

/**
 * @brief Checkpoint 队列遍历辅助函数（保持模块化和可复用性）
 * @param traversal 遍历回调函数
 * @param content appspawn 管理器
 * @param data 传递给回调函数的自定义数据
 */
APPSPAWN_STATIC void TraversalImgProcess(CheckPointProcessTraversal traversal, const AppSpawnMgr *content, void *data)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL && traversal != NULL, return);

    ListNode *node = content->checkPointIdQueue.next;
    while (node != &content->checkPointIdQueue) {
        ListNode *next = node->next;
        AppSpawnedCheckPointProcesses *checkPointInfo = ListEntry(node, AppSpawnedCheckPointProcesses, node);
        traversal(content, checkPointInfo, data);
        node = next;
    }
}

/**
 * @brief 销毁所有 checkpoint 镜像进程队列
 *
 * 该函数在服务退出时被调用，清理所有 checkpoint 进程
 * 使用 TraversalImgProcess 辅助函数保持代码模块化
 */
APPSPAWN_STATIC void SpawnDestoryImgQue(const AppSpawnMgr *content)
{
    if (content == NULL || (content->content.mode != MODE_FOR_APP_SPAWN)) {
        return;
    }

    // 获取 checkpoint 设备 fd
    AppSpawnFds *fdNode = GetSpawningFd(content, TYPE_FOR_FORK_ALL);
    if (fdNode == NULL) {
        APPSPAWN_LOGW("SpawnDestoryImgQue: fdNode is NULL, no checkpoint to destroy");
        return;
    }

    if (fdNode->count <= 0 || fdNode->fds[0] <= 0) {
        APPSPAWN_LOGW("SpawnDestoryImgQue: invalid fd count or value, count=%{public}u, fd=%{public}d",
                      fdNode->count, fdNode->fds[0]);
        return;
    }

    int forkAllFd = fdNode->fds[0];
    TraversalImgProcess(ImgQueueDestroyProc, content, &forkAllFd);
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Register checkpoint hooks ...");

    AddAppSpawnHook(STAGE_PARENT_BOOT_IMG, HOOK_PRIO_HIGHEST, SpawnPrepareCheckpointFd);
    AddAppSpawnHook(STAGE_PARENT_BOOT_IMG, HOOK_PRIO_COMMON, CreateImageProcessHook);
    AddAppSpawnHook(STAGE_PARENT_BOOT_IMG, HOOK_PRIO_COMMON, CreateWorkerProcessHook);

    AddServerStageHook(STAGE_SERVER_EXIT, HOOK_PRIO_COMMON, SpawnDestoryImgQue);
}