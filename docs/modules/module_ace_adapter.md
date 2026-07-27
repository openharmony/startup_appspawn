# Module: ace_adapter

> 返回: [索引](../index.md)


## Overview

ace_adapter 模块负责 ACE（ArkUI Cross-platform Engine）的适配工作，包括 ArkUI 应用的 checkpoint/镜像进程管理、命令解析和 DFX 预加载。该模块通过 Hook 机制集成到 appspawn 的孵化流程中。

## Source Location
- Directory: `modules/ace_adapter/`
- Files: 4 C/C++ 源文件, 1 头文件
- Estimated LOC: ~1,313

## Dependencies
- Depends on: modulemgr_engine
- Used by: 通过 Hook 被 standard 模块调用

---

## KP-1: ACE 适配主流程

**Priority**: P1

### Summary

`ace_adapter.cpp` (~524行) 是 ACE 适配的主文件，负责模块注册和 Hook 挂载。它在 `MODULE_CONSTRUCTOR` 中注册各孵化阶段的处理函数。

---

## KP-2: Checkpoint/镜像进程管理

**Priority**: P1

### Summary

`appspawn_checkpoint.c` (~573行) 实现镜像进程（image process）的创建和管理。镜像进程是预先 fork 的进程快照，通过 checkpoint 机制加速后续应用启动。

### Key Data Structures

```c
// modules/module_engine/include/appspawn_msg.h:128
#define APP_CHECKPOINT_NAME_LEN 256
typedef struct {
    pid_t imgPid;                           // 镜像进程 PID
    uint64_t checkPointId;                  // checkpoint ID
    char imgName[APP_CHECKPOINT_NAME_LEN];  // 镜像进程名
} AppSpawnCheckpointInfo;
```

在 `AppSpawnMgr` 中维护了 `checkPointIdQueue` 链表来跟踪所有镜像进程。

---

## KP-3: 命令解析器与 DFX 预加载

**Priority**: P2

### Summary

- `command_lexer.cpp` (~103行): 解析命令行参数
- `dfx_preload.cpp` (~68行): DFX 预加载功能

---

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| Hook 注册 | modulemgr_engine | MODULE_CONSTRUCTOR | Outgoing |
| checkpoint 队列 | standard | checkPointIdQueue | Shared |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| modulemgr_engine | depends_on | [module_modulemgr_engine.md](module_modulemgr_engine.md) |
| standard | used_by | [module_standard.md](module_standard.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
