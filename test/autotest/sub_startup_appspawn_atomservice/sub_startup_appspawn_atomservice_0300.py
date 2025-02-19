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


class SubStartupAppspawnAtomservice0300(TestCase):
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
        bundle_name = "com.atomicservice.5765880207854649689"
        bundle_name_start = "+auid-ohosAnonymousUid+com.atomicservice.5765880207854649689"
        hap_path = Common.sourcepath('atomicservice.hap', "sub_startup_appspawn_atomservice")
        hap = self.driver.AppManager.has_app(bundle_name)
        if hap:
            self.driver.AppManager.clear_app_data(bundle_name)
            self.driver.AppManager.uninstall_app(bundle_name)
            self.driver.AppManager.install_app(hap_path)
        else:
            self.driver.AppManager.install_app(hap_path)
        self.driver.shell("aa start -a EntryAbility -b %s" % bundle_name)
        pid = self.driver.System.get_pid(bundle_name)

        for path in ["base", "database"]:
            for i in range(1, 5):
                result1 = self.driver.shell("ls /proc/%d/root/data/storage/el%d/%s" % (pid, i, path))
                result2 = self.driver.shell("ls /data/app/el%d/100/%s/%s" % (i, path, bundle_name_start))
                self.driver.Assert.equal(result1, result2)

    def teardown(self):
        Step("收尾工作.................")
        self.driver.AppManager.uninstall_app(bundle_name)
