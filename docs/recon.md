# Recon: appspawn 仓库结构发现

## 1. 项目标识

- **名称**: appspawn
- **仓库路径**: `base/startup/appspawn`
- **版本控制**: Git

---

## 2. 目录树（前3层，排除 test/.git/doc）

```
appspawn/
├── common/
│   ├── appspawn_server.c
│   ├── appspawn_server.h
│   ├── appspawn_trace.cpp
│   └── appspawn_trace.h
├── docs/
│   ├── modules/
│   │   └── module_*.md (12 个模块详情)
│   ├── architecture.md
│   ├── decomposition.md
│   ├── index.md
│   ├── quality_report.md
│   └── recon.md
├── etc/
│   └── sandbox/
├── figures/
├── interfaces/
│   └── innerkits/
│       ├── client/
│       ├── dec_util/
│       ├── hnp/
│       ├── include/
│       └── permission/
├── lite/
│   ├── appspawn_message.c
│   ├── appspawn_message.h
│   ├── appspawn_process.c
│   ├── appspawn_service.c
│   ├── appspawn_service.h
│   └── main.c
├── modules/
│   ├── ace_adapter/
│   ├── asan/
│   ├── common/
│   ├── module_engine/
│   │   ├── include/
│   │   └── stub/
│   ├── modulemgr/
│   ├── native_adapter/
│   ├── nweb_adapter/
│   ├── sandbox/
│   │   └── normal/
│   ├── spm/
│   └── sysevent/
├── service/
│   ├── devicedebug/
│   │   ├── base/
│   │   └── kill/
│   └── hnp/
│       ├── base/
│       ├── installer/
│       └── pack/
├── standard/
│   ├── appspawn_appmgr.c
│   ├── appspawn_fd_manager.c
│   ├── appspawn_fd_manager.h
│   ├── appspawn_kickdog.c
│   ├── appspawn_kickdog.h
│   ├── appspawn_main.c
│   ├── appspawn_manager.h
│   ├── appspawn_msgmgr.c
│   ├── appspawn_service.c
│   ├── appspawn_service.h
│   └── pid_ns_init.c
├── util/
│   ├── include/
│   └── src/
├── BUILD.gn
├── appspawn.gni
├── appspawn.cfg
├── appspawn.rc
├── bundle.json
├── appdata-sandbox*.json (多个沙箱配置)
├── appspawn_preload.json
└── README_zh.md
```

---

## 3. 文件统计

| 扩展名 | 文件数 | 估计代码行数 |
|--------|--------|-------------|
| .c     | 43     | ~6,804      |
| .h     | 43     | (含在上述中) |
| .cpp   | 19     | ~21,697     |
| .gn    | 20     | 未统计      |
| .gni   | 3      | 未统计      |
| .json  | 16     | 配置文件    |
| .xml   | 1      | OAT.xml     |
| .py    | 1      | 脚本        |

**总代码行数（.c + .h + .cpp）**: ~28,501 行

---

## 4. 构建系统

- **构建系统**: GN/Ninja (OpenHarmony 标准构建系统)
- **构建配置文件**: `BUILD.gn`（根目录及各子目录），`appspawn.gni`（全局变量定义）
- **bundle描述**: `bundle.json`
- **编译选项定义**: `appspawn.gni` 定义了 `appspawn_innerkits_path`、`appspawn_path` 等路径变量
- **条件编译标志**: `WITH_SELINUX`、`WITH_SECCOMP`、`CUSTOM_SANDBOX`、`NORMAL_SANDBOX`、`ALLOW_DUMPABLE`

---

## 5. 编程语言

- **主语言**: C (43 .c 文件) 和 C++ (19 .cpp 文件)
- **头文件**: 43 .h 文件
- **领域特定语言**: GN 构建脚本 (.gn/.gni), JSON 配置文件 (沙箱配置、预加载配置等)
- **脚本**: Python (1 文件)

---

## 6. 顶层目录描述

| 目录        | 文件数 | 描述                                      |
|-------------|--------|-------------------------------------------|
| common/     | 4      | 通用服务端代码（服务注册、trace 追踪）    |
| docs/       | 30+    | 代码知识库（架构/模块详情/质量报告）       |
| etc/        | 6      | 沙箱配置等 etc 文件                        |
| figures/    | -      | README 图片资源                            |
| interfaces/ | 15     | 对外接口：客户端 API、HNP API、权限接口    |
| lite/       | 7      | 小型系统 appspawn 实现                     |
| modules/    | 62     | 模块化组件：沙箱、SPM、适配器、模块引擎等  |
| service/    | 26     | 独立服务：HNP（原生包管理）、设备调试      |
| standard/   | 12     | 标准系统 appspawn 核心实现                 |
| util/       | 8      | 工具类：错误码、JSON 工具、parcel 工具     |

---

## 7. 关键文件

| 文件路径 | 说明 |
|----------|------|
| `standard/appspawn_main.c` | 标准系统 appspawn 主入口 |
| `standard/appspawn_service.c` | 标准系统核心服务实现 |
| `standard/appspawn_msgmgr.c` | 消息管理器，处理 IPC 请求 |
| `standard/appspawn_manager.h` | 核心管理器头文件，定义关键数据结构 |
| `lite/main.c` | 小型系统 appspawn 主入口 |
| `lite/appspawn_service.c` | 小型系统服务实现 |
| `interfaces/innerkits/include/appspawn.h` | 对外公共 API 头文件 |
| `interfaces/innerkits/client/appspawn_client.c` | 客户端实现 |
| `interfaces/innerkits/client/appspawn_msg.c` | 客户端消息构造 |
| `modules/modulemgr/appspawn_modulemgr.c` | 模块管理器 |
| `modules/module_engine/include/appspawn_hook.h` | Hook 机制头文件 |
| `modules/sandbox/normal/sandbox_core.cpp` | 沙箱核心逻辑 |
| `modules/spm/spm.c` | SPM（软件包管理器）权限管理 |
| `appspawn.gni` | 全局构建变量定义 |
| `BUILD.gn` | 根构建文件 |
| `bundle.json` | OpenHarmony 部件描述文件 |
| `appspawn.cfg` | appspawn 服务配置（init 脚本） |
| `appdata-sandbox.json` | 沙箱配置（主文件） |
| `appspawn_preload.json` | 预加载库配置 |
