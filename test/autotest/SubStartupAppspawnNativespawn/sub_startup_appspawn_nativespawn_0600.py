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


class SubStartupAppspawnNativespawn0600(TestCase):

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
        hap = Common.sourcepath('entry-default-nativespawn.hap', "SUB_STARTUP_STABILITY_APPSPAWN")
        is_hap = self.driver.has_app("com.acts.childprocessmanager")
        if(is_hap):
            self.driver.uninstall_app("com.acts.childprocessmanager")
        else:
            pass
        self.driver.install_app(hap)
        Step("步骤2:打开测试应用")
        self.driver.start_app("com.acts.childprocessmanager")
        Step("步骤3:点击StartArk用例按钮")
        self.driver.touch(BY.text("StartArk用例"))
        Step("步骤4:点击start Ark Process按钮")
        self.driver.touch(BY.text("Start Ark Process"))
        Step("步骤5:获取测试应用pid")
        pid = self.driver.System.get_pid("com.acts.childprocessmanager")
        Step("步骤6:获取孵化app子进程pid")
        child_pid = self.driver.System.get_pid("com.acts.childprocessmanager:ArkProcess0")
        Step("步骤7:测试应用沙箱路径data/storage/el1生成一个123.txt")
        self.driver.shell("echo 123456789 > /proc/%d/root/data/storage/el1/123.txt" % pid)
        Step("步骤8:孵化app子进程查看沙箱路径data/storage/el1")
        childres = self.driver.shell("cat /proc/%d/root/data/storage/el1/123.txt" % child_pid)
        Step("步骤8:预期结果检验")
        self.driver.Assert.equal(int(childres), 123456789)
        self.driver.shell("rm -rf /proc/%d/root/data/storage/el1/123.txt" % pid)


    def teardown(self):
        Step("收尾工作:卸载测试应用")
        self.driver.uninstall_app("com.acts.childprocessmanager")