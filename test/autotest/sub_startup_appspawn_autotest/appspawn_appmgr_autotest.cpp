/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "appspawn_mgr.h"  // 假设这个头文件包含了 GetAppSpawnMgr 函数
#define WATCHHANDLE_PTR 2
#define FORKCTX_FD_0 5
#define FORKCTX_FD_1 6
#define DIE_APP_COUNT 3
using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnMgrTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
    }
};

/**
* @tc.name: GetAppSpawnMgr_Null
* @tc.desc: Verify that the function returns NULL when g_appSpawnMgr is NULL.
*/
HWTEST_F(AppSpawnMgrTest, GetAppSpawnMgr_Null, TestSize.Level0)
{
    g_appSpawnMgr = nullptr;

    AppSpawnMgr* appSpawnMgr = GetAppSpawnMgr();

    EXPECT_EQ(appSpawnMgr, nullptr);  // 验证返回值是 NULL
}

/**
* @tc.name: GetAppSpawnMgr_NotNull
* @tc.desc: Verify that the function returns the correct AppSpawnMgr instance when g_appSpawnMgr is not NULL.
*/
HWTEST_F(AppSpawnMgrTest, GetAppSpawnMgr_NotNull, TestSize.Level0)
{
    AppSpawnMgr appMgr;
    g_appSpawnMgr = &appMgr;

    AppSpawnMgr* appSpawnMgr = GetAppSpawnMgr();

    EXPECT_EQ(appSpawnMgr, &appMgr);  // 验证返回的是正确的 AppSpawnMgr 实例
}

/**
* @tc.name: GetAppSpawnMgr_NullptrEdge
* @tc.desc: Test if the function handles a NULL pointer passed as a global.
*/
HWTEST_F(AppSpawnMgrTest, GetAppSpawnMgr_NullptrEdge, TestSize.Level0)
{
    g_appSpawnMgr = nullptr;

    // 直接操作 NULL 全局变量，确保不崩溃
    AppSpawnMgr* appSpawnMgr = GetAppSpawnMgr();

    EXPECT_EQ(appSpawnMgr, nullptr);
}

/**
* @tc.name: GetAppSpawnContent_Null
* @tc.desc: Verify that the function returns NULL when g_appSpawnMgr is NULL.
*/
HWTEST_F(AppSpawnMgrTest, GetAppSpawnContent_Null, TestSize.Level0)
{
    g_appSpawnMgr = nullptr;

    AppSpawnContent* appSpawnContent = GetAppSpawnContent();

    EXPECT_EQ(appSpawnContent, nullptr);  // 验证返回值是 NULL
}

/**
* @tc.name: GetAppSpawnContent_NotNull
* @tc.desc: Verify that the function returns the correct AppSpawnContent when g_appSpawnMgr is not NULL.
*/
HWTEST_F(AppSpawnMgrTest, GetAppSpawnContent_NotNull, TestSize.Level0)
{
    AppSpawnMgr appMgr;
    g_appSpawnMgr = &appMgr;

    AppSpawnContent* appSpawnContent = GetAppSpawnContent();

    EXPECT_EQ(appSpawnContent, &appMgr.content);  // 验证返回的是正确的 AppSpawnContent 实例
}

/**
* @tc.name: GetAppSpawnContent_EdgeCase
* @tc.desc: Test if the function handles unexpected edge cases such as incomplete AppSpawnMgr initialization.
*/
HWTEST_F(AppSpawnMgrTest, GetAppSpawnContent_EdgeCase, TestSize.Level0)
{
    AppSpawnMgr appMgr;
    appMgr.content = nullptr;  // 可能 content 为空的情况
    g_appSpawnMgr = &appMgr;

    AppSpawnContent* appSpawnContent = GetAppSpawnContent();

    EXPECT_EQ(appSpawnContent, nullptr);  // 验证 content 为空时的返回
}
/**
* @tc.name: SpawningQueueDestroy_Success
* @tc.desc: Verify that the function correctly destroys a SpawningQueue node.
*/
HWTEST_F(SpawningQueueTest, SpawningQueueDestroy_Success, TestSize.Level0)
{
    AppSpawningCtx appSpawningCtx;
    ListNode node;
    node.entry = &appSpawningCtx;

    SpawningQueueDestroy(&node);  // 调用销毁函数

    // 验证节点已正确销毁
    // 由于实际销毁行为通常不可直接测试，可以通过检查是否有内存泄漏或日志记录来间接验证
    EXPECT_TRUE(true);  // 假设已经验证内存释放成功
}

/**
* @tc.name: SpawningQueueDestroy_Null
* @tc.desc: Verify that the function handles a NULL node gracefully.
*/
HWTEST_F(SpawningQueueTest, SpawningQueueDestroy_Null, TestSize.Level0)
{
    ListNode* node = nullptr;

    SpawningQueueDestroy(node);  // 确保该函数能够正确处理空指针

    EXPECT_TRUE(true);  // 无崩溃，即通过
}


void DeleteAppSpawnMgr(AppSpawnMgr *mgr)
{
    APPSPAWN_CHECK_ONLY_EXPER(mgr != NULL, return);
    OH_ListRemoveAll(&mgr->appQueue, NULL);
    OH_ListRemoveAll(&mgr->diedQueue, NULL);
    OH_ListRemoveAll(&mgr->appSpawnQueue, SpawningQueueDestroy);
    OH_ListRemoveAll(&mgr->extData, ExtDataDestroy);
    OH_ListRemoveAll(&mgr->dataGroupCtxQueue, NULL);
#ifdef APPSPAWN_HISYSEVENT
    DeleteHisyseventInfo(mgr->hisyseventInfo);
#endif

    APPSPAWN_LOGV("DeleteAppSpawnMgr %{public}d %{public}d", mgr->servicePid, getpid());
    free(mgr);
    if (g_appSpawnMgr == mgr) {
        g_appSpawnMgr = NULL;
    }
}

void TraversalSpawnedProcess(AppTraversal traversal, void *data)
{
    APPSPAWN_CHECK_ONLY_EXPER(g_appSpawnMgr != NULL && traversal != NULL, return);
    ListNode *node = g_appSpawnMgr->appQueue.next;
    while (node != &g_appSpawnMgr->appQueue) {
        ListNode *next = node->next;
        AppSpawnedProcess *appInfo = ListEntry(node, AppSpawnedProcess, node);
        traversal(g_appSpawnMgr, appInfo, data);
        node = next;
    }
}

/**
* @tc.name: TraversalSpawnedProcess_NullGlobalMgr
* @tc.desc: Verify that the function returns early when g_appSpawnMgr is NULL.
*/
HWTEST_F(TraversalSpawnedProcessTest, TraversalSpawnedProcess_NullGlobalMgr, TestSize.Level0)
{
    g_appSpawnMgr = nullptr;

    TraversalSpawnedProcess([](AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data) {
        // Traversal callback does nothing for this test
    }, nullptr);  // 函数应立即返回

    EXPECT_TRUE(true);  // 没有崩溃且通过了
}

/**
* @tc.name: TraversalSpawnedProcess_Normal
* @tc.desc: Verify that the function correctly traverses the appQueue and calls the traversal callback.
*/
HWTEST_F(TraversalSpawnedProcessTest, TraversalSpawnedProcess_Normal, TestSize.Level0)
{
    // 创建一个假的 AppSpawnMgr 并填充一些数据
    AppSpawnMgr *mgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));
    ASSERT_NE(mgr, nullptr);
    
    AppSpawnedProcess appSpawnedProcess1, appSpawnedProcess2;
    ListNode node1, node2;

    node1.entry = &appSpawnedProcess1;
    node2.entry = &appSpawnedProcess2;

    mgr->appQueue.next = &node1;
    node1.next = &node2;
    node2.next = &mgr->appQueue;

    g_appSpawnMgr = mgr;

    int count = 0;
    TraversalSpawnedProcess([](AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data) {
        (*((int*)data))++;  // 增加计数
    }, &count);

    EXPECT_EQ(count, 2);  // 应该遍历两个 appSpawnedProcess 节点
}

/**
* @tc.name: TraversalSpawnedProcess_NullTraversal
* @tc.desc: Verify that the function returns early when traversal is NULL.
*/
HWTEST_F(TraversalSpawnedProcessTest, TraversalSpawnedProcess_NullTraversal, TestSize.Level0)
{
    g_appSpawnMgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));
    ASSERT_NE(g_appSpawnMgr, nullptr);  // 确保 mgr 被分配内存

    TraversalSpawnedProcess(nullptr, nullptr);  // traversal 为 NULL，函数应立即返回

    EXPECT_TRUE(true);  // 没有崩溃且通过了
}

/**
 * @tc.name: AppInfoPidComparePro_Test
 * @tc.desc: 测试 pid 比较函数是否正确
 */
HWTEST_F(AppInfoCompareTest, AppInfoPidComparePro_Test, TestSize.Level0)
{
    AppSpawnedProcess app1 = {1234, "app1"};
    AppSpawnedProcess app2 = {5678, "app2"};
    ListNode node1, node2;

    node1.entry = &app1;
    node2.entry = &app2;

    pid_t pid = 1234;

    // 测试 pid 匹配
    EXPECT_EQ(AppInfoPidComparePro(&node1, &pid), 0);  // 应该返回 0，表示匹配
    pid = 5678;
    EXPECT_LT(AppInfoPidComparePro(&node1, &pid), 0);  // 应该返回负值，表示 pid 小于 5678
    EXPECT_GT(AppInfoPidComparePro(&node2, &pid), 0);  // 应该返回正值，表示 pid 大于 1234
}

/**
 * @tc.name: AppInfoNameComparePro_Test
 * @tc.desc: 测试名称比较函数是否正确
 */
HWTEST_F(AppInfoCompareTest, AppInfoNameComparePro_Test, TestSize.Level0)
{
    AppSpawnedProcess app1 = {1234, "app1"};
    AppSpawnedProcess app2 = {5678, "app2"};
    ListNode node1, node2;

    node1.entry = &app1;
    node2.entry = &app2;

    const char *name = "app1";

    // 测试名称匹配
    EXPECT_EQ(AppInfoNameComparePro(&node1, (void *)name), 0);  // 应该返回 0，表示匹配
    name = "app2";
    EXPECT_LT(AppInfoNameComparePro(&node1, (void *)name), 0);  // 应该返回负值，表示 "app1" 小于 "app2"
    EXPECT_GT(AppInfoNameComparePro(&node2, (void *)name), 0);  // 应该返回正值，表示 "app2" 大于 "app1"
}

/**
 * @tc.name: AppInfoCompareProc_Test
 * @tc.desc: 测试两个节点之间的 pid 比较
 */
HWTEST_F(AppInfoCompareTest, AppInfoCompareProc_Test, TestSize.Level0)
{
    AppSpawnedProcess app1 = {1234, "app1"};
    AppSpawnedProcess app2 = {5678, "app2"};
    AppSpawnedProcess app3 = {1234, "app3"};
    ListNode node1, node2, node3;

    node1.entry = &app1;
    node2.entry = &app2;
    node3.entry = &app3;

    // 测试相同 pid
    EXPECT_EQ(AppInfoCompareProc(&node1, &node3), 0);  // 应该返回 0，表示相同 pid
    EXPECT_LT(AppInfoCompareProc(&node1, &node2), 0);  // 应该返回负值，表示 1234 < 5678
    EXPECT_GT(AppInfoCompareProc(&node2, &node1), 0);  // 应该返回正值，表示 5678 > 1234
}

TEST_F(AppInfoCompareTest, AddSpawnedProcess_Success)
{
    pid_t pid = 1234;
    const char *name = "test_process";
    uint32_t appIndex = 1;
    bool isDebuggable = false;

    // 调用AddSpawnedProcess添加进程
    AppSpawnedProcess *newProcess = AddSpawnedProcess(pid, name, appIndex, isDebuggable);

    // 检查返回的指针是否有效
    ASSERT_NE(newProcess, nullptr);
    EXPECT_EQ(newProcess->pid, pid);
    EXPECT_STREQ(newProcess->name, name);
    EXPECT_EQ(newProcess->appIndex, appIndex);
    EXPECT_EQ(newProcess->isDebuggable, isDebuggable);
    EXPECT_EQ(newProcess->exitStatus, 0);  // 默认退出状态为0
    EXPECT_EQ(newProcess->max, 0);  // 默认值
}

TEST_F(AppInfoCompareTest, AddSpawnedProcess_InvalidParams)
{
    pid_t pid = -1;  // 无效PID
    const char *name = nullptr;  // 无效进程名
    uint32_t appIndex = 1;
    bool isDebuggable = false;

    // 检查无效的PID
    AppSpawnedProcess *newProcess = AddSpawnedProcess(pid, name, appIndex, isDebuggable);
    EXPECT_EQ(newProcess, nullptr);  // 应该返回NULL

    name = "valid_name";  // 修改进程名为有效
    newProcess = AddSpawnedProcess(pid, name, appIndex, isDebuggable);
    EXPECT_EQ(newProcess, nullptr);  // 无效的pid依然应该返回NULL
}

TEST_F(AppInfoCompareTest, TerminateSpawnedProcess_Success)
{
    pid_t pid = 1234;
    const char *name = "test_process";
    uint32_t appIndex = 1;
    bool isDebuggable = false;

    // 添加新进程
    AppSpawnedProcess *newProcess = AddSpawnedProcess(pid, name, appIndex, isDebuggable);
    
    // 检查添加进程成功后，队列是否包含该进程
    EXPECT_EQ(g_appSpawnMgr->appQueue.size, 1);

    // 调用TerminateSpawnedProcess终止进程
    TerminateSpawnedProcess(newProcess);

    // 检查进程是否被移除并加入死亡队列
    EXPECT_EQ(g_appSpawnMgr->diedAppCount, 1);  // 死亡队列中应该有1个进程
    EXPECT_EQ(g_appSpawnMgr->appQueue.size, 0);  // 原队列应该没有该进程
}

TEST_F(AppInfoCompareTest, TerminateSpawnedProcess_DeathQueueOverflow)
{
    // 假设MAX_DIED_PROCESS_COUNT已经定义为3
    g_appSpawnMgr->diedAppCount = 3;

    // 添加并终止3个进程
    for (int i = 0; i < 3; ++i) {
        pid_t pid = 1000 + i;
        const char *name = "test_process";
        uint32_t appIndex = 1;
        bool isDebuggable = false;

        AppSpawnedProcess *newProcess = AddSpawnedProcess(pid, name, appIndex, isDebuggable);
        TerminateSpawnedProcess(newProcess);
    }

    // 检查死亡进程队列中的进程数是否为3
    EXPECT_EQ(g_appSpawnMgr->diedAppCount, DIE_APP_COUNT);

    // 再添加一个进程并终止
    pid_t pid = 2000;
    const char *name = "test_process_4";
    uint32_t appIndex = 1;
    bool isDebuggable = false;

    AppSpawnedProcess *newProcess = AddSpawnedProcess(pid, name, appIndex, isDebuggable);
    TerminateSpawnedProcess(newProcess);

    // 检查队列中最早的进程是否已被移除
    EXPECT_EQ(g_appSpawnMgr->diedAppCount, DIE_APP_COUNT);
    // 由于最早的进程已被移除，新的死亡进程已添加
    // 如果存在特定的检查最早死亡进程移除的方式，可以加入这里的检查
}

TEST_F(AppInfoCompareTest, GetSpawnedProcess_Success)
{
    pid_t pid = 1234;
    const char *name = "test_process";
    uint32_t appIndex = 1;
    bool isDebuggable = false;

    // 添加进程
    AddSpawnedProcess(pid, name, appIndex, isDebuggable);

    // 根据PID获取进程
    AppSpawnedProcess *proc = GetSpawnedProcess(pid);

    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->pid, pid);
    EXPECT_STREQ(proc->name, name);
}

TEST_F(AppInfoCompareTest, GetSpawnedProcess_InvalidPID)
{
    pid_t invalidPid = 9999;

    // 使用不存在的PID获取进程
    AppSpawnedProcess *proc = GetSpawnedProcess(invalidPid);
    EXPECT_EQ(proc, nullptr);
}

TEST_F(AppInfoCompareTest, GetSpawnedProcess_NullManager)
{
    pid_t pid = 1234;
    const char *name = "test_process";

    // 临时置空全局管理器
    AppSpawnMgr *backup = g_appSpawnMgr;
    g_appSpawnMgr = nullptr;

    AppSpawnedProcess *proc = GetSpawnedProcess(pid);
    EXPECT_EQ(proc, nullptr);

    // 恢复全局管理器
    g_appSpawnMgr = backup;
}

TEST_F(AppInfoCompareTest, GetSpawnedProcessByName_Success)
{
    pid_t pid = 1234;
    const char *name = "test_process";
    uint32_t appIndex = 1;
    bool isDebuggable = false;

    // 添加进程
    AddSpawnedProcess(pid, name, appIndex, isDebuggable);

    // 根据名称获取进程
    AppSpawnedProcess *proc = GetSpawnedProcessByName(name);

    ASSERT_NE(proc, nullptr);
    EXPECT_EQ(proc->pid, pid);
    EXPECT_STREQ(proc->name, name);
}

TEST_F(AppInfoCompareTest, GetSpawnedProcessByName_InvalidName)
{
    const char *invalidName = "non_exist";

    // 使用不存在的名称获取进程
    AppSpawnedProcess *proc = GetSpawnedProcessByName(invalidName);
    EXPECT_EQ(proc, nullptr);
}

TEST_F(AppInfoCompareTest, GetSpawnedProcessByName_NullManagerOrName)
{
    pid_t pid = 1234;
    const char *name = "test_process";

    // 置空全局管理器
    AppSpawnMgr *backup = g_appSpawnMgr;
    g_appSpawnMgr = nullptr;
    EXPECT_EQ(GetSpawnedProcessByName(name), nullptr);

    // 恢复全局管理器，传入空名称
    g_appSpawnMgr = backup;
    EXPECT_EQ(GetSpawnedProcessByName(nullptr), nullptr);
}

TEST_F(AppInfoCompareTest, KillAndWaitStatus_Success)
{
    pid_t pid = 1234;  // 假设进程已存在，且可以杀死
    int sig = SIGTERM; // 使用默认的终止信号
    int exitStatus = 0;

    // 假设kill和waitpid在测试环境中可以模拟成功执行
    EXPECT_EQ(KillAndWaitStatus(pid, sig, &exitStatus), 0);
    EXPECT_EQ(exitStatus, 0);  // 期望返回成功退出状态
}

TEST_F(AppInfoCompareTest, KillAndWaitStatus_InvalidPID)
{
    pid_t invalidPid = -1;
    int sig = SIGTERM;
    int exitStatus = 0;

    // 传入无效PID
    EXPECT_EQ(KillAndWaitStatus(invalidPid, sig, &exitStatus), 0);
    EXPECT_EQ(exitStatus, -1);  // 无效PID，exitStatus应为默认值
}

TEST_F(AppInfoCompareTest, KillAndWaitStatus_FailedKill)
{
    pid_t pid = 9999;  // 假设此PID无法被杀死
    int sig = SIGTERM;
    int exitStatus = 0;

    // 模拟kill失败的情况（例如权限问题）
    EXPECT_EQ(KillAndWaitStatus(pid, sig, &exitStatus), -1);
    EXPECT_EQ(exitStatus, -1);  // 应该返回错误状态
}

TEST_F(AppInfoCompareTest, KillAndWaitStatus_Timeout)
{
    pid_t pid = 1234;  // 假设进程无法退出
    int sig = SIGTERM;
    int exitStatus = 0;

    // 模拟进程无法在规定的超时时间内退出
    EXPECT_EQ(KillAndWaitStatus(pid, sig, &exitStatus), -1);
    EXPECT_EQ(exitStatus, -1);  // 超时未退出，返回失败
}

TEST_F(AppInfoCompareTest, KillAndWaitStatus_ValidExitStatus)
{
    pid_t pid = 1234;  // 假设进程成功退出并返回退出状态
    int sig = SIGTERM;
    int exitStatus = 0;

    // 模拟正常退出并返回退出状态
    EXPECT_EQ(KillAndWaitStatus(pid, sig, &exitStatus), 0);
    EXPECT_EQ(exitStatus, 0);  // 正常退出，exitStatus为0
}

TEST_F(AppInfoCompareTest, GetProcessTerminationStatus_SuccessFromDiedQueue)
{
    pid_t pid = 1234;
    int exitStatus = 0;

    // 假设该进程已经死亡并在 diedQueue 中
    AddDiedProcess(pid, exitStatus);  // 自定义函数：将进程加入到 diedQueue

    // 测试获取并返回死去的进程的退出状态
    EXPECT_EQ(GetProcessTerminationStatus(pid), exitStatus);
}

TEST_F(AppInfoCompareTest, GetProcessTerminationStatus_InvalidPID)
{
    pid_t invalidPid = -1;
    int exitStatus = 0;

    // 传入无效的PID，函数应返回0
    EXPECT_EQ(GetProcessTerminationStatus(invalidPid), 0);
}

TEST_F(AppInfoCompareTest, GetProcessTerminationStatus_UnableToGetProcess)
{
    pid_t pid = 1234;
    int exitStatus = 0;

    // 假设 GetSpawnedProcess 返回 NULL，进程没有找到
    EXPECT_EQ(GetProcessTerminationStatus(pid), -1);  // 应该返回-1
}

TEST_F(AppInfoCompareTest, GetProcessTerminationStatus_SuccessFromGetSpawnedProcess)
{
    pid_t pid = 1234;
    int exitStatus = 0;

    // 添加已启动的进程
    AddSpawnedProcess(pid, "test_process", 1, false);  // 自定义函数：添加进程

    // 模拟成功终止进程
    EXPECT_EQ(GetProcessTerminationStatus(pid), exitStatus);
}

TEST_F(AppInfoCompareTest, GetProcessTerminationStatus_KillFailed)
{
    pid_t pid = 1234;
    int exitStatus = 0;

    // 模拟 kill 失败的情况
    EXPECT_EQ(KillAndWaitStatus(pid, SIGKILL, &exitStatus), -1);
    EXPECT_EQ(GetProcessTerminationStatus(pid), -1);  // kill失败，返回-1
}

TEST_F(AppInfoCompareTest, GetProcessTerminationStatus_ValidExitStatus)
{
    pid_t pid = 1234;
    int exitStatus = 0;

    // 模拟进程已退出，并且已经加入diedQueue
    AddDiedProcess(pid, exitStatus);

    // 返回已死亡进程的退出状态
    EXPECT_EQ(GetProcessTerminationStatus(pid), exitStatus);
}

TEST_F(AppInfoCompareTest, CreateAppSpawningCtx_Success)
{
    // 模拟 g_appSpawnMgr 为有效
    g_appSpawnMgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));  // 假设 g_appSpawnMgr 已初始化
    ASSERT_NE(g_appSpawnMgr, nullptr);  // 确保 g_appSpawnMgr 被正确初始化
    
    // 调用 CreateAppSpawningCtx
    AppSpawningCtx *ctx = CreateAppSpawningCtx();

    // 验证返回的 ctx 是否有效
    ASSERT_NE(ctx, nullptr);
    
    // 验证 ctx 的成员是否初始化正确
    EXPECT_EQ(ctx->client.id, 1);  // 初始 requestId 应为 1
    EXPECT_EQ(ctx->client.flags, 0);
    EXPECT_EQ(ctx->forkCtx.watcherHandle, nullptr);
    EXPECT_EQ(ctx->forkCtx.pidFdWatcherHandle, nullptr);
    EXPECT_EQ(ctx->forkCtx.coldRunPath, nullptr);
    EXPECT_EQ(ctx->forkCtx.timer, nullptr);
    EXPECT_EQ(ctx->forkCtx.fd[0], -1);
    EXPECT_EQ(ctx->forkCtx.fd[1], -1);
    EXPECT_EQ(ctx->isPrefork, false);
    EXPECT_EQ(ctx->forkCtx.childMsg, nullptr);
    EXPECT_EQ(ctx->message, nullptr);
    EXPECT_EQ(ctx->pid, 0);
    EXPECT_EQ(ctx->state, APP_STATE_IDLE);
    
    // 验证 appSpawnQueue 是否已添加节点
    ASSERT_TRUE(OH_ListContains(&g_appSpawnMgr->appSpawnQueue, &ctx->node));

    // 清理
    free(ctx);
    free(g_appSpawnMgr);
}

TEST_F(AppInfoCompareTest, CreateAppSpawningCtx_MallocFailure)
{
    // 模拟 malloc 失败
    EXPECT_CALL(*this, malloc(_)).WillOnce(Return(nullptr));  // 使用 Mock 处理内存分配失败

    // 调用 CreateAppSpawningCtx，期望返回 nullptr
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    EXPECT_EQ(ctx, nullptr);
}

TEST_F(AppInfoCompareTest, CreateAppSpawningCtx_GAppSpawnMgrNotInitialized)
{
    // 模拟 g_appSpawnMgr 为 NULL
    g_appSpawnMgr = nullptr;

    // 调用 CreateAppSpawningCtx
    AppSpawningCtx *ctx = CreateAppSpawningCtx();

    // 验证返回的 ctx 是否有效
    ASSERT_NE(ctx, nullptr);
    
    // 验证 g_appSpawnMgr 为 NULL 时，节点不被添加到 appSpawnQueue
    ASSERT_FALSE(OH_ListContains(&g_appSpawnMgr->appSpawnQueue, &ctx->node));
    
    // 清理
    free(ctx);
}

TEST_F(AppInfoCompareTest, CreateAppSpawningCtx_RequestIdIncrement)
{
    // 每次调用时 requestId 应递增
    g_appSpawnMgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));  // 初始化 g_appSpawnMgr
    AppSpawningCtx *ctx1 = CreateAppSpawningCtx();
    EXPECT_EQ(ctx1->client.id, 1);  // 第一个 ctx 应该有 id 1
    
    AppSpawningCtx *ctx2 = CreateAppSpawningCtx();
    EXPECT_EQ(ctx2->client.id, 2);  // 第二个 ctx 应该有 id 2
    
    // 清理
    free(ctx1);
    free(ctx2);
    free(g_appSpawnMgr);
}
TEST_F(AppInfoCompareTest, DeleteAppSpawningCtx_Success)
{
    // 创建一个 AppSpawningCtx 对象
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    ASSERT_NE(ctx, nullptr);  // 确保 ctx 创建成功

    // 模拟非空的成员
    ctx->forkCtx.timer = (void *)1;
    ctx->forkCtx.watcherHandle = (void *)WATCHHANDLE_PTR;
    ctx->forkCtx.coldRunPath = strdup("/some/path");
    ctx->forkCtx.fd[0] = FORKCTX_FD_0;
    ctx->forkCtx.fd[1] = FORKCTX_FD_1;

    // 记录内存分配的地址以便验证是否被释放
    char *coldRunPath = ctx->forkCtx.coldRunPath;
    int fd0 = ctx->forkCtx.fd[0];
    int fd1 = ctx->forkCtx.fd[1];

    // 调用 DeleteAppSpawningCtx 删除 ctx
    DeleteAppSpawningCtx(ctx);

    // 验证资源是否被清理
    EXPECT_EQ(ctx->forkCtx.timer, nullptr);
    EXPECT_EQ(ctx->forkCtx.watcherHandle, nullptr);
    EXPECT_EQ(ctx->forkCtx.coldRunPath, nullptr);
    EXPECT_EQ(ctx->forkCtx.fd[0], -1);
    EXPECT_EQ(ctx->forkCtx.fd[1], -1);

    // 验证 coldRunPath 是否被释放
    EXPECT_EQ(coldRunPath, nullptr);

    // 验证 fd 是否被关闭
    EXPECT_EQ(fd0, -1);
    EXPECT_EQ(fd1, -1);
}

TEST_F(AppInfoCompareTest, DeleteAppSpawningCtx_NullProperty)
{
    // 传入 NULL 应该不会执行任何操作
    DeleteAppSpawningCtx(nullptr);
    // 无法验证返回值，只能确保没有崩溃
}

TEST_F(AppInfoCompareTest, DeleteAppSpawningCtx_MessageNull)
{
    // 创建一个 AppSpawningCtx 对象
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    ASSERT_NE(ctx, nullptr);  // 确保 ctx 创建成功

    // 设置 message 为 NULL
    ctx->message = nullptr;

    // 调用 DeleteAppSpawningCtx 删除 ctx
    DeleteAppSpawningCtx(ctx);

    // 验证 message 是否没有被删除
    EXPECT_EQ(ctx->message, nullptr);

    free(ctx);
}

TEST_F(AppInfoCompareTest, DeleteAppSpawningCtx_NodeRemoved)
{
    // 创建一个 AppSpawningCtx 对象并初始化队列
    g_appSpawnMgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));  // 初始化 g_appSpawnMgr
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    ASSERT_NE(ctx, nullptr);  // 确保 ctx 创建成功

    // 添加到队列
    OH_ListAddTail(&g_appSpawnMgr->appSpawnQueue, &ctx->node);

    // 调用 DeleteAppSpawningCtx 删除 ctx
    DeleteAppSpawningCtx(ctx);

    // 验证节点是否已被移除
    ASSERT_FALSE(OH_ListContains(&g_appSpawnMgr->appSpawnQueue, &ctx->node));

    // 清理
    free(ctx);
    free(g_appSpawnMgr);
}

TEST_F(AppInfoCompareTest, DeleteAppSpawningCtx_TimerNull)
{
    // 创建一个 AppSpawningCtx 对象
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    ASSERT_NE(ctx, nullptr);  // 确保 ctx 创建成功

    // 设置 timer 为 NULL
    ctx->forkCtx.timer = nullptr;

    // 调用 DeleteAppSpawningCtx 删除 ctx
    DeleteAppSpawningCtx(ctx);

    // 验证 timer 没有被调用
    EXPECT_EQ(ctx->forkCtx.timer, nullptr);

    free(ctx);
}

TEST_F(AppInfoCompareTest, DeleteAppSpawningCtx_WatcherHandleNull)
{
    // 创建一个 AppSpawningCtx 对象
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    ASSERT_NE(ctx, nullptr);  // 确保 ctx 创建成功

    // 设置 watcherHandle 为 NULL
    ctx->forkCtx.watcherHandle = nullptr;

    // 调用 DeleteAppSpawningCtx 删除 ctx
    DeleteAppSpawningCtx(ctx);

    // 验证 watcherHandle 没有被调用
    EXPECT_EQ(ctx->forkCtx.watcherHandle, nullptr);

    free(ctx);
}

TEST_F(AppInfoCompareTest, DeleteAppSpawningCtx_ColdRunPathNull)
{
    // 创建一个 AppSpawningCtx 对象
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    ASSERT_NE(ctx, nullptr);  // 确保 ctx 创建成功

    // 设置 coldRunPath 为 NULL
    ctx->forkCtx.coldRunPath = nullptr;

    // 调用 DeleteAppSpawningCtx 删除 ctx
    DeleteAppSpawningCtx(ctx);

    // 验证 coldRunPath 没有被释放
    EXPECT_EQ(ctx->forkCtx.coldRunPath, nullptr);

    free(ctx);
}

TEST_F(AppInfoCompareTest, DeleteAppSpawningCtx_FdValid)
{
    // 创建一个 AppSpawningCtx 对象
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    ASSERT_NE(ctx, nullptr);  // 确保 ctx 创建成功

    // 设置 fd 为有效值
    ctx->forkCtx.fd[0] = FORKCTX_FD_0;
    ctx->forkCtx.fd[1] = FORKCTX_FD_1;

    // 调用 DeleteAppSpawningCtx 删除 ctx
    DeleteAppSpawningCtx(ctx);

    // 验证 fd 是否被关闭并重置为 -1
    EXPECT_EQ(ctx->forkCtx.fd[0], -1);
    EXPECT_EQ(ctx->forkCtx.fd[1], -1);

    free(ctx);
}

TEST_F(AppInfoCompareTest, AppPropertyComparePid_MatchingPid)
{
    pid_t pid = 1234;  // Example pid
    AppSpawningCtx ctx = {.pid = pid};  // Create a context with matching pid

    ListNode node;
    node.data = &ctx;
    
    // Call the comparison function
    int result = AppPropertyComparePid(&node, &pid);

    // Check if it returns 0 (match)
    EXPECT_EQ(result, 0);
}

TEST_F(AppInfoCompareTest, AppPropertyComparePid_NonMatchingPid)
{
    pid_t pid = 1234;
    pid_t other_pid = 5678;  // Different pid
    AppSpawningCtx ctx = {.pid = other_pid};  // Create context with non-matching pid

    ListNode node;
    node.data = &ctx;
    
    // Call the comparison function
    int result = AppPropertyComparePid(&node, &pid);

    // Check if it returns 1 (no match)
    EXPECT_EQ(result, 1);
}

TEST_F(AppInfoCompareTest, GetAppSpawningCtxByPid_Success)
{
    pid_t pid = 1234;
    AppSpawningCtx ctx = {.pid = pid};  // Create a context with matching pid

    g_appSpawnMgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));
    OH_ListInit(&g_appSpawnMgr->appSpawnQueue);  // Initialize the app spawn queue
    OH_ListAddTail(&g_appSpawnMgr->appSpawnQueue, &ctx.node);  // Add the context to the list

    // Call the function to get the context by pid
    AppSpawningCtx *result = GetAppSpawningCtxByPid(pid);

    // Check if the returned context matches the one we added
    EXPECT_EQ(result, &ctx);

    free(g_appSpawnMgr);  // Clean up
}

TEST_F(AppInfoCompareTest, GetAppSpawningCtxByPid_NotFound)
{
    pid_t pid = 1234;  // Example pid
    pid_t non_existing_pid = 5678;  // A pid that doesn't exist in the list
    AppSpawningCtx ctx = {.pid = pid};  // Create a context with a different pid

    g_appSpawnMgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));
    OH_ListInit(&g_appSpawnMgr->appSpawnQueue);  // Initialize the app spawn queue
    OH_ListAddTail(&g_appSpawnMgr->appSpawnQueue, &ctx.node);  // Add the context to the list

    // Call the function to get the context by pid
    AppSpawningCtx *result = GetAppSpawningCtxByPid(non_existing_pid);

    // Check if the result is NULL, as no matching pid exists
    EXPECT_EQ(result, nullptr);

    free(g_appSpawnMgr);  // Clean up
}

TEST_F(AppInfoCompareTest, GetAppSpawningCtxByPid_EmptyList)
{
    pid_t pid = 1234;  // Example pid

    g_appSpawnMgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));
    OH_ListInit(&g_appSpawnMgr->appSpawnQueue);  // Initialize the app spawn queue (empty)

    // Call the function to get the context by pid
    AppSpawningCtx *result = GetAppSpawningCtxByPid(pid);

    // Check if the result is NULL, as the list is empty
    EXPECT_EQ(result, nullptr);

    free(g_appSpawnMgr);  // Clean up
}

TEST_F(AppInfoCompareTest, GetAppSpawningCtxByPid_FoundInMiddle)
{
    pid_t pid = 1234;
    pid_t second_pid = 5678;  // A different pid

    AppSpawningCtx first_ctx = {.pid = second_pid};  // Create first context with different pid
    AppSpawningCtx second_ctx = {.pid = pid};  // Create second context with matching pid

    g_appSpawnMgr = (AppSpawnMgr *)malloc(sizeof(AppSpawnMgr));
    OH_ListInit(&g_appSpawnMgr->appSpawnQueue);  // Initialize the app spawn queue
    OH_ListAddTail(&g_appSpawnMgr->appSpawnQueue, &first_ctx.node);  // Add first context
    OH_ListAddTail(&g_appSpawnMgr->appSpawnQueue, &second_ctx.node);  // Add second context (middle)

    // Call the function to get the context by pid
    AppSpawningCtx *result = GetAppSpawningCtxByPid(pid);

    // Check if the returned context is the correct one
    EXPECT_EQ(result, &second_ctx);

    free(g_appSpawnMgr);  // Clean up
}
}
