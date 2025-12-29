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

#include "appspawn_utils.h"
#include "securec.h"
#include "parameter.h"
#include "securec.h"
#include "dirent.h"

#include "hnp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ARGV_NUM 256
#define MAX_ENV_NUM (128 + 2)
#define IS_OPTION_SET(x, option) ((x) & (1 << (option)))
#define BUFFER_SIZE 1024
#define CMD_API_TEXT_LEN 50
#define PARAM_BUFFER_SIZE 10
#define HNP_BIN_PATH "/system/bin/hnp"

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

/*
* 通过pipe[0] 获取子进程执行结果
*/
APPSPAWN_STATIC int HnpCmdApiReturnGet(int readFd, int *ret)
{
    HnpResult result = {0};
    ssize_t bytesRead = read(readFd, &result, sizeof(HnpResult));
    HNPAPI_ERROR_CHECK(bytesRead == sizeof(HnpResult) && ret != NULL, return HNP_API_ERRNO_RETURN_VALUE_GET_FAILED,
        "read api result faild %{public}zd", bytesRead);
    *ret = result.result;
    HNPAPI_LOG("\r\n [HNP API] get return is:%{public}d\r\n", result.result);
    return 0;
}

/**
* 设置pipe[1]到子进程环境变量中 用于后续回写执行结果到父进程
*/
APPSPAWN_STATIC void ChildProcessHandler(int pipeFd[2], char *const argv[])
{
    close(pipeFd[0]);
    
    char fdEnvBuffer[FD_BUFFER_LEN] = {0};
    int bytes = snprintf_s(fdEnvBuffer, FD_BUFFER_LEN, FD_BUFFER_LEN - 1,
        "%s=%d", HNP_INFO_RET_FD_ENV, pipeFd[1]);
    HNPAPI_ERROR_CHECK(bytes > 0 && bytes < FD_BUFFER_LEN, close(pipeFd[1]); _exit(-1),
        "\r\n [HNP API] pipe env error\r\n");

    char *const apcEnv[] = {
        fdEnvBuffer, NULL
    };
    execve(HNP_BIN_PATH, argv, apcEnv);
    HNPAPI_LOG("\r\n [HNP API] execv failed %{public}d\r\n", errno);
    close(pipeFd[1]);
    _exit(-1);
}

/*
* 1.创建pipe
* 2.fork + execv 拉起hnp子进程
* 3.父进程监听pipe[0]获取子进程执行结果
*/
static int StartHnpProcess(char *const argv[])
{
    int fd[2];
    pid_t pid;
    int exitVal = -1;
    int ret;
    int status;

    if (pipe(fd) < 0) {
        HNPAPI_LOG("\r\n [HNP API] pipe fd unsuccess!\r\n");
        return HNP_API_ERRNO_PIPE_CREATED_FAILED;
    }

    pid = fork();
    if (pid < 0) {
        HNPAPI_LOG("\r\n [HNP API] fork unsuccess!\r\n");
        return HNP_API_ERRNO_FORK_FAILED;
    } else if (pid == 0) {
        ChildProcessHandler(fd, argv);
    }

    HNPAPI_LOG("\r\n [HNP API] this is fork father! chid=%{public}d\r\n", pid);
    close(fd[1]);
    if (waitpid(pid, &status, 0) == -1) {
        close(fd[0]);
        HNPAPI_LOG("\r\n [HNP API] wait pid unsuccess!\r\n");
        return HNP_API_WAIT_PID_FAILED;
    }
    ret = HnpCmdApiReturnGet(fd[0], &exitVal);
    close(fd[0]);
    if (ret != 0) {
        HNPAPI_LOG("\r\n [HNP API] get api return value unsuccess!\r\n");
        return ret;
    }
    HNPAPI_LOG("\r\n [HNP API] Child process exited with exitval=%{public}d\r\n", exitVal);

    return exitVal;
}

static bool IsHnpInstallEnable()
{
    char buffer[PARAM_BUFFER_SIZE] = {0};
    int ret = GetParameter("const.startup.hnp.install.enable", "false", buffer, PARAM_BUFFER_SIZE);
    if (ret <= 0) {
        HNPAPI_LOG("\r\n [HNP API] get hnp install enable param unsuccess! ret =%{public}d\r\n", ret);
        return false;
    }

    if (strcmp(buffer, "true") == 0) {
        return true;
    }

    return false;
}

int NativeInstallHnp(const char *userId, const char *hnpRootPath,  const HapInfo *hapInfo, int installOptions)
{
    char *argv[MAX_ARGV_NUM] = {0};
    int index = 0;

    if ((userId == NULL) || (hnpRootPath == NULL) || (hapInfo == NULL)) {
        return HNP_API_ERRNO_PARAM_INVALID;
    }

    if (!IsHnpInstallEnable()) {
        return HNP_API_ERRNO_HNP_INSTALL_DISABLED;
    }

    HNPAPI_LOG("\r\n [HNP API] native package install! userId=%{public}s, hap path=%{public}s, sys abi=%{public}s, "
        "hnp root path=%{public}s, package name=%{public}s install options=%{public}d\r\n", userId, hapInfo->hapPath,
        hapInfo->abi, hnpRootPath, hapInfo->packageName, installOptions);

    argv[index++] = "hnp";
    argv[index++] = "install";
    argv[index++] = "-u";
    argv[index++] = (char *)userId;
    argv[index++] = "-i";
    argv[index++] = (char *)hnpRootPath;
    argv[index++] = "-p";
    argv[index++] = (char *)hapInfo->packageName;
    argv[index++] = "-s";
    argv[index++] = (char *)hapInfo->hapPath;
    argv[index++] = "-a";
    argv[index++] = (char *)hapInfo->abi;

    argv[index++] = "-I";
    argv[index++] = (char *)hapInfo->appIdentifier;

    // 传递需要独立签名的hnp信息 相对路径 private/xx.hnp  public/xx.hnp
    if (hapInfo->independentSignHnpPaths != NULL && hapInfo->count > 0) {
        HNPAPI_ERROR_CHECK(index < MAX_ARGV_NUM && (hapInfo->count <= (MAX_ARGV_NUM - index) / 2),
            return HNP_API_ERRNO_TOO_MANY_PARAM, "\r\n [HNP API] param count outof bound!\r\n");
        for (int i = 0; i < hapInfo->count; i++) {
            HNPAPI_ERROR_CHECK(hapInfo->independentSignHnpPaths[i] != NULL, return HNP_API_ERRNO_PARAM_INVALID,
                "\r\n [HNP API] sign hnp param invalid %{public}d!\r\n", i);
            argv[index++] = "-S";
            argv[index++] = hapInfo->independentSignHnpPaths[i];
        }
    }
    if (installOptions >= 0 && IS_OPTION_SET((unsigned int)installOptions, OPTION_INDEX_FORCE)) {
        HNPAPI_ERROR_CHECK(index < MAX_ARGV_NUM, return HNP_API_ERRNO_TOO_MANY_PARAM,
            "\r\n [HNP API] param count outof bound!\r\n");
        argv[index++] = "-f";
    }

    return StartHnpProcess(argv);
}

int NativeUnInstallHnp(const char *userId, const char *packageName)
{
    char *argv[MAX_ARGV_NUM] = {0};
    int index = 0;

    if ((userId == NULL) || (packageName == NULL)) {
        return HNP_API_ERRNO_PARAM_INVALID;
    }

    HNPAPI_LOG("\r\n [HNP API] native package uninstall! userId=%{public}s, package name=%{public}s\r\n", userId,
        packageName);

    argv[index++] = "hnp";
    argv[index++] = "uninstall";
    argv[index++] = "-u";
    argv[index++] = (char *)userId;
    argv[index++] = "-p";
    argv[index++] = (char *)packageName;

    return StartHnpProcess(argv);
}

#ifdef __cplusplus
}
#endif