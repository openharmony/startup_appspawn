#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (c) 2025 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import time
from devicetest.core.test_case import TestCase, Step, CheckPoint
from hypium import UiDriver
from hypium.action.os_hypium.device_logger import DeviceLogger
from hypium.model import UiParam, WindowFilter


class SubStartupAppspawnAppclone0200(TestCase):
    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.tag, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        self.driver = UiDriver(self.device1)
        Step("预置工作:唤醒屏幕.................")
        self.driver.enable_auto_wakeup(self.device1)
        Step("预置工作:滑动解锁.................")
        self.driver.swipe(UiParam.UP, side=UiParam.BOTTOM)
        Step('设置屏幕常亮')
        self.driver.Screen.enable_stay_awake()

    def test_step1(self):
        Step("步骤1：安装测试hap包")
        ishap = self.driver.has_app("com.ohos.mytest")
        if(ishap):
            self.driver.uninstall_app("com.ohos.mytest")
        path = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        hap = os.path.abspath(
            os.path.join(os.path.join(path, "testFile"),
                         'sub_startup_appspawn_appclone/entry-signed-release.hap'))
        self.driver.install_app(hap)
        Step("步骤2：打开测试应用")
        self.driver.start_app("com.ohos.mytest", "EntryAbility")
        time.sleep(2)
        self.driver.check_window_exist(WindowFilter().bundle_name("com.ohos.mytest"))
        Step("步骤3：点击安装克隆应用")
        self.driver.touch(BY.text("安装克隆应用"))
        self.driver.touch(BY.text("startAbilityByAppIndex"))
        Step("步骤4：分身应用的源目录新建123.txt")
        self.driver.shell("touch ../data/app/el2/100/database/com.ohos.mytest/abc.txt")
        Step("步骤5：主应用沙盒路径是否存在abc.txt")
        cpid = self.driver.System.get_pid("com.ohos.mytest")
        result1 = self.driver.shell("ls ../proc/%d/root/data/storage/el2/database" % cpid)
        Step("步骤6：预期结果校验")
        self.driver.Assert.contains(result1, "abc.txt")
        Step("步骤7：分身应用沙盒路径是否存在abc.txt")
        pid = self.driver.System.get_pid("com.ohos.mytest1")
        result2 = self.driver.shell("ls ../proc/%d/root/data/storage/el2/database" % pid)
        Step("步骤6：预期结果校验")
        if ("abc.txt" in result2):
            raise AssertionError()
        else:
            pass

    def teardown(self):
        Step("收尾工作:删除hap")
        self.driver.uninstall_app("com.ohos.mytest")
