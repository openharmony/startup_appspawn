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

from devicetest.core.test_case import Step, TestCase
from hypium import *


class sub_startup_appspawn_hostsandbox_0300(TestCase):

    def __init__(self, controllers):
        self.TAG = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化PC开始.................")
        Step(self.devices[0].device_id)
        Step("查看hosts文件是否存在.................")
        hosts_file = self.driver.shell('ls -l data/service/el1/network/hosts_user/hosts')
        self.driver.Assert.contains(hosts_file, "hosts", "hosts文件不存在")
        Step("备份hosts文件.................")
        self.driver.Storage.remount()
        self.driver.shell('cp data/service/el1/network/hosts_user/hosts data/service/el1/network/hosts_user/hosts_bak')
        Step("为hosts文件写入数据.................")
        self.driver.shell('echo 99998888 > data/service/el1/network/hosts_user/hosts')
        hosts_result = self.driver.shell('cat data/service/el1/network/hosts_user/hosts')
        self.driver.Assert.contains(hosts_result, "99998888", "hosts文件写入失败")
        self.driver.shell("reboot")
        self.driver.System.wait_for_boot_complete()

    def test_step1(self):
        pid = self.driver.System.get_pid("com.ohos.sceneboard")
        result1 = self.driver.shell("cat /proc/%d/root/data/service/el1/network/hosts_user/hosts" % pid)
        self.driver.Assert.contains(result1, "99998888")
        result2 = self.driver.shell("cat /proc/%d/root/system/etc/hosts" % pid)
        self.driver.Assert.contains(result2, "99998888")

    def teardown(self):
        Step("收尾工作.................")
        Step("还原hosts文件.................")
        self.driver.Storage.remount()
        self.driver.shell('mv data/service/el1/network/hosts_user/hosts_bak data/service/el1/network/hosts_user/hosts')
        self.driver.shell("reboot")
        self.driver.System.wait_for_boot_complete()
