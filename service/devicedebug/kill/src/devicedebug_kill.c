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
#include <stdlib.h>
#include <signal.h>

#include "appspawn.h"
#include "appspawn_utils.h"

#include "devicedebug_base.h"
#include "devicedebug_kill.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEVICEDEBUG_KILL_CMD_NUM 4
#define DEVICEDEBUG_KILL_CMD_SIGNAL_INDEX 2
#define DEVICEDEBUG_KILL_CMD_PID_INDEX 3

APPSPAWN_STATIC int DeviceDebugKill(char *signal, char *pid)
{
    AppSpawnClientHandle clientHandle;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    if (ret != 0) {
        DEVICEDEBUG_LOGE("devicedebug appspawn client init unsuccess, ret=%{public}d", ret);
        return ret;
    }

    AppSpawnReqMsgHandle reqHandle;
    ret = AppSpawnReqMsgCreate(MSG_DEVICE_DEBUG, "devicedebug", &reqHandle);
    if (ret != 0) {
        DEVICEDEBUG_LOGE("devicedebug appspawn message create unsuccess, ret=%{public}d", ret);
        return ret;
    }

    ret = AppSpawnReqMsgAddStringInfo(reqHandle, "signal", signal);
    if (ret != 0) {
        DEVICEDEBUG_LOGE("devicedebug appspawn message add signal unsuccess, ret=%{public}d", ret);
        return ret;
    }

    ret = AppSpawnReqMsgAddStringInfo(reqHandle, "pid", pid);
    if (ret != 0) {
        DEVICEDEBUG_LOGE("devicedebug appspawn message add pid unsuccess, ret=%{public}d", ret);
        return ret;
    }

    AppSpawnResult result = {0};
    ret = AppSpawnClientSendMsg(clientHandle, reqHandle, &result);
    AppSpawnClientDestroy(clientHandle);
    if (ret != 0) {
        DEVICEDEBUG_LOGE("devicedebug appspawn send msg unsuccess, ret=%{public}d", ret);
        return ret;
    }

    if (result.result != 0) {
        DEVICEDEBUG_LOGE("devicedebug appspawn kill process unsuccess, result=%{public}d", result.result);
        return result.result;
    }

    return 0;
}

int DeviceDebugCmdKill(int argc, char *argv[])
{
    if (!IsDeveloperModeOpen()) {
        return DEVICEDEBUG_ERRNO_NOT_IN_DEVELOPER_MODE;
    }

    if (argc < DEVICEDEBUG_KILL_CMD_NUM) {
        DEVICEDEBUG_LOGE("devicedebug cmd operator num is %{public}d < %{public}d", argc, DEVICEDEBUG_KILL_CMD_NUM);
        return DEVICEDEBUG_ERRNO_OPERATOR_ARGV_MISS;
    }

    int signal = atoi(argv[DEVICEDEBUG_KILL_CMD_SIGNAL_INDEX] + 1);
    if (signal > SIGRTMAX || signal <= 0) {
        DEVICEDEBUG_LOGE("signal is %{public}d > %{public}d", signal, SIGRTMAX);
        return DEVICEDEBUG_ERRNO_PARAM_INVALID;
    }

    int pid = atoi(argv[DEVICEDEBUG_KILL_CMD_PID_INDEX]);
    DEVICEDEBUG_LOGI("devicedebug cmd kill start signal[%{public}d], pid[%{public}d]", signal, pid);

    return DeviceDebugKill(argv[DEVICEDEBUG_KILL_CMD_SIGNAL_INDEX], argv[DEVICEDEBUG_KILL_CMD_PID_INDEX]);
}

#ifdef __cplusplus
}
#endif
