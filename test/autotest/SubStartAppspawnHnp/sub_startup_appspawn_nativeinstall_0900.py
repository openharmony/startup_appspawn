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


class SUB_STARTUP_APPSPAWN_NATIVEINSTALL_0900(TestCase):
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
        self.driver.shell(f"chmod +x {self.hnp_tools_path}/hnp")
        self.driver.shell(f"chmod +x {self.hnp_tools_path}/hnpcli")

        Step("创建一个userId为300的路径，为后续测试使用目录")
        self.driver.shell(f"mkdir {self.default_install_root_path}/300")

    # 打包文件未设置软链接路径，安装hnp包时可执行文件在bin目录,且bin目录下仅存在一个文件夹
    def test_step1(self):
        Step("识别设备型号...............................")
        device = self.driver.shell("param get const.product.model")
        device = device.replace("\n", "").replace(" ", "")
        device = str(device)
        if ("HYM" in device or "HAD" in device or "ohos" in device or "ALN" in device):
            Step("创建一个temp_dir目录，创建bin/test目录")
            self.driver.shell(f"mkdir {self.work_space_path}/temp_dir/")
            self.driver.shell(f"mkdir {self.work_space_path}/temp_dir/bin/")
            self.driver.shell(f"mkdir {self.work_space_path}/temp_dir/bin/test/")

            Step("打包该native文件为test.hnp")
            result = self.driver.shell(
                f"{self.hnpcli_commamd} pack -i {self.work_space_path}/temp_dir/ "
                f"-o {self.pack_out_path}/public -n test -v 1.0")
            self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0")

            Step("强制安装test.hnp")
            result = self.driver.shell(
                f"{self.hnp_commamd} install -u 300 -p baidu -i {self.pack_out_path} -f")
            self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0 ")

            Step("验证是否存在test.org")
            result = self.driver.shell(f"find {self.default_install_root_path}/300/hnppublic -name test.org")
            self.driver.Assert.contains(result, "test.org")

            result = self.driver.shell(f"find {self.default_install_root_path}/300/hnppublic -name bin")
            self.driver.Assert.contains(result, "bin")

            Step("验证bin目录下是否存在test文件夹")
            result = self.driver.shell(f"find {self.default_install_root_path}/300/hnppublic/bin -name test")
            assert ("test" not in result) == True

            Step("清除安装内容")
            self.driver.shell(f"rm -rf {self.work_space_path}/temp_dir/")

            Step("卸载安装的工具")
            result = self.driver.shell(
                f"{self.hnp_commamd} uninstall -u 300 -p baidu")
            self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0 ")
        else:
            pass

    def teardown(self):
        self.driver.shell(f'rm -rf {self.work_space_path}')
        self.driver.shell(f'rm -rf {self.default_install_root_path}/300/')
        Step("收尾工作")

