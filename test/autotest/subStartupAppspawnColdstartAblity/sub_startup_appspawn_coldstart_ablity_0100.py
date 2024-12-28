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

import time
from devicetest.core.test_case import TestCase, Step
from hypium import UiDriver

from aw import Common


class SubStartupAppspawnColdstartAblity0100(TestCase):


    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.tag, controllers)
        self.driver = UiDriver(self.device1)
    
    def setup(self):
        Step("预置工作开始")
        Step(self.devices[0].device_id)
        Step("安装测试hap")
        sourcepath = Common.sourcepath("asan-enable.hap", "SUB_STARTUP_APPSPAWN_COLDSTARTABLITY")
        Step(sourcepath)
        self.driver.AppManager.install_app(sourcepath)
        Step("预置工作：检查屏幕")
        self.driver.Screen.wake_up()
        self.driver.ScreenLock.unlock()
        self.driver.Screen.enable_stay_awake()

    def test_step1(self):
        Step("打开测试hap...............")
        result = self.driver.startup_app("com.example.myapplication", "EntryAbility")
        sleep(3)
        pid = self.driver.System.get_pid('com.example.myapplication')
        Step(pid)
        Step("校验")
        result = self.driver.shell('cat /proc/self/maps|grep libclang_rt.asan.so')
        Step(result)
        self.driver.Assert.contains(result, '/system/lib64/libclang_rt.asan.so')
        self.driver.Assert.contains(result, 'anon:libclang_rt.asan.so.bss')

    def teardown(self):
        Step("收尾工作...")
        self.driver.AppManager.clear_app_data('com.example.myapplication')
        self.driver.AppManager.uninstall_app('com.example.myapplication')