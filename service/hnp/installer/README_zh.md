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
    usage:hnp <command> <args> [-u <user id>][-p <hap package name>][-i <hap install path>][-f][-h <hap source path>][-a <system abi>]

    These are common hnp commands used in various situations:

    install: install one hap package
        hnp install <-u [user id]> <-p [hap package name]> <-i [hap install path]> <-f>
        -u    : [required]    user id
        -p    : [required]    hap package name
        -i    : [required]    hap install path
        -s    : [required]    hap source path
        -a    : [required]    system abi
        -f    : [optional]    if provided, the hnp package will be installed forcely, ignoring old versions of the hnp package

    uninstall: uninstall one hap package
        hnp uninstall <-u [user id]> <-p [hap package name]>
        -u    : [required]    user id
        -p    : [required]    hap package name
    for example:

        hnp install -u 1000 -p app_sample -i /data/app_sample/ -s /data/app_hap/ -a bin64 -f
        hnp uninstall -u 1000 -p app_sample
  ```
2） hnp命令行安装：
  ```
    hnp install -u [系统用户ID] -p [hap包名] <-i [hnp安装包所在路径]> <-f>
  ```
  -u [必选] 用户ID

  -p [必选] 指定hap应用软件名。

  -i [必选] 表示hnp安装包所在路径。

  -f [可选] 选项表示是否开启强制安装，强制安装下如果发现软件已安装则会将已安装软件先卸载再安装新的。

  注：-i指向的路径需要满足以下目录格式。Native公私有软件需要通过所在目录名public和private进行区分。
  ```
  hnp安装包路径
  |__public             #公有安装包存放目录
  |____xxx.hnp
  |__private            #私有安装包存放目录
  |____xxx.hnp
  |____subdir           #支持子目录存放
  |______yyy.hnp

  ```
  该命令执行时会遍历-i传入的路径下public和private目录，将所有.hnp文件进行依次安装。

  public目录下的hnp包作为公有软件安装在公有路径。private目录下的hnp包作为应用的私有软件安装在应用的私有路径下面，具体路径如下：
  ```
    公有软件：/data/app/el1/bundle/[userId]/hnppublic/
    私有软件：/data/app/el1/bundle/[userId]/hnp/[packageName]
  ```
  如果公有软件安装成功，则会在hnp_info.json管理文件中记录对应的安装信息。该文件所在路径：
  ```
    公有软件安装信息管理文件：
    /data/service/el1/startup/hnp_info.json
  ```
  hnp_info.json文件记录格式：
  ```
  [
    {
        "hap":"baidu",              #应用软件hap包名
        "hnp":[                     #hnp安装信息
            {
                "name":"python",    #hnp软件名
                "version":"2.7"     #hnp软件版本号
            }
        ]
    },
    ...
]
  ```

  样例：
  ```
    # baidu应用软件的hnp安装包所在目录
    baidu_hnp_path
    |__public
    |____hnpsample.hnp
    |__private
    |____hnpsample.hnp

    # 安装baidu应用下的hnp软件：
    强制安装baidu应用下的hnp软件，安装包所在目录baidu_hnp_path，安装在系统用户ID 100 下面。
    执行命令：hnp install -u 100 -p baidu -i ./baidu_hnp_path -f

    执行成功后会在以下路径下生成输出件：
    hnpsample公有软件：
    /data/app/el1/bundle/100/hnppublic/hnpsample.org/hnpsample_1.1
    生成的软链接配置关系：
    软链接文件：/data/app/el1/bundle/100/hnppublic/bin/hnpsample 指向 /data/app/el1/bundle/100/hnppublic/hnpsample.org/hnpsample_1.1/bin/hnpsample

    hnpsample私有软件：
    /data/app/el1/bundle/100/hnp/baidu/hnpsample.org/hnpsample_1.1
    生成的软链接配置关系：
    软链接文件：/data/app/el1/bundle/100/hnp/baidu/bin/hnpsample 指向 /data/app/el1/bundle/100/hnp/baidu/hnpsample.org/hnpsample_1.1/bin/hnpsample

    hnp_info.json安装管理信息增加以下记录：
    {
        "hap":"baidu",
        "hnp":[
            {
                "name":"hnpsample",
                "version":"1.1"
            }
        ]
    }
  ```
注：

a. 安装时发现已经有同款软件相同版本则跳过安装，返回安装成功。

b. 非强制安装模式下，安装时发现已经有同款软件不同版本会安装失败.

b. 强制安装会将已安装的软件先卸载掉之后再安装当前新的软件。

c. 批量安装应用的hnp软件时如果中间安装出错，则直接退出安装流程返回，之前已安装的软件继续保留。


3） 接口调用安装

  安装接口原型：
  ```
    /**
     * Install native software package.
     *
     * @param userId Indicates id of user.
     * @param hapPath Indicates path of hap.
     * @param hnpRootPath  Indicates the root path of hnp packages
     * @param packageName Indicates the packageName of HAP.
     * @param installOptions Indicates install options.
     *
     * @return 0:success;other means failure.
     */
    int NativeInstallHnp(const char *userId, const char *hapPath, const char *hnpRootPath, const char *packageName, int installOptions);
  ```
样例：
  ```
    #include "hnp_api.h"
    ...
    /* 强制安装baidu应用软件下的hnp软件 */
    int ret = NativeInstallHnp(100, "./sample.hap", "./baidu_hnp_path", "baidu", 0x1);
    ...

    执行成功后输出件和上面命令行的一样
  ```
**2. Native软件包卸载**

  Native软件包卸载就是将已安装到系统上的Native软件进行卸载。当期望卸载的软件正在运行时，则卸载失败。当前提供接口调用以及命令行两种方式进行卸载。

  1） hnp命令行卸载：
  ```
    hnp uninstall -u [系统用户ID] -p [hap包名]
  ```
  该命令根据用户传入的信息对已安装的属于对应hap应用下的所有hnp软件进行卸载。

  -u [必选] 系统用户ID，用于拼接软件的安装路径

  -p [必选] 要卸载hnp包所属的hap包名（应用软件名）

  样例：
  ```
    baidu应用下hnp软件卸载：
    hnp uninstall -u 100 -p baidu

    卸载之前已经安装的baidu应用下所有hnp软件。100为安装所在的系统用户ID。
    执行成功观察点：
    观察点1：之前baidu应用安装的时候分别安装了公有和私有的hnpsample软件，所以本次卸载需要观察以下之前安装的软件目录“hnpsample.org”是否已删除。
    公有软件：
    /data/app/el1/bundle/100/hnppublic/hnpsample.org
    私有软件：
    /data/app/el1/bundle/100/hnp/baidu/hnpsample.org

    观察点2：查看/data/service/el1/startup/hnp_info.json安装管理文件中对应baidu这一hap节点信息是否删除。
  ```
注：

a. 如果公有hnp软件被其它应用所共有，则卸载本应用不会删除hnp公有软件，需等其所属的所有应用都删除了才会删除公有hnp软件。公有hnp软件被哪些应用所共有由hnp_info.json文件管理。

b. 公有hnp软件卸载前会判断该软件是否正在运行，如果正在运行则会卸载失败。私有hnp软件因为其所属应用已经卸载，不存在正在使用的情况，因此私有软件不用校验是否正在运行。

2） 接口调用卸载

  卸载接口原型：
  ```
    /**
     * Uninstall native software package.
     *
     * @param userId Indicates id of user.
     * @param packageName Indicates the packageName of HAP.
     *
     * @return 0:success;other means failure.
     */
    int NativeUnInstallHnp(const char *userId, const char *packageName);
  ```
  样例：
  ```
    #include "hnp_api.h"
    ...
    /* 卸载baidu应用软件下的所有hnp软件 */
    int ret = NativeUnInstallHnp(100, "baidu");
    ...

    执行成功后观察点和上面命令行执行一致。
  ```

  **3. 运行管控**

  native包管理功能运行控制，需要在用户开启“开发者模式”场景下才能使用native包管理的安装卸载软件功能，否则命令会执行失败。在PC设备上打开“开发者模式”的方法如下：
  ```
    点击“设置”按钮——》选择“系统和更新”界面——》选择“开发者选项”——》打开“开发者选项”
  ```
