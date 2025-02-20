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
from devicetest.core.test_case import TestCase, Step
from hypium import UiDriver
from aw import Common


class SubStartupAppspawnBase0300(TestCase):

    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        TestCase.__init__(self, self.tag, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step(self.devices[0].device_id)
        device = self.driver.shell("param get const.product.model")
        device = device.replace("\n", "").replace(" ", "")
        device = str(device)
        Step(device)
        # 解锁屏幕
        wake = self.driver.Screen.is_on()
        time.sleep(0.5)
        if wake:
            self.driver.ScreenLock.unlock()
        else:
            self.driver.Screen.wake_up()
            self.driver.ScreenLock.unlock()
        self.driver.shell("power-shell timeout -o 86400000")

    def process(self):
        Step("安装测试hap并打开")
        bundle_name = "com.example.myapplication"
        hap_path = Common.sourcepath('asan.hap', "SUB_STARTUP_APPSPAWN_BASE")
        hap = self.driver.AppManager.has_app(bundle_name)
        if hap:
            self.driver.AppManager.uninstall_app(bundle_name)
            self.driver.AppManager.install_app(hap_path)
        else:
            self.driver.AppManager.install_app(hap_path)
        self.driver.AppManager.start_app(bundle_name)

        result = self.driver.shell("ps -efZ|grep %s|grep -v grep" % bundle_name).split()
        for i in ["u:r:debug_hap:s0", bundle_name]:
            self.driver.Assert.contains(result, i)

    def teardown(self):
        Step("收尾工作.................")
        self.driver.AppManager.uninstall_app(bundle_name)