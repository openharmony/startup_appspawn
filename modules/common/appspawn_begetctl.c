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

#include "appspawn_adapter.h"

#include <sys/signalfd.h>
#include <sys/wait.h>
#include "securec.h"

#include "appspawn_hook.h"
#include "appspawn_utils.h"
#include "init_utils.h"

APPSPAWN_STATIC void SetSystemEnv(void)
{
    int32_t ret;
    char envValue[MAX_ENV_VALUE_LEN];
    ret = ConvertEnvValue("/bin:/system/bin:${PATH}", envValue, MAX_ENV_VALUE_LEN);
    APPSPAWN_CHECK(ret == 0, return, "Convert env value failed");
    ret = setenv("PATH", envValue, true);
    APPSPAWN_CHECK(ret == 0, return, "Set env fail value=%{public}s", envValue);
}

APPSPAWN_STATIC void RunAppSandbox(const char *ptyName)
{
    if (ptyName == NULL) {
        return;
    }
#ifndef APPSPAWN_TEST
    SetSystemEnv();
    OpenConsole();
    char *realPath = realpath(ptyName, NULL);
    if (realPath == NULL) {
        APPSPAWN_CHECK(errno == ENOENT,return,
            "Failed to resolve %{public}s real path err=%{public}d", ptyName, errno);
    }
    APPSPAWN_CHECK(realPath != NULL, _exit(1), "Failed get realpath, err=%{public}d", errno);
    int n = strncmp(realPath, "/dev/pts/", strlen("/dev/pts/"));
    APPSPAWN_CHECK(n == 0, free(realPath); _exit(1), "pts path %{public}s is invaild", realPath);
    int fd = open(realPath, O_RDWR);
    free(realPath);
    APPSPAWN_CHECK(fd >= 0, _exit(1), "Failed open %s, err=%{public}d", ptyName, errno);
    (void)dup2(fd, STDIN_FILENO);
    (void)dup2(fd, STDOUT_FILENO);
    (void)dup2(fd, STDERR_FILENO); // Redirect fd to 0, 1, 2
    (void)close(fd);

    char *argv[] = { (char *)"/bin/sh", NULL };
    APPSPAWN_CHECK_ONLY_LOG(execv(argv[0], argv) == 0,
        "app %{public}s execv sh failed! err %{public}d.", ptyName, errno);
    _exit(0x7f); // 0x7f: user specified
#endif
    APPSPAWN_LOGE("Exit RunAppSandbox %{public}s exit", ptyName);
}

APPSPAWN_STATIC int RunBegetctlBootApp(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_CHECK_ONLY_EXPER(property != NULL, return -1);
    UNUSED(content);
    if ((property->client.flags & APP_BEGETCTL_BOOT) != APP_BEGETCTL_BOOT) {
        APPSPAWN_LOGW("Enter begetctl boot without BEGETCTL_BOOT flag set");
        return -1;
    }
    uint32_t len = 0;
    const char *cmdMsg = (const char *)GetAppSpawnMsgExtInfo(property->message, MSG_EXT_NAME_BEGET_PTY_NAME, &len);
    APPSPAWN_CHECK(cmdMsg != NULL, return -1, "Failed to get extInfo");
    RunAppSandbox(cmdMsg);
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    AddAppSpawnHook(STAGE_CHILD_PRE_RUN, HOOK_PRIO_LOWEST, RunBegetctlBootApp);
}

