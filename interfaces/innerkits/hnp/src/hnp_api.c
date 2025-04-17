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
#include "securec.h"
#include "parameter.h"

#include "hnp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HNPAPI_LOG(fmt, ...) \
    HILOG_INFO(LOG_CORE, "[%{public}s:%{public}d]" fmt, (__FILE_NAME__), (__LINE__), ##__VA_ARGS__)
#define MAX_ARGV_NUM 256
#define MAX_ENV_NUM (128 + 2)
#define IS_OPTION_SET(x, option) ((x) & (1 << (option)))
#define BUFFER_SIZE 1024
#define CMD_API_TEXT_LEN 50
#define PARAM_BUFFER_SIZE 10

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

static int HnpCmdApiReturnGet(int readFd, int *ret)
{
    char buffer[BUFFER_SIZE] = {0};
    ssize_t bytesRead;
    int bufferEnd = 0; // 跟踪缓冲区中有效数据的末尾
    const char *prefix = "native manager process exit. ret=";

    // 循环读取子进程的输出，直到结束
    while ((bytesRead = read(readFd, buffer + bufferEnd, BUFFER_SIZE - bufferEnd - 1)) > 0) {
        // 更新有效数据的末尾
        bufferEnd += bytesRead;

        // 如果缓冲区中的数据超过或等于50个字节，移动数据以保留最后50个字节
        if (bufferEnd >= CMD_API_TEXT_LEN) {
            if (memmove_s(buffer, BUFFER_SIZE, buffer + bufferEnd - CMD_API_TEXT_LEN, CMD_API_TEXT_LEN) != EOK) {
                HNPAPI_LOG("\r\n [HNP API] mem move unsuccess!\r\n");
                return HNP_API_ERRNO_MEMMOVE_FAILED;
            }
            bufferEnd = CMD_API_TEXT_LEN;
        }

        buffer[bufferEnd] = '\0';
    }

    if (bytesRead == -1) {
        HNPAPI_LOG("\r\n [HNP API] read stream unsuccess!\r\n");
        return HNP_API_ERRNO_PIPE_READ_FAILED;
    }

    char *retStr = strstr(buffer, prefix);

    if (retStr != NULL) {
        // 获取后续的数字字符串
        retStr += strlen(prefix);
        *ret = atoi(retStr);

        return 0;
    }

    HNPAPI_LOG("\r\n [HNP API] get return unsuccess!, buffer is:%{public}s\r\n", buffer);
    return HNP_API_ERRNO_RETURN_VALUE_GET_FAILED;
}

static int StartHnpProcess(char *const argv[], char *const apcEnv[])
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

    pid = vfork();
    if (pid < 0) {
        HNPAPI_LOG("\r\n [HNP API] fork unsuccess!\r\n");
        return HNP_API_ERRNO_FORK_FAILED;
    } else if (pid == 0) {
        close(fd[0]);
        // 将子进程的stdout重定向到管道的写端
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        ret = execve("/system/bin/hnp", argv, apcEnv);
        if (ret < 0) {
            HNPAPI_LOG("\r\n [HNP API] execve unsuccess!\r\n");
            _exit(-1);
        }
        _exit(0);
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
    char *apcEnv[MAX_ENV_NUM] = {0};
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

    if (installOptions >= 0 && IS_OPTION_SET((unsigned int)installOptions, OPTION_INDEX_FORCE)) {
        argv[index++] = "-f";
    }

    return StartHnpProcess(argv, apcEnv);
}

int NativeUnInstallHnp(const char *userId, const char *packageName)
{
    char *argv[MAX_ARGV_NUM] = {0};
    char *apcEnv[MAX_ENV_NUM] = {0};
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

    return StartHnpProcess(argv, apcEnv);
}

#ifdef __cplusplus
}
#endif