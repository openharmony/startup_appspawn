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
from hypium.model import UiParam, WindowFilter
import os
import time


class SubStartupAppspawnNativepack0100(TestCase):

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
        self.driver.Screen.enable_stay_awake()
        Step('预置工作：新增一个123.txt文件')
        path = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        file = os.path.abspath(os.path.join(os.path.join(path, "testFile"), 'sub_startup_appspawn_nativepack/window/sample'))
        with open(file + "\\lib\\123.txt", "a+") as f1:
            f1.write("12345678hdaskheiwoi4eliqwkldsajoejqooiudsisa")

    def test_step1(self):
        Step("获取bat文件路径")
        path = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        bat = os.path.abspath(os.path.join(os.path.join(path, "testFile"), 'sub_startup_appspawn_nativepack/window/run.bat'))
        Step("执行bat文件")
        host.shell(bat)
        Step("复制压缩文件")
        out = os.path.abspath(os.path.join(os.path.join(path, "testFile"), 'sub_startup_appspawn_nativepack/window/out/'))
        sample = os.path.abspath(os.path.join(os.path.join(path, "testFile"), 'sub_startup_appspawn_nativepack/window/sample/'))
        file1 = os.path.abspath(os.path.join(os.path.join(path, "testFile"), 'sub_startup_appspawn_nativepack/window/out/hnpsample.hnp'))
        file2 = os.path.abspath(os.path.join(os.path.join(path, "testFile"), 'sub_startup_appspawn_nativepack/window/out/hnpsample.zip'))
        host.shell("copy "+file1 + " " + file2)
        Step("步骤7：解压hnpsample.zip文件")
        host.unzip_file(file2, out)
        Step("步骤8：比较2个文件内容是否一致")
        time.sleep(1)
            dif1 = f.read()
        with open(out+"\\sample\\cfg\\hnpsample.cfg") as f:
            dif2 = f.read()
        self.driver.Assert.equal(dif1, dif2)
            dif1 = f.read()
            dif2 = f.read()
        self.driver.Assert.equal(dif1, dif2)
        with open(sample + "\\lib\\123.txt") as f:
            dif1 = f.read()
        with open(out + "\\sample\\lib\\123.txt") as f:
            dif2 = f.read()
        self.driver.Assert.equal(dif1, dif2)

    def teardown(self):
        Step("收尾工作:删除out目录文件")
        path = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
        out = os.path.abspath(os.path.join(os.path.join(path, "testFile"), 'sub_startup_appspawn_nativepack/window/out/'))