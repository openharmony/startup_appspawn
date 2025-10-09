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
from hyp import HypiumClass1, HypiumClass2, HypiumFunction1

from aw import Common


class Substartupappspawncoldstartablity0200(TestCase):

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
        Step("安装测试hap.................")
        sourpath = Common.sourcepath("asan-disable.hap", "SUB_STARTUP_APPSPAWN_COLDSTARTABLITY")
        Step(sourpath)
        self.driver.AppManager.install_app(sourpath)
        Step("预置工作:检测屏幕是否亮.................")
        self.driver.Screen.wake_up()
        self.driver.ScreenLock.unlock()
        self.driver.Screen.enable_stay_awake()

    def test_step1(self):
        Step("打开测试hap.................")
        self.driver.start_app("com.example.myapplication", "EntryAbility")
        sleep(3)
        pid = self.driver.System.get_pid('com.example.myapplication')
        Step(pid)
        Step("校验hap应用的maps.................")
        Step(result)
        self.driver.Assert.equal(result, '')


    def teardown(self):
        Step("收尾工作.................")
        Step("收尾工作：卸载hap......................")
        self.driver.AppManager.clear_app_data('com.example.myapplication')
        self.driver.AppManager.uninstall_app('com.example.myapplication')
