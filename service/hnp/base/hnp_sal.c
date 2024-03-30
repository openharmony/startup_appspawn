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
#include <unistd.h>

#include "hnp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

static int HnpPidGetByBinName(const char *programName, int *pids, int *count)
{
    FILE *cmdOutput;
    char cmdBuffer[BUFFER_SIZE];
    char command[HNP_COMMAND_LEN];
    int pidNum = 0;

    /* programName为卸载命令输入的进程名拼接命令command */
    if (sprintf_s(command, HNP_COMMAND_LEN, "pgrep -x %s", programName) < 0) {
        HNP_LOGE("hnp uninstall program[%s] run command sprintf unsuccess", programName);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    cmdOutput = popen(command, "rb");
    if (cmdOutput == NULL) {
        HNP_LOGE("hnp uninstall program[%s] not found", programName);
        return HNP_ERRNO_BASE_PROGRAM_NOT_FOUND;
    }

    while (fgets(cmdBuffer, sizeof(cmdBuffer), cmdOutput) != NULL) {
        pids[pidNum++] = atoi(cmdBuffer);
        if (pidNum >= MAX_PROCESSES) {
            HNP_LOGI("hnp uninstall program[%s] num over size", programName);
            break;
        }
    }
    pclose(cmdOutput);
    *count = pidNum;

    return 0;
}

int HnpProgramRunCheck(const char *binName, const char *runPath)
{
    int ret;
    int pids[MAX_PROCESSES];
    int count = 0;
    char command[HNP_COMMAND_LEN];
    char cmdBuffer[BUFFER_SIZE];

    HNP_LOGI("process[%s] running check", binName);

    /* 对programName进行空格过滤，防止外部命令注入 */
    if (strchr(binName, ' ') != NULL) {
        HNP_LOGE("hnp uninstall program name[%s] inval", binName);
        return HNP_ERRNO_BASE_PARAMS_INVALID;
    }

    ret = HnpPidGetByBinName(binName, pids, &count);
    if ((ret != 0) || (count == 0)) { // 返回非0代表未找到进程对应的pid
        return 0;
    }

    /* 判断进程是否运行 */
    for (int index = 0; index < count; index++) {
        if (sprintf_s(command, HNP_COMMAND_LEN, "lsof -p %d", pids[index]) < 0) {
            HNP_LOGE("hnp uninstall pid[%d] run check command sprintf unsuccess", pids[index]);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        FILE *cmdOutput = popen(command, "rb");
        if (cmdOutput == NULL) {
            HNP_LOGE("hnp uninstall pid[%d] not found", pids[index]);
            continue;
        }
        while (fgets(cmdBuffer, sizeof(cmdBuffer), cmdOutput) != NULL) {
            if (strstr(cmdBuffer, runPath) != NULL) {
                pclose(cmdOutput);
                HNP_LOGE("hnp install process[%s] is running now", binName);
                return HNP_ERRNO_PROGRAM_RUNNING;
            }
        }
        pclose(cmdOutput);
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