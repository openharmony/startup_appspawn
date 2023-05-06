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
#include <inttypes.h>
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

struct XpmRegionInfo {
    unsigned long addr;
    unsigned long length;
};

#define XPM_DEV_PATH "/dev/xpm"
#define XPM_REGION_LEN 0x8000000
#define SET_XPM_REGION _IOW('x', 0x01, struct XpmRegionInfo)

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
    size_t len = strlen(appProperty->processName);
    bool isRet = longProcName == NULL || len <= 0;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "process name is nullptr or length error");

    char shortName[MAX_LEN_SHORT_NAME] = {0};
    // process short name max length 16 bytes.
    size_t copyLen = len;
    const char *pos = appProperty->processName;
    if (len >= MAX_LEN_SHORT_NAME) {
        copyLen = MAX_LEN_SHORT_NAME - 1;
        pos += (len - copyLen);
    }
    isRet = strncpy_s(shortName, MAX_LEN_SHORT_NAME, pos, copyLen) != EOK;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "strncpy_s short name error: %{public}d", errno);

    // set short name
    isRet = prctl(PR_SET_NAME, shortName) == -1;
    APPSPAWN_CHECK(!isRet, return -errno, "prctl(PR_SET_NAME) error: %{public}d", errno);

    // reset longProcName
    isRet = memset_s(longProcName, (size_t)longProcNameLen, 0, (size_t)longProcNameLen) != EOK;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "Failed to memset long process name");

    // set long process name
    isRet = strncpy_s(longProcName, longProcNameLen, appProperty->processName, len) != EOK;
    APPSPAWN_CHECK(!isRet, return -EINVAL,
        "strncpy_s long name error: %{public}d longProcNameLen %{public}u", errno, longProcNameLen);

    return 0;
}

static int SetKeepCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    // set keep capabilities when user not root.
    if (appProperty->property.uid != 0) {
        bool isRet = prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == -1;
        APPSPAWN_CHECK(!isRet, return -errno, "set keepcaps failed: %{public}d", errno);
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
    APPSPAWN_CHECK(!isRet, return -errno, "capset failed: %{public}d", errno);
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
    APPSPAWN_CHECK(!isRet, return, "access failed, errno = %{public}d", errno);

    void *handle = dlopen(debugSoPath, RTLD_LAZY);
    APPSPAWN_CHECK(handle != NULL, return, "Failed to dlopen libhidebug.so, %{public}s", dlerror());

    bool (*initParam)(const char *name);
    initParam = (bool (*)(const char *name))dlsym(handle, "InitEnvironmentParam");
    APPSPAWN_CHECK(initParam != NULL, dlclose(handle);
        return, "Failed to dlsym InitEnvironmentParam, %{public}s", dlerror());
    (*initParam)(appProperty->property.processName);
    dlclose(handle);
#endif
}

static void ClearEnvironment(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGI("ClearEnvironment id %{public}d", client->id);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // close child fd
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    close(appProperty->fd[0]);
    SetAsanEnabledEnv(content, client);
    return;
}

int SetXpmRegion(struct AppSpawnContent_ *content)
{
    struct XpmRegionInfo info = { 0, XPM_REGION_LEN };

    // 32-bit system no xpm dev file
    int fd = open(XPM_DEV_PATH, O_RDWR);
    APPSPAWN_CHECK(fd != -1, return 0, "open xpm device file failed: %s", strerror(errno));

    int ret = ioctl(fd, SET_XPM_REGION, &info);
    APPSPAWN_CHECK_ONLY_LOG(ret != -1, "set xpm region failed: %s", strerror(errno));

    close(fd);
    return ret;
}

static int SetUidGid(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifdef GRAPHIC_PERMISSION_CHECK
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    // set gids
    bool isRet = setgroups(appProperty->property.gidCount, (const gid_t *)(&appProperty->property.gidTable[0])) == -1;
    APPSPAWN_CHECK(!isRet, return -errno,
        "setgroups failed: %{public}d, gids.size=%{public}u", errno, appProperty->property.gidCount);

    if (client->cloneFlags & CLONE_NEWPID) {
        /* setresuid and setresgid have multi-thread synchronous operations.
         * after clone, the C library has not cleaned up the multi-thread information, so need to call syscall.
         */
        // set gid
        long ret = syscall(SYS_setresgid, appProperty->property.gid,
            appProperty->property.gid, appProperty->property.gid);
        APPSPAWN_CHECK(ret == 0, return -errno,
            "setgid(%{public}u) failed: %{public}d", appProperty->property.gid, errno);

        if (content->setSeccompFilter) {
            ret = content->setSeccompFilter(content, client);
            APPSPAWN_CHECK(ret == 0, return ret, "Failed to set setSeccompFilter");
        }

        /* If the effective user ID is changed from 0 to nonzero,
         * then all capabilities are cleared from the effective set
         */
        ret = syscall(SYS_setresuid, appProperty->property.uid, appProperty->property.uid, appProperty->property.uid);
        APPSPAWN_CHECK(ret == 0, return -errno,
            "setuid(%{public}u) failed: %{public}d", appProperty->property.uid, errno);
    } else {
        // set gid
        isRet = setresgid(appProperty->property.gid, appProperty->property.gid, appProperty->property.gid) == -1;
        APPSPAWN_CHECK(!isRet, return -errno,
            "setgid(%{public}u) failed: %{public}d", appProperty->property.gid, errno);

        if (content->setSeccompFilter) {
            long ret = content->setSeccompFilter(content, client);
            APPSPAWN_CHECK(ret == 0, return ret, "Failed to set setSeccompFilter");
        }

        /* If the effective user ID is changed from 0 to nonzero,
         * then all capabilities are cleared from the effective set
         */
        isRet = setresuid(appProperty->property.uid, appProperty->property.uid, appProperty->property.uid) == -1;
        APPSPAWN_CHECK(!isRet, return -errno,
            "setuid(%{public}u) failed: %{public}d", appProperty->property.uid, errno);
    }
#endif
    if ((appProperty->property.flags & APP_DEBUGGABLE) != 0) {
        APPSPAWN_LOGV("Debuggable app");
#ifndef ASAN_DETECTOR
        setenv("HAP_DEBUGGABLE", "true", 1);
#endif
        if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
            APPSPAWN_LOGE("Failed to set app dumpable: %{public}s", strerror(errno));
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
        APPSPAWN_LOGE("open dev_null error: %{public}d", errno);
        return (-errno);
    }

    // stdin
    if (dup2(dev_null_fd, STDIN_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDIN error: %{public}d", errno);
        return (-errno);
    };

    // stdout
    if (dup2(dev_null_fd, STDOUT_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDOUT error: %{public}d", errno);
        return (-errno);
    };

    // stderr
    if (dup2(dev_null_fd, STDERR_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDERR error: %{public}d", errno);
        return (-errno);
    };
#endif
    return 0;
}

static int32_t CheckTraceStatus(void)
{
    int fd = open("/proc/self/status", O_RDONLY);
    if (fd == -1) {
        APPSPAWN_LOGE("open /proc/self/status error: %{public}d", errno);
        return (-errno);
    }

    char data[1024];
    ssize_t dataNum = read(fd, data, sizeof(data));
    if (close(fd) < 0) {
        APPSPAWN_LOGE("close fd error: %{public}d", errno);
        return (-errno);
    }

    if (dataNum <= 0) {
        APPSPAWN_LOGE("fail to read data");
        return -1;
    }

    const char* tracerPid = "TracerPid:\t";
    char *traceStr = strstr(data, tracerPid);
    if (traceStr == NULL) {
        APPSPAWN_LOGE("fail to find %{public}s", tracerPid);
        return -1;
    }
    char *separator = strchr(traceStr, '\n');
    if (separator == NULL) {
        APPSPAWN_LOGE("fail to find line break");
        return -1;
    }

    int len = separator - traceStr - strlen(tracerPid);
    char pid = *(traceStr + strlen(tracerPid));
    if (len > 1 || pid != '0') {
        return 0;
    }
    return -1;
}

static int32_t WaitForDebugger(AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;

    // wait for debugger only debugging is required and process is debuggable
    if ((appProperty->property.flags & APP_NATIVEDEBUG) != 0 &&
        (appProperty->property.flags & APP_DEBUGGABLE) != 0) {
        uint32_t count = 0;
        while (CheckTraceStatus() != 0) {
            usleep(1000 * 100); // sleep 1000 * 100 microsecond
            count++;
            // remind users to connect to the debugger every 60 * 10 times
            if (count % (10 * 60) == 0) {
                count = 0;
                APPSPAWN_LOGI("wait for debugger, please attach the process");
            }
        }
    }
    return 0;
}

static void Free(char **argv, HspList *hspList)
{
    argv[0] = NULL;
    for (int i = 0; i < NULL_INDEX; i++) {
        if (argv[i] != NULL) {
            free(argv[i]);
            argv[i] = NULL;
        }
    }
    free(argv);

    if (hspList != NULL) {
        hspList->totalLength = 0;
        hspList->savedLength = 0;
        hspList->data = NULL;
    }
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
                   "Not wrap %{public}s.", appProperty->bundleName);
    APPSPAWN_LOGI("Asan: GetParameter %{public}s the value is %{public}s.", wrapBundleNameKey, wrapBundleNameValue);
    return 0;
}
#endif

static int EncodeAppClient(AppSpawnClient *client, char *param, int32_t originLen)
{
    AppParameter *appProperty = &((AppSpawnClientExt *)client)->property;
    int32_t startLen = 0;
    int32_t len = sprintf_s(param + startLen, originLen - startLen, "%u:%u:%u:%u:%u:%u:%u:%u:%u:%u",
        client->id, client->flags, client->cloneFlags, appProperty->code,
        appProperty->flags, appProperty->uid, appProperty->gid,
        appProperty->setAllowInternet, appProperty->allowInternet, appProperty->gidCount);
    APPSPAWN_CHECK(len > 0 && (len < (originLen - startLen)), return -1, "Invalid to format");
    startLen += len;
    for (uint32_t i = 0; i < appProperty->gidCount; i++) {
        len = sprintf_s(param + startLen, originLen - startLen, ":%u", appProperty->gidTable[i]);
        APPSPAWN_CHECK(len > 0 && (len < (originLen - startLen)), return -1, "Invalid to format gid");
        startLen += len;
    }
    // processName
    if (appProperty->soPath[0] == '\0') {
        strcpy_s(appProperty->soPath, sizeof(appProperty->soPath), "NULL");
    }
    len = sprintf_s(param + startLen, originLen - startLen, ":%s:%s:%s:%u:%s:%s:%u:%" PRId64 "",
        appProperty->processName, appProperty->bundleName, appProperty->soPath,
        appProperty->accessTokenId, appProperty->apl, appProperty->renderCmd,
        appProperty->hapFlags, appProperty->accessTokenIdEx);
    APPSPAWN_CHECK(len > 0 && (len < (originLen - startLen)), return -1, "Invalid to format processName");
    startLen += len;
    return 0;
}

static int ColdStartApp(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppParameter *appProperty = &((AppSpawnClientExt *)client)->property;
    APPSPAWN_LOGI("ColdStartApp::appName %{public}s", appProperty->processName);
    char buffer[32] = {0};  // 32 buffer for fd
    int len = sprintf_s(buffer, sizeof(buffer), "%d", ((AppSpawnClientExt *)client)->fd[1]);
    APPSPAWN_CHECK(len > 0, return -1, "Invalid to format fd");
    char **argv = calloc(1, (NULL_INDEX + 1) * sizeof(char *));
    APPSPAWN_CHECK(argv != NULL, return -1, "Failed to get argv");
    int ret = -1;
    do {
        const int32_t originLen = sizeof(AppParameter) + PARAM_BUFFER_LEN;
        char *param = malloc(originLen + APP_LEN_PROC_NAME);
        APPSPAWN_CHECK(param != NULL, break, "Failed to malloc for param");
        argv[PARAM_INDEX] = param;
        argv[0] = param + originLen;
        const char *appSpawnPath = "/system/bin/appspawn";
#ifdef ASAN_DETECTOR
        if (GetWrapBundleNameValue(content, client) == 0) {
            appSpawnPath = "/system/asan/bin/appspawn";
        }
#endif
        ret = strcpy_s(argv[0], APP_LEN_PROC_NAME, appSpawnPath);
        APPSPAWN_CHECK(ret >= 0, break, "Invalid strcpy");
        ret = -1;
        argv[START_INDEX] = strdup("cold-start");
        APPSPAWN_CHECK(argv[START_INDEX] != NULL, break, "Invalid strdup");
        argv[FD_INDEX] = strdup(buffer);
        APPSPAWN_CHECK(argv[FD_INDEX] != NULL, break, "Invalid strdup");
        ret = EncodeAppClient(client, param, originLen);
        APPSPAWN_CHECK(ret == 0, break, "Failed to encode client");

        len = sprintf_s(buffer, sizeof(buffer), "%u", appProperty->hspList.totalLength);
        APPSPAWN_CHECK(len > 0 && len < (int)sizeof(buffer), break, "Invalid hspList.totalLength");
        argv[HSP_LIST_LEN_INDEX] = strdup(buffer);
        argv[HSP_LIST_INDEX] = appProperty->hspList.data;
        ret = 0;
    } while (0);

    if (ret == 0) {
        argv[NULL_INDEX] = NULL;
#ifndef APPSPAWN_TEST
        ret = execv(argv[0], argv);
#else
        ret = -1;
#endif
        if (ret) {
            APPSPAWN_LOGE("Failed to execv, errno = %{public}d", errno);
        }
    }
    argv[0] = NULL;
    Free(argv, &appProperty->hspList);
    return ret;
}

static int GetUInt32FromArg(char *begin, char **end, uint32_t *value)
{
    char *start = strtok_r(begin, ":", end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get uint32 value");
    *value = atoi(start);
    return 0;
}

static int GetUInt64FromArg(char *begin, char **end, uint64_t *value)
{
    char *start = strtok_r(begin, ":", end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get uint64 value");
    APPSPAWN_LOGV("GetUInt64FromArg %{public}s ", start);
    *value = atoll(start);
    return 0;
}

static int GetStringFromArg(char *begin, char **end, char *value, uint32_t valueLen)
{
    char *start = strtok_r(NULL, ":", end);
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get string");
    if (strcmp(start, "NULL")) {
        return strcpy_s(value, valueLen, start);
    } else {
        value[0] = '\0';
    }
    return 0;
}

int GetAppSpawnClientFromArg(int argc, char *const argv[], AppSpawnClientExt *client)
{
    APPSPAWN_CHECK(argv != NULL && argc > PARAM_INDEX, return -1, "Invalid argv argc %{public}d", argc);
    client->fd[1] = atoi(argv[FD_INDEX]);
    APPSPAWN_LOGV("GetAppSpawnClientFromArg %{public}s ", argv[PARAM_INDEX]);
    // clientid
    char *end = NULL;
    int ret = GetUInt32FromArg(argv[PARAM_INDEX], &end, &client->client.id);
    ret += GetUInt32FromArg(NULL, &end, &client->client.flags);
    ret += GetUInt32FromArg(NULL, &end, &client->client.cloneFlags);
    ret += GetUInt32FromArg(NULL, &end, &client->property.code);
    ret += GetUInt32FromArg(NULL, &end, &client->property.flags);
    ret += GetUInt32FromArg(NULL, &end, &client->property.uid);
    ret += GetUInt32FromArg(NULL, &end, &client->property.gid);
    uint32_t value = 0;
    ret += GetUInt32FromArg(NULL, &end, &value);
    client->property.setAllowInternet = (uint8_t)value;
    ret += GetUInt32FromArg(NULL, &end, &value);
    client->property.allowInternet = (uint8_t)value;
    ret += GetUInt32FromArg(NULL, &end, &client->property.gidCount);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get client info");
    for (uint32_t i = 0; i < client->property.gidCount; i++) {
        ret = GetUInt32FromArg(NULL, &end, &client->property.gidTable[i]);
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to get gidTable");
    }

    // processname
    ret = GetStringFromArg(NULL, &end, client->property.processName, sizeof(client->property.processName));
    ret += GetStringFromArg(NULL, &end, client->property.bundleName, sizeof(client->property.bundleName));
    ret += GetStringFromArg(NULL, &end, client->property.soPath, sizeof(client->property.soPath));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get process name");

    // access token
    ret = GetUInt32FromArg(NULL, &end, &client->property.accessTokenId);
    ret += GetStringFromArg(NULL, &end, client->property.apl, sizeof(client->property.apl));
    ret += GetStringFromArg(NULL, &end, client->property.renderCmd, sizeof(client->property.renderCmd));
    ret += GetUInt32FromArg(NULL, &end, &value);
    client->property.hapFlags = (int32_t)value;
    ret += GetUInt64FromArg(NULL, &end, &client->property.accessTokenIdEx);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to access token info");

    client->property.hspList.totalLength = 0;
    client->property.hspList.data = NULL;
    ret = -1;
    if (argc > HSP_LIST_LEN_INDEX && argv[HSP_LIST_LEN_INDEX] != NULL) {
        client->property.hspList.totalLength = atoi(argv[HSP_LIST_LEN_INDEX]);
        APPSPAWN_CHECK_ONLY_EXPER(client->property.hspList.totalLength != 0, return 0);
        APPSPAWN_CHECK(argc > HSP_LIST_INDEX && argv[HSP_LIST_INDEX] != NULL, return -1, "Invalid hspList.data");
        client->property.hspList.data = malloc(client->property.hspList.totalLength);
        APPSPAWN_CHECK(client->property.hspList.data != NULL, return -1, "Failed to malloc hspList.data");
        ret = strcpy_s(client->property.hspList.data, client->property.hspList.totalLength, argv[HSP_LIST_INDEX]);
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy hspList.data");
    }
    return ret;
}

void SetContentFunction(AppSpawnContent *content)
{
    APPSPAWN_LOGI("SetContentFunction");
    content->clearEnvironment = ClearEnvironment;
    content->initDebugParams = InitDebugParams;
    content->setProcessName = SetProcessName;
    content->setKeepCapabilities = SetKeepCapabilities;
    content->setUidGid = SetUidGid;
    content->setXpmRegion = SetXpmRegion;
    content->setCapabilities = SetCapabilities;
    content->setFileDescriptors = SetFileDescriptors;
    content->setAppSandbox = SetAppSandboxProperty;
    content->setAppAccessToken = SetAppAccessToken;
    content->coldStartApp = ColdStartApp;
    content->setAsanEnabledEnv = SetAsanEnabledEnv;
#ifdef ASAN_DETECTOR
    content->getWrapBundleNameValue = GetWrapBundleNameValue;
#endif
    content->setSeccompFilter = SetSeccompFilter;
    content->setUidGidFilter = SetUidGidFilter;
    content->handleInternetPermission = HandleInternetPermission;
    content->waitForDebugger = WaitForDebugger;
}
