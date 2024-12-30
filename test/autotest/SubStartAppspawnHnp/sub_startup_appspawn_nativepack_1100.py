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


class SUB_STARTUP_APPSPAWN_NATIVEPACK_1100(TestCase):
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

    # hnpcli 打包native包传参个数异常场景
    def test_step1(self):
        Step("执行./hnpcli pack -i native包路径")
        result = self.driver.shell(f"{self.hnpcli_commamd} pack -i {self.native_path[0]}")
        self.driver.Assert.contains(result, "[ERROR][HNP]name or version argv is miss.")

        Step("执行./hnpcli pack -o 存储包目标路径")
        result = self.driver.shell(f"{self.hnpcli_commamd} pack -o {self.pack_out_path[0]}")
        self.driver.Assert.contains(result, "[ERROR][HNP]source dir is null.")

        Step("执行./hnpcli 传参等于四个参数，./hnpcli pack -i native包路径 -o 存储包目标路径，native包中无cfg配置文件")
        result = self.driver.shell(f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path}")
        self.driver.Assert.contains(result, "[ERROR][HNP]name or version argv is miss.")

    def teardown(self):
        self.driver.shell(f'rm -rf {self.work_space_path}')
        Step("收尾工作")

