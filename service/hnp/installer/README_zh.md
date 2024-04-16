# Native包管理安装卸载开发指导

## 场景介绍

  Native包管理功能模块提供了对Native软件的打包、安装、卸载及运行管控的能力。本文档主要针对安装、卸载及运行管控功能进行描述。

## 接口说明

  针对安装卸载功能提供了以下API调用接口

| 接口名                                                       | 描述                                     |
| :----------------------------------------------------------- | :--------------------------------------- |
| [NativeInstallHnp](api_hnp.md#nativeinstallhnp)| 安装Native软件包          |
|[NativeUnInstallHnp](api_hnp.md#nativeuninstallhnp)| 卸载Native软件包 |


## 开发步骤

**1. Native软件包安装**

  Native软件包安装就是将从应用市场下载解压出来的hnp包安装到鸿蒙PC设备上。当前提供接口调用以及hnp命令行两种方式进行安装。

  1） hnp帮助命令 hnp -h
  ```
    usage:hnp <command> <args> [-u <user id>][-p <software package path>][-i <private path>][-f]

    These are common hnp commands used in various situations:

    install:   install one or more native hnp packages
               hnp install <-u [user id]> <-p [hnp package path]> <-i [hnp install path]> <-f>
               -u    : [required]    user id
               -p    : [required]    path of hnp package to be installed, multiple packages are supported
               -i    : [optional]    hnp install path; if not provided, it will be installed to public hnp path
               -f    : [optional]    if provided, the hnp package will be installed forcely, ignoring old versions of the hnp package

    uninstall: uninstall one hnp package
               hnp uninstall <-u [user id]> <-n [hnp package name]> <-v [hnp package version]> <-i [hnp package uninstall path]>
               -u    : [required]    user id
               -n    : [required]    hnp package name
               -v    : [required]    hnp package version
               -i    : [optional]    the path for uninstalling the hnp package; if not provided, it will install from the default public hnp path

    for example:

        hnp install -u 1000 -p /usr1/hnp/sample.hnp -p /usr1/hnp/sample2.hnp -i /data/app/el1/bundle/ -f
        hnp uninstall -u 1000 -n native_sample -v 1.1 -i /data/app/el1/bundle/
  ```
2） hnp命令行安装：
  ```
    hnp install -u [系统用户ID] -p [hnp包所在路径] <-i [安装路径]> <-f>
  ```
  该命令执行时会将对用户传入的多个hnp包进行安装。

  -p [必选] 指定待安装的hnp包存放路径，支持重复设置，则表示安装多个包。

  -i [可选] 表示指定安装路径，如果没有指定则安装到公有路径。

  -f [可选] 选项表示是否开启强制安装，强制安装下如果发现软件已安装则会将已安装软件先卸载再安装新的。

  安装路径根据用户是否-i传入路径进行区分，没有传入-i则表示公有软件路径，否则是安装到用户指定的路径。设备上安装后的软件路径如下：
  ```
    公有软件：/data/app/el1/bundle/[userId]/hnppublic/
    私有软件：用户指定的软件路径
  ```
  样例：
  ```
    # 安装hnpsample.hnp公有软件：
    所在目录./hnp_path/下。安装在系统用户ID 100 下面。
    执行命令：hnp install -u 100 -p ./hnp_path/hnpsample.hnp -f
    
    执行成功后会在以下路径下生成输出件
    /data/app/el1/bundle/100/hnppublic/hnpsample.org/hnpsample_1.1
    生成的软链接配置关系：
    软链接文件：/data/app/el1/bundle/100/hnppublic/bin/hnpsample 指向 /data/app/el1/bundle/100/hnppublic/hnpsample.org/hnpsample_1.1/bin/hnpsample

    # 安装hnpsample.hnp私有有软件（应用软件为baidu）：
    所在目录./hnp_path/下。安装在系统用户ID 100 下面。
    执行命令：hnp install -u 100 -p ./hnp_path/hnpsample.hnp -i /data/app/el1/bundle/100/hnp/baidu -f
    
    执行成功后会在以下路径下生成输出件
    /data/app/el1/bundle/100/hnp/baidu/hnpsample.org/hnpsample_1.1
    生成的软链接配置关系：
    软链接文件：/data/app/el1/bundle/100/hnp/baidu/bin/hnpsample 指向 /data/app/el1/bundle/100/hnp/baidu/hnpsample.org/hnpsample_1.1/bin/hnpsample
  ```

3） 接口调用安装

  安装接口原型：
  ```
    /**
    * Install native software package.
    *
    * @param userId Indicates id of user.
    * @param packages  Indicates the path of hnp file.
    * @param count  Indicates num of hnp file.
    * @param installPath  Indicates the path for private hnp file.
    * @param installOptions Indicates install options.
    *
    * @return 0:success;other means failure.
    */
    int NativeInstallHnp(const char *userId, const char *packages[], int count, const char *installPath, int installOptions);
  ```
样例：
  ```
    #include "hnp_api.h"
    ...
    /* 安装公有软件 */
    int ret = NativeInstallHnp(100, "./hnp_path/hnpsample.hnp", 0, 0x1);
    ...
    /* 安装私有软件 */
    int ret = NativeInstallHnp(100, "./hnp_path", "/data/app/el1/bundle/100/hnp/baidu", 0x1);
    ...

    执行成功后输出件和上面命令行的一样
  ```
**2. Native软件包卸载**

  Native软件包卸载就是将已安装到系统上的Native软件进行卸载。当期望卸载的软件正在运行时，则卸载失败。当前提供接口调用以及命令行两种方式进行卸载。

  1） hnp命令行卸载：
  ```
    hnp uninstall -u [系统用户ID] -n [Native软件名] -v [软件版本号] <-i [安装路径]>
  ```
  该命令根据用户传入的信息对已安装的native软件进行卸载。

  -u [必选] 系统用户ID，用于拼接软件的安装路径

  -n [必选] 要卸载的native软件名

  -v [必选] native软件版本号

  -i [可选] 软件的安装路径。如果用户没有设置则默认为native软件公有安装路径

  样例：
  ```
    公有软件卸载：
    hnp uninstall -u 100 -n hnpsample -v 1.1

    私有软件卸载：
    hnp uninstall -u 100 -n hnpsample -v 1.1 -i /data/app/el1/bundle/100/hnp/baidu

    卸载之前已经安装的hnpsample 1.1版本的软件。100为安装所在的系统用户ID。
    执行成功观察点：观察以下之前安装的软件目录“hnpsample.org”是否已删除。
    公有软件：
    /data/app/el1/bundle/100/hnppublic/hnpsample.org
    私有软件：
    /data/app/el1/bundle/100/hnp/baidu/hnpsample.org
  ```
2） 接口调用卸载

  卸载接口原型：
  ```
    /**
    * Uninstall native software package.
    *
    * @param userId Indicates id of user.
    * @param hnpName  Indicates the name of native software.
    * @param hnpVersion Indicates the version of native software.
    * @param installPath  Indicates the path for private hnp file.
    *
    * @return 0:success;other means failure.
    */
    int NativeUnInstallHnp(const char *userId, const char *hnpName, const char *hnpVersion, const char *installPath);
  ```
  样例：
  ```
    #include "hnp_api.h"
    ...
    /* 卸载公有软件 */
    int ret = NativeUnInstallHnp(100, "hnpsample", "1.1"， 0);
    ...
    /* 卸载私有软件 */
    int ret = NativeUnInstallHnp(100, "hnpsample", "1.1"， "/data/app/el1/bundle/100/hnp/baidu");
    ...

    执行成功后观察点和上面命令行执行一致。
  ```
  
  **3. 运行管控**

  native包管理功能运行控制，需要在用户开启“开发者模式”场景下才能使用native包管理的安装卸载软件功能，否则命令会执行失败。在PC设备上打开“开发者模式”的方法如下：
  ```
    点击“设置”按钮——》选择“系统和更新”界面——》选择“开发者选项”——》打开“USB调试”

