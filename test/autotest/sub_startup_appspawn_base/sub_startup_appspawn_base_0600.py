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


class SubStartupAppspawnBase0600(TestCase):
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
        bundle_name = "com.ohos.mytest"
        clone_bundle_name = "com.ohos.mytest1"
        hap_path = Common.sourcepath("mytest.hap", "SUB_STARTUP_APPSPAWN_BASE")
        hap = self.driver.AppManager.has_app(bundle_name)
        if hap:
            self.driver.AppManager.uninstall_app(bundle_name)
            self.driver.AppManager.install_app(hap_path)
        else:
            self.driver.AppManager.install_app(hap_path)
        self.driver.AppManager.start_app(bundle_name)

        for text in ["安装克隆应用", "startAbilityByAppIndex"]:
            self.driver.touch(BY.text(text))

        dict01 = {
            bundle_name: "master.txt",
            clone_bundle_name: "slave.txt"
        }

        dict02 = {
            bundle_name: "slave.txt",
            clone_bundle_name: "master.txt"
        }

        for i, txt in dict01.items():
            pid = self.driver.System.get_pid(i)
            if i == clone_bundle_name:
                self.driver.shell("touch ../data/app/el2/100/base/+clone-1+%s/%s" % (bundle_name, txt))
            else:
                self.driver.shell("touch ../data/app/el2/100/base/%s/%s" % (i, txt))
            result = self.driver.Storage.has_file("/proc/%d/root/data/storage/el2/base/%s" % (pid, txt))
            self.driver.Assert.equal(result, True)

        for i, txt in dict02.items():
            pid = self.driver.System.get_pid(i)
            result = self.driver.Storage.has_file("/proc/%d/root/data/storage/el2/base/%s" % (pid, txt))
            self.driver.Assert.equal(result, False)

    def teardown(self):
        Step("收尾工作.................")
        self.driver.AppManager.uninstall_app(bundle_name)