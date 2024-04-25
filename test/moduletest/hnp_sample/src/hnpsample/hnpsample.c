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

#include "hnpsamplelib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HNPSAMPLE_INDEX_2 2
#define MAX_DELAY_TIME 60

// hnpsample软件功能包括：读取cfg文件并打印内容，执行依赖的so函数。通过argv[1]参数控制运行的时间，单位s
int main(int argc, char *argv[])
{
    int sectime = 0;

    printf("\nstart hnp sample");

    if (argc == HNPSAMPLE_INDEX_2) {
        sectime = atoi(argv[1]);
        if (sectime > MAX_DELAY_TIME) {
            sectime = 0;
        }
    }

    // 读取配置文件内容并打印
    FILE *file = fopen("../cfg/hnpsample.cfg", "r");
    if (file != NULL) {
        printf("\ncfg file content:\n");
        int ch;
        while ((ch = fgetc(file)) != EOF) {
            putchar(ch);
        }
        printf("\ncfg file end.");
        fclose(file);
    } else {
        printf("\n open cfg=../cfg/hnpsample.cfg failed.");
    }

    // 调用依赖so的函数并进行相应延时处理
    HnpSampleLibDelay(sectime);

    printf("\nhnp sample end");
    return 0;
}


#ifdef __cplusplus
}
#endif