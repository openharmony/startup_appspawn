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
#include "parameter.h"
#include "securec.h"

#define APPSPAWN_PRELOAD "libappspawn_helper.z.so"

static void CheckPreload(char *const argv[])
{
    char *preload = getenv("LD_PRELOAD");
    if (preload && strstr(preload, APPSPAWN_PRELOAD)) {
        return;
    }
    char buf[128] = APPSPAWN_PRELOAD;  // 128 is enough in most cases
    if (preload && preload[0]) {
        int len = sprintf_s(buf, sizeof(buf), "%s:" APPSPAWN_PRELOAD, preload);
        APPSPAWN_CHECK(len > 0, return, "preload too long: %{public}s", preload);
    }
    int ret = setenv("LD_PRELOAD", buf, true);
    APPSPAWN_CHECK(ret == 0, return, "setenv fail: %{public}s", buf);
    ssize_t nread = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    APPSPAWN_CHECK(nread != -1, return, "readlink fail: /proc/self/exe: %{public}d", errno);
    buf[nread] = 0;
    ret = execv(buf, argv);
    APPSPAWN_LOGE("execv fail: %{public}s: %{public}d: %{public}d", buf, errno, ret);
}

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
    AppSpawnStartArg arg = {};
    arg.mode = MODE_FOR_APP_SPAWN;
    arg.socketName = APPSPAWN_SOCKET_NAME;
    arg.serviceName = APPSPAWN_SERVER_NAME;
    arg.moduleType = MODULE_APPSPAWN;
    arg.initArg = 1;
    if (argc <= MODE_VALUE_INDEX) {  // appspawn start
        arg.mode = MODE_FOR_APP_SPAWN;
    } else if (strcmp(argv[MODE_VALUE_INDEX], "app_cold") == 0) {  // cold start
        APPSPAWN_CHECK(argc >= ARG_NULL, return 0, "Invalid arg for cold start %{public}d", argc);
        arg.mode = MODE_FOR_APP_COLD_RUN;
        arg.initArg = 0;
    } else if (strcmp(argv[MODE_VALUE_INDEX], "nweb_cold") == 0) {  // cold start
        APPSPAWN_CHECK(argc >= ARG_NULL, return 0, "Invalid arg for cold start %{public}d", argc);
        arg.mode = MODE_FOR_NWEB_COLD_RUN;
        arg.moduleType = MODULE_NWEBSPAWN;
        arg.serviceName = NWEBSPAWN_SERVER_NAME;
        arg.initArg = 0;
    } else if (strcmp(argv[MODE_VALUE_INDEX], NWEBSPAWN_SERVER_NAME) == 0) {  // nweb spawn start
        APPSPAWN_CHECK(argvSize >= APP_LEN_PROC_NAME,
            return 0, "Invalid arg size for service %{public}s", arg.serviceName);
        arg.mode = MODE_FOR_NWEB_SPAWN;
        arg.moduleType = MODULE_NWEBSPAWN;
        arg.socketName = NWEBSPAWN_SOCKET_NAME;
        arg.serviceName = NWEBSPAWN_SERVER_NAME;
    } else {
        APPSPAWN_CHECK(argvSize >= APP_LEN_PROC_NAME,
            return 0, "Invalid arg size for service %{public}s", arg.serviceName);
    }
    AppSpawnContent *content = StartSpawnService(&arg, argvSize, argc, argv);
    if (content != NULL) {
        content->runAppSpawn(content, argc, argv);
    }
    return 0;
}
