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
from devicetest.core.test_case import Step, TestCase
from hypium import *


class SubStartupAppspawnPluginfw0100(TestCase):

    def __init__(self, controllers):
        self.TAG = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        Step(self.devices[0].device_id)

    def test_step1(self):
        Step("通过命令查看/system/lib64/appspawn.................")
        result = self.driver.shell("ls -al /system/lib64/appspawn")
        sleep(3)
        self.driver.Assert.contains(result, "appspawn", "路径下缺少组件appspawn")
        self.driver.Assert.contains(result, "common", "路径下缺少组件common")
        self.driver.Assert.contains(result, "libappspawn_asan.z.so", "路径下缺少组件libappspawn_asan.z.so")
        self.driver.Assert.contains(result, "nwebspawn", "路径下缺少组件nwebspawn")

    def teardown(self):
        Step("收尾工作.................")
