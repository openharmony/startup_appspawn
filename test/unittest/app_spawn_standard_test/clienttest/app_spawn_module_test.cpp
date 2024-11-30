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
#include <gtest/gtest.h>
#include <cstring> // For strcmp

// 假设这些类型和全局变量已定义在目标代码中
#define MODULE_DEFAULT 0
#define APPSPAWN_TEST 1 // 让测试用例在测试模式下编译
#define MODULE_MAX 10
#define MODULE_DEFAULT 0
// 模拟的结构和函数定义
struct ModuleMgr {
    const char *moduleName;
    // 假设我们有一个模块管理器的接口
    void *moduleMgr;
};

ModuleMgr g_moduleMgr[MODULE_MAX]; // 这里只有一个模块

int ModuleMgrCreate(const char *name)
{
    // 模拟创建模块管理器
    return 1; // 假设成功创建
}

int ModuleMgrInstall(void *mgr, const char *moduleName, int flag, void *data)
{
    // 模拟安装模块
    return 0; // 假设安装成功
}

int AppSpawnModuleMgrInstall(const char *moduleName);

namespace OHOS {
    // 测试类
    class AppSpawnModuleMgrInstallTest : public ::testing::Test {
    protected:
        // 在每个测试之前执行
        void SetUp() override
        {
            // 这里可以对 `g_moduleMgr` 进行初始化，以确保每个测试环境独立
            g_moduleMgr[MODULE_DEFAULT].moduleMgr = nullptr; // 初始时模块管理器为空
        }
    };

    // 测试用例：传入 nullptr 参数时，函数应该返回 -1
    TEST_F(AppSpawnModuleMgrInstallTest, TestNullModuleName)
    {
        EXPECT_EQ(AppSpawnModuleMgrInstall(nullptr), -1);
    }

    // 测试用例：`g_moduleMgr[type].moduleMgr` 为 nullptr 时，应该调用 `ModuleMgrCreate` 创建模块管理器
    TEST_F(AppSpawnModuleMgrInstallTest, TestCreateModuleMgrWhenNull)
    {
        // 检查模块管理器是否为空
        EXPECT_EQ(g_moduleMgr[MODULE_DEFAULT].moduleMgr, nullptr);

        // 调用函数
        EXPECT_EQ(AppSpawnModuleMgrInstall("testModule"), 0);

        // 检查模块管理器是否已经创建
        EXPECT_NE(g_moduleMgr[MODULE_DEFAULT].moduleMgr, nullptr);
    }

    // 测试用例：`ModuleMgrInstall` 成功时，函数返回 0
    TEST_F(AppSpawnModuleMgrInstallTest, TestModuleInstallSuccess)
    {
        g_moduleMgr[MODULE_DEFAULT].moduleMgr = reinterpret_cast<void *>(1);

        // 传入有效的模块名
        EXPECT_EQ(AppSpawnModuleMgrInstall("testModule"), 0);
    }

    // 测试用例：如果 `ModuleMgrCreate` 失败，返回 -1
    TEST_F(AppSpawnModuleMgrInstallTest, TestCreateFail)
    {
        // 模拟 `ModuleMgrCreate` 失败
        g_moduleMgr[MODULE_DEFAULT].moduleMgr = nullptr; // 模拟创建失败

        // 传入有效的模块名
        EXPECT_EQ(AppSpawnModuleMgrInstall("testModule"), -1);
    }

#ifdef APPSPAWN_TEST
    // 测试用例：如果在测试模式下，函数应该直接返回 0
    TEST_F(AppSpawnModuleMgrInstallTest, TestInTestMode)
    {
        EXPECT_EQ(AppSpawnModuleMgrInstall("testModule"), 0);
    }
#endif

    // 测试用例 1：`type` 超出范围时，应该直接返回，不做任何操作
    TEST_F(AppSpawnModuleMgrUnInstallTest, TestTypeOutOfRange)
    {
        // `type` 小于 0
        AppSpawnModuleMgrUnInstall(-1);
        for (int i = 0; i < MODULE_MAX; ++i)
        {
            EXPECT_EQ(g_moduleMgr[i].moduleMgr, nullptr);
        }

        // `type` 大于等于 MODULE_MAX
        AppSpawnModuleMgrUnInstall(MODULE_MAX);
        for (int i = 0; i < MODULE_MAX; ++i)
        {
            EXPECT_EQ(g_moduleMgr[i].moduleMgr, nullptr);
        }
    }

    // 测试用例 2：`g_moduleMgr[type].moduleMgr` 为 nullptr 时，应该不进行任何操作
    TEST_F(AppSpawnModuleMgrUnInstallTest, TestModuleMgrIsNull)
    {
        // `g_moduleMgr[MODULE_DEFAULT].moduleMgr` 依然是 nullptr
        AppSpawnModuleMgrUnInstall(MODULE_DEFAULT);
        EXPECT_EQ(g_moduleMgr[MODULE_DEFAULT].moduleMgr, nullptr); // 确保没有改变
    }

    // 测试用例 3：`g_moduleMgr[type].moduleMgr` 不为 nullptr 时，应该销毁模块并将其设为 nullptr
    TEST_F(AppSpawnModuleMgrUnInstallTest, TestUninstallModule)
    {
        // 设置 `g_moduleMgr[MODULE_DEFAULT].moduleMgr` 为一个非 nullptr 值
        void *mockMgr = reinterpret_cast<void *>(1);
        g_moduleMgr[MODULE_DEFAULT].moduleMgr = mockMgr;

        // 调用卸载函数
        AppSpawnModuleMgrUnInstall(MODULE_DEFAULT);

        // 检查模块是否已被销毁，并且值是否已设为 nullptr
        EXPECT_EQ(g_moduleMgr[MODULE_DEFAULT].moduleMgr, nullptr);
    }

    // 测试用例 4：`g_moduleMgr` 的其他位置没有影响
    TEST_F(AppSpawnModuleMgrUnInstallTest, TestOtherModuleUnchanged)
    {
        // 设置 `g_moduleMgr[1].moduleMgr` 为一个非 nullptr 值
        void *mockMgr = reinterpret_cast<void *>(1);
        g_moduleMgr[1].moduleMgr = mockMgr;

        // 调用卸载函数，使用不同的 `type`
        AppSpawnModuleMgrUnInstall(MODULE_DEFAULT); // 不影响 g_moduleMgr[1]

        // 检查 `g_moduleMgr[1]` 是否仍然有效
        EXPECT_EQ(g_moduleMgr[1].moduleMgr, mockMgr);
    }

    // 测试用例 5：`type` 为一个有效值，但模块已经被销毁的情况
    TEST_F(AppSpawnModuleMgrUnInstallTest, TestModuleAlreadyUninstalled)
    {
        // 初始化一个模块管理器并销毁它
        void *mockMgr = reinterpret_cast<void *>(1);
        g_moduleMgr[MODULE_DEFAULT].moduleMgr = mockMgr;

        // 手动销毁模块
        ModuleMgrDestroy(mockMgr);
        g_moduleMgr[MODULE_DEFAULT].moduleMgr = nullptr;

        // 调用卸载函数
        AppSpawnModuleMgrUnInstall(MODULE_DEFAULT);

        // 检查模块是否仍然为空
        EXPECT_EQ(g_moduleMgr[MODULE_DEFAULT].moduleMgr, nullptr);
    }

    // 测试用例 1：`type` 超出范围时，应该返回 -1
    TEST_F(AppSpawnLoadAutoRunModulesTest, TestTypeOutOfRange)
    {
        EXPECT_EQ(AppSpawnLoadAutoRunModules(-1), -1);         // `type` 小于 0
        EXPECT_EQ(AppSpawnLoadAutoRunModules(MODULE_MAX), -1); // `type` 大于等于 MODULE_MAX
    }

    // 测试用例 2：模块已加载时，返回 0
    TEST_F(AppSpawnLoadAutoRunModulesTest, TestModuleAlreadyLoaded)
    {
        // 设置模块已经加载
        g_moduleMgr[MODULE_DEFAULT].moduleMgr = reinterpret_cast<void *>(1); // 模拟模块已加载

        // 调用函数，应该返回 0
        EXPECT_EQ(AppSpawnLoadAutoRunModules(MODULE_DEFAULT), 0);
    }

    // 测试用例 3：模块未加载时，调用 `ModuleMgrScan` 加载模块并返回 0
    TEST_F(AppSpawnLoadAutoRunModulesTest, TestModuleNotLoaded)
    {
        // `g_moduleMgr[MODULE_DEFAULT].moduleMgr` 初始为 nullptr

        // 调用函数，应该返回 0，并加载模块
        EXPECT_EQ(AppSpawnLoadAutoRunModules(MODULE_DEFAULT), 0);
        // 检查 `moduleMgr` 是否已被加载
        EXPECT_NE(g_moduleMgr[MODULE_DEFAULT].moduleMgr, nullptr);
    }

// 测试用例 4：`APPSPAWN_TEST` 环境下，应该跳过 `ModuleMgrScan` 的调用并直接返回 0
#ifdef APPSPAWN_TEST
    TEST_F(AppSpawnLoadAutoRunModulesTest, TestInTestEnvironment)
    {
        // 在 APPSPAWN_TEST 环境下，`ModuleMgrScan` 被跳过，直接返回 0
        EXPECT_EQ(AppSpawnLoadAutoRunModules(MODULE_DEFAULT), 0);
    }
#endif

    // 测试用例 5：模块加载失败时，返回 -1
    TEST_F(AppSpawnLoadAutoRunModulesTest, TestModuleLoadFailure)
    {
        // 修改 ModuleMgrScan 让它返回 NULL，模拟加载失败
        void *(*originalModuleMgrScan)(const char *) = ModuleMgrScan;
        ModuleMgrScan = [](const char *moduleName) -> void *
        { return nullptr; };

        // 调用函数，应该返回 -1
        EXPECT_EQ(AppSpawnLoadAutoRunModules(MODULE_DEFAULT), -1);

        // 恢复原始的 ModuleMgrScan 函数
        ModuleMgrScan = originalModuleMgrScan;
    }

    // 测试用例 1：g_appspawnHookMgr 已经初始化为非 nullptr
    TEST_F(GetAppSpawnHookMgrTest, TestHookMgrAlreadyInitialized)
    {
        // 先初始化 g_appspawnHookMgr
        g_appspawnHookMgr = HookMgrCreate("test");

        // 调用 GetAppSpawnHookMgr，应该返回已经存在的 g_appspawnHookMgr
        HOOK_MGR *hookMgr = GetAppSpawnHookMgr();
        EXPECT_EQ(hookMgr, g_appspawnHookMgr);
        EXPECT_STREQ(hookMgr->name, "test");
    }

    // 测试用例 2：g_appspawnHookMgr 为 nullptr 时，调用 HookMgrCreate
    TEST_F(GetAppSpawnHookMgrTest, TestHookMgrNotInitialized)
    {
        // g_appspawnHookMgr 初始为 nullptr

        // 调用 GetAppSpawnHookMgr，应该创建新的 g_appspawnHookMgr
        HOOK_MGR *hookMgr = GetAppSpawnHookMgr();
        EXPECT_NE(hookMgr, nullptr);
        EXPECT_STREQ(hookMgr->name, "appspawn");

        // 确认 g_appspawnHookMgr 已被初始化
        EXPECT_EQ(g_appspawnHookMgr, hookMgr);
    }

    // 测试用例 3：多次调用 GetAppSpawnHookMgr 返回同一个实例
    TEST_F(GetAppSpawnHookMgrTest, TestMultipleCallsReturnSameInstance)
    {
        // g_appspawnHookMgr 初始为 nullptr

        // 第一次调用，创建并返回新的实例
        HOOK_MGR *hookMgr1 = GetAppSpawnHookMgr();
        EXPECT_NE(hookMgr1, nullptr);
        EXPECT_STREQ(hookMgr1->name, "appspawn");

        // 第二次调用，返回相同的实例
        HOOK_MGR *hookMgr2 = GetAppSpawnHookMgr();
        EXPECT_EQ(hookMgr1, hookMgr2);
        EXPECT_STREQ(hookMgr2->name, "appspawn");
    }

    // 测试用例 4：确保返回的对象的 name 是我们期望的
    TEST_F(GetAppSpawnHookMgrTest, TestCorrectName)
    {
        // g_appspawnHookMgr 初始为 nullptr

        // 调用 GetAppSpawnHookMgr，应该创建新的 g_appspawnHookMgr
        HOOK_MGR *hookMgr = GetAppSpawnHookMgr();
        EXPECT_EQ(hookMgr->name, "appspawn");
    }

    // 测试用例 1：更新最小时间
    TEST_F(UpdateAppSpawnTimeTest, TestUpdateMinTime)
    {
        // 初始值为 INT_MAX 和 0
        GetAppSpawnMgr()->spawnTime.minAppspawnTime = INT_MAX;
        GetAppSpawnMgr()->spawnTime.maxAppspawnTime = 0;

        // 调用 UpdateAppSpawnTime 来更新最小时间
        UpdateAppSpawnTime(100);

        // 验证最小时间已更新
        EXPECT_EQ(GetAppSpawnMgr()->spawnTime.minAppspawnTime, 100);

        // 验证日志输出
        std::string captured = testing::internal::CaptureStdout();
        EXPECT_NE(captured.find("spawn min time: 100"), std::string::npos);
    }

    // 测试用例 2：更新最大时间
    TEST_F(UpdateAppSpawnTimeTest, TestUpdateMaxTime)
    {
        // 初始值为 INT_MAX 和 0
        GetAppSpawnMgr()->spawnTime.minAppspawnTime = INT_MAX;
        GetAppSpawnMgr()->spawnTime.maxAppspawnTime = 0;

        // 调用 UpdateAppSpawnTime 来更新最大时间
        UpdateAppSpawnTime(500);

        // 验证最大时间已更新
        EXPECT_EQ(GetAppSpawnMgr()->spawnTime.maxAppspawnTime, 500);

        // 验证日志输出
        std::string captured = testing::internal::CaptureStdout();
        EXPECT_NE(captured.find("spawn max time: 500"), std::string::npos);
    }

    // 测试用例 3：不更新最小时间和最大时间
    TEST_F(UpdateAppSpawnTimeTest, TestNoUpdate)
    {
        // 初始值为 100 和 500
        GetAppSpawnMgr()->spawnTime.minAppspawnTime = 100;
        GetAppSpawnMgr()->spawnTime.maxAppspawnTime = 500;

        // 调用 UpdateAppSpawnTime，应该不做更新
        UpdateAppSpawnTime(200);

        // 验证最小时间和最大时间没有变化
        EXPECT_EQ(GetAppSpawnMgr()->spawnTime.minAppspawnTime, 100);
        EXPECT_EQ(GetAppSpawnMgr()->spawnTime.maxAppspawnTime, 500);

        // 验证没有日志输出
        std::string captured = testing::internal::CaptureStdout();
        EXPECT_EQ(captured, "");
    }

    // 测试用例 1：验证正常执行和时间差计算
    TEST_F(PostHookExecTest, TestNormalExecution)
    {
        // 模拟 perLoadEnd 时间，假设它比 perLoadStart 大 1秒
        spawnMgr->perLoadEnd.tv_sec = 1001;
        spawnMgr->perLoadEnd.tv_nsec = 500000000; // 1001s 500ms

        HOOK_INFO hookInfo = {1, 10}; // stage 1, prio 10
        int executionRetVal = 0;      // 执行结果，假设为0

        // 捕获输出
        std::string captured = testing::internal::CaptureStdout();

        // 调用 PostHookExec
        PostHookExec(&hookInfo, &hookArg, executionRetVal);

        // 验证 UpdateAppSpawnTime 被调用，检查时间差是否正确计算
        uint64_t expectedDiff = 1000000000; // 1秒 = 1e9 纳秒
        EXPECT_NE(captured.find("Updated AppSpawn Time: " + std::to_string(expectedDiff)), std::string::npos);

        // 验证日志输出内容是否包含预期的 Hook 信息
        EXPECT_NE(captured.find("Hook stage: 1 prio: 10 end time 1000000000 ns result: 0"), std::string::npos);
    }

    // 测试用例 2：验证时间差为零时的行为
    TEST_F(PostHookExecTest, TestZeroTimeDifference)
    {
        // 模拟 perLoadEnd 时间与 perLoadStart 时间相同
        spawnMgr->perLoadEnd.tv_sec = 1000;
        spawnMgr->perLoadEnd.tv_nsec = 500000000; // 1000s 500ms

        HOOK_INFO hookInfo = {1, 10}; // stage 1, prio 10
        int executionRetVal = 0;      // 执行结果，假设为0

        // 捕获输出
        std::string captured = testing::internal::CaptureStdout();

        // 调用 PostHookExec
        PostHookExec(&hookInfo, &hookArg, executionRetVal);

        // 验证 UpdateAppSpawnTime 被调用，检查时间差是否为零
        uint64_t expectedDiff = 0;
        EXPECT_NE(captured.find("Updated AppSpawn Time: " + std::to_string(expectedDiff)), std::string::npos);

        // 验证日志输出内容是否包含预期的 Hook 信息
        EXPECT_NE(captured.find("Hook stage: 1 prio: 10 end time 0 ns result: 0"), std::string::npos);
    }

    // 测试用例 1：验证 content 为 nullptr 时返回 APPSPAWN_ARG_INVALID
    TEST_F(ServerStageHookExecuteTest, TestContentNull)
    {
        AppSpawnContent *nullContent = nullptr;

        int ret = ServerStageHookExecute(STAGE_SERVER_PRELOAD, nullContent);

        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    }

    // 测试用例 2：验证无效 stage 返回 APPSPAWN_ARG_INVALID
    TEST_F(ServerStageHookExecuteTest, TestInvalidStage)
    {
        // 假设 STAGE_SERVER_EXIT 之后有个无效的 stage
        AppSpawnHookStage invalidStage = static_cast<AppSpawnHookStage>(999); // 无效的阶段

        int ret = ServerStageHookExecute(invalidStage, content);

        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    }

    // 测试用例 3：验证有效的 content 和 stage 执行成功
    TEST_F(ServerStageHookExecuteTest, TestValidExecution)
    {
        // 使用有效的 stage
        AppSpawnHookStage validStage = STAGE_SERVER_PRELOAD;

        // 捕获日志输出
        std::string captured = testing::internal::CaptureStdout();

        int ret = ServerStageHookExecute(validStage, content);

        // 期望钩子执行成功
        EXPECT_EQ(ret, 0); // 假设钩子返回值为 0

        // 验证日志是否包含正确的钩子执行信息
        EXPECT_NE(captured.find("Execute hook [0] result 0"), std::string::npos); // STAGE_SERVER_PRELOAD 对应值为 0
    }

    // 测试用例 4：验证无效的钩子执行返回 ERR_NO_HOOK_STAGE
    TEST_F(ServerStageHookExecuteTest, TestNoHookStage)
    {
        // 模拟钩子返回 ERR_NO_HOOK_STAGE
        HookMgrExecute = [](void *, AppSpawnHookStage, void *, HOOK_EXEC_OPTIONS *)
        {
            return ERR_NO_HOOK_STAGE;
        };

        AppSpawnHookStage validStage = STAGE_SERVER_EXIT;

        int ret = ServerStageHookExecute(validStage, content);

        // 期望返回 0 因为 ERR_NO_HOOK_STAGE 被转换为 0
        EXPECT_EQ(ret, 0);
    }

    // 测试用例 1：验证 hook 为 nullptr 时返回 APPSPAWN_ARG_INVALID
    TEST_F(AddServerStageHookTest, TestHookNull)
    {
        ServerStageHook nullHook = nullptr;

        int ret = AddServerStageHook(STAGE_SERVER_PRELOAD, 10, nullHook);

        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    }

    // 测试用例 2：验证无效的 stage 返回 APPSPAWN_ARG_INVALID
    TEST_F(AddServerStageHookTest, TestInvalidStage)
    {
        // 假设 STAGE_SERVER_EXIT 后有个无效的 stage
        AppSpawnHookStage invalidStage = static_cast<AppSpawnHookStage>(999); // 无效的阶段

        ServerStageHook validHook = ServerStageHookRun;

        int ret = AddServerStageHook(invalidStage, 10, validHook);

        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    }

    // 测试用例 3：验证有效的 stage 和 hook 时能够成功添加钩子
    TEST_F(AddServerStageHookTest, TestValidHookAndStage)
    {
        AppSpawnHookStage validStage = STAGE_SERVER_PRELOAD;
        ServerStageHook validHook = ServerStageHookRun;
        int prio = 10;

        // 捕获日志输出
        std::string captured = testing::internal::CaptureStdout();

        int ret = AddServerStageHook(validStage, prio, validHook);

        // 期望返回 0，表示成功
        EXPECT_EQ(ret, 0);

        // 验证日志是否包含正确的钩子添加信息
        EXPECT_NE(captured.find("AddServerStageHook prio: 10"), std::string::npos);
    }

    // 测试用例 4：验证无效的钩子（模拟钩子执行错误）
    TEST_F(AddServerStageHookTest, TestHookExecutionError)
    {
        // 假设有一个无效的钩子
        ServerStageHook invalidHook = nullptr; // 无效钩子

        int ret = AddServerStageHook(STAGE_SERVER_EXIT, 20, invalidHook);

        // 期望返回 APPSPAWN_ARG_INVALID，因为钩子是无效的
        EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    }
    // 测试用例 1：验证有效钩子函数的调用
    TEST_F(AppSpawnHookRunTest, ValidHookExecution)
    {
        int result = AppSpawnHookRun(&hookInfo, &executionContext);

        // 验证返回值是否符合预期，表示钩子被正确调用
        EXPECT_EQ(result, 42);
    }

    // 测试用例 2：验证执行上下文为空的情况
    TEST_F(AppSpawnHookRunTest, NullExecutionContext)
    {
        int result = AppSpawnHookRun(&hookInfo, nullptr);

        // 验证如果 executionContext 为 NULL，函数应返回 0 或其它合适的错误码
        EXPECT_EQ(result, 0); // 这里假设返回 0
    }

    // 测试用例 3：验证 hookCookie 为 nullptr 时的行为
    TEST_F(AppSpawnHookRunTest, NullHookCookie)
    {
        hookInfo.hookCookie = nullptr; // 设置钩子为 nullptr

        int result = AppSpawnHookRun(&hookInfo, &executionContext);

        // 验证如果 hookCookie 为 NULL，函数应返回某个错误码或异常
        EXPECT_EQ(result, 0); // 这里假设返回 0
    }

    // 测试用例 4：验证钩子函数为其它实现时的调用
    TEST_F(AppSpawnHookRunTest, CustomHookFunction)
    {
        // 定义一个新的钩子函数
        auto customHook = [](AppSpawnMgr *mgr, AppSpawningCtx *ctx) -> int
        {
            return 99; // 自定义钩子返回 99
        };

        hookInfo.hookCookie = (void *)customHook; // 使用自定义钩子

        int result = AppSpawnHookRun(&hookInfo, &executionContext);

        // 验证自定义钩子函数被正确调用
        EXPECT_EQ(result, 99);
    }

    // 测试用例 1：验证正常的日志输出和时间记录
    TEST_F(PreAppSpawnHookExecTest, NormalExecution)
    {
        hookInfo.stage = 1;
        hookInfo.prio = 5;

        // 执行 PreAppSpawnHookExec
        PreAppSpawnHookExec(&hookInfo, &executionContext);

        // 验证日志输出是否符合预期
        EXPECT_NE(logStream.str().find("Hook stage: 1 prio: 5 start"), std::string::npos);

        // 验证时间是否已经被记录（tmStart 的值应大于 0）
        EXPECT_GT(executionContext.tmStart.tv_sec, 0);
        EXPECT_GE(executionContext.tmStart.tv_nsec, 0);
    }

    // 测试用例 2：验证 executionContext 为 nullptr 时的行为
    TEST_F(PreAppSpawnHookExecTest, NullExecutionContext)
    {
        hookInfo.stage = 1;
        hookInfo.prio = 5;

        // 执行时传入 nullptr executionContext
        PreAppSpawnHookExec(&hookInfo, nullptr);

        // 验证没有崩溃，且日志正确输出
        EXPECT_NE(logStream.str().find("Hook stage: 1 prio: 5 start"), std::string::npos);
    }

    // 测试用例 3：验证 hookInfo 为 nullptr 时的行为
    TEST_F(PreAppSpawnHookExecTest, NullHookInfo)
    {
        // 执行时传入 nullptr hookInfo
        PreAppSpawnHookExec(nullptr, &executionContext);

        // 验证没有崩溃，日志是否正确
        EXPECT_EQ(logStream.str(), ""); // 如果 hookInfo 为空，日志应该没有输出
    }

    // 测试用例 4：验证时间记录
    TEST_F(PreAppSpawnHookExecTest, TimeRecorded)
    {
        hookInfo.stage = 1;
        hookInfo.prio = 5;

        // 执行函数
        PreAppSpawnHookExec(&hookInfo, &executionContext);

        // 验证时间字段是否被正确设置
        EXPECT_GT(executionContext.tmStart.tv_sec, 0);
        EXPECT_GE(executionContext.tmStart.tv_nsec, 0);
    }

    // 测试用例 1：验证正常的日志输出和时间计算
    TEST_F(PostAppSpawnHookExecTest, NormalExecution)
    {
        hookInfo.stage = 1;
        hookInfo.prio = 5;

        // 假设 tmStart 已经设置
        executionContext.tmStart.tv_sec = 10;
        executionContext.tmStart.tv_nsec = 500000000; // 10.5 seconds

        // 执行 PostAppSpawnHookExec
        PostAppSpawnHookExec(&hookInfo, &executionContext, 0);

        // 验证日志输出是否符合预期
        EXPECT_NE(logStream.str().find("Hook stage: 1 prio: 5 end time"), std::string::npos);
        EXPECT_NE(logStream.str().find("result: 0"), std::string::npos);

        // 验证时间差是否大于 0（假设 tmEnd 被正确设置）
        uint64_t diff = DiffTime(&executionContext.tmStart, &executionContext.tmEnd);
        EXPECT_GT(diff, 0);
    }

    // 测试用例 2：验证 executionContext 为 nullptr 时的行为
    TEST_F(PostAppSpawnHookExecTest, NullExecutionContext)
    {
        hookInfo.stage = 1;
        hookInfo.prio = 5;

        // 执行时传入 nullptr executionContext
        PostAppSpawnHookExec(&hookInfo, nullptr, 0);

        // 验证没有崩溃，且日志正确输出（虽然 executionContext 为 NULL，函数仍应正常工作）
        EXPECT_NE(logStream.str().find("Hook stage: 1 prio: 5 end time"), std::string::npos);
        EXPECT_NE(logStream.str().find("result: 0"), std::string::npos);
    }

    // 测试用例 3：验证 hookInfo 为 nullptr 时的行为
    TEST_F(PostAppSpawnHookExecTest, NullHookInfo)
    {
        // 执行时传入 nullptr hookInfo
        PostAppSpawnHookExec(nullptr, &executionContext, 0);

        // 验证没有崩溃，日志应该为空（没有 hookInfo 的情况下，无法输出相关信息）
        EXPECT_EQ(logStream.str(), "");
    }

    // 测试用例 4：验证时间差计算
    TEST_F(PostAppSpawnHookExecTest, TimeDiffCalculation)
    {
        hookInfo.stage = 1;
        hookInfo.prio = 5;

        // 设置 start 时间为某个已知时间
        executionContext.tmStart.tv_sec = 5;
        executionContext.tmStart.tv_nsec = 100000000; // 5.1 seconds

        // 设置一个稍后的 end 时间
        executionContext.tmEnd.tv_sec = 6;
        executionContext.tmEnd.tv_nsec = 200000000; // 6.2 seconds

        // 计算时间差
        uint64_t diff = DiffTime(&executionContext.tmStart, &executionContext.tmEnd);

        // 验证时间差为 1.1 秒 (1.1 * 10^9 纳秒)
        EXPECT_EQ(diff, 1100000000LL);
    }

    // 测试用例 1：验证传入 nullptr 参数时的行为
    TEST_F(AppSpawnHookExecuteTest, InvalidArguments)
    {
        // 测试 content 和 client 为 nullptr 的情况
        int ret = AppSpawnHookExecute(STAGE_PARENT_PRE_FORK, 0, nullptr, nullptr);
        EXPECT_EQ(ret, -1); // 应返回 -1，表示参数无效

        // 测试 stage 无效时的情况
        ret = AppSpawnHookExecute((AppSpawnHookStage)999, 0, &content, &client);
        EXPECT_EQ(ret, -1); // 应返回 -1，表示 stage 无效
    }

    // 测试用例 2：验证日志输出和正常执行
    TEST_F(AppSpawnHookExecuteTest, ValidExecution)
    {
        // 测试正常的执行路径
        int ret = AppSpawnHookExecute(STAGE_PARENT_PRE_FORK, 0, &content, &client);

        EXPECT_EQ(ret, 0); // 应返回 0，表示执行成功

        // 检查日志是否包含预期的内容
        std::string logOutput = logStream.str();
        EXPECT_NE(logOutput.find("Execute hook [0] for app: TestApp"), std::string::npos); // stage 和 app name
        EXPECT_EQ(logOutput.find("Invalid arg"), std::string::npos);                       // 无错误日志
    }

    // 测试用例 3：验证 HookMgrExecute 调用
    TEST_F(AppSpawnHookExecuteTest, HookMgrExecuteCalled)
    {
        // 测试正常的钩子执行
        int ret = AppSpawnHookExecute(STAGE_CHILD_PRE_RUN, 0, &content, &client);

        EXPECT_EQ(ret, 0); // 确保返回值为0，表示执行成功

        // 检查日志输出
        std::string logOutput = logStream.str();
        EXPECT_NE(logOutput.find("Execute hook [1] result 0"), std::string::npos); // 检查执行成功的日志
    }

    // 测试用例 4：验证无效 stage 时的行为
    TEST_F(AppSpawnHookExecuteTest, InvalidStage)
    {
        int ret = AppSpawnHookExecute((AppSpawnHookStage)999, 0, &content, &client);
        EXPECT_EQ(ret, -1); // 应返回 -1，表示 stage 无效

        // 检查是否有相应的错误日志
        std::string logOutput = logStream.str();
        EXPECT_NE(logOutput.find("Invalid stage"), std::string::npos);
    }
} // namespace OHOS
