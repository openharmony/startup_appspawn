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
#include <fcntl.h>
#include <grp.h>
#include <inttypes.h>
#include <limits.h>
#include <sys/capability.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#undef _GNU_SOURCE
#define _GNU_SOURCE
#include <dlfcn.h>
#include <malloc.h>
#include <sched.h>

#include "appspawn_adapter.h"
#include "appspawn_hook.h"
#include "appspawn_msg.h"
#include "appspawn_manager.h"
#include "appspawn_silk.h"
#include "init_param.h"
#include "parameter.h"
#include "securec.h"

#ifdef CODE_SIGNATURE_ENABLE  // for xpm
#include "code_sign_attr_utils.h"
#endif
#ifdef SECURITY_COMPONENT_ENABLE
#include "sec_comp_enhance_kit_c.h"
#endif
#ifdef WITH_SELINUX
#include "selinux/selinux.h"
#endif

#define DEVICE_NULL_STR "/dev/null"
#define BITLEN32 32
#define PID_NS_INIT_UID 100000  // reserved for pid_ns_init process, avoid app, render proc, etc.
#define PID_NS_INIT_GID 100000

static int SetProcessName(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    const char *processName = GetProcessName(property);
    APPSPAWN_CHECK(processName != NULL, return -EINVAL, "Can not get process name");
    // 解析时已经检查
    size_t len = strlen(processName);
    char shortName[MAX_LEN_SHORT_NAME] = {0};
    // process short name max length 16 bytes.
    size_t copyLen = len;
    const char *pos = processName;
    if (len >= MAX_LEN_SHORT_NAME) {
        copyLen = MAX_LEN_SHORT_NAME - 1;
        pos += (len - copyLen);
    }
    bool isRet = strncpy_s(shortName, MAX_LEN_SHORT_NAME, pos, copyLen) != EOK;
    APPSPAWN_CHECK(!isRet, return EINVAL, "strncpy_s short name error: %{public}d", errno);

    // set short name
    isRet = prctl(PR_SET_NAME, shortName) == -1;
    APPSPAWN_CHECK(!isRet, return errno, "prctl(PR_SET_NAME) error: %{public}d", errno);

    // reset longProcName
    isRet = memset_s(content->content.longProcName,
        (size_t)content->content.longProcNameLen, 0, (size_t)content->content.longProcNameLen) != EOK;
    APPSPAWN_CHECK(!isRet, return EINVAL, "Failed to memset long process name");

    // set long process name
    isRet = strncpy_s(content->content.longProcName, content->content.longProcNameLen, processName, len) != EOK;
    APPSPAWN_CHECK(!isRet, return EINVAL,
        "strncpy_s long name error: %{public}d longProcNameLen %{public}u", errno, content->content.longProcNameLen);
    return 0;
}

static int SetKeepCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DOMAIN_INFO, GetProcessName(property));

    // set keep capabilities when user not root.
    if (dacInfo->uid != 0) {
        bool isRet = prctl(PR_SET_KEEPCAPS, 1, 0, 0, 0) == -1;
        APPSPAWN_CHECK(!isRet, return errno, "set keepcaps failed: %{public}d", errno);
    }
    return 0;
}

static int SetCapabilities(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    // init cap
    struct __user_cap_header_struct capHeader;

    bool isRet = memset_s(&capHeader, sizeof(capHeader), 0, sizeof(capHeader)) != EOK;
    APPSPAWN_CHECK(!isRet, return -EINVAL, "Failed to memset cap header");

    capHeader.version = _LINUX_CAPABILITY_VERSION_3;
    capHeader.pid = 0;

    struct __user_cap_data_struct capData[2]; // 2 is data number
    isRet = memset_s(&capData, sizeof(capData), 0, sizeof(capData)) != EOK;
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

    capData[0].inheritable = (__u32)(inheriTable);
    capData[1].inheritable = (__u32)(inheriTable >> BITLEN32);
    capData[0].permitted = (__u32)(permitted);
    capData[1].permitted = (__u32)(permitted >> BITLEN32);
    capData[0].effective = (__u32)(effective);
    capData[1].effective = (__u32)(effective >> BITLEN32);

    // set capabilities
    isRet = capset(&capHeader, &capData[0]) != 0;
    APPSPAWN_CHECK(!isRet, return -errno, "Failed to capset errno: %{public}d", errno);
    return 0;
}

static void InitDebugParams(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
#if defined(__aarch64__) || defined(__x86_64__)
    const char *debugSoPath = "/system/lib64/libhidebug.so";
#else
    const char *debugSoPath = "/system/lib/libhidebug.so";
#endif
    const char *processName = GetProcessName(property);
    APPSPAWN_CHECK(processName != NULL, return, "Can not get process name ");

    bool isRet = access(debugSoPath, F_OK) != 0;
    APPSPAWN_CHECK(!isRet, return,
        "access failed, errno: %{public}d debugSoPath: %{public}s", errno, debugSoPath);

    void *handle = dlopen(debugSoPath, RTLD_LAZY);
    APPSPAWN_CHECK(handle != NULL, return, "Failed to dlopen libhidebug.so errno: %{public}s", dlerror());

    bool (*initParam)(const char *name);
    initParam = (bool (*)(const char *name))dlsym(handle, "InitEnvironmentParam");
    APPSPAWN_CHECK(initParam != NULL, dlclose(handle);
        return, "Failed to dlsym errno: %{public}s", dlerror());
    (*initParam)(processName);
    dlclose(handle);
}

static void ClearEnvironment(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // close child fd
    AppSpawningCtx *ctx = (AppSpawningCtx *)property;
    close(ctx->forkCtx.fd[0]);
    ctx->forkCtx.fd[0] = -1;
    return;
}

static int SetXpmConfig(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
#ifdef CODE_SIGNATURE_ENABLE
    // nwebspawn no permission set xpm config
    if (IsNWebSpawnMode(content)) {
        return 0;
    }
    AppSpawnMsgOwnerId *ownerInfo = (AppSpawnMsgOwnerId *)GetAppProperty(property, TLV_OWNER_INFO);
    int ret = InitXpmRegion();
    APPSPAWN_CHECK(ret == 0, return ret, "init xpm region failed: %{public}d", ret);

    if (CheckAppMsgFlagsSet(property, APP_FLAGS_DEBUGGABLE)) {
        ret = SetXpmOwnerId(PROCESS_OWNERID_DEBUG, NULL);
    } else if (ownerInfo == NULL) {
        ret = SetXpmOwnerId(PROCESS_OWNERID_COMPAT, NULL);
    } else {
        ret = SetXpmOwnerId(PROCESS_OWNERID_APP, ownerInfo->ownerId);
    }
    APPSPAWN_CHECK(ret == 0, return ret, "set xpm region failed: %{public}d", ret);
#endif
    return 0;
}

static int SetUidGid(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppProperty(property, TLV_DAC_INFO);
    APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_TLV_NONE,
        "No tlv %{public}d in msg %{public}s", TLV_DAC_INFO, GetProcessName(property));

    // set gids
    int ret = setgroups(dacInfo->gidCount, (const gid_t *)(&dacInfo->gidTable[0]));
    APPSPAWN_CHECK(ret == 0, return errno,
        "setgroups failed: %{public}d, gids.size=%{public}u", errno, dacInfo->gidCount);

    // set gid
    if (IsNWebSpawnMode(content)) {
        gid_t gid = dacInfo->gid / UID_BASE;
        if (gid >= 100) { // 100
            APPSPAWN_LOGE("SetUidGid invalid uid for nwebspawn %{public}d", dacInfo->gid);
            return 0;
        }
        ret = setresgid(dacInfo->gid, dacInfo->gid, dacInfo->gid);
    } else {
        ret = setresgid(dacInfo->gid, dacInfo->gid, dacInfo->gid);
    }
    APPSPAWN_CHECK(ret == 0, return errno,
        "setgid(%{public}u) failed: %{public}d", dacInfo->gid, errno);

    ret = SetSeccompFilter(content, property);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to set setSeccompFilter");

    /* If the effective user ID is changed from 0 to nonzero,
     * then all capabilities are cleared from the effective set
     */
    ret = setresuid(dacInfo->uid, dacInfo->uid, dacInfo->uid);
    APPSPAWN_CHECK(ret == 0, return errno,
        "setuid(%{public}u) failed: %{public}d", dacInfo->uid, errno);

    if (CheckAppMsgFlagsSet(property, APP_FLAGS_DEBUGGABLE) && IsDeveloperModeOn(property)) {
        setenv("HAP_DEBUGGABLE", "true", 1);
        if (prctl(PR_SET_DUMPABLE, 1, 0, 0, 0) == -1) {
            APPSPAWN_LOGE("Failed to set app dumpable: %{public}s", strerror(errno));
        }
    }
    return 0;
}

static int32_t SetFileDescriptors(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
#ifndef APPSPAWN_TEST
    // redirect stdin stdout stderr to /dev/null
    int devNullFd = open(DEVICE_NULL_STR, O_RDWR);
    if (devNullFd == -1) {
        APPSPAWN_LOGE("open dev_null error: %{public}d", errno);
        return (-errno);
    }

    // stdin
    if (dup2(devNullFd, STDIN_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDIN error: %{public}d", errno);
        return (-errno);
    }
    // stdout
    if (dup2(devNullFd, STDOUT_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDOUT error: %{public}d", errno);
        return (-errno);
    }
    // stderr
    if (dup2(devNullFd, STDERR_FILENO) == -1) {
        APPSPAWN_LOGE("dup2 STDERR error: %{public}d", errno);
        return (-errno);
    }

    if (devNullFd > STDERR_FILENO) {
        close(devNullFd);
    }
#endif
    return 0;
}

static int32_t CheckTraceStatus(void)
{
    int fd = open("/proc/self/status", O_RDONLY);
    APPSPAWN_CHECK(fd >= 0, return errno, "Failed to open /proc/self/status error: %{public}d", errno);

    char data[1024] = {0};  // 1024 is data length
    ssize_t dataNum = read(fd, data, sizeof(data));
    (void)close(fd);
    APPSPAWN_CHECK(dataNum > 0, return -1, "Failed to read file /proc/self/status error: %{public}d", errno);

    const char *tracerPid = "TracerPid:\t";
    char *traceStr = strstr(data, tracerPid);
    APPSPAWN_CHECK(traceStr != NULL, return -1, "Not found %{public}s", tracerPid);

    char *separator = strchr(traceStr, '\n');
    APPSPAWN_CHECK(separator != NULL, return -1, "Not found %{public}s", "\n");

    int len = separator - traceStr - strlen(tracerPid);
    char pid = *(traceStr + strlen(tracerPid));
    if (len > 1 || pid != '0') {
        return 0;
    }
    return -1;
}

static int32_t WaitForDebugger(const AppSpawningCtx *property)
{
    // wait for debugger only debugging is required and process is debuggable
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_NATIVEDEBUG) && CheckAppMsgFlagsSet(property, APP_FLAGS_DEBUGGABLE)) {
        uint32_t count = 0;
        while (CheckTraceStatus() != 0) {
#ifndef APPSPAWN_TEST
            usleep(1000 * 100);  // sleep 1000 * 100 microsecond
#else
            if (count > 0) {
                break;
            }
#endif
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

static int SpawnInitSpawningEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_LOGV("Spawning: clear env");
    int ret = SetProcessName(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    // close socket id and signal for child
    ClearEnvironment(content, property);

    ResetParamSecurityLabel();

    ret = SetAppAccessToken(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    return 0;
}

static int SpawnSetAppEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_LOGV("Spawning: set appEnv");
    int ret = SetEnvInfo(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    return 0;
}

static int SpawnEnableCache(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_LOGV("Spawning: enable cache for app process");
    // enable cache for app process
    mallopt(M_OHOS_CONFIG, M_TCACHE_PERFORMANCE_MODE);
    mallopt(M_OHOS_CONFIG, M_ENABLE_OPT_TCACHE);
    mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_ENABLE);
    mallopt(M_DELAYED_FREE, M_DELAYED_FREE_ENABLE);

    int ret = SetInternetPermission(property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    return ret;
}

static void SpawnLoadSilk(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    const char *processName = GetBundleName(property);
    APPSPAWN_CHECK(processName != NULL, return, "Can not get bundle name");
    LoadSilkLibrary(processName);
}

static int SpawnSetProperties(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_LOGV("Spawning: set child property");
    SpawnLoadSilk(content, property);
    (void)umask(DEFAULT_UMASK);
    int ret = SetKeepCapabilities(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    ret = SetXpmConfig(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = SetProcessName(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = SetUidGid(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = SetFileDescriptors(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = SetCapabilities(content, property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = SetSelinuxCon(content, property) == -1;
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    ret = WaitForDebugger(property);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    APPSPAWN_ONLY_EXPER(GetAppSpawnMsgType(property) == MSG_SPAWN_NATIVE_PROCESS, return 0);
#ifdef SECURITY_COMPONENT_ENABLE
    InitSecCompClientEnhance();
#endif
    return 0;
}

static int PreLoadSetSeccompFilter(AppSpawnMgr *content)
{
    // set uid gid filetr
    int ret = SetUidGidFilter(content);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    return ret;
}

static int SpawnComplete(AppSpawnMgr *content, AppSpawningCtx *property)
{
    InitDebugParams(content, property);
    return 0;
}

static int CheckEnabled(const char *param, const char *value)
{
    char tmp[32] = {0};  // 32 max
    int ret = GetParameter(param, "", tmp, sizeof(tmp));
    APPSPAWN_LOGV("CheckEnabled key %{public}s ret %{public}d result: %{public}s", param, ret, tmp);
    int enabled = (ret > 0 && strcmp(tmp, value) == 0);
    return enabled;
}

static int SpawnGetSpawningFlag(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_LOGV("Spawning: prepare app %{public}s", GetProcessName(property));
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_COLD_BOOT)) {
        // check cold start
        property->client.flags |= CheckEnabled("startup.appspawn.cold.boot", "true") ? APP_COLD_START : 0;
    }
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_BEGETCTL_BOOT)) {
       // Start app from begetctl for debugging.
        property->client.flags |=  APP_BEGETCTL_BOOT;
        APPSPAWN_LOGI("Spawning: prepare app %{public}s, start from begetctl", GetProcessName(property));
     }
    // check developer mode
    property->client.flags |= CheckEnabled("const.security.developermode.state", "true") ? APP_DEVELOPER_MODE : 0;
    return 0;
}

static int SpawnLoadConfig(AppSpawnMgr *content)
{
    LoadSilkConfig();
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load common module ...");
    AddPreloadHook(HOOK_PRIO_COMMON, PreLoadSetSeccompFilter);
    AddPreloadHook(HOOK_PRIO_COMMON, SpawnLoadConfig);

    AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_HIGHEST, SpawnGetSpawningFlag);
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, HOOK_PRIO_HIGHEST, SpawnInitSpawningEnv);
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, HOOK_PRIO_COMMON + 1, SpawnSetAppEnv);
    AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_HIGHEST, SpawnEnableCache);
    AddAppSpawnHook(STAGE_CHILD_EXECUTE, HOOK_PRIO_PROPERTY, SpawnSetProperties);
    AddAppSpawnHook(STAGE_CHILD_POST_RELY, HOOK_PRIO_HIGHEST, SpawnComplete);
}
