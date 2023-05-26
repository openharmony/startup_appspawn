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

#include <string>
#include <cerrno>
#include <memory>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>

#include "appspawn_service.h"
#include "appspawn_adapter.h"
#include "appspawn_server.h"
#include "app_spawn_stub.h"
#include "securec.h"
#include "json_utils.h"
#include "init_hashmap.h"
#include "le_task.h"
#include "loop_event.h"
#include "sandbox_utils.h"
#include "parameter.h"
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
bool ReceiveRequestData(const TaskHandle taskHandle, AppSpawnClientExt *appProperty,
    const uint8_t *buffer, uint32_t buffLen);
void AddAppInfo(pid_t pid, const char *processName);
void ProcessTimer(const TimerHandle taskHandle, void *context);
void SignalHandler(const struct signalfd_siginfo *siginfo);
#ifdef __cplusplus
    }
#endif

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
        if (gap < 0) { // end with '/'
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

int32_t TestSetAppSandboxProperty(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    return 0;
}

static void FreeHspList(HspList &hspList)
{
    if (hspList.data != nullptr) {
        free(hspList.data);
    }
    hspList = {};
}

/**
* @tc.name: App_Spawn_Standard_002
* @tc.desc: fork app son process and set content.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_002, TestSize.Level0)
{
    CheckAndCreateDir("/data/appspawn_ut/dev/unix/socket/");
    GTEST_LOG_(INFO) << "App_Spawn_Standard_002 start";
    char longProcName[124] = "App_Spawn_Standard_002";
    int64_t longProcNameLen = 124; // 124 is str length
    AppSpawnClientExt* client = (AppSpawnClientExt*)malloc(sizeof(AppSpawnClientExt));
    client->client.id = 8; // 8 is client id
    client->client.flags = 0;
    client->client.cloneFlags = CLONE_NEWNS;
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
    client->property.hspList = {0, 0, nullptr};

    AppSpawnContent *content = AppSpawnCreateContent("AppSpawn", longProcName, longProcNameLen, 1);
    content->loadExtendLib = LoadExtendLib;
    content->runChildProcessor = RunChildProcessor;
    SetContentFunction(content);

    content->clearEnvironment(content, &client->client);
    EXPECT_EQ(content->setProcessName(content, &client->client, (char *)longProcName, longProcNameLen), 0);
    EXPECT_EQ(content->setKeepCapabilities(content, &client->client), 0);
    EXPECT_EQ(content->setUidGid(content, &client->client), 0);
    EXPECT_EQ(content->setCapabilities(content, &client->client), 0);
    EXPECT_EQ(content->setFileDescriptors(content, &client->client), 0);

    // test invalid
    EXPECT_NE(content->setProcessName(content, &client->client, nullptr, 0), 0);

    content->setAppSandbox(content, &client->client);
    content->setAppAccessToken(content, &client->client);
    EXPECT_NE(content->coldStartApp(content, &client->client), 0);

    GTEST_LOG_(INFO) << "App_Spawn_Standard_002 end";
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
* @tc.desc:  Verify set Arg if GetAppSpawnClient succeed, with HspList
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
            "default:671201800:system_core:default:0:671201800";
        char arg5[] = "10";
        char arg6[] = "012345678";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
        FreeHspList(client.property.hspList);
    }
    { // hsp length is 0
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:0:671201800";
        char arg5[] = "0";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // hsp length is nullptr
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:0:671201800";
        char arg6[] = "0123456789";
        char* argv[] = {arg1, arg2, arg3, arg4, nullptr, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // hsp length is non-zero, but argc is 5
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:0:671201800";
        char arg5[] = "10";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }
    { // hsp length is non-zero, but content is nullptr
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:0:671201800";
        char arg5[] = "10";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, nullptr};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
    }

    APPSPAWN_LOGI("App_Spawn_Standard_003_1 en");
}

/**
* @tc.name: App_Spawn_Standard_003_2
* @tc.desc:  Verify set Arg if GetAppSpawnClient succeed, wrong HspList length
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
            "default:671201800:system_core:default:0:671201800";
        char arg5[] = "10";
        char arg6[] = "01234";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(0, GetAppSpawnClientFromArg(argc, argv, &client));
        FreeHspList(client.property.hspList);
    }
    { // actual data is longer than totalLength
        char arg4[] = "1:1:1:1:1:1:1:1:1:2:1000:1000:ohos.samples:ohos.samples.ecg:"
            "default:671201800:system_core:default:0:671201800";
        char arg5[] = "5";
        char arg6[] = "0123456789";
        char* argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv)/sizeof(argv[0]);
        EXPECT_EQ(-1, GetAppSpawnClientFromArg(argc, argv, &client));
        FreeHspList(client.property.hspList);
    }

    APPSPAWN_LOGI("App_Spawn_Standard_003_2 end");
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
            "default:671201800:system_core:default:0:671201800";
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
    int ret = DoStartApp((AppSpawnContent_*)content, &clientExt->client, (char*)"", 0);
    EXPECT_EQ(ret, 0);

    if (strcpy_s(clientExt->property.bundleName, APP_LEN_BUNDLE_NAME, "moduleTestProcessName") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 2" << std::endl;
    }
    ret = SetAppSandboxProperty(nullptr, nullptr);
    EXPECT_NE(ret, 0);
    ret = SetAppSandboxProperty((AppSpawnContent_*)content, &clientExt->client);
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
    SetParameter("const.appspawn.preload", "false");
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
    AddAppInfo(111, "111");
    AddAppInfo(65, "112");
    AddAppInfo(97, "113");

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
* @tc.desc: verify receive hspList
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
    { // no HspList
        bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)&param, sizeof(param));
        EXPECT_TRUE(ret);
    }
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08 end";
}

/**
* @tc.name: App_Spawn_Standard_08_1
* @tc.desc: receive AppParameter and HspList separately
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
    param.hspList = {sizeof(hspListStr), 0, nullptr};
    bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)&param, sizeof(param));
    EXPECT_FALSE(ret);

    // send HspList
    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr, sizeof(hspListStr));
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, strcmp(hspListStr, client.property.hspList.data));

    FreeHspList(client.property.hspList);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_1 end";
}

/**
* @tc.name: App_Spawn_Standard_08_2
* @tc.desc: receive AppParameter and HspList together
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_08_2, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_2 start";
    AppSpawnClientExt client = {};
    AppParameter param = {};

    // put AppParameter and HspList together
    char hspListStr[] = "01234";
    char buff[sizeof(param) + sizeof(hspListStr)];
    param.hspList = {sizeof(hspListStr), 0, nullptr};
    int res = memcpy_s(buff, sizeof(param), (void *)&param, sizeof(param));
    EXPECT_EQ(0, res);
    res = memcpy_s(buff + sizeof(param), sizeof(hspListStr), (void *)hspListStr, sizeof(hspListStr));
    EXPECT_EQ(0, res);

    // send
    bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)buff, sizeof(buff));
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, strcmp(hspListStr, client.property.hspList.data));

    FreeHspList(client.property.hspList);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_2 end";
}

/**
* @tc.name: App_Spawn_Standard_08_3
* @tc.desc: receive AppParameter and part of HspList
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

    // put AppParameter and part of HspList together
    char hspListStr[] = "0123456789";
    char buff[sizeof(param) + splitLen];
    param.hspList = {sizeof(hspListStr), 0, hspListStr};
    int res = memcpy_s(buff, sizeof(param), (void *)&param, sizeof(param));
    EXPECT_EQ(0, res);
    res = memcpy_s(buff + sizeof(param), splitLen, (void *)hspListStr, splitLen);
    EXPECT_EQ(0, res);

    // send AppParameter and part of HspList
    bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)buff, sizeof(buff));
    EXPECT_FALSE(ret);

    // send left HspList
    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr + splitLen, sizeof(hspListStr) - splitLen);
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, strcmp(hspListStr, client.property.hspList.data));

    FreeHspList(client.property.hspList);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08_3 end";
}

/**
* @tc.name: App_Spawn_Standard_08_4
* @tc.desc: receive AppParameter and splited HspList
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
    param.hspList = {sizeof(hspListStr), 0, nullptr};
    bool ret = ReceiveRequestData(nullptr, &client, (uint8_t *)&param, sizeof(param));
    EXPECT_FALSE(ret);

    // send splited HspList
    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr + sentLen, splitLen);
    sentLen += splitLen;
    EXPECT_FALSE(ret);

    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr + sentLen, splitLen);
    sentLen += splitLen;
    EXPECT_FALSE(ret);

    ret = ReceiveRequestData(nullptr, &client, (uint8_t *)hspListStr + sentLen, sizeof(hspListStr) - sentLen);
    EXPECT_TRUE(ret);
    EXPECT_EQ(0, strcmp(hspListStr, client.property.hspList.data));

    FreeHspList(client.property.hspList);
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
} // namespace OHOS
