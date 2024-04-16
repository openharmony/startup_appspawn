/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <unistd.h>

#include "hnp_base.h"
#include "hnp_installer.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int HnpShowHelp(int argc, char *argv[]);

typedef int (*HNP_CMD_PROCESS_FUNC)(int argc, char *argv[]);

typedef struct NativeManagerCmdInfoStru {
    char *cmd;
    HNP_CMD_PROCESS_FUNC process;
} NativeManagerCmdInfo;

NativeManagerCmdInfo g_nativeManagerCmd[] = {
    {"help", HnpShowHelp},
    {"-h", HnpShowHelp},
    {"install", HnpCmdInstall},
    {"uninstall", HnpCmdUnInstall}
};

int HnpShowHelp(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    HNP_LOGI("\r\nusage:hnp <command> <args> [-u <user id>][-p <software package path>]"
        "[-i <private path>][-f]\r\n"
        "\r\nThese are common hnp commands used in various situations:\r\n"
        "\r\ninstall: install one or more native hnp packages"
        "\r\n           hnp install <-u [user id]> <-p [hnp package path]> <-i [hnp install path]> <-f>"
        "\r\n           -u    : [required]    user id"
        "\r\n           -p    : [required]    path of hnp package to be installed, multiple packages are supported"
        "\r\n           -i    : [optional]    hnp install path; if not provided, it will be installed to"
            " public hnp path"
        "\r\n           -f    : [optional]    if provided, the hnp package will be installed forcely, ignoring old"
            " versions of the hnp package\r\n"
        "\r\nuninstall: uninstall one hnp package"
        "\r\n           hnp uninstall <-u [user id]> <-n [hnp package name]> <-v [hnp package version]>"
            " <-i [hnp package uninstall path]>"
        "\r\n           -u    : [required]    user id"
        "\r\n           -n    : [required]    hnp package name"
        "\r\n           -v    : [required]    hnp package version"
        "\r\n           -i    : [optional]    the path for uninstalling the hnp package; if not provided, it will"
            " install from the default public hnp path\r\n"
        "\r\nfor example:\r\n"
        "\r\n    hnp install -u 1000 -p /usr1/hnp/sample.hnp -p /usr1/hnp/sample2.hnp -i /data/app/el1/bundle/ -f"
        "\r\n    hnp uninstall -u 1000 -n native_sample -v 1.1 -i /data/app/el1/bundle/\r\n");

    return 0;
}

static NativeManagerCmdInfo* HnpCmdCheck(const char *cmd)
{
    int i;
    int cmdNum = sizeof(g_nativeManagerCmd) / sizeof(NativeManagerCmdInfo);

    for (i = 0; i < cmdNum; i++) {
        if (!strcmp(cmd, g_nativeManagerCmd[i].cmd)) {
            return &g_nativeManagerCmd[i];
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int ret;
    NativeManagerCmdInfo *cmdInfo = NULL;

    if (argc < HNP_INDEX_2) {
        HnpShowHelp(argc, argv);
        return HNP_ERRNO_PARAM_INVALID;
    }

    HNP_LOGI("native manager process start.");

    /* 检验用户命令，获取对应的处理函数 */
    cmdInfo = HnpCmdCheck(argv[HNP_INDEX_1]);
    if (cmdInfo == NULL) {
        HNP_LOGE("invalid cmd!. cmd:%s\r\n", argv[HNP_INDEX_1]);
        return HNP_ERRNO_OPERATOR_TYPE_INVALID;
    }

    /* 执行命令 */
    ret = cmdInfo->process(argc, argv);
    if (ret == HNP_ERRNO_OPERATOR_ARGV_MISS) {
        HnpShowHelp(argc, argv);
    }

    HNP_LOGI("native manager process exit. ret=%d \r\n", ret);
    return ret;
}

#ifdef __cplusplus
}
#endif