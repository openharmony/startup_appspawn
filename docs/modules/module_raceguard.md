# Module: raceguard

> 返回: [索引](../index.md)


## Overview

raceguard 模块在应用冷启动时通过 `LD_PRELOAD` 自动预加载 RaceGuard so（`/system/lib64/libraceguard.z.so`），为问题定位工程师提供零误报的数据竞争检测能力。仅产品编译配置 `raceguard_detector = true`（构建级 GN 参数，机制同 `asan_detector`，声明于 build 仓 `build/config/sanitizers/sanitizers.gni`）时编入；未配置的产品完全不受影响。

模块遵循仓内 ASAN/DFX 预加载的成熟机制：在 `STAGE_CHILD_PRE_COLDBOOT` 阶段（fork 子进程内、冷启动判定之前）设置 `LD_PRELOAD` 并置 `APP_COLD_START` 标志，经 `execv` 冷启动使动态链接器真正加载 so。

## Source Location
- Directory: `modules/asan/`（天网版本检测器，与 asan 同属检测器族）
- Files: 1 C 源文件（`modules/asan/raceguard_detector.c`）

## Dependencies
- Depends on: modulemgr_engine（hook 注册）
- Used by: 通过 Hook 被孵化流程调用（条件编入 `appspawn_asan` 模块 so，安装于 `lib64/appspawn`，经 `ASAN_MODULE_PATH` 被 `StartSpawnService`（各孵化器共享入口）显式加载，hook 内按 `IsAppSpawnMode` 守卫）

---

## KP-1: 注入流程与守卫

**Priority**: P0

### Summary

模块以 `MODULE_CONSTRUCTOR` 在加载时注册 `STAGE_CHILD_PRE_COLDBOOT` hook（优先级 `HOOK_PRIO_DFX_PRELOAD + 100`，晚于 asan/dfx 判定）。hook 依次执行：

1. `IsAppSpawnMode` 守卫：仅标准 appspawn 的应用孵化注入（检测器代码随 asan so 被各孵化器加载，运行时自判）；
2. msg-type 守卫：`MSG_SPAWN_NATIVE_PROCESS`（native 经 nativespawn 孵化）跳过；
3. 多检测器互斥：`APP_COLD_START` 已置位（asan/dfx/hwasan 等已启用冷启动）时跳过；
4. 依赖降级：`access(F_OK)` 检查 so 存在性，缺失则记录 `APPSPAWN_LOGW` 告警日志，应用正常启动；
5. 组装 `LD_PRELOAD`（已有值冒号追加、已包含时幂等跳过，`sprintf_s` 全检返回值）并 `setenv`；
6. `flags |= APP_COLD_START` 触发 `AppSpawnChild` 冷启动判定走 `execv` 路径。

hook 恒返回 0：注入相关任何失败不阻塞孵化。

### Key Code

```c
// modules/asan/raceguard_detector.c
#define RACEGUARD_LD_PRELOAD "/system/lib64/libraceguard.z.so"  // 编译期常量，无注入面
#define HOOK_PRIO_RACEGUARD (HOOK_PRIO_DFX_PRELOAD + 100)

MODULE_CONSTRUCTOR(void)
{
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, HOOK_PRIO_RACEGUARD, SetRaceGuardPreloadEnv);
}
```

## KP-2: 编译开关与降级

**Priority**: P0

### Summary

- **开关**：`raceguard_detector`（build 仓 sanitizers.gni declare_args，默认 `false`）为唯一开关，直接映射 `RACEGUARD_ENABLE` 宏；`modules/asan/BUILD.gn` 在既有 `appspawn_asan` 目标内以 `if (raceguard_detector)` 条件块编入本文件（条件 sources + define，参照 `if (is_asan) { defines += [ "APPSPAWN_ASAN" ] }` 先例，不新增独立 so）。默认关闭时源文件不参与编译（文件隔离），`libappspawn_asan.z.so` 与存量完全一致，根 BUILD.gn 与可执行目标零改动；回退通过版本编译配置（`raceguard_detector = false`）实现，无运行时参数。
- **降级**：so 缺失/不可访问时跳过注入、记录 `APPSPAWN_LOGW` 告警，应用正常启动；冷启动 `execv` 失败按存量 "cold start fail, to start normal" 路径回退普通孵化。

## References
- SDD 制品：`OpenHarmonyAI/sdd-delivery/startup_appspawn/20260803816765/`
- 同机制先例：`modules/asan/asan_detector.c`、`modules/ace_adapter/dfx_preload.cpp`
- 单元测试：`test/unittest/app_spawn_standard_test/app_spawn_raceguard_test/`（`--wrap=access` 模拟 so 存在性）
