# Module: client_api

> 返回: [索引](../index.md)


## Overview

client_api 模块提供 appspawn 的客户端 API，供 AMS（Ability Manager Service）等外部服务调用。客户端通过这些 API 构建孵化请求消息，经本地 socket 发送给 appspawn 服务端，并接收孵化结果。

客户端 API 采用句柄模型：`AppSpawnClientHandle` 代表客户端连接（线程安全），`AppSpawnReqMsgHandle` 代表单条请求消息（非线程安全）。

## Source Location
- Directory: `interfaces/innerkits/`
- Files: client/appspawn_client.c, client/appspawn_msg.c, include/appspawn.h, client/appspawn_client.h
- Estimated LOC: ~1,803

## Dependencies
- Depends on: 无（独立客户端库）
- Used by: AMS, 外部系统服务

---

## KP-1: 客户端 SDK 架构

**Priority**: P0

### Summary

公共 API 定义在 `appspawn.h` 中，使用不透明句柄（`void *` 类型）隐藏内部实现。核心设计是两个句柄：`AppSpawnReqMsgHandle`（消息构建）和 `AppSpawnClientHandle`（连接管理）。

### Key Code

#### 句柄类型定义
```c
// interfaces/innerkits/include/appspawn.h:39
typedef void *AppSpawnReqMsgHandle;  // 请求消息句柄，不支持多线程
typedef void *AppSpawnClientHandle;  // 客户端句柄，支持多线程

#define INVALID_PERMISSION_INDEX (-1)
#define INVALID_REQ_HANDLE NULL
#define APPSPAWN_SERVER_NAME "appspawn"
#define NWEBSPAWN_SERVER_NAME "nwebspawn"
#define NATIVESPAWN_SERVER_NAME "nativespawn"
```

#### 核心数据结构
```c
// interfaces/innerkits/client/appspawn_client.h:84
typedef struct TagAppSpawnReqMsgNode {
    struct ListNode node;
    uint32_t reqId;
    uint32_t retryCount;
    int fdCount;
    int fds[APP_MAX_FD_COUNT];
    int isColdRun;
    AppSpawnMsgFlags *msgFlags;
    AppSpawnMsgFlags *permissionFlags;
    AppSpawnMsg *msg;
    struct ListNode msgBlocks;  // 保存实际的消息数据
} AppSpawnReqMsgNode;

// interfaces/innerkits/client/appspawn_client.h:74
typedef struct TagAppSpawnReqMsgMgr {
    AppSpawnClientType type;
    uint32_t maxRetryCount;
    uint32_t timeout;
    uint32_t msgNextId;
    int socketId;
    pthread_mutex_t mutex;       // 线程安全锁
    AppSpawnMsgBlock recvBlock;  // 消息接收缓存
} AppSpawnReqMsgMgr;
```

---

## KP-2: 消息构造与发送

**Priority**: P0

### Summary

消息构造通过 `AppSpawnReqMsgCreate()` 创建消息句柄，然后通过一系列 `AppSpawnClientAdd*` / `AppSpawnClientSet*` API 填充 TLV 字段。最后通过 `AppSpawnClientSendMsg()` 发送。

### Key Code

#### DAC 信息结构体
```c
// interfaces/innerkits/include/appspawn.h:64
typedef struct {
    uint32_t uid;       // 子进程 setuid() 后的 UID
    uint32_t gid;       // 子进程 setgid() 后的 GID
    uint32_t gidCount;  // gidTable 大小
    uint32_t gidTable[APP_MAX_GIDS];  // 64 个附加 GID
    char userName[APP_USER_NAME];
} AppDacInfo;
```

#### 响应结构体
```c
// interfaces/innerkits/include/appspawn.h:72
typedef struct {
    int result;
    pid_t pid;
    uint64_t checkPointId;  // checkpoint ID（镜像进程响应有效）
} AppSpawnResult;
```

#### 消息块管理
```c
// interfaces/innerkits/client/appspawn_client.h:67
typedef struct {
    struct ListNode node;
    uint32_t blockSize;     // block 大小
    uint32_t currentIndex;  // 当前填充位置
    uint8_t buffer[0];      // 柔性数组
} AppSpawnMsgBlock;
```
消息数据以 block 链表形式存储，支持动态扩展。

---

## KP-3: DAC/权限信息设置 API

**Priority**: P0

### Summary

API 层提供丰富的信息设置接口，覆盖孵化请求的所有字段。

### 主要 API

| API | 文件 | 用途 |
|-----|------|------|
| AppSpawnClientInit | `appspawn.h` | 初始化客户端句柄 |
| AppSpawnReqMsgCreate | `appspawn.h` | 创建请求消息句柄 |
| AppSpawnClientAddBundleInfo | `appspawn.h` | 设置 bundle name 和 index |
| AppSpawnClientSetAppDacInfo | `appspawn.h` | 设置 UID/GID/GID 表 |
| AppSpawnClientSetAppDomainInfo | `appspawn.h` | 设置 APL 和 hapFlags |
| AppSpawnClientSetAppAccessToken | `appspawn.h` | 设置 AccessToken |
| AppSpawnClientAddPermission | `appspawn.h` | 添加权限位 |
| AppSpawnClientSetAppInternetPermission | `appspawn.h` | 设置网络权限 |
| AppSpawnClientSetAppOwnerId | `appspawn.h` | 设置 OwnerId |
| AppSpawnClientSendMsg | `appspawn.h` | 发送请求并等待结果 |

---

## KP-4: 客户端连接管理

**Priority**: P1

### Summary

客户端连接支持多线程安全，使用 `pthread_mutex_t` 保护。支持超时和重试机制。

### Key Code

#### 超时与重试
```c
// interfaces/innerkits/client/appspawn_client.h:29
#define APPSPAWN_CLIENT_TIMEOUT_MIN 2       // 最小超时 2 秒
#define APPSPAWN_CLIENT_TIMEOUT_MAX 120     // 最大超时 120 秒
#define TIMEOUT_DEF 2                        // 默认超时
#define RETRY_TIME (200 * 1000)             // 重试间隔 200ms
#define MAX_RETRY_SEND_COUNT 2              // 最大重试次数
```

#### 客户端类型
```c
// interfaces/innerkits/client/appspawn_client.h:57
typedef enum {
    CLIENT_FOR_APPSPAWN,
    CLIENT_FOR_NWEBSPAWN,
    CLIENT_FOR_CJAPPSPAWN,
    CLIENT_FOR_NATIVESPAWN,
    CLIENT_FOR_HYBRIDSPAWN,
    CLIENT_FOR_APPSPAWNDF,
    CLIENT_MAX
} AppSpawnClientType;
```

---

## Key Data Structures

| Structure | File | Purpose |
|-----------|------|---------|
| AppDacInfo | `appspawn.h:64` | DAC 信息（UID/GID/GID 表） |
| AppSpawnResult | `appspawn.h:72` | 孵化响应 |
| AppSpawnReqMsgMgr | `appspawn_client.h:74` | 请求管理器（含互斥锁） |
| AppSpawnReqMsgNode | `appspawn_client.h:84` | 单条请求消息 |
| AppSpawnMsgBlock | `appspawn_client.h:67` | 消息数据块 |
| AppSpawnClientType | `appspawn_client.h:57` | 客户端类型枚举 |

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| Socket 通信 | standard | 本地 socket | Outgoing |
| 消息格式 | modulemgr_engine | TLV 格式共享 | Shared |
| 错误码 | util | APPSPAWN错误码 | Shared |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| standard | used_by | [module_standard.md](module_standard.md) |
| modulemgr_engine | shares_with | [module_modulemgr_engine.md](module_modulemgr_engine.md) |
| util | depends_on | [module_util.md](module_util.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
