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

#ifndef SPAWN_POLICY_H
#define SPAWN_POLICY_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#include "appspawn_hook.h"  // AppSpawningCtx forward decl

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define SPAWN_POLICY_NAME_LEN 32
#define SPAWN_POLICY_CFG_LEN 64
#define SPAWN_POLICY_SELINUX_LEN 64
#define SPAWN_POLICY_MAX 8

// 参数来源标记：把"参数从哪来"数据化，共享 Hook 据此取参，见 daemon_spawn_design §6.4/§6.5
typedef enum {
    POLICY_SRC_INVALID = 0,
    POLICY_SRC_TLV_DAC,        // uid/gid 取自 TLV_DAC_INFO
    POLICY_SRC_FIXED,          // uid/gid 策略表固定值（daemon=0）
    POLICY_SRC_SUDO,           // caps = sudo 掩码（默认 0x3fffffffff，TBD-R1 待安全 owner 定）
    POLICY_SRC_TLV_CAPS,       // caps 取自 TLV
    POLICY_SRC_SELINUX_SUDO,   // selinux = sudo 域（默认 u:r:sudo:s0，TBD-R2 待 SELinux owner 定）
    POLICY_SRC_TLV_SELINUX,    // selinux 取自 TLV
    POLICY_SRC_TLV_BUNDLE,     // 沙箱 root 取自 TLV_BUNDLE_INFO
    POLICY_SRC_EXT_OWNER,      // 沙箱 root 取自属主 ext
    POLICY_SRC_TLV_HAP_ENTRY,  // exec 目标取自 TLV hap 入口
    POLICY_SRC_EXT_BIN_PATH,   // exec 目标取自 ext:daemon_bin_path
} PolicySrc;

/**
 * @brief 单条孵化策略（策略表条目）。
 *
 * 对应 appspawn-spawn-policy.json 的一条 policy。共享 Hook 读 ResolvePolicy()
 * 得到此结构，据此取参调引擎原语，见 daemon_spawn_design §6.4/§6.5。
 */
typedef struct {
    char name[SPAWN_POLICY_NAME_LEN];   // "app" / "daemon-owned" / "daemon-system"
    PolicySrc uidFrom;
    PolicySrc gidFrom;
    uid_t uid;                          // POLICY_SRC_FIXED 时的固定 uid（daemon=0）
    gid_t gid;                          // POLICY_SRC_FIXED 时的固定 gid
    PolicySrc capsFrom;                 // POLICY_SRC_SUDO / POLICY_SRC_TLV_CAPS
    uint64_t capsMask;                  // POLICY_SRC_SUDO 时的掩码（默认 TBD-R1）
    PolicySrc selinuxFrom;              // POLICY_SRC_SELINUX_SUDO / POLICY_SRC_TLV_SELINUX
    char selinuxCtx[SPAWN_POLICY_SELINUX_LEN]; // POLICY_SRC_SELINUX_SUDO 时的 ctx（默认 TBD-R2）
    char sandboxCfg[SPAWN_POLICY_CFG_LEN];     // 沙箱配置 JSON 文件名
    PolicySrc sandboxRootFrom;          // POLICY_SRC_TLV_BUNDLE / POLICY_SRC_EXT_OWNER
    PolicySrc execFrom;                 // POLICY_SRC_TLV_HAP_ENTRY / POLICY_SRC_EXT_BIN_PATH
    bool notifyAms;                     // 是否回调 ams（取代 isAppspawn 门控，见 §6.6）
} SpawnPolicy;

/**
 * @brief 启动期加载策略表 appspawn-spawn-policy.json。
 *
 * 解析 JSON 填充 g_policy 表；文件缺失/解析失败则回退内置默认 app 策略，
 * 保证不裸奔。在 main() InitCommonEnv 后调用，见 daemon_spawn_design §10.2 P2。
 *
 * @return 0 成功；-1 失败（已回退默认，不致命）
 */
int SpawnPolicyInit(void);

/**
 * @brief 按请求数据解析孵化策略（零类型分支的关键）。
 *
 * 粗分类信号 = 消息类型（P4 加 MSG_SPAWN_DAEMON 后启用 daemon 分支），
 * 细分类信号 = 请求数据（有无属主 ext）。P2 旁路态：仅返回 app 策略
 * （值与原硬编码一致），daemon 分类在 P4 启用，见 daemon_spawn_design §6.4。
 *
 * @param ctx 孵化上下文（含消息类型/TLV/ext）
 * @return 命中策略指针（永不为 NULL，未命中回退 app）
 */
const SpawnPolicy *ResolvePolicy(const AppSpawningCtx *ctx);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif  // SPAWN_POLICY_H
