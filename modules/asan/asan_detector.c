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
#ifndef ASAN_DETECTOR

#if defined(__aarch64__) || defined(__x86_64__)
#define ASAN_LD_PRELOAD "/system/lib64/libclang_rt.asan.so"
#else
#define ASAN_LD_PRELOAD "/system/lib/libclang_rt.asan.so"
#endif
#define HWASAN_LD_PRELOAD "/system/lib64/libclang_rt.hwasan.so"
#define TSAN_LD_PRELOAD "/system/lib64/libclang_rt.tsan.so"

#define ASAN_OPTIONS "include=/system/etc/asan.options"
#define HWASAN_OPTIONS "include=/system/etc/asan.options"
#define TSAN_OPTIONS "include=/system/etc/tsan.options"
#define UBSAN_OPTIONS "print_stacktrace=1:print_module_map=2:log_exe_name=1"

// 配置表数据
static const EnvConfig g_configTable[] = {
    { APP_FLAGS_HWASAN_ENABLED, HWASAN_LD_PRELOAD, NULL, NULL, NULL, HWASAN_OPTIONS },
    { APP_FLAGS_ASANENABLED, ASAN_LD_PRELOAD, ASAN_OPTIONS, NULL, NULL, NULL },
    { APP_FLAGS_TSAN_ENABLED, TSAN_LD_PRELOAD, NULL, TSAN_OPTIONS, NULL, NULL },
    { APP_FLAGS_UBSAN_ENABLED, NULL, NULL, NULL, UBSAN_OPTIONS, NULL },
};

bool isInRenderProcess(const AppSpawningCtx *property)
{
    static bool isRender = false;
    static bool firstIn = true;
    if (!firstIn) {
        return isRender;
    }
    firstIn = false;
    char *processType = (char *)GetAppPropertyExt(property, "ProcessType", NULL);
    if (!processType) {
        APPSPAWN_LOGE("GetAppPropertyExt ProcessType is null");
        return false;
    }
    if (strcmp(processType, "render") == 0 || strcmp(processType, "gpu") == 0) {
        isRender = true;
    }
    return isRender;
}

static int SetAsanEnabledEnv(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    size_t configTableSize = sizeof(g_configTable) / sizeof(g_configTable[0]);
    for (size_t i = 0; i < configTableSize; ++i) {
        if (CheckAppMsgFlagsSet(property, g_configTable[i].flag)) {
            if (g_configTable[i].flag == APP_FLAGS_TSAN_ENABLED && isInRenderProcess(property)) {
                APPSPAWN_LOGI("SetAsanEnabledEnv skip tsan setting if render process");
                continue;
            }
            if (g_configTable[i].ldPreload) {
                setenv("LD_PRELOAD", g_configTable[i].ldPreload, 1);
            }
            if (g_configTable[i].asanOptions) {
                setenv("ASAN_OPTIONS", g_configTable[i].asanOptions, 1);
            } else {
                unsetenv("ASAN_OPTIONS");
            }
            if (g_configTable[i].tsanOptions) {
                setenv("TSAN_OPTIONS", g_configTable[i].tsanOptions, 1);
            } else {
                unsetenv("TSAN_OPTIONS");
            }
            if (g_configTable[i].ubsanOptions) {
                setenv("UBSAN_OPTIONS", g_configTable[i].ubsanOptions, 1);
            } else {
                unsetenv("UBSAN_OPTIONS");
            }
            if (g_configTable[i].hwasanOptions) {
                setenv("HWASAN_OPTIONS", g_configTable[i].hwasanOptions, 1);
            } else {
                unsetenv("HWASAN_OPTIONS");
            }
            APPSPAWN_LOGV("SetAsanEnabledEnv %{public}d,%{public}s,%{public}s,%{public}s,%{public}s,%{public}s",
                g_configTable[i].flag, g_configTable[i].ldPreload, g_configTable[i].asanOptions,
                g_configTable[i].tsanOptions, g_configTable[i].ubsanOptions, g_configTable[i].hwasanOptions);
            return 0;
        }
    }
    return -1;
}
#endif

static void SetGwpAsanEnabled(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    int enforce = CheckAppMsgFlagsSet(property, APP_FLAGS_GWP_ENABLED_FORCE);
    APPSPAWN_LOGV("SetGwpAsanEnabled with flags: %{public}d", enforce);
    may_init_gwp_asan(enforce);
}

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
#ifndef CJAPP_SPAWN
        property->forkCtx.coldRunPath = strdup("/system/asan/bin/appspawn");
#elif NATIVE_SPAWN
        property->forkCtx.coldRunPath = strdup("/system/asan/bin/nativespawn");
#else
        property->forkCtx.coldRunPath = strdup("/system/asan/bin/cjappspawn");
#endif
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
#ifndef ASAN_DETECTOR
    if (CheckSupportColdStart(GetBundleName(property)) == 0) {
        setenv("LD_PRELOAD", "/system/lib64/libclang_rt.hwasan.so", 1);
        setenv("HWASAN_OPTIONS", "include=/system/etc/asan.options", 1);
        property->client.flags |= APP_COLD_START;
    }
    int ret = SetAsanEnabledEnv(content, property);
    if (ret == 0) {
        APPSPAWN_LOGI("SetAsanEnabledEnv cold start app %{public}s", GetProcessName(property));
        property->client.flags |= APP_COLD_START;
    }
#endif
    (void)SetGwpAsanEnabled(content, property);
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGV("Load asan module ...");
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, HOOK_PRIO_COMMON, AsanSpawnInitSpawningEnv);
    AddAppSpawnHook(STAGE_PARENT_PRE_FORK, HOOK_PRIO_COMMON, AsanSpawnGetSpawningFlag);
}
