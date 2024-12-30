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
from aw import Common
from devicetest.core.test_case import Step, TestCase


class SUB_STARTUP_APPSPAWN_NATIVEPACK_2100(TestCase):
    def __init__(self, controllers):
        self.TAG = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]

        TestCase.__init__(self, self.TAG, controllers)
        self.current_path = os.getcwd()
        self.driver = UiDriver(self.device1)
        self.work_space_path = "/data/hnp_test"  # 测试设备工作路径
        self.pack_out_path = f"{self.work_space_path}/pack_output"
        self.native_path = [f"{self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/native/sample1",
                            f"{self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/native/sample2"]
        self.hnp_tools_path = f"{self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/hnp"
        self.hnp_test_file_path = f"{self.current_path}\\testFile\SUB_STARTUP_APPSPAWN_HNP"
        self.hnpcli_commamd = f"./{self.hnp_tools_path}/hnpcli"

    def creat_json_and_push_remote(self, json_file_name, json_data, target_path):
        Common.writeJsonDateToFile(json_file_name, json_data)
        self.driver.push_file(f"{self.current_path}\\{json_file_name}", target_path)
        assert Common.deleteFile(json_file_name) == 0

    def setup(self):
        Step("预置工作:初始化PC开始")
        Step(self.devices[0].device_id)

        Step("预置工作:创建测试目录，将测试文件拷贝到测试PC")
        self.driver.shell(f'mkdir {self.work_space_path}')
        self.driver.push_file(self.hnp_test_file_path, self.work_space_path)

        Step("预置工作:创建打包保存文件，为native打包后的保存路径")
        self.driver.shell(f'mkdir {self.pack_out_path}')

        Step("预置工作:进入到bin文件目录，bin文件增加可执行权限")
        self.driver.shell(f"chmod +x {self.hnp_tools_path}/hnpcli")

    # 升级打包同名native包
    def test_step1(self):
        Step("识别设备型号...............................")
        device = self.driver.shell("param get const.product.model")
        device = device.replace("\n", "").replace(" ", "")
        device = str(device)
        if ("HYM" in device or "HAD" in device or "ohos" in device or "ALN" in device):
            Step("配置文件字段正确，存在两个软连接，不同的软连接路径，相同的配置名称")
            hnp_cfg_data = Common.readJsonFile("testFile\SUB_STARTUP_APPSPAWN_HNP\hnp.json")
            self.creat_json_and_push_remote("hnp.json", hnp_cfg_data, self.native_path[0])

            result = self.driver.shell(
                f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path}")
            self.driver.Assert.contains(result, "linkNum=1, ret=0")
            self.driver.shell(f"rm {self.native_path[0]}/hnp.json")

            Step("根据name + version打包与配置文件的同名hnp包")
            result = self.driver.shell(
                f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path} -n hnp_test -v 1.0")
            self.driver.Assert.contains(result, "linkNum=0, ret=0")
            result = self.driver.shell(f"find {self.pack_out_path} -name hnp_test.hnp")
            self.driver.Assert.contains(result, f"{self.pack_out_path}/hnp_test.hnp")

            Step("根据name + version打包与配置文件的同名hnp包，版本号不同")
            result = self.driver.shell(
                f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path} -n hnp_test -v 1.2")
            self.driver.Assert.contains(result, "linkNum=0, ret=0")
            result = self.driver.shell(f"find {self.pack_out_path} -name hnp_test.hnp")
            self.driver.Assert.contains(result, f"{self.pack_out_path}/hnp_test.hnp")

            Step("根据name + version打包与配置文件的同名hnp包，打包的native包与原有不同，版本号不同")
            result = self.driver.shell(
                f"{self.hnpcli_commamd} pack -i {self.native_path[1]} -o {self.pack_out_path} -n hnp_test1 -v 1.3")
            self.driver.Assert.contains(result, "linkNum=0, ret=0")
            result = self.driver.shell(f"find {self.pack_out_path} -name hnp_test1.hnp")
            self.driver.Assert.contains(result, f"{self.pack_out_path}/hnp_test1.hnp")
        else:
            pass

    def teardown(self):
        self.driver.shell(f'rm -rf {self.work_space_path}')
        Step("收尾工作")

