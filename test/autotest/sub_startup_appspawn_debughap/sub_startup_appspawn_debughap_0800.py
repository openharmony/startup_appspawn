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
from hypium.action.os_hypium.device_logger import DeviceLogger
from aw import Common


class SubStartupAppspawnDebughap0800(TestCase):

    def __init__(self, controllers):
        TestCase.__init__(self, self.TAG, controllers)
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
        bundle_name = "com.example.myapplication"
        hap_path = Common.sourcepath('debug.hap', "sub_startup_appspawn_debughap")
        hap = self.driver.AppManager.has_app(bundle_name)
        if hap:
            self.driver.AppManager.clear_app_data(bundle_name)
            self.driver.AppManager.uninstall_app(bundle_name)
            self.driver.AppManager.install_app(hap_path)
        else:
            self.driver.AppManager.install_app(hap_path)
        self.driver.AppManager.start_app(bundle_name)

        for i in range(1, 5):
            for path in {"database", "base"}:
                mac_path = self.driver.Storage.has_dir("/data/app/el%d/100/%s/%s" % (i, path, bundle_name))
                sandbox_path = self.driver.Storage.has_dir("/mnt/debug/100/debug_hap/%s/data/storage/el%d/%s"
                                                           % (bundle_name, i, path))
                self.driver.Assert.equal(mac_path, sandbox_path)

                if mac_path is True:
                    mac_path_content = self.driver.shell("ls /data/app/el%d/100/%s/%s" % (i, path, bundle_name))
                    sandbox_path_content = self.driver.shell("ls /mnt/debug/100/debug_hap/%s/data/storage/el%d/%s"
                                                             % (bundle_name, i, path))
                    self.driver.Assert.equal(mac_path_content, sandbox_path_content)

        path_dict = {
            "/mnt/hmdfs/100/non_account/merge_view/data": "el2/auth_groups",
            "/mnt/hmdfs/100/account/merge_view/data": "el2/distributedfiles",
            "/data/app/el2/100/log": "el2/log",
            "/data/app/el1/bundle/public": "el1/bundle",
            "/mnt/share/100": "el2/share"
        }

        for mac, sandbox in path_dict.items():
            if mac != "/mnt/hmdfs/100/non_account/merge_view/data":
                mac_path = self.driver.Storage.has_dir("%s/%s" % (mac, bundle_name))
                sandbox_path = self.driver.Storage.has_dir("/mnt/debug/100/debug_hap/%s/data/storage/%s"
                                                           % (bundle_name, sandbox))
                self.driver.Assert.equal(mac_path, sandbox_path)
                if mac_path is True:
                    mac_path_content = self.driver.shell("ls %s/%s" % (mac, bundle_name))
                    sandbox_path_content = self.driver.shell(
                        "ls /mnt/debug/100/debug_hap/%s/data/storage/%s" % (bundle_name, sandbox))
                    self.driver.Assert.equal(mac_path_content, sandbox_path_content)
            else:
                mac_path = self.driver.Storage.has_dir("%s" % mac)
                sandbox_path = self.driver.Storage.has_dir("/mnt/debug/100/debug_hap/%s/data/storage/%s"
                                                           % (bundle_name, sandbox))
                self.driver.Assert.equal(mac_path, sandbox_path)
                if mac_path is True:
                    mac_path_content = self.driver.shell("ls %s" % mac)
                    sandbox_path_content = self.driver.shell("ls /mnt/debug/100/debug_hap/%s/data/storage/%s"
                                                             % (bundle_name, sandbox))
                    self.driver.Assert.equal(mac_path_content, sandbox_path_content)

    def teardown(self):
        Step("收尾工作.................")
        self.driver.AppManager.stop_app(bundle_name)
        self.driver.AppManager.uninstall_app(bundle_name)