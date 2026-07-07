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


class SubStartupAppspawnAtomservice0100(TestCase):
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
        hap_info01, hap_info02 = [], []
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

        for path in ["base", "database"]:
            for i in range(1, 5):
                has_dir = self.driver.Storage.has_dir("/data/app/el%d/100/%s/%s" % (i, path, bundle_name))
                has_dir_login = self.driver.Storage.has_dir("/data/app/el%d/100/%s/%s" % (i, path, bundle_name_start))
                self.driver.Assert.equal(has_dir, True)
                self.driver.Assert.equal(has_dir_login, False)

        log_has_dir = self.driver.Storage.has_dir("/data/app/el2/100/log/%s" % bundle_name)
        log_has_dir_login = self.driver.Storage.has_dir("/data/app/el2/100/log/%s" % bundle_name_start)
        self.driver.Assert.equal(log_has_dir, True)
        self.driver.Assert.equal(log_has_dir_login, False)

        mnt_has_dir = self.driver.Storage.has_dir("/mnt/share/100/%s" % bundle_name)
        mnt_has_dir_login = self.driver.Storage.has_dir("/mnt/share/100/%s" % bundle_name_start)
        self.driver.Assert.equal(mnt_has_dir, True)
        self.driver.Assert.equal(mnt_has_dir_login, False)

        self.driver.shell("aa start -a EntryAbility -b %s" % bundle_name)
        for path in ["base", "database"]:
            for i in range(1, 5):
                has_dir_login = self.driver.Storage.has_dir("/data/app/el%d/100/%s/%s" % (i, path, bundle_name_start))
                self.driver.Assert.equal(has_dir_login, True)

        mnt_has_dir_login = self.driver.Storage.has_dir("/mnt/share/100/%s" % bundle_name_start)
        self.driver.Assert.equal(mnt_has_dir_login, True)

        for i in range(1, 5):
            hap_detail = self.driver.shell("ls -lZ ../data/app/el%d/100/%s" % (i, "base")).split("\n")
            for hap in hap_detail[1:-2]:
                if hap.split()[-1] == bundle_name:
                    hap_info01 = hap
                if hap.split()[-1] == bundle_name_start:
                    hap_info02 = hap
            for index in range(2, 5):
                self.driver.Assert.equal(hap_info01.split()[index], hap_info02.split()[index])

        hap_detail = self.driver.shell("ls -lZ ../mnt/share/100").split("\n")
        for hap in hap_detail[1:-2]:
            if hap.split()[-1] == bundle_name:
                hap_info01 = hap
            if hap.split()[-1] == bundle_name_start:
                hap_info02 = hap
        for index in range(2, 5):
            self.driver.Assert.equal(hap_info01.split()[index], hap_info02.split()[index])

    def teardown(self):
        Step("收尾工作.................")
        self.driver.AppManager.uninstall_app("com.atomicservice.5765880207854649689")
