/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <dirent.h>

#include "securec.h"
#include "appspawn_utils.h"
#include "appspawn_msg.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"

#define ISOLATE_PATH_BUFF_SIZE 512
#define ISOLATE_PATH_NUM 3
#define ISOLATE_PATH_CAPACITY 16
#define HM_DEC_IOCTL_BASE 's'
#define HM_ADD_ISOLATE_DIR 16
#define ADD_ISOLATE_DIR_CMD _IOWR(HM_DEC_IOCTL_BASE, HM_ADD_ISOLATE_DIR, IsolateDirInfo)

typedef struct {
    char *path;
    int len;
} IsolateDir;

typedef struct {
    IsolateDir dirs[ISOLATE_PATH_CAPACITY];
    int dirNum;
} IsolateDirInfo;

typedef struct {
    const char *prefix;
    const char *suffix;
} IsolateDirAffix;

static const IsolateDirAffix g_dirAffix[ISOLATE_PATH_NUM] = {{"/data/app/el2", "base"},
                                                             {"/storage/media", "local/files/Docs"},
                                                             {"/data/app/el1", "base"}};

static void FreeIsolatePath(IsolateDirInfo *isolateDirInfo)
{
    for (int i = 0; i < isolateDirInfo->dirNum; ++i) {
        free(isolateDirInfo->dirs[i].path);
        isolateDirInfo->dirs[i].path = NULL;
    }

    isolateDirInfo->dirNum = 0;
}

/*
 * To avoid memory leaks,
 * it needs to be called in pairs with FreeIsolatePath.
 */
static int MallocIsolatePath(IsolateDirInfo *isolateDirInfo)
{
    isolateDirInfo->dirNum = 0;

    for (int i = 0; i < ISOLATE_PATH_NUM; ++i) {
        char *path = (char *)calloc(1, ISOLATE_PATH_BUFF_SIZE);
        if (path == NULL) {
            APPSPAWN_LOGE("Failed to calloc buffer for path");
            FreeIsolatePath(isolateDirInfo);
            return APPSPAWN_SYSTEM_ERROR;
        }

        isolateDirInfo->dirs[i].path = path;
        ++isolateDirInfo->dirNum;
    }

    return 0;
}

static int GetNormalUser(const AppSpawningCtx *property, uid_t *user)
{
    const char *bundleName = GetBundleName(property);
    APPSPAWN_CHECK(bundleName != NULL, return APPSPAWN_ARG_INVALID, "Can not get bundle name");

    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_TLV_NONE, "No tlv %{public}d in msg %{public}s",
                    TLV_DOMAIN_INFO, bundleName);
    APPSPAWN_CHECK(dacInfo->uid / UID_BASE != 0, return APPSPAWN_ARG_INVALID, "Not normal user");

    *user = dacInfo->uid / UID_BASE;
    return APPSPAWN_OK;
}

static int FillIsolateInfo(const AppSpawningCtx *property, IsolateDirInfo *isolateDirInfo)
{
    uid_t user = 0;
    int ret = GetNormalUser(property, &user);
    if (ret != 0) {
        APPSPAWN_LOGE("Get normal User failed, ret %{public}d", ret);
        return ret;
    }

    for (int dirIdx = 0; dirIdx < ISOLATE_PATH_NUM; dirIdx++) {
        ret = snprintf_s(isolateDirInfo->dirs[dirIdx].path, ISOLATE_PATH_BUFF_SIZE,
                         ISOLATE_PATH_BUFF_SIZE - 1, "%s/%u/%s",
                         g_dirAffix[dirIdx].prefix, user, g_dirAffix[dirIdx].suffix);
        if (ret < 0) {
            APPSPAWN_LOGE("snprintf_s dir path failed, dirIdx %{public}d, ret %{public}d", dirIdx, ret);
            isolateDirInfo->dirs[dirIdx].len = 0;
            continue;
        }

        isolateDirInfo->dirs[dirIdx].len = ret;
    }

    return 0;
}

APPSPAWN_STATIC int SetIsolateDir(const AppSpawningCtx *property)
{
    int ret = 0;
    IsolateDirInfo isolateDirInfo = {0};

    ret = MallocIsolatePath(&isolateDirInfo);
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to malloc isolate dir");
        return APPSPAWN_SYSTEM_ERROR;
    }

    ret = FillIsolateInfo(property, &isolateDirInfo);
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to fill isolate dir info");
        FreeIsolatePath(&isolateDirInfo);
        return APPSPAWN_SYSTEM_ERROR;
    }

    int fd = open("/dev/dec", O_RDWR);
    if (fd < 0) {
        APPSPAWN_LOGE("Open dec file fail, errno %{public}d", errno);
        FreeIsolatePath(&isolateDirInfo);
        return APPSPAWN_SYSTEM_ERROR;
    }

    ret = ioctl(fd, ADD_ISOLATE_DIR_CMD, &isolateDirInfo);
    if (ret != 0) {
        APPSPAWN_LOGE("ioctl ADD_ISOLATE_DIR_CMD fail, errno %{public}d", errno);
        FreeIsolatePath(&isolateDirInfo);
        close(fd);
        return APPSPAWN_SYSTEM_ERROR;
    }

    APPSPAWN_LOGI("ioctl ADD_ISOLATE_DIR_CMD success");
    close(fd);

    FreeIsolatePath(&isolateDirInfo);
    return APPSPAWN_OK;
}

static bool IsIsolateDirNeeded(const AppSpawnMgr *content)
{
    /*
    * NWebSpawn is used for spawn render process and gpu process,
    * so it is no need to set isolate dir.
    */
    if (IsNWebSpawnMode(content)) {
        return false;
    }

    return true;
}

static int SpawnSetIsolateDir(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (IsIsolateDirNeeded(content)) {
        int ret = SetIsolateDir(property);
        APPSPAWN_CHECK_ONLY_LOG(ret == 0, "Failed to set isolate dir, ret %{public}d", ret);
    }

    return APPSPAWN_OK;
}

MODULE_CONSTRUCTOR(void)
{
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, HOOK_PRIO_HIGHEST, SpawnSetIsolateDir);
}
