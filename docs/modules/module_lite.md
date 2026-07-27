# Module: lite

> 返回: [索引](../index.md)


## Overview

lite 模块是 appspawn 在小型系统（轻量级 IoT 设备）上的实现。相比标准系统的 appspawn，lite 版本功能简化，通过 IPC 框架接收 JSON 格式的请求消息。

## Source Location
- Directory: `lite/`
- Files: main.c, appspawn_service.c, appspawn_message.c, appspawn_process.c, appspawn_service.h, appspawn_message.h
- Estimated LOC: ~1,500

## Dependencies
- Depends on: interfaces/innerkits
- Used by: 无（独立于标准系统）

---

## KP-1: 小型系统 appspawn 实现

**Priority**: P2

### Summary

小型系统 appspawn 的消息格式为 JSON（而非标准系统的 TLV），注册的 IPC 服务名称为 "appspawn"。

### Key Code

#### 消息格式
```json
{"bundleName":"testvalid1","identityID":"1234","uID":1000,"gID":1000,"capability":[0]}
```

小型系统消息字段：
| 字段 | 说明 |
|------|------|
| bundleName | 应用包名（7-127字节） |
| identityID | AMS 生成的进程标识符（1-24字节） |
| uID | 进程 UID |
| gID | 进程 GID |
| capability | capability 权限列表（最多10个） |

### 组件
- `main.c`: 主入口
- `appspawn_service.c`: 服务实现（IPC 注册、消息处理）
- `appspawn_message.c`: 消息解析
- `appspawn_process.c`: 进程孵化

---

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| IPC 通信 | 外部（AMS） | RPC/binder | Incoming |
| 消息定义 | client_api | appspawn.h | Shared |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| client_api | depends_on | [module_client_api.md](module_client_api.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
