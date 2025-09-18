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
from hypium import *
from devicetest.core.test_case import Step, TestCase
from aw import Common


class sub_startup_appspawn_selinuxlabelconf_1000(TestCase):

    def __init__(self, controllers):
        self.TAG = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("TestCase: setup")
        Step("预置工作:初始化手机开始.................")
        Step(self.devices[0].device_id)
        Step("安装测试hap.................")
        sourpath = Common.sourcepath("selinux.hap", "sub_startup_appspawn_selinuxlabelconf")
        Step(sourpath)
        self.driver.AppManager.install_app(sourpath)
        sleep(3)

    def test_step1(self):
        Step("打开测试hap.................")
        self.driver.start_app("com.example.myapplication", "EntryAbility")
        result_el1 = self.driver.shell('ls -lZ /data/app/el1/100/base/ | grep com.example.myapplication')
        Step(result_el1)
        result_el2 = self.driver.shell('ls -lZ /data/app/el2/100/base/ | grep com.example.myapplication')
        Step(result_el2)
        self.driver.Assert.contains(result_el1, "hap_data_file", "安装非预置应用，对应标签为normal_hap")
        self.driver.Assert.contains(result_el2, "hap_data_file", "安装非预置应用，对应标签为normal_hap")

    def teardown(self):
        Step("TestCase: teardown")
        Step("收尾工作：卸载hap......................")
        self.driver.AppManager.clear_app_data('com.example.myapplication')
        self.driver.AppManager.uninstall_app('com.example.myapplication')
