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

#include "sandbox_dec.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "appspawn_utils.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "dec_config.h"
#include "securec.h"

// Kernel ABI batch type for DEC ioctl. path capacity is fixed to KERNEL_BATCH_SIZE.
// Used by all DEC batch ioctl commands to keep _IOWR/_IOW size consistent with the kernel.
typedef struct IoctlDecPolicyBatch {
    uint64_t tokenId;
    uint64_t timestamp;
    PathInfo path[KERNEL_BATCH_SIZE];
    uint32_t pathNum;
    int32_t userId;
    uint64_t reserved[DEC_POLICY_HEADER_RESERVED];
    bool flag;
} IoctlDecPolicyBatch;

// Internal ioctl commands, all based on IoctlDecPolicyBatch (path[KERNEL_BATCH_SIZE]).
#define SET_DEC_POLICY_CMD _IOWR(HM_DEC_IOCTL_BASE, HM_SET_POLICY_ID, IoctlDecPolicyBatch)
#define CONSTRAINT_DEC_POLICY_CMD _IOW(HM_DEC_IOCTL_BASE, HM_CONSTRAINT_POLICY_ID, IoctlDecPolicyBatch)
#define SET_DEC_PREFIX_CMD _IOWR(HM_DEC_IOCTL_BASE, HM_SET_PREFIX_ID, IoctlDecPolicyBatch)
#define SET_DEC_IGNORE_CASE_CMD _IOWR(HM_DEC_IOCTL_BASE, HM_SET_DEC_IGNORE_CASE_ID, IoctlDecPolicyBatch)

static const char *g_decConstraintDir[] = {
    "/storage/Users",
    "/storage/External",
    "/storage/Share",
    "/storage/hmdfs",
    "/mnt/data/fuse",
    "/mnt/debug",
    "/storage/userExternal",
    "/storage/ANCO_APP_DATA"
};

static const char *g_decForcedPrefix[] = {
    "/storage/Users/currentUser/appdata",
};

APPSPAWN_STATIC int SetIgnoreCaseDirs(AppSpawnMgr *content)
{
    APPSPAWN_CHECK(IsNativeSpawnMode(content) || IsAppSpawnMode(content), return 0,
        "not support ignore case dir");

    uint32_t pathNum = 0;
    const DecIgnoreCaseInfo *setInfo = GetDecIgnoreCaseList(IsNoShareFsEnable(), &pathNum);
    APPSPAWN_CHECK(setInfo != NULL && pathNum > 0 && pathNum <= MAX_POLICY_NUM, return 0,
        "invalid dec ignore case list %{public}u", pathNum);
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    APPSPAWN_CHECK(fd >= 0, return 0, "Open dec file failed errno %{public}d", errno);

    uint32_t batches = (pathNum + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    for (uint32_t batch = 0; batch < batches; batch++) {
        uint32_t start = batch * KERNEL_BATCH_SIZE;
        uint32_t end = start + KERNEL_BATCH_SIZE;
        APPSPAWN_ONLY_EXPER(end > pathNum, end = pathNum);
        IoctlDecPolicyBatch decPolicyInfos = {0};
        decPolicyInfos.pathNum = end - start;
        for (uint32_t i = start; i < end; i++) {
            PathInfo pathInfo = {
                .path = (char *)setInfo[i].path,
                .pathLen = (uint32_t)strlen(setInfo[i].path),
                .mode = (uint32_t)setInfo[i].mode,
                .flag = false
            };
            APPSPAWN_LOGV("set ignore case dec policy %{public}s %{public}d", setInfo[i].path, setInfo[i].mode);
            decPolicyInfos.path[i - start] = pathInfo;
        }
        int ret = ioctl(fd, SET_DEC_IGNORE_CASE_CMD, &decPolicyInfos);
        APPSPAWN_CHECK(ret >= 0, continue, "set dec ignore failed errno %{public}d", errno);
        for (uint32_t i = start; i < end; i++) {
            APPSPAWN_DUMPI("set dec ignore case %{public}s", setInfo[i].path);
        }
    }
    close(fd);
    return 0;
}

static GlobalDecPolicyInfo *g_decPolicyInfos = NULL;

void DestroyDecPolicyInfos(GlobalDecPolicyInfo *decPolicyInfos)
{
    if (decPolicyInfos == NULL) {
        return;
    }
    for (uint32_t i = 0; i < decPolicyInfos->pathNum; i++) {
        if (decPolicyInfos->path[i].path) {
            free(decPolicyInfos->path[i].path);
            decPolicyInfos->path[i].pathLen = 0;
            decPolicyInfos->path[i].flag = 0;
            decPolicyInfos->path[i].mode = 0;
        }
    }
    decPolicyInfos->pathNum = 0;
    decPolicyInfos->tokenId = 0;
    decPolicyInfos->flag = 0;
    free(decPolicyInfos);
}

void SetDecPolicyInfos(DecPolicyInfo *decPolicyInfos)
{
    if (decPolicyInfos == NULL || decPolicyInfos->pathNum == 0) {
        return;
    }
    // Defensive clamp: DecPolicyInfo.path[] capacity is MAX_CONFIG_POLICY_NUM
    APPSPAWN_CHECK_LOGW(decPolicyInfos->pathNum <= MAX_CONFIG_POLICY_NUM,
        decPolicyInfos->pathNum = MAX_CONFIG_POLICY_NUM,
        "DecPolicy pathNum %{public}u exceeds path capacity %{public}d, clamp",
        decPolicyInfos->pathNum, MAX_CONFIG_POLICY_NUM);

    if (g_decPolicyInfos == NULL) {
        g_decPolicyInfos = (GlobalDecPolicyInfo *)calloc(1, sizeof(GlobalDecPolicyInfo));
        if (g_decPolicyInfos == NULL) {
            APPSPAWN_LOGE("calloc failed");
            return;
        }
    }

    if (g_decPolicyInfos->pathNum + decPolicyInfos->pathNum > MAX_POLICY_NUM) {
        APPSPAWN_LOGW("DecPolicy exceeds MAX_POLICY_NUM %{public}d, cur=%{public}u, add=%{public}u, partial apply",
            MAX_POLICY_NUM, g_decPolicyInfos->pathNum, decPolicyInfos->pathNum);
    }
    for (uint32_t i = 0; i < decPolicyInfos->pathNum; i++) {
        if (g_decPolicyInfos->pathNum >= MAX_POLICY_NUM) {
            APPSPAWN_LOGW("DecPolicy full at MAX_POLICY_NUM %{public}d, skip remaining", MAX_POLICY_NUM);
            break;
        }
        PathInfo pathInfo = {0};
        if (decPolicyInfos->path[i].path == NULL) {
            APPSPAWN_LOGW("DecPolicy path[%{public}u] is NULL, skip", i);
            continue;
        }
        pathInfo.path = strdup(decPolicyInfos->path[i].path);
        if (pathInfo.path == NULL) {
            APPSPAWN_LOGW("DecPolicy path[%{public}u] %{public}s strdup failed, skip",
                i, decPolicyInfos->path[i].path);
            continue;
        }
        pathInfo.pathLen = (uint32_t)strlen(pathInfo.path);
        pathInfo.mode = decPolicyInfos->path[i].mode;
        g_decPolicyInfos->path[g_decPolicyInfos->pathNum] = pathInfo;
        g_decPolicyInfos->pathNum++;
    }
    g_decPolicyInfos->tokenId = decPolicyInfos->tokenId;
    g_decPolicyInfos->flag = false;
    g_decPolicyInfos->userId = 0;
}

APPSPAWN_STATIC int SetDenyConstraintDirs(AppSpawnMgr *content)
{
    UNUSED(content);
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    APPSPAWN_CHECK(fd >= 0, return 0, "Open dec file failed errno %{public}d", errno);

    uint32_t decDirsSize = ARRAY_LENGTH(g_decConstraintDir);
    uint32_t batches = (decDirsSize + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    for (uint32_t batch = 0; batch < batches; batch++) {
        uint32_t start = batch * KERNEL_BATCH_SIZE;
        uint32_t end = start + KERNEL_BATCH_SIZE;
        APPSPAWN_ONLY_EXPER(end > decDirsSize, end = decDirsSize);
        IoctlDecPolicyBatch decPolicyInfos = {0};
        decPolicyInfos.pathNum = end - start;
        for (uint32_t i = start; i < end; i++) {
            PathInfo pathInfo = {(char *)g_decConstraintDir[i],
                                 (uint32_t)strlen(g_decConstraintDir[i]), SANDBOX_MODE_READ};
            decPolicyInfos.path[i - start] = pathInfo;
        }
        int ret = ioctl(fd, CONSTRAINT_DEC_POLICY_CMD, &decPolicyInfos);
        APPSPAWN_CHECK(ret >= 0, continue,
            "set deny constraint sandbox policy failed errno %{public}d", errno);
        APPSPAWN_LOGI("set CONSTRAINT_DEC_POLICY_CMD sandbox policy success");
        for (uint32_t i = start; i < end; i++) {
            APPSPAWN_DUMPI("%{public}s", g_decConstraintDir[i]);
        }
    }
    close(fd);
    return 0;
}

APPSPAWN_STATIC int SetForcedPrefixDirs(AppSpawnMgr *content)
{
    UNUSED(content);
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    APPSPAWN_CHECK(fd >= 0, return 0, "Open dec file failed errno %{public}d", errno);

    uint32_t decDirsSize = ARRAY_LENGTH(g_decForcedPrefix);
    uint32_t batches = (decDirsSize + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    for (uint32_t batch = 0; batch < batches; batch++) {
        uint32_t start = batch * KERNEL_BATCH_SIZE;
        uint32_t end = start + KERNEL_BATCH_SIZE;
        APPSPAWN_ONLY_EXPER(end > decDirsSize, end = decDirsSize);
        IoctlDecPolicyBatch decPolicyInfos = {0};
        decPolicyInfos.pathNum = end - start;
        for (uint32_t i = start; i < end; i++) {
            PathInfo pathInfo = {(char *)g_decForcedPrefix[i],
                                 (uint32_t)strlen(g_decForcedPrefix[i]), SANDBOX_MODE_READ};
            decPolicyInfos.path[i - start] = pathInfo;
        }
        int ret = ioctl(fd, SET_DEC_PREFIX_CMD, &decPolicyInfos);
        APPSPAWN_CHECK(ret >= 0, continue,
            "set forced prefix sandbox policy failed errno %{public}d", errno);
        APPSPAWN_LOGI("set SET_DEC_PREFIX_CMD sandbox policy success");
        for (uint32_t i = start; i < end; i++) {
            APPSPAWN_DUMPI("%{public}s", g_decForcedPrefix[i]);
        }
    }
    close(fd);
    return 0;
}

/**
 * @brief Deliver a single batch of DEC policies to the kernel.
 * @param fd DEC device file descriptor.
 * @param decPolicyInfos Full policy info.
 * @param timestamp Timestamp.
 * @param start Starting path index.
 * @param count Number of paths in this batch.
 * @return 0 on success, negative on failure.
 */
APPSPAWN_STATIC int SetDecPolicyBatch(int fd, GlobalDecPolicyInfo *decPolicyInfos,
    uint64_t timestamp, uint32_t start, uint32_t count)
{
    APPSPAWN_CHECK(decPolicyInfos != NULL && count > 0 && count <= KERNEL_BATCH_SIZE,
        return -1, "Invalid param");

    IoctlDecPolicyBatch batchInfo = {0};
    batchInfo.tokenId = decPolicyInfos->tokenId;
    batchInfo.timestamp = timestamp;
    batchInfo.pathNum = count;
    batchInfo.userId = decPolicyInfos->userId;
    batchInfo.flag = decPolicyInfos->flag;
    errno_t memRet = memcpy_s(batchInfo.reserved, sizeof(batchInfo.reserved),
                              decPolicyInfos->reserved, sizeof(decPolicyInfos->reserved));
    APPSPAWN_CHECK(memRet == EOK, return -1, "Failed to memcpy_s reserved");

    for (uint32_t i = 0; i < count; i++) {
        APPSPAWN_LOGV("SetDecPolicyBatch: ts %{public}llu idx %{public}u mode %{public}u "
                      "flag %{public}u len %{public}u path %{public}s",
                      (unsigned long long)batchInfo.timestamp, start + i,
                      decPolicyInfos->path[start + i].mode, decPolicyInfos->path[start + i].flag,
                      decPolicyInfos->path[start + i].pathLen, decPolicyInfos->path[start + i].path);
        batchInfo.path[i].path = decPolicyInfos->path[start + i].path;
        batchInfo.path[i].pathLen = decPolicyInfos->path[start + i].pathLen;
        batchInfo.path[i].mode = decPolicyInfos->path[start + i].mode;
        batchInfo.path[i].flag = decPolicyInfos->path[start + i].flag;
    }

    int ret = ioctl(fd, SET_DEC_POLICY_CMD, &batchInfo);
    APPSPAWN_CHECK(ret >= 0, return ret,
        "SetDecPolicyBatch: ioctl failed, start %{public}u, count %{public}u, errno %{public}d",
        start, count, errno);
    return ret;
}

void SetDecPolicy(void)
{
    APPSPAWN_CHECK(g_decPolicyInfos != NULL, return, "Invalid g_decPolicyInfos");
    // Defensive check before open: invalid pathNum must not leak fd or stale global state
    APPSPAWN_CHECK(g_decPolicyInfos->pathNum <= MAX_POLICY_NUM, DestroyDecPolicyInfos(g_decPolicyInfos);
        g_decPolicyInfos = NULL;
        return, "DecPolicy pathNum invalid %{public}u", g_decPolicyInfos->pathNum);
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGE("Open dec file failed errno %{public}d", errno);
        DestroyDecPolicyInfos(g_decPolicyInfos);
        g_decPolicyInfos = NULL;
        return;
    }

    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t timestamp = ts.tv_sec * APPSPAWN_SEC_TO_NSEC + ts.tv_nsec;

    uint32_t pathNum = g_decPolicyInfos->pathNum;
    uint32_t batches = (pathNum + KERNEL_BATCH_SIZE - 1) / KERNEL_BATCH_SIZE;
    for (uint32_t batch = 0; batch < batches; batch++) {
        uint32_t start = batch * KERNEL_BATCH_SIZE;
        uint32_t end = start + KERNEL_BATCH_SIZE;
        if (end > pathNum) {
            end = pathNum;
        }
        SetDecPolicyBatch(fd, g_decPolicyInfos, timestamp, start, end - start);
    }

    close(fd);
    DestroyDecPolicyInfos(g_decPolicyInfos);
    g_decPolicyInfos = NULL;
    return;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGI("Load sandbox dec module ...");
    AddPreloadHook(HOOK_PRIO_COMMON, SetDenyConstraintDirs);
    AddPreloadHook(HOOK_PRIO_COMMON, SetForcedPrefixDirs);
    AddPreloadHook(HOOK_PRIO_HIGHEST, SetIgnoreCaseDirs);
}
