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

from time import sleep
from devicetest.core.test_case import Step, TestCase
from hypium import *
from hypium.action.os_hypium.device_logger import DeviceLogger


class SubStartupAppspawnPrefork0100(TestCase):

    def __init__(self, controllers):
        self.TAG = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        Step(self.devices[0].device_id)
        Step("设置参数..........................")
        self.driver.shell("param set persist.sys.prefork.enable false")
        Step("重启手机..........................")
        self.driver.shell("reboot")
        self.driver.System.wait_for_boot_complete()
        Step("预置工作:检测屏幕是否亮.................")
        self.driver.Screen.wake_up()
        self.driver.ScreenLock.unlock()
        self.driver.Screen.enable_stay_awake()

    def test_step1(self):
        self.d = UiExplorer(self.device1)
        device_logger = DeviceLogger(self.d)
        Step("设置关键字..........................")
        device_logger.set_filter_string("appspawn")
        Step("开启日志..........................")
        device_logger.start_log("testFile/Log/log.txt")
        Step("打开备忘录.................")
        self.driver.stop_app("com.huawei.hmos.notepad")
        self.driver.start_app("com.huawei.hmos.notepad", "MainAbility", wait_time=2)
        sleep(5)
        Step("停止日志..........................")
        device_logger.stop_log()
        Step("校验日志内容..........................")
        device_logger.check_not_exist_keyword("prefork")


    def teardown(self):
        Step("收尾工作.................")
        Step("关闭备忘录................")
        self.driver.stop_app("com.huawei.hmos.notepad")
