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
class AppSpawnModuleInterfaceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief test for appspawn_manager.h
 *
 */
/**
 * @brief IsNWebSpawnMode
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_IsNWebSpawnMode_001, TestSize.Level0)
{
    int ret = IsNWebSpawnMode(nullptr);
    EXPECT_EQ(0, ret);  //  false
    AppSpawnMgr content;
    content.content.mode = MODE_FOR_NWEB_SPAWN;
    ret = IsNWebSpawnMode(&content);
    EXPECT_EQ(1, ret);  //  true

    content.content.mode = MODE_INVALID;
    ret = IsNWebSpawnMode(&content);
    EXPECT_EQ(0, ret);  //  true
}

/**
 * @brief IsColdRunMode
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_IsColdRunMode_001, TestSize.Level0)
{
    int ret = IsColdRunMode(nullptr);
    EXPECT_EQ(0, ret);  //  false
    AppSpawnMgr content;
    content.content.mode = MODE_FOR_APP_COLD_RUN;
    ret = IsColdRunMode(&content);
    EXPECT_EQ(1, ret);  //  true

    content.content.mode = MODE_FOR_NWEB_COLD_RUN;
    ret = IsColdRunMode(&content);
    EXPECT_EQ(1, ret);  //  true

    content.content.mode = MODE_INVALID;
    ret = IsColdRunMode(&content);
    EXPECT_EQ(0, ret);  //  true
}

/**
 * @brief GetAppSpawnMsgType
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetAppSpawnMsgType_001, TestSize.Level0)
{
    int ret = GetAppSpawnMsgType(nullptr);
    EXPECT_EQ(MAX_TYPE_INVALID, ret);
    AppSpawningCtx content = {};
    AppSpawnMsgNode node = {};
    content.message = &node;
    ret = GetAppSpawnMsgType(&content);
    EXPECT_NE(MAX_TYPE_INVALID, ret);
    content.message->msgHeader.msgType = MODE_INVALID;
    ret = GetAppSpawnMsgType(&content);
    EXPECT_EQ(MAX_TYPE_INVALID, ret);
}

/**
 * @brief GetBundleName
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetBundleName_001, TestSize.Level0)
{
    const char *bundleName = GetBundleName(nullptr);
    EXPECT_EQ(nullptr, bundleName);
}

/**
 * @brief GetAppProperty
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetAppProperty_001, TestSize.Level0)
{
    void *ret = GetAppProperty(nullptr, 8);
    EXPECT_EQ(nullptr, ret);  //  false

    AppSpawningCtx content = {};
    AppSpawnMsgNode node = {};
    content.message = &node;
    ret = GetAppProperty(&content, 8);
    EXPECT_EQ(nullptr, ret);  //  true

    ret = GetAppProperty(&content, 9);
    EXPECT_EQ(nullptr, ret);  //  true

    ret = GetAppProperty(&content, 10);
    EXPECT_EQ(nullptr, ret);  //  true
}

/**
 * @brief GetProcessName
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetProcessName_001, TestSize.Level0)
{
    const char *ret = GetProcessName(nullptr);
    EXPECT_EQ(nullptr, ret);  //  false

    AppSpawningCtx content = {};
    AppSpawnMsgNode node = {};
    content.message = &node;
    const char *processName = "com.example.myapplication";
    int isRst = strncpy_s(content.message->msgHeader.processName, sizeof(content.message->msgHeader.processName) - 1,
                          processName, strlen(processName));
    EXPECT_EQ(0, isRst);
    ret = GetProcessName(&content);
    EXPECT_NE(nullptr, ret);  //  true
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_IsDeveloperModeOn_001, TestSize.Level0)
{
    int ret = IsDeveloperModeOn(nullptr);
    EXPECT_EQ(0, ret);  //  false

    AppSpawningCtx content = {};
    content.client.flags = APP_DEVELOPER_MODE;
    ret = IsDeveloperModeOn(&content);
    EXPECT_EQ(1, ret);
    content.client.flags = APP_COLD_START;
    ret = IsDeveloperModeOn(&content);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetAppPropertyExt_001, TestSize.Level0)
{
    uint32_t size = strlen("xxx");
    void *ret = GetAppPropertyExt(nullptr, "xxx", &size);
    EXPECT_EQ(nullptr, ret);  //  false
    AppSpawningCtx context;
    ret = GetAppPropertyExt(&context, nullptr, &size);
    EXPECT_EQ(nullptr, ret);  //  false

    context.message = nullptr;
    ret = GetAppPropertyExt(&context, "name", &size);
    EXPECT_EQ(nullptr, ret);  //  false
}

/**
 * @brief test interface in appspawn_modulemgr.h
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AppSpawnModuleMgrInstall_001, TestSize.Level1)
{
    int ret = AppSpawnModuleMgrInstall(nullptr);
    EXPECT_NE(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_MAX);
    EXPECT_NE(0, ret);

    ret = AppSpawnLoadAutoRunModules(MODULE_MAX);
    EXPECT_NE(0, ret);
    ret = AppSpawnModuleMgrInstall("/");
    EXPECT_EQ(0, ret);
    ret = AppSpawnModuleMgrInstall("");
    EXPECT_EQ(0, ret);
    ret = AppSpawnModuleMgrInstall("libappspawn_asan");
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AppSpawnLoadAutoRunModules_001, TestSize.Level1)
{
    int ret = AppSpawnLoadAutoRunModules(MODULE_DEFAULT);
    EXPECT_EQ(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_APPSPAWN);
    EXPECT_EQ(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_NWEBSPAWN);
    EXPECT_EQ(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_COMMON);
    EXPECT_EQ(0, ret);
    ret = AppSpawnLoadAutoRunModules(MODULE_MAX);
    EXPECT_NE(0, ret);
    ret = AppSpawnLoadAutoRunModules(100);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AppSpawnModuleMgrUnInstall_001, TestSize.Level1)
{
    AppSpawnModuleMgrUnInstall(MODULE_DEFAULT);
    AppSpawnModuleMgrUnInstall(MODULE_APPSPAWN);
    AppSpawnModuleMgrUnInstall(MODULE_NWEBSPAWN);
    AppSpawnModuleMgrUnInstall(MODULE_COMMON);
    AppSpawnModuleMgrUnInstall(MODULE_MAX);
    AppSpawnModuleMgrUnInstall(100);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_DeleteAppSpawnHookMgr_001, TestSize.Level1)
{
    DeleteAppSpawnHookMgr();
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_ServerStageHookExecute_001, TestSize.Level1)
{
    int ret = ServerStageHookExecute(static_cast<AppSpawnHookStage>(0), nullptr);
    EXPECT_NE(0, ret);

    AppSpawnContent content;
    content.longProcName = nullptr;
    content.longProcNameLen = 256;
    content.coldStartApp = nullptr;
    content.runAppSpawn = nullptr;
    ret = ServerStageHookExecute(static_cast<AppSpawnHookStage>(0), &content);
    EXPECT_NE(0, ret);
}

/**
 * @brief int ProcessMgrHookExecute(AppSpawnHookStage stage, const AppSpawnContent *content,
 * const AppSpawnedProcess *appInfo);
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_ProcessMgrHookExecute_001, TestSize.Level1)
{
    TagAppSpawnedProcess appInfo;
    appInfo.pid = 33;                 // 33
    appInfo.uid = 200000 * 200 + 21;  // 200000 200 21
    appInfo.max = 0;
    appInfo.exitStatus = 0;
    AppSpawnContent content;
    content.longProcName = nullptr;
    content.longProcNameLen = 256;
    content.coldStartApp = nullptr;
    content.runAppSpawn = nullptr;
    int ret = ProcessMgrHookExecute(STAGE_SERVER_APP_ADD, &content, &appInfo);
    EXPECT_EQ(0, ret);
    ret = ProcessMgrHookExecute(STAGE_SERVER_APP_DIED, &content, &appInfo);
    EXPECT_EQ(0, ret);
    ret = ProcessMgrHookExecute(STAGE_SERVER_APP_DIED, nullptr, nullptr);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AppSpawnHookExecute_001, TestSize.Level1)
{
    int ret = AppSpawnHookExecute(STAGE_PARENT_PRE_FORK, 0, nullptr, nullptr);
    EXPECT_NE(0, ret);
}

/**
 * @brief AddPreloadHook
 *
 */
static int TestAppPreload(AppSpawnMgr *content)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Add_Preload_Hook_001, TestSize.Level0)
{
    int ret = AddPreloadHook(HOOK_PRIO_HIGHEST, TestAppPreload);
    EXPECT_EQ(0, ret);
    ret = AddPreloadHook(-1, TestAppPreload);
    EXPECT_EQ(0, ret);
    ret = AddPreloadHook(1001, TestAppPreload);
    EXPECT_EQ(0, ret);
    ret = AddPreloadHook(8000, TestAppPreload);  // 8000 正常
    EXPECT_EQ(0, ret);
    ret = AddPreloadHook(8000, nullptr);  // 影响功能测试
    EXPECT_NE(0, ret);
}

/**
 * @brief AddAppSpawnHook
 *
 */
static int TestAppSpawn(AppSpawnMgr *content, AppSpawningCtx *property)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Add_App_Spawn_Hook_001, TestSize.Level0)
{
    int ret = AddAppSpawnHook(STAGE_CHILD_POST_RELY, HOOK_PRIO_HIGHEST, TestAppSpawn);
    EXPECT_EQ(0, ret);

    ret = AddAppSpawnHook(static_cast<AppSpawnHookStage>(-1), HOOK_PRIO_HIGHEST, TestAppSpawn);
    EXPECT_NE(0, ret);
    ret = AddAppSpawnHook(static_cast<AppSpawnHookStage>(1), HOOK_PRIO_HIGHEST, TestAppSpawn);
    EXPECT_NE(0, ret);
    ret = AddAppSpawnHook(static_cast<AppSpawnHookStage>(11), HOOK_PRIO_HIGHEST, TestAppSpawn);
    EXPECT_NE(0, ret);
    ret = AddAppSpawnHook(static_cast<AppSpawnHookStage>(53), HOOK_PRIO_HIGHEST, TestAppSpawn);
    EXPECT_NE(0, ret);
    ret = AddAppSpawnHook(STAGE_CHILD_POST_RELY, -1, TestAppSpawn);
    EXPECT_EQ(0, ret);
    ret = AddAppSpawnHook(STAGE_CHILD_POST_RELY, 1001, TestAppSpawn);
    EXPECT_EQ(0, ret);
    ret = AddAppSpawnHook(STAGE_CHILD_POST_RELY, 8000, TestAppSpawn);  // 8000 正常
    EXPECT_EQ(0, ret);
    ret = AddAppSpawnHook(STAGE_CHILD_POST_RELY, 8000, nullptr);  // 影响功能测试
    EXPECT_NE(0, ret);
}

/**
 * @brief AddProcessMgrHook
 *
 */
static int ReportProcessExitInfo(const AppSpawnMgr *content, const AppSpawnedProcess *appInfo)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Add_App_Change_Hook_001, TestSize.Level0)
{
    int ret = AddProcessMgrHook(STAGE_SERVER_APP_DIED, 0, ReportProcessExitInfo);
    EXPECT_EQ(0, ret);
    ret = AddProcessMgrHook(static_cast<AppSpawnHookStage>(-1), 0, ReportProcessExitInfo);
    EXPECT_NE(0, ret);
    ret = AddProcessMgrHook(static_cast<AppSpawnHookStage>(1), 0, ReportProcessExitInfo);
    EXPECT_NE(0, ret);
    ret = AddProcessMgrHook(static_cast<AppSpawnHookStage>(11), 0, ReportProcessExitInfo);
    EXPECT_NE(0, ret);
    ret = AddProcessMgrHook(static_cast<AppSpawnHookStage>(53), 0, ReportProcessExitInfo);
    EXPECT_NE(0, ret);
    ret = AddProcessMgrHook(STAGE_SERVER_APP_DIED, -1, ReportProcessExitInfo);
    EXPECT_EQ(0, ret);
    ret = AddProcessMgrHook(STAGE_SERVER_APP_DIED, 1001, ReportProcessExitInfo);
    EXPECT_EQ(0, ret);
    ret = AddProcessMgrHook(STAGE_SERVER_APP_DIED, 8000, ReportProcessExitInfo);  // 8000 正常
    EXPECT_EQ(0, ret);
    ret = AddProcessMgrHook(STAGE_SERVER_APP_ADD, 0, nullptr);  // 影响功能测试
    EXPECT_NE(0, ret);
}

/**
 * @brief RegChildLooper
 *
 */
static int TestChildLoop(AppSpawnContent *content, AppSpawnClient *client)
{
    return 0;
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Add_App_RegChildLooper_001, TestSize.Level0)
{
    RegChildLooper(nullptr, TestChildLoop);

    AppSpawnContent content;
    content.longProcName = nullptr;
    content.longProcNameLen = 256;
    content.coldStartApp = nullptr;
    content.runAppSpawn = nullptr;
    RegChildLooper(&content, nullptr);
    RegChildLooper(&content, TestChildLoop);
}

/**
 * @brief MakeDirRec
 * int MakeDirRec(const char *path, mode_t mode, int lastPath);
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Add_App_MakeDirRec_001, TestSize.Level0)
{
    int ret = MakeDirRec(nullptr, 0, -1);
    EXPECT_EQ(ret, -1);
    ret = MakeDirRec("/data/appspawn/", 0, -1);
    EXPECT_NE(ret, -1);
}

/**
 * @brief SandboxMountPath
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Add_App_SandboxMountPath_001, TestSize.Level0)
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
static int TestReplaceVarHandler(const SandboxContext *context, const char *buffer, uint32_t bufferLen,
                                 uint32_t *realLen, const VarExtraData *extraData)
{
    return 0;
}
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AddVariableReplaceHandler_001, TestSize.Level0)
{
    int ret = AddVariableReplaceHandler(nullptr, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    ret = AddVariableReplaceHandler("xxx", nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    ret = AddVariableReplaceHandler(nullptr, TestReplaceVarHandler);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    ret = AddVariableReplaceHandler("global", TestReplaceVarHandler);
    EXPECT_EQ(ret, 0);

    ret = AddVariableReplaceHandler("<Test-Var-001>", TestReplaceVarHandler);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief RegisterExpandSandboxCfgHandler
 *
 */
static int TestProcessExpandSandboxCfg(const SandboxContext *context, const AppSpawnSandboxCfg *appSandBox,
                                       const char *name)
{
    return 0;
}
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_RegisterExpandSandboxCfgHandler_001, TestSize.Level0)
{
    int ret = RegisterExpandSandboxCfgHandler(nullptr, 0, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    ret = RegisterExpandSandboxCfgHandler("test", 0, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    ret = RegisterExpandSandboxCfgHandler(nullptr, 0, TestProcessExpandSandboxCfg);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

#ifdef APPSPAWN_SANDBOX_NEW
    ret = RegisterExpandSandboxCfgHandler("HspList", 0, TestProcessExpandSandboxCfg);
    EXPECT_EQ(ret, APPSPAWN_NODE_EXIST);
#else
    ret = RegisterExpandSandboxCfgHandler("HspList", 0, TestProcessExpandSandboxCfg);
    EXPECT_EQ(ret, 0);
#endif
    ret = RegisterExpandSandboxCfgHandler("HspList", -1, TestProcessExpandSandboxCfg);
    EXPECT_EQ(ret, APPSPAWN_NODE_EXIST);
}

/**
 * @brief AddSandboxPermissionNode
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_AddSandboxPermissionNode_001, TestSize.Level1)
{
    int ret = AddSandboxPermissionNode(nullptr, nullptr);
    EXPECT_NE(0, ret);
    ret = AddSandboxPermissionNode("xxx", nullptr);
    EXPECT_NE(0, ret);

    SandboxQueue queue;
    OH_ListInit(&queue.front);
    EXPECT_NE(nullptr, &queue.front);
    ret = AddSandboxPermissionNode(nullptr, &queue);
    EXPECT_NE(0, ret);

    ret = AddSandboxPermissionNode("xxx", &queue);
    EXPECT_EQ(0, ret);
}

/**
 * @brief GetPermissionIndexInQueue
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetPermissionIndexInQueue_001, TestSize.Level1)
{
    int ret = GetPermissionIndexInQueue(nullptr, nullptr);
    EXPECT_NE(0, ret);
    ret = GetPermissionIndexInQueue(nullptr, "xxx");
    EXPECT_NE(0, ret);

    SandboxQueue queue;
    OH_ListInit(&queue.front);
    ret = GetPermissionIndexInQueue(&queue, nullptr);
    EXPECT_NE(0, ret);

    ret = AddSandboxPermissionNode("test", &queue);
    EXPECT_EQ(0, ret);

    ret = GetPermissionIndexInQueue(&queue, "test1");
    EXPECT_NE(0, ret);

    ret = GetPermissionIndexInQueue(&queue, "xxx");
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetPermissionNodeInQueue_001, TestSize.Level1)
{
    EXPECT_EQ(nullptr, GetPermissionNodeInQueue(nullptr, nullptr));
    EXPECT_EQ(nullptr, GetPermissionNodeInQueue(nullptr, "xxx"));

    SandboxQueue queue;
    OH_ListInit(&queue.front);
    EXPECT_EQ(nullptr, GetPermissionNodeInQueue(&queue, nullptr));
    EXPECT_EQ(nullptr, GetPermissionNodeInQueue(&queue, "xxxxx"));
    EXPECT_EQ(nullptr, GetPermissionNodeInQueue(&queue, "xxx"));
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetPermissionNodeInQueueByIndex_001, TestSize.Level1)
{
    const SandboxPermissionNode *node = GetPermissionNodeInQueueByIndex(nullptr, 0);
    EXPECT_EQ(nullptr, node);
    node = GetPermissionNodeInQueueByIndex(nullptr, 0);
    EXPECT_EQ(nullptr, node);

    SandboxQueue queue;
    OH_ListInit(&queue.front);
    int ret = AddSandboxPermissionNode("xxx", &queue);
    EXPECT_EQ(0, ret);
    node = GetPermissionNodeInQueueByIndex(&queue, 0);
    EXPECT_NE(nullptr, node);
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_PermissionRenumber_001, TestSize.Level1)
{
    int ret = PermissionRenumber(nullptr);
    EXPECT_NE(0, ret);

    SandboxQueue queue;
    OH_ListInit(&queue.front);
    ret = PermissionRenumber(&queue);
    EXPECT_EQ(0, ret);
}

/**
 * @brief AppSpawnSandboxCfg op
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetAppSpawnSandbox_001, TestSize.Level1)
{
    AppSpawnSandboxCfg *cfg = CreateAppSpawnSandbox();
    EXPECT_NE(nullptr, cfg);

    AppSpawnSandboxCfg *sandboxCfg = GetAppSpawnSandbox(nullptr);
    EXPECT_EQ(nullptr, sandboxCfg);

    AppSpawnMgr content;
    OH_ListInit(&content.extData);
    sandboxCfg = GetAppSpawnSandbox(&content);
    EXPECT_NE(nullptr, cfg);

    int ret = LoadAppSandboxConfig(nullptr, 0);
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ret);

    DumpAppSpawnSandboxCfg(nullptr);
    DumpAppSpawnSandboxCfg(cfg);

    DeleteAppSpawnSandbox(cfg);
    DeleteAppSpawnSandbox(nullptr);
}

/**
 * @brief SandboxSection op
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_CreateSandboxSection_001, TestSize.Level1)
{
    EXPECT_EQ(nullptr, CreateSandboxSection(nullptr, 0, 0));
    EXPECT_EQ(nullptr, CreateSandboxSection("xxx", 99999, 0));
    EXPECT_EQ(nullptr, CreateSandboxSection("xxx", 0, 0));
    EXPECT_EQ(nullptr, CreateSandboxSection("xxx", 111, 0));

    SandboxQueue queue;
    OH_ListInit(&queue.front);
    EXPECT_EQ(nullptr, GetSandboxSection(nullptr, nullptr));
    EXPECT_EQ(nullptr, GetSandboxSection(nullptr, "xxx"));
    EXPECT_EQ(nullptr, GetSandboxSection(&queue, nullptr));
    EXPECT_EQ(nullptr, GetSandboxSection(&queue, ""));
    EXPECT_EQ(nullptr, GetSandboxSection(&queue, "xxx"));

    SandboxSection node;
    OH_ListInit(&node.front);
    OH_ListInit(&node.sandboxNode.node);

    SandboxMountNode *mountNode = (SandboxMountNode *)malloc(sizeof(SandboxMountNode));
    OH_ListInit(&mountNode->node);
    node.nameGroups = &mountNode;

    AddSandboxSection(nullptr, nullptr);
    AddSandboxSection(nullptr, &queue);
    AddSandboxSection(&node, nullptr);
    AddSandboxSection(&node, &queue);

    EXPECT_EQ(SANDBOX_TAG_INVALID, GetSectionType(nullptr));
    EXPECT_NE(SANDBOX_TAG_INVALID, GetSectionType(&node));

    DeleteSandboxSection(nullptr);
}

/**
 * @brief SandboxMountNode op
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_CreateAppSpawnSandbox_001, TestSize.Level1)
{
    EXPECT_EQ(nullptr, CreateSandboxMountNode(0, 0));
    EXPECT_EQ(nullptr, CreateSandboxMountNode(sizeof(SandboxMountNode) - 1, 0));
    EXPECT_EQ(nullptr, CreateSandboxMountNode(sizeof(PathMountNode) + 1, 0));
    SandboxMountNode *node = CreateSandboxMountNode(sizeof(PathMountNode), 0);
    EXPECT_NE(nullptr, node);
    OH_ListInit(&node->node);

    SandboxSection *section = CreateSandboxSection("xxx", sizeof(SandboxSection), 0);
    OH_ListInit(&section->front);
    ListNode item;
    OH_ListInit(&item);
    OH_ListAddTail(&section->front, &item);
    EXPECT_EQ(nullptr, GetFirstSandboxMountNode(nullptr));
    EXPECT_NE(nullptr, GetFirstSandboxMountNode(section));

    SandboxSection section1;
    OH_ListInit(&section1.front);
    EXPECT_EQ(nullptr, GetFirstSandboxMountNode(&section1));

    OH_ListInit(&section->front);
    OH_ListInit(&section->sandboxNode.node);
    AddSandboxMountNode(node, section);
    AddSandboxMountNode(nullptr, nullptr);
    AddSandboxMountNode(node, nullptr);
    AddSandboxMountNode(nullptr, section);

    DeleteSandboxMountNode(nullptr);
    DeleteSandboxMountNode(node);
    SandboxMountNode *node1 = CreateSandboxMountNode(sizeof(PathMountNode), 0);
    EXPECT_NE(nullptr, node1);
    OH_ListInit(&node1->node);
    node1->type = SANDBOX_TAG_INVALID;
    DeleteSandboxMountNode(node1);
}

/**
 * @brief sandbox mount interface
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_MountSandboxConfigs_001, TestSize.Level1)
{
    AppSpawnSandboxCfg *sandbox = CreateAppSpawnSandbox();
    AppSpawnClientHandle clientHandle;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgHandle reqHandle;
    ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);

    AppSpawningCtx property;
    OH_ListInit(&property.node);
    EXPECT_NE(0, MountSandboxConfigs(nullptr, nullptr, 0));
    EXPECT_NE(0, MountSandboxConfigs(sandbox, nullptr, 0));

    EXPECT_NE(0, StagedMountSystemConst(nullptr, nullptr, 0));
    EXPECT_NE(0, StagedMountSystemConst(sandbox, nullptr, 0));

    SandboxContext content;
    EXPECT_EQ(0, StagedMountPreUnShare(&content, sandbox));
    EXPECT_NE(0, StagedMountPreUnShare(nullptr, nullptr));
    EXPECT_NE(0, StagedMountPreUnShare(&content, nullptr));
    EXPECT_NE(0, StagedMountPreUnShare(nullptr, sandbox));

    EXPECT_NE(0, UnmountSandboxConfigs(sandbox, 0, nullptr));
    EXPECT_NE(0, UnmountSandboxConfigs(nullptr, 0, nullptr));
}

/**
 * @brief Variable op
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_GetSandboxRealVar_001, TestSize.Level1)
{
    AddDefaultVariable();
    const char *var = GetSandboxRealVar(nullptr, 0, "", "", nullptr);
    EXPECT_EQ(nullptr, var);

    SandboxContext content;
    var = GetSandboxRealVar(&content, 4, "", "", nullptr);
    EXPECT_EQ(nullptr, var);

    ClearVariable();
}

/**
 * @brief expand config
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_expand_config_001, TestSize.Level1)
{
    SandboxContext content;
    AppSpawnSandboxCfg *cfg = CreateAppSpawnSandbox();
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ProcessExpandAppSandboxConfig(nullptr, nullptr, nullptr));
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ProcessExpandAppSandboxConfig(nullptr, cfg, nullptr));
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ProcessExpandAppSandboxConfig(nullptr, cfg, "name"));
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ProcessExpandAppSandboxConfig(&content, nullptr, nullptr));
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ProcessExpandAppSandboxConfig(&content, cfg, nullptr));
    EXPECT_NE(APPSPAWN_ARG_INVALID, ProcessExpandAppSandboxConfig(&content, cfg, "test"));
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ProcessExpandAppSandboxConfig(&content, cfg, nullptr));
    EXPECT_NE(APPSPAWN_ARG_INVALID, ProcessExpandAppSandboxConfig(&content, cfg, "test"));

    AddDefaultExpandAppSandboxConfigHandle();
    ClearExpandAppSandboxConfigHandle();
}

/**
 * @brief Sandbox Context op
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Sandbox_Content_Op_001, TestSize.Level1)
{
    SandboxContext *sandboxContent = GetSandboxContext();
    EXPECT_NE(nullptr, sandboxContent);
    DeleteSandboxContext(nullptr);
    DeleteSandboxContext(sandboxContent);
}

/**
 * @brief Sandbox Context op
 *
 */
HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Sandbox_Op_001, TestSize.Level1)
{
    EXPECT_EQ(MOUNT_TMP_DEFAULT, GetMountCategory(""));
    EXPECT_EQ(MOUNT_TMP_DEFAULT, GetMountCategory(nullptr));
    EXPECT_EQ(MOUNT_TMP_DEFAULT, GetMountCategory("name"));
}

HWTEST(AppSpawnModuleInterfaceTest, App_Spawn_Sandbox_Op_002, TestSize.Level1)
{
    PathMountNode pathNode;
    pathNode.category = 123456;
    DumpMountPathMountNode(&pathNode);

    EXPECT_EQ(0, GetPathMode("test"));
    EXPECT_EQ(0, GetPathMode(nullptr));

    EXPECT_EQ(nullptr, GetSandboxFlagInfo(nullptr, nullptr, 0));
    EXPECT_EQ(nullptr, GetSandboxFlagInfo("key", nullptr, 0));

    EXPECT_EQ(nullptr, GetMountArgTemplate(123456));
}
}  // namespace OHOS
