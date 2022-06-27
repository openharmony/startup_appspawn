/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "appspawn_server.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef OHOS_DEBUG
#include <time.h>
#endif // OHOS_DEBUG

static void NotifyResToParent(struct AppSpawnContent_ *content, AppSpawnClient *client, int result)
{
    if (content->notifyResToParent != NULL) {
        content->notifyResToParent(content, client, result);
    }
}

static void ProcessExit(void)
{
    APPSPAWN_LOGI("App exit %d.", getpid());
#ifndef APPSPAWN_TEST
#ifdef OHOS_LITE
    _exit(0x7f); // 0x7f user exit
#else
    quick_exit(0);
#endif
#endif
}

int DoStartApp(struct AppSpawnContent_ *content, AppSpawnClient *client, char *longProcName, uint32_t longProcNameLen)
{
    APPSPAWN_LOGI("DoStartApp id %d longProcNameLen %u", client->id, longProcNameLen);
    int32_t ret = 0;

    if (content->setAppSandbox) {
        ret = content->setAppSandbox(content, client);
        APPSPAWN_CHECK(ret == 0, NotifyResToParent(content, client, ret);
            return ret, "Failed to set app sandbox");
    }

    if (content->setKeepCapabilities) {
        ret = content->setKeepCapabilities(content, client);
        APPSPAWN_CHECK(ret == 0, NotifyResToParent(content, client, ret);
            return ret, "Failed to set KeepCapabilities");
    }

    if (content->setProcessName) {
        ret = content->setProcessName(content, client, longProcName, longProcNameLen);
        APPSPAWN_CHECK(ret == 0, NotifyResToParent(content, client, ret);
            return ret, "Failed to set setProcessName");
    }

    if (content->setUidGid) {
        ret = content->setUidGid(content, client);
        APPSPAWN_CHECK(ret == 0, NotifyResToParent(content, client, ret);
            return ret, "Failed to setUidGid");
    }

    if (content->setFileDescriptors) {
        ret = content->setFileDescriptors(content, client);
        APPSPAWN_CHECK(ret == 0, NotifyResToParent(content, client, ret);
            return ret, "Failed to setFileDescriptors");
    }

    if (content->setCapabilities) {
        ret = content->setCapabilities(content, client);
        APPSPAWN_CHECK(ret == 0, NotifyResToParent(content, client, ret);
            return ret, "Failed to setCapabilities");
    }

    // notify success to father process and start app process
    NotifyResToParent(content, client, 0);
    return 0;
}

int ForkChildProc(struct AppSpawnContent_ *content, AppSpawnClient *client, pid_t pid)
{
    if (pid < 0) {
        return -errno;
    } else if (pid == 0) {
#ifdef OHOS_DEBUG
        struct timespec tmStart = {0};
        GetCurTime(&tmStart);
#endif // OHOS_DEBUG

        // close socket id and signal for child
        if (content->clearEnvironment != NULL) {
            content->clearEnvironment(content, client);
        }

        if (content->setAppAccessToken != NULL) {
            content->setAppAccessToken(content, client);
        }

        int ret = -1;
#ifdef ASAN_DETECTOR
        if ((content->getWrapBundleNameValue != NULL && content->getWrapBundleNameValue(content, client) == 0)
            || (client->flags & APP_COLD_START)) {
#else
        if (client->flags & APP_COLD_START) {
#endif
            if (content->coldStartApp != NULL && content->coldStartApp(content, client) == 0) {
#ifndef APPSPAWN_TEST
                _exit(0x7f); // 0x7f user exit
#endif
                return -1;
            } else {
                ret = DoStartApp(content, client, content->longProcName, content->longProcNameLen);
            }
        } else {
            ret = DoStartApp(content, client, content->longProcName, content->longProcNameLen);
        }

#ifdef OHOS_DEBUG
        struct timespec tmEnd = {0};
        GetCurTime(&tmEnd);
        // 1s = 1000000000ns
        long timeUsed = (tmEnd.tv_sec - tmStart.tv_sec) * 1000000000L + (tmEnd.tv_nsec - tmStart.tv_nsec);
        APPSPAWN_LOGI("App timeused %d %ld ns.", getpid(), timeUsed);
#endif  // OHOS_DEBUG

        if (ret == 0 && content->runChildProcessor != NULL) {
            content->runChildProcessor(content, client);
        }
        ProcessExit();
    }
    return 0;
}

int AppSpawnProcessMsg(struct AppSpawnContent_ *content, AppSpawnClient *client, pid_t *childPid)
{
    APPSPAWN_CHECK(content != NULL, return -1, "Invalid content for appspawn");
    APPSPAWN_CHECK(client != NULL && childPid != NULL, return -1, "Invalid client for appspawn");
    APPSPAWN_LOGI("AppSpawnProcessMsg id %d 0x%x", client->id, client->flags);
#ifndef APPSPAWN_TEST
    pid_t pid = fork();
#else
    pid_t pid = 0;
#endif
    int ret = ForkChildProc(content, client, pid);
    APPSPAWN_CHECK(ret == 0, return ret, "fork child process error: %d", ret);
    *childPid = pid;
    return 0;
}

#ifdef OHOS_DEBUG
void GetCurTime(struct timespec *tmCur)
{
    if (tmCur == NULL) {
        return;
    }

    if (clock_gettime(CLOCK_REALTIME, tmCur) != 0) {
        APPSPAWN_LOGE("[appspawn] invoke, get time failed! err %d", errno);
    }
}
#endif  // OHOS_DEBUG
