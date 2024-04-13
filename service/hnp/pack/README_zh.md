# Native软件打包功能开发指导

## 场景介绍

Native包管理功能模块提供了对Native软件的打包、安装、卸载及运行管控的能力，当前文档主要对打包功能进行描述。

## 接口说明

  打包功能通过hnpcli命令行执行，不涉及对外接口。


## 开发步骤

**1. 操作前准备：Native软件包**

  编译后的Native软件包里通常包含以下内容：
  ```
    bin         #可执行二进制存放路径
    cfg         #配置文件存放路径
    lib         #依赖库存放路径
    hnp.json    #hnp配置文件
    ...         #其它
  ```
  其中hnp.json为hnp打包配置文件，需要放在软件包的外层目录，只能命名为hnp.json，内容要满足json文件的格式要求，该文件支持对可执行二进制进行软链接配置，具体的配置格式如下：
  ```
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
  ```
  注：
  1. 用户在配置文件中指定了软链接关系则安装时根据用户配置进行软链接设置。如果用户不使用配置文件或者配置文件中没有设置软链接，则安装时默认将软件包内bin目录下的可执行二进制都进行软链接。
  2. 为了保证bin目录里面的二进制执行过程中能自动加载lib下的库文件，则需要在编译二进制时增加以下rpath链接选项。HNP_PRIVATE_HOME表示私有安装路径，HNP_PUBLIC_HOME表示公有安装路径

  ```
    ldflags = [
        "-Wl,-rpath=${HNP_PRIVATE_HOME}:${HNP_PUBLIC_HOME}",
        "-Wl,--disable-new-dtags",
    ]
  ```

  样例：
  ```
    hnpsample软件包目录：

    hnpsample
    |__bin
        |__hnpsample
    |__cfg
        |__hnpsample.cfg
    |__lib
        |__libhnpsamplelib.z.so
    |__hnp.json
    
    配置文件hnp.json内容如下：
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
    注：
    上述软链接关系表示为：作为target的hnpsample文件是生成的软链接文件，它是由源文件/bin/hnpsample软链接生成的。
  ```
**2. Native软件包打包**

  Native软件打包的目的是为了将Native软件打包成hnp文件以便后面上传到应用市场，为支持不同操作系统（linux、windows、mac、ohos），当前提供了hnpcli命令集成到sdk中，用户通过hnpcli命令进行打包。

  hnpcli命令提供帮助信息：
  ```
    帮助命令：hnpcli -h 或者 hnpcli help
    输出内容：
    usage:hnpcli <command> <args> [-i <software package dir>][-o <hnp output path>][-n <native package name>][-v <native package version>]

    These are common hnpcli commands used in various situations:

    pack:    packet native software package to .hnp file
         hnpcli pack <-i [source path]> <-o [dst path]> <-n [software name]> <-v [software version]>
         -i    : [required]    input path of software package dir
         -o    : [optional]    output path of hnp file. if not set then ouput to current directory
         -n    : [optional]    software name. if not hnp.json in input dir then must set
         -v    : [optional]    software version. if not hnp.json in input dir then must set

    for example:

    hnpcli pack -i /usr1/native_sample -o /usr1/output -n native_sample -v 1.1
  ```
  hnpcli打包命令有两种使用方式。
  
  一种是软件包里有hnp.json的打包命令：
  ```
    hnpcli pack -i [待打包路径] < -o [输出路径] > 
    注：
    1. 如果没有指定-o，则输出路径为当前目录。
    2. 打包路径下有hnp.json配置文件，使用配置文件名里的软件名和版本号作为打包参数。
  ```
  另一种是软件包里没有hnp.json的打包方式：
  ```
    hnpcli pack -i [待打包路径] < -o [输出路径] > -n [软件名] -v [版本号]
    注：
    1. 打包路径下没有hnp.json配置文件，则需要用户传入-n和-v参数，否则打包失败（打包软件会根据入参在压缩文件根目录中主动生成hnp.json）。
  ```
  打包成功后会在输出路径下生成"[Native软件名].hnp"的文件。

  样例：
  ```
    1. 对hnpsample软件进行打包，由于hnpsample目录下存在hnp.json文件，不需要指定软件名和版本号。因此命令如下：
    hnpcli pack -i ./hnpsample -o ./out
    2. 命令返回成功，则在out目录下生成hnpsample.hnp文件
  ```

**3. 上架应用市场**

  Native软件包通过HAP包上架应用市场发布，具体应用软件上架流程请参考[上架指南](https://developer.huawei.com/consumer/cn/fa/)

