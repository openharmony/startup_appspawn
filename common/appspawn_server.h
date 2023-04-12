/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef APPSPAWN_SERVER_H
#define APPSPAWN_SERVER_H
#include "beget_ext.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#ifdef OHOS_DEBUG
#include <time.h>
#endif  // OHOS_DEBUG

#ifdef __cplusplus
extern "C" {
#endif

#define UNUSED(x) (void)(x)
#define APP_COLD_START 0x01
#define ERR_PIPE_FAIL (-100)
#define MAX_LEN_SHORT_NAME 16
#define WAIT_DELAY_US (100 * 1000)  // 100ms
#define GID_FILE_ACCESS 1006  // only used for ExternalFileManager.hap
#define GID_USER_DATA_RW 1008

typedef struct AppSpawnClient_ {
    uint32_t id;
    uint32_t flags;
    uint32_t cloneFlags;
#ifndef APPSPAWN_TEST
#ifndef OHOS_LITE
    uint8_t setAllowInternet;
    uint8_t allowInternet;
    uint8_t reserved1;
    uint8_t reserved2;
#endif
#endif
} AppSpawnClient;

#define MAX_SOCKEYT_NAME_LEN 128
typedef struct AppSpawnContent_ {
    char *longProcName;
    uint32_t longProcNameLen;

    // system
    void (*loadExtendLib)(struct AppSpawnContent_ *content);
    void (*initAppSpawn)(struct AppSpawnContent_ *content);
    void (*runAppSpawn)(struct AppSpawnContent_ *content, int argc, char *const argv[]);
    void (*setUidGidFilter)(struct AppSpawnContent_ *content);

    // for child
    void (*clearEnvironment)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    void (*initDebugParams)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    void (*setAppAccessToken)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*setAppSandbox)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*setKeepCapabilities)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*setFileDescriptors)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*setProcessName)(struct AppSpawnContent_ *content, AppSpawnClient *client,
        char *longProcName, uint32_t longProcNameLen);
    int (*setUidGid)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*setCapabilities)(struct AppSpawnContent_ *content, AppSpawnClient *client);

    void (*notifyResToParent)(struct AppSpawnContent_ *content, AppSpawnClient *client, int result);
    void (*runChildProcessor)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*setAsanEnabledEnv)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    // for cold start
    int (*coldStartApp)(struct AppSpawnContent_ *content, AppSpawnClient *client);
#ifdef ASAN_DETECTOR
    int (*getWrapBundleNameValue)(struct AppSpawnContent_ *content, AppSpawnClient *client);
#endif
    void (*setSeccompFilter)(struct AppSpawnContent_ *content, AppSpawnClient *client);
} AppSpawnContent;

typedef struct {
    struct AppSpawnContent_ *content;
    AppSpawnClient *client;
} AppSandboxArg;

AppSpawnContent *AppSpawnCreateContent(const char *socketName, char *longProcName, uint32_t longProcNameLen, int cold);
int AppSpawnProcessMsg(AppSandboxArg *sandbox, pid_t *childPid);
int DoStartApp(struct AppSpawnContent_ *content, AppSpawnClient *client, char *longProcName, uint32_t longProcNameLen);
int AppSpawnChild(void *arg);

#ifdef OHOS_DEBUG
void GetCurTime(struct timespec* tmCur);
#endif

typedef enum {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL,
} AppspawnLogLevel;

void AppspawnLogPrint(AppspawnLogLevel logLevel, const char *file, int line, const char *fmt, ...);

#ifndef FILE_NAME
#define FILE_NAME   (strrchr((__FILE__), '/') ? strrchr((__FILE__), '/') + 1 : (__FILE__))
#endif

#define UNUSED(x) (void)(x)

#ifndef APPSPAWN_LABEL
#define APPSPAWN_LABEL "APPSPAWN"
#endif
#define APPSPAWN_DOMAIN (BASE_DOMAIN + 0x11)

#define APPSPAWN_LOGI(fmt, ...) STARTUP_LOGI(APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGE(fmt, ...) STARTUP_LOGE(APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGV(fmt, ...) STARTUP_LOGV(APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGW(fmt, ...) STARTUP_LOGW(APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGF(fmt, ...) STARTUP_LOGF(APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)

#define APPSPAWN_CHECK(retCode, exper, ...) \
    if (!(retCode)) {                    \
        APPSPAWN_LOGE(__VA_ARGS__);         \
        exper;                           \
    }

#define APPSPAWN_CHECK_ONLY_EXPER(retCode, exper) \
    if (!(retCode)) {                  \
        exper;                 \
    }                         \

#define APPSPAWN_CHECK_ONLY_LOG(retCode, ...) \
    if (!(retCode)) {                    \
        APPSPAWN_LOGE(__VA_ARGS__);      \
    }

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_SERVER_H
