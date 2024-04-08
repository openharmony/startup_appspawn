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

#include "appspawn_server.h"

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
#else
#define APPSPAWN_SOCKET_DIR APPSPAWN_BASE_DIR "/dev/socket/"
#endif

#define APPSPAWN_CHECK_EXIT "AppSpawnCheckUnexpectedExitCall"
#define UNUSED(x) (void)(x)

#define APP_COLD_START 0x01
#define APP_ASAN_DETECTOR 0x02
#define APP_DEVELOPER_MODE 0x04

#define MAX_LEN_SHORT_NAME 16
#define DEFAULT_UMASK 0002
#define UID_BASE 200000 // 20010029
#define DEFAULT_DIR_MODE 0711
#define USER_ID_BUFFER_SIZE 32

#define APPSPAWN_SEC_TO_NSEC 1000000000
#define APPSPAWN_MSEC_TO_NSEC 1000000
#define APPSPAWN_USEC_TO_NSEC 1000
#define APPSPAWN_SEC_TO_MSEC 1000

#define MAX_JSON_FILE_LEN 102400

#define CHECK_FLAGS_BY_INDEX(flags, index) ((((flags) >> (index)) & 0x1) == 0x1)
#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))
#endif

#define INVALID_PERMISSION_INDEX (-1)

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
    APPSPAWN_SANDBOX_MOUNT_FAIL,
    APPSPAWN_SPAWN_TIMEOUT,
    APPSPAWN_CHILD_CRASH,
    APPSPAWN_NATIVE_NOT_SUPPORT,
    APPSPAWN_ACCESS_TOKEN_INVALID,
    APPSPAWN_PERMISSION_NOT_SUPPORT,
    APPSPAWN_BUFFER_NOT_ENOUGH,
    APPSPAWN_TIMEOUT,
    APPSPAWN_FORK_FAIL,
    APPSPAWN_NODE_EXIST,
} AppSpawnErrorCode;

typedef struct cJSON cJSON;
typedef struct TagParseJsonContext ParseJsonContext;
typedef int (*ParseConfig)(const cJSON *root, ParseJsonContext *context);
int ParseJsonConfig(const char *path, const char *fileName, ParseConfig parseConfig, ParseJsonContext *context);
cJSON *GetJsonObjFromFile(const char *jsonPath);

uint8_t *Base64Decode(const char *data, uint32_t dataLen, uint32_t *outLen);
char *Base64Encode(const uint8_t *data, uint32_t len);
void AppSpawnDump(const char *fmt, ...);
void SetDumpToStream(FILE *stream);

typedef int (*SplitStringHandle)(const char *str, void *context);
int32_t StringSplit(const char *str, const char *separator, void *context, SplitStringHandle handle);
char *GetLastStr(const char *str, const char *dst);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // APPSPAWN_UTILS_H
