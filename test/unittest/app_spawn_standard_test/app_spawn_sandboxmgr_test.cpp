/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnSandboxMgrTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief AppSpawnSandboxCfg
 *
 */
HWTEST(AppSpawnSandboxMgrTest, App_Spawn_AppSpawnSandboxCfg_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = GetAppSpawnSandbox(mgr);
    EXPECT_EQ(sandbox == nullptr, 1);

    sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    sandbox = GetAppSpawnSandbox(mgr);
    EXPECT_EQ(sandbox != nullptr, 1);

    // dump
    DumpAppSpawnSandboxCfg(sandbox);

    // delete
    DeleteAppSpawnSandbox(sandbox);
    // get none
    sandbox = GetAppSpawnSandbox(mgr);
    EXPECT_EQ(sandbox == nullptr, 1);
    DumpAppSpawnSandboxCfg(sandbox);

    DeleteAppSpawnMgr(mgr);
    sandbox = GetAppSpawnSandbox(nullptr);
    EXPECT_EQ(sandbox == nullptr, 1);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_AppSpawnSandboxCfg_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    // for appspawn
    int ret = LoadAppSandboxConfig(sandbox, 0);
    EXPECT_EQ(ret, 0);
    ret = LoadAppSandboxConfig(sandbox, 0);  // 重复load
    EXPECT_EQ(ret, 0);

    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);

    ret = LoadAppSandboxConfig(nullptr, 0);
    EXPECT_NE(ret, 0);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_AppSpawnSandboxCfg_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);
    int ret = 0;
#ifdef APPSPAWN_SANDBOX_NEW
    // for nwebspawn
    ret = LoadAppSandboxConfig(sandbox, 1);
    EXPECT_EQ(ret, 0);
    ret = LoadAppSandboxConfig(sandbox, 1);  // 重复load
    EXPECT_EQ(ret, 0);
    ret = LoadAppSandboxConfig(sandbox, 2);  // 重复load
    EXPECT_EQ(ret, 0);
#else
    // for nwebspawn
    ret = LoadAppSandboxConfig(sandbox, 0);
    EXPECT_EQ(ret, 0);
    ret = LoadAppSandboxConfig(sandbox, 0);  // 重复load
    EXPECT_EQ(ret, 0);
#endif
    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);

    ret = LoadAppSandboxConfig(nullptr, 1);
    EXPECT_NE(ret, 0);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_SandboxSection_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    const uint32_t nameCount = 3;
    const uint32_t lenCount = 7;
    const char *inputName[nameCount] = {"test-001", nullptr, ""};
    const uint32_t inputDataLen[lenCount] = {
        0,
        sizeof(SandboxSection),
        sizeof(SandboxPackageNameNode),
        sizeof(SandboxFlagsNode),
        sizeof(SandboxNameGroupNode),
        sizeof(SandboxPermissionNode),
        sizeof(SandboxNameGroupNode) + 100
    };
    int result[lenCount] = {0};
    result[1] = 1;
    result[2] = 1;
    result[3] = 1;
    result[4] = 1;
    result[5] = 1;
    auto testFunc = [result](const char *name, uint32_t len, size_t nameIndex, size_t lenIndex) {
        for (size_t type = SANDBOX_TAG_PERMISSION; type <= SANDBOX_TAG_REQUIRED; type++) {
            SandboxSection *section = CreateSandboxSection(name, len, type);
            EXPECT_EQ(section != nullptr, nameIndex == 0 && result[lenIndex]);
            if (section) {
                EXPECT_EQ(GetSectionType(section), type);
                free(section->name);
                free(section);
            }
        }
    };
    for (size_t i = 0; i < nameCount; i++) {
        for (size_t j = 0; j < lenCount; j++) {
            testFunc(inputName[i], inputDataLen[j], i, j);
        }
    }
    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_SandboxSection_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    const uint32_t nameCount = 3;
    const uint32_t lenCount = 7;
    const char *inputName[nameCount] = {"test-001", nullptr, ""};
    const uint32_t inputDataLen[lenCount] = {
        0,
        sizeof(SandboxSection),
        sizeof(SandboxPackageNameNode),
        sizeof(SandboxFlagsNode),
        sizeof(SandboxNameGroupNode),
        sizeof(SandboxPermissionNode),
        sizeof(SandboxNameGroupNode) + 100
    };
    for (size_t i = 0; i < nameCount; i++) {
        for (size_t j = 0; j < lenCount; j++) {
            SandboxSection *section = CreateSandboxSection(inputName[i], inputDataLen[j], 0);
            EXPECT_EQ(section == nullptr, 1);
            section = CreateSandboxSection(inputName[i], inputDataLen[j], 1);
            EXPECT_EQ(section == nullptr, 1);
            section = CreateSandboxSection(inputName[i], inputDataLen[j], SANDBOX_TAG_INVALID);
            EXPECT_EQ(section == nullptr, 1);
            section = CreateSandboxSection(inputName[i], inputDataLen[j], SANDBOX_TAG_INVALID + 1);
            EXPECT_EQ(section == nullptr, 1);
        }
    }
    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_SandboxSection_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    SandboxSection *section = CreateSandboxSection("system-const", sizeof(SandboxSection), SANDBOX_TAG_SYSTEM_CONST);
    EXPECT_EQ(section != nullptr, 1);
    AddSandboxSection(section, &sandbox->requiredQueue);
    // GetSandboxSection
    section = GetSandboxSection(&sandbox->requiredQueue, "system-const");
    EXPECT_EQ(section != nullptr, 1);
    // DeleteSandboxSection
    DeleteSandboxSection(section);

    // GetSandboxSection has deleted
    section = GetSandboxSection(&sandbox->requiredQueue, "system-const");
    EXPECT_EQ(section != nullptr, 0);

    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_SandboxSection_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    AddSandboxSection(nullptr, &sandbox->requiredQueue);
    AddSandboxSection(nullptr, nullptr);

    // GetSandboxSection
    SandboxSection *section = GetSandboxSection(nullptr, "system-const");
    EXPECT_EQ(section == nullptr, 1);
    section = GetSandboxSection(nullptr, "");
    EXPECT_EQ(section == nullptr, 1);
    section = GetSandboxSection(nullptr, nullptr);
    EXPECT_EQ(section == nullptr, 1);
    section = GetSandboxSection(&sandbox->requiredQueue, "");
    EXPECT_EQ(section == nullptr, 1);
    section = GetSandboxSection(&sandbox->requiredQueue, nullptr);
    EXPECT_EQ(section == nullptr, 1);

    // DeleteSandboxSection
    DeleteSandboxSection(section);
    DeleteSandboxSection(nullptr);

    // GetSandboxSection has deleted
    section = GetSandboxSection(&sandbox->requiredQueue, "system-const");
    EXPECT_EQ(section == nullptr, 1);

    EXPECT_EQ(GetSectionType(nullptr), SANDBOX_TAG_INVALID);

    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief SandboxMountNode
 *
 */
HWTEST(AppSpawnSandboxMgrTest, App_Spawn_SandboxMountNode_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    SandboxSection *section = CreateSandboxSection("system-const", sizeof(SandboxSection), SANDBOX_TAG_SYSTEM_CONST);
    EXPECT_EQ(section != nullptr, 1);
    AddSandboxSection(section, &sandbox->requiredQueue);

    const uint32_t lenCount = 4;
    const uint32_t inputDataLen[lenCount] = {
        0,
        sizeof(PathMountNode),
        sizeof(SymbolLinkNode),
        sizeof(SymbolLinkNode) + 100
    };
    int result[lenCount] = {0, 1, 1, 0};
    for (size_t i = 0; i < lenCount; i++) {
        for (size_t j = 0; j < SANDBOX_TAG_INVALID; j++) {
            SandboxMountNode *path = CreateSandboxMountNode(inputDataLen[i], j);
            EXPECT_EQ(path != nullptr, result[i]);
            if (path) {
                free(path);
            }
        }
    }

    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_SandboxMountNode_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    SandboxSection *section = CreateSandboxSection("system-const", sizeof(SandboxSection), SANDBOX_TAG_SYSTEM_CONST);
    EXPECT_EQ(section != nullptr, 1);
    AddSandboxSection(section, &sandbox->requiredQueue);

    SandboxMountNode *path = CreateSandboxMountNode(sizeof(PathMountNode), SANDBOX_TAG_MOUNT_PATH);
    EXPECT_EQ(path != nullptr, 1);
    AddSandboxMountNode(path, section);

    path = GetFirstSandboxMountNode(section);
    EXPECT_EQ(path != nullptr, 1);
    DeleteSandboxMountNode(path);
    path = GetFirstSandboxMountNode(section);
    EXPECT_EQ(path == nullptr, 1);

    path = GetFirstSandboxMountNode(nullptr);
    EXPECT_EQ(path == nullptr, 1);
    DeleteSandboxMountNode(nullptr);

    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_SandboxMountNode_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    SandboxSection *section = CreateSandboxSection("system-const", sizeof(SandboxSection), SANDBOX_TAG_SYSTEM_CONST);
    EXPECT_EQ(section != nullptr, 1);
    AddSandboxSection(section, &sandbox->requiredQueue);

    SandboxMountNode *path = CreateSandboxMountNode(sizeof(PathMountNode), SANDBOX_TAG_MOUNT_PATH);
    EXPECT_EQ(path != nullptr, 1);
    PathMountNode *pathNode = reinterpret_cast<PathMountNode *>(path);
    const char *testPath = "xxx/xxx/xxx";
    pathNode->source = strdup(testPath);
    pathNode->target = strdup(testPath);
    AddSandboxMountNode(path, section);

    pathNode = GetPathMountNode(section, SANDBOX_TAG_MOUNT_PATH, testPath, testPath);
    EXPECT_EQ(pathNode != nullptr, 1);

    // 异常
    for (size_t j = 0; j < SANDBOX_TAG_INVALID; j++) {
        pathNode = GetPathMountNode(section, j, testPath, testPath);
        EXPECT_EQ(pathNode != nullptr, j == SANDBOX_TAG_MOUNT_PATH);
        pathNode = GetPathMountNode(section, j, nullptr, testPath);
        EXPECT_EQ(pathNode != nullptr, 0);
        pathNode = GetPathMountNode(section, j, nullptr, nullptr);
        EXPECT_EQ(pathNode != nullptr, 0);
        pathNode = GetPathMountNode(section, j, testPath, nullptr);
        EXPECT_EQ(pathNode != nullptr, 0);

        EXPECT_EQ(pathNode != nullptr, 0);
        pathNode = GetPathMountNode(nullptr, j, nullptr, testPath);
        EXPECT_EQ(pathNode != nullptr, 0);
        pathNode = GetPathMountNode(nullptr, j, nullptr, nullptr);
        EXPECT_EQ(pathNode != nullptr, 0);
        pathNode = GetPathMountNode(nullptr, j, testPath, nullptr);
        EXPECT_EQ(pathNode != nullptr, 0);
    }

    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_SandboxMountNode_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    SandboxSection *section = CreateSandboxSection("system-const", sizeof(SandboxSection), SANDBOX_TAG_SYSTEM_CONST);
    EXPECT_EQ(section != nullptr, 1);
    AddSandboxSection(section, &sandbox->requiredQueue);

    SandboxMountNode *path = CreateSandboxMountNode(sizeof(SymbolLinkNode), SANDBOX_TAG_SYMLINK);
    EXPECT_EQ(path != nullptr, 1);
    SymbolLinkNode *pathNode = reinterpret_cast<SymbolLinkNode *>(path);
    const char *testPath = "xxx/xxx/xxx";
    pathNode->linkName = strdup(testPath);
    pathNode->target = strdup(testPath);
    AddSandboxMountNode(path, section);

    pathNode = GetSymbolLinkNode(section, testPath, testPath);
    EXPECT_EQ(pathNode != nullptr, 1);

    // 异常
    pathNode = GetSymbolLinkNode(section, testPath, testPath);
    EXPECT_EQ(pathNode != nullptr, 1);
    pathNode = GetSymbolLinkNode(section, nullptr, testPath);
    EXPECT_EQ(pathNode != nullptr, 0);
    pathNode = GetSymbolLinkNode(section, nullptr, nullptr);
    EXPECT_EQ(pathNode != nullptr, 0);
    pathNode = GetSymbolLinkNode(section, testPath, nullptr);
    EXPECT_EQ(pathNode != nullptr, 0);

    EXPECT_EQ(pathNode != nullptr, 0);
    pathNode = GetSymbolLinkNode(nullptr, nullptr, testPath);
    EXPECT_EQ(pathNode != nullptr, 0);
    pathNode = GetSymbolLinkNode(nullptr, nullptr, nullptr);
    EXPECT_EQ(pathNode != nullptr, 0);
    pathNode = GetSymbolLinkNode(nullptr, testPath, nullptr);
    EXPECT_EQ(pathNode != nullptr, 0);

    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief mount
 *
 */
static AppSpawningCtx *TestCreateAppSpawningCtx()
{
    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);  // 1024 * 2  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    appCtx->message = outMsg;
    return appCtx;
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_Mount_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    AppSpawningCtx *property = TestCreateAppSpawningCtx();
    EXPECT_EQ(property != nullptr, 1);

    // 只做异常测试，正常流程需要基于业务流进行测试
    const AppSpawningCtx *inputAppSpawningCtx[2] = {property, nullptr};
    const AppSpawnSandboxCfg *inputAppSpawnSandboxCfg[2] = {sandbox, nullptr};
    uint32_t inputSpawn[3] = {0, 1, 2};
    int ret = 0;
    for (uint32_t i = 0; i < 2; i++) {          // 2
        for (uint32_t j = 0; j < 2; j++) {      // 2
            for (uint32_t k = 0; k < 2; k++) {  // 2
                ret = MountSandboxConfigs(inputAppSpawnSandboxCfg[i], inputAppSpawningCtx[j], inputSpawn[k]);
                EXPECT_EQ(ret == 0, i == 0 && j == 0);
            }
        }
    }

    for (uint32_t i = 0; i < 2; i++) {          // 2
        for (uint32_t j = 0; j < 2; j++) {      // 2
            for (uint32_t k = 0; k < 2; k++) {  // 2
                ret = StagedMountSystemConst(inputAppSpawnSandboxCfg[i], inputAppSpawningCtx[j], inputSpawn[k]);
                EXPECT_EQ(ret == 0, i == 0 && j == 0);
            }
        }
    }

    DeleteAppSpawningCtx(property);
    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_Mount_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    SandboxContext context = {};
    // 只做异常测试，正常流程需要基于业务流进行测试
    const SandboxContext *inputContext[2] = {&context, nullptr};
    const AppSpawnSandboxCfg *inputAppSpawnSandboxCfg[2] = {sandbox, nullptr};

    int ret = 0;
    for (uint32_t i = 0; i < 2; i++) {      // 2
        for (uint32_t j = 0; j < 2; j++) {  // 2
            ret = StagedMountPreUnShare(inputContext[i], inputAppSpawnSandboxCfg[j]);
            EXPECT_EQ(ret == 0, i == 0 && j == 0);
        }
    }

    for (uint32_t i = 0; i < 2; i++) {      // 2
        for (uint32_t j = 0; j < 2; j++) {  // 2
            ret = StagedMountPostUnshare(inputContext[i], inputAppSpawnSandboxCfg[j]);
            EXPECT_EQ(ret == 0, i == 0 && j == 0);
        }
    }

    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_Mount_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    EXPECT_EQ(sandbox != nullptr, 1);
    sandbox->rootPath = strdup("/data/appspawn_ut/sandbox/");
    OH_ListAddTail(&sandbox->extData.node, &mgr->extData);

    // 只做异常测试，正常流程需要基于业务流进行测试
    const AppSpawnSandboxCfg *inputAppSpawnSandboxCfg[2] = {sandbox, nullptr};
    const char *inputName[2] = {"test", nullptr};

    for (uint32_t k = 0; k < 2; k++) {  // 2
        int ret = UnmountDepPaths(inputAppSpawnSandboxCfg[k], 0);
        EXPECT_EQ(ret == 0, k == 0);
    }
    for (uint32_t i = 0; i < 2; i++) {      // 2
        for (uint32_t k = 0; k < 2; k++) {  // 2
            int ret = UnmountSandboxConfigs(inputAppSpawnSandboxCfg[k], 0, inputName[i]);
            EXPECT_EQ(ret == 0, k == 0 && i == 0);
        }
    }
    DeleteAppSpawnSandbox(sandbox);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief SandboxMountPath
 *
 */
HWTEST(AppSpawnSandboxMgrTest, App_Spawn_Add_App_SandboxMountPath_001, TestSize.Level0)
{
    MountArg arg = {};
    arg.originPath = "/data/";
    arg.destinationPath = "/data/appspawn/test";
    arg.mountSharedFlag = 1;

    int ret = SandboxMountPath(&arg);
    EXPECT_EQ(ret, 0);
    ret = SandboxMountPath(&arg);
    EXPECT_EQ(ret, 0);
    ret = SandboxMountPath(nullptr);
    EXPECT_NE(ret, 0);
    arg.mountSharedFlag = -1;
    ret = SandboxMountPath(&arg);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief AddVariableReplaceHandler
 *
 */
static int TestReplaceVarHandler(const SandboxContext *context,
    const char *buffer, uint32_t bufferLen, uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_AddVariableReplaceHandler_001, TestSize.Level0)
{
    int ret = AddVariableReplaceHandler(nullptr, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    ret = AddVariableReplaceHandler("xxx", nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    ret = AddVariableReplaceHandler(nullptr, TestReplaceVarHandler);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    ret = AddVariableReplaceHandler("global", TestReplaceVarHandler);
    EXPECT_EQ(ret, 0);
    ret = AddVariableReplaceHandler("global", TestReplaceVarHandler);
    EXPECT_EQ(ret, APPSPAWN_NODE_EXIST);

    ret = AddVariableReplaceHandler("<Test-Var-005>", TestReplaceVarHandler);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief RegisterExpandSandboxCfgHandler
 *
 */
static int TestProcessExpandSandboxCfg(const SandboxContext *context,
    const AppSpawnSandboxCfg *appSandBox, const char *name)
{
    return 0;
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_RegisterExpandSandboxCfgHandler_001, TestSize.Level0)
{
    int ret = RegisterExpandSandboxCfgHandler(nullptr, 0, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    ret = RegisterExpandSandboxCfgHandler("test", 0, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    ret = RegisterExpandSandboxCfgHandler(nullptr, 0, TestProcessExpandSandboxCfg);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    ret = RegisterExpandSandboxCfgHandler("test-001", 0, TestProcessExpandSandboxCfg);
    EXPECT_EQ(ret, 0);
    ret = RegisterExpandSandboxCfgHandler("test-001", 0, TestProcessExpandSandboxCfg);
    EXPECT_EQ(ret, APPSPAWN_NODE_EXIST);
    ret = RegisterExpandSandboxCfgHandler("test-001", -1, TestProcessExpandSandboxCfg);
    EXPECT_EQ(ret, APPSPAWN_NODE_EXIST);
}

/**
 * @brief permission test
 *
 */
HWTEST(AppSpawnSandboxMgrTest, App_Spawn_Permission_001, TestSize.Level0)
{
    int ret = LoadPermission(CLIENT_FOR_APPSPAWN);
    EXPECT_EQ(ret, 0);
    ret = LoadPermission(CLIENT_FOR_NWEBSPAWN);
    EXPECT_EQ(ret, 0);
    ret = LoadPermission(CLIENT_MAX);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    int32_t max = GetPermissionMaxCount();
    EXPECT_EQ(max >= 0, 1);

    DeletePermission(CLIENT_FOR_APPSPAWN);
    DeletePermission(CLIENT_FOR_NWEBSPAWN);
    DeletePermission(CLIENT_MAX);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_Permission_002, TestSize.Level0)
{
    int ret = LoadPermission(CLIENT_FOR_APPSPAWN);
    EXPECT_EQ(ret, 0);
    ret = LoadPermission(CLIENT_FOR_NWEBSPAWN);
    EXPECT_EQ(ret, 0);

    int32_t max = GetPermissionMaxCount();
    EXPECT_EQ(max >= 0, 1);

    AppSpawnClientHandle clientHandle;
    ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    EXPECT_EQ(ret, 0);

    max = GetMaxPermissionIndex(clientHandle);
    int32_t index = GetPermissionIndex(clientHandle, "ohos.permission.ACCESS_BUNDLE_DIR");
    EXPECT_EQ(index >= 0, 1);
    EXPECT_EQ(index < max, 1);

    const char *permission = GetPermissionByIndex(clientHandle, index);
    EXPECT_EQ(permission != nullptr, 1);
    EXPECT_EQ(strcmp(permission, "ohos.permission.ACCESS_BUNDLE_DIR") == 0, 1);
    AppSpawnClientDestroy(clientHandle);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_Permission_003, TestSize.Level0)
{
    int ret = LoadPermission(CLIENT_FOR_APPSPAWN);
    EXPECT_EQ(ret, 0);
    ret = LoadPermission(CLIENT_FOR_NWEBSPAWN);
    EXPECT_EQ(ret, 0);
    ret = LoadPermission(CLIENT_MAX);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    int32_t max = GetPermissionMaxCount();
    EXPECT_EQ(max >= 0, 1);

    AppSpawnClientHandle clientHandle;
    ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    EXPECT_EQ(ret, 0);

#ifndef APPSPAWN_SANDBOX_NEW
    max = GetMaxPermissionIndex(clientHandle);
    int32_t index = GetPermissionIndex(clientHandle, "ohos.permission.ACCESS_BUNDLE_DIR");
    EXPECT_EQ(index >= 0, 1);
    EXPECT_EQ(max >= index, 1);

    const char *permission = GetPermissionByIndex(clientHandle, index);
    EXPECT_EQ(permission != nullptr, 1);
    EXPECT_EQ(strcmp(permission, "ohos.permission.ACCESS_BUNDLE_DIR") == 0, 1);
#else
    // nweb no permission, so max = 0
    max = GetMaxPermissionIndex(clientHandle);
    EXPECT_EQ(max, 0);
#endif
    AppSpawnClientDestroy(clientHandle);
}

HWTEST(AppSpawnSandboxMgrTest, App_Spawn_Permission_004, TestSize.Level0)
{
    int ret = LoadPermission(CLIENT_FOR_APPSPAWN);
    EXPECT_EQ(ret, 0);
    ret = LoadPermission(CLIENT_FOR_NWEBSPAWN);
    EXPECT_EQ(ret, 0);
    ret = LoadPermission(CLIENT_MAX);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    int32_t max = GetPermissionMaxCount();
    EXPECT_EQ(max >= 0, 1);

    AppSpawnClientHandle clientHandle;
    ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    EXPECT_EQ(ret, 0);

    max = GetMaxPermissionIndex(nullptr);
    EXPECT_EQ(max >= 0, 1);

    int32_t index = GetPermissionIndex(clientHandle, nullptr);
    EXPECT_EQ(index, INVALID_PERMISSION_INDEX);
    index = GetPermissionIndex(nullptr, "ohos.permission.ACCESS_BUNDLE_DIR");
    EXPECT_EQ(max >= index, 1);
    const char *permission = GetPermissionByIndex(clientHandle, INVALID_PERMISSION_INDEX);
    EXPECT_EQ(permission == nullptr, 1);

    AppSpawnClientDestroy(clientHandle);
}
}  // namespace OHOS
