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
from devicetest.core.test_case import TestCase, Step
from hypium import *
from hypium.action.os_hypium.device_logger import DeviceLogger
from aw import Common


SubStartupAppspawnDevicedebug0200(TestCase):

    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        TestCase.__init__(self, self.tag, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step(self.devices[0].device_id)
        global device
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
        self.driver.Screen.enable_stay_awake()

    def process(self):
        Step("安装测试hap并打开")
        global bundle_name
        bundle_name = "com.example.myapplication"
        hap_path = Common.sourcepath('debug.hap', "sub_startup_appspawn_devicedebug")
        hap = self.driver.AppManager.has_app(bundle_name)
        if hap:
            self.driver.AppManager.clear_app_data(bundle_name)
            self.driver.AppManager.uninstall_app(bundle_name)
            self.driver.AppManager.install_app(hap_path)
        else:
            self.driver.AppManager.install_app(hap_path)
        self.driver.AppManager.start_app(bundle_name)
        pid1 = self.driver.System.get_pid(bundle_name)

        Step("开启日志")
        device_logger = DeviceLogger(self.driver).set_filter_string("C02C11")
        log_name = 'LOG_' + os.path.basename(__file__).split('.')[0]
        device_logger.start_log("testFile/sub_startup_appspawn_devicedebug/%s.txt" % log_name)

        Step("devicedebug发送kill信号")
        self.driver.shell("devicedebug kill -9 %d" % pid1)

        Step("匹配日志")
        device_logger.stop_log()
        time.sleep(10)
        assert device_logger.check_log("devicedebug cmd kill start signal[9], pid[%d]" % pid1)
        assert device_logger.check_log("appspawn devicedebug debugable=1, pid=%d, signal=9" % pid1)
        assert device_logger.check_log("devicedebug manager process exit. ret=0")
        self.driver.AppManager.start_app(bundle_name)
        pid2 = self.driver.System.get_pid(bundle_name)
        self.driver.Assert.not_equal(pid1, pid2)

    def teardown(self):
        Step("收尾工作.................")
        self.driver.AppManager.stop_app(bundle_name)
        self.driver.AppManager.uninstall_app(bundle_name)