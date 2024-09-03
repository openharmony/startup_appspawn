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

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>

#include "appspawn_hook.h"
#include "appspawn_modulemgr.h"
#include "appspawn_manager.h"
#include "appspawn_service.h"
#include "appspawn_kickdog.h"
#include "parameter.h"
#include "securec.h"

#define APPSPAWN_PRELOAD "libappspawn_helper.z.so"

#ifndef CJAPP_SPAWN
static AppSpawnStartArgTemplate g_appSpawnStartArgTemplate[PROCESS_INVALID] = {
    {APPSPAWN_SERVER_NAME, {MODE_FOR_APP_SPAWN, MODULE_APPSPAWN, APPSPAWN_SOCKET_NAME, APPSPAWN_SERVER_NAME, 1}},
    {NWEBSPAWN_SERVER_NAME, {MODE_FOR_NWEB_SPAWN, MODULE_NWEBSPAWN, NWEBSPAWN_SOCKET_NAME, NWEBSPAWN_SERVER_NAME, 1}},
    {"app_cold", {MODE_FOR_APP_COLD_RUN, MODULE_APPSPAWN, APPSPAWN_SOCKET_NAME, APPSPAWN_SERVER_NAME, 0}},
    {"nweb_cold", {MODE_FOR_NWEB_COLD_RUN, MODULE_NWEBSPAWN, APPSPAWN_SOCKET_NAME, NWEBSPAWN_SERVER_NAME, 0}},
    {NATIVESPAWN_SERVER_NAME, {MODE_FOR_NATIVE_SPAWN, MODULE_NATIVESPAWN, NATIVESPAWN_SOCKET_NAME,
        NATIVESPAWN_SERVER_NAME, 1}},
    {NWEBSPAWN_RESTART, {MODE_FOR_NWEB_SPAWN, MODULE_NWEBSPAWN, NWEBSPAWN_SOCKET_NAME, NWEBSPAWN_SERVER_NAME, 1}},
};
#else
static AppSpawnStartArgTemplate g_appCJSpawnStartArgTemplate[CJPROCESS_INVALID] = {
    {CJAPPSPAWN_SERVER_NAME, {MODE_FOR_CJAPP_SPAWN, MODULE_APPSPAWN, CJAPPSPAWN_SOCKET_NAME, CJAPPSPAWN_SERVER_NAME,
        1}},
    {"app_cold", {MODE_FOR_APP_COLD_RUN, MODULE_APPSPAWN, CJAPPSPAWN_SOCKET_NAME, CJAPPSPAWN_SERVER_NAME, 0}},
};
#endif

static void CheckPreload(char *const argv[])
{
    char buf[256] = APPSPAWN_PRELOAD;  // 256 is enough in most cases
    char *preload = getenv("LD_PRELOAD");
    char *pos = preload ? strstr(preload, APPSPAWN_PRELOAD) : NULL;
    if (pos) {
        int len = pos - preload;
        len = sprintf_s(buf, sizeof(buf), "%.*s%s", len, preload, pos + strlen(APPSPAWN_PRELOAD));
        APPSPAWN_CHECK(len >= 0, return, "preload too long?: %{public}s", preload);
        if (len == 0) {
            int ret = unsetenv("LD_PRELOAD");
            APPSPAWN_CHECK(ret == 0, return, "unsetenv fail(%{public}d)", errno);
        } else {
            int ret = setenv("LD_PRELOAD", buf, true);
            APPSPAWN_CHECK(ret == 0, return, "setenv fail(%{public}d): %{public}s", errno, buf);
        }
        return;
    }
    if (preload && preload[0]) {
        int len = sprintf_s(buf, sizeof(buf), "%s:" APPSPAWN_PRELOAD, preload);
        APPSPAWN_CHECK(len > 0, return, "preload too long: %{public}s", preload);
    }
    int ret = setenv("LD_PRELOAD", buf, true);
    APPSPAWN_CHECK(ret == 0, return, "setenv fail(%{public}d): %{public}s", errno, buf);
    ssize_t nread = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    APPSPAWN_CHECK(nread != -1, return, "readlink fail: /proc/self/exe: %{public}d", errno);
    buf[nread] = 0;
    ret = execv(buf, argv);
    APPSPAWN_LOGE("execv fail: %{public}s: %{public}d: %{public}d", buf, errno, ret);
}

#ifndef NATIVE_SPAWN
static AppSpawnStartArgTemplate *GetAppSpawnStartArg(const char *serverName, AppSpawnStartArgTemplate *argTemplate,
    int count)
{
    for (int i = 0; i < count; i++) {
        if (strcmp(serverName, argTemplate[i].serverName) == 0) {
            return &argTemplate[i];
        }
    }

    return argTemplate;
}
#endif

// appspawn -mode appspawn | cold | nwebspawn -param app_property -fd clientFd
int main(int argc, char *const argv[])
{
    APPSPAWN_LOGE("main argc: %{public}d", argc);
    if (argc <= 0) {
        return 0;
    }
    uintptr_t start = (uintptr_t)argv[0];
    uintptr_t end = (uintptr_t)strchr(argv[argc - 1], 0);
    if (end == 0) {
        return 0;
    }
    InitCommonEnv();
    CheckPreload(argv);
    (void)signal(SIGPIPE, SIG_IGN);
    uint32_t argvSize = end - start;
    AppSpawnStartArg *arg;
    AppSpawnStartArgTemplate *argTemp = NULL;

#ifdef CJAPP_SPAWN
    argTemp = &g_appCJSpawnStartArgTemplate[CJPROCESS_FOR_APP_SPAWN];
    if (argc > MODE_VALUE_INDEX) {
        argTemp = GetAppSpawnStartArg(argv[MODE_VALUE_INDEX], g_appCJSpawnStartArgTemplate,
            ARRAY_LENGTH(g_appCJSpawnStartArgTemplate));
    }
#elif NATIVE_SPAWN
    argTemp = &g_appSpawnStartArgTemplate[PROCESS_FOR_NATIVE_SPAWN];
#else
    argTemp = &g_appSpawnStartArgTemplate[PROCESS_FOR_APP_SPAWN];
    if (argc > MODE_VALUE_INDEX) {
        argTemp = GetAppSpawnStartArg(argv[MODE_VALUE_INDEX], g_appSpawnStartArgTemplate,
            ARRAY_LENGTH(g_appSpawnStartArgTemplate));
    }
#endif
    arg = &argTemp->arg;
    if (arg->initArg == 0) {
        APPSPAWN_CHECK(argc >= ARG_NULL, return 0, "Invalid arg for cold start %{public}d", argc);
    } else {
        if (strcmp(argTemp->serverName, NWEBSPAWN_RESTART) == 0) {  // nweb spawn restart
            APPSPAWN_CHECK_ONLY_EXPER(argvSize >= APP_LEN_PROC_NAME, argvSize = APP_LEN_PROC_NAME);
        } else {
            APPSPAWN_CHECK(argvSize >= APP_LEN_PROC_NAME, return 0, "Invalid arg size for service %{public}s",
                arg->serviceName);
        }
    }
    AppSpawnContent *content = StartSpawnService(arg, argvSize, argc, argv);
    if (content != NULL) {
        if (arg->moduleType == MODULE_APPSPAWN) {
            AppSpawnKickDogStart(content);
        }
        content->runAppSpawn(content, argc, argv);
    }
    return 0;
}
