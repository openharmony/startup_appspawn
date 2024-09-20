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

#ifndef DEVICEDEBUG_BASE_H
#define DEVICEDEBUG_BASE_H

#include "hilog/log.h"
#undef LOG_TAG
#define LOG_TAG "APPSPAWN_DEVICEDEBUG"
#undef LOG_DOMAIN
#define LOG_DOMAIN (0xD002C00 + 0x11)

#ifdef __cplusplus
extern "C" {
#endif

/* 数字索引 */
enum {
    DEVICEDEBUG_NUM_0 = 0,
    DEVICEDEBUG_NUM_1,
    DEVICEDEBUG_NUM_2,
    DEVICEDEBUG_NUM_3,
    DEVICEDEBUG_NUM_4,
    DEVICEDEBUG_NUM_5,
    DEVICEDEBUG_NUM_6,
    DEVICEDEBUG_NUM_7
};

// 0x11 操作类型非法
#define DEVICEDEBUG_ERRNO_OPERATOR_TYPE_INVALID    0x11
// 0x12 参数缺失
#define DEVICEDEBUG_ERRNO_OPERATOR_ARGV_MISS       0x12
// 0x13 参数缺失
#define DEVICEDEBUG_ERRNO_OPERATOR_ARGV_MISS       0x12
// 0x16 参数错误
#define DEVICEDEBUG_ERRNO_PARAM_INVALID            0x16
// 0x17 内存不足
#define DEVICEDEBUG_ERRNO_NOMEM                    0x17


#define DEVICEDEBUG_LOGI(args, ...) \
    HILOG_INFO(LOG_CORE, "[%{public}s:%{public}d]" args, (__FILE_NAME__), (__LINE__), ##__VA_ARGS__); \

#define DEVICEDEBUG_LOGE(args, ...) \
    HILOG_ERROR(LOG_CORE, "[%{public}s:%{public}d]" args, (__FILE_NAME__), (__LINE__), ##__VA_ARGS__); \

#ifdef __cplusplus
}
#endif

#endif