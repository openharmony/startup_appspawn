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

#include "hnp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HNPAPI_LOG(fmt, ...) \
    HILOG_INFO(LOG_CORE, "[%{public}s:%{public}d]" fmt, (__FILE_NAME__), (__LINE__), ##__VA_ARGS__)
#define MAX_ARGV_NUM 256
#define MAX_ENV_NUM (128 + 2)

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
        ret = execve("./hnp", argv, apcEnv);
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

int NativeInstallHnp(const char *userId, const char *hnpPath, Bool isForce)
{
    char *argv[MAX_ARGV_NUM] = {0};
    char *apcEnv[MAX_ENV_NUM] = {0};

    if ((userId == NULL) || (hnpPath == NULL)) {
        return HNP_API_ERRNO_PARAM_INVALID;
    }

    HNPAPI_LOG("\r\n [HNP API] native package install! userId=%s, hnpPath=%s, IsForce=%d\r\n",
        userId, hnpPath, isForce);

    argv[HNP_INDEX_0] = "hnp";
    argv[HNP_INDEX_1] = "install";
    argv[HNP_INDEX_2] = (char*)userId;
    argv[HNP_INDEX_3] = (char*)hnpPath;

    return StartHnpProcess(argv, apcEnv);
}

int NativeUnInstallHnp(const char *userId, const char *hnpName, const char *hnpVersion)
{
    char *argv[MAX_ARGV_NUM] = {0};
    char *apcEnv[MAX_ENV_NUM] = {0};

    if ((userId == NULL) || (hnpName == NULL) || (hnpVersion == NULL)) {
        return HNP_API_ERRNO_PARAM_INVALID;
    }

    HNPAPI_LOG("\r\n [HNP API] native package uninstall! userId=%s, hnpName=%s, hnpVersion=%s\r\n",
        userId, hnpName, hnpVersion);

    argv[HNP_INDEX_0] = "hnp";
    argv[HNP_INDEX_1] = "uninstall";
    argv[HNP_INDEX_2] = (char*)userId;
    argv[HNP_INDEX_3] = (char*)hnpName;
    argv[HNP_INDEX_4] = (char*)hnpVersion;
    
    return StartHnpProcess(argv, apcEnv);
}

#ifdef __cplusplus
}
#endif