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
* @tc.name: App_Spawn_Sandbox_11
* @tc.desc: parse namespace config define by self set app sandbox property.
* @tc.type: FUNC
* @tc.require: issueI5OE8Q
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_011, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_011 start";
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

    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_011 end";
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName_index>\", \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
        \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\" \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    bool ret = OHOS::AppSpawn::SandboxUtils::GetSbxSwitchStatusByConfig(j_config1);
    EXPECT_TRUE(ret);

    std::string mJsconfig2 = "{ \
        \"sandbox-switch\": \"OFF\", \
        \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\" \
    }";
    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::GetSbxSwitchStatusByConfig(j_config2);
    EXPECT_FALSE(ret);

    std::string mJsconfig3 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\" \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
    GetAppSpawnClient()->cloneFlags = CLONE_NEWPID;

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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\": [{ \
            \"test.bundle.name\" : [{ \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\" \
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
    const char *strl1 = "/mnt/sandbox/test.bundle1";
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
    const char *strl1 = "/mnt/sandbox/test.bundle1";
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\" \
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
                    \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
                    \"mount-paths\" : [{ \
                        \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                        \"sandbox-path\" : \"/data/storage/el2/base\", \
                        \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                        \"check-action-status\": \"false\" \
                    }]\
                }], \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\" \
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
                    \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
                    \"mount-paths\" : [{ \
                        \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                        \"sandbox-path\" : \"/data/storage/el2/base\", \
                        \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                        \"check-action-status\": \"false\" \
                    }] \
                }], \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\" \
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
                    \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
                    \"mount-paths\" : [{ \
                        \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                        \"sandbox-path\" : \"/data/storage/el2/base\", \
                        \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                        \"check-action-status\": \"false\" \
                    }] \
                }], \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\" \
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
        \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
        \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
        \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
        \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\", \
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
    std::string mJsconfig = "{ \
        \"test-namespace\" : [{ \
            \"com.ohos.note\" : [{ \
                \"clone-flags\": [ \"mnt\", \"pid\" ] \
            }] \
        }] \
    }";
    nlohmann::json j_config = nlohmann::json::parse(mJsconfig.c_str());
    OHOS::AppSpawn::SandboxUtils::StoreNamespaceJsonConfig(j_config);
    int ret = OHOS::AppSpawn::SandboxUtils::GetNamespaceFlagsFromConfig("ohos.test.bundle");
    EXPECT_EQ(ret, 0);

    ret = OHOS::AppSpawn::SandboxUtils::DoAppSandboxMountOnce(nullptr, "", nullptr, 0, nullptr);
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

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_33, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_33 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    m_appProperty->uid = 1000;
    m_appProperty->gid = 1000;

    const char *srcPath = "/data/app/el1";
    std::string path = srcPath;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path);

    const char *srcPath1 = "/data/app/el2/100/base/ohos.test.bundle1";
    std::string path1 = srcPath1;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path1);

    const char *srcPath2 = "/data/app/el2/100/database/ohos.test.bundle2";
    std::string path2 = srcPath2;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path2);

    const char *srcPath3 = "/data/app/el2/100/test123/ohos.test.bundle3";
    std::string path3 = srcPath3;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path3);

    const char *srcPath4 = "/data/serivce/el2/100/hmdfs/account/data/ohos.test.bundle4";
    std::string path4 = srcPath4;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path4);

    const char *srcPath5 = "/data/serivce/el2/100/hmdfs/non_account/data/ohos.test.bundle5";
    std::string path5 = srcPath5;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path5);

    const char *srcPath6 = "/data/serivce/el2/100/hmdfs/non_account/test/ohos.test.bundle6";
    std::string path6 = srcPath6;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path6);
    SUCCEED();
}

/**
* @tc.name: App_Spawn_Sandbox_34
* @tc.desc: parse and mount HspList
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_34, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_34 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    const char *strl1 = "/mnt/sandbox/test.bundle1";
    std::string testBundle = strl1;

    { // totalLength is 0
        m_appProperty->hspList = {};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_EQ(0, ret);
    }
    { // data is nullptr
        m_appProperty->hspList = {1, 0, nullptr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_EQ(0, ret);
    }
    { // success
        char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_EQ(0, ret);
    }

    m_appProperty->hspList = {};
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_34 end";
}

/**
* @tc.name: App_Spawn_Sandbox_35
* @tc.desc: validate json format of HspList
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_35, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_35 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    const char *strl1 = "/mnt/sandbox/test.bundle1";
    std::string testBundle = strl1;
    { // invalid json
        char hspListStr[] = "{";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // no bundles
        char hspListStr[] = "{ \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // no modules
        char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // no versions
        char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // list size not same
        char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // value type is not arrary
        char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\": 1001 \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // element type is not string
        char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\": [1001, 1002] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // element type is not same, string + number
        char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\": [\"v10001\", 1002] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // element type is not same, number + string
        char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\": [1001, \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }

    m_appProperty->hspList = {};
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_35 end";
}

/**
* @tc.name: App_Spawn_Sandbox_36
* @tc.desc: validate bundle、module and version in hspList
* @tc.type: FUNC
* @tc.require:issueI6798L
* @tc.author:
*/
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_36, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_36 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    const char *strl1 = "/mnt/sandbox/test.bundle1";
    std::string testBundle = strl1;

    { // empty name
        char hspListStr[] = "{ \
            \"bundles\":[\"\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // name is .
        char hspListStr[] = "{ \
            \"bundles\":[\".\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // name is ..
        char hspListStr[] = "{ \
            \"bundles\":[\"..\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }
    { // name contains /
        char hspListStr[] = "{ \
            \"bundles\":[\"test/bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        m_appProperty->hspList = {strlen(hspListStr), 0, hspListStr};
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(m_appProperty, testBundle);
        EXPECT_NE(0, ret);
    }

    m_appProperty->hspList = {};
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_36 end";
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_37, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Sandbox_37 start");
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    m_appProperty->uid = 1000;
    m_appProperty->gid = 1000;
    (void)strcpy_s(m_appProperty->bundleName, sizeof(m_appProperty->bundleName), "ohos.samples.xxx");
    LoadAppSandboxConfig();
    std::string sandboxPackagePath = "/mnt/sandbox/";
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
            \"sandbox-root\" : \"/mnt/sandbox/<PackageName>\",  \
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
    std::string sandboxPackagePath = "/mnt/sandbox/";
    const std::string bundleName = m_appProperty->bundleName;
    sandboxPackagePath += bundleName;
    int ret = SandboxUtils::SetPrivateAppSandboxProperty(m_appProperty);
    EXPECT_EQ(0, ret);
    ret = SandboxUtils::SetCommonAppSandboxProperty(m_appProperty, sandboxPackagePath);
    EXPECT_EQ(0, ret);
    APPSPAWN_LOGI("App_Spawn_Sandbox_38 end");
}
} // namespace OHOS
