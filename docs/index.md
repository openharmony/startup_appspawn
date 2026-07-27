# appspawn 知识库

> 生成日期: 2026-07-15
> 质量评分: 95/100 (A)
> 最后更新: 2026-07-16

## Overview

appspawn 是 OpenHarmony 操作系统的**应用进程孵化器**，负责接收应用框架（如 Ability Manager Service）的命令，fork 出应用进程，并为其配置安全沙箱、UID/GID 权限、DEC 加密策略等运行时环境。

系统采用**模块化 Hook 架构**：核心服务通过 Hook 引擎将沙箱、SPM 权限管理、ACE 适配等功能模块以插件形式编排到孵化流程的各个阶段（pre-fork、post-fork、child-execute 等）。支持多种孵化模式（appspawn/nwebspawn/nativespawn/hybridspawn）和冷启动模式。

## 快速开始

**初次了解本项目?** 按以下顺序阅读:

1. 本页 -- 项目概述、模块索引、术语表、Gap 进度
2. [系统架构](architecture.md) -- 完整架构图、数据流、模块清单、跨模块关系
3. 从下方模块索引中选择一个模块，阅读其详情页

## 模块索引

| 模块 | KPs | 概述 | 详情 |
|------|-----|------|------|
| standard | 11 | 核心服务: 进程孵化、消息处理、fork/prefork 管理 | [module_standard.md](modules/module_standard.md) |
| sandbox | 6 | 沙箱: 安全隔离、挂载管理、DEC 策略 | [module_sandbox.md](modules/module_sandbox.md) |
| spm | 5 | SPM: 安全进程管理、引用计数 | [module_spm.md](modules/module_spm.md) |
| client_api | 4 | 客户端 SDK: TLV 消息构造、Socket 通信 | [module_client_api.md](modules/module_client_api.md) |
| modulemgr_engine | 3 | Hook 引擎: 阶段定义、模块加载 | [module_modulemgr_engine.md](modules/module_modulemgr_engine.md) |
| common_modules | 5 | 通用模块: cgroup、namespace、隔离 | [module_common_modules.md](modules/module_common_modules.md) |
| ace_adapter | 3 | ACE 适配: checkpoint、镜像进程 | [module_ace_adapter.md](modules/module_ace_adapter.md) |
| util | 3 | 工具类: 字符串/JSON/TLV 等 35+ 函数 | [module_util.md](modules/module_util.md) |
| server_common | 2 | 通用服务: AppSpawnContent、trace | [module_server_common.md](modules/module_server_common.md) |
| hnp | 1 | 原生包管理: 安装、打包 | [module_hnp.md](modules/module_hnp.md) |
| lite | 1 | 小型系统: JSON 消息格式 | [module_lite.md](modules/module_lite.md) |
| adapters | 2 | 适配器集合: native/nweb/sysevent/asan | [module_adapters.md](modules/module_adapters.md) |

## 术语表

| Term | Definition |
|------|------------|
| appspawn | 应用进程孵化器，OpenHarmony 核心系统服务 |
| TLV | Type-Length-Value，appspawn 使用的消息编码格式 |
| APL | Ability Privilege Level，应用特权等级（NORMAL/SYSTEM_BASIC/SYSTEM_CORE） |
| DAC | Discretionary Access Control，自主访问控制（UID/GID） |
| DEC | Dynamic Enhance Control，动态增强控制（通过 `/dev/dec` 设备下发 tokenId + path 策略；**非 fscrypt**） |
| SPM | Security Process Manager，安全进程管理器（内核子系统，管理 tokenid/uid 引用计数） |
| HNP | Harmony Native Package，鸿蒙原生包 |
| prefork | 预创建子进程机制（单进程预留，非进程池） |
| cold run | 冷启动模式，在已 fork 的子进程中直接执行 |
| nwebspawn | NWeb 渲染进程孵化器 |
| nativespawn | 原生进程孵化器 |
| hybridspawn | 混合孵化器 |
| Hook Stage | Hook 阶段（pre-fork/post-fork/child-execute 等） |
| sandbox | 应用沙箱，隔离的文件系统视图 |
| checkpoint | 镜像进程快照，用于启动加速 |

## Identified Gaps

| Gap | Description | Severity | Status |
|-----|-------------|----------|--------|
| G-1 | util 模块的工具函数细节 | Low | **Resolved** (module_util.md) |
| G-2 | devicedebug 服务未探索 | Low | **Resolved** (module_standard.md KP-9) |
| G-3 | JSON 配置文件结构未分析 | Medium | **Resolved** (module_sandbox.md KP-3) |
| G-4 | prefork 完整代码路径未追踪 | Medium | **Resolved** (module_standard.md KP-10) |
| G-5 | DEC/fscrypt 交互（误述已修正） | Low | **Resolved** (module_sandbox.md KP-5) |
| G-6 | DEC util 接口层未单独成节 | Low | **Resolved** (module_sandbox.md KP-5) |
| G-7 | KickDog 看门狗未文档化 | Low | **Resolved** (module_standard.md KP-11) |

## 完整文档索引

| 文档 | 描述 |
|------|------|
| [系统架构](architecture.md) | 架构图、数据流、模块清单、跨模块关系、系统级模式 |
| [模块详情](modules/) | 12 个模块的详细文档（含交叉引用与代码片段） |
| [Recon](recon.md) | 仓库结构和统计 |
| [分解计划](decomposition.md) | 模块分解与知识点规划 |
| [质量报告](quality_report.md) | 质量评估与统计数据 |
