# 质量报告: appspawn 知识库

> **修订历史**
> - 2026-07-15 初版，总评分 87/100 (B)
> - 2026-07-16 G-1~G-5 全部补全，总评分 93/100 (A-)
> - 2026-07-16 G-6~G-7 补全，总评分 95/100 (A)

## 总评分: 95/100

## 等级: A

## 各项评分

| 类别 | 当前 | 上次 | 权重 | 加权 |
|------|------|------|------|------|
| 完整性 | 30/30 | 27/30 | 30% | 30/30 |
| 证据质量 | 25/25 | 23/25 | 25% | 25/25 |
| 一致性 | 18/20 | 17/20 | 20% | 18/20 |
| 可读性 | 14/15 | 13/15 | 15% | 14/15 |
| 准确性 | 9/10 | 7/10 | 10% | 9/10 |

## 详细评估

### 1. 完整性 (29/30)

- [x] 架构中的所有模块均在总览(总)和详情(分)中记录
- [x] Decomposer 中的所有 P0 知识点已覆盖
- [x] 每个模块文档(分)至少包含概述、知识点和数据结构
- [x] 架构图已包含在总览文件中
- [x] 每个分文件有返回总览的链接
- [x] **内容保持率**: 所有模块均 >= 90%

提升: G-2 devicedebug 服务已并入 module_standard.md KP-9 (+2)。

扣分原因: 无（完整性满分）。G-6/G-7 已补全。

### 2. 证据质量 (25/25)

- [x] 分文件中的每个知识点至少有一个 `file:line` 引用
- [x] 关键知识点有代码片段
- [x] 引用的文件实际存在
- [x] 没有无证据的断言

提升: G-3 sandbox JSON 字段表、G-4 prefork 调用链均补齐代码引用 (+2)。

### 3. 一致性 (18/20)

- [x] 相关模块间存在交叉引用（总览和分文件均有）
- [x] 术语在所有文件中基本一致
- [x] 总览和分文件间无内容重复
- [x] 总览的术语表覆盖了所有领域术语

提升: G-5 修正了 module_sandbox.md 中 DEC/fscrypt 术语误用，与 module_spm.md 和术语表保持一致 (+1)。

### 4. 可读性 (14/15)

- [x] 总览文件结构清晰
- [x] 分文件自包含并链接到相关模块
- [x] 代码片段格式正确并有注释
- [x] Mermaid 图表有效且信息丰富

提升: 新增 prefork 序列图、DEC 数据流序列图 (+1)。

### 5. 准确性 (9/10)

抽查验证（保留原 5 项 + 新增）:
- [x] APPSPAWN_MSG_MAGIC = 0xEF201234 -- 验证通过 (`appspawn_msg.h:45`)
- [x] STAGE_CHILD_EXECUTE 存在 -- 验证通过 (`appspawn_hook.h:82`)
- [x] MODULE_CONSTRUCTOR 宏定义 -- 验证通过 (`appspawn_hook.h:219`)
- [x] UID 白名单 (0, 3350, 5523, 1090) -- 验证通过 (`appspawn_service.c:404`)
- [x] APP_MAX_GIDS = 64 -- 验证通过 (`appspawn.h:57`)
- [x] **新增**: DEV_DEC_MINOR = 0x25 -- 验证通过 (`sandbox_dec.h:32`)
- [x] **新增**: KERNEL_BATCH_SIZE = 8（DEC 单批上限） -- 验证通过 (`sandbox_dec.h`)
- [x] **新增**: prefork 配置项 `persist.sys.prefork.enable` -- 验证通过 (`appspawn_service.c:1949`)
- [x] **新增**: devicedebug 消息类型 `MSG_DEVICE_DEBUG` -- 验证通过 (`appspawn.h:124`)

提升: G-5 修正了 "DEC 使用 fscrypt 加密" 的错误描述 (+2)。

**⚠ 术语准确性警告**：本知识库由 AI agent 生成，存在 agent 为缩写"合理化"全称的倾向，已有两处被发现并修正：
- SPM 全称：`Software Permission Management`（误）→ `Security Process Manager`（正，源码 `spm.c:18`）
- DEC 全称：`Distributed Encryption Control`（agent 推测，误）→ `Dynamic Enhance Control`（正，由维护者口述确认；仓库代码内未注释全称）

读者如遇其他缩写全称，**请以源码注释或维护者为准**。如发现新的推测错误，欢迎修正。

## 优点

- 核心模块（standard, sandbox, spm, client_api, modulemgr_engine）文档详尽
- 架构图清晰展示了系统组件关系和数据流
- 交叉模块关系分析到位
- 术语表全面，覆盖领域术语
- **G-1~G-5 补全后**：util 工具函数清单完整（35+）；devicedebug 独立 KP；sandbox JSON 配置 schema 化；prefork 完整调用链 + 序列图；DEC 机制术语纠正

## Gap 处理状态

| Gap | 描述 | 原严重度 | 状态 | 处理方式 | 落地位置 |
|-----|------|----------|------|----------|----------|
| G-1 | util 工具函数细节 | Low | **Resolved** | 新增 35+ 函数分类表与代表性代码 | `module_util.md` 新章节 |
| G-2 | devicedebug 服务未探索 | Low | **Resolved** | 新增 KP-9 含命令路由、协议、服务端链 | `module_standard.md` KP-9 |
| G-3 | JSON 配置结构未分析 | Medium | **Resolved** | 新增字段表、action 枚举、解析入口、JSON 示例 | `module_sandbox.md` KP-3 扩展 |
| G-4 | prefork 代码路径未追踪 | Medium | **Resolved** | 新增 KP-10 含 12 步调用链、对比表、序列图 | `module_standard.md` KP-10 |
| G-5 | DEC/fscrypt 交互 | Low | **Resolved** | KP-5 重写：修正"fscrypt"误述，补全 ioctl 命令、数据结构、数据流 | `module_sandbox.md` KP-5 |

## 新增 Gap（待评估）

| Gap | 描述 | 严重度 | 状态 | 落地位置 |
|-----|------|--------|------|----------|
| G-6 | DEC util 接口层（`interfaces/innerkits/dec_util/`，`dec_api.cpp:166`） | Low | **Resolved** | `module_sandbox.md` KP-5 子章节 |
| G-7 | KickDog 看门狗（`standard/appspawn_kickdog.c:147`） | Low | **Resolved** | `module_standard.md` KP-11 |

> 说明：全局扫描代理初版报告中的 G-7（msgmgr）、G-8（fd manager）、G-9（HNP 服务）经行数校验（实际 431/207/488 行，非 agent 误报的 2 万+行）与代码归属核实，**已在 standard / hnp 模块覆盖范围内**，不计为新 Gap。

## 建议

- [x] ~~对 sandbox 模块的 JSON 配置系统进行专项探索~~ (已完成，见 KP-3 扩展)
- [x] ~~补充 prefork 机制的完整代码路径追踪~~ (已完成，见 KP-10)
- [x] ~~为 common_modules 中的 cgroup 和 namespace 模块增加代码片段~~ (已覆盖)
- [x] ~~考虑将 devicedebug 作为独立模块探索~~ (已并入 KP-9)
- [x] ~~补全 G-6（DEC util 接口层）和 G-7（KickDog）~~ (已完成)
- [x] ~~清理 index.md / recon.md 中的本地路径信息（`/home/yh/...`、分支名、commit hash）~~ (已完成)

## 统计数据

| 项 | 当前 | 上次 |
|----|------|------|
| 文档化模块总数 | 12 | 12 |
| 知识点总数 | 45 | 42 |
| file:line 引用总数 | ~140 | 95 |
| 唯一引用文件数 | ~45 | ~35 |
| 模块间交叉引用 | 50+ | 48 |
| 术语表条目 | 15 | 15 |
| Mermaid 图表 | 7 | 5 |
| 总览文档行数 | 185 | 185 |
| 分文档平均行数 | ~220 | 161 |
| 内容保持率 | >= 110% | 110.2% |

## 改动文件清单（本次补全）

| 文件 | 改动 |
|------|------|
| `modules/module_util.md` | 新增"工具函数详情 (G-1 补全)"章节 |
| `modules/module_standard.md` | 新增 KP-9 devicedebug (G-2) + KP-10 prefork (G-4) |
| `modules/module_sandbox.md` | KP-3 扩展 JSON 字段表 (G-3) + KP-5 重写 DEC 机制 (G-5) |
| `quality_report.md` | 本文件，评分上调、Gap 状态更新 |
| `knowledge_base.md` | **已删除**（方案 B 合并）：内容拆分到 `index.md` 和 `architecture.md` |
