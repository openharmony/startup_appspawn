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

#include "appspawn_service.h"
#include "appspawn_adapter.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/capability.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>

#include "securec.h"
#include "parameter.h"
#include "limits.h"
#include "string.h"

#define DEVICE_NULL_STR "/dev/null"

// ide-asan
static int SetAsanEnabledEnv(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppParameter *appProperty = &((AppSpawnClientExt *)client)->property;
    char *bundleName = appProperty->bundleName;

    if ((appProperty->flags & APP_ASANENABLED) != 0) {
        char *devPath = "/dev/asanlog";
        char logPath[PATH_MAX] = {0};
        int ret = snprintf_s(logPath, sizeof(logPath), sizeof(logPath) - 1,
                "/data/app/el1/100/base/%s/log", bundleName);
        APPSPAWN_CHECK(ret > 0, return -1, "Invalid snprintf_s");
        char asanOptions[PATH_MAX] = {0};
        ret = snprintf_s(asanOptions, sizeof(asanOptions), sizeof(asanOptions) - 1,
                "log_path=%s/asan.log:include=/system/etc/asan.options", devPath);
        APPSPAWN_CHECK(ret > 0, return -1, "Invalid snprintf_s");

#if defined (__aarch64__) || defined (__x86_64__)
        setenv("LD_PRELOAD", "/system/lib64/libclang_rt.asan.so", 1);
#else
        setenv("LD_PRELOAD", "/system/lib/libclang_rt.asan.so", 1);
#endif
        setenv("UBSAN_OPTIONS", asanOptions, 1);
        client->flags |= APP_COLD_START;
    }
    return 0;
}

static int SetProcessName(struct AppSpawnContent_ *content, AppSpawnClient *client,
    char *longProcName, uint32_t longProcNameLen)
{
    AppSpawnClientExt *appPropertyExt = (AppSpawnClientExt *)client;
    AppParameter *appProperty = &appPropertyExt->property;
    int len = strlen(appProperty->processName);
    bool isRet = longProcName == NULL || len <= 0;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "process name is nullptr or length error");

    char shortName[MAX_LEN_SHORT_NAME] = {0};
    // process short name max length 16 bytes.
    if (len >= MAX_LEN_SHORT_NAME) {
        isRet = strncpy_s(shortName, MAX_LEN_SHORT_NAME, appProperty->processName, MAX_LEN_SHORT_NAME - 1) != EOK;
        APPSPAWN_CHECK(!isRet, return -EINVAL, "strncpy_s short name error: %d", errno);
    } else {
        if (strncpy_s(shortName, MAX_LEN_SHORT_NAME, appProperty->processName, len) != EOK) {
            APPSPAWN_LOGE("strncpy_s short name error: %d", errno);
            return -EINVAL;
        }
    }

    // set short name
    isRet = prctl(PR_SET_NAME, shortName) == -1;
    APPSPAWN_CHECK(!isRet, return -errno, "prctl(PR_SET_NAME) error: %d", errno);

    // reset longProcName
    isRet = memset_s(longProcName, (size_t)longProcNameLen, 0, (size_t)longProcNameLen) != EOK;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "Failed to memset long process name");

    // set long process name
    isRet = strncpy_s(longProcName, longProcNameLen, appProperty->processName, len) != EOK;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "strncpy_s long name error: %d longProcNameLen %u", errno, longProcNameLen);

    return 0;
}

static int SetKeepCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    // set keep capabilities when user not root.
    if (appProperty->property.uid != 0) {
        bool isRet = prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == -1;
        APPSPAWN_CHECK(!isRet, return -errno, "set keepcaps failed: %d", errno);
    }
    return 0;
}

static int SetCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    // init cap
    struct __user_cap_header_struct cap_header;

    bool isRet = memset_s(&cap_header, sizeof(cap_header), 0, sizeof(cap_header)) != EOK;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "Failed to memset cap header");

    cap_header.version = _LINUX_CAPABILITY_VERSION_3;
    cap_header.pid = 0;

    struct __user_cap_data_struct cap_data[2];
    isRet = memset_s(&cap_data, sizeof(cap_data), 0, sizeof(cap_data)) != EOK;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "Failed to memset cap data");

    // init inheritable permitted effective zero
#ifdef GRAPHIC_PERMISSION_CHECK
    const uint64_t inheriTable = 0;
    const uint64_t permitted = 0;
    const uint64_t effective = 0;
#else
    const uint64_t inheriTable = 0x3fffffffff;
    const uint64_t permitted = 0x3fffffffff;
    const uint64_t effective = 0x3fffffffff;
#endif

    cap_data[0].inheritable = (__u32)(inheriTable);
    cap_data[1].inheritable = (__u32)(inheriTable >> BITLEN32);
    cap_data[0].permitted = (__u32)(permitted);
    cap_data[1].permitted = (__u32)(permitted >> BITLEN32);
    cap_data[0].effective = (__u32)(effective);
    cap_data[1].effective = (__u32)(effective >> BITLEN32);

    // set capabilities
    isRet = capset(&cap_header, &cap_data[0]) == -1;
    APPSPAWN_CHECK(!isRet, return -errno, "capset failed: %d", errno);
    SetSelinuxCon(content, client);
    return 0;
}

static void InitDebugParams(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifndef APPSPAWN_TEST
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
#if defined (__aarch64__) || defined (__x86_64__)
    const char *debugSoPath = "/system/lib64/libhidebug.so";
#else
    const char *debugSoPath = "/system/lib/libhidebug.so";
#endif
    bool isRet = access(debugSoPath, F_OK) != 0;
    APPSPAWN_CHECK(!isRet, return, "access failed, errno = %d", errno);

    void *handle = dlopen(debugSoPath, RTLD_LAZY);
    APPSPAWN_CHECK(handle != NULL, return, "Failed to dlopen libhidebug.so, %s", dlerror());

    bool (*initParam)(const char *name);
    initParam = (bool (*)(const char *name))dlsym(handle, "InitEnvironmentParam");
    APPSPAWN_CHECK(initParam != NULL, dlclose(handle);
        return, "Failed to dlsym InitEnvironmentParam, %s", dlerror());
    (*initParam)(appProperty->property.processName);
    dlclose(handle);
#endif
}

static void ClearEnvironment(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGI("ClearEnvironment id %d", client->id);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // close child fd
#ifndef APPSPAWN_TEST
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    close(appProperty->fd[0]);
#endif
    SetAsanEnabledEnv(content, client);
    return;
}

static int SetUidGid(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
#ifdef GRAPHIC_PERMISSION_CHECK
    // set gids
    bool isRet = setgroups(appProperty->property.gidCount, (const gid_t *)(&appProperty->property.gidTable[0])) == -1;
    APPSPAWN_CHECK(!isRet, return -errno, "setgroups failed: %d, gids.size=%u", errno, appProperty->property.gidCount);

    if (client->cloneFlags & CLONE_NEWPID) {
        /* setresuid and setresgid have multi-thread synchronous operations.
         * after clone, the C library has not cleaned up the multi-thread information, so need to call syscall.
         */
        // set gid
        long ret = syscall(SYS_setresgid, appProperty->property.gid,
            appProperty->property.gid, appProperty->property.gid);
        APPSPAWN_CHECK(ret == 0, return -errno, "setgid(%u) failed: %d", appProperty->property.gid, errno);

        /* If the effective user ID is changed from 0 to nonzero,
         * then all capabilities are cleared from the effective set
         */
        ret = syscall(SYS_setresuid, appProperty->property.uid, appProperty->property.uid, appProperty->property.uid);
        APPSPAWN_CHECK(ret == 0, return -errno, "setuid(%u) failed: %d", appProperty->property.uid, errno);
    } else {
        // set gid
        isRet = setresgid(appProperty->property.gid, appProperty->property.gid, appProperty->property.gid) == -1;
        APPSPAWN_CHECK(!isRet, return -errno, "setgid(%u) failed: %d", appProperty->property.gid, errno);

        /* If the effective user ID is changed from 0 to nonzero,
         * then all capabilities are cleared from the effective set
         */
        isRet = setresuid(appProperty->property.uid, appProperty->property.uid, appProperty->property.uid) == -1;
        APPSPAWN_CHECK(!isRet, return -errno, "setuid(%u) failed: %d", appProperty->property.uid, errno);
    }
#endif
    if ((appProperty->property.flags & APP_DEBUGGABLE) != 0) {
        APPSPAWN_LOGV("Debuggable app");
        setenv("HAP_DEBUGGABLE", "true", 1);
        if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
            APPSPAWN_LOGE("Failed to set app dumpable: %s", strerror(errno));
        }
    }
    return 0;
}

static int32_t SetFileDescriptors(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifndef APPSPAWN_TEST
    // close stdin stdout stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // redirect to /dev/null
    int dev_null_fd = open(DEVICE_NULL_STR, O_RDWR);
    if (dev_null_fd == -1) {
        APPSPAWN_LOGE("open dev_null error: %d", errno);
        return (-errno);
    }

    // stdin
    if (dup2(dev_null_fd, STDIN_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDIN error: %d", errno);
        return (-errno);
    };

    // stdout
    if (dup2(dev_null_fd, STDOUT_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDOUT error: %d", errno);
        return (-errno);
    };

    // stderr
    if (dup2(dev_null_fd, STDERR_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDERR error: %d", errno);
        return (-errno);
    };
#endif
    return 0;
}

static void Free(char **argv)
{
    argv[0] = NULL;
    for (int i = 0; i < NULL_INDEX; i++) {
        if (argv[i] != NULL) {
            free(argv[i]);
            argv[i] = NULL;
        }
    }
    free(argv);
}

#ifdef ASAN_DETECTOR
#define WRAP_VALUE_MAX_LENGTH 96
static int GetWrapBundleNameValue(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppParameter *appProperty = &((AppSpawnClientExt *)client)->property;
    char wrapBundleNameKey[WRAP_VALUE_MAX_LENGTH] = {0};
    char wrapBundleNameValue[WRAP_VALUE_MAX_LENGTH] = {0};

    int len = sprintf_s(wrapBundleNameKey, WRAP_VALUE_MAX_LENGTH, "wrap.%s", appProperty->bundleName);
    APPSPAWN_CHECK(len > 0 && (len < WRAP_VALUE_MAX_LENGTH), return -1, "Invalid to format wrapBundleNameKey");

    int ret = GetParameter(wrapBundleNameKey, "", wrapBundleNameValue, WRAP_VALUE_MAX_LENGTH);
    APPSPAWN_CHECK(ret > 0 && (!strcmp(wrapBundleNameValue, "asan_wrapper")), return -1,
                   "Not wrap %s.", appProperty->bundleName);
    APPSPAWN_LOGI("Asan: GetParameter %s the value is %s.", wrapBundleNameKey, wrapBundleNameValue);
    return 0;
}
#endif

static int ColdStartApp(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppParameter *appProperty = &((AppSpawnClientExt *)client)->property;
    APPSPAWN_LOGI("ColdStartApp::appName %s", appProperty->processName);
    char buffer[32] = {0};  // 32 buffer for fd
    int len = sprintf_s(buffer, sizeof(buffer), "%d", ((AppSpawnClientExt *)client)->fd[1]);
    APPSPAWN_CHECK(len > 0, return -1, "Invalid to format fd");
    char **argv = calloc(1, (NULL_INDEX + 1) * sizeof(char *));
    APPSPAWN_CHECK(argv != NULL, return -1, "Failed to get argv");

    int32_t startLen = 0;
    const int32_t originLen = sizeof(AppParameter) + PARAM_BUFFER_LEN;
    // param
    char *param = malloc(originLen + APP_LEN_PROC_NAME);
    APPSPAWN_CHECK(param != NULL, free(argv);
        return -1, "Failed to malloc for param");

    int ret = -1;
    do {
        argv[PARAM_INDEX] = param;
        argv[0] = param + originLen;
#ifdef ASAN_DETECTOR
        if (GetWrapBundleNameValue(content, client) == 0) {
            ret = strcpy_s(argv[0], APP_LEN_PROC_NAME, "/system/asan/bin/appspawn");
        } else {
            ret = strcpy_s(argv[0], APP_LEN_PROC_NAME, "/system/bin/appspawn");
        }
#else
        ret = strcpy_s(argv[0], APP_LEN_PROC_NAME, "/system/bin/appspawn");
#endif
        APPSPAWN_CHECK(ret >= 0, break, "Invalid strcpy");
        ret = -1;
        argv[START_INDEX] = strdup("cold-start");
        APPSPAWN_CHECK(argv[START_INDEX] != NULL, break, "Invalid strdup");
        argv[FD_INDEX] = strdup(buffer);
        APPSPAWN_CHECK(argv[FD_INDEX] != NULL, break, "Invalid strdup");

        len = sprintf_s(param + startLen, originLen - startLen, "%u:%u:%u:%u:%u",
            ((AppSpawnClientExt *)client)->client.id, ((AppSpawnClientExt *)client)->client.cloneFlags,
            appProperty->uid, appProperty->gid, appProperty->gidCount);
        APPSPAWN_CHECK(len > 0 && (len < (originLen - startLen)), break, "Invalid to format");
        startLen += len;
        for (uint32_t i = 0; i < appProperty->gidCount; i++) {
            len = sprintf_s(param + startLen, originLen - startLen, ":%u", appProperty->gidTable[i]);
            APPSPAWN_CHECK(len > 0 && (len < (originLen - startLen)), break, "Invalid to format gid");
            startLen += len;
        }
        // processName
        if (appProperty->soPath[0] == '\0') {
            strcpy_s(appProperty->soPath, sizeof(appProperty->soPath), "NULL");
        }
        len = sprintf_s(param + startLen, originLen - startLen, ":%s:%s:%s:%u:%s:%s",
            appProperty->processName, appProperty->bundleName, appProperty->soPath,
            appProperty->accessTokenId, appProperty->apl, appProperty->renderCmd);
        APPSPAWN_CHECK(len > 0 && (len < (originLen - startLen)), break, "Invalid to format processName");
        startLen += len;
        ret = 0;
    } while (0);

    if (ret == 0) {
        argv[NULL_INDEX] = NULL;
#ifndef APPSPAWN_TEST
        ret = execv(argv[0], argv);
#endif
        if (ret) {
            APPSPAWN_LOGE("Failed to execv, errno = %d", errno);
        }
    }
    Free(argv);
    return ret;
}

int GetAppSpawnClientFromArg(int argc, char *const argv[], AppSpawnClientExt *client)
{
    APPSPAWN_CHECK(argv != NULL && argc > PARAM_INDEX, return -1, "Invalid argv argc %d", argc);

    client->fd[1] = atoi(argv[FD_INDEX]);
    APPSPAWN_LOGV("GetAppSpawnClientFromArg %s ", argv[PARAM_INDEX]);
    char *end = NULL;
    char *start = strtok_r(argv[PARAM_INDEX], ":", &end);

    // clientid
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get client id");
    client->client.id = atoi(start);
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get client cloneFlags");
    client->client.cloneFlags = atoi(start);
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get uid");
    client->property.uid = atoi(start);
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get gid");
    client->property.gid = atoi(start);

    // gidCount
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get gidCount");
    client->property.gidCount = atoi(start);
    for (uint32_t i = 0; i < client->property.gidCount; i++) {
        start = strtok_r(NULL, ":", &end);
        APPSPAWN_CHECK(start != NULL, return -1, "Failed to get gidTable");
        client->property.gidTable[i] = atoi(start);
    }

    // processname
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get processName");
    int ret = strcpy_s(client->property.processName, sizeof(client->property.processName), start);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy processName");
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get bundleName");
    ret = strcpy_s(client->property.bundleName, sizeof(client->property.bundleName), start);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy bundleName");
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get soPath");
    if (strcmp(start, "NULL")) {
        ret = strcpy_s(client->property.soPath, sizeof(client->property.soPath), start);
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy soPath");
    } else {
        client->property.soPath[0] = '\0';
    }

    // accesstoken
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get accessTokenId");
    client->property.accessTokenId = atoi(start);
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get apl");
    ret = strcpy_s(client->property.apl, sizeof(client->property.apl), start);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy apl");
    start = strtok_r(NULL, ":", &end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get renderCmd");
    ret = strcpy_s(client->property.renderCmd, sizeof(client->property.renderCmd), start);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy renderCmd");

    return 0;
}

void SetContentFunction(AppSpawnContent *content)
{
    APPSPAWN_LOGI("SetContentFunction");
    content->clearEnvironment = ClearEnvironment;
    content->initDebugParams = InitDebugParams;
    content->setProcessName = SetProcessName;
    content->setKeepCapabilities = SetKeepCapabilities;
    content->setUidGid = SetUidGid;
    content->setCapabilities = SetCapabilities;
    content->setFileDescriptors = SetFileDescriptors;
    content->setAppSandbox = SetAppSandboxProperty;
    content->setAppAccessToken = SetAppAccessToken;
    content->coldStartApp = ColdStartApp;
#ifdef ASAN_DETECTOR
    content->getWrapBundleNameValue = GetWrapBundleNameValue;
#endif
    content->setSeccompFilter = SetSeccompFilter;
    content->setAsanEnabledEnv = SetAsanEnabledEnv;
}
