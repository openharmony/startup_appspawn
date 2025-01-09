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

#include <gtest/gtest.h>

#include <cerrno>
#include <memory>
#include <string>

#include "appspawn_server.h"
#include "appspawn_service.h"
#include "json_utils.h"
#include "parameter.h"
#include "sandbox_utils.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;
using nlohmann::json;

namespace OHOS {
AppSpawnTestHelper g_testHelper;

class AppSpawnSandboxTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppSpawnSandboxTest::SetUpTestCase() {}

void AppSpawnSandboxTest::TearDownTestCase() {}

void AppSpawnSandboxTest::SetUp() {}

void AppSpawnSandboxTest::TearDown() {}

static AppSpawningCtx *GetTestAppProperty()
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create reqMgr");
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, return nullptr, "Failed to create req");
    return g_testHelper.GetAppProperty(clientHandle, reqHandle);
}

static AppSpawningCtx *GetTestAppPropertyWithExtInfo(const char *name, const char *value)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create reqMgr");
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, return nullptr, "Failed to create req");
    AppSpawnReqMsgAddStringInfo(reqHandle, name, value);
    return g_testHelper.GetAppProperty(clientHandle, reqHandle);
}

/**
 * @tc.name: App_Spawn_Sandbox_005
 * @tc.desc: load system config SetAppSandboxProperty by App ohos.samples.ecg.
 * @tc.type: FUNC
 * @tc.require:issueI5NTX6
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_08, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_08 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("ohos.samples.ecg");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_08 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_09
 * @tc.desc: load system config SetAppSandboxProperty by App com.ohos.dlpmanager.
 * @tc.type: FUNC
 * @tc.require:issueI5NTX6
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09 start";
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09_1, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(nullptr);
    EXPECT_NE(ret, 0);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09_2, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    int ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    EXPECT_NE(ret, 0);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09_3, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.\\ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    EXPECT_NE(ret, 0);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09_4, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com./ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int ret = OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    EXPECT_NE(ret, 0);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09_1 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_10
 * @tc.desc: parse config define by self Set App Sandbox Property.
 * @tc.type: FUNC
 * @tc.require:issueI5NTX6
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_10, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);
    GTEST_LOG_(INFO) << "SetAppSandboxProperty start" << std::endl;
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    GTEST_LOG_(INFO) << "SetAppSandboxProperty section 2" << std::endl;
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_10 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_13
 * @tc.desc: parse config define by self Set App Sandbox Property.
 * @tc.type: FUNC
 * @tc.require:issueI5NTX6
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_13, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);

    GTEST_LOG_(INFO) << "SetAppSandboxProperty start" << std::endl;
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_13 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_14
 * @tc.desc: parse sandbox config without sandbox-root label
 * @tc.type: FUNC
 * @tc.require:issueI5NTX6
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_14, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);

    GTEST_LOG_(INFO) << "SetAppSandboxProperty start" << std::endl;

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    GTEST_LOG_(INFO) << "SetAppSandboxProperty section 2" << std::endl;
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_14 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_15
 * @tc.desc: parse app sandbox config with PackageName_index label
 * @tc.type: FUNC
 * @tc.require:issueI5NTX6
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_15, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);

    GTEST_LOG_(INFO) << "SetAppSandboxProperty start" << std::endl;
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    GTEST_LOG_(INFO) << "SetAppSandboxProperty section 2" << std::endl;
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_15 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_16
 * @tc.desc: parse config define by self Set App Sandbox Property.
 * @tc.type: FUNC
 * @tc.require:issueI5NTX6
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_16, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);

    GTEST_LOG_(INFO) << "SetAppSandboxProperty start" << std::endl;
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    DeleteAppSpawningCtx(appProperty);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_16 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_17, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09 start";
    nlohmann::json appSandboxConfig;
    bool rc = JsonUtils::GetJsonObjFromJson(appSandboxConfig, "");
    EXPECT_FALSE(rc);
    std::string path(256, 'w');  // 256 test
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
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);

    std::string value;
    rc = JsonUtils::GetStringFromJson(j_config, "common", value);
    EXPECT_FALSE(rc);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_09 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_18, TestSize.Level0)
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

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_20, TestSize.Level0)
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
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);

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
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config1, SANBOX_APP_JSON_CONFIG);
    OHOS::AppSpawn::SandboxUtils::SetAppSandboxProperty(appProperty);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_21, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_21 start";

    bool ret = OHOS::AppSpawn::SandboxUtils::CheckBundleNameForPrivate(std::string("__internal__"));
    EXPECT_FALSE(ret);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_22, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_22 start";
    std::string mJsconfig1 = "{ \
        \"common\":[], \
        \"individual\": [] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config1, SANBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    int ret = OHOS::AppSpawn::SandboxUtils::SetCommonAppSandboxProperty(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_23, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_23 start";
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    int ret = OHOS::AppSpawn::SandboxUtils::SetRenderSandboxProperty(nullptr, testBundle);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_24, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_24 start";

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.bundle.name1");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

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
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonSymlink(appProperty, j_config1);
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
    ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonSymlink(appProperty, j_config2);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_25, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_25 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.bundle.wps");
    g_testHelper.SetTestApl("system_basic");
    g_testHelper.SetTestMsgFlags(4);  // 4 is test parameter
    AppSpawningCtx *appProperty = GetTestAppProperty();

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
                    \"src-path\" : \"/data/app/el1/<currentUserId>/database/<PackageName_index>\", \
                    \"sandbox-path\" : \"/data/storage/el1/database\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\" \
                }], \
                \"symbol-links\" : [] \
            }] \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonBind(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonFlagsPointHandle(appProperty, j_config1);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_26, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_26 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();
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
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFileCommonBind(appProperty, j_config2);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_27, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_27 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.bundle.name");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"individual\": [{ \
            \"test.bundle.name\" : [{ \
                \"sandbox-switch\": \"OFF\", \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
            }] \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(appProperty, j_config1);
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
    ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(appProperty, j_config2);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_28, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_28 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.bundle.name");
    g_testHelper.SetTestApl("system_basic");
    g_testHelper.SetTestMsgFlags(4);  // 4 is test parameter
    AppSpawningCtx *appProperty = GetTestAppProperty();

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
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(appProperty, j_config3);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_29, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_29 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.bundle.name");
    g_testHelper.SetTestApl("system_basic");
    g_testHelper.SetTestMsgFlags(4);  // 4 is test parameter
    AppSpawningCtx *appProperty = GetTestAppProperty();

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
    int ret = OHOS::AppSpawn::SandboxUtils::DoSandboxFilePrivateFlagsPointHandle(appProperty, j_config3);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_30, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_30 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.wps");
    g_testHelper.SetTestApl("apl123");
    g_testHelper.SetTestMsgFlags(4);  // 4 is test parameter
    AppSpawningCtx *appProperty = GetTestAppProperty();

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
    int ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(appProperty, j_config3, nullptr);
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
    ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(appProperty, j_config4, nullptr);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_31, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_31 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    g_testHelper.SetTestApl("apl123");
    AppSpawningCtx *appProperty = GetTestAppProperty();

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
    int ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(appProperty, j_config1, nullptr);
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

    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(appProperty, j_config2, nullptr);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_TRUE(ret != 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_32, TestSize.Level0)
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

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_34, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_34 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    g_testHelper.SetTestApl("apl123");

    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(0, ret);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_34 end";
}

static void InvalidJsonTest(std::string &testBundle)
{
    char hspListStr[] = "{";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

static void NoBundleTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

static void NoModulesTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";

    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

static void NoVersionsTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

static void ListSizeNotSameTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

static void ValueTypeIsNotArraryTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": 1001 \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

static void ElementTypeIsNotStringTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": [1001, 1002] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

static void ElementTypeIsNotSameTestSN(std::string &testBundle)
{
    // element type is not same, string + number
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": [\"v10001\", 1002] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

static void ElementTypeIsNotSameTestNS(std::string &testBundle)
{
    // element type is not same, number + string
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": [1001, \"v10002\"] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(0, ret);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_35, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_35 start";
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    InvalidJsonTest(testBundle);
    NoBundleTest(testBundle);
    NoModulesTest(testBundle);
    NoVersionsTest(testBundle);
    ListSizeNotSameTest(testBundle);
    ValueTypeIsNotArraryTest(testBundle);
    ElementTypeIsNotStringTest(testBundle);
    ElementTypeIsNotSameTestSN(testBundle);
    ElementTypeIsNotSameTestNS(testBundle);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_35 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_36, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_36 start";
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;

    {  // empty name
        char hspListStr[] = "{ \
            \"bundles\":[\"\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
        DeleteAppSpawningCtx(appProperty);
        EXPECT_NE(0, ret);
    }
    {  // name is .
        char hspListStr[] = "{ \
            \"bundles\":[\".\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
        DeleteAppSpawningCtx(appProperty);
        EXPECT_NE(0, ret);
    }
    {  // name is ..
        char hspListStr[] = "{ \
            \"bundles\":[\"..\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
        DeleteAppSpawningCtx(appProperty);
        EXPECT_NE(0, ret);
    }
    {  // name contains /
        char hspListStr[] = "{ \
            \"bundles\":[\"test/bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
        int ret = OHOS::AppSpawn::SandboxUtils::MountAllHsp(appProperty, testBundle);
        DeleteAppSpawningCtx(appProperty);
        EXPECT_NE(0, ret);
    }
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_36 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_37, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Sandbox_37 start");
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("ohos.samples.xxx");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    AppSpawnMgr content;
    LoadAppSandboxConfig(&content);
    std::string sandboxPackagePath = "/mnt/sandbox/100/";
    const std::string bundleName = GetBundleName(appProperty);
    sandboxPackagePath += bundleName;

    int ret = SandboxUtils::SetPrivateAppSandboxProperty(appProperty);
    EXPECT_EQ(0, ret);
    ret = SandboxUtils::SetCommonAppSandboxProperty(appProperty, sandboxPackagePath);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(0, ret);
    APPSPAWN_LOGI("App_Spawn_Sandbox_37 end");
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_38, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Sandbox_38 start");
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.deviceinfo");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

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

    nlohmann::json p_config1 = nlohmann::json::parse(pJsconfig1.c_str());
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(p_config1, SANBOX_APP_JSON_CONFIG);

    std::string sandboxPackagePath = "/mnt/sandbox/100/";
    const std::string bundleName = GetBundleName(appProperty);
    sandboxPackagePath += bundleName;
    int ret = SandboxUtils::SetPrivateAppSandboxProperty(appProperty);
    EXPECT_EQ(0, ret);
    ret = SandboxUtils::SetCommonAppSandboxProperty(appProperty, sandboxPackagePath);
    DeleteAppSpawningCtx(appProperty);
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
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_39, TestSize.Level0)
{
    APPSPAWN_LOGI("App_Spawn_Sandbox_39 start");
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.demo");
    g_testHelper.SetTestApl("normal");
    g_testHelper.SetTestMsgFlags(0x100);  // 0x100 is test parameter

    std::string overlayInfo = "/data/app/el1/bundle/public/com.ohos.demo/feature.hsp|";
    overlayInfo += "/data/app/el1/bundle/public/com.ohos.demo/feature.hsp|";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("Overlay", overlayInfo.c_str());
    std::string sandBoxRootDir = "/mnt/sandbox/100/com.ohos.demo";
    int32_t ret = OHOS::AppSpawn::SandboxUtils::SetOverlayAppSandboxProperty(appProperty, sandBoxRootDir);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(0, ret);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_39 end";
}
/**
 * @tc.name: App_Spawn_Sandbox_40
 * @tc.desc: load group info config SetAppSandboxProperty
 * @tc.type: FUNC
 * @tc.require:issueI7FUPV
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_40, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_40 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.demo");
    g_testHelper.SetTestApl("normal");
    g_testHelper.SetTestMsgFlags(0x100);
    std::string sandboxPrefix = "/mnt/sandbox/100/testBundle";
    char dataGroupInfoListStr[] = R"([
        {
            "gid": "1002",
            "dir": "data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": "43200",
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        },
        {
            "gid": "1002",
            "dir": "data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": "43200",
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    ])";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("DataGroup", dataGroupInfoListStr);
    int ret = OHOS::AppSpawn::SandboxUtils::MountAllGroup(appProperty, sandboxPrefix);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(0, ret);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_40 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_41
 * @tc.desc: parse namespace config.
 * @tc.type: FUNC
 * @tc.require:issueI8B63M
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_41, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);

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
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_42, TestSize.Level0)
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
    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);

    std::string fsType = OHOS::AppSpawn::SandboxUtils::GetSandboxFsType(j_secondConfig);
    int ret = strcmp(fsType.c_str(), "sharefs");
    EXPECT_EQ(ret, 0);

    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_42 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_43
 * @tc.desc: get sandbox mount config when section is common.
 * @tc.type: FUNC
 * @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_43, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);
    OHOS::AppSpawn::SandboxUtils::SandboxMountConfig mountConfig;
    std::string section = "common";
    AppSpawningCtx *appProperty = GetTestAppProperty();
    OHOS::AppSpawn::SandboxUtils::GetSandboxMountConfig(appProperty, section, j_secondConfig, mountConfig);
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
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_44, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);
    OHOS::AppSpawn::SandboxUtils::SandboxMountConfig mountConfig;
    std::string section = "permission";
    AppSpawningCtx *appProperty = GetTestAppProperty();
    OHOS::AppSpawn::SandboxUtils::GetSandboxMountConfig(appProperty, section, j_secondConfig, mountConfig);
    int ret = strcmp(mountConfig.fsType.c_str(), "sharefs");
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_44 end";
}

/**
 * @tc.name: App_Spawn_Sandbox_45
 * @tc.desc: parse config file for options.
 * @tc.type: FUNC
 * @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_45, TestSize.Level0)
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

    OHOS::AppSpawn::SandboxUtils::StoreJsonConfig(j_config, SANBOX_APP_JSON_CONFIG);
    AppSpawningCtx *appProperty = GetTestAppProperty();
    std::string options = OHOS::AppSpawn::SandboxUtils::GetSandboxOptions(appProperty, j_secondConfig);
    int ret = strcmp(options.c_str(), "support_overwrite=1,user_id=100");
    EXPECT_EQ(ret, 0);
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_45 end";
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_46, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_46 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("apl123");
    g_testHelper.SetTestMsgFlags(4);  // 4 is test parameter
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/service/el1/public/themes/<currentUserId>/a/app/\", \
            \"sandbox-path\" : \"/data/themes/a/app\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\" \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(appProperty, j_config1, nullptr);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig2 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/service/el1/public/themes/<currentUserId>/a/app/createSandboxPath01\", \
            \"sandbox-path\" : \"/data/themes/a/app/createSandboxPath01\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\", \
            \"create-sandbox-path\": \"true\" \
        }] \
    }";
    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(appProperty, j_config2, nullptr);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_47, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_47 start";
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("apl123");
    g_testHelper.SetTestMsgFlags(4);  // 4 is test parameter
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/service/el1/public/themes/<currentUserId>/a/app/\", \
            \"sandbox-path\" : \"/data/themes/a/app\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\" \
        }] \
    }";
    nlohmann::json j_config1 = nlohmann::json::parse(mJsconfig1.c_str());
    int ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(appProperty, j_config1, nullptr);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig2 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/service/el1/public/themes/<currentUserId>/a/app/createSandboxPath01\", \
            \"sandbox-path\" : \"/data/themes/a/app/createSandboxPath01\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\", \
            \"create-sandbox-path\": \"false\" \
        }] \
    }";
    nlohmann::json j_config2 = nlohmann::json::parse(mJsconfig2.c_str());
    ret = OHOS::AppSpawn::SandboxUtils::DoAllMntPointsMount(appProperty, j_config2, nullptr);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}
/**
 * @brief 测试app extension
 *
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_AppExtension_004, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    std::string path = SandboxUtils::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    APPSPAWN_LOGV("path %{public}s", path.c_str());
    ASSERT_EQ(path.c_str() != nullptr, 1);
    ASSERT_EQ(strcmp(path.c_str(), "/system/com.example.myapplication/module") == 0, 1);
    DeleteAppSpawningCtx(spawningCtx);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_AppExtension_005, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);
    int ret = SetAppSpawnMsgFlag(spawningCtx->message, TLV_MSG_FLAGS, APP_FLAGS_CLONE_ENABLE);
    ASSERT_EQ(ret, 0);

    std::string path = SandboxUtils::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    APPSPAWN_LOGV("path %{public}s", path.c_str());
    ASSERT_EQ(path.c_str() != nullptr, 1);  // +clone-bundleIndex+packageName
    ASSERT_EQ(strcmp(path.c_str(), "/system/+clone-100+com.example.myapplication/module") == 0, 1);
    DeleteAppSpawningCtx(spawningCtx);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_AppExtension_006, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_EQ(reqHandle != nullptr, 1);
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_APP_EXTENSION, "test001");
    ASSERT_EQ(ret, 0);
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_EXTENSION_SANDBOX);
    ASSERT_EQ(ret, 0);
    AppSpawningCtx *spawningCtx = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_EQ(spawningCtx != nullptr, 1);

    std::string path = SandboxUtils::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    APPSPAWN_LOGV("path %{public}s", path.c_str());
    ASSERT_EQ(path.c_str() != nullptr, 1);  // +extension-<extensionType>+packageName
    ASSERT_EQ(strcmp(path.c_str(), "/system/+extension-test001+com.example.myapplication/module") == 0, 1);
    DeleteAppSpawningCtx(spawningCtx);
    AppSpawnClientDestroy(clientHandle);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_AppExtension_007, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_EQ(reqHandle != nullptr, 1);
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_APP_EXTENSION, "test001");
    ASSERT_EQ(ret, 0);
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_EXTENSION_SANDBOX);
    ASSERT_EQ(ret, 0);
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_CLONE_ENABLE);
    ASSERT_EQ(ret, 0);
    AppSpawningCtx *spawningCtx = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_EQ(spawningCtx != nullptr, 1);

    std::string path = SandboxUtils::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    APPSPAWN_LOGV("path %{public}s", path.c_str());
    ASSERT_EQ(path.c_str() != nullptr, 1);  // +clone-bundleIndex+extension-<extensionType>+packageName

    ASSERT_EQ(strcmp(path.c_str(), "/system/+clone-100+extension-test001+com.example.myapplication/module") == 0, 1);
    DeleteAppSpawningCtx(spawningCtx);
    AppSpawnClientDestroy(clientHandle);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_AppExtension_008, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_EQ(reqHandle != nullptr, 1);
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_EXTENSION_SANDBOX);
    ASSERT_EQ(ret, 0);
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_CLONE_ENABLE);
    ASSERT_EQ(ret, 0);
    AppSpawningCtx *spawningCtx = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_EQ(spawningCtx != nullptr, 1);

    std::string path = SandboxUtils::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    APPSPAWN_LOGV("path %{public}s", path.c_str());
    ASSERT_STREQ(path.c_str(), "");

    DeleteAppSpawningCtx(spawningCtx);
    AppSpawnClientDestroy(clientHandle);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_AppExtension_009, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    ASSERT_EQ(ret, 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_EQ(reqHandle != nullptr, 1);
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_EXTENSION_SANDBOX);
    ASSERT_EQ(ret, 0);
    AppSpawningCtx *spawningCtx = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_EQ(spawningCtx != nullptr, 1);

    std::string path = SandboxUtils::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    APPSPAWN_LOGV("path %{public}s", path.c_str());
    ASSERT_STREQ(path.c_str(), "");

    DeleteAppSpawningCtx(spawningCtx);
    AppSpawnClientDestroy(clientHandle);
}
}  // namespace OHOS
