# Module: adapters

> 返回: [索引](../index.md)


## Overview

adapters 模块包含多个适配器子模块，为不同的孵化模式（nativespawn、nwebspawn）和系统事件提供适配层。

## Source Location
- Directories: `modules/native_adapter/`, `modules/nweb_adapter/`, `modules/sysevent/`, `modules/asan/`
- Files: native_adapter.cpp, nwebspawn_adapter.cpp, appspawn_hisysevent.cpp, event_reporter.cpp, hisysevent_adapter.cpp, asan_detector.c
- Estimated LOC: ~2,000

## Dependencies
- Depends on: modulemgr_engine
- Used by: 通过 Hook 被 standard 模块调用

---

## KP-1: 原生适配器与 NWeb 适配器

**Priority**: P2

### Summary

- `native_adapter.cpp`: nativespawn 的适配层，注册 MODULE_NATIVESPAWN 类型的 Hook
- `nwebspawn_adapter.cpp`: nwebspawn 的适配层，注册 MODULE_NWEBSPAWN 类型的 Hook

这些适配器负责为各自的孵化模式提供特定的初始化和后处理逻辑。

---

## KP-2: 系统事件与 ASAN 检测

**Priority**: P2

### Summary

- `appspawn_hisysevent.cpp` + `event_reporter.cpp` + `hisysevent_adapter.cpp`: 系统事件（HiSysEvent）上报，包括孵化失败、超时等异常事件
- `asan_detector.c`: ASAN（AddressSanitizer）检测器模块，在编译时启用 ASAN 时加载

### Key Code

#### ASAN 模块路径
```c
// modules/modulemgr/appspawn_modulemgr.h:32
#if defined(__aarch64__) || defined(__x86_64__)
#define ASAN_MODULE_PATH "/system/lib64/appspawn/libappspawn_asan"
#else
#define ASAN_MODULE_PATH "/system/lib/appspawn/libappspawn_asan"
#endif
```

ASAN 模块作为独立的 .so 文件在 ASAN 模式下被动态加载。

---

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| Hook 注册 | modulemgr_engine | MODULE_CONSTRUCTOR | Outgoing |
| 事件上报 | sysevent（系统） | HiSysEvent API | Outgoing |
| 模块加载 | modulemgr_engine | AppSpawnLoadAutoRunModules | Incoming |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| modulemgr_engine | depends_on | [module_modulemgr_engine.md](module_modulemgr_engine.md) |
| standard | used_by | [module_standard.md](module_standard.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
