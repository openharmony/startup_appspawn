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

#ifndef SPAWN_PRIMITIVES_H
#define SPAWN_PRIMITIVES_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

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
int SetUidGid(uid_t uid, gid_t gid, const gid_t *gids, size_t gidCount);

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
int SetCapabilities(uint64_t inheritable, uint64_t permitted, uint64_t effective);

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
int SetAmbientCapabilities(uint64_t mask);

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
int SetSelinuxCon(const char *context);

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
int ExecvTarget(const char *path, char *const argv[], char *const envp[]);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif  // SPAWN_PRIMITIVES_H
