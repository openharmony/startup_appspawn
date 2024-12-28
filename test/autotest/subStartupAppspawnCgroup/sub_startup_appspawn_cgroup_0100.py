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


class SubStartupAppspawnCgroup0100(TestCase):


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
        #解锁屏幕
        wake = self.driver.Screen.is_on()
        time.sleep(0.5)
        if wake:
            self.driver.ScreenLock.unlock()
        else:
            self.driver.Screen.wake_up()
            self.driver.ScreenLock.unlock()
        self.driver.shell("power-shell setmdoe 602")

    def process(self):
        hap_path = Common.sourcepath('ForkTest1.hap', "SUB_STARTUP_APPSPAWN_CGROUPS")
        bundle_name = "com.example.exe_sys_cmd"
        hap = self.driver.AppManager.has_app(bundle_name)
        if hap:
            self.driver.AppManager.clear_app_data(bundle_name)
            self.driver.AppManager.uninstall_app(bundle_name)
            self.driver.AppManager.install_app(bundle_name)
        else:
            self.driver.AppManager.install_app(bundle_name)
        self.driver.AppManager.start_app(bundle_name)
        pid = self.driver.System.get_pid(bundle_name)
        result1 = self.driver.shell("ps -ef |grep %s" % bundle_name)
        cgroup1 = self.driver.shell("cat /dev/pids/100/%s/app_%d/cgroup.procs" % (bundle_name, pid)).split()
        count = 0
        for i in cgroup1:
            self.driver.Assert.contains(result1.split(), i)
            if result1.count(i) == 2:
                count += 1
        self.driver.Assert.equal(count, 1)
        result2 = self.driver.shell("kill -9 %d" % pid)
        self.driver.Assert.equal(result2, '')
        result3 = self.driver.shell("ps -ef |grep %s|grep -v grep " % bundle_name)
        self.driver.Assert.equal(result3, '')

    def teardown(self):
        Step("收尾工作...")
        self.driver.AppManager.clear_app_data(bundle_name)
        self.driver.AppManager.uninstall_app(bundle_name)