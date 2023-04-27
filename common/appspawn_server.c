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
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <sched.h>
#ifdef OHOS_DEBUG
#include <time.h>
#endif // OHOS_DEBUG
#include <stdbool.h>

#include "syspara/parameter.h"
#include "securec.h"

#define DEFAULT_UMASK 0002

#ifndef APPSPAWN_TEST
#ifndef OHOS_LITE
void DisallowInternet(void);
#endif
#endif

#define SANDBOX_STACK_SIZE (1024 * 1024 * 8)
#define APPSPAWN_CHECK_EXIT "AppSpawnCheckUnexpectedExitCall"

static void SetInternetPermission(const AppSpawnClient *client)
{
#ifndef APPSPAWN_TEST
#ifndef OHOS_LITE
    if (client == NULL) {
        return;
    }

    APPSPAWN_LOGI("SetInternetPermission id %d setAllowInternet %hhu allowInternet %hhu", client->id,
                  client->setAllowInternet, client->allowInternet);
    if (client->setAllowInternet == 1 && client->allowInternet == 0) {
        DisallowInternet();
    }
#endif
#endif
}

static void NotifyResToParent(struct AppSpawnContent_ *content, AppSpawnClient *client, int result)
{
    if (content->notifyResToParent != NULL) {
        content->notifyResToParent(content, client, result);
    }
}

static void ProcessExit(int code)
{
    APPSPAWN_LOGI("App exit: %{public}d", code);
#ifndef APPSPAWN_TEST
#ifdef OHOS_LITE
    _exit(0x7f); // 0x7f user exit
#else
    quick_exit(0);
#endif
#endif
}

#ifdef APPSPAWN_HELPER
__attribute__((visibility("default")))
_Noreturn
void exit(int code)
{
    char *checkExit = getenv(APPSPAWN_CHECK_EXIT);
    if (checkExit && atoi(checkExit) == getpid()) {
        APPSPAWN_LOGF("Unexpected exit call: %{public}d", code);
        abort();
    }
    // hook `exit` to `ProcessExit` to ensure app exit in a clean way
    ProcessExit(code);
    // should not come here
    abort();
}
#endif

int DoStartApp(struct AppSpawnContent_ *content, AppSpawnClient *client, char *longProcName, uint32_t longProcNameLen)
{
    SetInternetPermission(client);

    APPSPAWN_LOGI("DoStartApp id %d longProcNameLen %u", client->id, longProcNameLen);
    int32_t ret = 0;

    if ((client->cloneFlags & CLONE_NEWNS) && (content->setAppSandbox)) {
        ret = content->setAppSandbox(content, client);
        APPSPAWN_CHECK(ret == 0, NotifyResToParent(content, client, ret);
            return ret, "Failed to set app sandbox");
    }

    (void)umask(DEFAULT_UMASK);
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

    if (content->setSeccompFilter) {
        content->setSeccompFilter(content, client);
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

static int AppSpawnChildRun(void *arg)
{
    APPSPAWN_CHECK(arg != NULL, return -1, "Invalid arg for appspawn child");
    AppSandboxArg *sandbox = (AppSandboxArg *)arg;
    struct AppSpawnContent_ *content = sandbox->content;
    AppSpawnClient *client = sandbox->client;

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
        || ((client->flags & APP_COLD_START) != 0)) {
#else
    if ((client->flags & APP_COLD_START) != 0) {
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
    if (content->initDebugParams != NULL) {
        content->initDebugParams(content, client);
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
    return 0;
}

int AppSpawnChild(void *arg)
{
    char checkExit[16] = ""; // 16 is enough to store an int
    if (GetIntParameter("persist.init.debug.checkexit", false)) {
        (void)sprintf_s(checkExit, sizeof(checkExit), "%d", getpid());
    }
    setenv(APPSPAWN_CHECK_EXIT, checkExit, true);
    int ret = AppSpawnChildRun(arg);
    unsetenv(APPSPAWN_CHECK_EXIT);
    return ret;
}

int AppSpawnProcessMsg(AppSandboxArg *sandbox, pid_t *childPid)
{
    pid_t pid;
    APPSPAWN_CHECK(sandbox != NULL && sandbox->content != NULL, return -1, "Invalid content for appspawn");
    APPSPAWN_CHECK(sandbox->client != NULL && childPid != NULL, return -1, "Invalid client for appspawn");
    APPSPAWN_LOGI("AppSpawnProcessMsg id %d 0x%x", sandbox->client->id, sandbox->client->flags);

#ifndef APPSPAWN_TEST
    AppSpawnClient *client = sandbox->client;
    if (client->cloneFlags & CLONE_NEWPID) {
        APPSPAWN_CHECK(client->cloneFlags & CLONE_NEWNS, return -1, "clone flags error");
        char *childStack = (char *)malloc(SANDBOX_STACK_SIZE);
        APPSPAWN_CHECK(childStack != NULL, return -1, "malloc failed");

        pid = clone(AppSpawnChild, childStack + SANDBOX_STACK_SIZE, client->cloneFlags | SIGCHLD, (void *)sandbox);
        if (pid > 0) {
            free(childStack);
            *childPid = pid;
            return 0;
        }

        client->cloneFlags &= ~CLONE_NEWPID;
        free(childStack);
    }

    pid = fork();
    APPSPAWN_CHECK(pid >= 0, return -errno, "fork child process error: %d", -errno);
    *childPid = pid;
#else
    pid = 0;
    *childPid = pid;
#endif
    if (pid == 0) {
        ProcessExit(AppSpawnChild((void *)sandbox));
    }
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
