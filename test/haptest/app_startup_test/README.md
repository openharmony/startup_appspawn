# Web和应用的跳转与拉起

## 介绍

本示例基于应用拉起相关能力，实现了Web页面与ArkTS页面的相互拉起、从Web页面拉起各类应用、以及拉起应用市场详情页等场景。

## 效果预览
手机运行效果图如下：

<img src="screenshots/device/phone.gif" />


## 工程目录结构

```
├──entry/src/main/ets                                   // 代码区
│  ├──common
│  |  ├──Logger.ets                                     // 日志工具类
│  |  └──Constants.ets                                  // 常量
│  ├──entryability
│  |  └──EntryAbility.ets
│  ├──entrybackupability
│  |  └──EntryBackupAbility.ets
│  └──pages
│     ├──Index.ets                                      // 入口界面
│     └──OriginPage.ets                                 // 原生页面
└──entry/src/main/resources                             // 应用资源目录
```