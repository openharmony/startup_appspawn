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
    {"install", HnpCmdInstall},
    {"uninstall", HnpCmdUnInstall}
};

int HnpShowHelp(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    HNP_LOGI("\r\nusage:hnp <command> <args>\r\n"
        "\r\nThese are common hnp commands used in various situations:\r\n"
        "\r\nIf the [package name] is null, it represents a public install/nuninstall,\r\n"
        "    or it represents a private install/nuninstall\r\n"
        "\r\ninstall:      install native software"
        "\r\n              hnp install [--user id] [--hnp package dir] [--package name] <-f>\r\n"
        "\r\n              -u, --user id (required param)\r\n"
        "\r\n              -p, --hnp software name, support multiple (required param)\r\n"
        "\r\n              -i, --hnp install dir (optional param)\r\n"
        "\r\n              -f,  is force install (optional param)\r\n"
        "\r\nuninstall:    uninstall native software"
        "\r\n              hnp uninstall [--user id] [--software name] [--software version] [--package name]\r\n"
        "\r\n              -u, --user id (required param)\r\n"
        "\r\n              -n, --software name (required param)\r\n"
        "\r\n              -v, --software version (required param)\r\n"
        "\r\n              -i, --hnp install dir (optional param)\r\n"
        "\r\nfor example:\r\n"
        "\r\n    hnp install -u 1000 -p /usr1/hnp/sample.hnp -p /usr1/hnp/sample2.hnp -i wechat -f\r\n"
        "    hnp uninstall -u 1000 -n native_sample -v 1.1 -i wechat\r\n");

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
    int opt;
    NativeManagerCmdInfo *cmdInfo = NULL;

    if (argc < HNP_INDEX_2) {
        HnpShowHelp(argc, argv);
        return HNP_ERRNO_PARAM_INVALID;
    }

    HNP_LOGI("native manager process start.");

    while ((opt = getopt(argc, argv, "hu:p:ifn:v:")) != -1) {
        switch (opt) {
            case 'h' :
                HnpShowHelp(argc, argv);
                return 0;
            default:
                break;
        }
    }

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