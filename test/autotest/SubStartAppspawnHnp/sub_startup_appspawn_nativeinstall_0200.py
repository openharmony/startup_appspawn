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

import os
from hypium import *
from devicetest.core.test_case import Step, TestCase


class SUB_STARTUP_APPSPAWN_NATIVEINSTALL_0200(TestCase):
    def __init__(self, controllers):
        self.TAG = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)

        self.default_install_root_path = "/data/app/el1/bundle"
        self.current_path = os.getcwd()
        self.driver = UiDriver(self.device1)
        self.work_space_path = "/data/hnp_test"  # 测试设备工作路径
        self.pack_out_path = f"{self.work_space_path}/pack_output"
        self.hnp_tools_path = f"{self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/hnp"
        self.hnp_test_file_path = f"{self.current_path}\\testFile\SUB_STARTUP_APPSPAWN_HNP"
        self.hnp_commamd = f"./{self.hnp_tools_path}/hnp"
        self.hnpcli_commamd = f"./{self.hnp_tools_path}/hnpcli"
        self.native_path = f"{self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/native/sample1"

    def setup(self):
        Step("预置工作:初始化PC开始")
        Step(self.devices[0].device_id)

        Step("预置工作:创建测试目录，将测试文件拷贝到测试PC")
        self.driver.shell(f'mkdir {self.work_space_path}')

        Step("预置工作:创建打包保存文件，为native打包后的保存路径")
        self.driver.shell(f'mkdir {self.pack_out_path}')
        self.driver.shell(f'mkdir {self.pack_out_path}/public')

        self.driver.push_file(self.hnp_test_file_path, self.work_space_path)

        Step("预置工作:进入到bin文件目录，bin文件增加可执行权限")
        self.driver.shell(f"chmod +x {self.hnp_tools_path}/hnpcli")
        self.driver.shell(f"chmod +x {self.hnp_tools_path}/hnp")

        Step("创建一个userId为300的路径，为后续测试使用目录")
        self.driver.shell(f"mkdir {self.default_install_root_path}/300")

    # hnp进程安装传参校验
    def test_step1(self):
        Step("识别设备型号...............................")
        device = self.driver.shell("param get const.product.model")
        device = device.replace("\n", "").replace(" ", "")
        device = str(device)
        if ("HYM" in device or "HAD" in device or "ohos" in device or "ALN" in device):
            Step("执行./hnp install")
            result = self.driver.shell(
                f"{self.hnp_commamd} install")
            self.driver.Assert.contains(result, "native manager process exit. ret=8392706")
            self.driver.Assert.contains(result, "[ERROR][HNP]hnp install params invalid,uid[(null)],hnp src path[(null)], package name[(null)]")

            Step("执行./hnp install -u 300, 只传userId")
            result = self.driver.shell(
                f"{self.hnp_commamd} install -u 300")
            self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=8392706 ")

            Step("执行./hnp install -p baidu, 只传应用软件名")
            result = self.driver.shell(
                f"{self.hnp_commamd} install -p baidu")
            self.driver.Assert.contains(result,
                                        "[ERROR][HNP]hnp install params invalid,uid[(null)],"
                                        "hnp src path[(null)], package name[baidu]")

            Step("执行./hnp install -i hnppath, 只传安装路径")
            result = self.driver.shell(
                f"{self.hnpcli_commamd} pack -i {self.native_path} -o {self.pack_out_path}/public/ -n hnp_test1 -v 1.3")
            self.driver.Assert.contains(result, "linkNum=0, ret=0")
            result = self.driver.shell(f"find {self.pack_out_path}/public/ -name hnp_test1.hnp")
            self.driver.Assert.contains(result, f"{self.pack_out_path}/public/hnp_test1.hnp")

            result = self.driver.shell(
                f"{self.hnp_commamd} install -i {self.pack_out_path}")
            self.driver.Assert.contains(result, "[ERROR][HNP]hnp install params invalid,"
                                                "uid[(null)],hnp src path[/data/hnp_test/pack_output], "
                                                "package name[(null)]")

            Step("执行./hnp install -f, 只传-f")
            result = self.driver.shell(
                f"{self.hnp_commamd} install -f")
            self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=8392706")

            Step("执行./hnp uninstall")
            result = self.driver.shell(
                f"{self.hnp_commamd} uninstall")
            self.driver.Assert.contains(result,
                                        "[ERROR][HNP]hnp uninstall params invalid uid[(null)], package name[(null)]")

            Step("执行./hnp pack")
            result = self.driver.shell(
                f"{self.hnp_commamd} pack")
            self.driver.Assert.contains(result, "[ERROR][HNP]invalid cmd!. cmd:pack")

            Step("执行./hnp help")
            result = self.driver.shell(
                f"{self.hnp_commamd} help")
            self.driver.Assert.contains(result, "install: install one hap package")
            self.driver.Assert.contains(result, "uninstall: uninstall one hap package")

            Step("执行./hnp -h")
            result = self.driver.shell(
                f"{self.hnp_commamd} -h")
            self.driver.Assert.contains(result, "install: install one hap package")
            self.driver.Assert.contains(result, "uninstall: uninstall one hap package")
        else:
            pass

    def teardown(self):
        self.driver.shell(f'rm -rf {self.work_space_path}')
        self.driver.shell(f'rm -rf {self.default_install_root_path}/300/')
        Step("收尾工作")

