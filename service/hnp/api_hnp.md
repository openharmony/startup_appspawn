# hnp_api.h


## 概述

提供支持Native软件的安装和卸载功能。

### 文件

| 名称 | 描述 |
| -------- | -------- |
| hnp_api.h | 提供支持Native软件的安装和卸载的函数。<br/>**引用文件**：<br/>**库**：libhnpapi.z.so |


### 结构体定义
NA

### 函数

| 名称 | 描述 |
| -------- | -------- |
| [NativeInstallHnp](#nativeinstallhnp) | 安装Native软件到设备中 |
| [NativeUnInstallHnp](#nativeuninstallhnp)| 卸载设备中已安装的Native软件 |



## 函数说明


### NativeInstallHnp

```
int NativeInstallHnp(const char *userId, const char *hnpPath, const char *packageName, Bool isForce);
```

**描述**

安装Native软件到设备中。
参数：
userId：用户ID；
hnpPath：hnp包所在路径；
packageName：应用软件名
isForce：是否强制安装

**返回：**

安装成功返回0；失败返回错误码

### NativeUnInstallHnp

```
int NativeUnInstallHnp(const char *userId, const char *hnpName, const char *hnpVersion, const char *packageName);
```

**描述**

卸载设备中已安装的Native软件。
参数：
userId：用户ID；
hnpName：软件名；
hnpVersion：版本号；
packageName：应用软件名;

**返回：**

卸载成功返回0；失败返回错误码
