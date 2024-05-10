/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef APPSPAWN_UTILS_H
#define APPSPAWN_UTILS_H

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "hilog/log.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#ifndef APPSPAWN_TEST
#define APPSPAWN_STATIC static
#else
#define APPSPAWN_STATIC
#endif

#ifndef APPSPAWN_BASE_DIR
#define APPSPAWN_BASE_DIR ""
#endif
#if defined(__MUSL__)
#define APPSPAWN_SOCKET_DIR APPSPAWN_BASE_DIR "/dev/unix/socket/"
#define APPSPAWN_MSG_DIR APPSPAWN_BASE_DIR "/mnt/startup/"
#else
#define APPSPAWN_SOCKET_DIR APPSPAWN_BASE_DIR "/dev/socket/"
#define APPSPAWN_MSG_DIR APPSPAWN_BASE_DIR "/mnt/startup/"
#endif

#define APPSPAWN_CHECK_EXIT "AppSpawnCheckUnexpectedExitCall"
#define UNUSED(x) (void)(x)

#define APP_COLD_START 0x01
#define APP_ASAN_DETECTOR 0x02
#define APP_DEVELOPER_MODE 0x04
#define APP_BEGETCTL_BOOT 0x400

#define MAX_LEN_SHORT_NAME 16
#define DEFAULT_UMASK 0002
#define UID_BASE 200000 // 20010029
#define DEFAULT_DIR_MODE 0711
#define USER_ID_BUFFER_SIZE 32

#define APPSPAWN_SEC_TO_NSEC 1000000000
#define APPSPAWN_MSEC_TO_NSEC 1000000
#define APPSPAWN_USEC_TO_NSEC 1000
#define APPSPAWN_SEC_TO_MSEC 1000

#define CHECK_FLAGS_BY_INDEX(flags, index) ((((flags) >> (index)) & 0x1) == 0x1)
#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))
#endif

#define INVALID_PERMISSION_INDEX (-1)

#define MAX_ENV_VALUE_LEN 1024

typedef struct TagAppSpawnCommonEnv {
    const char *envName;
    const char *envValue;
    int developerModeEnable;
} AppSpawnCommonEnv;

typedef enum {
    APPSPAWN_OK = 0,
    APPSPAWN_SYSTEM_ERROR = 0xD000000,
    APPSPAWN_ARG_INVALID,
    APPSPAWN_MSG_INVALID,
    APPSPAWN_MSG_TOO_LONG,
    APPSPAWN_TLV_NOT_SUPPORT,
    APPSPAWN_TLV_NONE,
    APPSPAWN_SANDBOX_NONE,
    APPSPAWN_SANDBOX_LOAD_FAIL,
    APPSPAWN_SANDBOX_INVALID,
    APPSPAWN_SANDBOX_MOUNT_FAIL,  // 0xD00000a
    APPSPAWN_SPAWN_TIMEOUT,       // 0xD00000a
    APPSPAWN_CHILD_CRASH,         // 0xD00000b
    APPSPAWN_NATIVE_NOT_SUPPORT,
    APPSPAWN_ACCESS_TOKEN_INVALID,
    APPSPAWN_PERMISSION_NOT_SUPPORT,
    APPSPAWN_BUFFER_NOT_ENOUGH,
    APPSPAWN_TIMEOUT,
    APPSPAWN_FORK_FAIL,
    APPSPAWN_DEBUG_MODE_NOT_SUPPORT,
    APPSPAWN_NODE_EXIST,
} AppSpawnErrorCode;

uint64_t DiffTime(const struct timespec *startTime, const struct timespec *endTime);
void AppSpawnDump(const char *fmt, ...);
void SetDumpToStream(FILE *stream);
typedef int (*SplitStringHandle)(const char *str, void *context);
int32_t StringSplit(const char *str, const char *separator, void *context, SplitStringHandle handle);
char *GetLastStr(const char *str, const char *dst);
uint32_t GetSpawnTimeout(uint32_t def);
void DumpCurrentDir(char *buffer, uint32_t bufferLen, const char *dirPath);
int IsDeveloperModeOpen();
void InitCommonEnv(void);
int ConvertEnvValue(const char *srcEnv, char *dstEnv, int len);

#ifndef APP_FILE_NAME
#define APP_FILE_NAME   (strrchr((__FILE__), '/') ? strrchr((__FILE__), '/') + 1 : (__FILE__))
#endif

#ifndef OHOS_LITE
#define APPSPAWN_DOMAIN (0xD002C00 + 0x11)
#ifndef APPSPAWN_LABEL
#define APPSPAWN_LABEL "APPSPAWN"
#endif

#undef LOG_TAG
#define LOG_TAG APPSPAWN_LABEL
#undef LOG_DOMAIN
#define LOG_DOMAIN APPSPAWN_DOMAIN

#define APPSPAWN_LOGI(fmt, ...) \
    HILOG_INFO(LOG_CORE, "[%{public}s:%{public}d]" fmt, (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGE(fmt, ...) \
    HILOG_ERROR(LOG_CORE, "[%{public}s:%{public}d]" fmt, (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGV(fmt, ...) \
    HILOG_DEBUG(LOG_CORE, "[%{public}s:%{public}d]" fmt, (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGW(fmt, ...) \
    HILOG_WARN(LOG_CORE, "[%{public}s:%{public}d]" fmt, (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGF(fmt, ...) \
    HILOG_FATAL(LOG_CORE, "[%{public}s:%{public}d]" fmt, (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)

#define APPSPAPWN_DUMP(fmt, ...) \
    do { \
        HILOG_INFO(LOG_CORE, fmt, ##__VA_ARGS__); \
        AppSpawnDump(fmt "\n", ##__VA_ARGS__); \
    } while (0)

#else

#define APPSPAWN_LOGI(fmt, ...) \
    HILOG_INFO(HILOG_MODULE_HIVIEW, "[%{public}s:%{public}d]" fmt,  (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGE(fmt, ...) \
    HILOG_ERROR(HILOG_MODULE_HIVIEW, "[%{public}s:%{public}d]" fmt,  (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGV(fmt, ...) \
    HILOG_DEBUG(HILOG_MODULE_HIVIEW, "[%{public}s:%{public}d]" fmt,  (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGW(fmt, ...) \
    HILOG_FATAL(HILOG_MODULE_HIVIEW, "[%{public}s:%{public}d]" fmt,  (APP_FILE_NAME), (__LINE__), ##__VA_ARGS__)
#endif

#define APPSPAWN_CHECK(retCode, exper, fmt, ...) \
    if (!(retCode)) {                    \
        APPSPAWN_LOGE(fmt, ##__VA_ARGS__);         \
        exper;                           \
    }

#define APPSPAWN_CHECK_ONLY_EXPER(retCode, exper) \
    if (!(retCode)) {                  \
        exper;                 \
    }                         \

#define APPSPAWN_ONLY_EXPER(retCode, exper) \
    if ((retCode)) {                  \
        exper;                 \
    }

#define APPSPAWN_CHECK_ONLY_LOG(retCode, fmt, ...) \
    if (!(retCode)) {                    \
        APPSPAWN_LOGE(fmt, ##__VA_ARGS__);      \
    }
#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // APPSPAWN_UTILS_H
