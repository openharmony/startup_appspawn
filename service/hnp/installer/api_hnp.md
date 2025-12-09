# hnp_api.h

## 概述

提供支持Native软件的安装和卸载功能。

### 文件

| 名称      | 描述                                                                                                                                  |
| --------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| hnp_api.h | 提供支持Native软件的安装和卸载的函数。<br/>**引用文件**：&lt;hnp_api.h&gt;<br/>**库**：libhnpapi.z.so |

### 结构体定义

NA.

### 函数

| 名称                                   | 描述                         |
| -------------------------------------- | ---------------------------- |
| [NativeInstallHnp](#nativeinstallhnp)     | 安装Native软件到设备中       |
| [NativeUnInstallHnp](#nativeuninstallhnp) | 卸载设备中已安装的Native软件 |

## 函数说明

### NativeInstallHnp

```
int NativeInstallHnp(const char *userId, const char *hnpRootPath, const HapInfo *hapInfo, int installOptions);
```

**描述**

  安装Native软件到设备中。

  参数：

  userId：用户ID。

  hapPath：hap包所在路径。用于签名校验。

  hnpRootPath：hnp安装包存放路径。

  hapInfo：hap应用软件信息，结构如下。
  ```
    #define PACK_NAME_LENTH 256
    #define HAP_PATH_LENTH 256
    #define ABI_LENTH 128

    typedef struct HapInfo {
        char packageName[PACK_NAME_LENTH]; // 包名
        char hapPath[HAP_PATH_LENTH];      // hap文件路径
        char abi[ABI_LENTH];               // abi路径
        char appIdentifier[APP_IDENTIFIER_LEN];     // 应用唯一标识appIdentifier
        int count;                                  // 独立签名hnp包数量
        char **independentSignHnpPaths;             // 独立签名hnp包相对hapPath路径
    } HapInfo;
  ```

  installOptions：安装选项。其中每一位对应的选项信息枚举如下。

```
    typedef enum {
        OPTION_INDEX_FORCE = 0,  /* installed forcely */
        OPTION_INDEX_BUTT
    } HnpInstallOptionIndex;
```

**返回：**
 
安装成功返回0；失败返回错误码。

### NativeUnInstallHnp

```
int NativeUnInstallHnp(const char *userId, const char *packageName);
```

**描述**

  卸载设备中已安装的Native软件。

  参数：

  userId：用户ID。

  packageName：hap应用软件包名。

**返回：**

卸载成功返回0；失败返回错误码。
