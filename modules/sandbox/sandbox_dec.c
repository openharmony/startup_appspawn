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

static const char *g_decConstraintDir[] = {
    "/storage/Users",
    "/storage/External",
    "/storage/Share",
    "/storage/hmdfs",
    "/mnt/data/fuse",
    "/mnt/debug",
    "/storage/userExternal"
};

static const char *g_decForcedPrefix[] = {
    "/storage/Users/currentUser/appdata",
};

static DecPolicyInfo *g_decPolicyInfos = NULL;

void DestroyDecPolicyInfos(DecPolicyInfo *decPolicyInfos)
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
        g_decPolicyInfos = (DecPolicyInfo *)calloc(1, sizeof(DecPolicyInfo));
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
    g_decPolicyInfos->flag = true;
    g_decPolicyInfos->userId = 0;
}

static int SetDenyConstraintDirs(AppSpawnMgr *content)
{
    APPSPAWN_LOGI("enter SetDenyConstraintDirs sandbox policy success.");
    UNUSED(content);
    const char *decFilename = "/dev/dec";
    int fd = open(decFilename, O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGE("open dec file fail.");
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
            APPSPAWN_LOGI("policy info: %{public}s", decPolicyInfos.path[i].path);
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
        APPSPAWN_LOGE("open dec file fail.");
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
        APPSPAWN_LOGI("set SET_DEC_PREFIX_CMD sandbox policy success.");
        for (uint32_t i = 0; i < decPolicyInfos.pathNum; i++) {
            APPSPAWN_LOGI("policy info: %{public}s", decPolicyInfos.path[i].path);
        }
    }
    close(fd);
    return 0;
}

void SetDecPolicy(void)
{
    if (g_decPolicyInfos == NULL) {
        return;
    }
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
    g_decPolicyInfos->timestamp = timestamp;

    if (ioctl(fd, SET_DEC_POLICY_CMD, g_decPolicyInfos) < 0) {
        APPSPAWN_LOGE("set sandbox policy failed.");
    } else {
        APPSPAWN_LOGI("set SET_DEC_POLICY_CMD sandbox policy success. timestamp:%{public}" PRId64 "", timestamp);
        for (uint32_t i = 0; i < g_decPolicyInfos->pathNum; i++) {
            APPSPAWN_LOGI("policy info: path %{public}s, mode 0x%{public}x",
                g_decPolicyInfos->path[i].path, g_decPolicyInfos->path[i].mode);
        }
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
}
