# HNP（Harmony Native Package）技术讲解

> 基于 `service/hnp/` 与 `interfaces/innerkits/hnp/` 源码整理。

## 1 概述

HNP 是 OpenHarmony 的 **Native 软件包** 管理体系，用于在设备上安装、卸载、运行原生二进制（如 Python、Node、Java 等 Native 程序）。Native 软件以 `.hnp`（zip 格式）为载体，随 HAP 应用分发，安装后在设备上生成软链接供命令行或应用 `execv` 调用。

整个体系由三个构建产物构成：

| 产物 | 源码入口 | 定义宏 | 运行位置 | 职责 |
|------|----------|--------|----------|------|
| `hnpcli` | `service/hnp/hnpcli_main.c` | `HNP_CLI` | 开发机（Linux/Win/Mac/OHOS） | 将 Native 软件目录打包成 `.hnp` 文件 |
| `hnp` | `service/hnp/hnp_main.c` | （默认） | 设备端（`/system/bin/hnp`） | 安装/卸载 `.hnp`，生成软链接，管理版本 |
| `libhnpapi.z.so` | `interfaces/innerkits/hnp/src/hnp_api.c` | — | 设备端 | 供 BMS 等系统服务调用的 C API，内部 fork+execve 拉起 `hnp` 进程 |

## 2 源码结构

```
service/hnp/
├── base/                     # 打包与安装共用的基础库
│   ├── hnp_base.h            # 核心数据结构、错误码、日志宏、路径常量
│   ├── hnp_json.c            # hnp.json 解析/生成；hnp_info.json 安装信息管理
│   ├── hnp_zip.c             # zip 压缩/解压；ELF 文件识别（判断可执行文件）
│   ├── hnp_sal.c             # 软链接生成、进程运行检查（lsof）、相对路径计算
│   ├── hnp_file.c            # 文件读写、目录创建/删除
│   └── hnp_log.c             # hnpcli 日志输出（非 HiLog 路径）
├── pack/                     # 打包功能（hnpcli 专用）
│   ├── include/hnp_pack.h    # 打包参数与信息结构体
│   └── src/hnp_pack.c        # 打包主逻辑：遍历目录→zip 压缩→注入 hnp.json
├── installer/                # 安装/卸载功能（hnp 专用）
│   ├── include/hnp_installer.h
│   └── src/hnp_installer.c   # 安装/卸载主逻辑：解压→软链接→签名→版本管理
├── hnp_main.c                # hnp 可执行文件 main()：命令分发 install/uninstall
├── hnpcli_main.c             # hnpcli 可执行文件 main()：命令分发 pack
└── BUILD.gn                   # 构建定义：hnp 链接 installer+base，hnpcli 链接 pack+base

interfaces/innerkits/hnp/
├── include/hnp_api.h         # 对外 API：NativeInstallHnp / NativeUnInstallHnp
├── src/hnp_api.c             # API 实现：组装 argv → fork → execve("/system/bin/hnp")
└── BUILD.gn                   # 构建 libhnpapi.z.so
```

**构建要点**（`service/hnp/BUILD.gn`）：
- `hnp` 链接 `base/*.c` + `installer/hnp_installer.c` + `hnp_main.c`，依赖 `hilog`、`selinux_adapter`、`zlib`、`code_signature`。
- `hnpcli` 链接 `base/*.c`（去掉 `hnp_sal.c`）+ `pack/hnp_pack.c` + `hnpcli_main.c`，定义 `HNP_CLI`（禁用 HiLog，改用 `HnpLogPrintf`；禁用 ELF 检查等设备端逻辑）。
- `libhnpapi.z.so` 仅含 `hnp_api.c`，依赖 `libappspawn_util`、`libbegetutil`、`libhilog`。

## 3 核心概念

### 3.1 hnp.json —— 软件包配置文件

每个 `.hnp` 内必须含 `hnp.json`，描述软件名、版本、软链接关系：

```json
{
    "type": "hnp-config",
    "name": "hnpsample",
    "version": "1.1",
    "install": {
        "links": [
            { "source": "/bin/hnpsample", "target": "hnpsample" }
        ]
    }
}
```

- `type` 固定为 `"hnp-config"`（`hnp_json.c:83` 校验）。
- `links.source` 为软件包内二进制相对路径，`links.target` 为生成的软链接名（为空时默认取 source 文件名）。
- 若 `links` 缺失，安装时默认将 `bin/` 目录下所有常规文件做软链接（`hnp_installer.c:113` `HnpGenerateSoftLinkAll`）。

### 3.2 公有（public）与私有（private）

HAP 内 hnp 包按目录名区分公私有：

```
hnp安装包路径
├── public/          # 公有 hnp
│   └── xxx.hnp
└── private/         # 私有 hnp
    └── yyy.hnp
```

安装后落盘路径（`hnp_installer.c:693`）：

| 类型 | 物理路径 | 沙箱软链接路径 | 环境变量 |
|------|----------|----------------|----------|
| 公有 | `/data/app/el1/bundle/<uid>/hnppublic/<name>.org/<name>_<ver>` | `/data/service/hnp/bin` | `HNP_PUBLIC_HOME=/data/service/hnp` |
| 私有 | `/data/app/el1/bundle/<uid>/hnp/<hapPkg>/<name>.org/<name>_<ver>` | `/data/app/<bundleName>/...` | `HNP_PRIVATE_HOME=/data/app` |

- `HNP_PRIVATE_HOME` 排在 `HNP_PUBLIC_HOME` **之前**，同名二进制优先执行私有路径下的。
- 公有 hnp 可被所有应用访问；私有 hnp 仅安装它的 HAP 可访问。
- 公有 hnp 安装信息记录在 `hnp_info_<uid>.json`（旧路径 `hnp_info.json`），私有 hnp 不记入此文件。

### 3.3 软链接机制

安装时在 `<hnpBasePath>/bin/` 下为目标二进制生成**相对路径**软链接（`hnp_sal.c:164` `HnpSymlink`）：

```
软链接文件: .../hnppublic/bin/hnpsample  →  ../../hnpsample.org/hnpsample_1.1/bin/hnpsample（相对路径）
```

使用相对路径而非绝对路径，保证路径迁移后链接不断。公有 hnp 软链接覆盖前会做 `CheckSymlink` 校验（`hnp_sal.c:56`），防止覆盖其他 hnp 的链接。

### 3.4 版本管理

`hnp_info_<uid>.json` 中每个 hnp 条目记录两个版本（`hnp_base.h:148` `HnpPackageInfo`）：

| 字段 | 含义 |
|------|------|
| `current_version` | 当前生效版本 |
| `install_version` | 本 HAP 实际安装的版本（未安装该版本时为 `"none"`） |

**核心约束：每个公有 hnp 名称仅属于一个 HAP。**

`CanRecovery`（`hnp_json.c:459`）在生成公有 hnp 软链接前校验：遍历 `hnp_info` 中所有 HAP 条目，若发现已有其他 HAP 安装了同名的 hnp，则返回 `false` → 安装失败（`HNP_ERRNO_SYMLINK_CHECK_FAILED`）。仅当无 HAP 安装过此名称、或安装者就是当前 HAP 时才允许覆盖。README 规格第 5 条也明确："Hap 应用 A 和 B 先后安装同名公有 hnp 包，后安装的应用 B 会无法安装"。

> 注：`hnp_base.h:122` 有一段注释描述了多 HAP (A→v1, B→v2, C→v3) 共享同一 hnp 不同版本的场景，但该模型被 `CanRecovery` 限制，在当前代码下不可达，属遗留/理论性描述。

**同一 HAP 的版本升级流程**：

```
1. 首次安装 v1:  current_version=v1, install_version=v1  → 仅 v1 目录在磁盘
2. 同 HAP 升级 v2:
   HnpPublicDealAfterInstall (hnp_installer.c:525)
     ├── HnpCurrentVersionUninstallCheck: 检查是否有 HAP 的 current==install
     │   当前 {current=v1, install=v1} 匹配 → 返回 "v1" → 跳过卸载旧目录
     │   （v1 目录保留在磁盘，不立即清理）
     └── HnpInstallInfoJsonWrite: 仅更新 current_version=v2, install_version 保持 v1
   结果: current=v2, install=v1 → v1 和 v2 两个目录都在磁盘
3. 卸载该 HAP:
   HnpNativeUnInstall (hnp_installer.c:347)
     ├── hnpExist=false（无其他 HAP 引用）→ 删除 current_version(v2) 目录
     └── install_version(v1) != "none" 且 != current → 删除 install_version(v1) 目录
   结果: v1、v2 目录均清理
```

`install_version = "none"` 的场景在当前正常流程中不产生（`isInstall` 在 `HnpInstallInfoJsonWrite` 前必被置 true），仅可能来自旧格式迁移（`DoRebuildHnpInfoCfg`）。

## 4 打包流程（hnpcli）

`hnpcli pack` 命令入口 `hnp_pack.c:202` `HnpCmdPack`：

```
参数: -i <源目录>  -o <输出目录>  -n <软件名>  -v <版本号>
```

1. **解析参数**（`ParsePackArgs`）：对源目录做 `realpath` 校验；检查源目录下是否存在 `hnp.json`。
   - 存在 → 解析其中的 name/version，校验 links.source 文件是否存在。
   - 不存在 → 要求用户传 `-n` 和 `-v`，打包时自动生成 `hnp.json`。
2. **压缩**（`PackHnp`）：调用 `HnpZip`（`hnp_zip.c:258`）递归遍历源目录，将所有文件以相对路径写入 `<name>.hnp`（zip 格式）。
   - zip 内保存文件 UGO 权限到 `external_fa` 字段（`hnp_zip.c:115`），安装时恢复。
   - Windows 打包时自动为 others 赋可执行权限；Linux/Mac/OHOS 继承源文件权限。
3. **注入 hnp.json**：若源目录无 `hnp.json`，调用 `AddHnpCfgFileToZip` 用 cJSON 生成并写入压缩包。

跨平台支持：`hnp_zip.c` 对 Windows 路径做 `\` → `/` 转换、用 `wchar_t` 支持长路径（>260 字符）。

## 5 安装流程（hnp install）

`hnp install` 命令入口 `hnp_installer.c:1110` `HnpCmdInstall`：

```
hnp install -u <uid> -p <hapPkg> -i <hnp根目录> -s <hap路径> -a <abi> [-f] [-I <appIdentifier>] [-S <独立签名hnp路径>]
```

### 5.1 整体流程

```
ParseInstallArgs          解析 -u/-p/-i/-s/-a/-f/-I/-S 参数
    │
    ▼
RebuildHnpInfoCfg         若 hnp_info 旧格式文件存在则重建为新格式（按 uid 分文件）
    │
    ▼
HnpInstallPre             安装前置
    ├── CheckInstallPath     拼装路径 + restorecon（SELinux 标签）
    ├── HnpInstallHapFileCountGet  统计 hnp 文件数（预分配签名信息数组）
    └── HapReadAndInstall     遍历 public/ 和 private/ 目录
         └── HnpPackageGetAndInstall  递归遍历子目录
              └── HnpReadAndInstall   对每个 .hnp 文件
                   ├── HnpCfgGetFromZip    从 zip 中读取 hnp.json
                   ├── HnpInstallPathGet  拼装安装路径 <base>/<name>.org/<name>_<ver>
                   ├── HnpInstallForceCheck 路径已存在则判断 -f 强制/报错
                   ├── HnpInstall              解压 + 生成软链接
                   │    ├── HnpUnZip             解压到目标目录 + 收集签名信息
                   │    └── HnpGenerateSoftLink  按 hnp.json links 或默认 bin/ 生成软链接
                   └── HnpPublicDealAfterInstall  公有 hnp：版本清理 + 写 hnp_info.json
    │
    ▼
CodeSign + BssInstall      （若 CODE_SIGNATURE_ENABLE）对可执行 ELF 做验签
    │
    ▼
GetRespFd → write(result)  通过管道回写结果给调用方（API 场景）
```

### 5.2 关键细节

- **路径拼装**（`hnp_installer.c:498`）：`<hnpBasePath>/<name>.org/<name>_<version>`，对 `..` 做路径穿越防护。
- **强制安装 `-f`**：路径已存在时先删后装；非强制则返回 `HNP_ERRNO_INSTALLER_PATH_IS_EXIST`。
- **同名版本跳过**：若目标版本目录已存在且为公有 hnp，跳过解压，仅刷新软链（`hnp_installer.c:565`）。
- **批量安装失败**：中途出错直接退出，已安装的保留；出错时对当前 hnp 做回滚卸载（`hnp_installer.c:592`）。
- **SELinux**：安装前对 `hnppublic/` 和 `hnp/` 目录做 `RestoreconRecurse`（`hnp_installer.c:831`）。

## 6 卸载流程（hnp uninstall）

`hnp uninstall` 命令入口 `hnp_installer.c:1140` `HnpCmdUnInstall`：

```
hnp uninstall -u <uid> -p <hapPkg>
```

流程（`HnpUnInstall`）：

1. `RebuildHnpInfoCfg`：配置格式迁移。
2. `HnpPackageInfoGet`：从 `hnp_info_<uid>.json` 读取该 HAP 安装的所有 hnp 条目。
3. 对每个条目调 `HnpNativeUnInstall`：
   - 若 `hnpExist=false`（无其他 HAP 引用此版本，因 `CanRecovery` 限制实际恒为 false）→ 删除 `current_version` 目录。
   - 若 `install_version != "none"` 且 != `current_version` → 删除 `install_version` 目录。
   - 删除前调 `HnpProcessRunCheck`（`hnp_sal.c:30`）：用 `lsof` 检查是否有进程正在使用该路径，有则卸载失败。
4. `HnpPackageInfoDelete`：从 `hnp_info_<uid>.json` 删除该 HAP 条目。
5. 删除私有 hnp 目录 `.../hnp/<hapPkg>`。
6. `ClearSoftLink`：清理 `hnppublic/bin/` 下失效的软链接（源文件已不存在的链接）。
7. `BssUninstall`：调用二进制安全 SDK 注销 BSS 信息。

> 代码中 `hnpExist`（`HnpOtherPackageInstallCheck`）仍保留了对"其他 HAP 是否引用此版本"的检查，但由于 `CanRecovery` 已限制每个公有 hnp 名称仅属于一个 HAP，该字段在正常流程下恒为 `false`，属兼容性遗留逻辑。

## 7 API 接口（libhnpapi.z.so）

`hnp_api.h` 对外暴露两个 C 函数，供 BMS 等系统服务在 HAP 安装/卸载时调用：

### 7.1 NativeInstallHnp

```c
int NativeInstallHnp(const char *userId, const char *hnpRootPath, const HapInfo *hapInfo, int installOptions);
```

**实现机制**（`hnp_api.c:149`）：
1. `IsHnpInstallEnable`：读系统参数 `const.startup.hnp.install.enable`，必须为 `"true"`（对应开发者模式开关）。
2. 组装 `hnp install` 命令行 argv（含 `-u`、`-i`、`-p`、`-s`、`-a`、`-I`、`-S`、`-f`）。
3. `StartHnpProcess`（`hnp_api.c:94`）：
   - `pipe(fd)` 创建管道。
   - `fork()` 子进程调 `ChildProcessHandler`：将 `pipe[1]` 写端设入环境变量 `HNP_INFO_RET_FD_ENV`，再 `execve("/system/bin/hnp", argv, env)`。
   - 父进程 `waitpid` 等待子进程退出，从 `pipe[0]` 读回 `HnpResult{int result}`。

返回值通过管道传递，而非进程退出码——这是 API 层与 hnp 进程的通信协议。

### 7.2 NativeUnInstallHnp

```c
int NativeUnInstallHnp(const char *userId, const char *packageName);
```

同样 fork+execve 拉起 `hnp uninstall -u <uid> -p <pkg>`，通过管道回传结果。

### 7.3 HapInfo 结构体

```c
typedef struct HapInfo {
    char packageName[256];        // HAP 包名
    char hapPath[512];            // hap 文件路径（用于签名校验）
    char abi[128];                // 系统 ABI（如 arm64-v8a）
    char appIdentifier[64];       // 应用唯一标识
    int count;                    // 独立签名 hnp 数量
    char **independentSignHnpPaths; // 独立签名 hnp 相对路径
} HapInfo;
```

`installOptions` 为位掩码，`OPTION_INDEX_FORCE = 0`（第 0 位为 1 表示强制安装）。

## 8 签名与安全

### 8.1 代码签名（Code Signature）

当 `CODE_SIGNATURE_ENABLE` 定义时（`hnp_installer.c:891`）：

1. 安装解压时 `HnpUnZip` 收集所有 ELF 文件的路径与签名 key（格式 `hnp/<abi>/<subdir>/<file>.hnp!/<internal_path>`，`hnp_zip.c:415`）。
2. `HnpELFFileCheck`（`hnp_zip.c:362`）：读文件头判断是否 ELF，进一步判断 `e_type`：
   - `ET_EXEC` → 可执行文件
   - `ET_DYN` 且 `e_entry != 0` → PIE 可执行文件
   - `ET_DYN` 且 `e_entry == 0` → 动态库（不可执行）
3. `EnforceCodeSignForApp`：对 hap 包做文件级强制验签。
4. 验签失败 → 回滚卸载已安装内容。

### 8.2 BSS（Binary Security SDK）

安装后若存在 `libsps_binary_security_sdk.z.so`（`hnp_installer.c:34`），通过 `dlopen`+`dlsym` 动态调用：
- `ProcessHnpInstall(bundleName, appIdentifier, userId, hnpFiles)`：注册可执行文件信息。
- `ProcessHnpUninstall(bundleName, userId)`：注销信息。

独立签名（`independentSign`）的 hnp 通过 `-S` 参数指定，与普通 hnp 区分处理。

### 8.3 路径穿越防护

多处对 `..` 做检查：`hnp_installer.c:76`（links 配置）、`hnp_installer.c:517`（版本路径）、`hnp_zip.c:489`（解压文件名）、`hnp_installer.c:1004`（-S 参数）。

## 9 错误码体系

错误码按位打包（`hnp_base.h:186`）：

```
高16位: HNP_ERRNO_HNP_MID (0x80) << 16 = 0x800000
中8位:  module id << 8
低8位:  具体错误号
```

模块 ID（`hnp_base.h:194`）：

| 模块 | ID | 范围 |
|------|----|------|
| `HNP_MID_MAIN` | 0x10 | `0x8010xx` |
| `HNP_MID_BASE` | 0x11 | `0x8011xx` |
| `HNP_MID_PACK` | 0x12 | `0x8012xx` |
| `HNP_MID_INSTALLER` | 0x13 | `0x8013xx` |

API 层错误码独立（`hnp_api.h:39`）：基值 `0x2000`，如 `HNP_API_ERRNO_HNP_INSTALL_DISABLED = 0x2009`。

## 10 日志体系

两套日志路径（`hnp_base.h:377`）：

| 宏 | hnp（设备端） | hnpcli（开发机） |
|----|---------------|-------------------|
| `HNP_LOGI` | `HILOG_INFO` + `HnpLogPrintf` | 仅 `HnpLogPrintf` |
| `HNP_LOGE` | `HILOG_ERROR` + `HnpLogPrintf` | 仅 `HnpLogPrintf` |

HiLog 格式串使用 `%{public}s` 等隐私标记。`hnp_log.c` 提供 `HnpLogPrintf` 的 printf 风格实现，供 hnpcli 在无 HiLog 环境下使用。

## 11 HNP 如何挂载到应用沙箱（appspawn 集成）

HNP 安装后写入的是设备物理路径（`/data/app/el1/bundle/...`），应用进程无法直接访问。appspawn 在 fork 应用进程后、通过沙箱 **bind mount** 将 HNP 目录挂载进应用沙箱，再配合环境变量让应用能直接 `execv` 执行 Native 二进制。

### 11.1 触发条件：开发者模式 + HNP 执行开关

应用孵化时，`appspawn_service.c:1481` `ProcessSpawnReqMsg` 做两重校验：

```c
if (IsDeveloperModeOpen()) {                          // 系统开发者模式开关
    if (IsSupportRunHnp()) {                          // 读系统参数 const.startup.hnp.execute.enable == "true"
        SetAppSpawnMsgFlag(message, TLV_MSG_FLAGS, APP_FLAGS_DEVELOPER_MODE);
    }
}
```

两个条件均满足时，孵化消息被打上 `APP_FLAGS_DEVELOPER_MODE`（`appspawn.h:196`，bit 17）标记。此标记是后续沙箱挂载 HNP 目录的开关。

### 11.2 沙箱挂载配置

挂载点定义在沙箱 JSON 配置中（`appdata-sandbox.json` + `appdata-sandbox-app.json`），归入 `DEVELOPER_MODE` 标志组：

```json
{
    "flags": "DEVELOPER_MODE",
    "mount-paths": [
        {
            "src-path": "/data/app/el1/bundle/<currentUserId>/hnppublic",
            "sandbox-path": "/data/service/hnp",
            "sandbox-flags": [ "bind", "rec" ],
            "check-action-status": "false"
        },
        {
            "src-path": "/data/app/el1/bundle/<currentUserId>/hnp/<PackageName>",
            "sandbox-path": "/data/app",
            "sandbox-flags": [ "bind", "rec" ],
            "check-action-status": "false"
        }
    ]
}
```

| 挂载源（物理路径） | 挂载目标（沙箱内路径） | 类型 | 对应 HNP |
|---------------------|------------------------|------|----------|
| `/data/app/el1/bundle/<uid>/hnppublic` | `/data/service/hnp` | bind + rec | 公有 hnp |
| `/data/app/el1/bundle/<uid>/hnp/<PackageName>` | `/data/app` | bind + rec | 私有 hnp |

- `<currentUserId>`、`<PackageName>` 由沙箱变量替换系统解析（`sandbox_core.cpp` 中的 `ConvertDebugRealPath` 等函数）。
- `bind` + `rec` 表示递归 bind mount，将整个 hnp 目录树（含 `bin/` 软链接）映射进沙箱。
- HNP 挂载点仅存在于 `appdata-sandbox.json`（标准应用）和 `appdata-sandbox-app.json`（app 类沙箱）中，isolated/render/gpu/debug 等沙箱配置不含 HNP 挂载。

### 11.3 挂载执行流程

沙箱模块（`modules/sandbox/normal/sandbox_core.cpp`）在 `STAGE_CHILD_POST_RELY` 阶段执行挂载：

```
HandleFlagsPoint (sandbox_core.cpp:923)
    │  遍历沙箱 JSON 中的 flags-points 数组
    ▼
对每条配置:
    ConvertFlagStr("DEVELOPER_MODE") → APP_FLAGS_DEVELOPER_MODE
    CheckAppMsgFlagsSet(property, APP_FLAGS_DEVELOPER_MODE)?
        │  是 → DoAllMntPointsMount → mount(src, dst, MS_BIND|MS_REC)
        │  否 → 跳过
        ▼
    DoAllSymlinkPointslink → 处理沙箱内软链接
```

只有 `APP_FLAGS_DEVELOPER_MODE` 被设置的应用才会触发 HNP 挂载；非开发者模式下应用沙箱中看不到 HNP 目录。

### 11.4 环境变量

appspawn 为每个应用进程注入环境变量（`util/src/appspawn_utils.c:40` `COMMON_ENV`）：

```c
{"HNP_PRIVATE_HOME", "/data/app", false},
{"HNP_PUBLIC_HOME",  "/data/service/hnp", false},
{"PATH", "${HNP_PRIVATE_HOME}/bin:${HNP_PUBLIC_HOME}/bin:${PATH}", false},
```

- `HNP_PRIVATE_HOME` 和 `HNP_PUBLIC_HOME` 指向沙箱内挂载点（与 11.2 的 sandbox-path 对应）。
- `PATH` 中 `HNP_PRIVATE_HOME/bin` 排在 `HNP_PUBLIC_HOME/bin` **前面**，同名二进制优先执行私有路径下的。
- 环境变量中的 `${HNP_PRIVATE_HOME}` 等占位符在注入前由 `ConvertEnvValue`（`appspawn_utils.c:50`）展开。

### 11.5 运行时视角

安装后，应用沙箱内看到的路径结构：

```
应用沙箱内（fork 后的子进程视角）
├── /data/service/hnp/          ← 公有 hnp 挂载点 (HNP_PUBLIC_HOME)
│   └── bin/
│       ├── python → ../../python.org/python_3.11/bin/python  (相对软链接)
│       └── node    → ../../node.org/node_20/bin/node
└── /data/app/                   ← 私有 hnp 挂载点 (HNP_PRIVATE_HOME)
    └── bin/
        └── mytool → ../../mytool.org/mytool_1.0/bin/mytool
```

应用通过 `execv("/data/service/hnp/bin/python")` 或直接 `execvp("python")`（依赖 PATH）即可执行 Native 二进制。`README_zh.md` 示例：

```c
int ret = execv("/data/app/test.org/test_1.1/bin/testBin", NULL);  // 私有 hnp 实际路径
```

## 12 端到端流程图

```
开发者侧                                  设备侧
─────────                                ──────────

Native 软件目录
  bin/ cfg/ lib/ hnp.json
        │
        ▼  hnpcli pack -i ... -o ...
     .hnp 文件
        │
        ▼  打入 HAP 包（module.json5 配置 hnpPackages）
     签名后的 HAP
        │
        ▼  应用市场分发 → 设备安装 HAP
                                   │
                                   ▼  BMS 调用 NativeInstallHnp()
                              libhnpapi.z.so
                                   │ fork + execve
                                   ▼
                              /system/bin/hnp install ...
                                   │
                    ┌──────────────┼──────────────┐
                    ▼              ▼              ▼
              解压 .hnp      生成软链接      验签(ELF)
                    │              │              │
                    ▼              ▼              ▼
              hnppublic/       bin/xxx →      EnforceCodeSign
              <name>.org/      原二进制       + BssInstall
              <name>_<ver>/
                                   │
                                    ▼  管道回写 result
                               NativeInstallHnp 返回
                                    │
                                    ▼  应用孵化时 appspawn 收到 spawn 请求
                               ProcessSpawnReqMsg
                                    │  IsDeveloperModeOpen() && IsSupportRunHnp()
                                    ▼  设置 APP_FLAGS_DEVELOPER_MODE
                               沙箱模块 HandleFlagsPoint
                                    │  bind mount 公有/私有 hnp 目录进沙箱
                                    ▼  注入 HNP_PUBLIC_HOME / HNP_PRIVATE_HOME / PATH 环境变量
                               fork 子进程 → 应用沙箱就绪
                                    │
                                    ▼  execv("/data/service/hnp/bin/xxx") 或 execvp("xxx")
                               运行 Native 二进制
```

## 13 关键文件速查

| 关注点 | 文件 | 关键函数 |
|--------|------|----------|
| 命令分发 | `hnp_main.c` | `main` → `HnpCmdCheck` → `HnpCmdInstall`/`HnpCmdUnInstall` |
| 命令分发 | `hnpcli_main.c` | `main` → `HnpCmdCheck` → `HnpCmdPack` |
| 安装主逻辑 | `installer/hnp_installer.c` | `HnpInstallPre` → `HapReadAndInstall` → `HnpReadAndInstall` |
| 卸载主逻辑 | `installer/hnp_installer.c` | `HnpUnInstall` → `HnpNativeUnInstall` |
| hnp.json 解析 | `base/hnp_json.c` | `ParseHnpCfgFile` → `ParseJsonStreamToHnpCfgInfo` |
| 安装信息管理 | `base/hnp_json.c` | `HnpInstallInfoJsonWrite` / `HnpPackageInfoGet` / `CanRecovery` |
| 压缩/解压 | `base/hnp_zip.c` | `HnpZip` / `HnpUnZip` / `HnpCfgGetFromZip` |
| ELF 识别 | `base/hnp_zip.c` | `HnpELFFileCheck` |
| 软链接 | `base/hnp_sal.c` | `HnpSymlink` / `CheckSymlink` / `HnpProcessRunCheck` |
| API 接口 | `interfaces/.../hnp_api.c` | `NativeInstallHnp` / `NativeUnInstallHnp` / `StartHnpProcess` |
| 核心定义 | `base/hnp_base.h` | 数据结构、错误码、路径常量、日志宏 |
| 沙箱挂载触发 | `standard/appspawn_service.c` | `ProcessSpawnReqMsg` → `IsSupportRunHnp` → 设置 `APP_FLAGS_DEVELOPER_MODE` |
| 沙箱挂载执行 | `modules/sandbox/normal/sandbox_core.cpp` | `HandleFlagsPoint` → `DoAllMntPointsMount`（bind mount HNP 目录） |
| 标志映射 | `modules/sandbox/normal/sandbox_common.cpp` | `ConvertFlagStr`（`"DEVELOPER_MODE"` → `APP_FLAGS_DEVELOPER_MODE`） |
| 挂载点配置 | `appdata-sandbox.json` / `appdata-sandbox-app.json` | `DEVELOPER_MODE` 组定义公有/私有 hnp 的 src/sandbox 路径 |
| 环境变量注入 | `util/src/appspawn_utils.c` | `COMMON_ENV`：`HNP_PUBLIC_HOME` / `HNP_PRIVATE_HOME` / `PATH` |
