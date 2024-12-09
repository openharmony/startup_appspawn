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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "devicedebug_base.h"
#include "devicedebug_kill.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*DEVICEDEBUG_CMD_PROCESS_FUNC)(int argc, char *argv[]);

typedef struct DeviceDebugManagerCmdInfoStru {
    char *cmd;
    DEVICEDEBUG_CMD_PROCESS_FUNC process;
} DeviceDebugManagerCmdInfo;

APPSPAWN_STATIC int DeviceDebugShowHelp(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    printf("\r\nusage: devicedebug <command> <options>\r\n"
        "\r\nThese are common devicedebug commands list:\r\n"
        "\r\n         help              list available commands"
        "\r\n         kill              send a signal(1-64) to a process\r\n");

    return 0;
}

DeviceDebugManagerCmdInfo g_deviceDebugManagerCmd[] = {
    {"help", DeviceDebugShowHelp},
    {"-h", DeviceDebugShowHelp},
    {"kill", DeviceDebugCmdKill},
};

APPSPAWN_STATIC DeviceDebugManagerCmdInfo* DeviceDebugCmdCheck(const char *cmd)
{
    int cmdNum = sizeof(g_deviceDebugManagerCmd) / sizeof(DeviceDebugManagerCmdInfo);

    for (int i = 0; i < cmdNum; i++) {
        if (!strcmp(cmd, g_deviceDebugManagerCmd[i].cmd)) {
            return &g_deviceDebugManagerCmd[i];
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    int ret = 0;
    DeviceDebugManagerCmdInfo *cmdInfo = NULL;

    if (argc < DEVICEDEBUG_NUM_2) {
        DeviceDebugShowHelp(argc, argv);
        return DEVICEDEBUG_ERRNO_PARAM_INVALID;
    }

    DEVICEDEBUG_LOGI("devicedebug manager process start.");

    /* 检验用户命令，获取对应的处理函数 */
    cmdInfo = DeviceDebugCmdCheck(argv[DEVICEDEBUG_NUM_1]);
    if (cmdInfo == NULL) {
        printf("devicedebug: '%s' is not a valid devicedebug command. See 'devicedebug help'.",
            argv[DEVICEDEBUG_NUM_1]);
        DeviceDebugShowHelp(argc, argv);
        return DEVICEDEBUG_ERRNO_OPERATOR_TYPE_INVALID;
    }

    /* 执行命令 */
    ret = cmdInfo->process(argc, argv);

    /* 返回值依赖此条log打印，切勿随意修改 */
    DEVICEDEBUG_LOGI("devicedebug manager process exit. ret=%{public}d \r\n", ret);
    return ret;
}

#ifdef __cplusplus
}
#endif