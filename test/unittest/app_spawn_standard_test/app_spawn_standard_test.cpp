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

#include <gtest/gtest.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "app_spawn_stub.h"
#include "appspawn_adapter.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "env_utils.h"
#include "init_hashmap.h"
#include "json_utils.h"
#include "le_task.h"
#include "loop_event.h"
#include "parameter.h"
#include "sandbox_utils.h"
#include "securec.h"
#ifdef REPORT_EVENT
#include "event_reporter.h"
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;
using nlohmann::json;

#ifdef __cplusplus
    extern "C" {
#endif
TaskHandle AcceptClient(const LoopHandle loopHandle, const TaskHandle server, uint32_t flags);
void AddAppInfo(pid_t pid, const char *processName, AppOperateType code);
void SignalHandler(const struct signalfd_siginfo *siginfo);
int GetCgroupPath(const AppSpawnAppInfo *appInfo, char *buffer, uint32_t buffLen);
#ifdef __cplusplus
    }
#endif

static bool ReceiveRequestDataToExtraInfo(const TaskHandle taskHandle, AppSpawnClientExt *client,
    const uint8_t *buffer, uint32_t buffLen)
{
    if (client->property.extraInfo.totalLength) {
        ExtraInfo *extraInfo = &client->property.extraInfo;
        if (extraInfo->savedLength == 0) {
            extraInfo->data = (char *)malloc(extraInfo->totalLength);
            APPSPAWN_CHECK(extraInfo->data != NULL, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
                return false, "ReceiveRequestData: malloc extraInfo failed %{public}u", extraInfo->totalLength);
        }

        uint32_t saved = extraInfo->savedLength;
        uint32_t total = extraInfo->totalLength;
        char *data = extraInfo->data;

        APPSPAWN_LOGI("Receiving extraInfo: (%{public}u saved + %{public}u incoming) / %{public}u total",
            saved, buffLen, total);

        APPSPAWN_CHECK((total - saved) >= buffLen, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "ReceiveRequestData: too many data for extraInfo %{public}u ", buffLen);

        int ret = memcpy_s(data + saved, buffLen, buffer, buffLen);
        APPSPAWN_CHECK(ret == 0, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "ReceiveRequestData: memcpy extraInfo failed");

        extraInfo->savedLength += buffLen;
        if (extraInfo->savedLength < extraInfo->totalLength) {
            return false;
        }
        extraInfo->data[extraInfo->totalLength - 1] = 0;
        return true;
    }
    return true;
}

static int CheckRequestMsgValid(AppSpawnClientExt *client)
{
    if (client->property.extraInfo.totalLength >= EXTRAINFO_TOTAL_LENGTH_MAX) {
        APPSPAWN_LOGE("extrainfo total length invalid,len: %{public}d", client->property.extraInfo.totalLength);
        return -1;
    }
    for (int i = 0; i < APP_LEN_PROC_NAME; i++) {
        if (client->property.processName[i] == '\0') {
            return 0;
        }
    }

    APPSPAWN_LOGE("processname invalid");
    return -1;
}

bool ReceiveRequestData(const TaskHandle taskHandle, AppSpawnClientExt *client,
    const uint8_t *buffer, uint32_t buffLen)
{
    APPSPAWN_CHECK(buffer != NULL && buffLen > 0, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
        return false, "ReceiveRequestData: Invalid buff, bufferLen:%{public}d", buffLen);

    // 1. receive AppParamter
    if (client->property.extraInfo.totalLength == 0) {
        APPSPAWN_CHECK(buffLen >= sizeof(client->property), LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "ReceiveRequestData: Invalid buffLen %{public}u", buffLen);

        int ret = memcpy_s(&client->property, sizeof(client->property), buffer, sizeof(client->property));
        APPSPAWN_CHECK(ret == 0, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "ReceiveRequestData: memcpy failed %{public}d:%{public}u", ret, buffLen);

        // reset extraInfo
        client->property.extraInfo.savedLength = 0;
        client->property.extraInfo.data = NULL;

        // update buffer
        buffer += sizeof(client->property);
        buffLen -= sizeof(client->property);
        ret = CheckRequestMsgValid(client);
        APPSPAWN_CHECK(ret == 0, LE_CloseTask(LE_GetDefaultLoop(), taskHandle);
            return false, "Invalid request msg");
    }

    // 2. check whether extraInfo exist
    if (client->property.extraInfo.totalLength == 0) { // no extraInfo
        APPSPAWN_LOGV("ReceiveRequestData: no extraInfo");
        return true;
    } else if (buffLen == 0) {
        APPSPAWN_LOGV("ReceiveRequestData: waiting for extraInfo");
        return false;
    }
    return ReceiveRequestDataToExtraInfo(taskHandle, client, buffer, buffLen);
}

static int MakeDir(const char *dir, mode_t mode)
{
    int rc = -1;
    if (dir == nullptr || *dir == '\0') {
        errno = EINVAL;
        return rc;
    }
    rc = mkdir(dir, mode);
    if (rc < 0 && errno != EEXIST) {
        return rc;
    }
    return 0;
}

static int MakeDirRecursive(const char *dir, mode_t mode)
{
    int rc = -1;
    char buffer[PATH_MAX] = {0};
    const char *p = nullptr;
    if (dir == nullptr || *dir == '\0') {
        errno = EINVAL;
        return rc;
    }
    p = dir;
    const char *slash = strchr(dir, '/');
    while (slash != nullptr) {
        int gap = slash - p;
        p = slash + 1;
        if (gap == 0) {
            slash = strchr(p, '/');
            continue;
        }
        if (gap < 0) {  // end with '/'
            break;
        }
        if (memcpy_s(buffer, PATH_MAX, dir, p - dir - 1) != EOK) {
            return -1;
        }
        rc = MakeDir(buffer, mode);
        if (rc < 0) {
            return rc;
        }
        slash = strchr(p, '/');
    }
    return MakeDir(dir, mode);
}

static void CheckAndCreateDir(const char *fileName)
{
    printf("create path %s\n", fileName);
    if (fileName == nullptr || *fileName == '\0') {
        return;
    }
    char *path = strndup(fileName, strrchr(fileName, '/') - fileName);
    if (path == nullptr) {
        return;
    }
    if (access(path, F_OK) == 0) {
        free(path);
        return;
    }
    MakeDirRecursive(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    free(path);
}

static void ProcessTimer(const TimerHandle taskHandle, void *context)
{
    UNUSED(context);
    APPSPAWN_LOGI("timeout stop appspawn");
    LE_StopLoop(LE_GetDefaultLoop());
}

namespace OHOS {
class AppSpawnStandardTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppSpawnStandardTest::SetUpTestCase()
{}

void AppSpawnStandardTest::TearDownTestCase()
{}

void AppSpawnStandardTest::SetUp()
{}

void AppSpawnStandardTest::TearDown()
{}

int32_t TestSetAppSandboxProperty(struct AppSpawnContent *content, AppSpawnClient *client)
{
    return 0;
}

static void FreeHspList(ExtraInfo &extraInfo)
{
    if (extraInfo.data != nullptr) {
        free(extraInfo.data);
    }
    extraInfo = {};
}

/**
* @tc.name: App_Spawn_Standard_003
* @tc.desc:  Verify set Arg if GetAppSpawnClient succeed.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_003 start";
    AppSpawnClientExt* client = (AppSpawnClientExt*)malloc(sizeof(AppSpawnClientExt));
    client->client.id = 8; // 8 is client id
    client->client.flags = 1; // 1 is flags
    client->fd[0] = 100; // 100 is fd
    client->fd[1] = 200; // 200 is fd
    client->property.uid = 10000; // 10000 is uid
    client->property.gid = 1000; // 1000 is gid
    client->property.gidCount = 1; // 1 is gidCount
    if (strcpy_s(client->property.processName, APP_LEN_PROC_NAME, "xxx.xxx.xxx") != 0) {
        GTEST_LOG_(INFO) << "strcpy_s failed";
    }
    if (strcpy_s(client->property.bundleName, APP_LEN_BUNDLE_NAME, "xxx.xxx.xxx") != 0) {
        GTEST_LOG_(INFO) << "strcpy_s failed";
    }
    if (strcpy_s(client->property.soPath, APP_LEN_SO_PATH, "xxx") != 0) {
        GTEST_LOG_(INFO) << "strcpy_s failed";
    }
    client->property.accessTokenId = 671201800; // 671201800 is accessTokenId
    if (strcpy_s(client->property.apl, APP_APL_MAX_LEN, "xxx") != 0) {
        GTEST_LOG_(INFO) << "strcpy_s failed";
    }
    if (strcpy_s(client->property.renderCmd, APP_RENDER_CMD_MAX_LEN, "xxx") != 0) {
        GTEST_LOG_(INFO) << "strcpy_s failed";
    }
    if (strcpy_s(client->property.ownerId, APP_RENDER_CMD_MAX_LEN, "xxx") != 0) {
        GTEST_LOG_(INFO) << "strcpy_s failed";
    }
    client->property.flags = 0;
    char arg1[] = "/system/bin/appspawn";
    char arg2[] = "cold-start";
    char arg3[] = "1";
    char arg4[] = "1:1:1:1:2:1000:1000:";
    char* argv[] = {arg1, arg2, arg3, arg4};
    int argc = sizeof(argv)/sizeof(argv[0]);

    // test invalid
    EXPECT_EQ(GetAppSpawnClientFromArg(2, argv, client), -1);
    EXPECT_EQ(GetAppSpawnClientFromArg(argc, nullptr, client), -1);
    EXPECT_EQ(GetAppSpawnClientFromArg(argc, argv, client), -1);
    free(client);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_003 end";
}

/**
* @tc.name: App_Spawn_Standard_003_1
* @tc.desc:  Verify set Arg if GetAppSpawnClient succeed, with ExtraInfo
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_003_1, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Standard_003_1 start");
    AppSpawnClientExt client = {};
    char arg1[] = "/system/bin/appspawn";
    char arg2[] = "cold-start";
    char arg3[] = "1";
    {
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "10";
        char arg6[] = "012345678";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
        FreeHspList(client.property.extraInfo);
    }
    { // hsp length is 0
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "0";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // hsp length is nullptr
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg6[] = "0123456789";
        char* argv[] = {arg1, arg2, arg3, arg4, nullptr, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // hsp length is non-zero, but argc is 5
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "10";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // hsp length is non-zero, but content is nullptr
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "10";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }

    APPSPAWN_LOGI("App_Spawn_Standard_003_1 en");
}

/**
* @tc.name: App_Spawn_Standard_003_2
* @tc.desc:  Verify set Arg if GetAppSpawnClient succeed, wrong ExtraInfo length
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_003_2, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Standard_003_2 start");
    AppSpawnClientExt client = {};
    char arg1[] = "/system/bin/appspawn";
    char arg2[] = "cold-start";
    char arg3[] = "1";
    { // actual data is shorter than totalLength
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "10";
        char arg6[] = "01234";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
        FreeHspList(client.property.extraInfo);
    }
    { // actual data is longer than totalLength
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "5";
        char arg6[] = "0123456789";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
        FreeHspList(client.property.extraInfo);
    }

    APPSPAWN_LOGI("App_Spawn_Standard_003_2 end");
}

/**
* @tc.name: App_Spawn_Standard_003_3
* @tc.desc:  Verify set Arg if GetAppSpawnClient succeed, with overlay
* @tc.type: FUNC
* @tc.require:issueI7D0H9
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_003_3, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Standard_003_3 start");
    AppSpawnClientExt client = {};
    char arg1[] = "/system/bin/appspawn";
    char arg2[] = "cold-start";
    char arg3[] = "1";
    char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
    char arg5[] = "0";
    char arg6[] = "0";
    {
        char arg7[] = "10";
        char arg8[] = "012345678";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
        FreeHspList(client.property.extraInfo);
    }
    { // overlay length is 0
        char arg7[] = "0";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // overlay length is nullptr
        char arg8[] = "0123456789";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, nullptr, arg8};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // overlay length is non-zero, but argc is 5
        char arg7[] = "10";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // overlay length is non-zero, but content is nullptr
        char arg7[] = "10";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    APPSPAWN_LOGI("App_Spawn_Standard_003_3 en");
}
/**
* @tc.name: App_Spawn_Standard_003_4
* @tc.desc:  Verify set Arg if GetAppSpawnClient succeed, with dataGroupList
* @tc.type: FUNC
* @tc.require:issueI7FUPV
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_003_4, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Standard_003_4 start");
    AppSpawnClientExt client = {};
    char arg1[] = "/system/bin/appspawn";
    char arg2[] = "cold-start";
    char arg3[] = "1";
    char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
    char arg5[] = "0";
    char arg6[] = "0";
    char arg7[] = "0";
    char arg8[] = "0";
    {
        char arg9[] = "10";
        char arg10[] = "012345678";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
        FreeHspList(client.property.extraInfo);
    }
    { // dataGroupList length is 0
        char arg9[] = "0";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // dataGroupList length is nullptr
        char arg10[] = "0123456789";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, nullptr, arg10};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // dataGroupList length is non-zero, but argc is 7
        char arg9[] = "10";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // dataGroupList length is non-zero, but content is nullptr
        char arg9[] = "10";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    APPSPAWN_LOGI("App_Spawn_Standard_003_4 en");
}

/**
* @tc.name: App_Spawn_Standard_003_5
* @tc.desc: Verify set Arg if GetAppSpawnClient succeed, with AppEnvInfo
* @tc.type: FUNC
* @tc.require:issueI936IH
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_003_5, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Standard_003_5 start");
    AppSpawnClientExt client = {};
    char arg1[] = "/system/bin/appspawn";
    char arg2[] = "cold-start";
    char arg3[] = "1";
    {
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "200";
        char arg6[] = "|AppEnv|{\"test.name1\": \"test.value1\", \"test.name2\": \"test.value2\"}|AppEnv|";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // AppEnvInfo content is valid, but length is 0
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "0";
        char arg6[] = "|AppEnv|{\"test.name1\": \"test.value1\", \"test.name2\": \"test.value2\"}|AppEnv|";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // AppEnvInfo content is valid, but length is nullptr
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg6[] = "|AppEnv|{\"test.name1\": \"test.value1\", \"test.name2\": \"test.value2\"}|AppEnv|";
        char* argv[] = {arg1, arg2, arg3, arg4, nullptr, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // AppEnvInfo length is valid, but content is nullptr
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "200";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // AppEnvInfo length is valid, but argc is 5
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char arg5[] = "200";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    APPSPAWN_LOGI("App_Spawn_Standard_003_5 end");
}

/**
* @tc.name: App_Spawn_Standard_004
* @tc.desc: App cold start.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_004 start";
    string longProcName = "App_Spawn_Standard_004";
    int64_t longProcNameLen = longProcName.length();
    int cold = 1;
    AppSpawnContent *content = AppSpawnCreateContent("AppSpawn", (char*)longProcName.c_str(), longProcNameLen, cold);
    EXPECT_TRUE(content);
    content->loadExtendLib = LoadExtendLib;
    content->runChildProcessor = RunChildProcessor;

    content->runChildProcessor(content, nullptr);

    char tmp0[] = "/system/bin/appspawn";
    char tmp1[] = "cold-start";
    char tmp2[] = "1";
    {
        char tmp3[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char * const argv[] = {tmp0, tmp1, tmp2, tmp3};
        AppSpawnColdRun(content, 4, argv);
    }
    // test invalid param
    {
        char tmp3[] = "1:1:1:1:2:1000:1000:ohos.samples.ecg.default:ohos.samples.ecg:";
        char * const argv[] = {tmp0, tmp1, tmp2, tmp3};
        AppSpawnColdRun(content, 4, argv);
    }
    {
        char tmp3[] = "1:1:1:1:2:1000:1000:ohos.samples.ecg.default";
        char * const argv[] = {tmp0, tmp1, tmp2, tmp3};
        AppSpawnColdRun(content, 4, argv);
    }
    {
        char tmp3[] = "1:1:1:1:2:1000:1000";
        char * const argv[] = {tmp0, tmp1, tmp2, tmp3};
        AppSpawnColdRun(content, 4, argv);
    }
    {
        char tmp3[] = "1:1:1:1:2:1000";
        char * const argv[] = {tmp0, tmp1, tmp2, tmp3};
        AppSpawnColdRun(content, 4, argv);
    }
    {
        char tmp3[] = "1:1:1:1";
        char * const argv[] = {tmp0, tmp1, tmp2, tmp3};
        AppSpawnColdRun(content, 4, argv);
    }
    GTEST_LOG_(INFO) << "App_Spawn_Standard_004 end";
}

/**
* @tc.name: App_Spawn_Standard_004_1
* @tc.desc: App cold start.
* @tc.type: FUNC
* @tc.require:issueI936IH
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_004_1, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Standard_004_1 start");
    string longProcName = "App_Spawn_Standard_004_1";
    int64_t longProcNameLen = longProcName.length();
    int cold = 1;
    AppSpawnContent *content = AppSpawnCreateContent("AppSpawn", (char*)longProcName.c_str(), longProcNameLen, cold);
    EXPECT_TRUE(content);
    content->loadExtendLib = LoadExtendLib;
    content->runChildProcessor = RunChildProcessor;
    content->setEnvInfo = SetEnvInfo;
    content->runChildProcessor(content, nullptr);

    char tmp0[] = "/system/bin/appspawn";
    char tmp1[] = "cold-start";
    char tmp2[] = "1";
    {
        char tmp3[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:owerid:0:671201800";
        char tmp4[] = "200";
        char tmp5[] = "|AppEnv|{\"test.name1\": \"test.value1\", \"test.name2\": \"test.value2\"}|AppEnv|";
        char * const argv[] = {tmp0, tmp1, tmp2, tmp3, tmp4, tmp5};
        AppSpawnColdRun(content, 6, argv);
    }
    APPSPAWN_LOGI("App_Spawn_Standard_004_1 end");
}

/**
* @tc.name: App_Spawn_Standard_005
* @tc.desc: Verify start App.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_005 start";
    string longProcName = "ohos.samples.ecg.default";
    int64_t longProcNameLen = longProcName.length();
    std::unique_ptr<AppSpawnClientExt> clientExt = std::make_unique<AppSpawnClientExt>();
    AppSpawnContent *content = AppSpawnCreateContent("AppSpawn", (char*)longProcName.c_str(), longProcNameLen, 1);
    content->loadExtendLib = LoadExtendLib;
    content->runChildProcessor = RunChildProcessor;
    int ret = DoStartApp((AppSpawnContent*)content, &clientExt->client, (char*)"", 0);
    EXPECT_EQ(ret, 0);

    if (strcpy_s(clientExt->property.bundleName, APP_LEN_BUNDLE_NAME, "moduleTestProcessName") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 2" << std::endl;
    }
    ret = SetAppSandboxProperty(nullptr, nullptr);
    EXPECT_NE(ret, 0);
    ret = SetAppSandboxProperty((AppSpawnContent*)content, &clientExt->client);
    EXPECT_EQ(ret, 0);

    // APP_NO_SANDBOX
    clientExt->property.flags |= APP_NO_SANDBOX;
    ret = SetAppSandboxProperty((AppSpawnContent*)content, &clientExt->client);
    EXPECT_EQ(ret, 0);

    clientExt->property.flags &= ~APP_NO_SANDBOX;
    // bundle name
    clientExt->property.extraInfo.data = strdup("{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }");
    clientExt->property.extraInfo.totalLength = strlen(clientExt->property.extraInfo.data);
    ret = SetAppSandboxProperty((AppSpawnContent*)content, &clientExt->client);
    EXPECT_EQ(ret, 0);

    free(content);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_005 end";
}

static int RunClient(AppSpawnContentExt *content, int flags, AppOperateType code, const std::string &processName)
{
    // create connection
    TaskHandle stream = AcceptClient(LE_GetDefaultLoop(), content->server, TASK_TEST);
    // process recv message
    if (stream == nullptr) {
        return -1;
    }

    // do not test sandbox in main
    content->content.setAppSandbox = TestSetAppSandboxProperty;
    StreamConnectTask *task = reinterpret_cast<StreamConnectTask *>(stream);
    AppParameter property = {};
    property.uid = 100; // 100 is uid
    property.gid = 100; // 100 is gid
    property.gidCount = 1; // 1 is gidCount
    property.gidTable[0] = 101; // 101 is gidTable
    if (code == SPAWN_NATIVE_PROCESS) {
        (void)strcpy_s(property.processName, sizeof(property.processName), "ohos.appspawn.test.cmd");
    } else {
        (void)strcpy_s(property.processName, sizeof(property.processName), processName.c_str());
    }
    (void)strcpy_s(property.bundleName, sizeof(property.bundleName), processName.c_str());
    (void)strcpy_s(property.renderCmd, sizeof(property.renderCmd), processName.c_str());
    (void)strcpy_s(property.soPath, sizeof(property.soPath), processName.c_str());
    (void)strcpy_s(property.ownerId, sizeof(property.ownerId), processName.c_str());
    (void)strcpy_s(property.apl, sizeof(property.apl), "system_core");
    property.flags = flags;
    property.code = code;
    property.accessTokenId = 0;
    property.setAllowInternet = 1;
    property.allowInternet = 0;

    task->recvMessage(stream, (const uint8_t *)&property, sizeof(property));
    LE_Buffer buffer = {};
    OH_ListInit(&buffer.node);
    buffer.result = -1;
    task->sendMessageComplete(stream, &buffer);
    buffer.result = 0;
    task->sendMessageComplete(stream, &buffer);
    // test signal
    struct signalfd_siginfo siginfo = {};
    siginfo.ssi_signo = SIGCHLD;
    siginfo.ssi_pid = GetTestPid();
    SignalHandler(&siginfo);
    return 0;
}

static AppSpawnContentExt *TestClient(int flags,
    AppOperateType code, const std::string &processName, const std::string &serverName)
{
    char buffer[64] = {0}; // 64 buffer size
    CheckAndCreateDir(SOCKET_DIR);
    AppSpawnContentExt *content =
        (AppSpawnContentExt *)AppSpawnCreateContent(serverName.c_str(), buffer, sizeof(buffer), 0);
    if (content == nullptr) {
        return nullptr;
    }
    EXPECT_NE(content->content.initAppSpawn, nullptr);
    EXPECT_NE(content->content.runAppSpawn, nullptr);

    // set common operation
    content->content.loadExtendLib = LoadExtendLib;
    content->content.runChildProcessor = RunChildProcessor;
    content->flags |= (flags & APP_COLD_BOOT) ? FLAGS_ON_DEMAND : 0;
    // test nullptr
    StreamServerTask *task = reinterpret_cast<StreamServerTask *>(content->server);
    task->incommingConnect(nullptr, nullptr);
    task->incommingConnect(LE_GetDefaultLoop(), nullptr);
    int ret;
    content->content.initAppSpawn(&content->content);
    if (content->timer == nullptr) { // create timer for exit
        ret = LE_CreateTimer(LE_GetDefaultLoop(), &content->timer, ProcessTimer, nullptr);
        EXPECT_EQ(ret, 0);
        ret = LE_StartTimer(LE_GetDefaultLoop(), content->timer, 500, 1); // 500 ms is timeout
        EXPECT_EQ(ret, 0);
    }
    ret = RunClient(content, flags, code, processName);
    EXPECT_EQ(ret, 0);

    if (content->timer == nullptr) { // create timer for exit
        ret = LE_CreateTimer(LE_GetDefaultLoop(), &content->timer, ProcessTimer, nullptr);
        EXPECT_EQ(ret, 0);
        ret = LE_StartTimer(LE_GetDefaultLoop(), content->timer, 500, 1); // 500 ms is timeout
        EXPECT_EQ(ret, 0);
    }
    return content;
}

/**
* @tc.name: App_Spawn_Standard_006
* @tc.desc: start App.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006 start";
    AppSpawnContentExt *content = TestClient(0, DEFAULT, "com.ohos.UserFile.ExternalFileManager", "test006");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_006_1, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_1 start";
    SetHapDomainSetcontextResult(1);
    AppSpawnContentExt *content = TestClient(0, DEFAULT, "com.ohos.medialibrary.medialibrarydata", "test006_1");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_1 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_006_2, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_2 start";
    SetHapDomainSetcontextResult(-1);
    SetParameter("startup.appspawn.cold.boot", "1");
    AppSpawnContentExt *content = TestClient(APP_COLD_BOOT,
        DEFAULT, "com.ohos.medialibrary.medialibrarydata", "test006_2");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_2 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_006_3, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_3 start";
    SetHapDomainSetcontextResult(-1);
    AppSpawnContentExt *content = TestClient(APP_COLD_BOOT,
        DEFAULT, "com.ohos.medialibrary.medialibrarydata", "test006_2");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_3 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_006_4, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_4 start";
    SetHapDomainSetcontextResult(-1);

    AppSpawnContentExt *content = TestClient(APP_COLD_BOOT,
        DEFAULT, "com.ohos.medialibrary.medialibrarydata", "test006_2");
    ASSERT_TRUE(content != nullptr);
    content->content.coldStartApp = nullptr;
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_4 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_006_5, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_5 start";
    SetHapDomainSetcontextResult(-1);
    AppSpawnContentExt *content = TestClient(0, DEFAULT, "ohos.samples.ecg", "test006_5");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_5 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_006_5_1, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_5_1 start";
    SetHapDomainSetcontextResult(-1);
    AppSpawnContentExt *content = TestClient(0, DEFAULT, "ohos.samples.ecg", "test006_5");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006_5_1 end";
}
/**
* @tc.name: App_Spawn_Standard_07
* @tc.desc: Verify signal deal function.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_07, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_07 start";
    AppSpawnContentExt *content = TestClient(0, DEFAULT, "ohos.test.testapp", "test007");
    EXPECT_TRUE(content != nullptr);
    AddAppInfo(111, "111", DEFAULT);
    AddAppInfo(65, "112", DEFAULT);
    AddAppInfo(97, "113", DEFAULT);

    struct signalfd_siginfo siginfo = {};
    siginfo.ssi_signo = SIGCHLD;
    siginfo.ssi_pid = 111; // 111 is pid
    SignalHandler(&siginfo);

    siginfo.ssi_signo = SIGTERM;
    siginfo.ssi_pid = 111; // 111 is pid
    SignalHandler(&siginfo);

    siginfo.ssi_signo = 0;
    siginfo.ssi_pid = 111; // 111 is pid
    SignalHandler(&siginfo);
    content->content.runAppSpawn(&content->content, 0, nullptr);

    struct timespec tmStart = {0};
    clock_gettime(CLOCK_REALTIME, &tmStart);
    tmStart.tv_nsec += 100000000; // 100000000 for test
    long long diff = DiffTime(&tmStart);
    APPSPAWN_LOGI("App timeused %{public}d %{public}lld ns.", getpid(), diff);

    GTEST_LOG_(INFO) << "App_Spawn_Standard_07 end";
}

/**
* @tc.name: App_Spawn_Standard_08
* @tc.desc: verify receive extraInfo
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_08, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08 start";
    AppSpawnClientExt client = {};
    AppParameter param = {};
    { // buff is nullptr
        bool ret = ReceiveRequestData(nullptr, &client, nullptr, sizeof(param));
        EXPECT_FALSE(ret);
    }
    { // buffLen is 0
        bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)&param, 0);
        EXPECT_FALSE(ret);
    }
    { // buffLen < sizeof(AppParameter)
        bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)&param, sizeof(param) - 1);
        EXPECT_FALSE(ret);
    }
    { // no ExtraInfo
        bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)&param, sizeof(param));
        EXPECT_TRUE(ret);
    }
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08 end";
}

/**
* @tc.name: App_Spawn_Standard_08_1
* @tc.desc: receive AppParameter and ExtraInfo separately
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_08_1, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_1 start";
    AppSpawnClientExt client = {};
    AppParameter param = {};

    // send AppParameter
    char hspListStr[] = "01234";
    param.extraInfo = {sizeof(hspListStr), 0, nullptr};
    bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)&param, sizeof(param));
    EXPECT_FALSE(ret);

    // send ExtraInfo
    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr, sizeof(hspListStr));
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, strcmp(hspListStr, client.property.extraInfo.data));

    FreeHspList(client.property.extraInfo);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_1 end";
}

/**
* @tc.name: App_Spawn_Standard_08_2
* @tc.desc: receive AppParameter and ExtraInfo together
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_08_2, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_2 start";
    AppSpawnClientExt client = {};
    AppParameter param = {};

    // put AppParameter and ExtraInfo together
    char hspListStr[] = "01234";
    char buff[sizeof(param) + sizeof(hspListStr)];
    param.extraInfo = {sizeof(hspListStr), 0, nullptr};
    int res = memcpy_s(buff, sizeof(param), (void *)&param, sizeof(param));
    EXPECT_EQ(0, res);
    res = memcpy_s(buff + sizeof(param), sizeof(hspListStr), (void *)hspListStr, sizeof(hspListStr));
    EXPECT_EQ(0, res);

    // send
    bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)buff, sizeof(buff));
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, strcmp(hspListStr, client.property.extraInfo.data));

    FreeHspList(client.property.extraInfo);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_2 end";
}

/**
* @tc.name: App_Spawn_Standard_08_3
* @tc.desc: receive AppParameter and part of ExtraInfo
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_08_3, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_3 start";
    AppSpawnClientExt client = {};
    AppParameter param = {};
    const uint32_t splitLen = 3;

    // put AppParameter and part of ExtraInfo together
    char hspListStr[] = "0123456789";
    char buff[sizeof(param) + splitLen];
    param.extraInfo = {sizeof(hspListStr), 0, hspListStr};
    int res = memcpy_s(buff, sizeof(param), (void *)&param, sizeof(param));
    EXPECT_EQ(0, res);
    res = memcpy_s(buff + sizeof(param), splitLen, (void *)hspListStr, splitLen);
    EXPECT_EQ(0, res);

    // send AppParameter and part of ExtraInfo
    bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)buff, sizeof(buff));
    EXPECT_FALSE(ret);

    // send left ExtraInfo
    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr + splitLen, sizeof(hspListStr) - splitLen);
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, strcmp(hspListStr, client.property.extraInfo.data));

    FreeHspList(client.property.extraInfo);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_3 end";
}

/**
* @tc.name: App_Spawn_Standard_08_4
* @tc.desc: receive AppParameter and splited ExtraInfo
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_08_4, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_4 start";
    AppSpawnClientExt client = {};
    AppParameter param = {};
    const uint32_t splitLen = 3;
    uint32_t sentLen = 0;

    // send AppParameter
    char hspListStr[] = "0123456789";
    param.extraInfo = {sizeof(hspListStr), 0, nullptr};
    bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)&param, sizeof(param));
    EXPECT_FALSE(ret);

    // send splited ExtraInfo
    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr + sentLen, splitLen);
    sentLen += splitLen;
    EXPECT_FALSE(ret);

    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr + sentLen, splitLen);
    sentLen += splitLen;
    EXPECT_FALSE(ret);

    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr + sentLen, sizeof(hspListStr) - sentLen);
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, strcmp(hspListStr, client.property.extraInfo.data));

    FreeHspList(client.property.extraInfo);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_4 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_ReportEvent, TestSize.Level0)
{
    ReportProcessExitInfo(nullptr, 100, 100, 0);
    ReportProcessExitInfo("nullptr", 100, 100, 0);
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_009_01, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_009_01 start";
    SetHapDomainSetcontextResult(-1);
    AppSpawnContentExt *content = TestClient(0,
        SPAWN_NATIVE_PROCESS, "ls -l > /data/appspawn_ut/test009_1", "test009_1");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_009_01 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_009_02, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_009_02 start";
    SetHapDomainSetcontextResult(-1);
    AppSpawnContentExt *content = TestClient(APP_NO_SANDBOX,
        SPAWN_NATIVE_PROCESS, "ls -l > /data/appspawn_ut/test009_02", "test009_02");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_009_02 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_009_03, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_009_03 start";
    SetHapDomainSetcontextResult(-1);
    AppSpawnContentExt *content = TestClient(APP_COLD_BOOT,
        SPAWN_NATIVE_PROCESS, "ls -l > /data/appspawn_ut/test009_03", "test009_03");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_009_03 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_Standard_009_04, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_009_04 start";
    SetHapDomainSetcontextResult(-1);
    AppSpawnContentExt *content = TestClient(APP_COLD_BOOT | APP_NO_SANDBOX,
        SPAWN_NATIVE_PROCESS, "ls -l > /data/appspawn_ut/test009_04", "test009_04");
    ASSERT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_009_04 end";
}

HWTEST(AppSpawnStandardTest, App_Spawn_CGroup_001, TestSize.Level0)
{
    int ret = -1;
    AppSpawnAppInfo *appInfo = nullptr;
    const char name[] = "app-test-001";
    do {
        appInfo = reinterpret_cast<AppSpawnAppInfo *>(malloc(sizeof(AppSpawnAppInfo) + strlen(name) + 1));
        APPSPAWN_CHECK(appInfo != nullptr, break, "Failed to create appInfo");
        appInfo->pid = 33; // 33
        appInfo->code = DEFAULT;
        appInfo->uid = 200000 * 200 + 21; // 200000 200 21
        ret = strcpy_s(appInfo->name, strlen(name) + 1, name);
        APPSPAWN_CHECK(ret == 0, break, "Failed to strcpy process name");
        HASHMAPInitNode(&appInfo->node);
        char path[PATH_MAX] = {};
        ret = GetCgroupPath(appInfo, path, sizeof(path));
        APPSPAWN_CHECK(ret == 0, break, "Failed to get real path errno: %d", errno);
        APPSPAWN_CHECK(strstr(path, "200") != nullptr && strstr(path, "33") != nullptr && strstr(path, name) != nullptr,
            break, "Invalid path: %s", path);
        ret = 0;
    } while (0);
    if (appInfo) {
        free(appInfo);
    }
    ASSERT_EQ(ret, 0);
}

HWTEST(AppSpawnStandardTest, App_Spawn_CGroup_002, TestSize.Level0)
{
    int ret = -1;
    AppSpawnAppInfo *appInfo = nullptr;
    AppSpawnContent *content = nullptr;
    FILE *file = nullptr;
    const char name[] = "app-test-001";
    do {
        char path[PATH_MAX] = {};
        appInfo = reinterpret_cast<AppSpawnAppInfo *>(malloc(sizeof(AppSpawnAppInfo) + strlen(name) + 1));
        APPSPAWN_CHECK(appInfo != nullptr, break, "Failed to create appInfo");
        appInfo->pid = 33; // 33
        appInfo->code = DEFAULT;
        appInfo->uid = 200000 * 200 + 21; // 200000 200 21
        ret = strcpy_s(appInfo->name, strlen(name) + 1, name);
        APPSPAWN_CHECK(ret == 0, break, "Failed to strcpy process name");
        HASHMAPInitNode(&appInfo->node);

        content = AppSpawnCreateContent(APPSPAWN_SOCKET_NAME, path, sizeof(path), 1);
        APPSPAWN_CHECK_ONLY_EXPER(content != nullptr, break);
        // spawn prepare process
        ProcessAppAdd(reinterpret_cast<AppSpawnContentExt *>(content), appInfo);
        // add success
        ret = GetCgroupPath(appInfo, path, sizeof(path));
        APPSPAWN_CHECK(ret == 0, break, "Failed to get real path errno: %{public}d", errno);
        ret = strcat_s(path, sizeof(path), "cgroup.procs");
        APPSPAWN_CHECK(ret == 0, break, "Failed to strcat_s errno: %{public}d", errno);
        file = fopen(path, "r");
        APPSPAWN_CHECK(file != NULL, break, "Open file fail %{public}s errno: %{public}d", path, errno);
        pid_t pid = 0;
        ret = -1;
        while (fscanf_s(file, "%d\n", &pid) == 1 && pid > 0) {
            APPSPAWN_LOGV("pid %d %d", pid, appInfo->pid);
            if (pid == appInfo->pid) {
                ret = 0;
                break;
            }
        }
        fclose(file);
        APPSPAWN_CHECK(ret == 0, break, "Error no pid write to path: %{public}s", path);
        ret = 0;
    } while (0);
    if (appInfo) {
        free(appInfo);
    }
    free(content);
    ASSERT_EQ(ret, 0);
}

HWTEST(AppSpawnStandardTest, App_Spawn_CGroup_003, TestSize.Level0)
{
    int ret = -1;
    AppSpawnAppInfo *appInfo = nullptr;
    AppSpawnContent *content = nullptr;
    const char name[] = "app-test-001";
    do {
        char path[PATH_MAX] = {};
        appInfo = reinterpret_cast<AppSpawnAppInfo *>(malloc(sizeof(AppSpawnAppInfo) + strlen(name) + 1));
        APPSPAWN_CHECK(appInfo != nullptr, break, "Failed to create appInfo");
        appInfo->pid = 33; // 33
        appInfo->code = DEFAULT;
        appInfo->uid = 200000 * 200 + 21; // 200000 200 21
        ret = strcpy_s(appInfo->name, strlen(name) + 1, name);
        APPSPAWN_CHECK(ret == 0, break, "Failed to strcpy process name");
        HASHMAPInitNode(&appInfo->node);

        content = AppSpawnCreateContent(APPSPAWN_SOCKET_NAME, path, sizeof(path), 1);
        APPSPAWN_CHECK_ONLY_EXPER(content != nullptr, break);
        // spawn prepare process
        ProcessAppDied(reinterpret_cast<AppSpawnContentExt *>(content), appInfo);
        ret = 0;
    } while (0);
    if (appInfo) {
        free(appInfo);
    }
    free(content);
    ASSERT_EQ(ret, 0);
}
} // namespace OHOS
