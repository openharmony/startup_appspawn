# Copyright (c) 2024 Huawei Device Co., Ltd.
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

from devicetest.core.test_case import TestCase, Step, CheckPoint
from devicetest.core.test_case import Step, TestCase
from hypium import UiParam
from hypium.model import UiParam
from aw import Common
import time


class SubStartupAppspawnNativespawn1900(TestCase):

    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.tag, controllers)
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
        Step('获取权限状态')
        is_status = self.driver.shell("param get persist.sys.abilityms.multi_process_model")
        if ("true" in is_status):
            pass
        else:
            Step('设置权限为true')
            self.driver.shell("param set persist.sys.abilityms.multi_process_model true")
            Step('重启生效')
            self.driver.shell("reboot")
            self.driver.System.wait_for_boot_complete()
            self.driver.ScreenLock.unlock()

    def test_step1(self):
        Step("步骤1:安装测试应用")
        hap1 = Common.sourcepath('entry-default-nativespawn.hap', "SUB_STARTUP_STABILITY_APPSPAWN")
        hap2 = Common.sourcepath('entry-default-nativespawn1.hap', "SUB_STARTUP_STABILITY_APPSPAWN")
        is_hap = self.driver.has_app("com.acts.childprocessmanager")
        if(is_hap):
            self.driver.uninstall_app("com.acts.childprocessmanager")
        else:
            pass
        is_hap = self.driver.has_app("com.acts.childprocessmanager1")
        if (is_hap):
            self.driver.uninstall_app("com.acts.childprocessmanager1")
        else:
            pass
        self.driver.install_app(hap1)
        self.driver.install_app(hap2)
        Step("步骤2:打开测试应用1")
        self.driver.start_app("com.acts.childprocessmanager")
        Step("步骤3:点击StartArk用例按钮")
        self.driver.touch(BY.text("StartArk用例"))
        Step("步骤4:首次获取nativesapwn进程号")
        pid1 = self.driver.System.get_pid("nativespawn")
        Step("步骤5:点击start Native Process Isolated按钮")
        self.driver.touch(BY.text("start Native Process Isolated"))
        Step("步骤6:第二次获取nativesapwn进程号")
        pid2 = self.driver.System.get_pid("nativespawn")
        Step("步骤7:打开测试应用2")
        self.driver.start_app("com.acts.childprocessmanager1")
        Step("步骤8:点击StartArk用例按钮")
        self.driver.touch(BY.text("StartArk用例"))
        Step("步骤9:点击start Native Process Isolated按钮")
        self.driver.touch(BY.text("start Native Process Isolated"))
        Step("步骤10:关闭测试应用1")
        self.driver.stop_app("com.acts.childprocessmanager")
        Step("步骤11:第三次获取nativesapwn进程号")
        pid3 = self.driver.System.get_pid("nativespawn")
        Step("步骤12:关闭测试应用1")
        self.driver.stop_app("com.acts.childprocessmanager1")
        Step("步骤13:第四次获取nativesapwn进程号")
        pid4 = self.driver.System.get_pid("nativespawn")
        Step("步骤12:预期结果校验")
        self.driver.Assert.equal(pid1, None)
        self.driver.Assert.not_equal(pid2, None)
        self.driver.Assert.not_equal(pid3, None)
        self.driver.Assert.equal(pid4, None)

    def teardown(self):
        Step("收尾工作:卸载测试应用")
        self.driver.uninstall_app("com.acts.childprocessmanager")
        self.driver.uninstall_app("com.acts.childprocessmanager1")