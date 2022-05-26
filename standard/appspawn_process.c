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

#include "appspawn_adapter.h"
#include "securec.h"


#define DEVICE_NULL_STR "/dev/null"

static int SetProcessName(struct AppSpawnContent_ *content, AppSpawnClient *client,
    char *longProcName, uint32_t longProcNameLen)
{
    AppSpawnClientExt *appPropertyExt = (AppSpawnClientExt *)client;
    AppParameter *appProperty = &appPropertyExt->property;
    int len = strlen(appProperty->processName);
    if (longProcName == NULL || len <= 0) {
        APPSPAWN_LOGE("process name is nullptr or length error");
        return -EINVAL;
    }

    char shortName[MAX_LEN_SHORT_NAME] = {0};
    // process short name max length 16 bytes.
    if (len >= MAX_LEN_SHORT_NAME) {
        if (strncpy_s(shortName, MAX_LEN_SHORT_NAME, appProperty->processName, MAX_LEN_SHORT_NAME - 1) != EOK) {
            APPSPAWN_LOGE("strncpy_s short name error: %d", errno);
            return -EINVAL;
        }
    } else {
        if (strncpy_s(shortName, MAX_LEN_SHORT_NAME, appProperty->processName, len) != EOK) {
            APPSPAWN_LOGE("strncpy_s short name error: %d", errno);
            return -EINVAL;
        }
    }

    // set short name
    if (prctl(PR_SET_NAME, shortName) == -1) {
        APPSPAWN_LOGE("prctl(PR_SET_NAME) error: %d", errno);
        return (-errno);
    }

    // reset longProcName
    if (memset_s(longProcName, (size_t)longProcNameLen, 0, (size_t)longProcNameLen) != EOK) {
        APPSPAWN_LOGE("Failed to memset long process name");
        return -EINVAL;
    }

    // set long process name
    if (strncpy_s(longProcName, sizeof(appProperty->processName) - 1, appProperty->processName, len) != EOK) {
        APPSPAWN_LOGE("strncpy_s long name error: %d longProcNameLen %u", errno, longProcNameLen);
        return -EINVAL;
    }
    return 0;
}

static int SetKeepCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    // set keep capabilities when user not root.
    if (appProperty->property.uid != 0) {
        if (prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == -1) {
            APPSPAWN_LOGE("set keepcaps failed: %d", errno);
            return (-errno);
        }
    }
    return 0;
}

static int SetCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    // init cap
    struct __user_cap_header_struct cap_header;

    if (memset_s(&cap_header, sizeof(cap_header), 0, sizeof(cap_header)) != EOK) {
        APPSPAWN_LOGE("Failed to memset cap header");
        return -EINVAL;
    }
    cap_header.version = _LINUX_CAPABILITY_VERSION_3;
    cap_header.pid = 0;

    struct __user_cap_data_struct cap_data[2];
    if (memset_s(&cap_data, sizeof(cap_data), 0, sizeof(cap_data)) != EOK) {
        APPSPAWN_LOGE("Failed to memset cap data");
        return -EINVAL;
    }

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
    if (capset(&cap_header, &cap_data[0]) == -1) {
        APPSPAWN_LOGE("capset failed: %d", errno);
        return (-errno);
    }
    return 0;
}

static void InitDebugParams(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
#ifdef __aarch64__
    const char *debugSoPath = "/system/lib64/libhidebug.so";
#else
    const char *debugSoPath = "/system/lib/libhidebug.so";
#endif
    if (access(debugSoPath, F_OK) != 0) {
        return;
    }
    void *handle = dlopen(debugSoPath, RTLD_LAZY);
    if (handle == NULL) {
        APPSPAWN_LOGE("Failed to dlopen libhidebug.so, %s", dlerror());
        return;
    }
    bool (*initParam)(const char *name);
    initParam = (bool (*)(const char *name))dlsym(handle, "InitEnvironmentParam");
    if (initParam == NULL) {
        APPSPAWN_LOGE("Failed to dlsym InitEnvironmentParam, %s", dlerror());
        dlclose(handle);
        return;
    }
    (*initParam)(appProperty->property.processName);
    dlclose(handle);
}

static void ClearEnvironment(AppSpawnContent *content, AppSpawnClient *client)
{
    APPSPAWN_LOGI("ClearEnvironment id %d", client->id);
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // close child fd
    close(appProperty->fd[0]);
    InitDebugParams(content, client);
    return;
}

static int SetUidGid(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
#ifdef GRAPHIC_PERMISSION_CHECK
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    // set gids
    if (setgroups(appProperty->property.gidCount, (const gid_t *)(&appProperty->property.gidTable[0])) == -1) {
        APPSPAWN_LOGE("setgroups failed: %d, gids.size=%u", errno, appProperty->property.gidCount);
        return (-errno);
    }

    // set gid
    if (setresgid(appProperty->property.gid, appProperty->property.gid, appProperty->property.gid) == -1) {
        APPSPAWN_LOGE("setgid(%u) failed: %d", appProperty->property.gid, errno);
        return (-errno);
    }

    // If the effective user ID is changed from 0 to nonzero, then all capabilities are cleared from the effective set
    if (setresuid(appProperty->property.uid, appProperty->property.uid, appProperty->property.uid) == -1) {
        APPSPAWN_LOGE("setuid(%u) failed: %d", appProperty->property.uid, errno);
        return (-errno);
    }
#endif
    return 0;
}

static int32_t SetFileDescriptors(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
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
        ret = strcpy_s(argv[0], APP_LEN_PROC_NAME, "/system/bin/appspawn");
        APPSPAWN_CHECK(ret >= 0, break, "Invalid strdup");
        argv[START_INDEX] = strdup("cold-start");
        APPSPAWN_CHECK(argv[START_INDEX] != NULL, break, "Invalid strdup");
        argv[FD_INDEX] = strdup(buffer);
        APPSPAWN_CHECK(argv[FD_INDEX] != NULL, break, "Invalid strdup");

        len = sprintf_s(param + startLen, originLen - startLen, "%u:%u:%u:%d",
            ((AppSpawnClientExt *)client)->client.id,
            appProperty->uid, appProperty->gid, appProperty->gidCount);
        APPSPAWN_CHECK(len > 0 && (len < (originLen - startLen)), break, "Invalid to format");
        startLen += len;
        for (uint32_t i = 0; i < appProperty->gidCount; i++) {
            len = sprintf_s(param + startLen, originLen - startLen, ":%u", appProperty->gidTable[i]);
            APPSPAWN_CHECK(len > 0 && (len < (originLen - startLen)), break, "Invalid to format gid");
            startLen += len;
        }
        // processName
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
    APPSPAWN_CHECK(argv != NULL, return -1, "Invalid arg");
    APPSPAWN_CHECK(argc > PARAM_INDEX, return -1, "Invalid argc %d", argc);

    client->fd[1] = atoi(argv[FD_INDEX]);
    APPSPAWN_LOGV("GetAppSpawnClientFromArg %s ", argv[PARAM_INDEX]);
    char *end = NULL;
    char *start = strtok_r(argv[PARAM_INDEX], ":", &end);
    // clientid
    APPSPAWN_CHECK(start != NULL, return -1, "Failed to get client id");
    client->client.id = atoi(start);
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
    ret = strcpy_s(client->property.soPath, sizeof(client->property.soPath), start);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy soPath");
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
    content->setProcessName = SetProcessName;
    content->setKeepCapabilities = SetKeepCapabilities;
    content->setUidGid = SetUidGid;
    content->setCapabilities = SetCapabilities;
    content->setFileDescriptors = SetFileDescriptors;
    content->setAppSandbox = SetAppSandboxProperty;
    content->setAppAccessToken = SetAppAccessToken;
    content->coldStartApp = ColdStartApp;
}
