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
#include "appspawn_hook.h"
#include "appspawn_msg.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "parameter.h"
#include "securec.h"

// for stub
extern bool may_init_gwp_asan(bool forceInit);

// ide-asan
static int SetAsanEnabledEnv(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    const char *bundleName = GetBundleName(property);
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_ASANENABLED)) {
        char *devPath = "/dev/asanlog";
        char logPath[PATH_MAX] = {0};
        int ret = snprintf_s(logPath, sizeof(logPath), sizeof(logPath) - 1,
                "/data/app/el1/100/base/%s/log", bundleName);
        APPSPAWN_CHECK(ret > 0, return -1, "Invalid snprintf_s");
        char asanOptions[PATH_MAX] = {0};
        ret = snprintf_s(asanOptions, sizeof(asanOptions), sizeof(asanOptions) - 1,
                "log_path=%s/asan.log:include=/system/etc/asan.options", devPath);
        APPSPAWN_CHECK(ret > 0, return -1, "Invalid snprintf_s");

#if defined(__aarch64__) || defined(__x86_64__)
        setenv("LD_PRELOAD", "/system/lib64/libclang_rt.asan.so", 1);
#else
        setenv("LD_PRELOAD", "/system/lib/libclang_rt.asan.so", 1);
#endif
        unsetenv("UBSAN_OPTIONS");
        setenv("ASAN_OPTIONS", asanOptions, 1);
        return 0;
    }
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_TSAN_ENABLED)) {
        setenv("LD_PRELOAD", "/system/lib64/libclang_rt.tsan.so", 1);
        unsetenv("UBSAN_OPTIONS");
        setenv("TSAN_OPTIONS", "include=/system/etc/tsan.options", 1);
        return 0;
    }
    return -1;
}

static void SetGwpAsanEnabled(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (!(CheckAppMsgFlagsSet(property, APP_FLAGS_GWP_ENABLED_FORCE) ||
        CheckAppMsgFlagsSet(property, APP_FLAGS_GWP_ENABLED_NORMAL))) {
        return;
    }
    if (IsDeveloperModeOn(property)) {
        APPSPAWN_LOGV("SetGwpAsanEnabled with flags: %{public}d",
            CheckAppMsgFlagsSet(property, APP_FLAGS_GWP_ENABLED_FORCE));
        may_init_gwp_asan(CheckAppMsgFlagsSet(property, APP_FLAGS_GWP_ENABLED_FORCE));
    }
}

#ifdef ASAN_DETECTOR
#define WRAP_VALUE_MAX_LENGTH 96
static int CheckSupportColdStart(const char *bundleName)
{
    char wrapBundleNameKey[WRAP_VALUE_MAX_LENGTH] = {0};
    char wrapBundleNameValue[WRAP_VALUE_MAX_LENGTH] = {0};

    int len = sprintf_s(wrapBundleNameKey, WRAP_VALUE_MAX_LENGTH, "wrap.%s", bundleName);
    APPSPAWN_CHECK(len > 0 && (len < WRAP_VALUE_MAX_LENGTH), return -1, "Invalid to format wrapBundleNameKey");

    int ret = GetParameter(wrapBundleNameKey, "", wrapBundleNameValue, WRAP_VALUE_MAX_LENGTH);
    APPSPAWN_CHECK(ret > 0 && (!strcmp(wrapBundleNameValue, "asan_wrapper")), return -1,
        "Not wrap %{public}s.", bundleName);
    APPSPAWN_LOGI("Asan: GetParameter %{public}s the value is %{public}s.", wrapBundleNameKey, wrapBundleNameValue);
    return 0;
}
#endif

static int AsanSpawnGetSpawningFlag(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_LOGV("Prepare spawn app %{public}s", GetProcessName(property));
#ifdef ASAN_DETECTOR
    if (CheckSupportColdStart(GetBundleName(property)) == 0) {
        property->client.flags |= APP_COLD_START;
        property->client.flags |= APP_ASAN_DETECTOR;
        if (property->forkCtx.coldRunPath) {
            free(property->forkCtx.coldRunPath);
        }
        property->forkCtx.coldRunPath = strdup("/system/asan/bin/appspawn");
        if (property->forkCtx.coldRunPath == NULL) {
            APPSPAWN_LOGE("Failed to set asan exec path %{public}s", GetProcessName(property));
        }
    }
#endif
    return 0;
}

static int AsanSpawnInitSpawningEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    if (GetAppSpawnMsgType(property) == MSG_SPAWN_NATIVE_PROCESS) {
        return 0;
    }
    int ret = SetAsanEnabledEnv(content, property);
    if (ret == 0) {
        APPSPAWN_LOGI("SetAsanEnabledEnv cold start app %{public}s", GetProcessName(property));
        property->client.flags |= APP_COLD_START;
    }
    (void)SetGwpAsanEnabled(content, property);
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load asan module ...");
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, HOOK_PRIO_COMMON, AsanSpawnInitSpawningEnv);
    AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_COMMON, AsanSpawnGetSpawningFlag);
}
