# Native包管理开发指导

## 场景介绍

Native包管理功能模块提供了对Native软件的打包、安装、卸载及运行管控报告的能力。

## 接口说明
针对安装卸载功能提供了以下API调用接口。打包功能使用sdk hnpcli命令行执行。

| 接口名                                                       | 描述                                     |
| :----------------------------------------------------------- | :--------------------------------------- |
| [NativeInstallHnp](api_hnp.md#nativeinstallhnp)| 安装Native软件包          |
|[NativeUnInstallHnp]((api_hnp.md#nativeuninstallhnp))| 卸载Native软件包 |


## 开发步骤

**1. 操作前准备：Native软件包**

编译后的Native软件包里通常包含以下内容：

    bin    #可执行二进制存放路径
    cfg    #配置文件存放路径
    lib    #依赖库存放路径
    ...    #其它
支持对可执行二进制进行软链接配置，具体的配置格式如下，需遵循json格式：

    {
        "type":"hnp-config",                #固定标识符“hnp-config”
        "name":"xxx",                       #Native软件名
        "version":"1.1",                    #版本号
        "install":{
            "links":[                       #软链接配置信息
                {
                    "source":"xxxxxx",
                    "target":"xxxxxx"
                }
            ]
        }
    }
注：用户在配置文件中指定了软链接关系则安装时根据用户配置进行软链接设置。如果用户不使用配置文件或者配置文件中没有设置软链接，则安装时默认将软件包内bin目录下的可执行二进制都进行软链接。
样例：

    hnpsample软件包目录：

    hnpsample
    |__bin
        |__hnpsample
    |__cfg
        |__hnpsample.cfg
    |__lib
        |__libhnpsamplelib.z.so
    
    配置文件hnpsample.json：
    {
        "type":"hnp-config",
        "name":"hnpsample",
        "version":"1.1",
        "install":{
            "links":[
                {
                    "source":"/bin/hnpsample",
                    "target":"hnpsample"
                }
            ]
        }
    }

**2. Native软件包打包**

Native软件打包的目的是为了将Native软件打包成hnp文件以便后面上传到应用市场，为支持不同操作系统（linux、windows、mac），当前提供了hnpcli命令集成到sdk中，用户通过hnpcli命令进行打包。
hnpcli打包命令有两种使用方式。
一种是通过传入软件名和版本号进行打包：

    hnpcli pack [待打包路径] [输出路径] -name [Native软件名] -v [版本号]
另一种是通过传入配置文件进行打包：

    hnpcli pack [待打包路径] [输出路径] -cfg [配置文件所在路径]
打包成功后会在输出路径下生成"[Native软件名].hnp"的文件。
样例：

    1. 对hnpsample软件进行打包，使用命令如下：
    hnpcli pack ./hnpsample ./out -name hnpsample -v 1.1
    或者
    hnpcli pack ./hnpsample ./out -cfg hnpsample.json
    2. 命令返回成功，则在out目录下生成hnpsample.hnp文件

**3. Native软件包安装**
Native软件包安装就是将从应用市场下载解压出来的hnp包安装到鸿蒙PC设备上。当前提供接口调用以及hnp命令行两种方式进行安装。
1） hnp命令行安装：

    hnp install [系统用户ID] [hnp包所在路径] [应用软件名] <-f>
该命令执行时会将用户传入的hnp所在路径下的所有hnp进行批量安装。-f选项表示是否开启强制安装，强制安装下如果发现软件已安装则会将已安装软件先卸载再安装新的。安装路径根据用户传入的应用软件名进行区分，应用软件名为0则为公有软件，否则是私有软件。设备上安装后的软件路径如下：

    公有软件：/data/app/el1/bundle/[userId]/hnppublic/
    私有软件：/data/app/el1/bundle/[userId]/hnp/[packageName]/
样例：
    
    # 安装hnpsample.hnp公有软件：
    所在目录./hnp_path/下。安装在系统用户ID 100 下面。
    执行命令：hnp install 100 ./hnp_path 0 -f
    
    执行成功后会在以下路径下生成输出件
    /data/app/el1/bundle/100/hnppublic/hnpsample.org/hnpsample_1.1
    生成的软链接配置关系：
    软链接文件：/data/app/el1/bundle/100/hnppublic/bin/hnpsample 指向 /data/app/el1/bundle/100/hnppublic/hnpsample.org/hnpsample_1.1/bin/hnpsample

    # 安装hnpsample.hnp私有有软件（应用软件为baidu）：
    所在目录./hnp_path/下。安装在系统用户ID 100 下面。
    执行命令：hnp install 100 ./hnp_path baidu -f
    
    执行成功后会在以下路径下生成输出件
    /data/app/el1/bundle/100/hnp/baidu/hnpsample.org/hnpsample_1.1
    生成的软链接配置关系：
    软链接文件：/data/app/el1/bundle/100/hnp/baidu/bin/hnpsample 指向 /data/app/el1/bundle/100/hnp/baidu/hnpsample.org/hnpsample_1.1/bin/hnpsample

2） 接口调用安装
安装接口原型：

    /**
    * Install native software package.
    *
    * @param userId Indicates id of user.
    * @param hnpPath  Indicates the directory path of hnp file.
    * @param packageNmae Indicates name of application software.
    * @param isForce Indicates whether to force install.
    *
    * @return 0:success;other means failure.
    */
    int NativeInstallHnp(const char *userId, const char *hnpPath, const char *packageName, Bool isForce);

样例：

    #include "hnp_api.h"
    ...
    /* 安装公有软件 */
    int ret = NativeInstallHnp(100, "./hnp_path", 0, true);
    ...
    /* 安装私有软件 */
    int ret = NativeInstallHnp(100, "./hnp_path", “baidu”, true);
    ...

    执行成功后输出件和上面命令行的一样

**4. Native软件包卸载**
Native软件包卸载就是将已安装到系统上的Native软件进行卸载。当期望卸载的软件正在运行时，则卸载失败。当前提供接口调用以及命令行两种方式进行卸载。
1） hnp命令行卸载：

    hnp uninstall [系统用户ID] [Native软件名] [软件版本号] [应用软件名]
用户需要传入指定的的软件名以及版本号进行卸载，也要告知卸载哪个系统用户下的软件以及是公有软件还是私有软件。
样例：

    公有软件卸载：
    hnp uninstall 100 hnpsample 1.1 0

    私有软件卸载：
    hnp uninstall 100 hnpsample 1.1 baidu

    卸载之前已经安装的hnpsample 1.1版本的软件。100为安装所在的系统用户ID。
    执行成功观察点：观察以下之前安装的软件目录“hnpsample.org”是否已删除。
    公有软件：
    /data/app/el1/bundle/100/hnppublic/hnpsample.org
    私有软件：
    /data/app/el1/bundle/100/hnp/baidu/hnpsample.org
2） 接口调用卸载
卸载接口原型：

    /**
    * Uninstall native software package.
    *
    * @param userId Indicates id of user.
    * @param hnpName  Indicates the name of native software.
    * @param hnpVersion Indicates the version of native software.
    * @param packageName Indicates the name of application software.
    *
    * @return 0:success;other means failure.
    */
    int NativeUnInstallHnp(const char *userId, const char *hnpName, const char *hnpVersion, const char *packageName);

样例：

    #include "hnp_api.h"
    ...
    /* 卸载公有软件 */
    int ret = NativeUnInstallHnp(100, "hnpsample", “1.1”， 0);
    ...
    /* 卸载私有软件 */
    int ret = NativeUnInstallHnp(100, "hnpsample", “1.1”， “baidu”);
    ...

    执行成功后观察点和上面命令行执行一致。


