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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

#include "hnp_base.h"

#define READ 0
#define WRITE 1

#define SHELL "/bin/bash"

#ifdef __cplusplus
extern "C" {
#endif

static FILE* HnpPopen(const char *command, const char *mode)
{
    int pfd[2];
    pid_t pid;
    int modeFd;
    int status;

    if (pipe(pfd) < 0) {
        return NULL;
    }

    if ((*mode != 'r') || (*mode != 'w')) {
        return NULL;
    }

    pid = fork();
    if (pid < 0) {
        close(pfd[READ]);
        close(pfd[WRITE]);
        return NULL;
    }

    if (pid == 0) {
        if (*mode == 'r') {
            close(pfd[READ]);
            dup2(pfd[WRITE], STDOUT_FILENO);
        } else {
            close(pfd[WRITE]);
            dup2(pfd[READ], STDIN_FILENO);
        }

        execl(SHELL, "sh", "-c", command, NULL);
        exit(0);
    }

    waitpid(pid, &status, 0);

    if (*mode == 'r') {
        modeFd = READ;
        close(pfd[WRITE]);
    } else {
        modeFd = WRITE;
        close(pfd[READ]);
    }

    return fdopen(pfd[modeFd], mode);
}

static void HnpPclose(FILE *stream)
{
    (void)fclose(stream);

    return;
}

static int HnpPidGetByBinName(const char *binName, int *pids, int *count)
{
    FILE *cmdOutput;
    char cmdBuffer[BUFFER_SIZE];
    char command[HNP_COMMAND_LEN];
    int pidNum = 0;

    /* binName为卸载命令输入的进程名拼接命令command */
    if (sprintf_s(command, HNP_COMMAND_LEN, "pgrep -x %s", binName) < 0) {
        HNP_LOGE("hnp uninstall process[%s] run command sprintf unsuccess", binName);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    cmdOutput = HnpPopen(command, "rb");
    if (cmdOutput == NULL) {
        HNP_LOGI("hnp uninstall process[%s] not found", binName);
        return HNP_ERRNO_BASE_PROCESS_NOT_FOUND;
    }

    while (fgets(cmdBuffer, sizeof(cmdBuffer), cmdOutput) != NULL) {
        pids[pidNum++] = atoi(cmdBuffer);
        if (pidNum >= MAX_PROCESSES) {
            HNP_LOGI("hnp uninstall process[%s] num over size", binName);
            break;
        }
    }
    HnpPclose(cmdOutput);
    *count = pidNum;

    return 0;
}

int HnpProcessRunCheck(const char *binName, const char *runPath)
{
    int ret;
    int pids[MAX_PROCESSES];
    int count = 0;
    char command[HNP_COMMAND_LEN];
    char cmdBuffer[BUFFER_SIZE];

    HNP_LOGI("process[%s] running check, runPath[%s]", binName, runPath);

    ret = HnpPidGetByBinName(binName, pids, &count);
    if ((ret != 0) || (count == 0)) { // 返回非0代表未找到进程对应的pid
        return 0;
    }

    /* 判断进程是否运行 */
    for (int index = 0; index < count; index++) {
        if (sprintf_s(command, HNP_COMMAND_LEN, "lsof -p %d | grep txt", pids[index]) < 0) {
            HNP_LOGE("hnp uninstall pid[%d] run check command sprintf unsuccess", pids[index]);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        FILE *cmdOutput = HnpPopen(command, "rb");
        if (cmdOutput == NULL) {
            HNP_LOGE("hnp uninstall pid[%d] not found", pids[index]);
            continue;
        }
        while (fgets(cmdBuffer, sizeof(cmdBuffer), cmdOutput) != NULL) {
            if (strstr(cmdBuffer, runPath) != NULL) {
                HnpPclose(cmdOutput);
                HNP_LOGE("hnp install process[%s] is running now", binName);
                return HNP_ERRNO_PROCESS_RUNNING;
            }
        }
        HnpPclose(cmdOutput);
    }

    return 0;
}

int HnpSymlink(const char *srcFile, const char *dstFile)
{
    int ret;

    /* 将源二进制文件权限设置为750 */
    ret = chmod(srcFile, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
    if (ret < 0) {
        HNP_LOGE("hnp install generate soft link chmod unsuccess, src:%s, errno:%d", srcFile, errno);
        return HNP_ERRNO_BASE_CHMOD_FAILED;
    }

    if (access(dstFile, F_OK) == 0) {
        unlink(dstFile);
    }

    ret = symlink(srcFile, dstFile);
    if (ret < 0) {
        HNP_LOGE("hnp install generate soft link unsuccess, src:%s, dst:%s, errno:%d", srcFile, dstFile, errno);
        return HNP_ERRNO_GENERATE_SOFT_LINK_FAILED;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif