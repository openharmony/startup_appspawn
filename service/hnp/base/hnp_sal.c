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
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>

#include "hnp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

int HnpProcessRunCheck(const char *runPath)
{
    char cmdBuffer[BUFFER_SIZE];
    FILE *cmdOutput;

    HNP_LOGI("runPath[%{public}s] running check", runPath);

    /* 判断进程是否运行 */
    cmdOutput = popen("lsof", "rb");
    if (cmdOutput == NULL) {
        HNP_LOGE("hnp uninstall lsof command unsuccess");
        return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
    }

    while (fgets(cmdBuffer, sizeof(cmdBuffer), cmdOutput) != NULL) {
        if (strstr(cmdBuffer, runPath) != NULL) {
            HNP_LOGE("hnp install path is running now, path[%{public}s]", cmdBuffer);
            pclose(cmdOutput);
            return HNP_ERRNO_PROCESS_RUNNING;
        }
    }

    pclose(cmdOutput);
    return 0;
}

APPSPAWN_STATIC void HnpRelPath(const char *fromPath, const char *toPath, char *relPath)
{
    char *from = strdup(fromPath);
    if (from == NULL) {
        return;
    }
    char *to = strdup(toPath);
    if (to == NULL) {
        return;
    }
    int numDirs = 0;
    int ret;

    char *fromHead = from;
    char *toHead = to;

    while ((*from != '\0') && (*to != '\0') && (*from == *to)) {
        from++;
        to++;
    }

    while (from > fromHead && *(from - 1) != DIR_SPLIT_SYMBOL) {
        from--;
        to--;
    }

    char *p = from;
    while (*p) {
        if (*p == DIR_SPLIT_SYMBOL) {
            numDirs++;
        }
        p++;
    }

    for (int i = 0; i < numDirs; i++) {
        ret = strcat_s(relPath, MAX_FILE_PATH_LEN, "../");
        if (ret != 0) {
            HNP_LOGE("Failed to strcat rel path");
            free(fromHead);
            free(toHead);
            return;
        }
    }
    ret = strcat_s(relPath, MAX_FILE_PATH_LEN, to);
    if (ret != 0) {
        HNP_LOGE("Failed to strcat rel path");
        free(fromHead);
        free(toHead);
        return;
    }

    free(fromHead);
    free(toHead);

    return;
}

int HnpSymlink(const char *srcFile, const char *dstFile)
{
    int ret;
    char relpath[MAX_FILE_PATH_LEN];

    (void)unlink(dstFile);

    HnpRelPath(dstFile, srcFile, relpath);
    ret = symlink(relpath, dstFile);
    if (ret < 0) {
        HNP_LOGE("hnp install generate soft link unsuccess, src:%{public}s, dst:%{public}s, errno:%{public}d", srcFile,
            dstFile, errno);
        return HNP_ERRNO_GENERATE_SOFT_LINK_FAILED;
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
