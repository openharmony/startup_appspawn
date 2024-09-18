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

#include <cerrno>
#include <cstring>
#include <dlfcn.h>
#include <set>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

#include "appspawn_hook.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "parameters.h"
#ifndef APPSPAWN_TEST
#include "main_thread.h"
#endif

APPSPAWN_STATIC int BuildFdInfoMap(const AppSpawnMsgNode *message, std::map<std::string, int> &fdMap, int isColdRun)
{
    APPSPAWN_CHECK_ONLY_EXPER(message != NULL && message->buffer != NULL, return -1);
    APPSPAWN_CHECK_ONLY_EXPER(message->tlvOffset != NULL, return -1);
    int findFdIndex = 0;
    AppSpawnMsgReceiverCtx recvCtx;
    if (!isColdRun) {
        APPSPAWN_CHECK_ONLY_EXPER(message->connection != NULL, return -1);
        recvCtx = message->connection->receiverCtx;
        if (recvCtx.fdCount <= 0) {
            APPSPAWN_LOGI("no need to build fd info %{public}d, %{public}d", recvCtx.fds != NULL, recvCtx.fdCount);
            return 0;
        }
    }
    for (uint32_t index = TLV_MAX; index < (TLV_MAX + message->tlvCount); index++) {
        if (message->tlvOffset[index] == INVALID_OFFSET) {
            return -1;
        }
        uint8_t *data = message->buffer + message->tlvOffset[index];
        if (((AppSpawnTlv *)data)->tlvType != TLV_MAX) {
            continue;
        }
        AppSpawnTlvExt *tlv = (AppSpawnTlvExt *)data;
        if (strcmp(tlv->tlvName, MSG_EXT_NAME_APP_FD) != 0) {
            continue;
        }
        std::string key((char *)data + sizeof(AppSpawnTlvExt));
        if (isColdRun) {
            std::string envKey = std::string(APP_FDENV_PREFIX) + key;
            char *fdChar = getenv(envKey.c_str());
            APPSPAWN_CHECK(fdChar != NULL, continue, "getfd from env failed %{public}s", envKey.c_str());
            int fd = atoi(fdChar);
            APPSPAWN_CHECK(fd > 0, continue, "getfd from env atoi errno %{public}s,%{public}d", envKey.c_str(), fd);
            fdMap[key] = fd;
        } else {
            APPSPAWN_CHECK(findFdIndex < recvCtx.fdCount && recvCtx.fds[findFdIndex] > 0,
                return -1, "invalid fd info  %{public}d %{public}d", findFdIndex, recvCtx.fds[findFdIndex]);
            fdMap[key] = recvCtx.fds[findFdIndex++];
            if (findFdIndex >= recvCtx.fdCount) {
                break;
            }
        }
    }
    return 0;
}

static int RunChildThread(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    std::string checkExit;
    if (OHOS::system::GetBoolParameter("persist.init.debug.checkexit", true)) {
        checkExit = std::to_string(getpid());
    }
    setenv(APPSPAWN_CHECK_EXIT, checkExit.c_str(), true);
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_CHILDPROCESS)) {
        std::map<std::string, int> fdMap;
        BuildFdInfoMap(property->message, fdMap, IsColdRunMode(content));
        AppSpawnEnvClear((AppSpawnContent *)&content->content, (AppSpawnClient *)&property->client);
        OHOS::AppExecFwk::MainThread::StartChild(fdMap);
    } else {
        AppSpawnEnvClear((AppSpawnContent *)&content->content, (AppSpawnClient *)&property->client);
        OHOS::AppExecFwk::MainThread::Start();
    }
    unsetenv(APPSPAWN_CHECK_EXIT);
    return 0;
}

static int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_CHECK(client != NULL && content != NULL, return -1, "Invalid client");
    AppSpawningCtx *property = reinterpret_cast<AppSpawningCtx *>(client);

    return RunChildThread(reinterpret_cast<AppSpawnMgr *>(content), property);
}

APPSPAWN_STATIC int PreLoadNativeSpawn(AppSpawnMgr *content)
{
    content->content.mode = MODE_FOR_NATIVE_SPAWN;
    RegChildLooper(&content->content, RunChildProcessor);
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load native module ...");
    AddPreloadHook(HOOK_PRIO_HIGHEST, PreLoadNativeSpawn);
}