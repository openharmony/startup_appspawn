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

#ifdef APPSPAWN_SUPPORT_NOSHAREFS
static const DecIgnoreCaseInfo g_setInfo[] = { DEC_IGNORE_CASE_LIST };
#endif

APPSPAWN_STATIC int SetIgnoreCaseDirs(AppSpawnMgr *content)
{
#ifdef APPSPAWN_SUPPORT_NOSHAREFS
    APPSPAWN_CHECK(IsNativeSpawnMode(content) || IsAppSpawnMode(content), return 0,
        "not support ignore case dir");
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    APPSPAWN_CHECK(fd >= 0, return 0, "open dec fd failed");

    DecPolicyInfo decPolicyInfos = {0};
    decPolicyInfos.tokenId = 0;
    decPolicyInfos.pathNum = ARRAY_LENGTH(g_setInfo);
    decPolicyInfos.flag = 0;
    
    for (uint32_t i = 0; i < decPolicyInfos.pathNum; i++) {
        PathInfo pathInfo = {
            .path = (char *)g_setInfo[i].path,
            .pathLen = (uint32_t)strlen(g_setInfo[i].path),
            .mode = (uint32_t)g_setInfo[i].mode,
            .flag = false
        };
        APPSPAWN_LOGV("set decpolicy %{public}s %{public}d", g_setInfo[i].path, g_setInfo[i].mode);
        decPolicyInfos.path[i] = pathInfo;
    }
    int ret = ioctl(fd, SET_DEC_IGNORE_CASE_CMD, &decPolicyInfos);
    APPSPAWN_CHECK_ONLY_LOG(ret >= 0, "set dec ignore failed %{public}d", errno);
    close(fd);
#else
    UNUSED(content);
    APPSPAWN_LOGV("ignore case dec not enable");
#endif
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

    if (g_decPolicyInfos == NULL) {
        g_decPolicyInfos = (GlobalDecPolicyInfo *)calloc(1, sizeof(GlobalDecPolicyInfo));
        if (g_decPolicyInfos == NULL) {
            APPSPAWN_LOGE("calloc failed");
            return;
        }
    }

    APPSPAWN_CHECK(g_decPolicyInfos->pathNum + decPolicyInfos->pathNum <= MAX_POLICY_NUM,
        DestroyDecPolicyInfos(g_decPolicyInfos);
        g_decPolicyInfos = NULL;
        return, "Out of MAX_POLICY_NUM %{public}d, %{public}d", g_decPolicyInfos->pathNum, decPolicyInfos->pathNum);
    for (uint32_t i = 0; i < decPolicyInfos->pathNum; i++) {
        PathInfo pathInfo = {0};
        if (decPolicyInfos->path[i].path == NULL) {
            DestroyDecPolicyInfos(g_decPolicyInfos);
            g_decPolicyInfos = NULL;
            return;
        }
        pathInfo.path = strdup(decPolicyInfos->path[i].path);
        if (pathInfo.path == NULL) {
            DestroyDecPolicyInfos(g_decPolicyInfos);
            g_decPolicyInfos = NULL;
            return;
        }
        pathInfo.pathLen = (uint32_t)strlen(pathInfo.path);
        pathInfo.mode = decPolicyInfos->path[i].mode;
        uint32_t index = g_decPolicyInfos->pathNum + i;
        g_decPolicyInfos->path[index] = pathInfo;
    }
    g_decPolicyInfos->tokenId = decPolicyInfos->tokenId;
    g_decPolicyInfos->pathNum += decPolicyInfos->pathNum;
    g_decPolicyInfos->flag = false;
    g_decPolicyInfos->userId = 0;
}

static int SetDenyConstraintDirs(AppSpawnMgr *content)
{
    APPSPAWN_LOGI("enter SetDenyConstraintDirs sandbox policy success.");
    UNUSED(content);
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    if (fd < 0) {
        APPSPAWN_CHECK_ONLY_LOG(IsNativeSpawnMode(content) != 0, "open dec file fail.");
        return 0;
    }

    uint32_t decDirsSize = ARRAY_LENGTH(g_decConstraintDir);
    DecPolicyInfo decPolicyInfos = {0};
    decPolicyInfos.tokenId = 0;
    decPolicyInfos.pathNum = decDirsSize;
    decPolicyInfos.flag = 0;

    for (uint32_t i = 0; i < decDirsSize; i++) {
        PathInfo pathInfo = {(char *)g_decConstraintDir[i], (uint32_t)strlen(g_decConstraintDir[i]), SANDBOX_MODE_READ};
        decPolicyInfos.path[i] = pathInfo;
    }

    if (ioctl(fd, CONSTRAINT_DEC_POLICY_CMD, &decPolicyInfos) < 0) {
        APPSPAWN_LOGE("set sandbox policy failed.");
    } else {
        APPSPAWN_LOGI("set CONSTRAINT_DEC_POLICY_CMD sandbox policy success.");
        for (uint32_t i = 0; i < decPolicyInfos.pathNum; i++) {
            APPSPAWN_DUMPI("%{public}s", decPolicyInfos.path[i].path);
        }
    }
    close(fd);
    return 0;
}

static int SetForcedPrefixDirs(AppSpawnMgr *content)
{
    APPSPAWN_LOGI("enter SetForcedPrefixDirs sandbox policy success.");
    UNUSED(content);
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    if (fd < 0) {
        APPSPAWN_CHECK_ONLY_LOG(IsNativeSpawnMode(content) != 0, "open dec file fail.");
        return 0;
    }

    uint32_t decDirsSize = ARRAY_LENGTH(g_decForcedPrefix);
    DecPolicyInfo decPolicyInfos = {0};
    decPolicyInfos.tokenId = 0;
    decPolicyInfos.pathNum = decDirsSize;
    decPolicyInfos.flag = 0;

    for (uint32_t i = 0; i < decDirsSize; i++) {
        PathInfo pathInfo = {(char *)g_decForcedPrefix[i], (uint32_t)strlen(g_decForcedPrefix[i]), SANDBOX_MODE_READ};
        decPolicyInfos.path[i] = pathInfo;
    }

    if (ioctl(fd, SET_DEC_PREFIX_CMD, &decPolicyInfos) < 0) {
        APPSPAWN_LOGE("set sandbox forced prefix failed.");
    } else {
        APPSPAWN_LOGV("set SET_DEC_PREFIX_CMD sandbox policy success.");
        for (uint32_t i = 0; i < decPolicyInfos.pathNum; i++) {
            APPSPAWN_DUMPI("%{public}s", decPolicyInfos.path[i].path);
        }
    }
    close(fd);
    return 0;
}

/**
 * @brief 下发单批次DEC策略到内核
 * @param fd dec设备文件描述符
 * @param decPolicyInfos 完整的策略信息
 * @param timestamp 时间戳
 * @param start 起始路径索引
 * @param count 本批次路径数量
 * @return 0成功，负数失败
 */
APPSPAWN_STATIC int SetDecPolicyBatch(int fd, GlobalDecPolicyInfo *decPolicyInfos,
    uint64_t timestamp, uint32_t start, uint32_t count)
{
    APPSPAWN_CHECK(decPolicyInfos != NULL && count > 0 && count <= KERNEL_BATCH_SIZE,
        return -1, "Invalid param");

    DecPolicyInfo batchInfo = {0};
    batchInfo.tokenId = decPolicyInfos->tokenId;
    batchInfo.timestamp = timestamp;
    batchInfo.pathNum = count;
    batchInfo.userId = decPolicyInfos->userId;
    batchInfo.flag = decPolicyInfos->flag;
    errno_t memRet = memcpy_s(batchInfo.reserved, sizeof(batchInfo.reserved),
                              decPolicyInfos->reserved, sizeof(decPolicyInfos->reserved));
    APPSPAWN_CHECK(memRet == EOK, return -1, "Failed to memcpy_s reserved");

    for (uint32_t i = 0; i < count; i++) {
        APPSPAWN_LOGV("SetDecPolicyBatch: [%{public}u] %{public}u %{public}u %{public}u %{public}s",
                      start + i, decPolicyInfos->path[start + i].mode, decPolicyInfos->path[start + i].flag,
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
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGE("open dec file fail.");
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
    AddPreloadHook(HOOK_PRIO_COMMON, SetIgnoreCaseDirs);
}
