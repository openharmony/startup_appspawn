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

import json
import time
from devicetest.core.test_case import TestCase, Step
from hypium import UiDriver
from hypium.model import UiParam
from aw import Common


class SubStartupAppspawnAtomservice0200(TestCase):
    def __init__(self, controllers):
        self.tag = self.__class__.__name__
        TestCase.__init__(self, self.tag, controllers)
        self.driver = UiDriver(self.device1)

    def setup(self):
        Step(self.devices[0].device_id)
        device = self.driver.shell("param get const.product.model")
        device = device.replace("\n", "").replace(" ", "")
        device = str(device)
        Step(device)

        Step("解锁屏幕")
        wake = self.driver.Screen.is_on()
        time.sleep(0.5)
        if wake:
            self.driver.ScreenLock.unlock()
        else:
            self.driver.Screen.wake_up()
            self.driver.ScreenLock.unlock()
        self.driver.Screen.enable_stay_awake()

    def login_account(self):
        Step("登录帐号")
        setting_name = "com.example.settings"
        with open(r"config/userLogin.json", 'r', encoding='UTF-8') as f:
            account = json.load(f)
        self.driver.AppManager.stop_app(setting_name)
        self.driver.AppManager.start_app(setting_name)
        self.driver.touch(BY.key("entry_image_account"))
        comps1 = self.driver.find_component(BY.key("inp_hwid_login_account_name_before"))
        if comps1:
            self.driver.input_text(comps1, account["userName"])
            self.driver.close_soft_keyboard()
            self.driver.touch(BY.key("button_text"))
            comps2 = self.driver.find_component(BY.key("inp_hwid_login_password"))
            self.driver.input_text(comps2, account["passWord"])
        else:
            comps3 = self.driver.find_component(BY.key("inp_hwid_login_account_name"))
            self.driver.clear_text(comps3)
            self.driver.input_text(comps3, account["userName"])
            comps4 = self.driver.find_component(BY.key("inp_hwid_login_password"))
            self.driver.input_text(comps4, account["passWord"])
        self.driver.touch(BY.key("button_text"))
        self.driver.wait_for_component(BY.text("继续"), timeout=15)
        self.driver.touch(BY.text("继续"))

    def exit_account(self):
        Step("退出帐号")
        setting_name = "com.example.settings"
        self.driver.AppManager.stop_app(setting_name)
        self.driver.AppManager.start_app(setting_name)
        self.driver.touch(BY.key("entry_image_account"))
        self.driver.swipe(UiParam.UP)
        comp = self.driver.find_component(BY.key("hwid_account_center_logout_text"), scroll_target=BY.type("Scroll"))
        self.driver.touch(comp)
        self.driver.touch(BY.key("button_text"))
        comps5 = self.driver.find_component(BY.key("advanced_dialog_button_1"))
        if comps5:
            self.driver.touch(comps5)
            time.sleep(5)
        else:
            pass

    def process(self):
        self.login_account()
        hap_info01, hap_info02 = [], []
        bundle_name = "com.atomicservice.5765880207854649689"
        bundle_name_start = "+auid-9E1E70ABFF4AAFE1E18AF18E0F75703713D" + "8678F9F9AB06C3B64C1CB1080615A+com.atomicservice.5765880207854649689"
        hap_path = Common.sourcepath('atomicservice.hap', "sub_startup_appspawn_atomservice")
        hap = self.driver.AppManager.has_app(bundle_name)
        if hap:
            self.driver.AppManager.clear_app_data(bundle_name)
            self.driver.AppManager.uninstall_app(bundle_name)
            self.driver.AppManager.install_app(hap_path)
        else:
            self.driver.AppManager.install_app(hap_path)

        for path in ["base", "database"]:
            for i in range(1, 5):
                has_dir = self.driver.Storage.has_dir("/data/app/el%d/100/%s/%s" % (i, path, bundle_name))
                has_dir_login = self.driver.Storage.has_dir("/data/app/el%d/100/%s/%s" % (i, path, bundle_name_start))
                self.driver.Assert.equal(has_dir, True)
                self.driver.Assert.equal(has_dir_login, False)

        log_has_dir = self.driver.Storage.has_dir("/data/app/el2/100/log/%s" % bundle_name)
        self.driver.Assert.equal(log_has_dir, True)

        mnt_has_dir = self.driver.Storage.has_dir("/mnt/share/100/%s" % bundle_name)
        mnt_has_dir_login = self.driver.Storage.has_dir("/mnt/share/100/%s" % bundle_name_start)
        self.driver.Assert.equal(mnt_has_dir, True)
        self.driver.Assert.equal(mnt_has_dir_login, False)

        self.driver.shell("aa start -a EntryAbility -b %s" % bundle_name)
        for path in ["base", "database"]:
            for i in range(1, 5):
                has_dir_login = self.driver.Storage.has_dir("/data/app/el%d/100/%s/%s" % (i, path, bundle_name_start))
                self.driver.Assert.equal(has_dir_login, True)

        Step("校验UGO、所有者、所属组、selinux标签一致")
        for i in range(1, 5):
            hap_detail = self.driver.shell("ls -lZ /data/app/el%d/100/%s" % (i, "base")).split("\n")
            for hap in hap_detail[1:-2]:
                if hap.split()[-1] == bundle_name:
                    hap_info01 = hap
                if hap.split()[-1] == bundle_name_start:
                    hap_info02 = hap
                    #校验用户字符串长度为64
            self.driver.Assert.equal(len(hap_info02.split()[-1].split("-")[1].split("+")[0]), 64)
            for index in range(2, 5):
                self.driver.Assert.equal(hap_info01.split()[index], hap_info02.split()[index])

        hap_detail = self.driver.shell("ls -lZ /mnt/share/100/").split("\n")
        for hap in hap_detail[1:-2]:
            if hap.split()[-1] == bundle_name:
                hap_info01 = hap
            if hap.split()[-1] == bundle_name_start:
                hap_info02 = hap
        self.driver.Assert.equal(len(hap_info02.split()[-1].split("-")[1].split("+")[0]), 64)
        for index in range(2, 5):
            self.driver.Assert.equal(hap_info01.split()[index], hap_info02.split()[index])
        self.exit_account()
        

    def teardown(self):
        Step("收尾工作.................")
        self.driver.AppManager.uninstall_app("com.atomicservice.5765880207854649689")
