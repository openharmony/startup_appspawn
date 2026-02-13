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
#include "sandbox_def.h"
#include "sandbox_core.h"
#include "sandbox_common.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"
#include "sandbox_dec.h"
#include "sandbox_shared_mount.h"
#include "parameters.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;

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

void AppSpawnSandboxTest::SetUp() {
    const TestInfo *info = UnitTest::GetInstance()->current_test_info();
    GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
    APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());

    std::vector<cJSON *> &appVec =
        AppSpawn::SandboxCommon::GetCJsonConfig(SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);
    appVec.erase(appVec.begin(), appVec.end());

    std::vector<cJSON *> &isolatedVec =
        AppSpawn::SandboxCommon::GetCJsonConfig(SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG);
    isolatedVec.erase(isolatedVec.begin(), isolatedVec.end());
}

void AppSpawnSandboxTest::TearDown() {
    std::vector<cJSON *> &appVec =
        AppSpawn::SandboxCommon::GetCJsonConfig(SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);
    appVec.erase(appVec.begin(), appVec.end());

    std::vector<cJSON *> &isolatedVec =
        AppSpawn::SandboxCommon::GetCJsonConfig(SandboxCommonDef::SANDBOX_ISOLATED_JSON_CONFIG);
    isolatedVec.erase(isolatedVec.begin(), isolatedVec.end());

    const TestInfo *info = UnitTest::GetInstance()->current_test_info();
    GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
}

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
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("ohos.samples.ecg");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
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
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09_1, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(nullptr, CLONE_NEWPID);
    EXPECT_NE(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09_2, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_NE(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09_3, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.\\ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_NE(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_09_4, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com./ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_NE(ret, 0);

    DeleteAppSpawningCtx(appProperty);
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
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
    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\": [] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_17, TestSize.Level0)
{
    cJSON *appSandboxConfig = GetJsonObjFromFile("");
    EXPECT_EQ(appSandboxConfig, nullptr);

    std::string path(256, 'w');  // 256 test
    appSandboxConfig = GetJsonObjFromFile(path.c_str());
    EXPECT_EQ(appSandboxConfig, nullptr);

    std::string mJsconfig = "{ \
        \"common\":[{ \
            \"app-base\":[{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"symbol-links\" : [] \
            }] \
        }], \
        \"individual\": [] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    const char *value = GetStringFromJsonObj(j_config, "common");
    EXPECT_EQ(value, nullptr);

    cJSON_Delete(j_config);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_18, TestSize.Level0)
{
    std::string mJsconfig1 = "{ \
        \"sandbox-switch\": \"ON\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    bool ret = AppSpawn::SandboxCommon::GetSwitchStatus(j_config1);
    EXPECT_TRUE(ret);

    std::string mJsconfig2 = "{ \
        \"sandbox-switch\": \"OFF\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
    }";
    cJSON *j_config2 = cJSON_Parse(mJsconfig2.c_str());
    ASSERT_NE(j_config2, nullptr);
    ret = AppSpawn::SandboxCommon::GetSwitchStatus(j_config2);
    EXPECT_FALSE(ret);

    std::string mJsconfig3 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
    }";
    cJSON *j_config3 = cJSON_Parse(mJsconfig3.c_str());
    ASSERT_NE(j_config3, nullptr);
    ret = AppSpawn::SandboxCommon::GetSwitchStatus(j_config3);
    EXPECT_TRUE(ret);

    cJSON_Delete(j_config1);
    cJSON_Delete(j_config2);
    cJSON_Delete(j_config3);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_20, TestSize.Level0)
{
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

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
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);

    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config1, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);
    ret = AppSpawn::SandboxCore::SetAppSandboxProperty(appProperty, CLONE_NEWPID);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_21, TestSize.Level0)
{
    bool ret = AppSpawn::SandboxCommon::HasPrivateInBundleName(std::string("__internal__"));
    EXPECT_FALSE(ret);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_22, TestSize.Level0)
{
    std::string mJsconfig1 = "{ \
        \"common\":[], \
        \"individual\": [] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("test.appspawn");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    int32_t ret = AppSpawn::SandboxCore::SetCommonAppSandboxProperty(appProperty, testBundle);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_24, TestSize.Level0)
{
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
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoSandboxFileCommonSymlink(appProperty, j_config1);
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
    cJSON *j_config2 = cJSON_Parse(mJsconfig2.c_str());
    ASSERT_NE(j_config2, nullptr);
    ret = AppSpawn::SandboxCore::DoSandboxFileCommonSymlink(appProperty, j_config2);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    cJSON_Delete(j_config2);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_25, TestSize.Level0)
{
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
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoSandboxFileCommonBind(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    ret = AppSpawn::SandboxCore::DoSandboxFileCommonFlagsPointHandle(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_26, TestSize.Level0)
{
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
    cJSON *j_config2 = cJSON_Parse(mJsconfig2.c_str());
    ASSERT_NE(j_config2, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoSandboxFileCommonBind(appProperty, j_config2);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config2);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_27, TestSize.Level0)
{
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
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoSandboxFilePrivateFlagsPointHandle(appProperty, j_config1);
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
    cJSON *j_config2 = cJSON_Parse(mJsconfig2.c_str());
    ASSERT_NE(j_config2, nullptr);
    ret = AppSpawn::SandboxCore::DoSandboxFilePrivateFlagsPointHandle(appProperty, j_config2);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    cJSON_Delete(j_config2);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_28, TestSize.Level0)
{
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
    cJSON *j_config3 = cJSON_Parse(mJsconfig3.c_str());
    ASSERT_NE(j_config3, nullptr);
    int ret = AppSpawn::SandboxCore::DoSandboxFilePrivateFlagsPointHandle(appProperty, j_config3);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config3);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_29, TestSize.Level0)
{
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
    cJSON *j_config3 = cJSON_Parse(mJsconfig3.c_str());
    ASSERT_NE(j_config3, nullptr);
    int ret = AppSpawn::SandboxCore::DoSandboxFilePrivateFlagsPointHandle(appProperty, j_config3);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config3);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_30, TestSize.Level0)
{
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
    cJSON *j_config3 = cJSON_Parse(mJsconfig3.c_str());
    ASSERT_NE(j_config3, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config3, nullptr);
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
    cJSON *j_config4 = cJSON_Parse(mJsconfig4.c_str());
    ASSERT_NE(j_config4, nullptr);
    ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config4, nullptr);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config3);
    cJSON_Delete(j_config4);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_31, TestSize.Level0)
{
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
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config1, nullptr);
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
    cJSON *j_config2 = cJSON_Parse(mJsconfig2.c_str());
    ASSERT_NE(j_config2, nullptr);
    ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config2, nullptr);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    cJSON_Delete(j_config2);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_32, TestSize.Level0)
{
    int32_t ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnce(nullptr, nullptr);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig1 = "{ \
        \"dest-mode\" : \"S_IRUSR|S_IWUSR|S_IXUSR\" \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    std::string sandboxRoot;
    const char *str = "/data/test11122";
    sandboxRoot = str;
    AppSpawn::SandboxCommon::SetSandboxPathChmod(j_config1, sandboxRoot);

    cJSON_Delete(j_config1);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_34, TestSize.Level0)
{
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
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

static void InvalidJsonTest(std::string &testBundle)
{
    char hspListStr[] = "{";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_EQ(ret, 0);
}

static void NoBundleTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
}

static void NoModulesTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";

    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
}

static void NoVersionsTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
}

static void ListSizeNotSameTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
}

static void ValueTypeIsNotArraryTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": 1001 \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
}

static void ElementTypeIsNotStringTest(std::string &testBundle)
{
    char hspListStr[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\": [1001, 1002] \
    }";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
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
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
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
    cJSON *hspRoot = cJSON_Parse(hspListStr);
    int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
    cJSON_Delete(hspRoot);
    DeleteAppSpawningCtx(appProperty);
    EXPECT_NE(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_35, TestSize.Level0)
{
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
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_36, TestSize.Level0)
{
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;

    {  // empty name
        char hspListStr[] = "{ \
            \"bundles\":[\"\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
        cJSON *hspRoot = cJSON_Parse(hspListStr);
        int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
        cJSON_Delete(hspRoot);
        DeleteAppSpawningCtx(appProperty);
        EXPECT_NE(ret, 0);
    }
    {  // name is .
        char hspListStr[] = "{ \
            \"bundles\":[\".\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
        cJSON *hspRoot = cJSON_Parse(hspListStr);
        int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
        cJSON_Delete(hspRoot);
        DeleteAppSpawningCtx(appProperty);
        EXPECT_NE(ret, 0);
    }
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_37, TestSize.Level0)
{
    const char *strl1 = "/mnt/sandbox/100/test.bundle1";
    std::string testBundle = strl1;
    {  // name is ..
        char hspListStr[] = "{ \
            \"bundles\":[\"..\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
        cJSON *hspRoot = cJSON_Parse(hspListStr);
        int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
        cJSON_Delete(hspRoot);
        DeleteAppSpawningCtx(appProperty);
        EXPECT_NE(ret, 0);
    }
    {  // name contains /
        char hspListStr[] = "{ \
            \"bundles\":[\"test/bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("HspList", hspListStr);
        cJSON *hspRoot = cJSON_Parse(hspListStr);
        int32_t ret = AppSpawn::SandboxCore::MountAllHsp(appProperty, testBundle, hspRoot);
        cJSON_Delete(hspRoot);
        DeleteAppSpawningCtx(appProperty);
        EXPECT_NE(ret, 0);
    }
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_38, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("ohos.samples.xxx");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    AppSpawnMgr content;
    AppSpawn::SandboxCommon::LoadAppSandboxConfigCJson(&content);
    std::string sandboxPackagePath = "/mnt/sandbox/100/";
    const std::string bundleName = GetBundleName(appProperty);
    sandboxPackagePath += bundleName;

    int32_t ret = AppSpawn::SandboxCore::SetPrivateAppSandboxProperty(appProperty);
    EXPECT_EQ(ret, 0);
    ret = AppSpawn::SandboxCore::SetCommonAppSandboxProperty(appProperty, sandboxPackagePath);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_39, TestSize.Level0)
{
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
    cJSON *j_config = cJSON_Parse(pJsconfig1.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    std::string sandboxPackagePath = "/mnt/sandbox/100/";
    const std::string bundleName = GetBundleName(appProperty);
    sandboxPackagePath += bundleName;
    int32_t ret = AppSpawn::SandboxCore::SetPrivateAppSandboxProperty(appProperty);
    EXPECT_EQ(ret, 0);
    ret = AppSpawn::SandboxCore::SetCommonAppSandboxProperty(appProperty, sandboxPackagePath);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_39
 * @tc.desc: load overlay config SetAppSandboxProperty by App com.ohos.demo.
 * @tc.type: FUNC
 * @tc.require:issueI7D0H9
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_40, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.demo");
    g_testHelper.SetTestApl("normal");
    g_testHelper.SetTestMsgFlags(0x100);  // 0x100 is test parameter

    std::string overlayInfo = "/data/app/el1/bundle/public/com.ohos.demo/feature.hsp|";
    overlayInfo += "/data/app/el1/bundle/public/com.ohos.demo/feature.hsp|";
    AppSpawningCtx *appProperty = GetTestAppPropertyWithExtInfo("Overlay", overlayInfo.c_str());
    std::string sandBoxRootDir = "/mnt/sandbox/100/com.ohos.demo";
    int32_t ret = AppSpawn::SandboxCore::SetOverlayAppSandboxProperty(appProperty, sandBoxRootDir);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_40
 * @tc.desc: load group info config SetAppSandboxProperty
 * @tc.type: FUNC
 * @tc.require:issueI7FUPV
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_41, TestSize.Level0)
{
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
    int32_t ret = AppSpawn::SandboxCore::MountAllGroup(appProperty, sandboxPrefix);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_41
 * @tc.desc: parse namespace config
 * @tc.type: FUNC
 * @tc.require:issueI8B63M
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_42, TestSize.Level0)
{
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    uint32_t cloneFlags = AppSpawn::SandboxCommon::GetSandboxNsFlags(false);
    EXPECT_EQ(!!(cloneFlags & CLONE_NEWPID), true);

    cloneFlags = AppSpawn::SandboxCommon::GetSandboxNsFlags(true);
    EXPECT_EQ(!!(cloneFlags & (CLONE_NEWPID | CLONE_NEWNET)), true);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_42
 * @tc.desc: parse config file for fstype
 * @tc.type: FUNC
 * @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_43, TestSize.Level0)
{
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mountPoint config");
            std::string fsType = AppSpawn::SandboxCommon::GetFsType(mntPoint);
            ret = strcmp(fsType.c_str(), "sharefs");
        }
        EXPECT_EQ(ret, 0);
    } while (0);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_43
 * @tc.desc: get sandbox mount config when section is common.
 * @tc.type: FUNC
 * @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_44, TestSize.Level0)
{
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    AppSpawningCtx *appProperty = GetTestAppProperty();
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mntPoint config");
            SandboxMountConfig mountConfig = {0};
            AppSpawn::SandboxCommon::GetSandboxMountConfig(appProperty, "common", mntPoint, mountConfig);
            ret = strcmp(mountConfig.fsType.c_str(), "sharefs");
        }
        EXPECT_EQ(ret, 0);
    } while (0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_44
 * @tc.desc: get sandbox mount config when section is permission.
 * @tc.type: FUNC
 * @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_45, TestSize.Level0)
{
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    AppSpawningCtx *appProperty = GetTestAppProperty();
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mntPoint config");
            SandboxMountConfig mountConfig = {0};
            AppSpawn::SandboxCommon::GetSandboxMountConfig(appProperty, "permission", mntPoint, mountConfig);
            ret = strcmp(mountConfig.fsType.c_str(), "sharefs");
        }
        EXPECT_EQ(ret, 0);
    } while (0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_45
 * @tc.desc: parse config file for options.
 * @tc.type: FUNC
 * @tc.require: https://gitee.com/openharmony/startup_appspawn/issues/I8OF9K
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_46, TestSize.Level0)
{
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
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    AppSpawningCtx *appProperty = GetTestAppProperty();
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mountPoint config");
            std::string options = AppSpawn::SandboxCommon::GetOptions(appProperty, mntPoint);
            ret = strcmp(options.c_str(), "support_overwrite=1,user_id=100");
        }
        EXPECT_EQ(ret, 0);
    } while (0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_47, TestSize.Level0)
{
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
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config1, nullptr);
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
    cJSON *j_config2 = cJSON_Parse(mJsconfig2.c_str());
    ASSERT_NE(j_config2, nullptr);
    ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config2, nullptr);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    cJSON_Delete(j_config2);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_48, TestSize.Level0)
{
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
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config1, nullptr);
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
    cJSON *j_config2 = cJSON_Parse(mJsconfig2.c_str());
    ASSERT_NE(j_config2, nullptr);
    ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config2, nullptr);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    cJSON_Delete(j_config2);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_49, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    int32_t ret = AppSpawn::SandboxCore::DoAllCreateOnDaemonMount(appProperty, nullptr);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig1 = "{ \
        \"mount-paths\" : \"\" \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    ret = AppSpawn::SandboxCore::DoAllCreateOnDaemonMount(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig2 = "{ \
        \"mount-paths\" : [{ \
            \"sandbox-path\" : \"/dev/shm\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\" \
        }] \
    }";
    cJSON *j_config2 = cJSON_Parse(mJsconfig2.c_str());
    ASSERT_NE(j_config2, nullptr);
    ret = AppSpawn::SandboxCore::DoAllCreateOnDaemonMount(appProperty, j_config2);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    cJSON_Delete(j_config2);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_50, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/mnt/sandbox/shm/<currentUserId>/app/<variablePackageName>\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\" \
        }] \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllCreateOnDaemonMount(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_51, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/mnt/sandbox/shm/<currentUserId>/app/<variablePackageName>\", \
            \"sandbox-path\" : \"/dev/shm\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\" \
        }] \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllCreateOnDaemonMount(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_52, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/mnt/sandbox/shm/<currentUserId>/app/<variablePackageName>\", \
            \"sandbox-path\" : \"/dev/shm\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\", \
            \"src-path-info\": { \
                \"uid\": \"0\", \
                \"gid\": \"0\", \
                \"mode\": \"1023\" \
            } \
        }] \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllCreateOnDaemonMount(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_53, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/mnt/sandbox/shm/<currentUserId>/app/<variablePackageName>\", \
            \"sandbox-path\" : \"/dev/shm\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\", \
            \"src-path-info\": { \
                \"uid\": 0, \
                \"gid\": 0, \
                \"mode\": 1023 \
            } \
        }] \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllCreateOnDaemonMount(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_54, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"common\":[{ \
            \"app-base\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                    \"sandbox-path\" : \"/data/storage/el2/base\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\" \
                }] \
            }], \
            \"app-resources\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                    \"sandbox-path\" : \"/data/storage/el2/base\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\" \
                }], \
                \"symbol-links\" : [] \
            }], \
            \"create-on-daemon\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/mnt/sandbox/shm/<currentUserId>/app/<variablePackageName>\", \
                    \"sandbox-path\" : \"/dev/shm\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\", \
                    \"src-path-info\": { \
                        \"uid\": 0, \
                        \"gid\": 0, \
                        \"mode\": 1023 \
                    } \
                }] \
            }] \
        }] \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoSandboxFileCommonBind(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_55, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"common\":[{ \
            \"app-base\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                    \"sandbox-path\" : \"/data/storage/el2/base\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\" \
                }] \
            }], \
            \"app-resources\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"\", \
                    \"sandbox-path\" : \"/data/storage/el2/base\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\" \
                }], \
                \"symbol-links\" : [] \
            }] \
        }] \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoSandboxFileCommonBind(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_56, TestSize.Level0)
{
    g_testHelper.SetProcessName("com.//ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxPropertyNweb(appProperty, CLONE_NEWPID);
    EXPECT_NE(ret, 0);
    DeleteAppSpawningCtx(appProperty);
    g_testHelper.SetProcessName("com.example.myapplication");
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_57, TestSize.Level0)
{
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxPropertyNweb(appProperty, CLONE_NEWPID);
    EXPECT_NE(ret, 0);
    DeleteAppSpawningCtx(appProperty);
    g_testHelper.SetProcessName("com.example.myapplication");
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_58, TestSize.Level0)
{
    g_testHelper.SetProcessName("");
    g_testHelper.SetTestApl("normal");
    AppSpawningCtx *appProperty = GetTestAppProperty();
    int32_t ret = AppSpawn::SandboxCore::SetAppSandboxPropertyNweb(appProperty, CLONE_NEWPID);
    EXPECT_NE(ret, 0);
    DeleteAppSpawningCtx(appProperty);
    g_testHelper.SetProcessName("com.example.myapplication");
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_59, TestSize.Level0)
{
    int32_t ret = 0;
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, nullptr);
    EXPECT_EQ(ret, 0);

    SharedMountArgs arg = {
        .srcPath = NULL,
        .destPath = "/ABC"
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg);
    EXPECT_EQ(ret, 0);

        SharedMountArgs arg1 = {
        .srcPath = "",
        .destPath = "/ABC"
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg1);
    EXPECT_EQ(ret, 0);

    SharedMountArgs arg2 = {
        .srcPath = "/ABC",
        .destPath = NULL
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg2);
    EXPECT_EQ(ret, 0);

    SharedMountArgs arg3 = {
        .srcPath = "/ABC",
        .destPath = ""
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg3);
    EXPECT_EQ(ret, 0);

    SharedMountArgs arg4 = {
        .srcPath = "/config",
        .destPath = "/config"
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg4);
    EXPECT_EQ(ret, 0);

    SharedMountArgs arg5 = {
        .srcPath = "system/etc/hosts/ABC",
        .destPath = "system/etc/hosts/ABC"
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg5);
    EXPECT_EQ(ret, 0);

    SharedMountArgs arg6 = {
        .srcPath = "system/etc/profile/ABC",
        .destPath = "system/etc/profile/ABC"
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg6);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_60, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("system_basic");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig1 = "{ \
        \"common\":[{ \
            \"app-base\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName_index>\", \
                    \"sandbox-path\" : \"/data/storage/el2/base\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\" \
                }] \
            }], \
            \"app-resources\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
            }], \
            \"create-on-daemon\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
            }], \
            \"app-nocheck\" : [{ \
                \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/mnt/sandbox/shm/<currentUserId>/app/<variablePackageName>\", \
                    \"sandbox-path\" : \"/dev/shm\", \
                    \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                    \"check-action-status\": \"false\", \
                    \"src-path-info\": { \
                        \"uid\": 0, \
                        \"gid\": 0, \
                        \"mode\": 1023 \
                    } \
                }] \
            }] \
        }] \
    }";
    cJSON *j_config1 = cJSON_Parse(mJsconfig1.c_str());
    ASSERT_NE(j_config1, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoSandboxFileCommonBind(appProperty, j_config1);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config1);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_61, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.ohos.wps");
    g_testHelper.SetTestApl("apl123");
    g_testHelper.SetTestMsgFlags(4);  // 4 is test parameter
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig3 = "{ \
        \"flags\": \"DLP_MANAGER\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\" \
    }";
    cJSON *j_config3 = cJSON_Parse(mJsconfig3.c_str());
    ASSERT_NE(j_config3, nullptr);
    int32_t ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config3, nullptr);
    EXPECT_EQ(ret, 0);

    std::string mJsconfig4 = "{ \
        \"flags\": \"DLP_MANAGER\", \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : \"abc\" \
    }";
    cJSON *j_config4 = cJSON_Parse(mJsconfig4.c_str());
    ASSERT_NE(j_config4, nullptr);
    ret = AppSpawn::SandboxCore::DoAllMntPointsMountNocheck(appProperty, j_config4, nullptr);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config3);
    cJSON_Delete(j_config4);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_62, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);  // 1000 test
    g_testHelper.SetTestGid(1000);  // 1000 test
    g_testHelper.SetProcessName("com.example.myapplication");
    g_testHelper.SetTestApl("apl123");
    g_testHelper.SetTestMsgFlags(4);  // 4 is test parameter
    AppSpawningCtx *appProperty = GetTestAppProperty();

    std::string mJsconfig = "{ \
        \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
        \"mount-paths\" : [{ \
            \"src-path\" : \"/data/service/el1/public/themes/<currentUserId>/a/app/createSandboxPath01\", \
            \"sandbox-path\" : \"/data/themes/a/app/createSandboxPath01\", \
            \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
            \"check-action-status\": \"false\", \
            \"create-sandbox-path\": \"false\" \
        }] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    int32_t ret = 0;
    ret = AppSpawn::SandboxCore::DoAllMntPointsMountNocheck(appProperty, j_config, nullptr);
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_63, TestSize.Level0)
{
    int32_t ret = 0;
    SharedMountArgs arg = {
        .srcPath = "system/etc/sudoers/ABC",
        .destPath = "system/etc/sudoers/ABC"
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg);
    EXPECT_EQ(ret, 0);

    SharedMountArgs arg1 = {
        .srcPath = "/system/etc/sudoers/ABC",
        .destPath = "/system/etc/sudoers/ABC"
    };
    ret = AppSpawn::SandboxCommon::DoAppSandboxMountOnceNocheck(nullptr, &arg1);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief 测试app extension
 *
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_AppExtension_004, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    std::string path = AppSpawn::SandboxCommon::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    ASSERT_NE(path.c_str(), nullptr);
    ASSERT_EQ(strcmp(path.c_str(), "/system/com.example.myapplication/module"), 0);
    DeleteAppSpawningCtx(spawningCtx);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_AppExtension_005, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);
    int ret = SetAppSpawnMsgFlag(spawningCtx->message, TLV_MSG_FLAGS, APP_FLAGS_CLONE_ENABLE);
    ASSERT_EQ(ret, 0);

    std::string path = AppSpawn::SandboxCommon::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    ASSERT_NE(path.c_str(), nullptr);   // +clone-bundleIndex+packageName
    ASSERT_EQ(strcmp(path.c_str(), "/system/+clone-100+com.example.myapplication/module"), 0);
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

    std::string path = AppSpawn::SandboxCommon::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    ASSERT_NE(path.c_str(), nullptr);  // +extension-<extensionType>+packageName
    ASSERT_EQ(strcmp(path.c_str(), "/system/+extension-test001+com.example.myapplication/module"), 0);

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

    std::string path = AppSpawn::SandboxCommon::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    ASSERT_NE(path.c_str(), nullptr);   // +clone-bundleIndex+extension-<extensionType>+packageName
    ASSERT_EQ(strcmp(path.c_str(), "/system/+clone-100+extension-test001+com.example.myapplication/module"), 0);

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

    std::string path = AppSpawn::SandboxCommon::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
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

    std::string path = AppSpawn::SandboxCommon::ConvertToRealPath(spawningCtx, "/system/<variablePackageName>/module");
    ASSERT_STREQ(path.c_str(), "");

    DeleteAppSpawningCtx(spawningCtx);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @tc.name: App_Spawn_Sandbox_dec_01
 * @tc.desc: parse config file for dec paths.
 * @tc.type: FUNC
 * @tc.author:
 */
#define DEC_PATH_SIZE 3
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_dec_01, TestSize.Level0)
{
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"\", \
            \"sandbox-path\": \"\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dec-paths\": [ \"/storage/Users\", \"/storage/External\", \"/storage/test\" ] \
        }] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    AppSpawningCtx *appProperty = GetTestAppProperty();
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mntPoint config");
            SandboxMountConfig mountConfig = {0};
            AppSpawn::SandboxCommon::GetSandboxMountConfig(appProperty, "permission", mntPoint, mountConfig);

            int decPathSize = mountConfig.decPaths.size();
            ASSERT_EQ(decPathSize, DEC_PATH_SIZE);
            ret = strcmp(mountConfig.decPaths[0].c_str(), "/storage/Users") ||
                  strcmp(mountConfig.decPaths[1].c_str(), "/storage/External") ||
                  strcmp(mountConfig.decPaths[2].c_str(), "/storage/test");
        }
        EXPECT_EQ(ret, 0);
    } while (0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_dec_02
 * @tc.desc: parse config file for dec paths.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_dec_02, TestSize.Level0)
{
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"\", \
            \"sandbox-path\": \"\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dec-paths\": [ ] \
        }] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    AppSpawningCtx *appProperty = GetTestAppProperty();
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mntPoint config");
            SandboxMountConfig mountConfig = {0};
            AppSpawn::SandboxCommon::GetSandboxMountConfig(appProperty, "permission", mntPoint, mountConfig);

            int decPathSize = mountConfig.decPaths.size();
            EXPECT_EQ(decPathSize, 0);
        }
    } while (0);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_dec_03
 * @tc.desc: parse config file for set policy.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_dec_03, TestSize.Level0)
{
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"\", \
            \"sandbox-path\": \"\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dec-paths\": [ \"/storage/Users\", \"/storage/External\", \"/storage/test\" ] \
        }] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    AppSpawningCtx *appProperty = GetTestAppProperty();
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mntPoint config");
            SandboxMountConfig mountConfig = {0};
            AppSpawn::SandboxCommon::GetSandboxMountConfig(appProperty, "permission", mntPoint, mountConfig);

            int decPathSize = mountConfig.decPaths.size();
            EXPECT_EQ(decPathSize, DEC_PATH_SIZE);
            ret = AppSpawn::SandboxCore::SetDecPolicyWithPermission(appProperty, mountConfig);
        }
        EXPECT_EQ(ret, 0);
    } while (0);
    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_dec_04
 * @tc.desc: parse config file for mount.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_dec_04, TestSize.Level0)
{
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"\", \
            \"sandbox-path\": \"\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dec-paths\": [ \"/storage/Users\", \"/storage/External\", \"/storage/test\" ] \
        }] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    AppSpawningCtx *appProperty = GetTestAppProperty();
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mntPoint config");
            SandboxMountConfig mountConfig = {0};
            AppSpawn::SandboxCommon::GetSandboxMountConfig(appProperty, "permission", mntPoint, mountConfig);

            int decPathSize = mountConfig.decPaths.size();
            EXPECT_EQ(decPathSize, DEC_PATH_SIZE);
        }
        EXPECT_EQ(ret, 0);
    } while (0);
    ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config, nullptr, "permission");
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_dec_05
 * @tc.desc: parse config file for READ_WRITE_DOWNLOAD_DIRECTORY.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_dec_05, TestSize.Level0)
{
    std::string mJsconfig = "{ \
        \"mount-paths\": [{ \
            \"src-path\": \"\", \
            \"sandbox-path\": \"\", \
            \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
            \"dec-paths\": [ \"/storage/Users/<currentUserId>/Download\" ] \
        }] \
    }";
    cJSON *j_config = cJSON_Parse(mJsconfig.c_str());
    ASSERT_NE(j_config, nullptr);
    AppSpawn::SandboxCommon::StoreCJsonConfig(j_config, SandboxCommonDef::SANDBOX_APP_JSON_CONFIG);

    int ret = 0;
    AppSpawningCtx *appProperty = GetTestAppProperty();
    do {
        cJSON *mountPoints = cJSON_GetObjectItemCaseSensitive(j_config, "mount-paths");
        APPSPAWN_CHECK(mountPoints != nullptr || cJSON_IsArray(mountPoints), ret = -1;
            break, "Invalid mountPaths config");

        for (int i = 0; i < cJSON_GetArraySize(mountPoints); ++i) {
            cJSON *mntPoint = cJSON_GetArrayItem(mountPoints, i);
            APPSPAWN_CHECK(mntPoint != nullptr, ret = -2; break, "Invalid mntPoint config");
            SandboxMountConfig mountConfig = {0};
            AppSpawn::SandboxCommon::GetSandboxMountConfig(appProperty, "permission", mntPoint, mountConfig);

            int decPathSize = mountConfig.decPaths.size();
            EXPECT_EQ(decPathSize, 1);
        }
        EXPECT_EQ(ret, 0);
    } while (0);
    ret = AppSpawn::SandboxCore::DoAllMntPointsMount(appProperty, j_config, nullptr, "permission");
    EXPECT_EQ(ret, 0);

    cJSON_Delete(j_config);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_01
 * @tc.desc: [IsValidDataGroupItem] input valid param
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_01, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": "1002",
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": "43200",
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_TRUE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_02
 * @tc.desc: [IsValidDataGroupItem] input valid param in json array
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_02, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"([
        {
            "gid": "1002",
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": "43200",
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        },
        {
            "gid": "1002",
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": "43200",
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    ])";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = false;
    cJSON *child = nullptr;
    cJSON_ArrayForEach(child, j_config) {
        ret = IsValidDataGroupItem(child);
        if (!ret) {
            break;
        }
    }
    EXPECT_TRUE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_03
 * @tc.desc: [IsValidDataGroupItem] input valid param, datagroupId is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_03, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": "1002",
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": 43200,
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_04
 * @tc.desc: [IsValidDataGroupItem] input valid param, gid is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_04, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": 1002,
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": "43200",
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_05
 * @tc.desc: [IsValidDataGroupItem] input valid param, dir is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_05, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": "1002",
            "dir": 100,
            "dataGroupId": "43200",
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_06
 * @tc.desc: [IsValidDataGroupItem] input valid param, uuid is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_06, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": "1002",
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": "43200",
            "uuid": 124
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_07
 * @tc.desc: [IsValidDataGroupItem] input valid param, datagroupId and gid is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_07, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": 1002,
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": 43200,
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_08
 * @tc.desc: [IsValidDataGroupItem] input valid param, datagroupId and dir is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_08, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": "1002",
            "dir": 100,
            "dataGroupId": 43200,
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_09
 * @tc.desc: [IsValidDataGroupItem] input valid param, datagroupId and uuid is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_09, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": "1002",
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": 43200,
            "uuid": 124
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_10
 * @tc.desc: [IsValidDataGroupItem] input valid param, gid and dir is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_10, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": 1002,
            "dir": 100,
            "dataGroupId": "43200",
            "uuid": "49c016e6-065a-abd1-5867-b1f91114f840"
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_11
 * @tc.desc: [IsValidDataGroupItem] input valid param, gid and uuid is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_11, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": 1002,
            "dir": "/data/app/el2/100/group/49c016e6-065a-abd1-5867-b1f91114f840",
            "dataGroupId": "43200",
            "uuid": 124
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_12
 * @tc.desc: [IsValidDataGroupItem] input valid param, dir and uuid is not string type
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_12, TestSize.Level0)
{
    char dataGroupInfoListStr[] = R"(
        {
            "gid": "1002",
            "dir": 100,
            "dataGroupId": "43200",
            "uuid": 124
        }
    )";
    cJSON *j_config = cJSON_Parse(dataGroupInfoListStr);
    ASSERT_NE(j_config, nullptr);
    bool ret = IsValidDataGroupItem(j_config);
    EXPECT_FALSE(ret);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_13
 * @tc.desc: [GetElxInfoFromDir] input valid, the directory contains el2.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_13, TestSize.Level0)
{
    std::string str = "/data/storage/el2/group/";
    int res = GetElxInfoFromDir(str.c_str());
    EXPECT_EQ(res, EL2);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_14
 * @tc.desc: [GetElxInfoFromDir] input valid, the directory contains el3.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_14, TestSize.Level0)
{
    std::string str = "/data/storage/el3/group/";
    int res = GetElxInfoFromDir(str.c_str());
    EXPECT_EQ(res, EL3);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_15
 * @tc.desc: [GetElxInfoFromDir] input valid, the directory contains el4.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_15, TestSize.Level0)
{
    std::string str = "/data/storage/el4/group/";
    int res = GetElxInfoFromDir(str.c_str());
    EXPECT_EQ(res, EL4);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_16
 * @tc.desc: [GetElxInfoFromDir] input valid, the directory contains el5.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_16, TestSize.Level0)
{
    std::string str = "/data/storage/el5/group/";
    int res = GetElxInfoFromDir(str.c_str());
    EXPECT_EQ(res, EL5);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_17
 * @tc.desc: [GetElxInfoFromDir] input invalid, the directory don't contains el2~el5, contains el0.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_17, TestSize.Level0)
{
    std::string str = "/data/storage/el0/group/";
    int res = GetElxInfoFromDir(str.c_str());
    EXPECT_EQ(res, ELX_MAX);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_18
 * @tc.desc: [GetElxInfoFromDir] input invalid, the directory don't contains el2~el5, contains el6.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_18, TestSize.Level0)
{
    std::string str = "/data/storage/el6/group/";
    int res = GetElxInfoFromDir(str.c_str());
    EXPECT_EQ(res, ELX_MAX);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_19
 * @tc.desc: [GetElxInfoFromDir] input invalid, the directory don't contains el2~el5, param is null.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_19, TestSize.Level0)
{
    int res = GetElxInfoFromDir(nullptr);
    EXPECT_EQ(res, ELX_MAX);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_20
 * @tc.desc: [GetDataGroupArgTemplate] input invalid, the category is between el2 and el5, category is EL2.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_20, TestSize.Level0)
{
    const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(EL2);
    ASSERT_EQ(templateItem != nullptr, 1);
    int res = strcmp(templateItem->elxName, "el2");
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_21
 * @tc.desc: [GetDataGroupArgTemplate] input invalid, the category is between el2 and el5, category is EL3.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_21, TestSize.Level0)
{
    const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(EL3);
    ASSERT_EQ(templateItem != nullptr, 1);
    int res = strcmp(templateItem->elxName, "el3");
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_22
 * @tc.desc: [GetDataGroupArgTemplate] input invalid, the category is between el2 and el5, category is EL4.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_22, TestSize.Level0)
{
    const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(EL4);
    ASSERT_EQ(templateItem != nullptr, 1);
    int res = strcmp(templateItem->elxName, "el4");
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_23
 * @tc.desc: [GetDataGroupArgTemplate] input invalid, the category is between el2 and el5, category is EL5.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_23, TestSize.Level0)
{
    const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(EL5);
    ASSERT_EQ(templateItem != nullptr, 1);
    int res = strcmp(templateItem->elxName, "el5");
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name: App_Spawn_Sandbox_Shared_Mount_24
 * @tc.desc: [GetDataGroupArgTemplate] input invalid, the category is between el2 and el5, category is 6.
 * @tc.type: FUNC
 * @tc.author:
 */
HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_Shared_Mount_24, TestSize.Level0)
{
    const DataGroupSandboxPathTemplate *templateItem = GetDataGroupArgTemplate(6);
    int res = -1;
    if (templateItem == nullptr) {
        res = 0;
    }
    EXPECT_EQ(res, 0);
}

HWTEST_F(AppSpawnSandboxTest, App_Spawn_Sandbox_DevModel_001, TestSize.Level0)
{
    AppSpawningCtx* spawningCtx = GetTestAppProperty();
    std::string path = AppSpawn::SandboxCommon::ConvertToRealPath(spawningCtx, "/version/special_cust/<devModel>");
    ASSERT_NE(path.c_str(), nullptr);
    std::string devModelPath =
        "/version/special_cust/" + system::GetParameter(SandboxCommonDef::DEVICE_MODEL_NAME_PARAM, "");

    ASSERT_EQ(strcmp(path.c_str(), devModelPath.c_str()), 0);
    DeleteAppSpawningCtx(spawningCtx);
}

HWTEST_F(AppSpawnSandboxTest, Handle_Flag_Point_PreInstall_Shell_Hap_001, TestSize.Level0)
{
    std::string flagPointConfigStr = "{ \
        \"flags-point\": [ \
            { \
                \"flags\": \"PREINSTALLED_HAP\", \
                \"mount-paths\": [{ \
                    \"src-path\": \"/version/special_cust/app\", \
                    \"sandbox-path\": \"/version/special_cust/app\", \
                    \"sandbox-flags\": [ \
                        \"bind\", \
                        \"rec\" \
                    ], \
                    \"check-action-status\": \"false\" \
                }] \
            }, \
            { \
                \"flags\": \"PREINSTALLED_SHELL_HAP\", \
                \"mount-paths\": [{ \
                    \"src-path\": \"/system/app/HiShell\", \
                    \"sandbox-path\": \"/system/app/HiShell\", \
                    \"sandbox-flags\": [ \
                        \"bind\", \
                        \"rec\" \
                    ], \
                    \"check-action-status\": \"false\" \
                }] \
            } \
        ] \
    }";
    cJSON *flagPointConfig = cJSON_Parse(flagPointConfigStr.c_str());
    AppSpawningCtx *appProperty = GetTestAppProperty();
    ASSERT_EQ(appProperty != nullptr, 1);
    int32_t ret = AppSpawn::SandboxCore::HandleFlagsPoint(appProperty, flagPointConfig);
    EXPECT_EQ(ret, 0);
    DeleteAppSpawningCtx(appProperty);
}

HWTEST_F(AppSpawnSandboxTest, Handle_Flag_Point_PreInstall_Shell_Hap_002, TestSize.Level0)
{
    std::string flagPointConfigStr = "{ \
        \"flags-point\": [ \
            { \
                \"flags\": \"PREINSTALLED_HAP\", \
                \"mount-paths\": [{ \
                    \"src-path\": \"/version/special_cust/app\", \
                    \"sandbox-path\": \"/version/special_cust/app\", \
                    \"sandbox-flags\": [ \
                        \"bind\", \
                        \"rec\" \
                    ], \
                    \"check-action-status\": \"false\" \
                }] \
            }, \
            { \
                \"flags\": \"PREINSTALLED_SHELL_HAP\", \
                \"mount-paths\": [{ \
                    \"src-path\": \"/system/app/HiShell\", \
                    \"sandbox-path\": \"/system/app/HiShell\", \
                    \"sandbox-flags\": [ \
                        \"bind\", \
                        \"rec\" \
                    ], \
                    \"check-action-status\": \"false\" \
                }] \
            } \
        ] \
    }";
    cJSON *flagPointConfig = cJSON_Parse(flagPointConfigStr.c_str());
    AppSpawningCtx *appProperty = GetTestAppProperty();
    ASSERT_EQ(appProperty != nullptr, 1);
    int ret = SetAppSpawnMsgFlag(appProperty->message, TLV_MSG_FLAGS, APP_FLAGS_PRE_INSTALLED_HAP);
    ASSERT_EQ(ret, 0);
    int32_t res = AppSpawn::SandboxCore::HandleFlagsPoint(appProperty, flagPointConfig);
    EXPECT_EQ(res, 0);
    DeleteAppSpawningCtx(appProperty);
}

/**
 * @tc.name: BuildFullParamSrcPathTest_01
 * @tc.desc: Test whether the param-src-path, pre-param-src-path,
 *           and post-param-src-path directories can be concatenated to form a complete src-path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, BuildFullParamSrcPathTest_01, TestSize.Level0)
{
    char paramSrcPathStr[] = R"(
        {
            "pre-param-src-path": "/system",
            "param-src-path": "<ohos.boot.hardware>",
            "post-param-src-path": "/opt",
            "sandbox-path": "/opt"
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);
    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(j_config);
    std::string srcPath = "/system/" + system::GetParameter("ohos.boot.hardware", "") + "/opt";
    EXPECT_EQ(result == srcPath, true);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: BuildFullParamSrcPathTest_02
 * @tc.desc: Test whether the param-src-path, and post-param-src-path,
 *           directories can be concatenated to form a complete src-path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, BuildFullParamSrcPathTest_02, TestSize.Level0)
{
    char paramSrcPathStr[] = R"(
        {
            "param-src-path": "/<ohos.boot.hardware>",
            "post-param-src-path": "/opt",
            "sandbox-path": "/opt"
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);
    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(j_config);
    std::string srcPath = "/<ohos.boot.hardware>/opt";
    EXPECT_EQ(result == srcPath, true);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: BuildFullParamSrcPathTest_03
 * @tc.desc: Test whether the param-src-path, and pre-param-src-path,
 *           directories can be concatenated to form a complete src-path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, BuildFullParamSrcPathTest_03, TestSize.Level0)
{
    char paramSrcPathStr[] = R"(
        {
            "pre-param-src-path": "/system",
            "param-src-path": "<ohos.boot.hardware>",
            "sandbox-path": "/opt"
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);
    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(j_config);
    std::string srcPath = "/system/" + system::GetParameter("ohos.boot.hardware", "");
    EXPECT_EQ(result == srcPath, true);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: BuildFullParamSrcPathTest_04
 * @tc.desc: Test whether the param-src-path,
 *           directories can be concatenated to form a complete src-path
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, BuildFullParamSrcPathTest_04, TestSize.Level0)
{
    char paramSrcPathStr[] = R"(
        {
            "param-src-path": "/<ohos.boot.hardware>",
            "sandbox-path": "/opt"
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);
    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(j_config);
    std::string srcPath = "/<ohos.boot.hardware>";
    EXPECT_EQ(result == srcPath, true);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: BuildFullParamSrcPathTest_05
 * @tc.desc: Test whether the complete src-path is generated when the param-src-path directories is not configured.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, BuildFullParamSrcPathTest_05, TestSize.Level0)
{
    char paramSrcPathStr[] = R"(
        {
            "param-src-path": "",
            "sandbox-path": "/opt"
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);
    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(j_config);
    EXPECT_EQ(result.empty(), true);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: BuildFullParamSrcPathTest_06
 * @tc.desc: Test whether the complete src-path is generated when the param-src-path directories is not configured.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, BuildFullParamSrcPathTest_06, TestSize.Level0)
{
    char paramSrcPathStr[] = R"(
        {
            "sandbox-path": "/opt"
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);
    std::string result = AppSpawn::SandboxCommon::BuildFullParamSrcPath(j_config);
    EXPECT_EQ(result.empty(), true);

    cJSON_Delete(j_config);
}

/**
 * @tc.name: JoinParamPathsTest_01
 * @tc.desc: All paths in the paths list are empty.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, JoinParamPathsTest_01, TestSize.Level0)
{
    std::vector<std::string> paths = {"", "", ""};
    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    EXPECT_EQ(result.empty(), true);
}

/**
 * @tc.name: JoinParamPathsTest_02
 * @tc.desc: All paths in the paths list start with '/'.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, JoinParamPathsTest_02, TestSize.Level0)
{
    std::vector<std::string> paths = {"/system", "/test", "/opt"};
    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    std::string srcPath = "/system/test/opt";
    EXPECT_EQ(result == srcPath, true);
}

/**
 * @tc.name: JoinParamPathsTest_03
 * @tc.desc: In paths, a slash(/) is carried at the end of path.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, JoinParamPathsTest_03, TestSize.Level0)
{
    std::vector<std::string> paths = {"/system/", "/test/", "/opt/"};
    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    std::string srcPath = "/system/test/opt/";
    EXPECT_EQ(result == srcPath, true);
}

/**
 * @tc.name: JoinParamPathsTest_04
 * @tc.desc: In paths, a slash(/) is carried at the end of path.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, JoinParamPathsTest_04, TestSize.Level0)
{
    std::vector<std::string> paths = {"/system/", "test/", "opt/"};
    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    std::string srcPath = "/system/test/opt/";
    EXPECT_EQ(result == srcPath, true);
}

/**
 * @tc.name: JoinParamPathsTest_05
 * @tc.desc: The paths in the path list contain ''..''
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, JoinParamPathsTest_05, TestSize.Level0)
{
    std::vector<std::string> paths = {"/system/..", "test", "opt"};
    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    EXPECT_EQ(result.empty(), true);
}

/**
 * @tc.name: JoinParamPathsTest_06
 * @tc.desc: Path is the paths list do not start or end with '/'
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, JoinParamPathsTest_06, TestSize.Level0)
{
    std::vector<std::string> paths = {"system", "test", "opt"};
    std::string result = AppSpawn::SandboxCommon::JoinParamPaths(paths);
    std::string srcPath = "system/test/opt";
    EXPECT_EQ(result == srcPath, true);
}

/**
 * @tc.name: ParseParamTemplateTest_01
 * @tc.desc: The templateStr string carried <>
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, ParseParamTemplateTest_01, TestSize.Level0)
{
    std::string templateStr = "<>";
    std::string result = AppSpawn::SandboxCommon::ParseParamTemplate(templateStr);
    std::string srcPath = "system/test/opt/";
    EXPECT_EQ(result == templateStr, true);
}

/**
 * @tc.name: ParseParamTemplateTest_02
 * @tc.desc: The templateStr string carried <system-param>
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, ParseParamTemplateTest_02, TestSize.Level0)
{
    std::string templateStr = "<ohos.boot.hardware>";
    std::string result = AppSpawn::SandboxCommon::ParseParamTemplate(templateStr);
    std::string srcPath = system::GetParameter("ohos.boot.hardware", "");
    EXPECT_EQ(result == srcPath, true);
}

/**
 * @tc.name: ParseParamTemplateTest_03
 * @tc.desc: The templateStr string carried <system-param>, and the value obtained through GetParameter is "".
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, ParseParamTemplateTest_03, TestSize.Level0)
{
    std::string templateStr = "<test.ParseParamTemplateTest_05>";
    std::string result = AppSpawn::SandboxCommon::ParseParamTemplate(templateStr);
    EXPECT_EQ(result == templateStr, true);
}

/**
 * @tc.name: ProcessMountPointTest_01
 * @tc.desc: The obtained srcPathChr value is nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, ProcessMountPointTest_01, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetProcessName("ohos.sample.test");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    char paramSrcPathStr[] = R"(
        {
            "param-src-path": "<ohos.boot.hardware>",
            "sandbox-path": "/opt",
            "sandbox-flags": [ "bind", "rec" ]
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);

    MountPointProcessParams mountPointParams = {
        .appProperty = appProperty,
        .checkFlag = false,
        .section = "",
        .sandboxRoot = "/mnt/sandbox",
        .bundleName = "ohos.sample.test"
    };

    int32_t result = AppSpawn::SandboxCore::ProcessMountPoint(j_config, mountPointParams);
    EXPECT_EQ(result, 0);
    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(j_config);
}

/**
 * @tc.name: ProcessMountPointTest_02
 * @tc.desc: The obtained srcPathChr value is nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, ProcessMountPointTest_02, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetProcessName("ohos.sample.test");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    char paramSrcPathStr[] = R"(
        {
            "param-src-path": "",
            "sandbox-path": "/opt",
            "sandbox-flags": [ "bind", "rec" ]
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);

    MountPointProcessParams mountPointParams = {
        .appProperty = appProperty,
        .checkFlag = false,
        .section = "",
        .sandboxRoot = "/mnt/sandbox",
        .bundleName = "ohos.sample.test"
    };

    int32_t result = AppSpawn::SandboxCore::ProcessMountPoint(j_config, mountPointParams);
    EXPECT_EQ(result, 0);
    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(j_config);
}

/**
 * @tc.name: ProcessMountPointTest_03
 * @tc.desc: The obtained srcPathChr and param-src-path value is nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, ProcessMountPointTest_03, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetProcessName("ohos.sample.test");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    char paramSrcPathStr[] = R"(
        {
            "sandbox-path": "/opt",
            "sandbox-flags": [ "bind", "rec" ]
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);

    MountPointProcessParams mountPointParams = {
        .appProperty = appProperty,
        .checkFlag = false,
        .section = "",
        .sandboxRoot = "/mnt/sandbox",
        .bundleName = "ohos.sample.test"
    };

    int32_t result = AppSpawn::SandboxCore::ProcessMountPoint(j_config, mountPointParams);
    EXPECT_EQ(result, 0);
    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(j_config);
}

/**
 * @tc.name: ProcessMountPointTest_04
 * @tc.desc: The obtained sandboxPathChr value is nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, ProcessMountPointTest_04, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetProcessName("ohos.sample.test");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    char paramSrcPathStr[] = R"(
        {
            "param-src-path": "<ohos.boot.hardware>",
            "sandbox-flags": [ "bind", "rec" ]
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);

    MountPointProcessParams mountPointParams = {
        .appProperty = appProperty,
        .checkFlag = false,
        .section = "",
        .sandboxRoot = "/mnt/sandbox",
        .bundleName = "ohos.sample.test"
    };

    int32_t result = AppSpawn::SandboxCore::ProcessMountPoint(j_config, mountPointParams);
    EXPECT_EQ(result, 0);
    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(j_config);
}

/**
 * @tc.name: ProcessMountPointTest_05
 * @tc.desc: The obtained all path value is nullptr.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, ProcessMountPointTest_05, TestSize.Level0)
{
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetProcessName("ohos.sample.test");
    AppSpawningCtx *appProperty = GetTestAppProperty();

    char paramSrcPathStr[] = R"(
        {
            "src-path": "",
            "param-src-path": "<ohos.boot.hardware>",
            "sandbox-path": "",
            "sandbox-flags": [ "bind", "rec" ]
        }
    )";

    cJSON *j_config = cJSON_Parse(paramSrcPathStr);
    ASSERT_NE(j_config, nullptr);

    MountPointProcessParams mountPointParams = {
        .appProperty = appProperty,
        .checkFlag = false,
        .section = "",
        .sandboxRoot = "/mnt/sandbox",
        .bundleName = "ohos.sample.test"
    };

    int32_t result = AppSpawn::SandboxCore::ProcessMountPoint(j_config, mountPointParams);
    EXPECT_EQ(result, 0);
    DeleteAppSpawningCtx(appProperty);
    cJSON_Delete(j_config);
}

/**
 * @tc.name: Handle_Dlp_Mount_01
 * @tc.desc: Test mounting the fuse directory in dlpmanager.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, Handle_Dlp_Mount_01, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);
    AppSpawnMsgDacInfo *dacInfo = reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(spawningCtx, TLV_DAC_INFO));
    ASSERT_EQ(dacInfo != nullptr, 1);

    int32_t ret = AppSpawn::SandboxCore::HandleDlpMount(dacInfo);

    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @tc.name: Check_Dlp_Mount_01
 * @tc.desc: Verify whether it is necessary to execute the mount of dlpmanager, when is dlpmanager.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, Check_Dlp_Mount_01, TestSize.Level0)
{
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);

    bool ret = AppSpawn::SandboxCore::CheckDlpMount(spawningCtx);
    EXPECT_EQ(ret, true);

    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @tc.name: Check_Dlp_Mount_02
 * @tc.desc: Verify whether it is necessary to execute the mount of dlpmanager, when is not dlpmanager.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, Check_Dlp_Mount_02, TestSize.Level0)
{
    g_testHelper.SetProcessName("com.ohos.notepad");
    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);

    bool ret = AppSpawn::SandboxCore::CheckDlpMount(spawningCtx);
    EXPECT_EQ(ret, false);

    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @tc.name: Set_App_Sandbox_Property_For_Dlp_01
 * @tc.desc: When dlpStatus is true and CheckDlpMount is true.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, Set_App_Sandbox_Property_For_Dlp_01, TestSize.Level0)
{
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetProcessName("com.ohos.dlpmanager");
    std::vector<const char *> &permissions = g_testHelper.GetPermissions();
    permissions.push_back("ohos.permission.ACCESS_DLP_FILE");

    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);
    bool ret = AppSpawn::SandboxCore::SetAppSandboxProperty(spawningCtx, CLONE_NEWPID);
    EXPECT_EQ(ret, false);

    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @tc.name: Set_App_Sandbox_Property_For_Dlp_02
 * @tc.desc: When dlpStatus is true and CheckDlpMount is false.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, Set_App_Sandbox_Property_For_Dlp_02, TestSize.Level0)
{
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetProcessName("com.ohos.notepad");
    std::vector<const char *> &permissions = g_testHelper.GetPermissions();
    permissions.push_back("ohos.permission.ACCESS_DLP_FILE");

    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);
    bool ret = AppSpawn::SandboxCore::SetAppSandboxProperty(spawningCtx, CLONE_NEWPID);
    EXPECT_EQ(ret, false);

    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @tc.name: Set_App_Sandbox_Property_For_Dlp_03
 * @tc.desc: When dlpStatus is false and CheckDlpMount is true.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, Set_App_Sandbox_Property_For_Dlp_03, TestSize.Level0)
{
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetProcessName("com.ohos.dlpmanager");

    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);
    bool ret = AppSpawn::SandboxCore::SetAppSandboxProperty(spawningCtx, CLONE_NEWPID);
    EXPECT_EQ(ret, false);

    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @tc.name: Set_App_Sandbox_Property_For_Dlp_04
 * @tc.desc: When dlpStatus is false and CheckDlpMount is true.
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnSandboxTest, Set_App_Sandbox_Property_For_Dlp_04, TestSize.Level0)
{
    g_testHelper.SetTestGid(1000);
    g_testHelper.SetTestUid(1000);
    g_testHelper.SetProcessName("com.ohos.notepad");

    AppSpawningCtx *spawningCtx = GetTestAppProperty();
    ASSERT_EQ(spawningCtx != nullptr, 1);
    bool ret = AppSpawn::SandboxCore::SetAppSandboxProperty(spawningCtx, CLONE_NEWPID);
    EXPECT_EQ(ret, false);

    DeleteAppSpawningCtx(spawningCtx);
}
}  // namespace OHOS
