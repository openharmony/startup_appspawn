# Module: modulemgr_engine

> 返回: [索引](../index.md)


## Overview

modulemgr_engine 模块是 appspawn 模块化架构的基础设施。它定义了 Hook 阶段体系、Hook 优先级、扩展数据模型和 TLV 消息格式。模块管理器负责动态加载功能模块（.so 文件），Hook 引擎按阶段和优先级执行注册的处理函数。

## Source Location
- Directory: `modules/modulemgr/`, `modules/module_engine/`
- Files: appspawn_modulemgr.c, appspawn_modulemgr.h, appspawn_hook.h, appspawn_msg.h
- Estimated LOC: ~700

## Dependencies
- Depends on: client_api（消息头文件）
- Used by: 几乎所有功能模块

---

## KP-1: Hook 阶段定义与执行

**Priority**: P0

### Summary

appspawn 的整个孵化流程被划分为多个 Hook 阶段（`AppSpawnHookStage`），每个阶段对应孵化流程的一个特定时间点。功能模块通过 `AddAppSpawnHook` / `AddServerStageHook` 注册处理函数。

### Key Code

#### Hook 阶段枚举
```c
// modules/module_engine/include/appspawn_hook.h:60
typedef enum TagAppSpawnHookStage {
    // 服务状态处理
    STAGE_SERVER_PRELOAD  = 10,     // 服务预加载
    STAGE_SERVER_LOCK,              // 服务锁
    STAGE_SERVER_ARKWEB_PRELOAD,    // ArkWeb 预加载
    STAGE_SERVER_ARKWEB_UNLOAD,
    STAGE_SERVER_EXIT,              // 服务退出
    // 应用状态处理
    STAGE_SERVER_APP_ADD,           // 应用添加
    STAGE_SERVER_APP_CLEANUP,       // 应用清理（只触发一次）
    STAGE_SERVER_APP_DIED,          // 应用死亡
    // fork 前后（父进程）
    STAGE_PARENT_PRE_FORK = 20,     // fork 前
    STAGE_PARENT_POST_FORK = 21,    // fork 后
    STAGE_PARENT_PRE_RELY = 22,
    STAGE_PARENT_POST_RELY = 23,
    STAGE_PARENT_MSG_DECODE,        // 消息解码后
    STAGE_PARENT_UNINSTALL,
    STAGE_PARENT_BOOT_IMG,
    STAGE_SERVER_SPAWN_ABORT,       // 孵化中止
    // 子进程
    STAGE_CHILD_PRE_COLDBOOT = 30,  // 冷启动前
    STAGE_CHILD_EXECUTE,            // 子进程执行（沙箱/权限设置）
    STAGE_CHILD_PRE_RELY,
    STAGE_CHILD_POST_RELY,
    STAGE_CHILD_PRE_RUN,            // 运行前
    STAGE_MAX
} AppSpawnHookStage;
```

#### Hook 优先级
```c
// modules/module_engine/include/appspawn_hook.h:89
typedef enum TagAppSpawnHookPrio {
    HOOK_PRIO_HIGHEST = 1000,
    HOOK_PRIO_COMMON = 2000,
    HOOK_PRIO_DFX_PRELOAD = 2500,
    HOOK_PRIO_SANDBOX = 3000,
    HOOK_PRIO_SANDBOX_MARK_PATH = 3500,
    HOOK_PRIO_PROPERTY = 4000,
    HOOK_PRIO_LOWEST = 5000,
} AppSpawnHookPrio;
```
同一阶段的 Hook 按优先级排序执行，数字小的先执行。

#### Hook 函数类型
```c
// modules/module_engine/include/appspawn_hook.h:105
typedef int (*ServerStageHook)(AppSpawnMgr *content);
typedef int (*AppSpawnHook)(AppSpawnMgr *content, AppSpawningCtx *property);
typedef int (*ProcessChangeHook)(const AppSpawnMgr *content, const AppSpawnedProcessInfo *appInfo);
```

#### 模块构造器宏
```c
// modules/module_engine/include/appspawn_hook.h:219
#define MODULE_CONSTRUCTOR(void) static void _init(void) __attribute__((constructor)); \
    static void _init(void)
#define MODULE_DESTRUCTOR(void) static void _destroy(void) __attribute__((destructor)); \
    static void _destroy(void)
```
功能模块使用此宏定义构造和析构函数，在 .so 加载/卸载时自动注册/注销 Hook。

---

## KP-2: 模块加载与管理

**Priority**: P1

### Summary

模块管理器（`appspawn_modulemgr.c`）负责动态加载功能模块 .so 文件，并根据模块类型（MODULE_APPSPAWN/MODULE_NWEBSPAWN 等）决定加载哪些模块。

### Key Code

#### 模块类型
```c
// modules/modulemgr/appspawn_modulemgr.h:38
typedef enum {
    MODULE_DEFAULT,
    MODULE_APPSPAWN,
    MODULE_NWEBSPAWN,
    MODULE_COMMON,
    MODULE_NATIVESPAWN,
    MODULE_HYBRIDSPAWN,
    MODULE_MAX
} AppSpawnModuleType;
```

#### Hook 执行接口
```c
// modules/modulemgr/appspawn_modulemgr.h:59
int ServerStageHookExecute(AppSpawnHookStage stage, AppSpawnContent *content);
int ProcessMgrHookExecute(AppSpawnHookStage stage,
    const AppSpawnContent *content, const AppSpawnedProcessInfo *appInfo);
int AppSpawnHookExecute(AppSpawnHookStage stage, uint32_t flags,
    AppSpawnContent *content, AppSpawnClient *client);
```

---

## KP-3: TLV 消息类型系统

**Priority**: P1

### Summary

TLV（Type-Length-Value）消息格式是 appspawn 客户端和服务端之间的通信协议。

### Key Code

#### TLV 类型定义
```c
// modules/module_engine/include/appspawn_msg.h:54
typedef enum {
    TLV_BUNDLE_INFO = 0,       // bundle name, index
    TLV_MSG_FLAGS,             // 消息标志位
    TLV_DAC_INFO,              // UID, GID, GID 表
    TLV_DOMAIN_INFO,           // APL, hapFlags
    TLV_OWNER_INFO,            // ownerId
    TLV_ACCESS_TOKEN_INFO,     // accessTokenIdEx
    TLV_PERMISSION,            // 权限位图
    TLV_INTERNET_INFO,         // 网络权限
    TLV_RENDER_TERMINATION_INFO, // 渲染终止信息
    TLV_CHECK_POINT_INFO,      // checkpoint 信息
    TLV_MAX
} AppSpawnMsgTlvType;
```

#### 消息头结构
```c
// modules/module_engine/include/appspawn_msg.h:135
typedef struct TagAppSpawnMsg {
    uint32_t magic;         // 0xEF201234
    uint32_t msgType;       // 消息类型（SPAWN/TERMINATE等）
    uint32_t msgLen;        // 消息总长度
    uint32_t msgId;         // 消息 ID（递增）
    uint32_t tlvCount;      // 扩展 TLV 数量
    char processName[APP_LEN_PROC_NAME];  // 进程名
} AppSpawnMsg;
```

#### TLV 和扩展 TLV
```c
// modules/module_engine/include/appspawn_msg.h:85
typedef struct {
    uint16_t tlvLen;
    uint16_t tlvType;
} AppSpawnTlv;

typedef struct {
    uint16_t tlvLen;
    uint16_t tlvType;
    uint16_t dataLen;
    uint16_t dataType;
    char tlvName[APPSPAWN_TLV_NAME_LEN];  // 32 字节名称
} AppSpawnTlvExt;
```

---

## Key Data Structures

| Structure | File | Purpose |
|-----------|------|---------|
| AppSpawnHookStage | `appspawn_hook.h:60` | Hook 阶段枚举 |
| AppSpawnHookPrio | `appspawn_hook.h:89` | Hook 优先级 |
| AppSpawnModuleType | `appspawn_modulemgr.h:38` | 模块类型 |
| AppSpawnMsg | `appspawn_msg.h:135` | 消息头 |
| AppSpawnTlv | `appspawn_msg.h:85` | 标准 TLV |
| AppSpawnTlvExt | `appspawn_msg.h:88` | 扩展 TLV |
| AppSpawnExtData | `appspawn_hook.h:53` | 扩展数据节点 |

## Public Interface

### `AddAppSpawnHook(stage, prio, hook)`
- **File**: `modules/module_engine/include/appspawn_hook.h:155`
- **Purpose**: 注册孵化阶段处理函数
- **Called by**: 各功能模块的 MODULE_CONSTRUCTOR

### `AddServerStageHook(stage, prio, hook)`
- **File**: `modules/module_engine/include/appspawn_hook.h:133`
- **Purpose**: 注册服务阶段处理函数

### `AppSpawnHookExecute(stage, flags, content, client)`
- **File**: `modules/modulemgr/appspawn_modulemgr.h:62`
- **Purpose**: 执行指定阶段的所有 Hook

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| Hook 执行 | standard | AppSpawnHookExecute | Incoming |
| Hook 执行 | sandbox | AddAppSpawnHook 注册 | Incoming |
| Hook 执行 | spm | AddAppSpawnHook 注册 | Incoming |
| Hook 执行 | common_modules | AddAppSpawnHook 注册 | Incoming |
| 消息格式 | client_api | AppSpawnMsg 共享 | Shared |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| standard | used_by | [module_standard.md](module_standard.md) |
| sandbox | used_by | [module_sandbox.md](module_sandbox.md) |
| spm | used_by | [module_spm.md](module_spm.md) |
| common_modules | used_by | [module_common_modules.md](module_common_modules.md) |
| ace_adapter | used_by | [module_ace_adapter.md](module_ace_adapter.md) |
| client_api | shares_with | [module_client_api.md](module_client_api.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
