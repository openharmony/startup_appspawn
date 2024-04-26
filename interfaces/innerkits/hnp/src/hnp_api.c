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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include "hilog/log.h"
#include "appspawn_utils.h"

#include "hnp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HNPAPI_LOG(fmt, ...) \
    HILOG_INFO(LOG_CORE, "[%{public}s:%{public}d]" fmt, (__FILE_NAME__), (__LINE__), ##__VA_ARGS__)
#define MAX_ARGV_NUM 256
#define MAX_ENV_NUM (128 + 2)
#define IS_OPTION_SET(x, option) ((x) & (1 << (option)))

/* 数字索引 */
enum {
    HNP_INDEX_0 = 0,
    HNP_INDEX_1,
    HNP_INDEX_2,
    HNP_INDEX_3,
    HNP_INDEX_4,
    HNP_INDEX_5,
    HNP_INDEX_6,
    HNP_INDEX_7
};

static int StartHnpProcess(char *const argv[], char *const apcEnv[])
{
    pid_t pid;
    int ret;
    int status;
    int exitVal = -1;

    pid = vfork();
    if (pid < 0) {
        HNPAPI_LOG("\r\n [HNP API] fork unsuccess!\r\n");
        return HNP_API_ERRNO_FORK_FAILED;
    } else if (pid == 0) {
        HNPAPI_LOG("\r\n [HNP API] this is fork children!\r\n");
        ret = execve("/system/bin/hnp", argv, apcEnv);
        if (ret < 0) {
            HNPAPI_LOG("\r\n [HNP API] execve unsuccess!\r\n");
            _exit(-1);
        }
        _exit(0);
    }

    HNPAPI_LOG("\r\n [HNP API] this is fork father! chid=%d\r\n", pid);
    if (waitpid(pid, &status, 0) == -1) {
        HNPAPI_LOG("\r\n [HNP API] wait pid unsuccess!\r\n");
        return HNP_API_WAIT_PID_FAILED;
    }
    if (WIFEXITED(status)) {
        exitVal = WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        exitVal = WTERMSIG(status);
    }
    HNPAPI_LOG("\r\n [HNP API] Child process exited with exitval=%d\r\n", exitVal);

    return exitVal;
}

int NativeInstallHnpEx(const char *userId, const char *packages[], int count, const char *installPath, int installOptions)
{
    char *argv[MAX_ARGV_NUM] = {0};
    char *apcEnv[MAX_ENV_NUM] = {0};
    int index = 0;

    if (!IsDeveloperModeOpen()) {
        HNPAPI_LOG("\r\n [HNP API] native package install not in developer mode");
        return HNP_API_NOT_IN_DEVELOPER_MODE;
    }

    if ((userId == NULL) || (packages == NULL) || (count == 0)) {
        return HNP_API_ERRNO_PARAM_INVALID;
    }

    HNPAPI_LOG("\r\n [HNP API] native package install! userId=%s, hnpPath count=%d, install path=%s "
        "install options=%d\r\n", userId, count, installPath, installOptions);

    argv[index++] = "hnp";
    argv[index++] = "install";
    argv[index++] = "-u";
    argv[index++] = (char*)userId;

    for (int i = 0; i < count; i++) {
        argv[index++] = "-p";
        argv[index++] = (char*)packages[i];
    }

    if (installPath != NULL) {
        argv[index++] = "-i";
        argv[index++] = (char*)installPath;
    }

    if (IS_OPTION_SET(installOptions, OPTION_INDEX_FORCE)) {
        argv[index++] = "-f";
    }

    return StartHnpProcess(argv, apcEnv);
}

int NativeUnInstallHnpEx(const char *userId, const char *hnpName, const char *hnpVersion, const char *installPath)
{
    char *argv[MAX_ARGV_NUM] = {0};
    char *apcEnv[MAX_ENV_NUM] = {0};
    int index = 0;

    if (!IsDeveloperModeOpen()) {
        HNPAPI_LOG("\r\n [HNP API] native package uninstall not in developer mode");
        return HNP_API_NOT_IN_DEVELOPER_MODE;
    }

    if ((userId == NULL) || (hnpName == NULL) || (hnpVersion == NULL)) {
        return HNP_API_ERRNO_PARAM_INVALID;
    }

    HNPAPI_LOG("\r\n [HNP API] native package uninstall! userId=%s, hnpName=%s, hnpVersion=%s\r\n",
        userId, hnpName, hnpVersion);

    argv[index++] = "hnp";
    argv[index++] = "uninstall";
    argv[index++] = "-u";
    argv[index++] = (char*)userId;
    argv[index++] = "-n";
    argv[index++] = (char*)hnpName;
    argv[index++] = "-v";
    argv[index++] = (char*)hnpVersion;

    if (installPath != NULL) {
        argv[index++] = "-i";
        argv[index++] = (char*)installPath;
    }

    return StartHnpProcess(argv, apcEnv);
}

int NativeInstallHnp(const char *userId, const char *hnpRootPath, const char *packageName, int installOptions)
{
    (void)userId;
    (void)hnpRootPath;
    (void)packageName;
    (void)installOptions;

    return 0;
}

int NativeUnInstallHnp(const char *userId, const char *packageName)
{
    (void)userId;
    (void)packageName;

    return 0;
}

#ifdef __cplusplus
}
#endif