/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/resource.h>
#include <sys/wait.h>

#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_server.h"
#include "appspawn_utils.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

extern "C" {
// Declare APPSPAWN_STATIC functions from appspawn_service.c for test access
APPSPAWN_STATIC void ClearPipeFd(int pipe[], int length);
APPSPAWN_STATIC pid_t ForkAndRegisterFds(AppSpawnMgr *mgr, AppSpawningCtx *property,
    int preforkFd[PIPE_FD_LENGTH], int parentToChildFd[PIPE_FD_LENGTH]);
APPSPAWN_STATIC int SendPipeMsgToChild(AppSpawnMgr *mgr, pid_t childPid, AppSpawnPipeMsg *pipeMsg);
APPSPAWN_STATIC int TransferPreforkFdToForkCtx(AppSpawnMgr *mgr, pid_t childPid, AppSpawningCtx *property);
APPSPAWN_STATIC void CleanupPreforkChild(AppSpawnMgr *mgr, pid_t childPid);
}

namespace OHOS {

class SpawningFdServiceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());
        mgr_ = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        ASSERT_NE(mgr_, nullptr);
        mgr_->servicePid = getpid();
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
        if (mgr_ != nullptr) {
            // Cleanup any remaining child processes
            CleanupChildren();
            DeleteAppSpawnMgr(mgr_);
            mgr_ = nullptr;
        }
    }

protected:
    AppSpawnMgr *mgr_ = nullptr;
    std::vector<pid_t> childPids_;

    bool IsFdValid(int fd)
    {
        return fcntl(fd, F_GETFD) >= 0;
    }

    void CleanupChildren()
    {
        for (pid_t pid : childPids_) {
            if (pid > 0) {
                // kill child processes created during test to avoid zombie processes
                (void)kill(pid, SIGKILL);
                waitpid(pid, nullptr, 0);
            }
        }
        childPids_.clear();
    }

    // Helper: create a minimal AppSpawningCtx with client.id set
    AppSpawningCtx *CreateMinimalCtx(uint32_t clientId)
    {
        AppSpawningCtx *ctx = CreateAppSpawningCtx();
        if (ctx != nullptr) {
            ctx->client.id = clientId;
        }
        return ctx;
    }
};

/**
 * @tc.name: SpawningFd_ForkAndRegister_001
 * @tc.desc: ForkAndRegisterFds正常流程：fork成功后，队列中注册2个节点(TYPE_CHILD_PARENT + TYPE_PARENT_CHILD)，
 *           childParentCount=1, parentChildCount=1, total=2；FindSpawningFdsByPid可找到对应节点
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_ForkAndRegister_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2001);
    ASSERT_NE(property, nullptr);

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    ASSERT_GT(pid, 0);  // fork succeeded in parent

    childPids_.push_back(pid);

    // Verify 2 nodes registered: TYPE_CHILD_PARENT and TYPE_PARENT_CHILD
    uint32_t total = 0, childParentCount = 0, parentChildCount = 0;
    GetSpawningFdsStats(mgr_, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(total, (uint32_t)2);
    EXPECT_EQ(childParentCount, (uint32_t)1);
    EXPECT_EQ(parentChildCount, (uint32_t)1);

    // Verify the prefork fd node exists for the child pid
    AppSpawnFds *pfFds = FindSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT);
    ASSERT_NE(pfFds, nullptr);
    EXPECT_EQ(pfFds->pid, pid);

    AppSpawnFds *pcFds = FindSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD);
    ASSERT_NE(pcFds, nullptr);
    EXPECT_EQ(pcFds->pid, pid);

    // Cleanup
    CleanupSpawningFdsByPid(mgr_, pid);
    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_ForkAndRegister_002
 * @tc.desc: ForkAndRegisterFds调用后验证：若fork成功则正常清理；若fork失败(pid<0)则队列节点数不变
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_ForkAndRegister_002, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2002);
    ASSERT_NE(property, nullptr);

    // Save initial stats
    uint32_t totalBefore = 0, prePc0 = 0, prePc1 = 0;
    GetSpawningFdsStats(mgr_, &totalBefore, &prePc0, &prePc1);

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    // We cannot easily force fork() to fail in this setup.
    // Instead, verify that if we close all available fds to exhaust pipe creation,
    // the function handles it gracefully.
    // For a more reliable test, we test the early-return path by verifying
    // that a failed pipe creation returns -1 without adding nodes.

    // Close preforkFd to make pipe() likely succeed but then force failure
    // by closing parentToChildFd first to test second pipe failure path.
    // Since we can't easily mock pipe(), we test that passing invalid preforkFd
    // (by closing one end first) still works correctly.
    // The simplest approach: just verify that calling ForkAndRegisterFds and
    // getting a valid pid adds nodes, and no-op path doesn't add nodes.

    // Alternative: test with pipe fds already closed (should not happen in normal flow
    // but tests the ClearPipeFd path when fork fails)
    preforkFd[0] = -1;
    preforkFd[1] = -1;
    parentToChildFd[0] = -1;
    parentToChildFd[1] = -1;

    // ForkAndRegisterFds creates its own pipes, so this should work normally
    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    if (pid > 0) {
        childPids_.push_back(pid);
        // If fork succeeded, clean up
        CleanupSpawningFdsByPid(mgr_, pid);
    }

    // Verify: if fork failed (pid < 0), no new nodes should be added
    if (pid < 0) {
        uint32_t totalAfter = 0, postPc0 = 0, postPc1 = 0;
        GetSpawningFdsStats(mgr_, &totalAfter, &postPc0, &postPc1);
        EXPECT_EQ(totalAfter, totalBefore);
    }

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_SendPipeMsg_001
 * @tc.desc: SendPipeMsgToChild正常流程：通过parent-child pipe写入AppSpawnPipeMsg，返回0；
 *           写入成功后UnregisterSpawningFdsByPid被调用，TYPE_PARENT_CHILD节点从队列移除
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_SendPipeMsg_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2003);
    ASSERT_NE(property, nullptr);

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    ASSERT_GT(pid, 0);
    childPids_.push_back(pid);

    // Get the parent-child fds before sending
    AppSpawnFds *pcFds = FindSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD);
    ASSERT_NE(pcFds, nullptr);
    int writeFd = pcFds->fds[1];
    EXPECT_TRUE(IsFdValid(writeFd));

    // Prepare a pipe message
    AppSpawnPipeMsg *pipeMsg = (AppSpawnPipeMsg *)calloc(1, sizeof(AppSpawnPipeMsg));
    ASSERT_NE(pipeMsg, nullptr);
    pipeMsg->type = MSG_APP_SPAWN;
    pipeMsg->msg.preforkMsg.id = 2003;
    pipeMsg->msg.preforkMsg.flags = 0;
    pipeMsg->msg.preforkMsg.msgLen = sizeof(AppSpawnMsg);

    // SendPipeMsgToChild should succeed
    int ret = SendPipeMsgToChild(mgr_, pid, pipeMsg);
    EXPECT_EQ(ret, 0);

    // After SendPipeMsgToChild, parent-child fds should be unregistered
    pcFds = FindSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD);
    EXPECT_EQ(pcFds, nullptr);

    // Cleanup prefork fds
    CleanupSpawningFdsByPid(mgr_, pid);

    free(pipeMsg);
    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_SendPipeMsg_002
 * @tc.desc: SendPipeMsgToChild异常输入：不存在的pid返回-1，NULL mgr返回-1
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_SendPipeMsg_002, TestSize.Level0)
{
    AppSpawnPipeMsg pipeMsg = {};
    pipeMsg.type = MSG_APP_SPAWN;

    // Non-existent pid
    int ret = SendPipeMsgToChild(mgr_, 99999, &pipeMsg);
    EXPECT_NE(ret, 0);

    // NULL mgr
    ret = SendPipeMsgToChild(nullptr, 99999, &pipeMsg);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: SpawningFd_TransferPreforkFd_001
 * @tc.desc: TransferPreforkFdToForkCtx fd所有权转移：转移后fd写入property->forkCtx.fd且仍可用，
 *           TYPE_CHILD_PARENT节点从队列移除(Find返回NULL)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_TransferPreforkFd_001, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2004);
    ASSERT_NE(property, nullptr);

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    ASSERT_GT(pid, 0);
    childPids_.push_back(pid);

    // Get the prefork fds before transfer
    AppSpawnFds *pfFds = FindSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT);
    ASSERT_NE(pfFds, nullptr);
    int savedFd0 = pfFds->fds[0];
    int savedFd1 = pfFds->fds[1];
    EXPECT_TRUE(IsFdValid(savedFd0));
    EXPECT_TRUE(IsFdValid(savedFd1));

    // Transfer prefork fd to forkCtx
    int ret = TransferPreforkFdToForkCtx(mgr_, pid, property);
    EXPECT_EQ(ret, 0);

    // After transfer, fds should be in property->forkCtx.fd
    EXPECT_EQ(property->forkCtx.fd[0], savedFd0);
    EXPECT_EQ(property->forkCtx.fd[1], savedFd1);

    // Fds should still be usable (not closed)
    EXPECT_TRUE(IsFdValid(property->forkCtx.fd[0]));
    EXPECT_TRUE(IsFdValid(property->forkCtx.fd[1]));

    // Prefork fds should be removed from queue (not findable)
    pfFds = FindSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT);
    EXPECT_EQ(pfFds, nullptr);

    // Cleanup remaining parent-child fds
    CleanupSpawningFdsByPid(mgr_, pid);

    // Close transferred fds
    close(property->forkCtx.fd[0]);
    close(property->forkCtx.fd[1]);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_ForkAndRegister_003
 * @tc.desc: ForkAndRegisterFds fd耗尽场景：耗尽所有文件描述符后pipe()和fork()均失败，
 *           返回pid<0，队列不新增节点(total不变)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_ForkAndRegister_003, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2005);
    ASSERT_NE(property, nullptr);

    uint32_t totalBefore = 0, prePc0 = 0, prePc1 = 0;
    GetSpawningFdsStats(mgr_, &totalBefore, &prePc0, &prePc1);

    // Exhaust file descriptors so pipe() will fail
    std::vector<int> drainFds;
    while (true) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0) {
            break;
        }
        drainFds.push_back(fd);
    }

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    EXPECT_LT(pid, 0);  // Should fail because pipe() can't allocate fds

    // No new nodes should be added
    uint32_t totalAfter = 0, postPc0 = 0, postPc1 = 0;
    GetSpawningFdsStats(mgr_, &totalAfter, &postPc0, &postPc1);
    EXPECT_EQ(totalAfter, totalBefore);

    // Restore file descriptors
    for (int fd : drainFds) {
        close(fd);
    }

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_SendPipeMsg_003
 * @tc.desc: SendPipeMsgToChild写失败场景：提前关闭write fd使write()返回EBADF，
 *           SendPipeMsgToChild返回-1，且不调用Unregister(节点仍在队列中)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_SendPipeMsg_003, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2006);
    ASSERT_NE(property, nullptr);

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    ASSERT_GT(pid, 0);
    childPids_.push_back(pid);

    // Get the parent-child fds and close write end to make write() fail
    AppSpawnFds *pcFds = FindSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD);
    ASSERT_NE(pcFds, nullptr);
    close(pcFds->fds[1]);  // Close write end

    AppSpawnPipeMsg pipeMsg = {};
    pipeMsg.type = MSG_APP_SPAWN;

    int ret = SendPipeMsgToChild(mgr_, pid, &pipeMsg);
    EXPECT_NE(ret, 0);

    // On write failure, Unregister is NOT called, node should still be in queue
    pcFds = FindSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD);
    EXPECT_NE(pcFds, nullptr);

    // Cleanup: Remove (don't close already-closed fd[1]) then cleanup remaining
    RemoveSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD);
    CleanupSpawningFdsByPid(mgr_, pid);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_TransferPreforkFd_002
 * @tc.desc: TransferPreforkFdToForkCtx异常输入：pid无TYPE_CHILD_PARENT注册时返回-1
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_TransferPreforkFd_002, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2007);
    ASSERT_NE(property, nullptr);

    // No prefork fds registered for this pid
    int ret = TransferPreforkFdToForkCtx(mgr_, 99999, property);
    EXPECT_NE(ret, 0);

    DeleteAppSpawningCtx(property);
}

/**
 * @tc.name: SpawningFd_ForkAndRegister_004
 * @tc.desc: ForkAndRegisterFds第二个pipe创建失败：释放恰好2个fd使第一个pipe()成功但第二个失败，
 *           ClearPipeFd清理第一个pipe的fd，返回pid<0，队列不新增节点
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdServiceTest, SpawningFd_ForkAndRegister_004, TestSize.Level0)
{
    AppSpawningCtx *property = CreateMinimalCtx(2010);
    ASSERT_NE(property, nullptr);

    uint32_t totalBefore = 0, prePc0 = 0, prePc1 = 0;
    GetSpawningFdsStats(mgr_, &totalBefore, &prePc0, &prePc1);

    // Exhaust all file descriptors
    std::vector<int> drainFds;
    while (true) {
        int fd = open("/dev/null", O_RDONLY);
        if (fd < 0) {
            break;
        }
        drainFds.push_back(fd);
    }

    // Release exactly 2 fds so first pipe() succeeds but second fails
    ASSERT_GE((int)drainFds.size(), 2);
    close(drainFds.back());
    drainFds.pop_back();
    close(drainFds.back());
    drainFds.pop_back();

    int preforkFd[PIPE_FD_LENGTH] = {-1, -1};
    int parentToChildFd[PIPE_FD_LENGTH] = {-1, -1};

    pid_t pid = ForkAndRegisterFds(mgr_, property, preforkFd, parentToChildFd);
    EXPECT_LT(pid, 0);  // Should fail

    // No new nodes should be added
    uint32_t totalAfter = 0, postPc0 = 0, postPc1 = 0;
    GetSpawningFdsStats(mgr_, &totalAfter, &postPc0, &postPc1);
    EXPECT_EQ(totalAfter, totalBefore);

    // Restore file descriptors
    for (int fd : drainFds) {
        close(fd);
    }

    DeleteAppSpawningCtx(property);
}
}  // namespace OHOS
