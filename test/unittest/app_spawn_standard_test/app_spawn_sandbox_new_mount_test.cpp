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

#include <cerrno>
#include <cstdbool>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include "appspawn_manager.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_server.h"
#include "appspawn_utils.h"
#include "cJSON.h"
#include "json_utils.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
static AppSpawnTestHelper g_testHelper;
class AppSpawnSandboxCoverageTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase()
    {
        StubNode *stub = GetStubNode(STUB_MOUNT);
        if (stub) {
            stub->flags &= ~STUB_NEED_CHECK;
        }
    }
    void SetUp() {}
    void TearDown() {}
};

static AppSpawningCtx *TestCreateAppSpawningCtx()
{
    AppSpawnClientHandle clientHandle = nullptr;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    APPSPAWN_CHECK(ret == 0, return nullptr, "Failed to create reqMgr");
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, return nullptr, "Failed to create req");
    return g_testHelper.GetAppProperty(clientHandle, reqHandle);
}

static SandboxContext *TestGetSandboxContext(const AppSpawningCtx *property, int nwebspawn)
{
    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppProperty(property, TLV_MSG_FLAGS);
    APPSPAWN_CHECK(msgFlags != nullptr, return nullptr, "No msg flags in msg %{public}s", GetProcessName(property));

    SandboxContext *context = GetSandboxContext();
    APPSPAWN_CHECK(context != nullptr, return nullptr, "Failed to get context");

    context->nwebspawn = nwebspawn;
    context->bundleName = GetBundleName(property);
    context->bundleHasWps = strstr(context->bundleName, "wps") != nullptr;
    context->dlpBundle = strstr(context->bundleName, "com.ohos.dlpmanager") != nullptr;
    context->appFullMountEnable = 0;
    context->dlpUiExtType = strstr(GetProcessName(property), "sys/commonUI") != nullptr;

    context->sandboxSwitch = 1;
    context->sandboxShared = false;
    context->message = property->message;
    context->rootPath = nullptr;
    context->rootPath = nullptr;
    return context;
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_ProcessExpandAppSandboxConfig, TestSize.Level0)
{
    int ret = ProcessExpandAppSandboxConfig(nullptr, nullptr, nullptr);
    ASSERT_NE(ret, 0);
    const SandboxContext context = {};
    const AppSpawnSandboxCfg sandboxCfg = {};
    ret = ProcessExpandAppSandboxConfig(&context, nullptr, nullptr);
    ASSERT_NE(ret, 0);
    ret = ProcessExpandAppSandboxConfig(nullptr, &sandboxCfg, nullptr);
    ASSERT_NE(ret, 0);
    ret = ProcessExpandAppSandboxConfig(&context, &sandboxCfg, nullptr);
    ASSERT_NE(ret, 0);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllHsp, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    char testJson[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    const cJSON *config = cJSON_Parse(testJson);
    int ret = MountAllHsp(nullptr, nullptr);
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(context, nullptr);
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(nullptr, config);
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(context, config);
    ASSERT_EQ(ret, 0);
    char testJson1[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\"], \
        \"versions\":[\"v10001\"] \
    }";
    const cJSON *config1 = cJSON_Parse(testJson1);
    ret = MountAllHsp(context, config1); // bundles count != modules
    ASSERT_EQ(ret, -1);

    char testJson2[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\"] \
    }";
    const cJSON *config2 = cJSON_Parse(testJson2);
    ret = MountAllHsp(context, config2); // bundles count != versions
    ASSERT_EQ(ret, -1);

    char testJson3[] = "{ \
        \"bundles\":[\".\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    const cJSON *config3 = cJSON_Parse(testJson3);
    ret = MountAllHsp(context, config3);
    ASSERT_EQ(ret, -1);

    char testJson4[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"..\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    const cJSON *config4 = cJSON_Parse(testJson4);
    ret = MountAllHsp(context, config4);
    ASSERT_EQ(ret, -1);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllHsp_001, TestSize.Level0)
{
    const SandboxContext context = {};
    char testJson1[] = "{ \
        \"text\":\"test.bundle1\"\
    }";
    char testJson2[] = "{ \
        \"bundles\":\"test.bundle1\"\
    }";
    const cJSON *config1 = cJSON_Parse(testJson1);
    const cJSON *config2 = cJSON_Parse(testJson2);
    int ret = MountAllHsp(&context, config1); // bundles is null
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(&context, config2); // bundles not Array
    ASSERT_EQ(ret, -1);

    char testJson3[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"test\":\"module1\" \
    }";
    char testJson4[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":\"module1\" \
    }";
    const cJSON *config3 = cJSON_Parse(testJson3);
    const cJSON *config4 = cJSON_Parse(testJson4);
    ret = MountAllHsp(&context, config3); // modules is null
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(&context, config4); // bundles not Array
    ASSERT_EQ(ret, -1);

    char testJson5[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"test\":\"v10001\" \
    }";
    char testJson6[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":\"v10001\" \
    }";
    const cJSON *config5 = cJSON_Parse(testJson5);
    const cJSON *config6 = cJSON_Parse(testJson6);
    ret = MountAllHsp(&context, config5); // versions is null
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(&context, config6); // versions not Array
    ASSERT_EQ(ret, -1);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllGroup, TestSize.Level0)
{
    int ret = MountAllGroup(nullptr, nullptr);
    ASSERT_EQ(ret, -1);

    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char testDataGroupStr[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"dir\":[\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\", \
                    \"/data/app/el2/100/group/ce876162-fe69-45d3-aa8e-411a047af564\"], \
        \"gid\":[\"20100001\", \"20100002\"] \
    }";
    const cJSON *config = cJSON_Parse(testDataGroupStr);
    ret = MountAllGroup(context, nullptr);
    ASSERT_EQ(ret, -1);
    ret = MountAllGroup(nullptr, config);
    ASSERT_EQ(ret, -1);
    ret = MountAllGroup(context, config);
    ASSERT_EQ(ret, 0);

    const char testDataGroupStr8[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"dir\":[\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\"], \
        \"gid\":[\"20100001\"] \
    }";
    const cJSON *config8 = cJSON_Parse(testDataGroupStr8);
    ret = MountAllGroup(context, config8); // gid count != dataGroupId
    ASSERT_EQ(ret, -1);

    const char testDataGroupStr9[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"dir\":[\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\"], \
        \"gid\":[\"20100001\", \"20100002\"] \
    }";
    const cJSON *config9 = cJSON_Parse(testDataGroupStr9);
    ret = MountAllGroup(context, config9); // dir count != dataGroupId
    ASSERT_EQ(ret, -1);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllGroup_001, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char testDataGroupStr1[] = "{ \
        \"test\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
    }";
    const cJSON *config1 = cJSON_Parse(testDataGroupStr1);
    int ret = MountAllGroup(context, config1); // dataGroupIds is null
    ASSERT_EQ(ret, -1);

    const char testDataGroupStr2[] = "{ \
        \"dataGroupId\":\"1234abcd5678efgh\", \
    }";
    const cJSON *config2 = cJSON_Parse(testDataGroupStr2);
    ret = MountAllGroup(context, config2); // dataGroupIds is not Array
    ASSERT_EQ(ret, -1);

    const char testDataGroupStr3[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"test\":[\"20100001\", \"20100002\"] \
    }";
    const cJSON *config3 = cJSON_Parse(testDataGroupStr3);
    ret = MountAllGroup(context, config3); // gid is null
    ASSERT_EQ(ret, -1);

    const char testDataGroupStr4[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"gid\":\"20100001\" \
    }";
    const cJSON *config4 = cJSON_Parse(testDataGroupStr4);
    ret = MountAllGroup(context, config4); // gid is not Array
    ASSERT_EQ(ret, -1);

    const char testDataGroupStr5[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"test\":[\"/data/app/el2/100/group/ce876162-fe69-45d3-aa8e-411a047af564\"], \
        \"gid\":[\"20100001\", \"20100002\"] \
    }";
    const cJSON *config5 = cJSON_Parse(testDataGroupStr5);
    ret = MountAllGroup(context, config5); // dir is null
    ASSERT_EQ(ret, -1);

    const char testDataGroupStr6[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"dir\":\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\", \
        \"gid\":[\"20100001\", \"20100002\"] \
    }";
    const cJSON *config6 = cJSON_Parse(testDataGroupStr6);
    ret = MountAllGroup(context, config6); // dir is not Array
    ASSERT_EQ(ret, -1);
}
}