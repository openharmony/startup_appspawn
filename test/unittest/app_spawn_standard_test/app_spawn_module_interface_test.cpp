/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "cJSON.h"
#include "json_utils.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnModuleInterfaceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief module interface
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AppSpawnModuleMgrInstall_001, TestSize.Level1)
{
    int ret = AppSpawnModuleMgrInstall(nullptr);
    EXPECT_NE(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_MAX);
    EXPECT_NE(0, ret);

    ret = AppSpawnLoadAutoRunModules(MODULE_MAX);
    EXPECT_NE(0, ret);
    ret = AppSpawnModuleMgrInstall("/");
    EXPECT_EQ(0, ret);
    ret = AppSpawnModuleMgrInstall("");
    EXPECT_EQ(0, ret);
    ret = AppSpawnModuleMgrInstall("libappspawn_asan");
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AppSpawnLoadAutoRunModules_001, TestSize.Level1)
{
    int ret = AppSpawnLoadAutoRunModules(MODULE_DEFAULT);
    EXPECT_EQ(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_APPSPAWN);
    EXPECT_EQ(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_NWEBSPAWN);
    EXPECT_EQ(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_COMMON);
    EXPECT_EQ(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_MAX);
    EXPECT_NE(0, ret);
    ret = AppSpawnLoadAutoRunModules(100);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AppSpawnModuleMgrUnInstall_001, TestSize.Level1)
{
    AppSpawnModuleMgrUnInstall(MODULE_DEFAULT);
    AppSpawnModuleMgrUnInstall(MODULE_APPSPAWN);
    AppSpawnModuleMgrUnInstall(MODULE_NWEBSPAWN);
    AppSpawnModuleMgrUnInstall(MODULE_COMMON);
    AppSpawnModuleMgrUnInstall(MODULE_MAX);
    AppSpawnModuleMgrUnInstall(100);
}

/**
 * @brief hook interface
 *
 */
static int TestAppPreload(AppSpawnMgr *content)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Server_Hook_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    int ret = 0;
    for (int i = 0; i < STAGE_MAX; i++) {
        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(mgr);

        for (int k = 0; k <= HOOK_PRIO_LOWEST + 1000; k += 1000) { // 1000
            ret = AddServerStageHook(static_cast<AppSpawnHookStage>(i), k, TestAppPreload);
            EXPECT_EQ(ret == 0, (i >= STAGE_SERVER_PRELOAD) && (i <= STAGE_SERVER_EXIT));
        }
        ret = AddServerStageHook(static_cast<AppSpawnHookStage>(i), 0, nullptr);
        EXPECT_EQ(ret == 0, 0);

        ret = ServerStageHookExecute(static_cast<AppSpawnHookStage>(i), content);
        EXPECT_EQ(ret == 0, (i >= STAGE_SERVER_PRELOAD) && (i <= STAGE_SERVER_EXIT));

        ret = ServerStageHookExecute(static_cast<AppSpawnHookStage>(i), nullptr);
        EXPECT_NE(ret, 0);
    }
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AddAppSpawnHook
 *
 */
static int TestAppSpawn(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_App_Spawn_Hook_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);

    int ret = 0;
    for (int i = 0; i < STAGE_MAX; i++) {
        AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(appCtx);
        AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(mgr);

        for (int k = 0; k <= HOOK_PRIO_LOWEST + 1000; k += 1000) { // 1000
            ret = AddAppSpawnHook(static_cast<AppSpawnHookStage>(i), k, TestAppSpawn);
            EXPECT_EQ(ret == 0, (i >= STAGE_PARENT_PRE_FORK) && (i <= STAGE_CHILD_PRE_RUN));
        }
        ret = AddAppSpawnHook(static_cast<AppSpawnHookStage>(i), 0, nullptr);
        EXPECT_EQ(ret == 0, 0);

        if (i == STAGE_CHILD_PRE_RUN || i == STAGE_CHILD_EXECUTE) {
            printf("App_Spawn_App_Spawn_Hook_001 %d \n", i);
            continue;
        }
        ret = AppSpawnHookExecute(static_cast<AppSpawnHookStage>(i), 0, content, client);
        EXPECT_EQ(ret == 0, (i >= STAGE_PARENT_PRE_FORK) && (i <= STAGE_CHILD_PRE_RUN));

        ret = AppSpawnHookExecute(static_cast<AppSpawnHookStage>(i), 0, nullptr, client);
        EXPECT_NE(ret, 0);

        ret = AppSpawnHookExecute(static_cast<AppSpawnHookStage>(i), 0, nullptr, nullptr);
        EXPECT_NE(ret, 0);

        ret = AppSpawnHookExecute(static_cast<AppSpawnHookStage>(i), 0, content, nullptr);
        EXPECT_NE(ret, 0);
    }
    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AddProcessMgrHook
 *
 */
static int ReportProcessExitInfo(const AppSpawnMgr *content, const AppSpawnedProcess *appInfo)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Process_Hook_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    AppSpawnedProcess *app = AddSpawnedProcess(1000, "test-001");
    EXPECT_EQ(app != nullptr, 1);

    int ret = 0;
    for (int i = 0; i < STAGE_MAX; i++) {
        for (int k = 0; k <= HOOK_PRIO_LOWEST + 1000; k += 1000) { // 1000
            ret = AddProcessMgrHook(static_cast<AppSpawnHookStage>(i), k, ReportProcessExitInfo);
            EXPECT_EQ(ret == 0, (i == STAGE_SERVER_APP_ADD || i == STAGE_SERVER_APP_DIED));
        }

        ret = AddProcessMgrHook(static_cast<AppSpawnHookStage>(i), 0, nullptr);
        EXPECT_EQ(ret == 0, 0);

        ret = ProcessMgrHookExecute(static_cast<AppSpawnHookStage>(i), reinterpret_cast<AppSpawnContent *>(mgr), app);
        EXPECT_EQ(ret == 0, (i == STAGE_SERVER_APP_ADD || i == STAGE_SERVER_APP_DIED));

        ret = ProcessMgrHookExecute(static_cast<AppSpawnHookStage>(i), nullptr, app);
        EXPECT_NE(ret, 0);

        ret = ProcessMgrHookExecute(static_cast<AppSpawnHookStage>(i), nullptr, nullptr);
        EXPECT_NE(ret, 0);

        ret = ProcessMgrHookExecute(static_cast<AppSpawnHookStage>(i),
            reinterpret_cast<AppSpawnContent *>(mgr), nullptr);
        EXPECT_NE(ret, 0);
    }
    TerminateSpawnedProcess(app);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief RegChildLooper
 *
 */
static int TestChildLoop(AppSpawnContent *content, AppSpawnClient *client)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_RegChildLooper_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    RegChildLooper(reinterpret_cast<AppSpawnContent *>(mgr), TestChildLoop);
    RegChildLooper(reinterpret_cast<AppSpawnContent *>(mgr), nullptr);
    DeleteAppSpawnMgr(mgr);
    RegChildLooper(nullptr, TestChildLoop);
}

/**
 * @brief MakeDirRec
 * int MakeDirRec(const char *path, mode_t mode, int lastPath);
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_MakeDirRec_001, TestSize.Level0)
{
    int ret = MakeDirRec(nullptr, 0, -1);
    EXPECT_EQ(ret, -1);
    ret = MakeDirRec(APPSPAWN_BASE_DIR "/test_appspawn/3", 0, 0);
    EXPECT_EQ(ret, 0);
    ret = MakeDirRec(APPSPAWN_BASE_DIR "/test_appspawn/1", 0711, 0); // create file
    EXPECT_EQ(ret, 0);
    ret = MakeDirRec(APPSPAWN_BASE_DIR "/test_appspawn/2", 0711, 1); // create path
    EXPECT_EQ(ret, 0);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_DiffTime_001, TestSize.Level0)
{
    struct timespec endTime;
    struct timespec startTime;
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    startTime.tv_sec += 1;
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    uint64_t diff = DiffTime(&startTime, &endTime);
    EXPECT_NE(diff, 0);
    diff = DiffTime(nullptr, &endTime);
    EXPECT_EQ(diff, 0);
    diff = DiffTime(&startTime, nullptr);
    EXPECT_EQ(diff, 0);
}

int TestSplitStringHandle(const char *str, void *context)
{
    static int index = 0;
    if (index == 3) { // 3 test
        EXPECT_EQ(strcmp(str, "dddd"), 0);
    }
    index++;
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_StringSplit_001, TestSize.Level0)
{
    const char *testStr = "aaaa|bbbb|cccc  |dddd|   eeee| 1111 | 2222";
    int ret = StringSplit(testStr, "|", nullptr, TestSplitStringHandle);
    EXPECT_EQ(ret, 0);
    ret = StringSplit(testStr, "|", nullptr, nullptr);
    EXPECT_NE(ret, 0);

    ret = StringSplit(testStr, nullptr, nullptr, TestSplitStringHandle);
    EXPECT_NE(ret, 0);
    ret = StringSplit(nullptr, "|", nullptr, TestSplitStringHandle);
    EXPECT_NE(ret, 0);
    ret = StringSplit(nullptr, "|", nullptr, nullptr);
    EXPECT_NE(ret, 0);
    ret = StringSplit(nullptr, nullptr, nullptr, nullptr);
    EXPECT_NE(ret, 0);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetLastStr_001, TestSize.Level0)
{
    const char *testStr = "aaaa|bbbb|cccc  |dddd|   eeee| 1111 | 2222";
    char *tmp = GetLastStr(testStr, "2222");
    EXPECT_EQ(tmp != nullptr, 1);

    tmp = GetLastStr(testStr, nullptr);
    EXPECT_EQ(tmp != nullptr, 0);

    tmp = GetLastStr(nullptr, nullptr);
    EXPECT_EQ(tmp != nullptr, 0);
    tmp = GetLastStr(nullptr, "2222");
    EXPECT_EQ(tmp != nullptr, 0);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_DumpCurrentDir_001, TestSize.Level0)
{
    char path[PATH_MAX] = {};
    DumpCurrentDir(path, sizeof(path), APPSPAWN_BASE_DIR "/test_appspawn/");
    DumpCurrentDir(path, sizeof(path), nullptr);
    DumpCurrentDir(path, 0, nullptr);
    DumpCurrentDir(path, 0, APPSPAWN_BASE_DIR "/test_appspawn/");
    DumpCurrentDir(nullptr, sizeof(path), APPSPAWN_BASE_DIR "/test_appspawn/");
    DumpCurrentDir(nullptr, sizeof(path), nullptr);
}

static int TestParseAppSandboxConfig(const cJSON *root, ParseJsonContext *context)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_ParseJsonConfig_001, TestSize.Level0)
{
    int ret = 0;
#ifdef APPSPAWN_SANDBOX_NEW
    ret = ParseJsonConfig("etc/sandbox", WEB_SANDBOX_FILE_NAME, TestParseAppSandboxConfig, nullptr);
    EXPECT_EQ(ret, 0);
    ret = ParseJsonConfig("etc/sandbox", APP_SANDBOX_FILE_NAME, TestParseAppSandboxConfig, nullptr);
    EXPECT_EQ(ret, 0);
#else
    ret = ParseJsonConfig("etc/sandbox", APP_SANDBOX_FILE_NAME, TestParseAppSandboxConfig, nullptr);
    EXPECT_EQ(ret, 0);
#endif
    ret = ParseJsonConfig("etc/sandbox", APP_SANDBOX_FILE_NAME, nullptr, nullptr);
    EXPECT_NE(ret, 0);
    ret = ParseJsonConfig("etc/sandbox", nullptr, TestParseAppSandboxConfig, nullptr);
    EXPECT_NE(ret, 0);
    ret = ParseJsonConfig("etc/sandbox", nullptr, nullptr, nullptr);
    EXPECT_NE(ret, 0);
    ret = ParseJsonConfig(nullptr, APP_SANDBOX_FILE_NAME, TestParseAppSandboxConfig, nullptr);
    EXPECT_NE(ret, 0);
    ret = ParseJsonConfig(nullptr, nullptr, TestParseAppSandboxConfig, nullptr);
    EXPECT_NE(ret, 0);
    ret = ParseJsonConfig(nullptr, nullptr, nullptr, nullptr);
    EXPECT_NE(ret, 0);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetJsonObjFromFile_001, TestSize.Level0)
{
    cJSON *json = GetJsonObjFromFile("/etc/sandbox/appdata-sandbox.json");
    EXPECT_EQ(json != nullptr, 1);
    cJSON_Delete(json);
    json = GetJsonObjFromFile(nullptr);
    EXPECT_EQ(json == nullptr, 1);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetStringFromJsonObj_001, TestSize.Level0)
{
    const std::string buffer = "{ \
        \"global\": { \
            \"sandbox-root\": \"/mnt/sandbox/<currentUserId>/app-root\", \
            \"sandbox-ns-flags\": [ \"pid\", \"net\" ], \
            \"sandbox-flags\": 100, \
            \"top-sandbox-switch\": \"ON\" \
        } \
    }";

    cJSON *json = cJSON_Parse(buffer.c_str());
    EXPECT_EQ(json != nullptr, 1);
    char *tmp = GetStringFromJsonObj(cJSON_GetObjectItemCaseSensitive(json, "global"), "sandbox-root");
    EXPECT_EQ(tmp != nullptr, 1);
    tmp = GetStringFromJsonObj(cJSON_GetObjectItemCaseSensitive(json, "global"), nullptr);
    EXPECT_EQ(tmp == nullptr, 1);
    tmp = GetStringFromJsonObj(nullptr, "sandbox-root");
    EXPECT_EQ(tmp == nullptr, 1);
    tmp = GetStringFromJsonObj(nullptr, nullptr);
    EXPECT_EQ(tmp == nullptr, 1);
    cJSON_Delete(json);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetBoolValueFromJsonObj_001, TestSize.Level0)
{
    const std::string buffer = "{ \
        \"global\": { \
            \"sandbox-root\": \"/mnt/sandbox/<currentUserId>/app-root\", \
            \"sandbox-ns-flags\": [ \"pid\", \"net\" ], \
            \"sandbox-flags\": 100, \
            \"top-sandbox-switch\": \"ON\" \
        } \
    }";

    cJSON *json = cJSON_Parse(buffer.c_str());
    EXPECT_EQ(json != nullptr, 1);
    bool ret = GetBoolValueFromJsonObj(cJSON_GetObjectItemCaseSensitive(json, "global"), "top-sandbox-switch", false);
    EXPECT_EQ(ret, true);
    ret = GetBoolValueFromJsonObj(cJSON_GetObjectItemCaseSensitive(json, "global"), nullptr, false);
    EXPECT_EQ(ret, 0);
    ret = GetBoolValueFromJsonObj(nullptr, "top-sandbox-switch", false);
    EXPECT_EQ(ret, 0);
    ret = GetBoolValueFromJsonObj(nullptr, nullptr, false);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(json);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetIntValueFromJsonObj_001, TestSize.Level0)
{
    const std::string buffer = "{ \
        \"global\": { \
            \"sandbox-root\": \"/mnt/sandbox/<currentUserId>/app-root\", \
            \"sandbox-ns-flags\": [ \"pid\", \"net\" ], \
            \"sandbox-flags\": 100, \
            \"top-sandbox-switch\": \"ON\" \
        } \
    }";
    cJSON *json = cJSON_Parse(buffer.c_str());
    EXPECT_EQ(json != nullptr, 1);
    int tmp = GetIntValueFromJsonObj(cJSON_GetObjectItemCaseSensitive(json, "global"), "sandbox-flags", 0);
    EXPECT_EQ(tmp, 100);
    tmp = GetIntValueFromJsonObj(cJSON_GetObjectItemCaseSensitive(json, "global"), nullptr, 0);
    EXPECT_EQ(tmp, 0);
    tmp = GetIntValueFromJsonObj(nullptr, "sandbox-flags", 0);
    EXPECT_EQ(tmp, 0);
    tmp = GetIntValueFromJsonObj(nullptr, nullptr, 0);
    EXPECT_EQ(tmp, 0);
    cJSON_Delete(json);
}
}  // namespace OHOS
