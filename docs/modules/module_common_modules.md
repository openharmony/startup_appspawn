# Module: common_modules

> 返回: [索引](../index.md)


## Overview

common_modules 包含 appspawn 的通用功能 Hook 模块，提供 cgroup 配置、namespace 管理、进程隔离、外部依赖适配、DFX 转储、SILK 引擎集成等功能。这些模块通过 `MODULE_CONSTRUCTOR` 宏在加载时自动注册到 Hook 体系。

## Source Location
- Directory: `modules/common/`
- Files: 10 C/C++ 源文件, 3 头文件
- Estimated LOC: ~3,070

## Dependencies
- Depends on: modulemgr_engine
- Used by: 通过 Hook 被 standard 模块调用

---

## KP-1: Cgroup 配置管理

**Priority**: P1

### Summary

`appspawn_cgroup.c` 负责将孵化进程分配到适当的 cgroup，实现资源隔离和优先级管理。

### Key Code

cgroup 配置包括 CPU、内存等资源限制。通过写入 cgroup 虚拟文件系统设置进程的 cgroup 归属。文件大小 ~389 行，涵盖多种 cgroup 控制器的配置。

---

## KP-2: 命名空间管理

**Priority**: P1

### Summary

`appspawn_namespace.c` 负责管理 mount namespace 和其他 Linux 命名空间，为应用进程提供隔离的文件系统视图。

---

## KP-3: 进程隔离

**Priority**: P1

### Summary

`appspawn_isolate.c` 实现进程级别的隔离机制，包括网络隔离、文件系统隔离等。

---

## KP-4: 通用适配层

**Priority**: P2

### Summary

`appspawn_adapter.cpp` 提供外部依赖的适配函数，包括 SELinux 上下文设置、系统参数读取等。

---

## KP-5: 封装工具与 SILK 引擎

**Priority**: P2

### Summary

- `appspawn_encaps.c`: 进程信息封装工具
- `appspawn_silk.c`: SILK 引擎集成（用于应用启动加速）

---

## Key Data Structures

| Structure | File | Purpose |
|-----------|------|---------|
| AppSpawnStartArg | `appspawn_server.h:65` | 启动参数 |

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| Hook 注册 | modulemgr_engine | MODULE_CONSTRUCTOR | Outgoing |
| 日志/工具 | util | APPSPAWN_LOGI | Outgoing |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| modulemgr_engine | depends_on | [module_modulemgr_engine.md](module_modulemgr_engine.md) |
| standard | used_by | [module_standard.md](module_standard.md) |
| util | depends_on | [module_util.md](module_util.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
