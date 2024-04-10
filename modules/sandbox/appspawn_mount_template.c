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

#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "securec.h"

/*
名称            fs-type      flags                          options                 说明
default                      MS_BIND MS_REC                                         默认可读写
rdonly                       MS_NODEV MS_RDONLY                                     只读挂载
epfs            epfs         MS_NODEV                                                待讨论
dac_override                 MS_NODEV MS_RDONLY             "support_overwrite=1"
开启const.filemanager.full_mount.enable
fuse            fuse         MS_NOSUID MS_NODEV MS_NOEXEC MS_NOATIME MS_LAZYTIME
dlp_fuse        fuse         MS_NOSUID MS_NODEV MS_NOEXEC MS_NOATIME MS_LAZYTIME     为dlpmanager管理应用专用的挂载参数
shared                       MS_BIND MS_REC                                           root
namespace上是MS_SHARED方式挂载

*/
/**
    "dac-override-sensitive": "true",  默认值：false。使能后直接使用默认值
    "sandbox-flags-customized": [ "MS_NODEV", "MS_RDONLY" ],
    "fs-type": "sharefs",
    "options": "support_overwrite=1"
*/
#define FUSE_MOUNT_FLAGS (MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_NOATIME | MS_LAZYTIME)
#define FUSE_MOUNT_OPTIONS  \
    "fd=%d, rootmode=40000,user_id=%d,group_id=%d,allow_other," \
    "context=\"u:object_r:dlp_fuse_file:s0\", fscontext=u:object_r:dlp_fuse_file:s0"

static const MountArgTemplate DEF_MOUNT_ARG_TMP[MOUNT_TMP_MAX] = {
    {"default", MOUNT_TMP_DEFAULT, NULL, MS_BIND | MS_REC, NULL, MS_SLAVE},
    {"rdonly", MOUNT_TMP_RDONLY, NULL, MS_NODEV | MS_RDONLY, NULL, MS_SLAVE},
    {"epfs", MOUNT_TMP_EPFS, "epfs", MS_NODEV, NULL, MS_SLAVE},
    {"dac_override", MOUNT_TMP_DAC_OVERRIDE, "sharefs", MS_NODEV | MS_RDONLY, "support_overwrite=1", MS_SLAVE},
    {"fuse", MOUNT_TMP_FUSE, "fuse", FUSE_MOUNT_FLAGS, FUSE_MOUNT_OPTIONS, MS_SHARED},
    {"dlp_fuse", MOUNT_TMP_DLP_FUSE, "fuse", FUSE_MOUNT_FLAGS, FUSE_MOUNT_OPTIONS, MS_SHARED},
    {"shared", MOUNT_TMP_SHRED, NULL, MS_BIND | MS_REC, NULL, MS_SHARED},
};

static const SandboxFlagInfo MOUNT_FLAGS_MAP[] = {
    {"rec", MS_REC}, {"MS_REC", MS_REC},
    {"bind", MS_BIND}, {"MS_BIND", MS_BIND}, {"move", MS_MOVE}, {"MS_MOVE", MS_MOVE},
    {"slave", MS_SLAVE}, {"MS_SLAVE", MS_SLAVE}, {"rdonly", MS_RDONLY}, {"MS_RDONLY", MS_RDONLY},
    {"shared", MS_SHARED}, {"MS_SHARED", MS_SHARED}, {"unbindable", MS_UNBINDABLE},
    {"MS_UNBINDABLE", MS_UNBINDABLE}, {"remount", MS_REMOUNT}, {"MS_REMOUNT", MS_REMOUNT},
    {"nosuid", MS_NOSUID}, {"MS_NOSUID", MS_NOSUID}, {"nodev", MS_NODEV}, {"MS_NODEV", MS_NODEV},
    {"noexec", MS_NOEXEC}, {"MS_NOEXEC", MS_NOEXEC}, {"noatime", MS_NOATIME}, {"MS_NOATIME", MS_NOATIME},
    {"lazytime", MS_LAZYTIME}, {"MS_LAZYTIME", MS_LAZYTIME}
};

static const SandboxFlagInfo PATH_MODE_MAP[] = {
    {"S_IRUSR", S_IRUSR}, {"S_IWUSR", S_IWUSR}, {"S_IXUSR", S_IXUSR},
    {"S_IRGRP", S_IRGRP}, {"S_IWGRP", S_IWGRP}, {"S_IXGRP", S_IXGRP},
    {"S_IROTH", S_IROTH}, {"S_IWOTH", S_IWOTH}, {"S_IXOTH", S_IXOTH},
    {"S_IRWXU", S_IRWXU}, {"S_IRWXG", S_IRWXG}, {"S_IRWXO", S_IRWXO}
};

uint32_t GetMountCategory(const char *name)
{
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL, return MOUNT_TMP_DEFAULT);
    uint32_t count = ARRAY_LENGTH(DEF_MOUNT_ARG_TMP);
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(DEF_MOUNT_ARG_TMP[i].name, name) == 0) {
            return DEF_MOUNT_ARG_TMP[i].category;
        }
    }
    return MOUNT_TMP_DEFAULT;
}

const MountArgTemplate *GetMountArgTemplate(uint32_t category)
{
    uint32_t count = ARRAY_LENGTH(DEF_MOUNT_ARG_TMP);
    if (category > count) {
        return NULL;
    }
    for (uint32_t i = 0; i < count; i++) {
        if (DEF_MOUNT_ARG_TMP[i].category == category) {
            return &DEF_MOUNT_ARG_TMP[i];
        }
    }
    return NULL;
}

const SandboxFlagInfo *GetSandboxFlagInfo(const char *key, const SandboxFlagInfo *flagsInfos, uint32_t count)
{
    APPSPAWN_CHECK_ONLY_EXPER(key != NULL, return NULL);
    APPSPAWN_CHECK_ONLY_EXPER(flagsInfos != NULL, return NULL);
    for (uint32_t i = 0; i < count; i++) {
        if (strcmp(flagsInfos[i].name, key) == 0) {
            return &flagsInfos[i];
        }
    }
    return NULL;
}

int GetPathMode(const char *name)
{
    APPSPAWN_CHECK_ONLY_EXPER(name != NULL, return 0);
    const SandboxFlagInfo *info = GetSandboxFlagInfo(name, PATH_MODE_MAP, ARRAY_LENGTH(PATH_MODE_MAP));
    if (info != NULL) {
        return info->flags;
    }
    return 0;
}

static void DumpSandboxFlags(char *buffer, uint32_t bufferSize, unsigned long flags,
    const SandboxFlagInfo *flagsInfos, uint32_t count)
{
    bool first = true;
    size_t currLen = 0;
    int len = 0;
    unsigned long tmp = flags;
    for (uint32_t i = 0; i < count; i++) {
        if ((flagsInfos[i].flags & tmp) == 0) {
            continue;
        }
        tmp &= ~(flagsInfos[i].flags);
        if (!first) {
            len = sprintf_s(buffer + currLen, bufferSize - currLen - 1, " %s ", "|");
            APPSPAWN_CHECK_ONLY_EXPER(len > 0, return);
            currLen += len;
        }
        first = false;
        len = sprintf_s(buffer + currLen, bufferSize - currLen, "%s", flagsInfos[i].name);
        APPSPAWN_CHECK_ONLY_EXPER(len > 0, return);
        currLen += len;
    }
}

static void DumpMode(const char *info, mode_t mode)
{
    APPSPAWN_CHECK_ONLY_EXPER(info != NULL, return);
    char buffer[64] = {0};  // 64 to show flags
    DumpSandboxFlags(buffer, sizeof(buffer), mode, PATH_MODE_MAP, ARRAY_LENGTH(PATH_MODE_MAP));
    APPSPAPWN_DUMP("%{public}s[0x%{public}x] %{public}s", info, (uint32_t)(mode), buffer);
}

static void DumpMountFlags(const char *info, unsigned long mountFlags)
{
    APPSPAWN_CHECK_ONLY_EXPER(info != NULL, return);
    char buffer[128] = {0};  // 64 to show flags
    DumpSandboxFlags(buffer, sizeof(buffer), mountFlags, MOUNT_FLAGS_MAP, ARRAY_LENGTH(MOUNT_FLAGS_MAP));
    APPSPAPWN_DUMP("%{public}s[0x%{public}x] %{public}s", info, (uint32_t)(mountFlags), buffer);
}

void DumpMountPathMountNode(const PathMountNode *pathNode)
{
    const MountArgTemplate *tmp = GetMountArgTemplate(pathNode->category);
    if (tmp == NULL) {
        return;
    }
    APPSPAPWN_DUMP("        sandbox node category: %{public}u(%{public}s)", tmp->category, tmp->name);
    DumpMountFlags("        sandbox node mountFlags: ", tmp->mountFlags);
    APPSPAPWN_DUMP("        sandbox node mountSharedFlag: %{public}s",
        tmp->mountSharedFlag == MS_SLAVE ? "MS_SLAVE" : "MS_SHARED");
    APPSPAPWN_DUMP("        sandbox node options: %{public}s", tmp->options ? tmp->options : "null");
    APPSPAPWN_DUMP("        sandbox node fsType: %{public}s", tmp->fsType ? tmp->fsType : "null");
    DumpMode("        sandbox node destMode: ", pathNode->destMode);
    APPSPAPWN_DUMP("        sandbox node config mountSharedFlag: %{public}s",
        pathNode->mountSharedFlag ? "MS_SHARED" : "MS_SLAVE");
}