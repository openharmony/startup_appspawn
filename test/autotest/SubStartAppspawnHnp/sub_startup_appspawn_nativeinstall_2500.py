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
import time
import threading
from hypium import *
from devicetest.core.test_case import Step, TestCase
from aw import Common


class SUB_STARTUP_APPSPAWN_NATIVEINSTALL_2500(TestCase):
    def __init__(self, controllers):
        self.TAG = self.__class__.__name__
        self.tests = [
            "test_step1"
        ]
        TestCase.__init__(self, self.TAG, controllers)

        self.default_install_root_path = "/data/app/el1/bundle"
        self.current_path = os.getcwd()
        self.driver = UiDriver(self.device1)
        self.work_space_path = "/data/hnp_test"
        self.pack_out_path = f"{self.work_space_path}/pack_output"
        self.hnp_tools_path = f"{self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/hnp"
        self.hnp_test_file_path = f"{self.current_path}\\testFile\SUB_STARTUP_APPSPAWN_HNP"
        self.hnp_commamd = f"./{self.hnp_tools_path}/hnp"
        self.hnpcli_commamd = f"./{self.hnp_tools_path}/hnpcli"
        self.hnp_public_dir = "hnppublic/bin"
        self.native_path = f"{self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/native/compute"

    def creat_json_and_push_remote(self, json_file_name, json_data, target_path):
        Common.writeJsonDateToFile(json_file_name, json_data)
        self.driver.push_file(f"{self.current_path}\\{json_file_name}", target_path)
        assert Common.deleteFile(json_file_name) == 0

    def worker(self):
        Step("执行安装后的软件")
        self.driver.shell(f"chmod +x /data/service/hnp/compute.org/compute_1.0/bin/add")
        result = self.driver.shell(
            f"./data/service/hnp/compute.org/compute_1.0/bin/add 10")
        self.driver.Assert.contains(result, "End: 1 + 1 = 2")

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

    # 重复安装升级版本的hnp包时，工具正在使用
    def test_step1(self):
        Step("识别设备型号...............................")
        device = self.driver.shell("param get const.product.model")
        device = device.replace("\n", "").replace(" ", "")
        device = str(device)
        if ("HYM" in device or "HAD" in device or "ALN" in device):
                Step("创建compute工具配置文件，工具add的软连接名称为compute")
                hnp_cfg_data = Common.readJsonFile("testFile\SUB_STARTUP_APPSPAWN_HNP\hnp.json")
                hnp_cfg_data['install']['links'][0]["source"] = "bin/add"
                hnp_cfg_data['install']['links'][0]["target"] = "compute"
                hnp_cfg_data['version'] = "1.0"
                hnp_cfg_data['name'] = "compute"
                self.creat_json_and_push_remote("hnp.json", hnp_cfg_data, self.native_path)

                Step("打包compute工具")
                result = self.driver.shell(
                    f"{self.hnpcli_commamd} pack -i {self.native_path} -o {self.pack_out_path}/public")
                self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0")

                self.driver.shell(f'mkdir -p /data/service/hnp/compute.org/compute_1.0/')
                self.driver.shell(f'cp -r {self.work_space_path}/SUB_STARTUP_APPSPAWN_HNP/native/compute/bin '
                                  f'/data/service/hnp/compute.org/compute_1.0/')

                Step("安装compute工具")
                result = self.driver.shell(
                    f"{self.hnp_commamd} install -u 300 -p baidu -i {self.pack_out_path}")
                self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0")

                t1 = threading.Thread(target=self.worker)
                t1.start()
                time.sleep(1)
                Step("创建compute工具配置文件，工具del的软连接名称为compute,版本为2.0")
                hnp_cfg_data = Common.readJsonFile("testFile\SUB_STARTUP_APPSPAWN_HNP\hnp.json")
                hnp_cfg_data['install']['links'][0]["source"] = "bin/del"
                hnp_cfg_data['install']['links'][0]["target"] = "del"
                hnp_cfg_data['version'] = "2.0"
                hnp_cfg_data['name'] = "compute"
                self.driver.shell(f"rm {self.native_path}/hnp.json")
                self.creat_json_and_push_remote("hnp.json", hnp_cfg_data, self.native_path)

                Step("打包升级后的compute.hnp")
                result = self.driver.shell(
                    f"{self.hnpcli_commamd} pack -i {self.native_path} -o {self.pack_out_path}/public")
                self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=0")

                Step("重复强制安装不同版本的hnp包")
                result = self.driver.shell(
                    f"{self.hnp_commamd} install -u 300 -p baidu -i {self.pack_out_path} -f")
                self.driver.Assert.contains(result, "[ERROR][HNP]hnp install path is running now,")
                self.driver.Assert.contains(result, "[INFO][HNP]native manager process exit. ret=8392978")
                t1.join()

                Step("卸载安装好的native软件包")
                self.driver.shell(f"{self.hnp_commamd} uninstall -u 300 -p baidu")
        else:
            pass

    def teardown(self):
        self.driver.shell(f'rm -rf {self.work_space_path}')
        self.driver.shell(f'rm -rf {self.default_install_root_path}/300/')
        Step("收尾工作")

