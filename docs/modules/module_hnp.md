# Module: hnp

> 返回: [索引](../index.md)


## Overview

HNP（Harmony Native Package）服务是 appspawn 仓库中的独立服务，负责原生包的安装、打包和管理。它与 appspawn 孵化服务独立运行，但共享部分基础设施。

## Source Location
- Directory: `service/hnp/`
- Files: hnp_main.c, hnpcli_main.c, installer/hnp_installer.c, pack/hnp_pack.c, base/*.c
- Estimated LOC: ~3,000+

## Dependencies
- Depends on: interfaces/innerkits/hnp
- Used by: 无（独立服务）

---

## KP-1: HNP 服务架构

**Priority**: P2

### Summary

HNP 服务包含服务端（hnp_main）、命令行客户端（hnpcli_main）、安装器和打包器。

### Key Code

#### HNP API 头文件
```c
// interfaces/innerkits/hnp/include/hnp_api.h
// 提供原生包安装、卸载、查询等 API
```

#### 服务组件
- `hnp_main.c`: HNP 服务主入口
- `hnpcli_main.c`: 命令行工具入口
- `installer/hnp_installer.c`: 原生包安装逻辑
- `pack/hnp_pack.c`: 原生包打包逻辑
- `base/hnp_file.c`: 文件操作工具
- `base/hnp_json.c`: JSON 处理工具
- `base/hnp_zip.c`: 压缩解压工具
- `base/hnp_sal.c`: 系统抽象层
- `base/hnp_log.c`: 日志工具

---

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| 独立运行 | 无 | 独立进程 | None |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
