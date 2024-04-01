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

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;
using nlohmann::json;

namespace OHOS {
class AppSpawnSandboxTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppSpawnSandboxTest::SetUpTestCase()
{}

void AppSpawnSandboxTest::TearDownTestCase()
{}

void AppSpawnSandboxTest::SetUp()
{}

void AppSpawnSandboxTest::TearDown()
{}

static AppSpawnClientExt *GetAppSpawnClientExt(void)
{
    static AppSpawnClientExt client;
    return &client;
}

static ClientSocket::AppProperty *GetAppProperty(void)
{
    return &GetAppSpawnClientExt()->property;
}

static AppSpawnClient *GetAppSpawnClient(void)
{
    return &GetAppSpawnClientExt()->client;
}

/**
* @tc.name: App_Spawn_Sandbox_005
* @tc.desc: load system config SetAppSandboxProperty by App ohos.samples.ecg.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_08, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_08 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

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

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_08 end";
}

/**
* @tc.name: App_Spawn_Sandbox_09
* @tc.desc: load system config SetAppSandboxProperty by App com.ohos.dlpmanager.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_09, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

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

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09 end";
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_09_1, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

    m_appProperty->uid = 1000; // the UNIX uid that the child process setuid() to after fork()
    m_appProperty->gid = 1000; // the UNIX gid that the child process setgid() to after fork()
    m_appProperty->gidCount = 1;

    if (strcpy_s(m_appProperty->processName, APP_LEN_PROC_NAME, "com.ohos.dlpmanager") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 1" << std::endl;
    }

    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "normal") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 3" << std::endl;
    }

    GTEST_LOG_(INFO) << "SetAppSandboxProperty section 2"  << std::endl;
    m_appProperty->accessTokenId = 671201800; // 671201800 is accessTokenId
    m_appProperty->pid = 354; // query render process exited status by render process pid

    int ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(nullptr);
    EXPECT_NE(ret, 0);
    m_appProperty->bundleName[0] = '\0';
    ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    EXPECT_NE(ret, 0);
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "com.\\ohos.dlpmanager") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 2" << std::endl;
    }
    ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    EXPECT_NE(ret, 0);
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "com./ohos.dlpmanager") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 2" << std::endl;
    }
    ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    EXPECT_NE(ret, 0);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 end";
}

/**
* @tc.name: App_Spawn_Sandbox_10
* @tc.desc: parse config define by self Set App Sandbox Property.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_10, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_10 start";
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"top-sandbox-switch\": \"ON\", \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
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
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

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

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_10 end";
}

/**
* @tc.name: App_Spawn_Sandbox_012
* @tc.desc: Create an application process parameter check.
* @tc.type: FUNC
* @tc.require: issueI5OE8Q
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_012, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_012 start";
    AppSpawnContent appContent = {0};
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
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_012 end";
}

/**
* @tc.name: App_Spawn_Sandbox_13
* @tc.desc: parse config define by self Set App Sandbox Property.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_13, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_13 start";
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/config\", \
                    \"sandbox-path\" : \"\", \
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
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

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

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_13 end";
}

/**
* @tc.name: App_Spawn_Sandbox_14
* @tc.desc: parse sandbox config without sandbox-root label
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_14, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_14 start";
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/config\", \
                    \"sandbox-path\" : \"\", \
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
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

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

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_14 end";
}

/**
* @tc.name: App_Spawn_Sandbox_15
* @tc.desc: parse app sandbox config with PackageName_index label
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_15, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_15 start";
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName_index>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/config\", \
                    \"sandbox-path\" : \"\", \
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
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

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

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_15 end";
}

/**
* @tc.name: App_Spawn_Sandbox_16
* @tc.desc: parse config define by self Set App Sandbox Property.
* @tc.type: FUNC
* @tc.require:issueI5NTX6
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_16, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_16 start";
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\": [] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);

    GTEST_LOG_(INFO) << "SetAppSandboxProperty start" << std::endl;
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

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

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_16 end";
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_17, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09 start";
    nlohmann::json appSandboxConfig;
    bool rc = JsonUtils::GetJsonObjFromJson(appSandboxConfig, "");
    EXPECT_FALSE(rc);
    std::string path(256, 'w'); // 256 test
    rc = JsonUtils::GetJsonObjFromJson(appSandboxConfig, path);
    EXPECT_FALSE(rc);

    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\": [] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);

    std::string value;
    rc = JsonUtils::GetStringFromJson(j_config, "common", value);
    EXPECT_FALSE(rc);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09 end";
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_18, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_18 start";
    std::string mJsconfig1 = "{ \
        \"sandbox-switch\": \"ON\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    bool ret = OHOS::AppSpawn::SandboxUtils::GetSbxSwitchStatusByConfig(j_config1);
    EXPECT_TRUE(ret);

    std::string mJsconfig2 = "{ \
        \"sandbox-switch\": \"OFF\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
    }";
    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::GetSbxSwitchStatusByConfig(j_config2);
    EXPECT_FALSE(ret);

    std::string mJsconfig3 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
    }";
    nlohmann::json j_config3 = nlohmann::json::parse(mJsconfig3.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::GetSbxSwitchStatusByConfig(j_config3);
    EXPECT_TRUE(ret);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_19, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_19 start";
    int ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(nullptr);
    EXPECT_EQ(ret, -1);

    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    ret = memset_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, 0, APP_LEN_BUNDLE_NAME);
    if (ret != 0) {
        GTEST_LOG_(ERROR) << "Failed to memset_s err=" << errno;
        ASSERT_TRUE(0);
    }
    ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    EXPECT_EQ(ret, -1);

    ret = strncpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "\\test", APP_LEN_BUNDLE_NAME - 1);
    if (ret != 0) {
        GTEST_LOG_(ERROR) << "Failed to strncpy_s err=" << errno;
        ASSERT_TRUE(0);
    }
    ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    EXPECT_EQ(ret, -1);

    ret = strncpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "/test", APP_LEN_BUNDLE_NAME - 1);
    if (ret != 0) {
        GTEST_LOG_(ERROR) << "Failed to strncpy_s err=" << errno;
        ASSERT_TRUE(0);
    }
    ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
    EXPECT_EQ(ret, -1);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_20, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_20 start";
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"top-sandbox-switch\": \"OFF\", \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\": [] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

    m_appProperty->uid = 1000; // the UNIX uid that the child process setuid() to after fork()
    m_appProperty->gid = 1000; // the UNIX gid that the child process setgid() to after fork()

    if (strcpy_s(m_appProperty->processName, APP_LEN_PROC_NAME, "test.appspawn") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "test.bundle.name") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "normal") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());

    std::string mJsconfig1 = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\": [{ \
            \"test.bundle.name\" : [{ \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
            }] \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config1);
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(GetAppSpawnClient());
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_21, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_21 start";

    bool ret = OHOS::AppSpawn::SandboxUtils::CheckBundleNameForPrivate(std::string("__internal__"));
    EXPECT_FALSE(ret);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_22, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_22 start";
    std::string mJsconfig1 = "{ \
        \"common\":[], \
        \"individual\": [] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config1);

    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    int ret = strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "system_basic");
    if (ret != 0) {
        GTEST_LOG_(ERROR) << "Failed to strcpy_s err=" << errno;
        ASSERT_TRUE(0);
    }
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    ret = OHOS::AppSpawn::SandboxUtils::SetCommonAppSandboxProperty(m_appProperty,
            testBundle);
    EXPECT_EQ(ret, 0);

    if (memset_s(m_appProperty->apl, APP_APL_MAX_LEN, 0, APP_APL_MAX_LEN) != 0) {
        GTEST_LOG_(ERROR) << "Failed to memset_s err=" << errno;
        ASSERT_TRUE(0);
    }
    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "system_core") != 0) {
        GTEST_LOG_(ERROR) << "Failed to strcpy_s err=" << errno;
        ASSERT_TRUE(0);
    }
    ret = OHOS::AppSpawn::SandboxUtils::SetCommonAppSandboxProperty(m_appProperty,
            testBundle);
    EXPECT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_23, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_23 start";
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    int ret = OHOS::AppSpawn::SandboxUtils::SetRenderSandboxProperty(nullptr,
            testBundle);
    EXPECT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_24, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_24 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "test.bundle.name1") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    m_appProperty->bundleIndex = 1;
    std::string mJsconfig1 = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"symbol-links\" : [{ \
                    \"target-name\" : \"/system/etc\", \
                    \"link-name\" : \"/etc\", \
                    \"check-action-status\": \"false\" \
                }, { \
                    \"target\" : \"/system/etc\", \
                    \"link\" : \"/etc\", \
                    \"check\": \"false\" \
                }] \
            }], \
            \"app-resources\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [], \
                \"symbol-links\" : [] \
            }] \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonSymlink(m_appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig2 = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"symbol-links\" : [{ \
                    \"target-name\" : \"/data/test123\", \
                    \"link-name\" : \"/test123\", \
                    \"check-action-status\": \"true\" \
                }] \
            }] \
        }] \
    }";
    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonSymlink(m_appProperty, j_config2);
    EXPECT_NE(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_25, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_25 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "test.bundle.wps") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    m_appProperty->flags = 4; // 4 is test parameter
    std::string mJsconfig1 = "{ \
        \"common\":[{ \
            \"app-resources\" : [{ \
                \"flags\": \"DLP_MANAGER\", \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                    \"sandbox-path\" : \"/data/storage/el2/base\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\", \
                    \"app-apl-name\": \"test123\", \
                    \"fs-type\": \"ext4\" \
                }, { \
                    \"src\" : \"/data/app/el1/<currentUserId>/database/<PackageName_index>\", \
                    \"sandbox-path\" : \"/data/storage/el1/database\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\" \
                }], \
                \"symbol-links\" : [] \
            }] \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonBind(m_appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonFlagsPointHandle(m_appProperty, j_config1);
    EXPECT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_26, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_26 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    std::string mJsconfig2 = "{ \
        \"common\":[{ \
            \"app-base\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                    \"sandbox-path\" : \"/data/storage/el2/base\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"fs-type\" : \"ext4\", \
                    \"check-action-status\": \"true\" \
                }] \
            }] \
        }] \
    }";
    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    if (memset_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, 0, APP_LEN_BUNDLE_NAME) != 0) {
        GTEST_LOG_(INFO) << "Failed to memset_s err=" << errno << std::endl;
    }
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "com.ohos.dlpmanager") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonBind(m_appProperty, j_config2);
    EXPECT_NE(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_27, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_27 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "test.bundle.name") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    std::string mJsconfig1 = "{ \
        \"individual\": [{ \
            \"test.bundle.name\" : [{ \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
            }] \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(m_appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig2 = "{ \
        \"individual\": [{ \
            \"test.bundle.name\" : [{ \
                \"flags-point\" : [{ \
                    \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                    \"mount-paths\" : [{ \
                        \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                        \"sandbox-path\" : \"/data/storage/el2/base\", \
                        \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                        \"check-action-status\": \"false\" \
                    }]\
                }], \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
            }] \
        }] \
    }";
    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(m_appProperty, j_config2);
    EXPECT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_28, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_28 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "test.bundle.name") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    m_appProperty->flags = 4;
    std::string mJsconfig3 = "{ \
        \"individual\": [{ \
            \"test.bundle.name\" : [{ \
                \"flags-point\" : [{ \
                    \"flags\": \"DLP_MANAGER\", \
                    \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                    \"mount-paths\" : [{ \
                        \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                        \"sandbox-path\" : \"/data/storage/el2/base\", \
                        \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                        \"check-action-status\": \"false\" \
                    }] \
                }], \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
            }] \
        }] \
    }";
    nlohmann::json j_config3 = nlohmann::json::parse(mJsconfig3.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(m_appProperty, j_config3);
    EXPECT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_29, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_29 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "test.bundle.name") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    m_appProperty->flags = 4;
    std::string mJsconfig3 = "{ \
        \"individual\": [{ \
            \"test.bundle.name\" : [{ \
                \"flags-point\" : [{ \
                    \"flags\": \"DLP_MANAGER\", \
                    \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                    \"mount-paths\" : [{ \
                        \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                        \"sandbox-path\" : \"/data/storage/el2/base\", \
                        \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                        \"check-action-status\": \"false\" \
                    }] \
                }], \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
            }] \
        }] \
    }";
    nlohmann::json j_config3 = nlohmann::json::parse(mJsconfig3.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(m_appProperty, j_config3);
    EXPECT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_30, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_30 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();

    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "apl123") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }

    std::string mJsconfig3 = "{ \
        \"flags\": \"DLP_MANAGER\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/app/el2/<currentUserId>/base/database/<PackageName>\", \
            \"sandbox-path\" : \"/data/storage/el2/base\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\" \
        }] \
    }";
    nlohmann::json j_config3 = nlohmann::json::parse(mJsconfig3.c_str());
    m_appProperty->flags = 4;
    if (memset_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, 0, APP_LEN_BUNDLE_NAME) != 0) {
        GTEST_LOG_(INFO) << "Failed to memset_s err=" << errno << std::endl;
    }
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "com.ohos.wps") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    int ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(m_appProperty, j_config3);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig4 = "{ \
        \"flags\": \"DLP_MANAGER\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/app/el2/<currentUserId>/database/<PackageName>\", \
            \"sandbox-path\" : \"/data/storage/el2/base\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\" \
        }] \
    }";
    nlohmann::json j_config4 = nlohmann::json::parse(mJsconfig4.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(m_appProperty, j_config4);
    EXPECT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_31, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_31 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "com.ohos.dlpmanager") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    std::string mJsconfig1 = "{ \
        \"flags\": \"DLP_TEST\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
            \"sandbox-path\" : \"/data/storage/el2/base\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\" \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(m_appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig2 = "{ \
        \"flags\": \"DLP_TEST\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
            \"sandbox-path\" : \"/data/storage/el2/base\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"fs-type\" : \"ext4\", \
            \"check-action-status\": \"false\", \
            \"app-apl-name\" : \"apl123\" \
        }, { \
            \"src-path\" : \"/data/app/el2/<currentUserId>/database/<PackageName_index>\", \
            \"sandbox-path\" : \"/data/storage/el2/base\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"fs-type\" : \"ext4\", \
            \"check-action-status\": \"true\" \
        }] \
    }";

    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "apl123") != 0) {
        GTEST_LOG_(INFO) << "Failed to strcpy_s err=" << errno << std::endl;
    }
    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(m_appProperty, j_config2);
    EXPECT_TRUE(ret != 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_32, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_32 start";
    int ret = OHOS::AppSpawn::SandboxUtils::DoAppSandboxMountOnce(nullptr, "", nullptr, 0, nullptr);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig1 = "{ \
        \"dest-mode\" : \"S_IRUSR|S_IWUSR|S_IXUSR\" \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    std::string sandboxRoot;
    const char *str = "/data/test11122";
    sandboxRoot = str;
    OHOS::AppSpawn::SandboxUtils::DoSandboxChmod(j_config1, sandboxRoot);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_34, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_34 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;

    { // totalLength is 0
        m_appProperty->extraInfo = {};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_EQ(0, ret);
    }
    { // data is nullptr
        m_appProperty->extraInfo = {1, 0, nullptr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_EQ(0, ret);
    }
    { // success
        char hspListStr[] = "|HspList|{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }|HspList|";
        m_appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_EQ(0, ret);
    }

    m_appProperty->extraInfo = {};
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_34 end";
}

static void InvalidJsonTest(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    char hspListStr[] = "|HspList|{|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

static void NoBundleTest(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    char hspListStr[] = "|HspList|{ \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

static void NoModulesTest(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    char hspListStr[] = "|HspList|{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

static void NoVersionsTest(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    char hspListStr[] = "|HspList|{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"] \
    }|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

static void ListSizeNotSameTest(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    char hspListStr[] = "|HspList|{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

static void ValueTypeIsNotArraryTest(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    char hspListStr[] = "|HspList|{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": 1001 \
    }|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

static void ElementTypeIsNotStringTest(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    char hspListStr[] = "|HspList|{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": [1001, 1002] \
    }|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

static void ElementTypeIsNotSameTestSN(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    // element type is not same, string + number
    char hspListStr[] = "|HspList|{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": [\"v10001\", 1002] \
    }|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

static void ElementTypeIsNotSameTestNS(ClientSocket::AppProperty* appProperty, std::string &testBundle)
{
    // element type is not same, number + string
    char hspListStr[] = "|HspList|{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": [1001, \"v10002\"] \
    }|HspList|";
    appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_35, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_35 start";
    ClientSocket::AppProperty *appProperty = GetAppProperty();
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    InvalidJsonTest(appProperty, testBundle);
    NoBundleTest(appProperty, testBundle);
    NoModulesTest(appProperty, testBundle);
    NoVersionsTest(appProperty, testBundle);
    ListSizeNotSameTest(appProperty, testBundle);
    ValueTypeIsNotArraryTest(appProperty, testBundle);
    ElementTypeIsNotStringTest(appProperty, testBundle);
    ElementTypeIsNotSameTestSN(appProperty, testBundle);
    ElementTypeIsNotSameTestNS(appProperty, testBundle);
    appProperty->extraInfo = {};
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_35 end";
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_36, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_36 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;

    { // empty name
        char hspListStr[] = "|HspList|{ \
            \"bundles\":[\"\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }|HspList|";
        m_appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // name is .
        char hspListStr[] = "|HspList|{ \
            \"bundles\":[\".\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }|HspList|";
        m_appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // name is ..
        char hspListStr[] = "|HspList|{ \
            \"bundles\":[\"..\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }|HspList|";
        m_appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // name contains /
        char hspListStr[] = "|HspList|{ \
            \"bundles\":[\"test/bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }|HspList|";
        m_appProperty->extraInfo = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }

    m_appProperty->extraInfo = {};
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_36 end";
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_37, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Sandbox_37 start");
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    m_appProperty->uid = 1000;
    m_appProperty->gid = 1000;
    (void)strcpy_s(m_appProperty->bundleName, sizeof(m_appProperty->bundleName), "ohos.samples.xxx");
    AppSpawnContent content;
    LoadAppSandboxConfig(&content);
    std::string sandboxPackagePath = "/mnt/sandbox/100/";
    const std::string bundleName = m_appProperty->bundleName;
    sandboxPackagePath += bundleName;

    int ret = SandboxUtils::SetPrivateAppSandboxProperty(m_appProperty);
    EXPECT_EQ(0, ret);
    ret = SandboxUtils::SetCommonAppSandboxProperty(m_appProperty, sandboxPackagePath);
    EXPECT_EQ(0, ret);
    APPSPAWN_LOGI("App_Spawn_Sandbox_37 end");
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_38, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Sandbox_38 start");
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    m_appProperty->uid = 1000;
    m_appProperty->gid = 1000;

    (void)strcpy_s(m_appProperty->bundleName, sizeof(m_appProperty->bundleName), "com.example.deviceinfo");
    std::string pJsconfig1 = "{ \
        \"common\":[],                      \
        \"individual\": [ {                  \
            \"com.example.deviceinfo\" : [{   \
            \"sandbox-switch\": \"ON\",     \
            \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\",  \
            \"mount-paths\" : [{    \
                    \"src-path\" : \"/data/app/el1/bundle/public/\",    \
                    \"sandbox-path\" : \"/data/accounts/account_0/applications/2222222\",   \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ],  \
                    \"check-action-status\": \"true\"   \
                }, { \
                    \"src-path\" : \"/data/app/el1/bundle/public/\",    \
                    \"sandbox-path\" : \"/data/bundles/aaaaaa\",    \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ],  \
                    \"check-action-status\": \"true\"   \
                }\
            ],\
            \"symbol-links\" : []   \
        }] \
        }] \
    }";
    try {
        nlohmann::json p_config1 = nlohmann::json::parse(pJsconfig1.c_str());
        OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(p_config1);
    } catch (nlohmann::detail::exception& e) {
        APPSPAWN_LOGE("App_Spawn_Sandbox_38 Invalid json");
        EXPECT_EQ(0, 1);
    }
    std::string sandboxPackagePath = "/mnt/sandbox/100/";
    const std::string bundleName = m_appProperty->bundleName;
    sandboxPackagePath += bundleName;
    int ret = SandboxUtils::SetPrivateAppSandboxProperty(m_appProperty);
    EXPECT_EQ(0, ret);
    ret = SandboxUtils::SetCommonAppSandboxProperty(m_appProperty, sandboxPackagePath);
    EXPECT_EQ(0, ret);
    APPSPAWN_LOGI("App_Spawn_Sandbox_38 end");
}

/**
* @tc.name: App_Spawn_Sandbox_39
* @tc.desc: load overlay config SetAppSandboxProperty by App com.ohos.demo.
* @tc.type: FUNC
* @tc.require:issueI7D0H9
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_39, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Sandbox_39 start");
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    m_appProperty->uid = 1000;
    m_appProperty->gid = 1000;
    m_appProperty->gidCount = 1;
    m_appProperty->flags |= 0x100;
    m_appProperty->extraInfo.totalLength = 55;
    string overlayInfo = "|Overlay|/data/app/el1/bundle/public/com.ohos.demo/feature.hsp|";
    overlayInfo+="/data/app/el1/bundle/public/com.ohos.demo/feature.hsp||Overlay|";
    m_appProperty->extraInfo.data = new char[overlayInfo.length() + 1];
    if (strcpy_s(m_appProperty->extraInfo.data, overlayInfo.length() + 1, overlayInfo.c_str()) != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 1" << std::endl;
    }
    std::string sandBoxRootDir = "/mnt/sandbox/100/com.ohos.demo";

    if (strcpy_s(m_appProperty->processName, APP_LEN_PROC_NAME, "com.ohos.demo") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 2" << std::endl;
    }

    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "com.ohos.demo") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 3" << std::endl;
    }

    if (strcpy_s(m_appProperty->apl, APP_APL_MAX_LEN, "normal") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty start 4" << std::endl;
    }

    GTEST_LOG_(INFO) << "SetAppSandboxProperty section 2"  << std::endl;
    m_appProperty->accessTokenId = 671201800; // 671201800 is accessTokenId
    m_appProperty->pid = 354; // query render process exited status by render process pid

    int32_t ret = OHOS::AppSpawn::SandboxUtils::SetOverlayAppSandboxProperty(m_appProperty, sandBoxRootDir);
    EXPECT_EQ(0, ret);
    m_appProperty->flags &= ~0x100;
    m_appProperty->extraInfo.totalLength = 0;
    if (m_appProperty->extraInfo.data != nullptr) {
        delete [] m_appProperty->extraInfo.data;
    }
    m_appProperty->extraInfo = {};

    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_39 end";
}
/**
* @tc.name: App_Spawn_Sandbox_40
* @tc.desc: load group info config SetAppSandboxProperty
* @tc.type: FUNC
* @tc.require:issueI7FUPV
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_40, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_40 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    m_appProperty->uid = 1100;
    m_appProperty->gid = 1100;
    m_appProperty->gidCount = 2;
    m_appProperty->flags |= 0x100;
    std::string sandboxPrefix = "/mnt/sandbox/100/testBundle";

    if (strcpy_s(m_appProperty->bundleName, APP_LEN_BUNDLE_NAME, "testBundle") != 0) {
        GTEST_LOG_(INFO) << "SetAppSandboxProperty set bundleName" << std::endl;
    }
    { // totalLength is 0
        m_appProperty->extraInfo = {};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllGroup(m_appProperty, sandboxPrefix);
        EXPECT_EQ(0, ret);
    }
    { // data is nullptr
        m_appProperty->extraInfo = {1, 0, nullptr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllGroup(m_appProperty, sandboxPrefix);
        EXPECT_EQ(0, ret);
    }
    { // success
        char dataGroupInfoListStr[] = "|DataGroup|{ \
            \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
            \"dir\":[\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\", \
                     \"/data/app/el2/100/group/ce876162-fe69-45d3-aa8e-411a047af564\"], \
            \"gid\":[\"20100001\", \"20100002\"] \
        }|DataGroup|";
        m_appProperty->extraInfo = {strlen(dataGroupInfoListStr), 0, dataGroupInfoListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllGroup(m_appProperty, sandboxPrefix);
        EXPECT_EQ(0, ret);
    }
    m_appProperty->extraInfo = {};
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_40 end";
}

/**
* @tc.name: App_Spawn_Sandbox_41
* @tc.desc: parse namespace config.
* @tc.type: FUNC
* @tc.require:issueI8B63M
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_41, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_41 start";
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"top-sandbox-switch\": \"ON\", \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"sandbox-ns-flags\": [ \"pid\" ], \
                \"mount-paths\" : [], \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\":[{ \
            \"__internal__.com.ohos.render\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/com.ohos.render/<PackageName>\", \
                \"sandbox-ns-flags\": [ \"pid\", \"net\" ], \
                \"mount-paths\" : [], \
                \"symbol-links\" : [] \
            }] \
        }] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);

    uint32_t cloneFlags = OHOS::AppSpawn::SandboxUtils::GetSandboxNsFlags(false);
    EXPECT_EQ(!!(cloneFlags & CLONE_NEWPID), true);

    cloneFlags = OHOS::AppSpawn::SandboxUtils::GetSandboxNsFlags(true);
    EXPECT_EQ(!!(cloneFlags & (CLONE_NEWPID | CLONE_NEWNET)), true);

    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_41 end";
}

/**
* @tc.name: App_Spawn_Sandbox_42
* @tc.desc: parse config file for fstype .
* @tc.type: FUNC
* @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_42, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_42 start";
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"/data/app/el1/<currentUserId>/base\", \
            \"sandbox-path\": \"/storage/Users/<currentUserId>/appdata/el1\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dac-override-sensitive\": \"true\", \
            \"fs-type\": \"sharefs\", \
            \"options\": \"support_overwrite=1\" \
        }] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());
    const char *mountPath = "mount-paths";
    nlohmann::json j_secondConfig = j_config[mountPath][0];
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);

    std::string fsType = OHOS::AppSpawn::SandboxUtils::GetSandboxFsType(j_secondConfig);
    int ret = strcmp(fsType.c_str(), "sharefs");
    if (SandboxUtils::deviceTypeEnable_ == true) {
        EXPECT_EQ(ret, 0);
    } else {
        EXPECT_NE(ret, 0);
    }

    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_42 end";
}

/**
* @tc.name: App_Spawn_Sandbox_43
* @tc.desc: get sandbox mount config when section is common.
* @tc.type: FUNC
* @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_43, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_43 start";
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"/data/app/el1/<currentUserId>/base\", \
            \"sandbox-path\": \"/storage/Users/<currentUserId>/appdata/el1\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dac-override-sensitive\": \"true\", \
            \"fs-type\": \"sharefs\", \
            \"options\": \"support_overwrite=1\" \
        }] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());
    const char *mountPath = "mount-paths";
    nlohmann::json j_secondConfig = j_config[mountPath][0];

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);
    OHOS::AppSpawn::SandboxUtils::SandboxMountConfig mountConfig;
    std::string section = "common";
    OHOS::AppSpawn::SandboxUtils::GetSandboxMountConfig(section, j_secondConfig, mountConfig);
    int ret = strcmp(mountConfig.fsType.c_str(), "sharefs");
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_43 end";
}

/**
* @tc.name: App_Spawn_Sandbox_44
* @tc.desc: get sandbox mount config when section is permission.
* @tc.type: FUNC
* @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_44, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_44 start";
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"/data/app/el1/<currentUserId>/base\", \
            \"sandbox-path\": \"/storage/Users/<currentUserId>/appdata/el1\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dac-override-sensitive\": \"true\", \
            \"fs-type\": \"sharefs\", \
            \"options\": \"support_overwrite=1\" \
        }] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());
    const char *mountPath = "mount-paths";
    nlohmann::json j_secondConfig = j_config[mountPath][0];

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);
    OHOS::AppSpawn::SandboxUtils::SandboxMountConfig mountConfig;
    std::string section = "permission";
    OHOS::AppSpawn::SandboxUtils::GetSandboxMountConfig(section, j_secondConfig, mountConfig);
    int ret = strcmp(mountConfig.fsType.c_str(), "sharefs");
    if (SandboxUtils::deviceTypeEnable_ == true) {
        EXPECT_EQ(ret, 0);
    } else {
        EXPECT_NE(ret, 0);
    }
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_44 end";
}

/**
* @tc.name: App_Spawn_Sandbox_45
* @tc.desc: parse config file for options.
* @tc.type: FUNC
* @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_45, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_45 start";
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"/data/app/el1/<currentUserId>/base\", \
            \"sandbox-path\": \"/storage/Users/<currentUserId>/appdata/el1\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dac-override-sensitive\": \"true\", \
            \"fs-type\": \"sharefs\", \
            \"options\": \"support_overwrite=1\" \
        }] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());
    const char *mountPath = "mount-paths";
    nlohmann::json j_secondConfig = j_config[mountPath][0];

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config);
    std::string options = OHOS::AppSpawn::SandboxUtils::GetSandboxOptions(j_secondConfig);
    int ret = strcmp(options.c_str(), "support_overwrite=1");
    if (SandboxUtils::deviceTypeEnable_ == true) {
        EXPECT_EQ(ret, 0);
    } else {
        EXPECT_NE(ret, 0);
    }
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_45 end";
}
} // namespace OHOS
