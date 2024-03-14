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

void HnpLogPrintf(int logLevel, char *module, const char *format, ...)
{
    int ret;

    ret = fprintf(stdout, "\n[%s][%s]", g_logLevelName[logLevel], module);
    if (ret < 0) {
        return;
    }

    va_list args;
    va_start(args, format);
    ret = vfprintf(stdout, format, args);
    va_end(args);
    if (ret < 0) {
        return;
    }
    
    return;
}

#ifdef __cplusplus
}
#endif