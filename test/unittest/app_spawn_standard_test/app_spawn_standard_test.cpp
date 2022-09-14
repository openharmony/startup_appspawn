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

// redefine private and protected since testcase need to invoke and test private function
#define private public
#define protected public
#include "appspawn_service.h"
#undef private
#undef protected

#include "appspawn_adapter.h"
#include "appspawn_server.h"
#include "securec.h"
#include "json_utils.h"
#include "init_hashmap.h"
#include "loop_event.h"
#include "sandbox_utils.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;
using nlohmann::json;

#ifdef __cplusplus
    extern "C" {
#endif
int OnConnection(const LoopHandle loopHandle, const TaskHandle server);
void AddAppInfo(pid_t pid, const char *processName);
void OnReceiveRequest(const TaskHandle taskHandle, const uint8_t *buffer, uint32_t buffLen);
void ProcessTimer(const TimerHandle taskHandle, void *context);
void SignalHandler(const struct signalfd_siginfo *siginfo);
void SendMessageComplete(const TaskHandle taskHandle, BufferHandle handle);
TaskHandle GetTestClientHandle();
#ifdef __cplusplus
    }
#endif

namespace OHOS {
static void runAppSpawn(struct AppSpawnContent_ *content, int argc, char *const argv[])
{}

static int setAppSandbox(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    return 0;
}

static int setKeepCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    return 0;
}

static int setFileDescriptors(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    return 0;
}

static int setProcessName(struct AppSpawnContent_ *content, AppSpawnClient *client,
    char *longProcName, uint32_t longProcNameLen)
{
    return 0;
}

static int setUidGid(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    return 0;
}

static int setCapabilities(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    return 0;
}

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

/**
* @tc.name: App_Spawn_Standard_002
* @tc.desc: fork app son process and set content.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_002, TestSize.Level0)
{
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

    AppSpawnContent *content = AppSpawnCreateContent("AppSpawn", longProcName, longProcNameLen, 1);
    content->loadExtendLib = LoadExtendLib;
    content->runChildProcessor = RunChildProcessor;
    SetContentFunction(content);
    AppSandboxArg *sandboxArg = (AppSandboxArg *)malloc(sizeof(AppSandboxArg));
    sandboxArg->content = content;
    sandboxArg->client = &client->client;
    EXPECT_EQ(AppSpawnChild(sandboxArg), 0);
    free(sandboxArg);

    content->clearEnvironment(content, &client->client);
    EXPECT_EQ(content->setProcessName(content, &client->client, (char *)longProcName, longProcNameLen), 0);
    EXPECT_EQ(content->setKeepCapabilities(content, &client->client), 0);
    EXPECT_EQ(content->setUidGid(content, &client->client), 0);
    EXPECT_EQ(content->setCapabilities(content, &client->client), 0);
    EXPECT_EQ(content->setFileDescriptors(content, &client->client), 0);

    content->setAppSandbox(content, &client->client);
    content->setAppAccessToken(content, &client->client);
    EXPECT_EQ(content->coldStartApp(content, &client->client), 0);

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
    char arg1[] = "xxx";
    char arg2[] = "xxx";
    char* argv[] = {arg1, arg2};
    int argc = sizeof(argv)/sizeof(argv[0]);

    EXPECT_EQ(GetAppSpawnClientFromArg(argc, argv, client), -1);
    free(client);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_003 end";
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
    char tmp3[] = "1:1:1:1:0:ohos.samples.ecg.default:ohos.samples.ecg:default:671201800:system_core:default";
    char * const argv[] = {tmp0, tmp1, tmp2, tmp3};

    AppSpawnColdRun(content, 4, argv);
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
    content->setAppSandbox = setAppSandbox;
    content->setKeepCapabilities = setKeepCapabilities;
    content->setProcessName = setProcessName;
    content->setUidGid = setUidGid;
    content->setFileDescriptors = setFileDescriptors;
    content->setCapabilities = setCapabilities;

    int ret = DoStartApp((AppSpawnContent_*)content, &clientExt->client, (char*)"", 0);
    EXPECT_EQ(ret, 0);

    free(content);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_005 end";
}

static AppSpawnContentExt *TestClient(int cold, AppOperateType code, const std::string &processName)
{
    char buffer[64] = {0}; // 64 buffer size
    AppSpawnContentExt *content =
        (AppSpawnContentExt *)AppSpawnCreateContent("AppSpawnTest009", buffer, sizeof(buffer), cold);
    if (content == nullptr) {
        return nullptr;
    }
    APPSPAWN_CHECK(content->content.initAppSpawn != nullptr, return nullptr, "Invalid content for appspawn");
    APPSPAWN_CHECK(content->content.runAppSpawn != nullptr, return nullptr, "Invalid content for appspawn");
    // set common operation
    content->content.loadExtendLib = LoadExtendLib;
    content->content.runChildProcessor = RunChildProcessor;
    content->content.initAppSpawn(&content->content);

    // create connection
    OnConnection(LE_GetDefaultLoop(), content->server);

    // process recv message
    if (GetTestClientHandle() == nullptr) {
        free(content);
        return nullptr;
    }

    AppParameter property = {};
    property.uid = 100; // 100 is uid
    property.gid = 100; // 100 is gid
    property.gidCount = 1; // 1 is gidCount
    property.gidTable[0] = 101; // 101 is gidTable
    (void)strcpy_s(property.processName, sizeof(property.processName), processName.c_str());
    (void)strcpy_s(property.bundleName, sizeof(property.bundleName), processName.c_str());
    (void)strcpy_s(property.renderCmd, sizeof(property.renderCmd), processName.c_str());
    (void)strcpy_s(property.soPath, sizeof(property.soPath), processName.c_str());
    (void)strcpy_s(property.apl, sizeof(property.apl), "system_core");
    property.flags = 0;
    property.code = code;
    property.accessTokenId = 0;
    OnReceiveRequest(GetTestClientHandle(), (const uint8_t *)&property, sizeof(property));

    SendMessageComplete(GetTestClientHandle(), nullptr);
    LE_CloseTask(LE_GetDefaultLoop(), GetTestClientHandle());
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
    AppSpawnContentExt *content = TestClient(0, DEFAULT, "ohos.test.testapp");
    EXPECT_TRUE(content != nullptr);
    content->content.runAppSpawn(&content->content, 0, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_006 end";
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
    AppSpawnContentExt *content = TestClient(0, DEFAULT, "ohos.test.testapp");
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

    ProcessTimer(nullptr, nullptr);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_07 end";
}

/**
* @tc.name: App_Spawn_Standard_005
* @tc.desc: load system config SetAppSandboxProperty by App ohos.samples.ecg.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_08, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08 start";
    AppSpawnClientExt* client = (AppSpawnClientExt*)malloc(sizeof(AppSpawnClientExt));

    ClientSocket::AppProperty *m_appProperty = nullptr;
    m_appProperty = &client->property;

    m_appProperty->uid = 1000; // the UNIX uid that the child process setuid() to after fork()
    m_appProperty->gid = 1000; // the UNIX gid that the child process setgid() to after fork()
    m_appProperty->gidCount = 1;

    if (strcpy_s(m_appProperty->processName, APP_LEN_PROC_NAME, "ohos.samples.ecg") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 1" << std::endl;
    }

    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "ohos.samples.ecg") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 2" << std::endl;
    }

    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "normal") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 3" << std::endl;
    }

    GTEST_LOG_(INFO) << "SetAppSandboxProperty section 2"  << std::endl;

    m_appProperty->accessTokenId = 671201800; // 671201800 is accessTokenId
    m_appProperty->pid = 354; // query render process exited status by render process pid

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(m_appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_08 end";
}

/**
* @tc.name: App_Spawn_Standard_09
* @tc.desc: load system config SetAppSandboxProperty by App com.ohos.dlpmanager.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_09, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_09 start";
    AppSpawnClientExt* client = (AppSpawnClientExt*)malloc(sizeof(AppSpawnClientExt));

    ClientSocket::AppProperty *m_appProperty = nullptr;
    m_appProperty = &client->property;

    m_appProperty->uid = 1000; // the UNIX uid that the child process setuid() to after fork()
    m_appProperty->gid = 1000; // the UNIX gid that the child process setgid() to after fork()
    m_appProperty->gidCount = 1;

    if (strcpy_s(m_appProperty->processName, APP_LEN_PROC_NAME, "com.ohos.dlpmanager") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 1" << std::endl;
    }

    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "com.ohos.dlpmanager") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 2" << std::endl;
    }

    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "normal") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 3" << std::endl;
    }

    GTEST_LOG_(INFO) << "SetAppSandboxProperty section 2"  << std::endl;
    m_appProperty->accessTokenId = 671201800; // 671201800 is accessTokenId
    m_appProperty->pid = 354; // query render process exited status by render process pid

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(m_appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_09 end";
}

/**
* @tc.name: App_Spawn_Standard_10
* @tc.desc: parse config define by self Set App Sandbox Property.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_10, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_10 start";
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"top-sandbox-switch\": \"ON\", \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/config\", \
                    \"sandbox-path\" : \"/config\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\", \
                    \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \" \
                }], \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\": [] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);

    GTEST_LOG_(INFO) << "SetAppSandboxProperty start" << std::endl;
    AppSpawnClientExt* client = (AppSpawnClientExt*)malloc(sizeof(AppSpawnClientExt));

    ClientSocket::AppProperty *m_appProperty = nullptr;
    m_appProperty = &client->property;

    m_appProperty->uid = 1000; // the UNIX uid that the child process setuid() to after fork()
    m_appProperty->gid = 1000; // the UNIX gid that the child process setgid() to after fork()

    if (strcpy_s(m_appProperty->processName, APP_LEN_PROC_NAME, "test.appspawn") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 1" << std::endl;
    }
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "test.bundle.name") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 2" << std::endl;
    }
    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "normal") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 3" << std::endl;
    }

    GTEST_LOG_(INFO) << "SetAppSandboxProperty section 2"  << std::endl;
    m_appProperty->accessTokenId = 671201800; // 671201800 is accessTokenId
    m_appProperty->pid = 354; // query render process exited status by render process pid

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(m_appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_10 end";
}

/**
* @tc.name: App_Spawn_Standard_11
* @tc.desc: parse namespace config define by self set app sandbox property.
* @tc.type: FUNC
* @tc.require: issueI5OE8Q
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_011, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_011 start";
    std::string namespaceJsconfig = "{ \
        \"sandbox-namespace\":[{ \
            \"com.ohos.app1\":[{ \
                \"clone-flags\": [ \"mnt\" ] \
            }], \
            \"com.ohos.app2\":[{ \
                \"clone-flags\": [ \"pid\" ] \
            }],\
            \"com.ohos.app3\":[{ \
                \"clone-flags\": [ \"mnt\", \"pid\" ] \
            }] \
        }] \
    }";
    nlohmann::json namespace_config = nlohmann::json::parse(namespaceJsconfig.c_str());

    OHOS::AppSpawn::SandboxUtils::StoreNamespaceJsonConfig(namespace_config);
    uint32_t cloneFlags = GetAppNamespaceFlags("com.ohos.app1");
    EXPECT_TRUE(cloneFlags & CLONE_NEWNS);

    cloneFlags = GetAppNamespaceFlags("com.ohos.app2");
    EXPECT_TRUE(cloneFlags & CLONE_NEWNS);
    EXPECT_TRUE(cloneFlags & CLONE_NEWPID);

    cloneFlags = GetAppNamespaceFlags("com.ohos.app3");
    EXPECT_TRUE(cloneFlags & CLONE_NEWNS);
    EXPECT_TRUE(cloneFlags & CLONE_NEWPID);

    cloneFlags = GetAppNamespaceFlags("com.ohos.app4");
    EXPECT_TRUE(cloneFlags & CLONE_NEWNS);

    GTEST_LOG_(INFO) << "App_Spawn_Standard_011 end";
}


/**
* @tc.name: App_Spawn_Standard_012
* @tc.desc: Create an application process parameter check.
* @tc.type: FUNC
* @tc.require: issueI5OE8Q
* @tc.author:
*/
HWTEST(AppSpawnStandardTest, App_Spawn_Standard_012, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Standard_012 start";
    AppSpawnContent_ appContent = {0};
    AppSpawnClient appClient = {0};
    pid_t pid = -1;
    AppSandboxArg *sandboxArg = (AppSandboxArg *)malloc(sizeof(AppSandboxArg));
    EXPECT_TRUE(sandboxArg != nullptr);
    (void)memset_s(sandboxArg, sizeof(AppSandboxArg), 0, sizeof(AppSandboxArg));
    int ret = AppSpawnProcessMsg(sandboxArg, &pid);
    EXPECT_TRUE(ret < 0);
    sandboxArg->content = &appContent;
    ret = AppSpawnProcessMsg(sandboxArg, &pid);
    EXPECT_TRUE(ret < 0);
    sandboxArg->client = &appClient;
    ret = AppSpawnProcessMsg(sandboxArg, NULL);
    EXPECT_TRUE(ret < 0);
    free(sandboxArg);
    GTEST_LOG_(INFO) << "App_Spawn_Standard_012 end";
}
} // namespace OHOS
