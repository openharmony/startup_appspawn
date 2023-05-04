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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "beget_ext.h"
#include "hilog/log.h"
#include "securec.h"
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
    int (*setXpmRegion)(struct AppSpawnContent_ *content);

    void (*notifyResToParent)(struct AppSpawnContent_ *content, AppSpawnClient *client, int result);
    void (*runChildProcessor)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*setAsanEnabledEnv)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    // for cold start
    int (*coldStartApp)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*getWrapBundleNameValue)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    int (*setSeccompFilter)(struct AppSpawnContent_ *content, AppSpawnClient *client);
    void (*handleInternetPermission)(const AppSpawnClient *client);
    int (*waitForDebugger)(AppSpawnClient *client);
} AppSpawnContent;

typedef struct {
    struct AppSpawnContent_ *content;
    AppSpawnClient *client;
} AppSandboxArg;

AppSpawnContent *AppSpawnCreateContent(const char *socketName, char *longProcName, uint32_t longProcNameLen, int cold);
int AppSpawnProcessMsg(AppSandboxArg *sandbox, pid_t *childPid);
int DoStartApp(struct AppSpawnContent_ *content, AppSpawnClient *client, char *longProcName, uint32_t longProcNameLen);
long long DiffTime(struct timespec *startTime);
pid_t AppSpawnFork(int (*childFunc)(void *arg), void *args);
#define UNUSED(x) (void)(x)

#ifndef OHOS_LITE
#define APPSPAWN_LOG(logLevel, domain, tag, fmt, ...) \
    HiLogPrint(LOG_CORE, (LogLevel)logLevel, domain, tag, \
        "[%{public}s:%{public}d]" fmt,  (FILE_NAME), (__LINE__), ##__VA_ARGS__)

#ifndef APPSPAWN_LABEL
#define APPSPAWN_LABEL "APPSPAWN"
#endif
#define APPSPAWN_DOMAIN (BASE_DOMAIN + 0x11)

#define APPSPAWN_LOGI(fmt, ...) \
    APPSPAWN_LOG(LOG_INFO, APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGE(fmt, ...) \
    APPSPAWN_LOG(LOG_ERROR, APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGV(fmt, ...) \
    APPSPAWN_LOG(LOG_DEBUG, APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGW(fmt, ...) \
    APPSPAWN_LOG(LOG_WARN, APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)
#define APPSPAWN_LOGF(fmt, ...) \
    APPSPAWN_LOG(LOG_FATAL, APPSPAWN_DOMAIN, APPSPAWN_LABEL, fmt, ##__VA_ARGS__)

#else

#define APPSPAWN_LOGI(fmt, ...) \
    HILOG_INFO(HILOG_MODULE_HIVIEW, "[%s:%d]" fmt,  (FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGE(fmt, ...) \
    HILOG_ERROR(HILOG_MODULE_HIVIEW, "[%s:%d]" fmt,  (FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGV(fmt, ...) \
    HILOG_DEBUG(HILOG_MODULE_HIVIEW, "[%s:%d]" fmt,  (FILE_NAME), (__LINE__), ##__VA_ARGS__)
#define APPSPAWN_LOGW(fmt, ...) \
    HILOG_FATAL(HILOG_MODULE_HIVIEW, "[%s:%d]" fmt,  (FILE_NAME), (__LINE__), ##__VA_ARGS__)
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

#define APPSPAWN_CHECK_ONLY_LOG(retCode, fmt, ...) \
    if (!(retCode)) {                    \
        APPSPAWN_LOGE(fmt, ##__VA_ARGS__);      \
    }

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_SERVER_H
