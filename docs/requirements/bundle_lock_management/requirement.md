# 应用沙盒资源锁定管理需求文档

## 1 概述

### 1.1 背景

在 OpenHarmony 系统中，当设备处于锁定状态时，应用启动后创建的沙盒资源可能会在设备解锁过程中被系统清理或修改，导致：

1. **数据一致性问题**：应用数据在设备解锁过程中可能被意外删除
2. **资源竞争问题**：多个进程同时访问同一沙盒目录可能导致文件系统不一致
3. **用户体验问题**：应用在锁定/解锁状态切换时可能出现崩溃或数据丢失

### 1.2 目标

本需求旨在实现设备锁定状态下的应用沙盒资源锁定管理机制：

- 为锁定状态下启动的应用创建独立的 `_locked` 资源目录
- 实现引用计数机制，支持多实例应用的精细化管理
- 确保应用退出或启动失败时资源能够正确清理
- 提供设备锁定状态的动态检测能力

### 1.3 适用范围

| 项目 | 说明 |
|------|------|
| 适用系统 | 标准系统 (Standard System) |
| 影响组件 | appspawn、sandbox 模块 |
| 适用版本 | OpenHarmony 3.2+ |

---

## 2 需求分析

### 2.1 现状分析

#### 2.1.1 当前架构

```
┌─────────────────────────────────────────────────────────────┐
│                      AppSpawn 服务                           │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  应用启动请求                                                 │
│       │                                                       │
│       ▼                                                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐      │
│  │ 权限检查    │───►│ 进程创建    │───►│ 沙盒挂载    │      │
│  └─────────────┘    └─────────────┘    └─────────────┘      │
│                                                 │             │
│                                                 ▼             │
│                                          /mnt/sandbox/       │
│                                          <uid>/<bundle>/     │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

#### 2.1.2 存在的问题

| 问题 | 影响 |
|------|------|
| 设备锁定状态下沙盒资源可能被清理 | 应用数据丢失 |
| 缺乏应用退出后的资源清理机制 | 资源泄漏 |
| 启动失败时已创建的资源未释放 | 孤儿目录残留 |
| 多实例应用（克隆/隔离）无独立管理 | 资源冲突 |

### 2.2 目标架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     AppSpawn 服务 (增强版)                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  应用启动请求                                                    │
│       │                                                          │
│       ▼                                                          │
│  ┌─────────────┐    ┌─────────────────────────────────┐         │
│  │ 权限检查    │───►│  设备锁定状态检测               │         │
│  └─────────────┘    │  (IsUnlockStatus)               │         │
│                     └─────────────────────────────────┘         │
│                               │                                 │
│                 ┌─────────────┴─────────────┐                   │
│                 │                           │                    │
│           已解锁 ▼                       已锁定 ▼                 │
│     ┌───────────────┐           ┌──────────────────┐           │
│     │ 正常挂载流程   │           │ 创建 _locked 目录 │           │
│     │ 无额外处理     │           │ + 引用计数管理    │           │
│     └───────────────┘           └──────────────────┘           │
│                                                  │              │
│                                          ┌───────┴───────┐      │
│                                          │               │      │
│                                  启动成功 ▼         失败 ▼      │
│                              ┌──────────┐    ┌──────────┐     │
│                              │  运行中  │    │  清理    │     │
│                              │  引用+1  │    │  目录    │     │
│                              └──────────┘    └──────────┘     │
│                                  │                               │
│                                  ▼                               │
│                              ┌──────────┐                       │
│                              │ 应用退出  │                       │
│                              │ 引用-1   │                       │
│                              └──────────┘                       │
│                                  │                               │
│                                  ▼                               │
│                              ┌──────────┐                       │
│                              │计数为0?  │                       │
│                              └──────────┘                       │
│                                  │                               │
│                        ┌─────────┴─────────┐                    │
│                    是 ▼                   否 ▼                    │
│              ┌──────────────┐      ┌──────────────┐            │
│              │ 删除目录      │      │  保持目录    │            │
│              │ 释放资源      │      │  (应用实例数量 > 0)  │            │
│              └──────────────┘      └──────────────┘            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                     全局引用计数管理表                            │
├─────────────────────────────────────────────────────────────────┤
│  Key: <uid>:<bundle>_locked  │  Value: LockBundleInfo           │
│  ├─ refCount: uint32_t       │  ├─ refCount (引用计数)          │
│  └─ lockPath: string         │  └─ lockPath (_locked目录路径)   │
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 方案选择说明

| 方案 | 优点 | 缺点 | 结论 |
|------|------|------|------|
| 方案A：引用计数+自动清理 | 资源管理精确，支持多实例 | 需要维护全局状态 | **采用** |
| 方案B：延迟清理 | 实现简单 | 资源释放不及时，可能泄漏 | 不采用 |
| 方案C：解锁时统一清理 | 管理集中 | 锁定期间可能积累大量目录 | 不采用 |

> **选择理由**：引用计数方案能够在应用退出时精确管理资源，避免资源泄漏，同时支持多实例应用的独立管理。

---

## 3 功能需求

### 3.1 设备锁定状态检测

#### 3.1.1 状态读取

- 通过系统参数 `startup.appspawn.lockstatus_<uid>` 读取设备锁定状态
- 参数值为 `"0"` 表示设备已解锁，`"1"` 表示设备已锁定
- 支持按用户ID独立判断不同用户的设备状态

#### 3.1.2 状态判断逻辑

```cpp
static bool IsUnlockStatus(uint32_t uid) {
    char key[PARAM_NAME_LEN_MAX] = {0};
    snprintf(key, sizeof(key), "startup.appspawn.lockstatus_%u", uid);

    char userLockStatus[PARAM_VALUE_LEN_MAX] = {0};
    int ret = GetParameter(key, LOCK_STATUS_LOCK, userLockStatus, PARAM_VALUE_LEN_MAX);

    return (strcmp(userLockStatus, LOCK_STATUS_UNLOCK) == 0);
}
```

### 3.2 Bundle 锁定目录管理

#### 3.2.1 目录路径构建

根据应用类型和用户信息构建唯一的 `_locked` 目录路径：

| 应用类型 | 路径格式 | 示例 |
|----------|----------|------|
| 普通应用 | `/mnt/sandbox/<uid>/<bundle>_locked` | `/mnt/sandbox/100/com.example.app_locked` |
| 克隆应用 | `/mnt/sandbox/<uid>/<bundle>+clone-<index>+<original>_locked` | `/mnt/sandbox/100/com.example.app+clone-0+com.example.app_locked` |
| 隔离应用 | `/mnt/sandbox/<uid>/isolated/<bundle>_locked` | `/mnt/sandbox/100/isolated/com.example.isolated_locked` |

#### 3.2.2 引用计数管理

**AddLockBundleRef**：增加目录引用计数
- 第一次引用时创建目录
- 后续引用仅增加计数

**ReleaseLockBundleRef**：减少目录引用计数
- 引用计数减1
- 当计数为0时删除目录

#### 3.2.3 全局管理表

```cpp
struct LockBundleInfo {
    uint32_t refCount;        // 引用计数
    std::string lockPath;     // _locked 目录完整路径
};

// 全局 map：key 为 bundle 路径标识，value 为 LockBundleInfo
std::map<std::string, LockBundleInfo> g_lockBundleMap;
```

### 3.3 应用生命周期钩子

#### 3.3.1 STAGE_SERVER_APP_CLEANUP

- **触发时机**：应用进程正常退出或被杀死后
- **功能**：释放 `_locked` 目录引用计数
- **触发次数**：每个应用实例退出时触发一次

#### 3.3.2 STAGE_SERVER_SPAWN_ABORT

- **触发时机**：应用孵化失败时，在 `DeleteAppSpawningCtx` 之前调用
- **功能**：清理已创建的 `_locked` 目录（调用 `ReleaseLockBundleRef`）
- **目的**：防止资源泄漏

**触发场景**（在 `appspawn_service.c` 中）：

| 位置 | 函数 | 触发条件 |
|------|------|----------|
| 行 1024 | CreateProcess | `RunAppSpawnProcessMsg` 失败 |
| 行 1031 | CreateProcess | `AddChildWatcher` 失败 |
| 行 1059 | WaitChildDied | 子进程崩溃 |
| 行 1087 | WaitChildTimeout | 子进程启动超时 |
| 行 1103 | ProcessChildFdCheck | 子进程返回非零失败码 |
| 行 1160 | HandleSpawnProcessInfoFail | 进程信息初始化失败 |
| 行 219 | AppSpawningCtxOnClose | 客户端连接断开（启动中） |

**调用位置示例**：
```c
// 在 DeleteAppSpawningCtx 之前调用
AppSpawnHookExecute(STAGE_SERVER_SPAWN_ABORT, 0, GetAppSpawnContent(), &property->client);
SendResponse(...);
DeleteAppSpawningCtx(property);
```

### 3.4 消息标志扩展

#### 3.4.1 新增标志位

| 标志位索引 | 宏定义 | 说明 |
|-----------|--------|------|
| 34 | APP_FLAGS_UNLOCKED_STATUS | 设备已解锁状态 |
| 191 | APP_FLAGS_CLONE_ENABLE | 克隆应用标志 |
| 198 | APP_FLAGS_ISOLATED_SANDBOX_TYPE | 隔离沙盒类型 |

#### 3.4.2 AppSpawnedProcess 结构扩展

```c
typedef struct TagAppSpawnedProcess {
    // ... 现有字段 ...
    AppSpawnMsgFlags *msgFlags;  // 保存完整的消息标志
    char name[0];
} AppSpawnedProcess;
```

---

## 4 接口设计

### 4.1 头文件修改

#### 4.1.1 appspawn_hook.h

```c
typedef enum TagAppSpawnHookStage {
    // ... 现有阶段 ...
    STAGE_SERVER_APP_ADD,
    STAGE_SERVER_APP_CLEANUP,  // 应用进程退出后的资源清理，只触发一次
    STAGE_SERVER_APP_DIED,

    // ...
    STAGE_PARENT_UNINSTALL,
    STAGE_SERVER_SPAWN_ABORT,  // spawn abort, release resources

    // ...
} AppSpawnHookStage;
```

#### 4.1.2 appspawn_manager.h

```c
typedef struct TagAppSpawnedProcess {
#ifdef APPSPAWN_TEST
    uint32_t pid;
    uint32_t peerUid;
#endif
    bool isDebuggable;
    uint32_t appIndex;
    AppSpawnMsgFlags *msgFlags;  // 保存完整的消息标志（如 APP_FLAGS_ISOLATED_SANDBOX_TYPE 等）
    char name[0];
} AppSpawnedProcess;

// 新增函数
int CheckAppSpawnMsgFlagsSet(const AppSpawnMsgFlags *msgFlags, uint32_t flagIndex);
```

### 4.2 新增函数接口

| 函数名 | 说明 | 所在文件 |
|--------|------|----------|
| IsUnlockStatus | 检查设备解锁状态 | sandbox_shared_mount.cpp |
| BuildLockPath | 构建 _locked 目录路径 | sandbox_shared_mount.cpp |
| AddLockBundleRef | 增加目录引用计数 | sandbox_shared_mount.cpp |
| ReleaseLockBundleRef | 减少目录引用计数 | sandbox_shared_mount.cpp |
| CheckAppSpawnMsgFlagsSet | 检查消息标志位 | appspawn_msgmgr.c |

### 4.3 函数原型

```cpp
/**
 * @brief 检查设备是否处于解锁状态
 * @param uid 用户ID
 * @return true 已解锁, false 已锁定
 */
static bool IsUnlockStatus(uint32_t uid);

/**
 * @brief 构建 _locked 目录路径（版本1：使用 varBundleName）
 * @param uid 用户ID
 * @param varBundleName 变量化的Bundle名称（可能包含 clone 前缀）
 * @param msgFlags 消息标志指针
 * @return _locked 目录完整路径
 */
static std::string BuildLockPath(uint32_t uid, const std::string &varBundleName,
                                  const AppSpawnMsgFlags *msgFlags);

/**
 * @brief 构建 _locked 目录路径（版本2：使用 bundleName + appIndex）
 * @param uid 用户ID
 * @param bundleName 原始Bundle名称
 * @param appIndex 应用索引（克隆应用使用）
 * @param msgFlags 消息标志指针
 * @return _locked 目录完整路径
 */
static std::string BuildLockPath(uint32_t uid, const std::string &bundleName,
                                  uint32_t appIndex, const AppSpawnMsgFlags *msgFlags);

/**
 * @brief 增加 _locked 目录引用计数
 * @param lockPath _locked 目录路径
 */
static void AddLockBundleRef(const std::string &lockPath);

/**
 * @brief 减少 _locked 目录引用计数，计数为0时删除目录
 * @param lockPath _locked 目录路径
 */
static void ReleaseLockBundleRef(const std::string &lockPath);
```

---

## 5 数据结构

### 5.1 LockBundleInfo 结构

```cpp
/**
 * @brief Bundle 锁定目录信息
 */
struct LockBundleInfo {
    uint32_t refCount;        /**< 引用计数 */
    std::string lockPath;     /**< _locked 目录完整路径 */
};
```

### 5.2 全局管理表

```cpp
/**
 * @brief 全局 Bundle 锁定管理表
 * @key bundle 唯一标识（含 uid、bundle名、类型信息）
 * @value LockBundleInfo 锁定目录信息
 */
std::map<std::string, LockBundleInfo> g_lockBundleMap;
```

### 5.3 系统参数

| 参数名 | 格式 | 值说明 |
|--------|------|--------|
| startup.appspawn.lockstatus_\<uid\> | 字符串 | "0"=已解锁, "1"=已锁定 |

---

## 6 流程图

### 6.1 应用启动流程（锁定状态）

```
┌──────────────┐
│  应用启动请求  │
└──────┬───────┘
       ▼
┌──────────────┐
│ 检查设备状态  │
└──────┬───────┘
       │
       ├──────────────┐
       │              │
    已解锁 ▼        已锁定 ▼
┌──────────────┐  ┌──────────────────┐
│设置解锁标志   │  │ 构建 _locked 路径 │
│跳过锁定流程   │  └─────────┬────────┘
└──────────────┘            ▼
                    ┌──────────────────┐
                    │ 创建 _locked 目录 │
                    └─────────┬────────┘
                              ▼
                    ┌──────────────────┐
                    │ 增加引用计数(+1)  │
                    └─────────┬────────┘
                              ▼
                    ┌──────────────────┐
                    │   挂载沙盒       │
                    └─────────┬────────┘
                              ▼
                    ┌──────────────────┐
                    │   启动应用进程   │
                    └─────────┬────────┘
                              │
                    ┌─────────┴─────────┐
                成功 ▼               失败 ▼
          ┌──────────────┐      ┌──────────────┐
          │  应用运行中  │      │ 清理 _locked │
          └──────┬───────┘      └──────────────┘
                 ▼
          ┌──────────────┐
          │  应用退出     │
          └──────┬───────┘
                 ▼
          ┌──────────────┐
          │ 减少引用(-1)  │
          └──────┬───────┘
                 ▼
          ┌──────────────┐
          │ 计数 == 0?   │
          └──────┬───────┘
                 │
         ┌───────┴───────┐
      是 ▼           否 ▼
  ┌──────────┐    ┌──────────┐
  │删除目录  │    │ 保持目录  │
  └──────────┘    └──────────┘
```

### 6.2 引用计数管理流程

```
┌─────────────────────────────────────────────────────────────┐
│                    AddLockBundleRef(lockPath)                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  检查 lockPath 是否存在于 g_lockBundleMap                    │
│       │                                                     │
│   ┌───┴────┐                                                │
│   │        │                                                │
│ 不存在 ▼   存在 ▼                                            │
│ ┌────────┐ ┌────────────┐                                   │
│ │ 创建   │ │ refCount++ │                                   │
│ │ 目录   │ │            │                                   │
│ └───┬────┘ └────────────┘                                   │
│     │                                                        │
│     ├────────────┐                                          │
│     ▼            ▼                                          │
│ refCount=1   更新 refCount                                   │
│     │            │                                          │
│     └─────┬──────┘                                          │
│           ▼                                                 │
│    写入 g_lockBundleMap                                      │
│           │                                                 │
│           ▼                                                 │
│      返回成功                                                │
│                                                             │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                 ReleaseLockBundleRef(lockPath)               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  从 g_lockBundleMap 获取 lockPath 对应的 LockBundleInfo       │
│           │                                                 │
│           ▼                                                 │
│  refCount--                                                 │
│           │                                                 │
│           ▼                                                 │
│  refCount == 0?                                             │
│       │                                                     │
│   ┌───┴────┐                                                │
│   │        │                                                │
│  是 ▼      否 ▼                                              │
│ ┌────────┐ ┌────────────┐                                   │
│ │删除目录│ │更新 refCount│                                   │
│ │移除Map │ │            │                                   │
│ └────────┘ └────────────┘                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 7 错误处理

### 7.1 错误码定义

| 错误码 | 宏定义 | 说明 |
|--------|--------|------|
| -1 | APPSPAWN_SYS_ERROR | 系统调用失败 |
| -2 | APPSPAWN_PARAM_ERROR | 参数错误 |
| -3 | APPSPAWN_DIR_ERROR | 目录操作失败 |

### 7.2 错误处理策略

| 错误场景 | 处理方式 | 日志级别 |
|----------|----------|----------|
| 系统参数读取失败 | 默认为锁定状态 | WARNING |
| 创建 _locked 目录失败 | 记录日志，继续启动流程 | ERROR |
| 引用计数下溢（过度释放） | 记录错误日志，不执行删除 | ERROR |
| 删除目录失败 | 记录错误日志 | ERROR |
| 空路径传入 | 记录错误日志，直接返回 | ERROR |

---

## 8 性能要求

| 指标 | 要求 | 说明 |
|------|------|------|
| 目录创建时间 | < 50 ms | 单个 _locked 目录创建 |
| 引用计数操作 | < 1 ms | map 查找和更新 |
| 内存占用 | < 10 KB | 全局 map 和对象 |
| 状态检测时间 | < 10 ms | 系统参数读取 |

---

## 9 测试用例

### 9.1 功能测试

| 编号 | 用例名称 | 前置条件 | 测试步骤 | 预期结果 |
|------|----------|----------|----------|----------|
| FT001 | 锁定状态创建目录 | 设备已锁定 | 1. 设置锁定状态<br>2. 启动应用 | _locked 目录被创建，引用计数=1 |
| FT002 | 解锁状态不创建目录 | 设备已解锁 | 1. 设置解锁状态<br>2. 启动应用 | 不创建 _locked 目录 |
| FT003 | 引用计数递增 | 已存在 _locked 目录 | 1. 启动应用A<br>2. 启动应用B（同bundle） | 引用计数=2，目录仍存在 |
| FT004 | 引用计数递减删除 | 引用计数=1 | 1. 应用A退出 | 引用计数=0，目录被删除 |
| FT005 | 克隆应用路径 | - | 1. 启动克隆应用 | 路径包含 +clone-<index>+ 前缀 |
| FT006 | 隔离应用路径 | - | 1. 启动隔离应用 | 路径为 <bundle>_locked |
| FT007 | 启动失败清理 | - | 1. 模拟启动失败 | _locked 目录被清理 |

### 9.2 异常测试

| 编号 | 用例名称 | 前置条件 | 测试步骤 | 预期结果 |
|------|----------|----------|----------|----------|
| ET001 | 空路径处理 | - | 1. 传入空路径 | 返回错误，不执行操作 |
| ET002 | 过度释放保护 | 引用计数=0 | 1. 连续调用 Release | 记录错误，不执行删除 |
| ET003 | 并发创建 | - | 1. 多进程同时创建 | 仅创建一次，计数正确 |
| ET004 | 参数读取失败 | - | 1. 模拟参数服务异常 | 默认为锁定状态 |

---

## 10 实现计划

### 10.1 实现阶段

| 阶段 | 内容 | 优先级 | 依赖 |
|------|------|--------|------|
| P0 | 引用计数管理核心逻辑 | 高 | 无 |
| P0 | Hook 阶段扩展 | 高 | 无 |
| P0 | 基础路径构建 | 高 | 无 |
| P1 | 克隆/隔离应用支持 | 中 | P0 |
| P1 | 测试用例编写 | 中 | P0 |

### 10.2 文件修改清单

| 文件路径 | 修改类型 | 说明 |
|----------|----------|------|
| modules/module_engine/include/appspawn_hook.h | 修改 | 新增 Hook 阶段枚举 |
| modules/module_engine/stub/libappspawn.stub.json | 修改 | 更新接口定义 |
| modules/sandbox/normal/sandbox_shared_mount.cpp | 修改 | 实现 Bundle Lock 逻辑 |
| modules/sysevent/hisysevent_adapter.h | 修改 | 新增事件定义 |
| standard/appspawn_manager.h | 修改 | 新增 msgFlags 字段和函数 |
| standard/appspawn_msgmgr.c | 修改 | 新增标志检查函数 |
| standard/appspawn_appmgr.c | 修改 | 应用管理适配 |
| standard/appspawn_service.c | 修改 | 服务流程适配 |
| test/unittest/.../appspawn_exit_lock_stub.cpp | 修改 | 测试桩代码 |
| test/unittest/.../app_spawn_lock_bundle_test.cpp | 新增 | Bundle Lock 单元测试 |
| test/unittest/.../sandbox_lock_bundle_test.h | 新增 | 测试头文件 |
| test/unittest/.../BUILD.gn | 修改 | 添加测试编译 |

---

## 11 双重释放保护机制

### 11.1 问题背景

在应用生命周期管理中，`_locked` 目录的引用计数可能因为异常代码路径而被多次释放：
- `STAGE_SERVER_SPAWN_ABORT`：应用启动中止时触发
- `STAGE_SERVER_APP_CLEANUP`：应用退出时触发

虽然这两个 Hook 针对不同的场景，但为了防御性编程，需要防止意外的重复释放。

### 11.2 双重保护机制

本方案采用 **双重防护** 策略：

#### 11.2.1 防护层级

| 防护层级 | 实现位置 | 触发时机 | 作用 |
|----------|----------|----------|------|
| **第一层：Flag 检查** | Hook 函数入口 | 调用 `ReleaseLockBundleRef` 之前 | 主动防护，防止不必要的调用 |
| **第二层：refCount 下溢保护** | `ReleaseLockBundleRef` 内部 | refCount 递减之前 | 被动防护，防止计数异常 |

#### 11.2.2 Flag 字段设计

**AppSpawningCtx 结构**（用于 `STAGE_SERVER_SPAWN_ABORT`）：
```c
typedef struct TagAppSpawningCtx {
    // ... 现有字段 ...
    bool lockBundleRefAdded;  // 是否已调用 AddLockBundleRef
    // ...
} AppSpawningCtx;
```

**AppSpawnedProcess 结构**（用于 `STAGE_SERVER_APP_CLEANUP`）：
```c
typedef struct TagAppSpawnedProcess {
    // ... 现有字段 ...
    bool lockBundleRefAdded;  // 是否已调用 AddLockBundleRef
    char name[0];
} AppSpawnedProcess;
```

#### 11.2.3 Flag 状态流转

```
┌─────────────────────────────────────────────────────────────────┐
│                      Flag 状态流转图                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  初始状态: lockBundleRefAdded = false                           │
│       │                                                          │
│       ▼                                                          │
│  ┌──────────────────┐                                           │
│  │ AddLockBundleRef │                                           │
│  │  (设备锁定状态)   │                                           │
│  └────────┬─────────┘                                           │
│           │                                                      │
│           ▼                                                      │
│  lockBundleRefAdded = true  ←─── 标记已添加引用                   │
│           │                                                      │
│           ├──────────────┐                                      │
│           │              │                                      │
│      正常 ▼          失败 ▼                                      │
│  ┌──────────┐    ┌──────────────────┐                          │
│  │ 应用运行  │    │ ReleaseLockBundle│                          │
│  └────┬─────┘    │      Ref         │                          │
│       │          └─────────┬────────┘                          │
│       ▼                    │                                     │
│  ┌──────────┐               │                                     │
│  │ 应用退出  │               │                                     │
│  └────┬─────┘               │                                     │
│       │                     │                                     │
│       └──────────┬──────────┘                                     │
│                  ▼                                               │
│       检查 flag && ReleaseLockBundleRef                          │
│                  │                                               │
│                  ▼                                               │
│  lockBundleRefAdded = false  ←─── 清除标记                        │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 11.3 实现要点

#### 11.3.1 AddLockBundleRef 后设置 Flag

**位置**：`sandbox_shared_mount.cpp` → `MountDirToShared`

```cpp
// 在 AddLockBundleRef 之后设置 flag
AddLockBundleRef(lockSbxPathStamp);
property->lockBundleRefAdded = true;
```

#### 11.3.2 ReleaseLockBundleRef 前检查并清除 Flag

**SpawnFailedHook** (使用 `AppSpawningCtx *property`)：
```cpp
static int SpawnFailedHook(AppSpawnMgr *content, AppSpawningCtx *property)
{
    // 获取 uid 用于早期检查
    AppDacInfo *dacInfo = reinterpret_cast<AppDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    uint32_t uid = dacInfo != nullptr ? dacInfo->uid : 0;

    // 检查 flag：仅在 uid > 0、设备已锁定且已添加引用时继续
    APPSPAWN_ONLY_EXPER(IsUnlockStatus(uid) || !property->lockBundleRefAdded, return 0);

    // ... 构建 lockPath ...

    ReleaseLockBundleRef(lockPath);
    property->lockBundleRefAdded = false;  // 清除 flag
    return 0;
}
```

**AppCleanupHook** (使用 `AppSpawnedProcessInfo *appInfo`)：
```cpp
static int AppCleanupHook(const AppSpawnMgr *content,
                          const AppSpawnedProcessInfo *appInfo)
{
    // 检查 flag：仅在 uid > 0、设备已锁定且已添加引用时继续
    APPSPAWN_ONLY_EXPER(IsUnlockStatus(appInfo->uid) || !appInfo->lockBundleRefAdded, return 0);

    // ... 构建 lockPath ...

    ReleaseLockBundleRef(lockPath);
    // 清除 flag（冗余但安全，防御性编程）
    const_cast<AppSpawnedProcessInfo *>(appInfo)->lockBundleRefAdded = false;
    return 0;
}
```

### 11.4 refCount 下溢保护（第二层防护）

**位置**：`sandbox_shared_mount.cpp` → `ReleaseLockBundleRef`

```cpp
APPSPAWN_STATIC void ReleaseLockBundleRef(const std::string &lockPath)
{
    // ... 查找 lockPath ...

    // 防止 refCount 下溢
    APPSPAWN_CHECK_LOGE(it->second.refCount > 0, return,
        "ReleaseLockBundleRef: refCount underflow for %{public}s, current refCount=%{public}u",
        lockPath.c_str(), it->second.refCount);

    it->second.refCount--;
    // ...
}
```

### 11.5 防护效果对比

| 场景 | 无防护 | 仅 refCount 下溢保护 | Flag + refCount 双重防护 |
|------|--------|---------------------|------------------------|
| 正常释放 | ✅ | ✅ | ✅ |
| 重复释放（相同实例） | ❌ refCount 下溢 | ✅ 拦截下溢 | ✅ Flag 拦截，不调用 Release |
| 计数异常 | ❌ 数据不一致 | ✅ 记录错误日志 | ✅ Flag 层拦截 + refCount 保护 |
| 调试便利性 | ❌ 难以定位 | ⚠️ 需分析 refCount | ✅ Flag 状态清晰 |

---

## 12 风险评估

### 12.1 技术风险

| 风险项 | 风险等级 | 影响 | 缓解措施 |
|--------|----------|------|----------|
| 引用计数重复释放 | 中 | refCount 下溢、误删目录 | **Flag 检查 + refCount 下溢保护** |
| 全局 map 线程安全 | 中 | 数据竞争 | 使用互斥锁保护 |
| 目录残留 | 低 | 磁盘空间泄漏 | 启动时清理残留目录 |
| 参数服务延迟 | 低 | 状态检测不准确 | 使用本地缓存 |
| 引用计数泄漏 | 中 | 内存/目录泄漏 | 定期扫描清理 |

---

## 13 术语表

| 术语 | 说明 |
|------|------|
| Bundle | 应用的唯一标识，通常为包名 |
| Sandbox | 应用沙盒，应用的隔离运行环境 |
| Clone Mode | 克隆模式，同一应用的多个独立实例 |
| Isolated Mode | 隔离模式，与主应用隔离的独立沙盒 |
| RefCount | 引用计数，记录资源被引用的次数 |
| Hook | 钩子，在特定时机执行的回调函数 |
| lockBundleRefAdded | 防重复释放标志位，标记是否已调用 AddLockBundleRef |

---

## 14 参考资料

1. OpenHarmony AppSpawn 模块文档
2. OpenHarmony 沙盒子系统设计文档
3. Linux 文件系统命名空间机制

---

## 修订记录

| 版本 | 日期 | 修订人 | 修订内容 |
|------|------|--------|----------|
| v1.3 | 2025-03-25 | - | 更新第 11.3.2 节：AppCleanupHook 添加冗余 flag 清除，条件检查改用 APPSPAWN_ONLY_EXPER 宏；更新 BuildLockPath 函数签名（两个重载版本） |
| v1.2 | 2025-03-24 | - | 新增第 11 章：双重释放保护机制（Flag 检查 + refCount 下溢保护） |
| v1.1 | 2025-03-24 | - | 新增 3 处 STAGE_SERVER_SPAWN_ABORT 调用（ProcessChildFdCheck、HandleSpawnProcessInfoFail、AppSpawningCtxOnClose） |
| v1.0 | 2025-03-24 | - | 初始版本，基于 unlock 分支分析生成 |
