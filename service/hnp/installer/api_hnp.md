# hnp_api.h


## 概述

提供支持Native软件的安装和卸载功能。

### 文件

| 名称 | 描述 |
| -------- | -------- |
| hnp_api.h | 提供支持Native软件的安装和卸载的函数。<br/>**引用文件**：&lt;hnp/include/hnp_api.h&gt;<br/>**库**：libhnpapi.z.so |


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
int NativeInstallHnp(int userId, const char *packages[], int count, const char *installPath, int installOptions);
```

**描述**

  安装Native软件到设备中。

  参数：
  
  userId：用户ID；

  packages：字符串数组，待安装hnp文件所在路径；

  count：待安装hnp文件个数；

  installPath：安装路径。为0或为NULL则表示安装到公有路径。

  installOptions：安装选项。其中每一位对应的选项信息枚举如下。
  ```
    typedef enum {
        OPTION_INDEX_FORCE = 0,  /* installed forcely */
        OPTION_INDEX_BUTT
    } HnpInstallOptionIndex;
  ```

**返回：**

安装成功返回0；失败返回错误码

### NativeUnInstallHnp

```
int NativeUnInstallHnp(int userId, const char *hnpName, const char *hnpVersion, const char *installPath);
```

**描述**

  卸载设备中已安装的Native软件。

  参数：

  userId：用户ID；

  hnpName：软件名；

  hnpVersion：版本号；

  installPath：安装路径，为0或为NULL则表示公有路径。

**返回：**

卸载成功返回0；失败返回错误码
