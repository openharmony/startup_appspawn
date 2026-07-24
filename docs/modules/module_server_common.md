# Module: server_common

> 返回: [索引](../index.md)


## Overview

server_common 模块提供 appspawn 的通用服务端功能，包括服务创建（socket 监听）和性能追踪（HiTrace）。

## Source Location
- Directory: `common/`
- Files: 2 C/C++ 源文件, 2 头文件
- Estimated LOC: ~600

## Dependencies
- Depends on: util, interfaces/innerkits
- Used by: standard, lite

---

## KP-1: AppSpawnContent 与服务创建

**Priority**: P2

### Summary

`appspawn_server.h` 定义了 `AppSpawnContent` 结构体，这是 appspawn 服务的核心内容载体。`AppSpawnCreateContent()` 负责创建 socket 服务并初始化。

### Key Code

#### AppSpawnContent 结构体
```c
// common/appspawn_server.h:98
typedef struct AppSpawnContent {
    char *longProcName;
    uint32_t longProcNameLen;
    uint32_t sandboxNsFlags;
    int wdgOpened;
    bool isLinux;
    int sandboxType;
    RunMode mode;
    int signalFd;
    char *propertyBuffer;       // 共享内存映射（prefork 用）
    pid_t reservedPid;          // prefork 预留 PID
    int enablePerfork;          // 是否启用 prefork
    struct TagSandboxQueue *permissionQueue;
    // 函数指针表
    void (*runAppSpawn)(struct AppSpawnContent *, int, char *const []);
    void (*notifyResToParent)(struct AppSpawnContent *, AppSpawnClient *, int);
    int (*runChildProcessor)(struct AppSpawnContent *, AppSpawnClient *);
    int (*coldStartApp)(struct AppSpawnContent *, AppSpawnClient *);
} AppSpawnContent;
```

`AppSpawnContent` 使用函数指针表实现多态：不同的孵化模式（appspawn/nwebspawn/cold run）设置不同的函数实现。

---

## KP-2: 性能追踪

**Priority**: P2

### Summary

`appspawn_trace.cpp` 提供 HiTrace 集成，用于性能分析。

### Key API

- `StartAppspawnTrace(name)`: 开始 trace 区间
- `FinishAppspawnTrace()`: 结束 trace 区间

---

## Key Data Structures

| Structure | File | Purpose |
|-----------|------|---------|
| AppSpawnContent | `appspawn_server.h:98` | 服务内容（函数指针表模式实现多态） |
| RunMode | `appspawn_server.h:33` | 运行模式枚举 |
| AppSpawnClient | `appspawn_server.h:66` | 客户端标识 |

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| 创建服务 | standard | AppSpawnCreateContent | Incoming |
| 函数指针回调 | standard | runAppSpawn/runChildProcessor | Outgoing |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| standard | used_by | [module_standard.md](module_standard.md) |
| util | depends_on | [module_util.md](module_util.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
