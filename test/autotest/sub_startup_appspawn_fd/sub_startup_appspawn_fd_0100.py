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
from hypium.action.os_hypium.device_logger import DeviceLogger
from aw import Common


class SubStartupAppspawnFd0100(TestCase):

    def __init__(self, controllers):
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        Step(self.devices[0].device_id)
        Step("安装测试hap.................")
        sourpath = Common.sourcepath("fd.hap", "sub_startup_appspawn_fd")
        Step(sourpath)
        self.driver.AppManager.install_app(sourpath)
        Step("设置系统参数.................")
        self.driver.shell('param set persist.sys.abilityms.multi_process_model true')
        Step("执行重启..........................")
        self.driver.System.reboot()
        sleep(3)
        self.driver.System.wait_for_boot_complete()
        Step("预置工作:检测屏幕是否亮.................")
        self.driver.Screen.wake_up()
        self.driver.ScreenLock.unlock()
        self.driver.Screen.enable_stay_awake()

    def test_step1(self):
        self.d = UiExplorer(self.device1)
        device_logger = DeviceLogger(self.d)
        Step("设置关键字..........................")
        device_logger.set_filter_string("ldprocessmanager:OtherProcess")
        Step("开启日志..........................")
        device_logger.start_log("testFile/Log/log.txt")
        Step("打开测试hap.................")
        self.driver.start_app("com.acts.childprocessmanager", "EntryAbility")
        sleep(3)
        Step("点击Start Child Process.................")
        self.driver.touch(BY.text("Start Child Process"))
        Step("点击Start Ark Process.................")
        self.driver.touch(BY.text("Start Ark Process"))
        sleep(10)
        Step("停止日志..........................")
        device_logger.stop_log()
        Step("校验日志内容..........................")
        Step("查看childprocessmanager服务pid..........................")
        pid = self.driver.System.get_pid('com.acts.childprocessmanager:OtherProcess1')
        result = self.driver.shell("ls -al /proc/%d/fd" % pid)
        Step("步骤6：预期结果校验")
        self.driver.Assert.contains(result, "/data/storage/el2/base/haps/entry/files/test.txt")
        self.driver.Assert.contains(result, "/data/storage/el2/base/haps/entry/files/test2.txt")

    def teardown(self):
        Step("收尾工作.................")
        Step("收尾工作：卸载hap......................")
        self.driver.AppManager.clear_app_data('com.acts.childprocessmanager')
        self.driver.AppManager.uninstall_app('com.acts.childprocessmanager')
        Step("恢复系统参数.................")
        self.driver.hdc('param set persist.sys.abilityms.multi_process_model false')
        Step("执行重启..........................")
        self.driver.System.reboot()
        sleep(3)
        self.driver.System.wait_for_boot_complete()
