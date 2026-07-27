# Module: util

> 返回: [索引](../index.md)


## Overview

util 模块提供 appspawn 的通用工具函数，包括内存操作、字符串处理、路径操作、日志宏、错误码定义和 DFX 工具。

## Source Location
- Directory: `util/`
- Files: 2 源文件, 5 头文件
- Estimated LOC: ~1,312

## Dependencies
- Depends on: 无
- Used by: 几乎所有模块

---

## KP-1: 通用工具函数

**Priority**: P2

### Summary

`appspawn_utils.c` (~462行) 提供:
- `APPSPAWN_CHECK` / `APPSPAWN_CHECK_ONLY_EXPER` / `APPSPAWN_ONLY_EXPER`: 条件检查宏
- `APPSPAWN_LOGI` / `APPSPAWN_LOGE` / `APPSPAWN_LOGV` / `APPSPAWN_DUMP`: 日志宏
- `InitCommonEnv()`: 通用环境初始化
- `DiffTime()`: 时间差计算
- 路径操作函数

### Key Code

#### 日志和检查宏
```c
// util/include/appspawn_utils.h
#define APPSPAWN_CHECK(condition, action, fmt, ...) \
    do { if (!(condition)) { APPSPAWN_LOGE(fmt, ##__VA_ARGS__); action; } } while (0)

#define APPSPAWN_CHECK_ONLY_EXPER(condition, action, ...) \
    do { if (!(condition)) { action; } } while (0)
```

---

## KP-2: 错误码体系

**Priority**: P2

### Summary

`appspawn_error.h` (~98行) 定义了 appspawn 的错误码体系。

### Key Code

主要错误码:
- `APPSPAWN_SUCCESS = 0`
- `APPSPAWN_ARG_INVALID`: 参数无效
- `APPSPAWN_MSG_INVALID`: 消息无效
- `APPSPAWN_SYSTEM_ERROR`: 系统错误
- `APPSPAWN_ERROR_UTILS_MEM_FAIL`: 内存分配失败

## 工具函数详情 (G-1 补全)

**Priority**: P2

### Summary

util 模块除宏和错误码外，还提供 35+ 个工具函数，分布在 7 个文件（~1013 行）。覆盖字符串/路径/JSON/系统参数/网络/环境/DF 路由/容器转换/TLV 构建等类别。

### 文件清单

| 文件 | 行数 | 用途 |
|------|------|------|
| `util/include/appspawn_utils.h` | 296 | 时间/字符串/路径/网络工具声明 |
| `util/src/appspawn_utils.c` | 462 | 核心工具函数实现 |
| `util/include/appspawndf_utils.h` | 49 | AppSpawnDF 消息路由工具 |
| `util/include/parcel_util.h` | 44 | C++ 容器转换（list↔vector） |
| `util/include/json_utils.h` | 64 | JSON 解析辅助 |
| `util/include/appspawn_error.h` | 98 | 错误码定义框架 |

### 函数分类表

| 类别 | 代表函数 | 头文件:行 | 用途 |
|------|----------|-----------|------|
| 时间测量 | `DiffTime` | `appspawn_utils.h:155` | 微秒级时间差 |
| 字符串 | `StringSplit` / `GetLastStr` / `ConvertEnvValue` | `appspawn_utils.h:159-166` | 分割 / 反向查找 / `${VAR}` 展开 |
| 路径/目录 | `DumpCurrentDir` / `MakeDirRec` | `appspawn_utils.c:138,315` | 递归打印 / 递归创建目录 |
| 文件 I/O | `ReadFile` | `appspawn_utils.c:238` | 整文件读入缓冲（≤`MAX_JSON_FILE_LEN`） |
| JSON | `GetJsonObjFromFile` / `ParseJsonConfig` / `GetStringFromJsonObj` / `GetBoolValueFromJsonObj` | `json_utils.h:31-60` | 配置策略化加载与字段提取 |
| 系统参数 | `CheckEnabled` / `IsDeveloperModeOpen` / `GetSpawnTimeout` | `appspawn_utils.c:394-429` | 系统参数查询（含冷启动分支） |
| 网络 NS | `EnableNewNetNamespace` | `appspawn_utils.h:170` | 写 `/sys/devices/virtual/net/lo/flags` 启用 loopback |
| 环境 | `InitCommonEnv` | `appspawn_utils.c:104` | 通用环境变量设置（PATH/HOME/TMPDIR） |
| 调试输出 | `AppSpawnDump` / `SetDumpToStream` | `appspawn_utils.c:343-391` | 格式化输出（自动剥离 `{public}/{private}`） |
| DF 路由 | `AppSpawndfIsServiceEnabled` / `AppSpawndfGetHandle` / `AppSpawndfIsBroadcastMsg` / `AppSpawndfMergeBroadcastResult` | `appspawndf_utils.h:30-41` | DF 子系统客户端管理 |
| 消息标志 | `SetAppSpawnMsgFlags` | `appspawndf_utils.h:44` | 消息标志位设置 |
| C++ 容器 | `TranslateListToVector` / `TranslateVectorToList` | `parcel_util.h:26-41` | `std::list` ↔ `std::vector` |
| TLV 构建 | `CreateTlvEntry` / `AddStandardTlv` / `AddExtTlv` / `WriteTlvEntriesToBuffer` / `FreeTlvEntries` | `modules/spm/tlv_builder.h:66-122` | TLV 描述符链表与缓冲区写入 |
| NoShareFS | `SetNoShareFsEnable` / `IsNoShareFsEnable` | `appspawn_utils.c:447-462` | `const.startup.appspawn_support_nosharefs.enable` |

### 代表性代码

#### 字符串分割（回调式）
```c
// util/include/appspawn_utils.h:159
typedef int (*SplitStringHandle)(const char *str, void *context);
int32_t StringSplit(const char *str, const char *separator, void *context, SplitStringHandle handle);
```

#### 多路径 JSON 配置加载（走配置策略）
```c
// util/src/appspawn_utils.c:282
int ParseJsonConfig(const char *basePath, const char *fileName,
                    ParseConfig parseConfig, ParseJsonContext *context);
```
通过 `GetCfgFiles(basePath)` 枚举配置策略目录，依次加载并解析每个目录下同名 JSON，回调消费。

#### 错误码构造宏（位段编码）
```c
// util/include/appspawn_error.h:34-41
#define DECLARE_APPSPAWN_ERRORCODE(module, submodule, error) \
    ((uint32_t)(DECLARE_APPSPAWN_ERRORCODE_SUBMODULE_BASE(module, submodule) | ((error) & 0x0fff)))
```
将 module（4 bit）/ submodule（4 bit）/ error（12 bit）打包为 32 位错误码，高 bit 固定为 `OHOS_SUBSYS_STARTUP_ID`。

---

## Cross-Module Interactions

| Interaction | With Module | Mechanism | Direction |
|-------------|-------------|-----------|-----------|
| 日志/检查宏 | 所有模块 | APPSPAWN_CHECK, APPSPAWN_LOGI | Outgoing |
| 错误码 | 所有模块 | APPSPAWN_SUCCESS 等 | Outgoing |
| JSON 配置加载 | sandbox | ParseJsonConfig / GetJsonObjFromFile | Outgoing |
| TLV 链表 | spm | CreateTlvEntry / AddStandardTlv | Outgoing |
| DF 路由 | standard, adapters | AppSpawndf* 工具 | Outgoing |

---

## Related Modules
| Module | Relationship | Link |
|--------|-------------|------|
| standard | used_by | [module_standard.md](module_standard.md) |
| sandbox | used_by | [module_sandbox.md](module_sandbox.md) |
| spm | used_by | [module_spm.md](module_spm.md) |
| client_api | used_by | [module_client_api.md](module_client_api.md) |

**另见**: 本模块与上述模块存在依赖关系。具体交互细节请参考 [系统架构](../architecture.md) 中的跨模块关系章节。
