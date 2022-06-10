# appspawn应用孵化器组件

## 简介

应用孵化器，负责接受应用程序框架的命令孵化应用进程，设置其对应权限，并调用应用程序框架的入口。

其主要的结构及流程如下图所示：

![](figures/appspawn.png)

## 目录
```
base/startup/appspawn_standard
├─adapter                # 适配外部依赖
│  └─sysevent            # 系统事件管理
├─common                 # 通用代码
├─etc
├─figures
├─interfaces             # 应用孵化器组件头文件以及对外接口
│  └─innerkits
│      ├─client          # 应用孵化器客户端源码
│      └─include         # 应用孵化器头文件
├─lite                   # 小型系统应用孵化器源码
├─standard               # 标准系统应用孵化器源码
├─test                   # 应用孵化器组件测试源码
└─util
    ├─include            # 应用孵化器工具类头文件
    └─src                # 应用孵化器工具类源码
```

## 使用说明

### 小型系统

  appspawn被init启动后，向IPC框架注册服务名称，之后等待接收进程间消息，根据消息解析结果启动应用服务并赋予其对应权限。

  appspawn注册的服务名称为“appspawn”，在安全子系统限制规则下，目前仅Ability Manager Service有权限可以向appspawn发送的进程间消息。

  appspawn接收的消息为json格式，如下所示：
  ```
  "{"bundleName":"testvalid1","identityID":"1234","uID":1000,"gID":1000,"capability":[0]}"
  ```

  **表 1** 小型系统字段说明
  | 字段名 | 说明 |
  | -------- | -------- |
  | bundleName | 即将启动的应用程序包名，长度≥7字节，≤127字节。 |
  | identityID | AMS为新进程生成的标识符，由appspawn透传给新进程，长度≥1字节，≤24字节。 |
  | uID | 即将启动的应用服务进程的uID。 |
  | gID | 即将启动的应用服务进程的gID。 |
  | capability | 即将启动的应用服务进程所需的capability权限，数量≤10个。 |

### 标准系统
  appspawn注册的服务名称为“appspawn”。appspawn 通过监听本地socket，接收来自客户端的请求消息。

  **表 2**  标准系统字段说明
  | 字段名 | 说明 |
  | -------- | -------- |
  | processName | 即将启动的应用服务进程名，最大256字节。 |
  | bundleName | 即将启动的应用程序包名，最大256字节。 |
  | soPath | 即应用程序指定的动态库的路径，最大256字节。 |
  | uid | 即将启动的应用进程的uid。 |
  | gid | 即将启动的应用进程的gid。 |
  | gidTable | 即将启动的应用进程组信息，长度由gidCount指定，最大支持64个进程组，必须为正值。 |
  | gidCount | 即将启动的应用进程组个数。 |
  | accessTokenId | 即应用进程权限控制的token id。 |
  | apl | 即应用进程权限控制的apl，最大32字节. |
  | renderCmd | 即图形图像渲染命令， 最大1024字节。 |
  | flags | 即冷启动标志位。 |
  | pid | 即渲染进程pid，查询渲染进程退出状态。 |
  | AppOperateType | 即App操作类型，0： 默认状态； 1：获取渲染终止状态。 |

## 限制与约束
仅支持小型系统和标准系统。

## 相关仓

[startup\_syspara\_lite](https://gitee.com/openharmony/startup_syspara_lite/blob/master/README_zh.md)

**[startup_appspawn](https://gitee.com/openharmony/startup_appspawn/blob/master/README_zh.md)**

[startup\_bootstrap\_lite](https://gitee.com/openharmony/startup_bootstrap_lite/blob/master/README_zh.md)

[startup\_init\_lite](https://gitee.com/openharmony/startup_init_lite/blob/master/README_zh.md)