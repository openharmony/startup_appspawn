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
static const std::string g_commonConfig = "{ \
    \"global\": { \
        \"sandbox-root\": \"/mnt/sandbox/<currentUserId>/app-root\", \
        \"sandbox-ns-flags\": [ \"pid\", \"net\" ], \
        \"top-sandbox-switch\": \"ON\" \
    }, \
    \"required\":{ \
        \"system-const\":{ \
            \"mount-paths\" : [{ \
                \"src-path\" : \"/lib\", \
                \"sandbox-path\" : \"/lib\", \
                \"check-action-status\": \"false\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                \"category\": \"shared\", \
                \"app-apl-name\" : \"system\" \
            }, { \
                \"src-path\" : \"/lib1\", \
                \"sandbox-path\" : \"/lib1\", \
                \"check-action-status\": \"false\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                \"category\": \"rdonly\", \
                \"app-apl-name\" : \"system\" \
            }, { \
                \"src-path\" : \"/none\", \
                \"sandbox-path\" : \"/storage/cloud/epfs\", \
                \"check-action-status\": \"false\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                \"category\": \"epfs\", \
                \"app-apl-name\" : \"system\" \
            }, { \
                \"src-path\" : \"/storage/Users/<currentUserId>/appdata/el1\", \
                \"sandbox-path\" : \"/storage/Users/<currentUserId>/appdata/el1\", \
                \"check-action-status\": \"false\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                \"category\": \"dac_override\", \
                \"app-apl-name\" : \"system\" \
            }, { \
                \"src-path\" : \"/dev/fuse\", \
                \"sandbox-path\" : \"/mnt/data/fuse\", \
                \"category\": \"fuse\", \
                \"check-action-status\": \"true\" \
            }], \
            \"symbol-links\" : [], \
            \"mount-groups\": [\"test-always\"] \
        }, \
        \"app-variable\":{ \
            \"mount-paths\" : [{ \
                \"src-path\" : \"/config\", \
                \"sandbox-path\" : \"/config\", \
                \"check-action-status\": \"false\", \
                \"app-apl-name\" : \"system\", \
                \"category\": \"shared\" \
            }], \
            \"symbol-links\" : [], \
            \"mount-groups\": [\"el5\"] \
        } \
    }, \
    \"name-groups\": [ \
        { \
            \"name\": \"user-public\", \
            \"type\": \"system-const\", \
            \"mount-paths\" : [{ \
                \"src-path\" : \"/data/app/el2/<currentUserId>/base/<PackageName>\", \
                \"sandbox-path\" : \"/data/storage/el2/base\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                \"category\": \"shared\" \
            }] \
        }, { \
            \"name\": \"el5\", \
            \"type\": \"app-variable\", \
            \"deps-mode\": \"not-exists\", \
            \"mount-paths-deps\": { \
                \"sandbox-path\": \"/data/storage/el5\", \
                \"src-path\": \"/data/app/el5/<currentUserId>/base\", \
                \"category\": \"shared\" \
            }, \
            \"mount-paths\" : [{ \
                \"src-path\" : \"/data/app/el5/<currentUserId>/base/<PackageName>\", \
                \"sandbox-path\" : \"<deps-path>/base\" \
            }] \
        }, { \
            \"name\": \"el6\", \
            \"type\": \"app-variable\", \
            \"deps-mode\": \"not-exists\", \
            \"mount-paths-deps\": { \
                \"sandbox-path\": \"/data/storage/el6\", \
                \"src-path\": \"/data/app/el6/<currentUserId>/base\", \
                \"category\": \"shared\" \
            }, \
            \"mount-paths\" : [{ \
                \"src-path\" : \"/data/app/el6/<currentUserId>/base/<PackageName>\", \
                \"sandbox-path\" : \"<deps-path>/base\" \
            }] \
        },{ \
            \"name\": \"test-always\", \
            \"type\": \"system-const\", \
            \"deps-mode\": \"always\", \
            \"mount-paths-deps\": { \
                \"sandbox-path\": \"/data/storage/e20\", \
                \"src-path\": \"/data/app/e20/<currentUserId>/base\", \
                \"category\": \"shared\" \
            }, \
            \"mount-paths\" : [{ \
                \"src-path\" : \"/data/app/e20/<currentUserId>/base/<PackageName>\", \
                \"sandbox-path\" : \"<deps-path>/base\" \
            }] \
        } \
    ] \
}";

static const std::string g_packageNameConfig = "{ \
    \"global\": { \
        \"sandbox-root\": \"/mnt/sandbox/<currentUserId>/app-root\", \
        \"sandbox-ns-flags\": [ \"pid\", \"net\" ], \
        \"top-sandbox-switch\": \"OFF\" \
    }, \
    \"conditional\":{ \
        \"package-name\": [{ \
            \"name\": \"test.example.ohos.com\", \
            \"sandbox-switch\": \"ON\", \
            \"sandbox-shared\" : \"true\", \
            \"sandbox-ns-flags\" : [ \"pid\", \"net\" ], \
            \"mount-paths\" : [{ \
                \"src-path\" : \"/config\", \
                \"sandbox-path\" : \"/config\", \
                \"check-action-status\": \"false\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                \"category\": \"shared\", \
                \"app-apl-name\" : \"system\" \
            }], \
            \"symbol-links\" : [{ \
                \"target-name\" : \"/system/etc\", \
                \"link-name\" : \"/etc\", \
                \"check-action-status\": \"false\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \" \
            }] \
        }, \
        { \
            \"name\": \"com.example.myapplication\", \
            \"sandbox-switch\": \"ON\", \
            \"sandbox-shared\" : \"true\", \
            \"mount-paths\" : [{ \
                    \"src-path\" : \"/mnt/data/<currentUserId>\", \
                    \"sandbox-path\" : \"/mnt/data\", \
                    \"category\": \"shared\", \
                    \"check-action-status\": \"true\" \
                }, { \
                    \"src-path\" : \"/dev/fuse\", \
                    \"sandbox-path\" : \"/mnt/data/fuse\", \
                    \"category\": \"fuse\", \
                    \"check-action-status\": \"true\" \
                }],\
            \"symbol-links\" : [] \
        }]\
    } \
}";

static const std::string g_permissionConfig = "{ \
    \"global\": { \
        \"sandbox-root\": \"/mnt/sandbox/<currentUserId>/app-root\", \
        \"sandbox-ns-flags\": [ \"pid\", \"net\" ] \
    }, \
    \"conditional\":{ \
        \"permission\": [{ \
                \"name\": \"ohos.permission.FILE_ACCESS_MANAGER\", \
                \"sandbox-switch\": \"ON\", \
                \"gids\": [\"file_manager\", \"user_data_rw\"], \
                \"sandbox-ns-flags\" : [ \"pid\", \"net\" ], \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/config--1\", \
                    \"sandbox-path\" : \"/data/app/el1/<currentUserId>/database/<PackageName_index>\", \
                    \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                    \"category\": \"shared\", \
                    \"app-apl-name\" : \"system\", \
                    \"check-action-status\": \"true\" \
                }], \
                \"symbol-links\" : [{ \
                        \"target-name\" : \"/system/etc\", \
                        \"link-name\" : \"/etc\", \
                        \"check-action-status\": \"false\", \
                        \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \" \
                    } \
                ] \
            }, \
            { \
                \"name\": \"ohos.permission.ACTIVATE_THEME_PACKAGE\", \
                \"sandbox-switch\": \"ON\", \
                \"gids\": [1006, 1008], \
                \"sandbox-ns-flags\" : [ \"pid\", \"net\" ], \
                \"mount-paths\" : [{ \
                    \"src-path\" : \"/config--2\", \
                    \"sandbox-path\" : \"/data/app/el1/<currentUserId>/database/<PackageName_index>\", \
                    \"check-action-status\": \"false\" \
                }], \
                \"symbol-links\" : [] \
            }] \
        } \
    }";

static const std::string g_spawnFlagsConfig = "{ \
    \"global\": { \
        \"sandbox-root\": \"/mnt/sandbox/<currentUserId>/app-root\", \
        \"sandbox-ns-flags\": [ \"pid\", \"net\" ], \
        \"top-sandbox-switch\": \"OFF\" \
    }, \
    \"conditional\":{ \
        \"spawn-flag\": [{ \
            \"name\": \"START_FLAGS_BACKUP\", \
            \"mount-paths\": [{ \
                \"src-path\" : \"/data/app/el1/bundle/public/\", \
                \"sandbox-path\" : \"/data/bundles/\", \
                \"check-action-status\": \"true\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                \"category\": \"shared\", \
                \"app-apl-name\" : \"system\" \
            }, { \
                \"sandbox-path\": \"/data/storage/el1/backup\", \
                \"src-path\": \"/data/service/el1/<currentUserId>/backup/bundles/<PackageName>\" \
            }], \
            \"mount-groups\": [] \
        }, { \
            \"name\": \"DLP_MANAGER\", \
            \"mount-paths\": [{ \
                \"src-path\" : \"/data/app/el1/bundle/public/\", \
                \"sandbox-path\" : \"/data/bundles/\", \
                \"check-action-status\": \"true\" \
            }], \
            \"mount-groups\": [] \
        }] \
    }\
}";

static inline SandboxMountNode *GetFirstSandboxMountPathNode(const SandboxSection *section)
{
    APPSPAWN_CHECK_ONLY_EXPER(section != nullptr, return nullptr);
    ListNode *node = section->front.next;
    if (node == &section->front) {
        return NULL;
    }
    return reinterpret_cast<SandboxMountNode *>(ListEntry(node, SandboxMountNode, node));
}

static inline SandboxMountNode *GetNextSandboxMountPathNode(const SandboxSection *section, SandboxMountNode *pathNode)
{
    APPSPAWN_CHECK_ONLY_EXPER(section != nullptr && pathNode != nullptr, return nullptr);
    if (pathNode->node.next == &section->front) {
        return NULL;
    }
    return reinterpret_cast<SandboxMountNode *>(ListEntry(pathNode->node.next, SandboxMountNode, node));
}

AppSpawnTestHelper g_testHelper;
class AppSpawnSandboxTest : public testing::Test {
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
    APPSPAWN_CHECK(msgFlags != NULL, return nullptr, "No msg flags in msg %{public}s", GetProcessName(property));

    SandboxContext *context = GetSandboxContext();
    APPSPAWN_CHECK(context != NULL, return nullptr, "Failed to get context");

    context->nwebspawn = nwebspawn;
    context->bundleName = GetBundleName(property);
    context->bundleHasWps = strstr(context->bundleName, "wps") != NULL;
    context->dlpBundle = strstr(context->bundleName, "com.ohos.dlpmanager") != NULL;
    context->appFullMountEnable = 0;
    context->dlpUiExtType = strstr(GetProcessName(property), "sys/commonUI") != NULL;

    context->sandboxSwitch = 1;
    context->sandboxShared = false;
    context->message = property->message;
    context->rootPath = NULL;
    context->rootPath = NULL;
    return context;
}

static int TestParseAppSandboxConfig(AppSpawnSandboxCfg *sandbox, const char *buffer)
{
    cJSON *config = cJSON_Parse(buffer);
    if (config == nullptr) {
        APPSPAWN_LOGE("TestParseAppSandboxConfig config %s", buffer);
        return -1;
    }
    int ret = 0;
    do {
        ret = ParseAppSandboxConfig(config, sandbox);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        uint32_t depNodeCount = sandbox->depNodeCount;
        APPSPAWN_CHECK_ONLY_EXPER(depNodeCount > 0, break);

        sandbox->depGroupNodes = (SandboxNameGroupNode **)calloc(1, sizeof(SandboxNameGroupNode *) * depNodeCount);
        APPSPAWN_CHECK(sandbox->depGroupNodes != NULL, break, "Failed alloc memory ");
        sandbox->depNodeCount = 0;
        ListNode *node = sandbox->nameGroupsQueue.front.next;
        while (node != &sandbox->nameGroupsQueue.front) {
            SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)ListEntry(node, SandboxMountNode, node);
            if (groupNode->depNode) {
                sandbox->depGroupNodes[sandbox->depNodeCount++] = groupNode;
            }
            node = node->next;
        }
        APPSPAWN_LOGI("LoadAppSandboxConfig depNodeCount %{public}d", sandbox->depNodeCount);
    } while (0);
    cJSON_Delete(config);
    return ret;
}

static int HandleSplitString(const char *str, void *context)
{
    std::vector<std::string> *results = reinterpret_cast<std::vector<std::string> *>(context);
    results->push_back(std::string(str));
    return 0;
}

static int TestJsonUtilSplit(const char *args[], uint32_t argc, const std::string &input, const std::string &pattern)
{
    std::vector<std::string> results;
    StringSplit(input.c_str(), pattern.c_str(), reinterpret_cast<void *>(&results), HandleSplitString);
    if (argc != results.size()) {
        return -1;
    }
    for (size_t i = 0; i < argc; i++) {
        if (strcmp(args[i], results[i].c_str()) != 0) {
            return -1;
        }
    }
    return 0;
}

HWTEST(AppSpawnSandboxTest, App_Spawn_JsonUtil_001, TestSize.Level0)
{
    const char *args[] = {"S_IRUSR", "S_IWOTH", "S_IRWXU"};
    std::string cmd = "   S_IRUSR   S_IWOTH      S_IRWXU   ";
    size_t size = sizeof(args) / sizeof(args[0]);
    ASSERT_EQ(TestJsonUtilSplit(args, size, cmd, " "), 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_JsonUtil_002, TestSize.Level0)
{
    const char *args[] = {"S_IRUSR", "S_IWOTH", "S_IRWXU"};
    std::string cmd = "S_IRUSR   S_IWOTH      S_IRWXU";
    size_t size = sizeof(args) / sizeof(args[0]);
    ASSERT_EQ(TestJsonUtilSplit(args, size, cmd, " "), 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_JsonUtil_003, TestSize.Level0)
{
    const char *args[] = {"S_IRUSR", "S_IWOTH", "S_IRWXU"};
    std::string cmd = "  S_IRUSR   S_IWOTH      S_IRWXU";
    size_t size = sizeof(args) / sizeof(args[0]);
    ASSERT_EQ(TestJsonUtilSplit(args, size, cmd, " "), 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_JsonUtil_004, TestSize.Level0)
{
    const char *args[] = {"S_IRUSR", "S_IWOTH", "S_IRWXU"};
    std::string cmd = "S_IRUSR   S_IWOTH      S_IRWXU  ";
    size_t size = sizeof(args) / sizeof(args[0]);
    ASSERT_EQ(TestJsonUtilSplit(args, size, cmd, " "), 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_JsonUtil_005, TestSize.Level0)
{
    const char *args[] = {"S_IRUSR"};
    std::string cmd = "  S_IRUSR    ";
    size_t size = sizeof(args) / sizeof(args[0]);
    ASSERT_EQ(TestJsonUtilSplit(args, size, cmd, " "), 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_JsonUtil_006, TestSize.Level0)
{
    const char *args[] = {"S_IRUSR", "S_IWOTH", "S_IRWXU"};
    std::string cmd = "  S_IRUSR |  S_IWOTH    |  S_IRWXU  ";
    size_t size = sizeof(args) / sizeof(args[0]);
    ASSERT_EQ(TestJsonUtilSplit(args, size, cmd, "|"), 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_JsonUtil_007, TestSize.Level0)
{
    const char *args[] = {"send", "--type", "2"};
    std::string cmd = "send --type 2 ";
    size_t size = sizeof(args) / sizeof(args[0]);
    ASSERT_EQ(TestJsonUtilSplit(args, size, cmd, " "), 0);
}

/**
 * @brief 测试Variable 变量替换 <currentUserId> <PackageName_index>
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_001, TestSize.Level0)
{
    AddDefaultVariable();
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char *real = "/data/app/el2/100/log/com.example.myapplication_100";
    const char *value = GetSandboxRealVar(context, 0,
        "/data/app/el2/<currentUserId>/log/<PackageName_index>", nullptr, nullptr);
    APPSPAWN_LOGV("value %{public}s", value);
    APPSPAWN_LOGV("real %{public}s", real);
    ASSERT_EQ(value != nullptr, 1);
    ASSERT_EQ(strcmp(value, real) == 0, 1);
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @brief 测试变量<lib>
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_002, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char *value = GetSandboxRealVar(context, 0, "/system/<lib>/module", nullptr, nullptr);
    APPSPAWN_LOGV("value %{public}s", value);
    ASSERT_EQ(value != nullptr, 1);
#ifdef APPSPAWN_64
    ASSERT_EQ(strcmp(value, "/system/lib64/module") == 0, 1);
#else
    ASSERT_EQ(strcmp(value, "/system/lib/module") == 0, 1);
#endif
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @brief 测试系统参数变量<param:test.variable.001>
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_003, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char *real = "/system/test.variable.001/test001";
    const char *value = GetSandboxRealVar(context, 0, "/system/<param:test.variable.001>/test001", nullptr, nullptr);
    APPSPAWN_LOGV("value %{public}s", value);
    ASSERT_EQ(value != nullptr, 1);
    ASSERT_EQ(strcmp(value, real) == 0, 1);
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

static int TestVariableReplace(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    int len = sprintf_s((char *)buffer, bufferLen, "%s", "Test-value-001.Test-value-002.Test-value-003.Test-value-004");
    APPSPAWN_CHECK(len > 0 && ((uint32_t)len < bufferLen),
        return -1, "Failed to format path app: %{public}s", context->bundleName);
    *realLen = (uint32_t)len;
    return 0;
}

/**
 * @brief 测试注册变量，和替换
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_004, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    AddVariableReplaceHandler("<Test-Var-001>", TestVariableReplace);

    const char *real = "/system/Test-value-001.Test-value-002.Test-value-003.Test-value-004/test001";
    const char *value = GetSandboxRealVar(context, 0, "/system/<Test-Var-001>/test001", nullptr, nullptr);
    APPSPAWN_LOGV("value %{public}s", value);
    ASSERT_EQ(value != nullptr, 1);
    ASSERT_EQ(strcmp(value, real) == 0, 1);
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @brief 测试dep-path 变量替换
 *
 */
static VarExtraData *TestGetVarExtraData(const SandboxContext *context, uint32_t sandboxTag,
    const PathMountNode *depNode)
{
    static VarExtraData extraData;
    (void)memset_s(&extraData, sizeof(extraData), 0, sizeof(extraData));
    extraData.sandboxTag = sandboxTag;
    if (sandboxTag == SANDBOX_TAG_NAME_GROUP) {
        extraData.data.depNode = (PathMountNode *)depNode;
    }
    return &extraData;
}

static inline void TestSetMountPathOperation(uint32_t *operation, uint32_t index)
{
    *operation |= (1 << index);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_005, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    PathMountNode pathNode;
    pathNode.source = const_cast<char *>("/data/app/el2/<currentUserId>/base");
    pathNode.target = const_cast<char *>("/data/storage/el2");
    pathNode.category = MOUNT_TMP_SHRED;
    VarExtraData *extraData = TestGetVarExtraData(context, SANDBOX_TAG_NAME_GROUP, &pathNode);
    const char *real = "/data/storage/el2/base";
    TestSetMountPathOperation(&extraData->operation, MOUNT_PATH_OP_REPLACE_BY_SANDBOX);
    const char *value = GetSandboxRealVar(context, 0, "<deps-path>/base", nullptr, extraData);
    APPSPAWN_LOGV("value %{public}s", value);
    ASSERT_EQ(value != nullptr, 1);
    ASSERT_EQ(strcmp(value, real) == 0, 1);
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @brief 测试dep-path 变量替换
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_006, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    PathMountNode pathNode;
    pathNode.source = const_cast<char *>("/data/app/el2/<currentUserId>/base");
    pathNode.target = const_cast<char *>("/data/storage/el2");
    pathNode.category = MOUNT_TMP_SHRED;
    VarExtraData *extraData = TestGetVarExtraData(context, SANDBOX_TAG_NAME_GROUP, &pathNode);
    const char *real = "/data/app/el2/100/base/base";
    const char *value = GetSandboxRealVar(context, 0, "<deps-path>/base", nullptr, extraData);
    APPSPAWN_LOGV("value %{public}s", value);
    ASSERT_EQ(value != nullptr, 1);
    ASSERT_EQ(strcmp(value, real) == 0, 1);
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @brief 测试dep-src-path 变量替换
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_007, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    PathMountNode pathNode;
    pathNode.source = const_cast<char *>("/data/app/el2/<currentUserId>/base");
    pathNode.target = const_cast<char *>("/data/storage/el2");
    pathNode.category = MOUNT_TMP_SHRED;
    VarExtraData *extraData = TestGetVarExtraData(context, SANDBOX_TAG_NAME_GROUP, &pathNode);
    const char *real = "/data/app/el2/100/base/base";
    const char *value = GetSandboxRealVar(context, 0, "<deps-src-path>/base", nullptr, extraData);
    APPSPAWN_LOGV("value %{public}s", value);
    ASSERT_EQ(value != nullptr, 1);
    ASSERT_EQ(strcmp(value, real) == 0, 1);
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @brief 测试dep-sandbox-path 变量替换
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_008, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    PathMountNode pathNode;
    pathNode.source = const_cast<char *>("/data/app/el2/<currentUserId>/base");
    pathNode.target = const_cast<char *>("/data/storage/el2");
    pathNode.category = MOUNT_TMP_SHRED;
    VarExtraData *extraData = TestGetVarExtraData(context, SANDBOX_TAG_NAME_GROUP, &pathNode);
    const char *real = "/data/storage/el2/base";
    const char *value = GetSandboxRealVar(context, 0, "<deps-sandbox-path>/base", nullptr, extraData);
    APPSPAWN_LOGV("value %{public}s", value);
    ASSERT_EQ(value != nullptr, 1);
    ASSERT_EQ(strcmp(value, real) == 0, 1);
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

/**
 * @brief 测试不存在的变量替换
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Variable_009, TestSize.Level0)
{
    AppSpawningCtx *spawningCtx = TestCreateAppSpawningCtx();
    SandboxContext *context = TestGetSandboxContext(spawningCtx, 0);
    ASSERT_EQ(context != nullptr, 1);

    const char *real = "<deps-test-path>/base";
    const char *value = GetSandboxRealVar(context, 0, "<deps-test-path>/base", nullptr, nullptr);
    APPSPAWN_LOGV("value %{public}s", value);
    ASSERT_EQ(value != nullptr, 1);
    ASSERT_EQ(strcmp(value, real) == 0, 1);
    DeleteSandboxContext(context);
    DeleteAppSpawningCtx(spawningCtx);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Permission_01, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        LoadAppSandboxConfig(sandbox, 0);
        sandbox->extData.dumpNode(&sandbox->extData);

        AppSpawnTestHelper testHelper;
        const std::vector<const char *> &permissions = testHelper.GetPermissions();
        for (auto permission : permissions) {
            const SandboxPermissionNode *node = GetPermissionNodeInQueue(&sandbox->permissionQueue, permission);
            APPSPAWN_CHECK(node != nullptr && strcmp(node->section.name, permission) == 0,
                break, "Failed to permission %{public}s", permission);
            const SandboxPermissionNode *node2 =
                GetPermissionNodeInQueueByIndex(&sandbox->permissionQueue, node->permissionIndex);
            APPSPAWN_CHECK(node2 != nullptr && strcmp(node->section.name, node2->section.name) == 0,
                break, "Failed to permission %{public}s", permission);
        }
        const char *permission = "ohos.permission.XXXXX";
        const SandboxPermissionNode *node = GetPermissionNodeInQueue(&sandbox->permissionQueue, permission);
        APPSPAWN_CHECK_ONLY_EXPER(node == nullptr, break);
        node = GetPermissionNodeInQueue(nullptr, permission);
        APPSPAWN_CHECK_ONLY_EXPER(node == nullptr, break);
        ret = 0;
    } while (0);
    if (sandbox != nullptr) {
        sandbox->extData.freeNode(&sandbox->extData);
    }
    ASSERT_EQ(ret, 0);
}

static int ProcessTestExpandConfig(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandBox, const char *name)
{
    uint32_t size = 0;
    char *extInfo = (char *)GetAppSpawnMsgExtInfo(context->message, name, &size);
    if (size == 0 || extInfo == NULL) {
        return 0;
    }
    return 0;
}

HWTEST(AppSpawnSandboxTest, App_Spawn_ExpandCfg_01, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        LoadAppSandboxConfig(sandbox, 0);
        // add default
        AddDefaultExpandAppSandboxConfigHandle();
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        // add expand info to msg
        const char hspListStr[] = "{ \
            \"bundles\":[\"test.bundle1\", \"test.bundle2\"], \
            \"modules\":[\"module1\", \"module2\"], \
            \"versions\":[\"v10001\", \"v10002\"] \
        }";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, "HspList",
            reinterpret_cast<uint8_t *>(const_cast<char *>(hspListStr)), strlen(hspListStr) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to ext tlv %{public}s", hspListStr);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = MountSandboxConfigs(sandbox, property, 0);
    } while (0);
    if (sandbox != nullptr) {
        sandbox->extData.freeNode(&sandbox->extData);
    }
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_ExpandCfg_02, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        LoadAppSandboxConfig(sandbox, 0);

        // add default
        AddDefaultExpandAppSandboxConfigHandle();
        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        // add expand info to msg
        const char dataGroupInfoListStr[] = "{ \
            \"dataGroupId\":[\"1234abcd5678efgh\", \"abcduiop1234\"], \
            \"dir\":[\"/data/app/el2/100/group/091a68a9-2cc9-4279-8849-28631b598975\", \
                     \"/data/app/el2/100/group/ce876162-fe69-45d3-aa8e-411a047af564\"], \
            \"gid\":[\"20100001\", \"20100002\"] \
        }";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, "DataGroup",
            reinterpret_cast<uint8_t *>(const_cast<char *>(dataGroupInfoListStr)), strlen(dataGroupInfoListStr) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to ext tlv %{public}s", dataGroupInfoListStr);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = MountSandboxConfigs(sandbox, property, 0);
    } while (0);
    if (sandbox != nullptr) {
        sandbox->extData.freeNode(&sandbox->extData);
    }
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_ExpandCfg_03, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        LoadAppSandboxConfig(sandbox, 0);

        // add default
        AddDefaultVariable();
        AddDefaultExpandAppSandboxConfigHandle();

        // create msg
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_OVERLAY);
        // add expand info to msg
        const char *overlayInfo = "/data/app/el1/bundle/public/com.ohos.demo/feature.hsp| "
            "/data/app/el1/bundle/public/com.ohos.demo/feature.hsp";

        ret = AppSpawnReqMsgAddExtInfo(reqHandle, "Overlay",
            reinterpret_cast<uint8_t *>(const_cast<char *>(overlayInfo)), strlen(overlayInfo) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to ext tlv %{public}s", overlayInfo);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = MountSandboxConfigs(sandbox, property, 0);
    } while (0);
    if (sandbox != nullptr) {
        sandbox->extData.freeNode(&sandbox->extData);
    }
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

HWTEST(AppSpawnSandboxTest, App_Spawn_ExpandCfg_04, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        LoadAppSandboxConfig(sandbox, 0);

        // add test
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        ret = RegisterExpandSandboxCfgHandler("test-cfg", EXPAND_CFG_HANDLER_PRIO_START, ProcessTestExpandConfig);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ret = RegisterExpandSandboxCfgHandler("test-cfg", EXPAND_CFG_HANDLER_PRIO_START, ProcessTestExpandConfig);
        APPSPAWN_CHECK_ONLY_EXPER(ret == APPSPAWN_NODE_EXIST, break);

        // create msg
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_SPAWN_NATIVE_PROCESS, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        // add expand info to msg
        const char *testInfo = "\"app-base\":[{ \
            \"sandbox-root\" : \"/mnt/sandbox/<currentUserId>/<PackageName>\", \
            \"mount-paths\" : [{ \
                \"src-path\" : \"/config\", \
                \"sandbox-path\" : \"/config\", \
                \"sandbox-flags\" : [ \"bind\", \"rec\" ], \
                \"check-action-status\": \"false\", \
                \"dest-mode\": \"S_IRUSR | S_IWOTH | S_IRWXU \", \
                \"sandbox-flags-customized\": [ \"MS_NODEV\", \"MS_RDONLY\" ], \
                \"dac-override-sensitive\": \"true\", \
                \"mount-shared-flag\" : \"true\", \
                \"app-apl-name\" : \"system\", \
                \"fs-type\": \"sharefs\", \
                \"options\": \"support_overwrite=1\" \
            }], \
            \"symbol-links\" : [] \
        }]";

        ret = AppSpawnReqMsgAddExtInfo(reqHandle, "test-cfg",
            reinterpret_cast<uint8_t *>(const_cast<char *>(testInfo)), strlen(testInfo) + 1);
        APPSPAWN_CHECK(ret == 0, break, "Failed to ext tlv %{public}s", testInfo);

        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = MountSandboxConfigs(sandbox, property, 0);
    } while (0);
    if (sandbox != nullptr) {
        sandbox->extData.freeNode(&sandbox->extData);
    }
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 加载系统的sandbox文件
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_cfg_001, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        LoadAppSandboxConfig(sandbox, 0);
        sandbox->extData.dumpNode(&sandbox->extData);
        ret = 0;
    } while (0);
    if (sandbox != nullptr) {
        sandbox->extData.freeNode(&sandbox->extData);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 加载基础的sandbox配置，并检查结果
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_cfg_002, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_LOGV("sandbox->rootPath: %{public}s", sandbox->rootPath);

        ASSERT_EQ(sandbox->topSandboxSwitch == 1, 1);
        ASSERT_EQ((sandbox->sandboxNsFlags & (CLONE_NEWPID | CLONE_NEWNET)) == (CLONE_NEWPID | CLONE_NEWNET), 1);
        ASSERT_EQ(strcmp(sandbox->rootPath, "/mnt/sandbox/<currentUserId>/app-root"), 0);

        SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "system-const");
        ASSERT_EQ(section != nullptr, 1);
        // check mount path
        PathMountNode *pathNode = reinterpret_cast<PathMountNode *>(GetFirstSandboxMountPathNode(section));
        ASSERT_EQ(pathNode != NULL, 1);
        ASSERT_EQ(pathNode->checkErrorFlag == 0, 1);
        ASSERT_EQ((pathNode->destMode & (S_IRUSR | S_IWOTH | S_IRWXU)) == (S_IRUSR | S_IWOTH | S_IRWXU), 1);
        ASSERT_EQ(pathNode->category, MOUNT_TMP_SHRED);
        pathNode = reinterpret_cast<PathMountNode *>(GetNextSandboxMountPathNode(section, &pathNode->sandboxNode));
        ASSERT_EQ(pathNode != NULL, 1);
        ASSERT_EQ(pathNode->category, MOUNT_TMP_RDONLY);
        pathNode = reinterpret_cast<PathMountNode *>(GetNextSandboxMountPathNode(section, &pathNode->sandboxNode));
        ASSERT_EQ(pathNode != NULL, 1);
        ASSERT_EQ(pathNode->category, MOUNT_TMP_EPFS);
        pathNode = reinterpret_cast<PathMountNode *>(GetNextSandboxMountPathNode(section, &pathNode->sandboxNode));
        ASSERT_EQ(pathNode != NULL, 1);
        ASSERT_EQ(pathNode->category, MOUNT_TMP_DAC_OVERRIDE);
        pathNode = reinterpret_cast<PathMountNode *>(GetNextSandboxMountPathNode(section, &pathNode->sandboxNode));
        ASSERT_EQ(pathNode != NULL, 1);
        ASSERT_EQ(pathNode->category, MOUNT_TMP_FUSE);
        ret = 0;
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 加载包名sandbox配置，并检查结果
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_cfg_003, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);

        TestParseAppSandboxConfig(sandbox, g_packageNameConfig.c_str());
        APPSPAWN_LOGV("sandbox->rootPath: %{public}s", sandbox->rootPath);

        // top check
        ASSERT_EQ(sandbox->topSandboxSwitch == 0, 1);  // not set
        ASSERT_EQ((sandbox->sandboxNsFlags & (CLONE_NEWPID | CLONE_NEWNET)) == (CLONE_NEWPID | CLONE_NEWNET), 1);

        // check private section
        SandboxPackageNameNode *sandboxNode = reinterpret_cast<SandboxPackageNameNode *>(
            GetSandboxSection(&sandbox->packageNameQueue, "test.example.ohos.com"));
        ASSERT_EQ(sandboxNode != NULL, 1);
        ASSERT_EQ(strcmp(sandboxNode->section.name, "test.example.ohos.com"), 0);
        ASSERT_EQ((sandboxNode->section.sandboxShared == 1) && (sandboxNode->section.sandboxSwitch == 1), 1);

        // check path node
        PathMountNode *pathNode = reinterpret_cast<PathMountNode *>(
            GetFirstSandboxMountPathNode(&sandboxNode->section));
        ASSERT_EQ(pathNode != NULL, 1);
        ASSERT_EQ(pathNode->checkErrorFlag == 0, 1);
        ASSERT_EQ((pathNode->destMode & (S_IRUSR | S_IWOTH | S_IRWXU)) == (S_IRUSR | S_IWOTH | S_IRWXU), 1);

        ASSERT_EQ((pathNode->appAplName != nullptr) && (strcmp(pathNode->appAplName, "system") == 0), 1);
        // check symlink
        SymbolLinkNode *linkNode = reinterpret_cast<SymbolLinkNode *>(
            GetNextSandboxMountPathNode(&sandboxNode->section, &pathNode->sandboxNode));
        ASSERT_EQ(linkNode != NULL, 1);
        ASSERT_EQ(linkNode->checkErrorFlag == 0, 1);
        ASSERT_EQ((linkNode->destMode & (S_IRUSR | S_IWOTH | S_IRWXU)) == (S_IRUSR | S_IWOTH | S_IRWXU), 1);
        ret = 0;
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 加载一个permission sandbox 配置。并检查配置解析是否正确
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_cfg_004, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);

        TestParseAppSandboxConfig(sandbox, g_permissionConfig.c_str());
        APPSPAWN_LOGV("sandbox->rootPath: %{public}s", sandbox->rootPath);
        // top check
        ASSERT_EQ(sandbox->topSandboxSwitch == 1, 1);  // not set, default value 1

        // check permission section
        SandboxPermissionNode *permissionNode = reinterpret_cast<SandboxPermissionNode *>(
            GetSandboxSection(&sandbox->permissionQueue, "ohos.permission.FILE_ACCESS_MANAGER"));
        ASSERT_EQ(permissionNode != nullptr, 1);
        ASSERT_EQ(permissionNode->section.gidTable != nullptr, 1);
        ASSERT_EQ(permissionNode->section.gidCount, 2);
        ASSERT_EQ(permissionNode->section.gidTable[0], 1006);
        ASSERT_EQ(permissionNode->section.gidTable[1], 1008);
        ASSERT_EQ(strcmp(permissionNode->section.name, "ohos.permission.FILE_ACCESS_MANAGER"), 0);

        // check path node
        PathMountNode *pathNode = reinterpret_cast<PathMountNode *>(
            GetFirstSandboxMountPathNode(&permissionNode->section));
        ASSERT_EQ(pathNode != NULL, 1);
        ASSERT_EQ(pathNode->checkErrorFlag == 1, 1);
        ASSERT_EQ((pathNode->destMode & (S_IRUSR | S_IWOTH | S_IRWXU)) == (S_IRUSR | S_IWOTH | S_IRWXU), 1);
        ASSERT_EQ((pathNode->appAplName != nullptr) && (strcmp(pathNode->appAplName, "system") == 0), 1);

        // check symlink
        SymbolLinkNode *linkNode = reinterpret_cast<SymbolLinkNode *>(
            GetNextSandboxMountPathNode(&permissionNode->section, &pathNode->sandboxNode));
        ASSERT_EQ(linkNode != NULL, 1);
        ASSERT_EQ(linkNode->checkErrorFlag == 0, 1);
        ASSERT_EQ((linkNode->destMode & (S_IRUSR | S_IWOTH | S_IRWXU)) == (S_IRUSR | S_IWOTH | S_IRWXU), 1);
        ret = 0;
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 加载一个spawn-flags sandbox 配置。并检查配置解析是否正确
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_cfg_005, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        TestParseAppSandboxConfig(sandbox, g_spawnFlagsConfig.c_str());

        // top check
        ASSERT_EQ(sandbox->topSandboxSwitch == 0, 1);  // not set
        ASSERT_EQ(sandbox->sandboxNsFlags == (CLONE_NEWPID | CLONE_NEWNET), 1);

        // check private section
        SandboxFlagsNode *sandboxNode = reinterpret_cast<SandboxFlagsNode *>(
            GetSandboxSection(&sandbox->spawnFlagsQueue, "START_FLAGS_BACKUP"));
        ASSERT_EQ(sandboxNode != nullptr, 1);
        ASSERT_EQ(strcmp(sandboxNode->section.name, "START_FLAGS_BACKUP"), 0);
        // no set, check default
        ASSERT_EQ((sandboxNode->section.sandboxShared == 0) && (sandboxNode->section.sandboxSwitch == 1), 1);
        ASSERT_EQ(sandboxNode->flagIndex == APP_FLAGS_BACKUP_EXTENSION, 1);

        // check path node
        PathMountNode *pathNode = reinterpret_cast<PathMountNode *>(
            GetFirstSandboxMountPathNode(&sandboxNode->section));
        ASSERT_EQ(pathNode != nullptr, 1);
        ASSERT_EQ(pathNode->checkErrorFlag == 1, 1);  // set
        ASSERT_EQ((pathNode->destMode & (S_IRUSR | S_IWOTH | S_IRWXU)) == (S_IRUSR | S_IWOTH | S_IRWXU), 1);
        ASSERT_EQ((pathNode->appAplName != nullptr) && (strcmp(pathNode->appAplName, "system") == 0), 1);
        ret = 0;
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 加载一个name-group sandbox 配置。并检查配置解析是否正确
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_cfg_006, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    int ret = -1;
    do {
        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());

        // check private section
        SandboxNameGroupNode *sandboxNode = reinterpret_cast<SandboxNameGroupNode *>(
            GetSandboxSection(&sandbox->nameGroupsQueue, "el5"));
        ASSERT_EQ(sandboxNode != nullptr, 1);
        ASSERT_EQ(strcmp(sandboxNode->section.name, "el5"), 0);
        // no set, check default
        ASSERT_EQ((sandboxNode->section.sandboxShared == 0) && (sandboxNode->section.sandboxSwitch == 1), 1);
        ASSERT_EQ(sandboxNode->depMode == MOUNT_MODE_NOT_EXIST, 1);
        ASSERT_EQ(sandboxNode->destType == SANDBOX_TAG_APP_VARIABLE, 1);
        ASSERT_EQ(sandboxNode->depNode != nullptr, 1);
        PathMountNode *pathNode = reinterpret_cast<PathMountNode *>(sandboxNode->depNode);
        ASSERT_EQ(pathNode->category, MOUNT_TMP_SHRED);
        ASSERT_EQ(strcmp(pathNode->target, "/data/storage/el5") == 0, 1);

        // check path node
        pathNode = reinterpret_cast<PathMountNode *>(GetFirstSandboxMountPathNode(&sandboxNode->section));
        ASSERT_EQ(pathNode != nullptr, 1);
        ASSERT_EQ(strcmp(pathNode->source, "/data/app/el5/<currentUserId>/base/<PackageName>") == 0, 1);
        ASSERT_EQ(strcmp(pathNode->target, "<deps-path>/base") == 0, 1);
        ret = 0;
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    ASSERT_EQ(ret, 0);
}
/**
 * @brief 沙盒执行，能执行到对应的检查项，并且检查通过
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_mount_001, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/storage/Users/100/appdata/el1";
        args.destinationPath = "/mnt/sandbox/100/app-root/storage/Users/100/appdata/el1";
        args.fsType = "sharefs";
        args.options = "support_overwrite=1";
        args.mountFlags = MS_NODEV | MS_RDONLY;
        args.mountSharedFlag = MS_SLAVE;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);

        ret = StagedMountSystemConst(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief app-variable部分执行。让mount执行失败，但是不需要返回错误结果
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_mount_002, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/config";
        args.destinationPath = "/mnt/sandbox/100/com.example.myapplication/config";
        args.fsType = "sharefs";
        args.mountFlags = MS_NODEV | MS_RDONLY;  // 当前条件走customizedFlags，这里设置为customizedFlags
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);
        // 执行失败, 但是不返回
        args.mountFlags = MS_NODEV;
        ret = MountSandboxConfigs(sandbox, property, 0);
        ASSERT_EQ(ret, 0);
        ASSERT_NE(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief app-variable部分执行。让mount执行失败，失败返回错误结果
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_mount_003, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;

        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, "app-variable");
        ASSERT_EQ(section != nullptr, 1);

        PathMountNode *pathNode = reinterpret_cast<PathMountNode *>(GetFirstSandboxMountPathNode(section));
        pathNode->checkErrorFlag = 1;  // 设置错误检查
        APPSPAWN_LOGV("pathNode %s => %s \n", pathNode->source, pathNode->target);
        // set check point
        MountArg args = {};
        args.originPath = "/config";
        args.destinationPath = "/mnt/sandbox/100/com.example.myapplication/config";
        args.fsType = "sharefs";
        args.mountFlags = MS_NODEV | MS_RDONLY;  // 当前条件走customizedFlags，这里设置为customizedFlags
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);

        // 执行失败, 返回错误
        args.mountFlags = MS_NODEV;
        ret = MountSandboxConfigs(sandbox, property, 0);
        ASSERT_NE(ret, 0);
        ASSERT_NE(stub->result, 0);
        ret = 0;
    } while (0);
    ASSERT_EQ(ret, 0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief package name 执行
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_mount_004, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_packageNameConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/dev/fuse";
        args.destinationPath = "/mnt/sandbox/100/app-root/mnt/data/fuse";
        args.fsType = "fuse";
        args.mountFlags = MS_LAZYTIME | MS_NOATIME | MS_NODEV | MS_NOEXEC | MS_NOSUID;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);

        ret = MountSandboxConfigs(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试package-name执行，执行失败
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_mount_005, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;

        ret = TestParseAppSandboxConfig(sandbox, g_packageNameConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/dev/fuse";
        args.destinationPath = "/home/axw/appspawn_ut/mnt/sandbox/100/com.example.myapplication/mnt/data/fuse";
        args.fsType = "fuse";
        args.mountFlags = MS_LAZYTIME | MS_NOATIME | MS_NODEV | MS_NOEXEC;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);
        ret = MountSandboxConfigs(sandbox, property, 0);
        ASSERT_NE(ret, 0);
        ASSERT_NE(stub->result, 0);
        ret = 0;
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试permission 添加下appFullMountEnable 打开
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_mount_006, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;

        ret = TestParseAppSandboxConfig(sandbox, g_permissionConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        // set permission flags
        int index = GetPermissionIndexInQueue(&sandbox->permissionQueue, "ohos.permission.FILE_ACCESS_MANAGER");
        SetAppPermissionFlags(property, index);

        // set check point
        MountArg args = {};
        args.originPath = "/config--1";
        args.destinationPath = "/mnt/sandbox/100/app-root/data/app/el1/currentUser/"
            "database/com.example.myapplication_100";
        // permission 下，fstype使用default
        // "sharefs"
        args.mountFlags = MS_BIND | MS_REC;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);
        stub->result = 0;
        ret = MountSandboxConfigs(sandbox, property, 0);
        ASSERT_EQ(ret, 0);  // do not check result
        ASSERT_EQ(stub->result, 0);
        ret = 0;
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试permission 添加下appFullMountEnable 打开，执行失败
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_mount_007, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;

        ret = TestParseAppSandboxConfig(sandbox, g_permissionConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        // set permission flags
        int index = GetPermissionIndexInQueue(&sandbox->permissionQueue, "ohos.permission.FILE_ACCESS_MANAGER");
        SetAppPermissionFlags(property, index);

        // set check point
        MountArg args = {};
        args.originPath = "/config--1";
        args.destinationPath = "/mnt/sandbox/100/app-root/data/app/el1/currentUser/"
            "database/com.example.myapplication_100";
        args.fsType = "sharefs";
        args.mountFlags = MS_RDONLY;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);
        ret = MountSandboxConfigs(sandbox, property, 0);
        ASSERT_NE(ret, 0);
        ASSERT_NE(stub->result, 0);
        ret = 0;
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief system-config部分执行，测试每一种模版结果是否正确
 *  测试 shared
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Category_001, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/lib";
        args.destinationPath = "/mnt/sandbox/100/app-root/lib";
        args.fsType = nullptr;
        args.mountFlags = MS_BIND | MS_REC;
        args.mountSharedFlag = MS_SHARED;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);

        ret = StagedMountSystemConst(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief system-config部分执行，测试每一种模版结果是否正确
 *  测试 rdonly
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Category_002, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/lib1";
        args.destinationPath = "/mnt/sandbox/100/app-root/lib1";
        args.fsType = nullptr;
        args.mountFlags = MS_NODEV | MS_RDONLY;
        args.mountSharedFlag = MS_SLAVE;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);

        ret = StagedMountSystemConst(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief system-config部分执行，测试每一种模版结果是否正确
 *  测试 epfs
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Category_003, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "none";
        args.destinationPath = "/mnt/sandbox/100/app-root/storage/cloud/epfs";
        args.fsType = "epfs";
        args.mountFlags = MS_NODEV;
        args.mountSharedFlag = MS_SLAVE;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);

        ret = StagedMountSystemConst(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief system-config部分执行，测试每一种模版结果是否正确
 *  测试 fuse
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Category_004, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/dev/fuse";
        args.destinationPath = "/mnt/sandbox/100/app-root/mnt/data/fuse";
        args.fsType = "fuse";
        args.mountFlags = MS_NOSUID | MS_NODEV | MS_NOEXEC | MS_NOATIME | MS_LAZYTIME;
        args.mountSharedFlag = MS_SLAVE;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);

        ret = StagedMountSystemConst(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试unshare前的执行，not-exist时，节点不存在，执行dep的挂载
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Deps_001, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/data/app/el5/100/base";
        args.destinationPath = "/mnt/sandbox/100/app-root/data/storage/el5";
        args.mountFlags = MS_BIND | MS_REC;
        args.mountSharedFlag = MS_SHARED;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);
        stub->result = -1;

        ret = MountSandboxConfigs(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试unshare前的执行，not-exist时，节点存在，不执行dep的挂载
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Deps_002, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/data/app/el6/100/base";
        args.destinationPath = "/mnt/sandbox/100/app-root/data/storage/el6";
        args.mountFlags = MS_BIND | MS_REC;
        args.mountSharedFlag = MS_SHARED;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);

        ret = MountSandboxConfigs(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试unshare前的执行，always时，执行dep的挂载
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Deps_003, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/data/app/e20/100/base";
        args.destinationPath = "/mnt/sandbox/100/app-root/data/storage/e20";
        args.mountFlags = MS_BIND | MS_REC;
        args.mountSharedFlag = MS_SHARED;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);
        stub->result = 0;

        ret = MountSandboxConfigs(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 测试unshare后执行，一次挂载时，使用sandbox-path
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Deps_004, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/data/app/e15/100/base/com.example.myapplication";
        args.destinationPath = "/mnt/sandbox/100/app-root/data/storage/e15/base";
        args.mountFlags = MS_BIND | MS_REC;
        args.mountSharedFlag = MS_SLAVE;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);
        stub->result = 0;

        ret = MountSandboxConfigs(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief system-const，一次挂载时，使用sandbox-path
 *
 */
HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_Deps_005, TestSize.Level0)
{
    AppSpawnSandboxCfg *sandbox = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    StubNode *stub = GetStubNode(STUB_MOUNT);
    ASSERT_NE(stub != nullptr, 0);
    int ret = -1;
    do {
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);

        ret = APPSPAWN_ARG_INVALID;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);

        sandbox = CreateAppSpawnSandbox();
        APPSPAWN_CHECK_ONLY_EXPER(sandbox != nullptr, break);
        sandbox->appFullMountEnable = 1;
        ret = TestParseAppSandboxConfig(sandbox, g_commonConfig.c_str());
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        // set check point
        MountArg args = {};
        args.originPath = "/data/app/e20/100/base/com.example.myapplication";
        args.destinationPath = "/mnt/sandbox/100/app-root/data/storage/e20/base";
        args.mountFlags = MS_BIND | MS_REC;
        args.mountSharedFlag = MS_SLAVE;
        stub->flags = STUB_NEED_CHECK;
        stub->arg = reinterpret_cast<void *>(&args);
        stub->result = -1;

        ret = StagedMountSystemConst(sandbox, property, 0);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);
        ASSERT_EQ(stub->result, 0);
    } while (0);
    if (sandbox) {
        DeleteAppSpawnSandbox(sandbox);
    }
    stub->flags &= ~STUB_NEED_CHECK;
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    ASSERT_EQ(ret, 0);
}
}  // namespace OHOS
