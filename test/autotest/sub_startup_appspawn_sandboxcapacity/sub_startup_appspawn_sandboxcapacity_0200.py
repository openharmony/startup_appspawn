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

from devicetest.core.test_case import CheckPoint
from devicetest.core.test_case import Step, TestCase
from hypium import *
from hypium.model import UiParam, WindowFilter
import time


class sub_startup_appspawn_sandboxcapacity_0200(TestCase):

    def __init__(self, controllers):
        self.TAG = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        self.driver = UiDriver(self.device1)
        Step("预置工作:检测屏幕是否亮.................")
        self.driver.enable_auto_wakeup(self.device1)
        Step("预置工作:滑动解锁.................")
        self.driver.swipe(UiParam.UP, side=UiParam.BOTTOM)
        Step('预置工作:设置屏幕常亮')
        self.driver.Screen.enable_stay_awake()

    def test_step1(self):
        Step("步骤1：打开设置应用")
        self.driver.AppManager.start_app("com.example.settings")
        time.sleep(2)
        self.driver.check_window_exist(WindowFilter().bundle_name("com.example.settings"))
        CheckPoint("检测点1:设置应用打开成功")
        Step("步骤2：获取设置应用pid")
        pid = self.driver.System.get_pid("com.example.settings")
        Step("步骤3：查看沙箱路径目录")
        self.driver.shell("echo 0 > /proc/sys/kernel/dec/dec_mode")
        result = self.driver.shell("ls /proc/%d/root/data" % pid).split()
        Step(result)
        Step("步骤4：预期结果校验")
        path_lst = ["storage", "bundles", "data", "global", "utd", "service"]
        for path in path_lst:
            self.driver.Assert.contains(result, path)


    def teardown(self):
        Step("收尾工作:关闭设置应用")
        self.driver.stop_app("com.example.settings")
        self.driver.shell("echo 1 > /proc/sys/kernel/dec/dec_mode")
