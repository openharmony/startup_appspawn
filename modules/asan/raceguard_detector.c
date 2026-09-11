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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "securec.h"

#ifdef RACEGUARD_ENABLE
// 需求 6.1: so 路径仅来自编译期常量宏, 不拼接任何外部输入
#define RACEGUARD_LD_PRELOAD "/system/lib64/libraceguard.z.so"
// 需求 6.2: LD_PRELOAD 拼接缓冲区, 容纳已有值 + 分隔符 + raceguard so 路径
#define RACEGUARD_ENV_BUFFER 512
// 晚于 asan(HOOK_PRIO_COMMON)/dfx(HOOK_PRIO_DFX_PRELOAD) 执行, 保证多检测器互斥判定顺序
#define HOOK_PRIO_RACEGUARD (HOOK_PRIO_DFX_PRELOAD + 100)

// 组装 LD_PRELOAD 值: 已有值冒号追加(不覆盖), 已含 raceguard so 路径时幂等跳过
APPSPAWN_STATIC int BuildRaceGuardPreloadValue(const char *curValue, char *out, size_t outLen)
{
    if (curValue != NULL && strstr(curValue, RACEGUARD_LD_PRELOAD) != NULL) {
        return -1;  // 幂等: 已包含, 无需重复追加
    }
    int len = (curValue == NULL || *curValue == '\0') ?
        sprintf_s(out, outLen, "%s", RACEGUARD_LD_PRELOAD) :
        sprintf_s(out, outLen, "%s:%s", curValue, RACEGUARD_LD_PRELOAD);
    // 需求 6.2: securec 返回值必检, 失败(含截断)不注入
    APPSPAWN_CHECK(len > 0 && len < (int)outLen, return -1, "Invalid to format raceguard preload env");
    return 0;
}

// 判定 + 注入入口, 注册于 STAGE_CHILD_PRE_COLDBOOT(fork 子进程内, 冷启动判定之前)
APPSPAWN_STATIC int SetRaceGuardPreloadEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    // 需求 1.3: 模块安装于 common 目录被各孵化器进程加载, 仅标准 appspawn 的应用孵化注入
    APPSPAWN_CHECK_ONLY_EXPER(content != NULL && property != NULL && IsAppSpawnMode(content), return 0);
    // 需求 2.1: native 进程经 nativespawn 孵化, 不在本特性范围
    APPSPAWN_CHECK_ONLY_EXPER(GetAppSpawnMsgType(property) != MSG_SPAWN_NATIVE_PROCESS, return 0);

    // 需求 2.4: 多检测器互斥, asan/dfx/hwasan 等已启用冷启动(APP_COLD_START 已置位)时跳过
    if (property->client.flags & APP_COLD_START) {
        APPSPAWN_LOGI("RaceGuard: cold start already enabled, skip %{public}s", GetProcessName(property));
        return 0;
    }
    // 需求 3.3: 依赖可用性降级, so 缺失时跳过注入并告警, 不阻塞孵化
    if (access(RACEGUARD_LD_PRELOAD, F_OK) != 0) {
        APPSPAWN_LOGW("RaceGuard: so missing, degrade %{public}s", RACEGUARD_LD_PRELOAD);
        return 0;
    }

    char buff[RACEGUARD_ENV_BUFFER] = {0};
    const char *cur = getenv("LD_PRELOAD");
    int ret = BuildRaceGuardPreloadValue(cur, buff, sizeof(buff));
    APPSPAWN_CHECK(ret == 0, return 0, "RaceGuard: build env fail");

    setenv("LD_PRELOAD", buff, 1);  // 需求 2.3: 追加不覆盖, 经 execv 冷启动对动态链接器生效(需求 2.2)
    property->client.flags |= APP_COLD_START;  // 触发 AppSpawnChild 冷启动判定走 execv 路径
    APPSPAWN_LOGI("RaceGuard: preload enabled for %{public}s", GetProcessName(property));  // 需求 4.1
    return 0;  // 恒返回 0: 注入相关任何失败不阻塞孵化(需求 3)
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGI("Load raceguard module ...");
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, HOOK_PRIO_RACEGUARD, SetRaceGuardPreloadEnv);
}
#endif  // RACEGUARD_ENABLE
