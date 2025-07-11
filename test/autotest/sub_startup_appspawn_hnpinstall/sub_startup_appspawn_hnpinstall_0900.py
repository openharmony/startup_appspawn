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

from devicetest.core.test_case import TestCase, Step, CheckPoint
from devicetest.core.test_case import Step, TestCase
from hypium.model import UiParam, WindowFilter
import os
from aw import Common


class SubStartupAppspawnHnpinstall0900(TestCase):

    def __init__(self, controllers):
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
        Step('设置屏幕常亮')
        self.driver.Screen.enable_stay_awake()

    def test_step1(self):
        Step("步骤1：安装测试应用")
        path = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        hnp = os.path.abspath(os.path.join(
            os.path.join(path, "testFile"), 'SUB_STARTUP_APPSPAWN_NATIVE/entry-default-hnpinsall.hap'))
        isHap = self.driver.has_app("com.example.hnp_test")
        if (isHap):
            self.driver.uninstall_app("com.example.hnp_test")
        else:
            pass
        self.driver.install_app(hnp)
        Step("是否刷root包")
        isroot = self.driver.shell("ls")
        if ("Permission denied" in isroot):
            Step("步骤2：执行公有目录二进制文件")
            th = Common.hnpexecute_async(self.driver, "/data/app/el1/bundle/100/hnppublic/bin/hnpsample 5")
            th.start()
            result3 = self.driver.shell("ps -ef | grep hnpsample | grep -v grep")
            self.driver.Assert.contains(result3, "hnpsample")
            Step("步骤3：读取私有目录列表")
            result2 = self.driver.shell("./data/app/el1/bundle/100/hnp/com.example.hnp_test/node.org/node_1.0/bin/node")
            self.driver.Assert.contains(result2, "inaccessible or not found")
        else:
            Step("步骤2：执行公有目录二进制文件")
            result1 = self.driver.shell("./data/app/el1/bundle/100/hnppublic/hnpsample.org/hnpsample_1.1/bin/hnpsample")
            self.driver.Assert.contains(result1, "start hnp sample")
            self.driver.Assert.contains(result1, "hnp sample end")

    def teardown(self):
        Step("收尾工作:卸载hap")
        self.driver.uninstall_app("com.example.hnp_test")
