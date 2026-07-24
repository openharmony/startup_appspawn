# Module: spm

> 返回: [索引](../index.md)


## Overview

SPM（Security Process Manager，安全进程管理器）是 OpenHarmony 内核的安全进程管理子系统。appspawn 的 spm 模块作为用户态客户端，与内核 SPM 交互，负责管理 `accessTokenIdEx` 与 `uid` 的引用计数，以及在孵化流程中重建权限 TLV 消息（从内核拉取最新的进程安全属性）。

SPM 模块在孵化流程中通过 `STAGE_PARENT_MSG_DECODE` Hook 拦截消息解码阶段，从内核获取最新的权限数据并重建 TLV 消息。同时通过 `STAGE_SERVER_SPAWN_ABORT` 和 `STAGE_SERVER_APP_CLEANUP` Hook 管理引用计数的回收。

## Source Location
- Directory: `modules/spm/`
- Files: 3 C 源文件, 3 头文件
- Estimated LOC: ~1,947

## Dependencies
- Depends on: modulemgr_engine, interfaces/innerkits/permission
- Used by: 通过 Hook 被 standard 模块调用

---

## KP-1: SPM 核心架构

**Priority**: P0

### Summary

SPM 模块通过 `MODULE_CONSTRUCTOR` 宏在模块加载时自动注册 Hook。核心接口包括 `OnMessageRebuildFromSPM`（消息重建）、`OnSpawnAbortUpdateRefCount`（abort 时引用计数回收）、`OnAppExitUpdateRefCount`（退出时引用计数回收）。

### Key Code

#### SPM 接口定义
```c
// modules/spm/spm.h:74
uint32_t GetSpawnId(void);
int CleanupStaleSpawns(void);
int OnMessageRebuildFromSPM(AppSpawnMgr *mgr, AppSpawningCtx *ctx);
int OnSpawnAbortUpdateRefCount(AppSpawnMgr *mgr, AppSpawningCtx *ctx);
int OnAppExitUpdateRefCount(const AppSpawnMgr *mgr, const AppSpawnedProcess *appInfo);
```

#### 引用计数标志位
```c
// modules/spm/spm.h:37
#define SPM_REF_NONE      0x00
#define SPM_REF_TOKENID   0x01  // bit 0: tokenid refcount incremented
#define SPM_REF_UID       0x02  // bit 1: uid refcount incremented
```

`AppSpawningCtx.spmRefAdded` 和 `AppSpawnedProcess.spmRefAdded` 字段使用位图记录哪些引用计数已被增加，确保在 spawn abort 或进程退出时能正确回收。

---

## KP-2: 权限消息重建

**Priority**: P0

### Summary

`OnMessageRebuildFromSPM` 是 SPM 的核心函数。它在消息解码后从内核 SPM 子系统获取最新的权限位图数据，并将其重建为 TLV 格式注入到孵化消息中。

### Flow

1. Hook 在 `STAGE_PARENT_MSG_DECODE` 阶段触发
2. 从消息中提取 accessTokenId、UID 等信息
3. 调用内核接口获取 SPM 权限数据
4. 使用 `tlv_builder.c` 构建新的 TLV 数据
5. 增加引用计数（tokenid, uid）
6. 更新 `spmRefAdded` 位图

---

## KP-3: 引用计数管理

**Priority**: P1

### Summary

SPM 模块维护两个维度的引用计数：tokenid 引用计数和 uid 引用计数。在孵化成功时增加，在孵化失败或进程退出时减少。

### Key Code

#### Spawn Abort 回收
```c
// modules/spm/spm.h:79
int OnSpawnAbortUpdateRefCount(AppSpawnMgr *mgr, AppSpawningCtx *ctx);
```
当孵化被中止时，根据 `spmRefAdded` 位图回收已增加的引用计数。

#### 进程退出回收
```c
// modules/spm/spm.h:82
int OnAppExitUpdateRefCount(const AppSpawnMgr *mgr, const AppSpawnedProcess *appInfo);
```
应用进程退出时，通过 `STAGE_SERVER_APP_CLEANUP` Hook 触发引用计数回收。

---

## KP-4: TLV 构建器

**Priority**: P1

### Summary

`tlv_builder.c` 提供将 SPM 权限数据构建为 TLV 格式的工具函数。

### Key Data Structures

| 结构体 | 文件 | 用途 |
|--------|------|------|
| SpmErrorCode | `spm.h:43` | SPM 错误码枚举 |
| AplLevel | `spm.h:67` | APL 等级（NORMAL/SYSTEM_BASIC/SYSTEM_CORE） |

---

## KP-5: APL 等级与权限映射

**Priority**: P1

### Summary

APL（Ability Privilege Level）定义应用的特权等级，不同等级拥有不同的系统权限。

```c
// modules/spm/spm.h:67
typedef enum {
    APL_INVALID = 0,
    APL_NORMAL = 1,
    APL_SYSTEM_BASIC = 2,
    APL_SYSTEM_CORE = 3,
} AplLevel;
```

### Error Codes

```c
// modules/spm/spm.h:43
typedef enum {
    SPM_SUCCESS = 0,
    SPM_ERROR_INVALID_PARAM = -1,
    SPM_ERROR_NO_MEMORY = -2,
    SPM_ERROR_TOKENID_ATTR_MISMATCH = -3,   // 安全错误，必须中止
    SPM_ERROR_INVALID_DATA = -4,            // 安全错误，必须中止
    SPM_ERROR_REF_COUNT_LIMIT = -5,
    SPM_ERROR_REF_COUNT_INC_FAILED = -6,
    // ...
} SpmErrorCode;
```

---

## Key Data Structures

| Structure | File | Purpose |
|-----------|------|---------|
| AplLevel | `spm.h:67` | APL 等级枚举 |
| SpmErrorCode | `spm.h:43` | SPM 错误码 |

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| 消息重建 Hook | modulemgr_engine | STAGE_PARENT_MSG_DECODE | Incoming |
| Abort 回收 Hook | modulemgr_engine | STAGE_SERVER_SPAWN_ABORT | Incoming |
| 进程退出 Hook | modulemgr_engine | STAGE_SERVER_APP_CLEANUP | Incoming |
| 读写 spmRefAdded | standard | AppSpawningCtx/AppSpawnedProcess | Bidirectional |
| 内核 SPM 交互 | 系统 | 系统调用 | Outgoing |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| modulemgr_engine | depends_on | [module_modulemgr_engine.md](module_modulemgr_engine.md) |
| standard | used_by | [module_standard.md](module_standard.md) |
| util | depends_on | [module_util.md](module_util.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
