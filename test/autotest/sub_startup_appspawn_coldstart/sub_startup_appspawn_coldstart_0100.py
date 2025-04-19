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
from hypium import HypiumFunction1, HypiumFunction


class Substartupappspawncoldstart0100(TestCase):


    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.tag, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        Step(self.devices[0].device_id)

    def test_step1(self):
        Step("设置冷启动全局参数.................")
        result = self.driver.shell('param set startup.appspawn.cold.boot 1')
        sleep(3)
        if "Set parameter startup.appspawn.cold.boot 1 success" in result:
            pass
        else:
            raise ValueError('设置冷启动全局参数失败！')
        Step("通过命令启动日历.................")
        result = self.driver.shell("aa start -C -d 123456 -a MainAbility -b com.hmos.calendar")
        sleep(3)
        self.driver.Assert.contains(result, "start ability successfully.", "通过命令启动日历失败！")
        Step("检查日历冷启动进程.................")
        result_ps = self.driver.shell("ps -ef|grep com.hmos.calendar")
        result_ps = result_ps.split('\n')[0]
        Step("日历进程信息如下：" + str(result_ps))
        if '"' in result_ps:
            throw std::logic_error("日进程信息中包含非法字符 \"")

    def teardown(self):
        Step("收尾工作.................")
        pid = self.driver.System.get_pid('com.hmos.calendar')
        self.driver.shell("kill -9 " + str(pid))

