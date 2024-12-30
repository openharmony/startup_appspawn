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
import shutil
from hypium import *
from aw import Common
from devicetest.core.test_case import Step, TestCase


class SUB_STARTUP_APPSPAWN_NATIVEPACK_1400(TestCase):
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
        self.hnp_cfg_path = f"{self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/hnp.json"
        self.hnp_commamd = f"./{self.hnp_tools_path}/hnp"
        self.hnpcli_commamd = f"./{self.hnp_tools_path}/hnpcli"

    def check_pack_is_correct(self, hnp_name, native_name, cfg_file, del_hnp):
        # 校验是否生成了对应的.hnp文件
        result = self.driver.shell(f"find {self.pack_out_path} -name {hnp_name}")
        self.driver.Assert.contains(result, f"{self.pack_out_path}/{hnp_name}")

        # 将生成的.hnp文件推到本地
        self.driver.pull_file(self.pack_out_path, self.hnp_test_file_path)

        # 是否删除生成的.hnp文件
        if del_hnp:
            self.driver.shell(f'rm {self.pack_out_path}/{hnp_name}')

        # 将hnp.json文件拷贝到native路径下，为文件比较做好准备工作
        shutil.copy2(cfg_file, f"{self.hnp_test_file_path}\\native\\{native_name}")
        Common.unzip_hnp(f"{self.hnp_test_file_path}/pack_output/{hnp_name}",
                         f"{self.hnp_test_file_path}/pack_output/")

        # 比较打包前后两个文件是否一致
        result = Common.compare_folder(f"{self.hnp_test_file_path}\\native\\{native_name}",
                                       f"{self.hnp_test_file_path}/pack_output/{native_name}")
        assert result == True

        # 删除测试PC中的残留文件
        Common.deleteFile(f"{self.hnp_test_file_path}\\native\\{native_name}\\hnp.json")
        shutil.rmtree(f"{self.hnp_test_file_path}/pack_output/")

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

    # hnp程序打包native包，cfg相关测试场景
    def test_step1(self):
        Step("native包中无cfg配置文件，不传name与version")
        result = self.driver.shell(
            f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path}")
        self.driver.Assert.contains(result, "[ERROR][HNP]name or version argv is miss.")
        Step("native包中无cfg配置文件，传name与version")
        result = self.driver.shell(
            f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path} -n test -v 1.1")
        self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0")
        result = self.driver.shell(f"find {self.pack_out_path} -name test.hnp")
        self.driver.Assert.contains(result, f"{self.pack_out_path}/test.hnp")
        self.driver.shell(f'rm {self.pack_out_path}/test.hnp')
        Step("native包中存在cfg配置文件，不传name与version")
        self.driver.shell(f'cp -i {self.hnp_cfg_path} {self.native_path[0]}')
        result = self.driver.shell(
            f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path}")
        self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0")
        self.check_pack_is_correct("hnp_test.hnp", "sample1", f"{self.hnp_test_file_path}\\hnp.json", True)
        self.driver.shell(f'rm {self.native_path[0]}/hnp.json')
        Step("native包中存在cfg配置文件，传name与version")
        self.driver.shell(f'cp -i {self.hnp_cfg_path} {self.native_path[0]}')
        result = self.driver.shell(
            f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path} -n test -v 1.1")
        self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0")
        self.check_pack_is_correct("hnp_test.hnp", "sample1", f"{self.hnp_test_file_path}\\hnp.json", True)
        self.driver.shell(f'rm {self.native_path[0]}/hnp.json')
        Step(
            "调用./hnpcli pack -i native包路径 -o 存储包目标路径 -name test -v test, version为字符串，非常规version表述")
        result = self.driver.shell(
            f"{self.hnpcli_commamd} pack -i {self.native_path[0]} -o {self.pack_out_path} -n test -v test")
        self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0")
        self.driver.Assert.contains(result, "hnpName=test, hnpVer=test")
        result = self.driver.shell(f"find {self.pack_out_path} -name test.hnp")
        self.driver.Assert.contains(result, f"{self.pack_out_path}/test.hnp")
        self.driver.shell(f'rm {self.pack_out_path}/test.hnp')
        self.driver.shell(f'rm {self.native_path[0]}/hnp.json')

    def teardown(self):
        self.driver.shell(f'rm -rf {self.work_space_path}')
        Step("收尾工作")

