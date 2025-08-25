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

from time import sleep
from devicetest.core.test_case import Step
from devicetest.core.suite.test_suite import TestSuite
from aw import Common


class SubStartupAppspawnNativeprocess(TestSuite):

    def setup(self):
        Step("TestCase: setup")
        self.driver = UiDriver(self.device1)
        Step("预置工作:初始化手机开始.................")
        Step(self.devices[0].device_id)
        Step("导入配置文件.................")
        self.driver.hdc("target mount")
        sourpath = Common.sourcepath("AppSpawnTest", "sub_startup_appspawn_nativeprocess")
        destpath = "/data/AppSpawnTest"
        self.driver.push_file(sourpath, destpath)
        sleep(3)
        Step("给测试文件AppSpawnTest可执行权限.................")
        self.driver.shell('chmod +x /data/AppSpawnTest')
        self.driver.shell('./data/AppSpawnTest')

    def teardown(self):
        Step("收尾工作，删除文件.................")
        self.driver.shell("rm /data/AppSpawnTest")
