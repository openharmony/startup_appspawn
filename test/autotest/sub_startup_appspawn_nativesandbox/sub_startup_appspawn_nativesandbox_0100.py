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
from devicetest.core.test_case import Step, TestCase
from hypium.model import UiParam, WindowFilter
import os


class SubStartupAppspawnNativesandbox0100(TestCase):

    def __init__(self, controllers):
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        self.driver = UiDriver(self.device1)
        Step("预置工作:检测屏幕是否亮.................")
        self.driver.enable_auto_wakeup(self.device1)
        Step("预置工作:滑动解锁.................")
        self.driver.swipe(UiParam.UP, side=UiParam.BOTTOM)
        Step('设置屏幕常亮')
        self.driver.shell("power-shell timeout -o 2147483647")

    def test_step1(self):
        Step("步骤1：创建公有hnp存放目录")
        self.driver.shell("mkdir -p data/UCbrowser/public")
        Step("步骤2：导入安装包文件hnp")
        path = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        hnp = os.path.abspath(os.path.join(
            os.path.join(path, "testFile"), 'SUB_STARTUP_APPSPAWN_NATIVE/sign/hnpsample.hnp'))
        hap = os.path.abspath(
            os.path.join(os.path.join(path, "testFile"), 'SUB_STARTUP_APPSPAWN_NATIVE/sign/entry-default-signedArm64-test11.hap'))
        self.driver.push_file(hnp, "data/UCbrowser/public/")
        self.driver.push_file(hap, "data/UCbrowser")
        Step("步骤3：执行安装命令")
        self.driver.shell(
            "hnp install -u 100 -p uc_browser -i data/UCbrowser  -s data/UCbrowser/entry-default-signedArm64-test11.hap -a arm64 -f")
        Step("步骤4：查看是是否安装成功")
        public = self.driver.shell("ls /data/app/el1/bundle/100/hnppublic/")
        self.driver.Assert.contains(public, "hnpsample.org")
        result = self.driver.shell("cat /data/service/el1/startup/hnp_info.json")
        self.driver.Assert.contains(result, "uc_browser")
        Step("步骤5：安装测试hap包")
        ishap = self.driver.has_app("com.example.hnptestapplication")
        if (ishap):
        hap1 = os.path.abspath(
            os.path.join(os.path.join(path, "testFile"),
                         'SUB_STARTUP_APPSPAWN_NATIVE/entry-default-sandbox.hap'))
        self.driver.install_app(hap1)
        Step("步骤6：打开测试hap包")
        self.driver.shell("aa start -a EntryAbility -b com.example.hnptestapplication")
        Step("步骤7：获取测试应用pid")
        time.sleep(3)
        pid = self.driver.System.get_pid("com.example.hnptestapplication")
        Step("步骤8：查看测试应用公有沙箱路径")
        result = self.driver.shell("ls /proc/%d/root/data/service/hnp" % pid)
        Step("步骤9：预期结果校验")
        Step("识别设备型号...............................")
        device = self.driver.shell("param get const.product.model")
        device = device.replace("\n", "").replace(" ", "")
        device = str(device)
        if ("HYM" in device or "HAD" in device):
            self.driver.Assert.contains(result, "bin")
            self.driver.Assert.contains(result, "hnpsample.org")
        else:
            self.driver.Assert.contains(result, "No such file or directory")

