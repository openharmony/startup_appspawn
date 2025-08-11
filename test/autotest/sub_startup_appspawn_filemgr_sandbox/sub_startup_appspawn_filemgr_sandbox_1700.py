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
from hypium import FilemgrSandbox, Appspawn, SubStartup


class SubStartupAppspawnFilemgrSandbox1700(TestCase):

    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        TestCase.__init__(self, self.tag, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step(self.devices[0].device_id)
        device
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
        notepad
        notepad = "com.hmos.notepad"
        self.driver.shell("echo 0 > /proc/sys/kernel/dec/dec_mode")
        self.driver.AppManager.start_app(notepad)
        time.sleep(3)
        pid = self.driver.System.get_pid(notepad)
        result = self.driver.shell("ls -Z /proc/%d/root/storage/Users/currentUser/appdata/el1" % pid).split("\n")
        self.driver.Assert.equal(result[0], "u:object_r:sharefs_appdata_file:s0 base")

    def teardown(self):
        Step("收尾工作.................")
        self.driver.shell("echo 1 > /proc/sys/kernel/dec/dec_mode")
        self.driver.AppManager.clear_app_data(notepad)
        self.driver.AppManager.stop_app(notepad)