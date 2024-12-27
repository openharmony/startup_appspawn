# Copyright (c) 2024 Huawei Device Co., Ltd.
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
from hypium import UiDriver
from hypium.model import UiParam
from hypium.action.os_hypium.device_logger import DeviceLogger
from aw import Common


class SubStartupAppspawnAppclone0900(TestCase):
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
        self.driver.Screen.enable_stay_awake()

    def process(self):
        Step("安装测试hap并打开")
        bundle_name = "com.example.intaketest"
        hap_path = Common.sourcepath('intaketest.hap', "sub_startup_appspawn_appclone")
        hap = self.driver.AppManager.has_app(bundle_name)
        if hap:
            self.driver.AppManager.clear_app_data(bundle_name)
            self.driver.AppManager.uninstall_app(bundle_name)
            self.driver.AppManager.install_app(hap_path)
        else:
            self.driver.AppManager.install_app(hap_path)

        self.driver.AppManager.stop_app("com.example.settings")
        self.driver.AppManager.start_app("com.example.settings")
        self.driver.swipe(UiParam.UP)
        for text in ["系统", "应用分身", "IntakeTest"]:
            self.driver.touch(BY.text(text))
        for i in range(5):
            self.driver.touch(BY.text("创建分身"))
            time.sleep(1)
        self.driver.AppManager.stop_app("com.example.settings")

        for i in range(1, 7):
            self.driver.AppManager.start_app(bundle_name)
            if i == 1:
                self.driver.touch(BY.text("IntakeTest"))
                pid = self.driver.System.get_pid(bundle_name)
            else:
                self.driver.touch(BY.text("更多打开方式"))
                self.driver.touch(BY.text("IntakeTest%d" % (i - 1)))
                pid = self.driver.System.get_pid("%s%d" % (bundle_name, i - 1))
            self.driver.Assert.equal(type(pid), int)

        self.driver.AppManager.uninstall_app(bundle_name)
        self.driver.AppManager.start_app("com.example.settings")
        self.driver.swipe(UiParam.UP)
        for text in ["系统", "应用分身"]:
            self.driver.touch(BY.text(text))
        self.driver.check_component_exist(BY.text("IntakeTest"), expect_exist=False)

    def teardown(self):
        Step("收尾工作.................")
        self.driver.AppManager.stop_app("com.example.settings")
