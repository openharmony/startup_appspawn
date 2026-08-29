/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "spawn_primitives.h"

#include <errno.h>
#include <grp.h>
#include <stdint.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

#include "appspawn_utils.h"
#include "securec.h"

#ifdef WITH_SELINUX
#include <selinux/selinux.h>
#endif

#define BITLEN32 32

/**
 * @brief 设置子进程 uid/gid/补充组（纯机制原语）。
 *
 * 仅封装 setgroups→setresgid→setresuid 的内核调用序列与顺序不变量
 * （setresgid 先于 setresuid）。不读 TLV、不感知孵化类型；
 * 参数来源（TLV/策略表）由共享 Hook 决定，见 daemon_spawn_design §6.9。
 *
 * @param uid 目标 uid；daemon 策略传 0，app 由 Hook 从 TLV_DAC 取
 * @param gid 目标 gid
 * @param gids 补充组表；为 NULL 表示无补充组
 * @param gidCount gids 元素数；gids 为 NULL 时忽略
 * @return 0 成功；负数失败（-errno）
 */
int SetUidGid(uid_t uid, gid_t gid, const gid_t *gids, size_t gidCount)
{
    if (gids != NULL && gidCount > 0) {
        if (setgroups(gidCount, gids) != 0) {
            APPSPAWN_LOGE("setgroups failed: %{public}d, size=%{public}zu", errno, gidCount);
            return -errno;
        }
    }
    if (setresgid(gid, gid, gid) != 0) {
        APPSPAWN_LOGE("setresgid(%{public}u) failed: %{public}d", gid, errno);
        return -errno;
    }
    // setresgid 必须先于 setresuid：setresuid 从 0→非0 会清空 effective caps
    if (setresuid(uid, uid, uid) != 0) {
        APPSPAWN_LOGE("setresuid(%{public}u) failed: %{public}d", uid, errno);
        return -errno;
    }
    return 0;
}

/**
 * @brief 设置进程 capability 集合（纯机制原语）。
 *
 * 仅封装 capset 系统调用（inheritable/permitted/effective 三集合）。
 * 掩码由调用方（共享 Hook 读策略表）提供；app 侧的“flag→mask 计算”、
 * ext perm、isolated sandbox 等策略不在此原语，见 daemon_spawn_design §6.9。
 *
 * @param inheritable inheritable 集合位图
 * @param permitted permitted 集合位图
 * @param effective effective 集合位图
 * @return 0 成功；负数失败（-errno）
 */
int SetCapabilities(uint64_t inheritable, uint64_t permitted, uint64_t effective)
{
    struct __user_cap_header_struct capHeader;
    if (memset_s(&capHeader, sizeof(capHeader), 0, sizeof(capHeader)) != EOK) {
        APPSPAWN_LOGE("Failed to memset cap header");
        return -EINVAL;
    }
    capHeader.version = _LINUX_CAPABILITY_VERSION_3;
    capHeader.pid = 0;
    struct __user_cap_data_struct capData[2];
    if (memset_s(capData, sizeof(capData), 0, sizeof(capData)) != EOK) {
        APPSPAWN_LOGE("Failed to memset cap data");
        return -EINVAL;
    }
    capData[0].inheritable = (__u32)(inheritable);
    capData[1].inheritable = (__u32)(inheritable >> BITLEN32);
    capData[0].permitted = (__u32)(permitted);
    capData[1].permitted = (__u32)(permitted >> BITLEN32);
    capData[0].effective = (__u32)(effective);
    capData[1].effective = (__u32)(effective >> BITLEN32);
    if (capset(&capHeader, &capData[0]) != 0) {
        APPSPAWN_LOGE("Failed to capset errno: %{public}d", errno);
        return -errno;
    }
    return 0;
}

/**
 * @brief 设置 ambient capability 集合（纯机制原语）。
 *
 * 遍历掩码中置位的每个 cap，逐个 prctl(PR_CAP_AMBIENT, RAISE)。
 * 掩码由调用方提供；app 侧的 CAP_DAC_OVERRIDE/CAP_KILL/CAP_FOWNER
 * 条件计算不在此原语，见 daemon_spawn_design §6.9。
 *
 * @param mask ambient 能力位图
 * @return 0 成功；-1 失败
 */
int SetAmbientCapabilities(uint64_t mask)
{
    if (mask == 0) {
        return 0;
    }
    for (int cap = 0; cap < BITLEN32 * (int)(sizeof(uint64_t) / sizeof(uint32_t)); cap++) {
        if ((mask & (CAP_TO_MASK(cap))) == 0) {
            continue;
        }
        if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, cap, 0, 0) != 0) {
            APPSPAWN_LOGE("prctl PR_CAP_AMBIENT raise cap %{public}d failed: %{public}d", cap, errno);
            return -1;
        }
    }
    return 0;
}

/**
 * @brief 设置子进程 SELinux 上下文（纯机制原语）。
 *
 * 仅封装 setcon()。上下文串由调用方（共享 Hook 读策略表）提供；
 * app 侧的 HapDomain 解析（provision type/APL→域）不在此原语，
 * 由 app Hook 算出 context 后调本原语，见 daemon_spawn_design §6.9。
 * 编译需 WITH_SELINUX；未启用时为空实现返回 0。
 *
 * @param context 目标 SELinux 上下文串，如 "u:r:sudo:s0"
 * @return 0 成功；-1 失败（或未启用 SELinux 时 0）
 */
int SetSelinuxCon(const char *context)
{
    if (context == NULL) {
        return 0;
    }
#ifdef WITH_SELINUX
    if (setcon(context) != 0) {
        APPSPAWN_LOGE("setcon(%{public}s) failed: %{public}d", context, errno);
        return -1;
    }
#else
    (void)context;
#endif
    return 0;
}

/**
 * @brief 执行目标二进制（纯机制原语）。
 *
 * envp 为 NULL 时走 execv，否则走 execve。路径与参数由调用方
 * （共享 Hook 读策略表 execFrom：app→TLV:hap_entry，daemon→ext:daemon_bin_path）
 * 提供，见 daemon_spawn_design §6.9。成功不返回。
 *
 * @param path 目标二进制路径
 * @param argv 参数数组（以 NULL 结尾）
 * @param envp 环境变量数组；NULL 表示用 execv（继承当前环境）
 * @return 失败返回 -errno；成功不返回
 */
int ExecvTarget(const char *path, char *const argv[], char *const envp[])
{
    if (path == NULL || argv == NULL) {
        return -EINVAL;
    }
    if (envp == NULL) {
        execv(path, argv);
    } else {
        execve(path, argv, envp);
    }
    APPSPAWN_LOGE("exec failed, path: %{public}s errno: %{public}d", path, errno);
    return -errno;
}
