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

import time
from devicetest.core.test_case import TestCase, Step, CheckPoint
from devicetest.core.test_case import Step, TestCase
from hypium.action.os_hypium.device_logger import DeviceLogger
from hypium.model import UiParam, WindowFilter
import os


class SubStartupAppspawnNativedmscheck0400(TestCase):

    def __init__(self, controllers):
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
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
        Step('开启DEBUG日志')
        self.driver.shell("hilog -b I -D 0xC02C11")
        self.driver.shell("hilog -b DEBUG -T APPSPAWN")
        self.driver.shell("hilog -b D -D 0xDC02C11")
        self.driver.shell("hilog -G 16M")
        host.shell('hdc shell "hilog -r"')

    def test_step1(self):
        Step("步骤1：安装私有路径hap包")
        path = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        hap1 = os.path.abspath(
            os.path.join(os.path.join(path, "testFile"),
                         'SUB_STARTUP_APPSPAWN_NATIVE/entry-default-signedArm64-test.hap'))
        hap2 = os.path.abspath(
            os.path.join(os.path.join(path, "testFile"),
                         'SUB_STARTUP_APPSPAWN_NATIVE/entry-default-signed-hnpselinux.hap'))
        self.driver.install_app(hap1)
        self.driver.install_app(hap2)
        Step("步骤2：过滤日志")
        device_logger = DeviceLogger(self.driver)
        device_logger.set_filter_string("MY_TAG")
        Step("步骤3：开始抓取日志")
        device_logger.start_log(path + '\\testFile\\log\\%s.log' % (self.TAG))
        Step("步骤4：打开测试应用")
        self.driver.start_app("com.example.hnpselinuxtestapplication", "EntryAbility")
        time.sleep(60)
        Step("步骤4：关闭日志")
        device_logger.stop_log()
        time.sleep(4)
        Step("识别设备型号...............................")
        device = self.driver.shell("param get const.product.model")
        device = device.replace("\n", "").replace(" ", "")
        device = str(device)
        if ("HYM" in device or "HAD" in device):
            device_logger.check_log("cfg file content:")
            device_logger.check_log("[This is hnp sample config file.].")
            device_logger.check_log("cfg file end")
            device_logger.check_log('Open public cfg file to write failed, errno=[13]')
        else:
            device_logger.check_log('Open public cfg file to read failed, errno=[2]')
            device_logger.check_log('Open public cfg file to write failed, errno=[2]')
        device_logger.check_log('Open private cfg file to read failed, errno=[2]')
        device_logger.check_log('Open private cfg file to write failed, errno=[2]')

    def teardown(self):
        Step("收尾工作:删除hap")
        self.driver.uninstall_app("com.example.hnp_test")
        self.driver.uninstall_app("com.example.hnpselinuxtestapplication")
        self.driver.shell("hilog -b I")
