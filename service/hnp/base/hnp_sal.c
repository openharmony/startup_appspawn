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

pid_t *gChildPid = NULL;
int gMax = 0;

static void HnpPopenForChild(int pipefd[], const char *command, const char *mode)
{
    close(pipefd[READ]);
    if (mode[0] == 'r') {
        if (pipefd[WRITE] != STDOUT_FILENO) {
            dup2(pipefd[WRITE], STDOUT_FILENO);
            close(pipefd[WRITE]);
        }
    } else {
        if (pipefd[WRITE] != STDIN_FILENO) {
            dup2(pipefd[WRITE], STDIN_FILENO);
            close(pipefd[WRITE]);
        }
    }

    for (int i = 0; i < gMax; i++) {
        if (gChildPid[i] > 0) {
            close(i);
        }
    }

    execl(SHELL, "sh", "-c", command, NULL);
    _exit(EISCONN);
}

static FILE* HnpPopen(const char *command, const char *mode)
{
    int pipefd[2];
    pid_t pid;
    FILE *stream;

    if (mode[0] != 'r' && mode[0] != 'w') {
        return NULL;
    }

    if (gChildPid == NULL) {
        gMax = sysconf(_SC_OPEN_MAX);
        gChildPid = (pid_t *)calloc(gMax, sizeof(pid_t));
        if (gChildPid == NULL) {
            return NULL;
        }
    }

    if (pipe(pipefd) < 0) {
        return NULL;
    }

    if ((pipefd[READ] >= gMax) || (pipefd[WRITE] >= gMax)) {
        close(pipefd[READ]);
        close(pipefd[WRITE]);
        return NULL;
    }

    pid = fork();
    if (pid < 0) {
        return NULL;
    }
    if (pid == 0) {
        HnpPopenForChild(pipefd, command, mode);
    }

    if (mode[0] == 'r') {
        close(pipefd[WRITE]);
        stream = fdopen(pipefd[READ], mode);
    } else {
        close(pipefd[READ]);
        stream = fdopen(pipefd[WRITE], mode);
    }
    if (stream != NULL) {
        gChildPid[fileno(stream)] = pid;
    }
    return stream;
}

static void HnpPclose(FILE *stream)
{
    int status;
    pid_t pid;

    if (stream == NULL) {
        return;
    }

    if (gChildPid == NULL) {
        return;
    }

    pid = gChildPid[fileno(stream)];
    if (pid <= 0) {
        return;
    }

    gChildPid[fileno(stream)] = 0;
    waitpid(pid, &status, 0);
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

    HNP_LOGI("process[%s] running check", binName);

    /* 对programName进行空格过滤，防止外部命令注入 */
    if (strchr(binName, ' ') != NULL) {
        HNP_LOGE("hnp uninstall process name[%s] invalid", binName);
        return HNP_ERRNO_BASE_PARAMS_INVALID;
    }

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