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

#include "hnp_base.h"
#include "hnp_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int HnpCliShowHelp(int argc, char *argv[]);

typedef int (*HNP_CMD_PROCESS_FUNC)(int argc, char *argv[]);

typedef struct NativeManagerCmdInfoStru {
    char *cmd;
    HNP_CMD_PROCESS_FUNC process;
} NativeManagerCmdInfo;

NativeManagerCmdInfo g_nativeManagerCmd[] = {
    {"help", HnpCliShowHelp},
    {"pack", HnpCmdPack}
};

int HnpCliShowHelp(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    HNP_LOGI("\r\nusage:hnpcli <command> <args> [-cfg <link cfg file>][-name <native package name>]"
        "[-v <native package version>]\r\n"
        "\r\nThese are common hnp commands used in various situations:\r\n"
        "\r\npack:    packet native software package to .hnp file"
        "\r\n         hnpcli pack [source path] [dst path] -name [software name] -v [software version]"
        "\r\n         hnpcli pack [source path] [dst path] -cfg [link config file]\r\n"
        "\r\nfor example:\r\n"
        "\r\n    hnpcli pack /usr1/native_sample /usr1/output -name native_sample -v 1.1\r\n"
        "    hnpcli pack /usr1/native_sample /usr1/output -cfg /usr1/native_sample.cfg\r\n");
        
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
        HnpCliShowHelp(argc, argv);
        return HNP_ERRNO_PARAM_INVALID;
    }

    HNP_LOGI("native manager process start.");

    /* 检验用户命令，获取对应的处理函数 */
    cmdInfo = HnpCmdCheck(argv[HNP_INDEX_1]);
    if (cmdInfo == NULL) {
        HNP_LOGE("invalid cmd!. cmd:%s", argv[HNP_INDEX_1]);
        return HNP_ERRNO_OPERATOR_TYPE_INVALID;
    }

    /* 执行命令 */
    ret = cmdInfo->process(argc, argv);

    HNP_LOGI("native manager process exit. ret=%d ", ret);
    return ret;
}

#ifdef __cplusplus
}
#endif