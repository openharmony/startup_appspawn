# Native软件包开发指南
## 1 简介

大量生产力软件需要在设备上运行Native软件包（例如Python、node、java等Native软件包）。当前提供Native软件包方案支持在生产力应用上安装并运行Native软件包，支持开发者用户或者自动化测试人员运行通过命令行运行Native软件包二进制。

## 2 Native软件包开发

所有Native软件包都需要通过hnpcli工具打包成hnp（OpenHarmony Native Package）文件，通过hap打包工具将hnp包放入生产力应用中安装和运行，Native软件包以Hap形式进行分发使用。总共分为以下几步：

* 将Native软件包源文件打包成hnp包。
* 将hnp包打入hap包。
* 签名hap包。

### 2.1 使用命令行工具开发Native软件包

#### 2.1.1 Native软件包打包成hnp包流程

参考[Native软件打hnp包功能开发指导](https://gitee.com/openharmony/startup_appspawn/blob/master/service/hnp/pack/README_zh.md)将Native软件包文件打包成hnp包。

#### 2.1.2 hnp包打入Hap包流程

**操作步骤：**
1. 下载DevEco Studio软件，创建一个hap工程。

2. Hap工程根目录下新增hnp根目录，在hap打包命令中用--hnp-path指定hnp根目录。hnp根目录下根据设备的操作系统ABI名称（例如arm64-v8a）创建文件夹，文件目录格式如下：

```
HAP工程根目录
|__hnp        #HAP包中新增hnp根目录
|____arm64-v8a        #（ABI）
|______python.hnp
|______sub_dir        #支持子目录
|________test.hnp
```

3. 修改hap工程根目录下entry/src/main/module.json5文件，配置hnp包信息，需要在"module"字段下增加以下字段。"package"字段指定hnp包在ABI文件夹下相对路径，"type"指定hnp包类型，包含公有（"public"）和私有（"private"）类型。点击DevEco Studio软件菜单栏中"Build"按钮下的"Build Hap(s)/APP(s)"按钮，点击"Build Hap(s)"，编译Hap工程。

```
"hnpPackages":[
  {
    "package": "python.hnp",
    "type": "public"
  },
  {
    "package": "sub_dir/test.hnp",
    "type": "private"
  }
]
```

4. 参考[打包拆包工具使用说明](https://gitee.com/openharmony/developtools_packing_tool)中“hap包模式打包指令”章节。
5. 执行hap包模式打包指令获取未签名的hap包。

**规格：**
1. 公有hnp包安装后应用进程沙箱路径为/data/service/hnp/xxx.org/xxx_yyy，私有hnp包安装后应用进程沙箱路径为/data/app/bundleName/xxx.org/xxx_yyy，其中xxx值hnp包hnp.json文件"name"字段，yyy值为hnp包中"version"字段。
2. 公有hnp包根路径的环境变量HNP_PUBLIC_HOME=/data/service/hnp，私有hnp包根路径的环境变量HNP_PRIVATE_HOME=/data/app。HNP_PRIVATE_HOME环境变量排序在HNP_PUBLIC_HOME前面，意味着如果存在同名二进制分别在公有hnp路径下和私有hnp路径下，优先执行私有hnp路径下二进制。
3. 公有hnp包可以被所有应用访问，私有hnp包只允许被安装该hnp包的hap应用访问。
4. 卸载hap应用会同步卸载该hap应用安装的所有hnp包，如果该hnp包中二进制正在被其他应用使用，则会导致hap应用卸载失败。
5. Hap应用A和B安装相同公有hnp包（hnp.json文件中"name"和"version"字段相同）。后安装的应用会跳过该hnp包的安装。仅当hap应用A和B都被卸载时，该公有hnp包会被卸载。
6. Hap应用A和B安装先后安装同名公有hnp包（hnp.json文件中"name"相同，但是"version"字段不同），则会尝试先卸载Hap应用A的hnp包，再安装Hap应用B的hnp包，如果Hap应用A的hnp包卸载失败会导致Hap应用B安装失败。

#### 2.1.3 hap包签名流程

**操作步骤：**
1. 参考[应用/服务签名](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides-V5/ide-signing-0000001587684945-V5#section297715173233)中“手动签名”章节完成手动配置签名信息。
2. 下载[hap-sign-tool.jar](https://gitee.com/openharmony/developtools_hapsigner/blob/master/dist/hap-sign-tool.jar)文件。
3. 参考[应用包签名工具](https://gitee.com/openharmony/developtools_hapsigner/blob/master/README_ZH.md)中“使用说明”章节，使用命令签名hap包，命令中的参数来源于第一步中的签名配置信息。
4. 执行签名命令获取签名后的hap包。

### 2.2 使用IDE工具开发Native软件包

暂不支持，待开发。

## 3 Native软件包的使用方法
### 3.1 在hap应用中访问Native二进制
以c++语言为例，可以在hap应用代码中通过system、execv等函数执行二进制。默认公有hnp包软链接路径为/data/service/hnp/bin，默认私有hnp包软链接路径为/data/app/bin，默认软链接路径已加入环境变量中。
### 3.2 hdc shell执行方法

**操作步骤：**
1. 从应用市场下载Native软件包hap应用并安装。
2. 通过数据线连接设备，执行hdc shell访问设备。
3. 公有hnp包安装后的物理路径为/data/app/el1/bundle/userid/hnppublic，私有hnp包安装后的物理路径为/data/app/el1/bundle/userid/hnp。可以进入这些目录下找到安装的Native包文件目录，执行相关二进制。
