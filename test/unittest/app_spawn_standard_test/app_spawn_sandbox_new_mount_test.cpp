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

#define MAX_BUFF 20
#define TEST_STR_LEN 30

extern "C" {
    void DumpSandboxMountNode(const SandboxMountNode *sandboxNode, uint32_t index);
}

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
    context->dlpBundle = strcmp(GetProcessName(property), "com.ohos.dlpmanager") == 0;
    context->appFullMountEnable = 0;

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

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllHsp_001, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    char testJson[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    cJSON *config = cJSON_Parse(testJson);
    ASSERT_NE(config, nullptr);
    int ret = MountAllHsp(nullptr, nullptr);
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(context, nullptr);
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(nullptr, config);
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(context, config);
    ASSERT_EQ(ret, 0);
    cJSON_Delete(config);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllHsp_002, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    char testJson1[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\"], \
        \"versions\":[\"v10001\"] \
    }";
    cJSON *config1 = cJSON_Parse(testJson1);
    ASSERT_NE(config1, nullptr);
    int ret = MountAllHsp(context, config1); // bundles count != modules
    cJSON_Delete(config1);
    ASSERT_EQ(ret, -1);

    char testJson2[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\"] \
    }";
    cJSON *config2 = cJSON_Parse(testJson2);
    ASSERT_NE(config2, nullptr);
    ret = MountAllHsp(context, config2); // bundles count != versions
    cJSON_Delete(config2);
    ASSERT_EQ(ret, -1);

    char testJson3[] = "{ \
        \"bundles\":[\".\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    cJSON *config3 = cJSON_Parse(testJson3);
    ASSERT_NE(config3, nullptr);
    ret = MountAllHsp(context, config3);
    cJSON_Delete(config3);
    ASSERT_EQ(ret, -1);

    char testJson4[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"..\", \"module2\"], \
        \"versions\":[\"v10001\", \"v10002\"] \
    }";
    cJSON *config4 = cJSON_Parse(testJson4);
    ASSERT_NE(config4, nullptr);
    ret = MountAllHsp(context, config4);
    cJSON_Delete(config4);
    ASSERT_EQ(ret, -1);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllHsp_003, TestSize.Level0)
{
    const SandboxContext context = {};
    char testJson1[] = "{ \
        \"text\":\"test.bundle1\"\
    }";
    char testJson2[] = "{ \
        \"bundles\":\"test.bundle1\"\
    }";
    cJSON *config1 = cJSON_Parse(testJson1);
    ASSERT_NE(config1, nullptr);
    cJSON *config2 = cJSON_Parse(testJson2);
    ASSERT_NE(config2, nullptr);
    int ret = MountAllHsp(&context, config1); // bundles is null
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(&context, config2); // bundles not Array
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config1);
    cJSON_Delete(config2);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllHsp_004, TestSize.Level0)
{
    const SandboxContext context = {};
    char testJson1[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"test\":\"module1\" \
    }";
    char testJson2[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":\"module1\" \
    }";
    cJSON *config1 = cJSON_Parse(testJson1);
    ASSERT_NE(config1, nullptr);
    cJSON *config2 = cJSON_Parse(testJson2);
    ASSERT_NE(config2, nullptr);
    int ret = MountAllHsp(&context, config1); // modules is null
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(&context, config2); // bundles not Array
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config1);
    cJSON_Delete(config2);

    char testJson3[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"test\":\"v10001\" \
    }";
    char testJson4[] = "{ \
        \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
        \"modules\":[\"module1\", \"module2\"], \
        \"versions\":\"v10001\" \
    }";
    cJSON *config3 = cJSON_Parse(testJson3);
    ASSERT_NE(config3, nullptr);
    cJSON *config4 = cJSON_Parse(testJson4);
    ASSERT_NE(config4, nullptr);
    ret = MountAllHsp(&context, config3); // versions is null
    ASSERT_EQ(ret, -1);
    ret = MountAllHsp(&context, config4); // versions not Array
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config3);
    cJSON_Delete(config4);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllGroup, TestSize.Level0)
{
    int ret = MountAllGroup(nullptr, nullptr);
    ASSERT_EQ(ret, -1);

    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char testDataGroupStr1[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"dir\":[\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\", \
                    \"/data/app/el2/100/group/ce876162-fe69-45d3-aa8e-411a047af564\"], \
        \"gid\":[\"20100001\", \"20100002\"] \
    }";
    cJSON *config1 = cJSON_Parse(testDataGroupStr1);
    ASSERT_NE(config1, nullptr);
    ret = MountAllGroup(context, nullptr);
    ASSERT_EQ(ret, -1);
    ret = MountAllGroup(nullptr, config1);
    ASSERT_EQ(ret, -1);
    ret = MountAllGroup(context, config1);
    ASSERT_EQ(ret, 0);
    cJSON_Delete(config1);

    const char testDataGroupStr2[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"dir\":[\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\"], \
        \"gid\":[\"20100001\"] \
    }";
    cJSON *config2 = cJSON_Parse(testDataGroupStr2);
    ASSERT_NE(config2, nullptr);
    ret = MountAllGroup(context, config2); // gid count != dataGroupId
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config2);

    const char testDataGroupStr3[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"dir\":[\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\"], \
        \"gid\":[\"20100001\", \"20100002\"] \
    }";
    cJSON *config3 = cJSON_Parse(testDataGroupStr3);
    ASSERT_NE(config3, nullptr);
    ret = MountAllGroup(context, config3); // dir count != dataGroupId
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config3);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllGroup_001, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char testDataGroupStr1[] = "{ \
        \"test\":[\"1234abcd5678efgh\", \"abcduiop1234\"] \
    }";
    cJSON *config1 = cJSON_Parse(testDataGroupStr1);
    ASSERT_NE(config1, nullptr);
    int ret = MountAllGroup(context, config1); // dataGroupIds is null
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config1);

    const char testDataGroupStr2[] = "{ \
        \"dataGroupId\":\"1234abcd5678efgh\" \
    }";
    cJSON *config2 = cJSON_Parse(testDataGroupStr2);
    ASSERT_NE(config2, nullptr);
    ret = MountAllGroup(context, config2); // dataGroupIds is not Array
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config2);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_MountAllGroup_002, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char testDataGroupStr1[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"test\":[\"20100001\", \"20100002\"] \
    }";
    cJSON *config1 = cJSON_Parse(testDataGroupStr1);
    ASSERT_NE(config1, nullptr);
    int ret = MountAllGroup(context, config1); // gid is null
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config1);

    const char testDataGroupStr2[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"gid\":\"20100001\" \
    }";
    cJSON *config2 = cJSON_Parse(testDataGroupStr2);
    ASSERT_NE(config2, nullptr);
    ret = MountAllGroup(context, config2); // gid is not Array
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config2);

    const char testDataGroupStr3[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"test\":[\"/data/app/el2/100/group/ce876162-fe69-45d3-aa8e-411a047af564\"], \
        \"gid\":[\"20100001\", \"20100002\"] \
    }";
    cJSON *config3 = cJSON_Parse(testDataGroupStr3);
    ASSERT_NE(config3, nullptr);
    ret = MountAllGroup(context, config3); // dir is null
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config3);

    const char testDataGroupStr4[] = "{ \
        \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
        \"dir\":\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\", \
        \"gid\":[\"20100001\", \"20100002\"] \
    }";
    cJSON *config4 = cJSON_Parse(testDataGroupStr4);
    ASSERT_NE(config4, nullptr);
    ret = MountAllGroup(context, config4); // dir is not Array
    ASSERT_EQ(ret, -1);
    cJSON_Delete(config4);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_cfgvar, TestSize.Level0)
{
    SandboxContext context = {};
    context.bundleName = "com.xxx.xxx.xxx";
    char buffer[MAX_BUFF] = {};
    uint32_t realLen = 0;
    int ret = VarPackageNameReplace(&context, buffer, MAX_BUFF, &realLen, nullptr);
    EXPECT_EQ(ret, 0);

    context.bundleName = "com.xxxxxxx.xxxxxxx.xxxxxxx";
    ret = VarPackageNameReplace(&context, buffer, MAX_BUFF, &realLen, nullptr);
    EXPECT_EQ(ret, -1);

    VarExtraData extraData = {};
    ret = ReplaceVariableForDepSandboxPath(nullptr, buffer, MAX_BUFF,  &realLen, nullptr);
    EXPECT_EQ(ret, -1);
    ret = ReplaceVariableForDepSrcPath(nullptr, buffer, MAX_BUFF,  &realLen, nullptr);
    EXPECT_EQ(ret, -1);
    ret = ReplaceVariableForDepSandboxPath(nullptr, buffer, MAX_BUFF,  &realLen, &extraData);
    EXPECT_EQ(ret, -1);
    ret = ReplaceVariableForDepSrcPath(nullptr, buffer, MAX_BUFF,  &realLen, &extraData);
    EXPECT_EQ(ret, -1);
    extraData.data.depNode = (PathMountNode *)malloc(sizeof(PathMountNode));
    ASSERT_EQ(extraData.data.depNode != nullptr, 1);

    extraData.data.depNode->target = (char*)malloc(sizeof(char) * TEST_STR_LEN);
    ASSERT_EQ(extraData.data.depNode->target != nullptr, 1);
    ret = strcpy_s(extraData.data.depNode->target, TEST_STR_LEN, "/xxxx/xxxx/xxxxxxx/xxxxxx/xxx");
    EXPECT_EQ(ret, 0);
    ret = ReplaceVariableForDepSandboxPath(nullptr, buffer, MAX_BUFF,  &realLen, &extraData);
    EXPECT_EQ(ret, -1);
    ret = strcpy_s(extraData.data.depNode->target, TEST_STR_LEN, "/xxxx/xxxx/xxxxxx");
    EXPECT_EQ(ret, 0);
    ret = ReplaceVariableForDepSandboxPath(nullptr, buffer, MAX_BUFF,  &realLen, &extraData);
    EXPECT_EQ(ret, 0);

    extraData.data.depNode->source = (char*)malloc(sizeof(char) * TEST_STR_LEN);
    ASSERT_EQ(extraData.data.depNode->source != nullptr, 1);
    ret = strcpy_s(extraData.data.depNode->source, TEST_STR_LEN, "/xxxx/xxxx/xxxxxxx/xxxxxx/xxx");
    EXPECT_EQ(ret, 0);
    ret = ReplaceVariableForDepSrcPath(nullptr, buffer, MAX_BUFF,  &realLen, &extraData);
    EXPECT_EQ(ret, -1);
    ret = strcpy_s(extraData.data.depNode->source, TEST_STR_LEN, "/xxxx/xxxx/xxxxxx");
    EXPECT_EQ(ret, 0);
    ret = ReplaceVariableForDepSrcPath(nullptr, buffer, MAX_BUFF,  &realLen, &extraData);
    EXPECT_EQ(ret, 0);
    free(extraData.data.depNode->target);
    free(extraData.data.depNode->source);
    free(extraData.data.depNode);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_cfgvar_001, TestSize.Level0)
{
    char buffer[MAX_BUFF] = {};
    uint32_t realLen = 0;
    VarExtraData extraData = {};
    int ret = ReplaceVariableForDepPath(nullptr, buffer, MAX_BUFF, &realLen, nullptr);
    EXPECT_EQ(ret, -1);
    ret = ReplaceVariableForDepPath(nullptr, buffer, MAX_BUFF, &realLen, &extraData);
    EXPECT_EQ(ret, -1);
    extraData.data.depNode = (PathMountNode *)malloc(sizeof(PathMountNode));
    ASSERT_EQ(extraData.data.depNode != nullptr, 1);
    extraData.data.depNode->source = (char*)malloc(sizeof(char) * TEST_STR_LEN);
    EXPECT_EQ(extraData.data.depNode->source != nullptr, 1);

    ret = ReplaceVariableForDepPath(nullptr, buffer, MAX_BUFF, &realLen, &extraData);
    EXPECT_EQ(ret, 0);
    extraData.operation = 0x1 << MOUNT_PATH_OP_REPLACE_BY_SRC;
    ret = ReplaceVariableForDepPath(nullptr, buffer, MAX_BUFF, &realLen, &extraData);
    EXPECT_EQ(ret, 0);

    ret = strcpy_s(extraData.data.depNode->source, TEST_STR_LEN, "/xxxx/xxxx/xxxxxxx/xxxxxx/xxx");
    EXPECT_EQ(ret, 0);
    ret = ReplaceVariableForDepPath(nullptr, buffer, MAX_BUFF, &realLen, &extraData);
    EXPECT_EQ(ret, -1);

    ret = strcpy_s(extraData.data.depNode->source, TEST_STR_LEN, "/xxxx/xxxx/xxxxxx");
    EXPECT_EQ(ret, 0);
    ret = ReplaceVariableForDepPath(nullptr, buffer, MAX_BUFF, &realLen, &extraData);
    EXPECT_EQ(ret, 0);
    free(extraData.data.depNode->source);
    free(extraData.data.depNode);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_cfgvar_002, TestSize.Level0)
{
    const char *realVar = GetSandboxRealVar(nullptr, 0, nullptr, nullptr, nullptr);
    EXPECT_EQ(realVar == nullptr, 1);

    SandboxContext context = {};
    realVar = GetSandboxRealVar(&context, 0, nullptr, nullptr, nullptr);
    EXPECT_EQ(realVar == nullptr, 1);

    context.buffer[0].buffer = (char*)malloc(sizeof(char) * MAX_BUFF);
    ASSERT_EQ(context.buffer[0].buffer != nullptr, 1);
    int ret = strcpy_s(context.buffer[0].buffer, MAX_BUFF, "xxxxxxxx");
    EXPECT_EQ(ret, 0);
    context.buffer[0].bufferLen = MAX_BUFF;
    context.buffer[0].current = 0;
    realVar = GetSandboxRealVar(&context, 0, nullptr, nullptr, nullptr);
    EXPECT_EQ(realVar != nullptr, 1);
    realVar = GetSandboxRealVar(&context, 4, nullptr, nullptr, nullptr);
    EXPECT_EQ(realVar == nullptr, 1);
    realVar = GetSandboxRealVar(&context, 0, nullptr, "test/xxxx", nullptr);
    EXPECT_EQ(realVar != nullptr, 1);
    realVar = GetSandboxRealVar(&context, 0, "xxxx/xxx", nullptr, nullptr);
    EXPECT_EQ(realVar != nullptr, 1);
    realVar = GetSandboxRealVar(&context, 0, "xxxx/xxx", "test/xxxx", nullptr);
    EXPECT_EQ(realVar != nullptr, 1);
    GetSandboxRealVar(&context, 1, nullptr, nullptr, nullptr);
    EXPECT_EQ(realVar != nullptr, 1);
    free(context.buffer[0].buffer);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_manager, TestSize.Level0)
{
    int ret = SpawnPrepareSandboxCfg(nullptr, nullptr);
    EXPECT_EQ(ret, -1);

    AppSpawnMgr content = {};
    ret = SpawnPrepareSandboxCfg(&content, nullptr);
    EXPECT_EQ(ret, -1);
    AppSpawningCtx property = {};
    property.message = (AppSpawnMsgNode *)malloc(sizeof(AppSpawnMsgNode));
    ASSERT_EQ(property.message != nullptr, 1);
    ret = strcpy_s(property.message->msgHeader.processName, APP_LEN_PROC_NAME, "com.xxx.xxx.xxx");
    EXPECT_EQ(ret, 0);
    ret = SpawnPrepareSandboxCfg(&content, &property);
    EXPECT_EQ(ret, -1);
    free(property.message);

    PathMountNode mountNode = {};
    mountNode.checkErrorFlag = true;
    mountNode.createDemand = 0;
    DumpSandboxMountNode(nullptr, 0);
    mountNode.sandboxNode.type = SANDBOX_TAG_MOUNT_PATH;
    DumpSandboxMountNode(&mountNode.sandboxNode, 0);
    mountNode.sandboxNode.type = SANDBOX_TAG_SYMLINK;
    DumpSandboxMountNode(&mountNode.sandboxNode, 0);
    mountNode.source = (char*)malloc(sizeof(char) * TEST_STR_LEN);
    ASSERT_EQ(mountNode.source!= nullptr, 1);
    mountNode.target = (char*)malloc(sizeof(char) * TEST_STR_LEN);
    ASSERT_EQ(mountNode.target!= nullptr, 1);
    mountNode.appAplName = (char*)malloc(sizeof(char) * TEST_STR_LEN);
    ASSERT_EQ(mountNode.appAplName!= nullptr, 1);
    ret = strcpy_s(mountNode.source, TEST_STR_LEN, "/xxxxx/xxx");
    EXPECT_EQ(ret, 0);
    ret = strcpy_s(mountNode.target, TEST_STR_LEN, "/test/xxxx");
    EXPECT_EQ(ret, 0);
    ret = strcpy_s(mountNode.appAplName, TEST_STR_LEN, "apl");
    EXPECT_EQ(ret, 0);
    mountNode.sandboxNode.type = SANDBOX_TAG_MOUNT_FILE;
    DumpSandboxMountNode(&mountNode.sandboxNode, 0);
    mountNode.checkErrorFlag = false;
    mountNode.createDemand = 1;
    mountNode.sandboxNode.type = SANDBOX_TAG_SYMLINK;
    DumpSandboxMountNode(&mountNode.sandboxNode, 0);
    mountNode.sandboxNode.type = SANDBOX_TAG_REQUIRED;
    DumpSandboxMountNode(&mountNode.sandboxNode, 0);
    free(mountNode.source);
    free(mountNode.target);
    free(mountNode.appAplName);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_load_001, TestSize.Level0)
{
    unsigned long ret = GetMountModeFromConfig(nullptr, nullptr, 1);
    EXPECT_EQ(ret, 1);
    const char testStr1[] = "{ \
        \"test\":\"xxxxxxxxx\", \
    }";
    cJSON *config1 = cJSON_Parse(testStr1);
    ASSERT_EQ(config1, nullptr);
    ret = GetMountModeFromConfig(config1, "test", 1);
    EXPECT_EQ(ret, 1);

    ret = GetFlagIndexFromJson(config1);
    EXPECT_EQ(ret, 0);
    const char testStr2[] = "{ \
        \"name\":\"xxxxxxxxx\", \
    }";
    cJSON *config2 = cJSON_Parse(testStr2);
    ASSERT_EQ(config2, nullptr);
    ret = GetFlagIndexFromJson(config2);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(config1);
    cJSON_Delete(config2);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_load_002, TestSize.Level0)
{
    const char testStr1[] = "{ \
        \"mount-paths\":[\"/xxxx/xxx\", \"/xxx/xxx\"] \
    }";
    cJSON *config1 = cJSON_Parse(testStr1);
    ASSERT_NE(config1, nullptr);
    cJSON *mountPaths = cJSON_GetObjectItemCaseSensitive(config1, "mount-paths");
    int result = ParseMountPathsConfig(nullptr, mountPaths, nullptr, 1);
    EXPECT_EQ(result, 0);
    cJSON_Delete(config1);

    const char testStr2[] = "{ \
        \"mount-paths\":[{\
            \"src-path\": \"/config\", \
            \"test\": \"/test/xxx\" \
        }] \
    }";
    cJSON *config2 = cJSON_Parse(testStr2);
    ASSERT_NE(config2, nullptr);
    mountPaths = cJSON_GetObjectItemCaseSensitive(config2, "mount-paths");
    result = ParseMountPathsConfig(nullptr, mountPaths, nullptr, 1);
    EXPECT_EQ(result, 0);
    cJSON_Delete(config2);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_load_003, TestSize.Level0)
{
    const char testStr1[] = "{ \
        \"mount-paths\":[{ \
            \"sandbox-path\": \"/config\", \
            \"test\": \"/test/xxx\"}] \
    }";
    cJSON *config1 = cJSON_Parse(testStr1);
    ASSERT_NE(config1, nullptr);
    cJSON *mountPaths = cJSON_GetObjectItemCaseSensitive(config1, "mount-paths");
    int result = ParseMountPathsConfig(nullptr, mountPaths, nullptr, 1);
    EXPECT_EQ(result, 0);
    cJSON_Delete(config1);

    const char testStr2[] = "{ \
        \"mount-paths\":[{ \
            \"src-path\": \"/xxx/xxx\", \
            \"sandbox-path\": \"/test/xxx\"}] \
    }";
    cJSON *config2 = cJSON_Parse(testStr2);
    ASSERT_NE(config2, nullptr);
    mountPaths = cJSON_GetObjectItemCaseSensitive(config2, "mount-paths");
    result = ParseMountPathsConfig(nullptr, mountPaths, nullptr, 1);
    EXPECT_EQ(result, 0);
    cJSON_Delete(config2);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_load_004, TestSize.Level0)
{
    int ret = ParseSymbolLinksConfig(nullptr, nullptr, nullptr);
    EXPECT_EQ(ret, -1);

    const char testStr1[] = "{ \
        \"test\":\"xxxxxxxxx\" \
    }";
    cJSON *config1 = cJSON_Parse(testStr1);
    ASSERT_NE(config1, nullptr);
    ret = ParseSymbolLinksConfig(nullptr, config1, nullptr);
    EXPECT_EQ(ret, -1);
    cJSON_Delete(config1);

    const char testStr2[] = "{ \
        \"symbol-links\":[\"/xxxx/xxx\", \"/xxx/xxx\"] \
    }";
    cJSON *config2 = cJSON_Parse(testStr2);
    ASSERT_NE(config2, nullptr);
    cJSON *symbolLinks = cJSON_GetObjectItemCaseSensitive(config2, "symbol-links");
    ret = ParseSymbolLinksConfig(nullptr, symbolLinks, nullptr);
    EXPECT_EQ(ret, -1);
    cJSON_Delete(config2);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_load_005, TestSize.Level0)
{
    const char testStr1[] = "{ \
        \"symbol-links\":[{\
            \"test\":\"/xxxx/xxx\", \
            \"target-name\":\"/xxx/xxx\"}] \
    }";
    cJSON *config1 = cJSON_Parse(testStr1);
    ASSERT_NE(config1, nullptr);
    cJSON *symbolLinks = cJSON_GetObjectItemCaseSensitive(config1, "symbol-links");
    int ret = ParseSymbolLinksConfig(nullptr, symbolLinks, nullptr);
    EXPECT_EQ(ret, -1);
    cJSON_Delete(config1);

    const char testStr2[] = "{ \
        \"symbol-links\":[{\
            \"test\":\"/xxxx/xxx\", \
            \"link-name\":\"/xxx/xxx\"}] \
    }";
    cJSON *config2 = cJSON_Parse(testStr2);
    ASSERT_NE(config2, nullptr);
    symbolLinks = cJSON_GetObjectItemCaseSensitive(config2, "symbol-links");
    ret = ParseSymbolLinksConfig(nullptr, symbolLinks, nullptr);
    EXPECT_EQ(ret, -1);
    cJSON_Delete(config2);

    const char testStr3[] = "{ \
        \"symbol-links\":[{\
            \"target-name\":\"/xxxx/xxx\", \
            \"link-name\":\"/xxx/xxx\"}] \
    }";
    cJSON *config3 = cJSON_Parse(testStr3);
    ASSERT_NE(config3, nullptr);
    symbolLinks = cJSON_GetObjectItemCaseSensitive(config3, "symbol-links");
    ret = ParseSymbolLinksConfig(nullptr, symbolLinks, nullptr);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(config3);
}

HWTEST_F(AppSpawnSandboxCoverageTest, App_Spawn_Sandbox_load_006, TestSize.Level0)
{
    const char testStr1[] = "{ \
        \"test\":\"xxxxxxxxx\" \
    }";
    cJSON *config1 = cJSON_Parse(testStr1);
    ASSERT_NE(config1, nullptr);
    int ret = ParseGidTableConfig(nullptr, config1, nullptr);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(config1);

    const char testStr2[] = "{ \
        \"test\":[] \
    }";
    cJSON *config2 = cJSON_Parse(testStr2);
    ASSERT_NE(config2, nullptr);
    cJSON *testCfg = cJSON_GetObjectItemCaseSensitive(config2, "test");
    ret = ParseGidTableConfig(nullptr, testCfg, nullptr);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(config2);

    SandboxSection section = {};
    const char testStr3[] = "{ \
        \"gids\":[\"202400\", \"202500\", \"202600\"] \
    }";
    cJSON *config3 = cJSON_Parse(testStr3);
    ASSERT_NE(config3, nullptr);
    testCfg = cJSON_GetObjectItemCaseSensitive(config3, "gids");
    ret = ParseGidTableConfig(nullptr, testCfg, &section);
    EXPECT_EQ(ret, 0);
    cJSON_Delete(config3);

    section.gidTable = (gid_t *)malloc(sizeof(gid_t) * 10);
    ASSERT_EQ(section.gidTable != nullptr, 1);
    ret = ParseGidTableConfig(nullptr, testCfg, &section);
    EXPECT_EQ(ret, 0);
    if (section.gidTable != nullptr) {
        free(section.gidTable);
    }
}
}