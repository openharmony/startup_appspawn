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

from aw import Common


class SubStartupAppspawnFtpd0100(TestCase):

    def __init__(self, controllers):
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step("预置工作:初始化手机开始.................")
        Step(self.devices[0].device_id)
        Step("设置系统参数sysctl -w kernel.xpm.xpm_mode=0..........................")
        self.driver.hdc("target mount")
        self.driver.shell("sed -i 's%/proc/sys/kernel/xpm/xpm_mode 4%/proc/sys/kernel/xpm/xpm_mode 0%g' /system/etc/init/key_enable.cfg")
        self.driver.shell("reboot")
        self.driver.System.wait_for_boot_complete()
        set_result = self.driver.shell("sysctl -w kernel.xpm.xpm_mode=0")
        self.driver.Assert.contains(set_result, "kernel.xpm.xpm_mode = 0")
        Step("安装测试hap.................")
        sourpath = Common.sourcepath("ftpd.hap", "sub_startup_appspawn_ftpd")
        Step(sourpath)
        self.driver.AppManager.install_app(sourpath)
        Step("预置工作:检测屏幕是否亮.................")
        self.driver.Screen.wake_up()
        self.driver.ScreenLock.unlock()
        self.driver.Screen.enable_stay_awake()

    def test_step1(self):
        Step("打开测试hap.................")
        self.driver.start_app("com.ohos.myapplication", "EntryAbility")
        Step("打开bftpd.................")
        aa_resutl = self.driver.shell("aa process -b com.ohos.myapplication -a EntryAbility -D '/system/bin/bftpd -D -p 9021'  -S")
        self.driver.Assert.contains(aa_resutl, "start native process successfully")
        Step("校验bftpd进程")
        pid = self.driver.System.get_pid('bftpd')

    def teardown(self):
        Step("收尾工作.................")
        Step("收尾工作：卸载hap......................")
        self.driver.AppManager.clear_app_data('com.ohos.myapplication')
        self.driver.AppManager.uninstall_app('com.ohos.myapplication')
        Step("恢复系统参数......................")
        recovery_result = self.driver.shell("sysctl -w kernel.xpm.xpm_mode=4")
        self.driver.Assert.contains(recovery_result, "kernel.xpm.xpm_mode = 4")
