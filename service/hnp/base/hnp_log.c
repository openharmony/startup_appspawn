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
#include <stdarg.h>

#include "securec.h"

#include "hnp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LOG_BUFF_LEN 1024

const char *g_logLevelName[HNP_LOG_BUTT] = {"INFO", "WARN", "ERROR", "DEBUG"};

// 辅助函数，用于替换字符串中的指定子串
static char* ReplaceSubstring(const char* str, const char* from, const char* to)
{
    char* result;
    int lenStr = strlen(str);
    int lenFrom = strlen(from);
    int lenTo = strlen(to);
    int count = 0;
    int i;
    int j;

    // 计算替换后字符串的长度
    for (i = 0; str[i] != '\0'; i++) {
        if (strncmp(&str[i], from, lenFrom) == 0) {
            count++;
        }
    }

    // 分配新字符串的内存
    result = (char*)malloc(lenStr + (lenTo - lenFrom) * count + 1);
    if (result == NULL) {
        return NULL;
    }

    // 复制字符串，替换子串
    i = 0;
    j = 0;
    while (str[i] != '\0') {
        if (strncmp(&str[i], from, lenFrom) == 0) {
            if (strcpy_s(&result[j], lenStr + 1 - j, to) != EOK) {
                free(result);
                return NULL;
            }
            j += lenTo;
            i += lenFrom;
        } else {
            result[j++] = str[i++];
        }
    }
    result[j] = '\0';
    return result;
}

void HnpLogPrintf(int logLevel, char *module, const char *format, ...)
{
    int ret;

    char* newFormat = ReplaceSubstring(format, "%{public}", "%");
    if (newFormat == NULL) {
        return;
    }

    ret = fprintf(stdout, "\n[%s][%s]", g_logLevelName[logLevel], module);
    if (ret < 0) {
        free(newFormat);
        return;
    }

    va_list args;
    va_start(args, format);
    ret = vfprintf(stdout, newFormat, args);
    va_end(args);
    free(newFormat);
    if (ret < 0) {
        return;
    }

    return;
}

#ifdef __cplusplus
}
#endif