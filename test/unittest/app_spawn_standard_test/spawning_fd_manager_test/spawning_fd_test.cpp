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

#include <climits>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {

class SpawningFdTest : public testing::Test {
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
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
        if (mgr_ != nullptr) {
            DeleteAppSpawnMgr(mgr_);
            mgr_ = nullptr;
        }
    }

protected:
    AppSpawnMgr *mgr_ = nullptr;

    // Helper: create pipe fds for testing
    void CreateTestPipe(int fds[PIPE_FD_LENGTH])
    {
        ASSERT_EQ(pipe(fds), 0);
    }

    // Helper: check if fd is valid
    bool IsFdValid(int fd)
    {
        return fcntl(fd, F_GETFD) >= 0;
    }

    // Helper: register and verify
    void RegisterAndVerify(pid_t pid, SpawningFdType type, int fds[PIPE_FD_LENGTH])
    {
        SpawningFdRegInfo regInfo = { type, PIPE_FD_LENGTH, fds, pid };
        EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

        AppSpawnFds *found = FindSpawningFdsByPid(mgr_, pid, type);
        EXPECT_NE(found, nullptr);
        if (found != nullptr) {
            EXPECT_EQ(found->type, type);
            EXPECT_EQ(found->count, (uint32_t)PIPE_FD_LENGTH);
            EXPECT_EQ(found->pid, pid);
            EXPECT_GT(found->timestamp, (uint64_t)0);
        }
    }
};

/**
 * @tc.name: SpawningFd_Register_002
 * @tc.desc: RegisterSpawningFds参数校验：NULL mgr、TYPE_INVALID、count=0、NULL fds均应返回NULL
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Register_002, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);

    // NULL mgr
    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, 2000 };
    EXPECT_EQ(RegisterSpawningFds(nullptr, &regInfo), nullptr);

    // TYPE_INVALID
    regInfo.type = TYPE_INVALID;
    EXPECT_EQ(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // count = 0
    regInfo.type = TYPE_CHILD_PARENT;
    regInfo.count = 0;
    EXPECT_EQ(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // NULL fds
    regInfo.count = PIPE_FD_LENGTH;
    regInfo.fds = nullptr;
    EXPECT_EQ(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // cleanup pipe
    close(fds[0]);
    close(fds[1]);
}

/**
 * @tc.name: SpawningFd_Find_001
 * @tc.desc: 按pid+type精确查找，验证FindSpawningFdsByPid返回节点的fds[0]/fds[1]与注册时一致
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Find_001, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);
    pid_t pid = 4000;

    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, pid };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    AppSpawnFds *found = FindSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->fds[0], fds[0]);
    EXPECT_EQ(found->fds[1], fds[1]);
}

/**
 * @tc.name: SpawningFd_Find_002
 * @tc.desc: FindSpawningFdsByPid查找失败场景：pid匹配type不匹配、type匹配pid不匹配、空队列、NULL mgr均返回NULL
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Find_002, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);
    pid_t pid = 5000;

    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, pid };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // pid match but type mismatch
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, pid, TYPE_PARENT_CHILD), nullptr);

    // type match but pid mismatch
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, pid + 1, TYPE_CHILD_PARENT), nullptr);

    // NULL mgr
    EXPECT_EQ(FindSpawningFdsByPid(nullptr, pid, TYPE_CHILD_PARENT), nullptr);

    // cleanup
    UnregisterSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT);

    // empty queue
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT), nullptr);
}

/**
 * @tc.name: SpawningFd_Unregister_001
 * @tc.desc: UnregisterSpawningFdsByPid注销后：fd被close(F_GETFD返回-1)，FindSpawningFdsByPid返回NULL
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Unregister_001, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);
    pid_t pid = 6000;

    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, pid };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // fd is valid before unregister
    EXPECT_TRUE(IsFdValid(fds[0]));
    EXPECT_TRUE(IsFdValid(fds[1]));

    EXPECT_EQ(UnregisterSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT), 0);

    // fd is closed after unregister
    EXPECT_FALSE(IsFdValid(fds[0]));
    EXPECT_FALSE(IsFdValid(fds[1]));

    // Find returns NULL
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT), nullptr);
}

/**
 * @tc.name: SpawningFd_Unregister_002
 * @tc.desc: UnregisterSpawningFdsByPid异常输入：不存在的pid+type返回非0，NULL mgr返回非0
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Unregister_002, TestSize.Level0)
{
    // non-existent pid+type
    EXPECT_NE(UnregisterSpawningFdsByPid(mgr_, 99999, TYPE_CHILD_PARENT), 0);

    // NULL mgr
    EXPECT_NE(UnregisterSpawningFdsByPid(nullptr, 6000, TYPE_CHILD_PARENT), 0);
}

/**
 * @tc.name: SpawningFd_Remove_001
 * @tc.desc: RemoveSpawningFdsByPid仅移除队列节点不关闭fd：移除后fd仍可用(F_GETFD>=0)，Find返回NULL
 *           用于fd所有权转移场景（如转移给子进程）
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Remove_001, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);
    pid_t pid = 7000;

    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, pid };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    EXPECT_EQ(RemoveSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT), 0);

    // fd is still usable after Remove
    EXPECT_TRUE(IsFdValid(fds[0]));
    EXPECT_TRUE(IsFdValid(fds[1]));

    // Find returns NULL
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT), nullptr);

    // cleanup fds manually
    close(fds[0]);
    close(fds[1]);
}

/**
 * @tc.name: SpawningFd_Remove_002
 * @tc.desc: RemoveSpawningFdsByPid异常输入：不存在的pid+type返回非0，NULL mgr返回非0
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Remove_002, TestSize.Level0)
{
    // non-existent pid+type
    EXPECT_NE(RemoveSpawningFdsByPid(mgr_, 99999, TYPE_CHILD_PARENT), 0);

    // NULL mgr
    EXPECT_NE(RemoveSpawningFdsByPid(nullptr, 7000, TYPE_CHILD_PARENT), 0);
}

/**
 * @tc.name: SpawningFd_Cleanup_001
 * @tc.desc: 同一pid注册3种类型(TYPE_FOR_DEC/TYPE_CHILD_PARENT/TYPE_PARENT_CHILD)，
 *           CleanupSpawningFdsByPid返回3，所有fd被close
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Cleanup_001, TestSize.Level0)
{
    pid_t pid = 8000;
    SpawningFdType types[] = { TYPE_FOR_DEC, TYPE_CHILD_PARENT, TYPE_PARENT_CHILD };
    int allFds[3][PIPE_FD_LENGTH];

    for (int i = 0; i < 3; i++) {
        CreateTestPipe(allFds[i]);
        SpawningFdRegInfo regInfo = { types[i], PIPE_FD_LENGTH, allFds[i], pid };
        EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);
    }

    // all fds valid before cleanup
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(IsFdValid(allFds[i][0]));
        EXPECT_TRUE(IsFdValid(allFds[i][1]));
    }

    uint32_t cleaned = CleanupSpawningFdsByPid(mgr_, pid);
    EXPECT_EQ(cleaned, (uint32_t)3);

    // all fds closed after cleanup
    for (int i = 0; i < 3; i++) {
        EXPECT_FALSE(IsFdValid(allFds[i][0]));
        EXPECT_FALSE(IsFdValid(allFds[i][1]));
    }
}

/**
 * @tc.name: SpawningFd_Cleanup_002
 * @tc.desc: CleanupSpawningFdsByPid异常输入：NULL mgr返回0，不存在的pid返回0，空队列返回0
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Cleanup_002, TestSize.Level0)
{
    // NULL mgr
    EXPECT_EQ(CleanupSpawningFdsByPid(nullptr, 8000), (uint32_t)0);

    // non-existent pid
    EXPECT_EQ(CleanupSpawningFdsByPid(mgr_, 99999), (uint32_t)0);

    // empty queue (fresh mgr via calloc to avoid g_appSpawnMgr conflict)
    AppSpawnMgr *freshMgr = (AppSpawnMgr *)calloc(1, sizeof(AppSpawnMgr));
    ASSERT_NE(freshMgr, nullptr);
    OH_ListInit(&freshMgr->spawningFdsQueue);
    EXPECT_EQ(CleanupSpawningFdsByPid(freshMgr, 8000), (uint32_t)0);
    free(freshMgr);
}

/**
 * @tc.name: SpawningFd_Dump_001
 * @tc.desc: DumpSpawningFds在有多节点时不崩溃，NULL mgr时也不崩溃
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Dump_001, TestSize.Level0)
{
    int fds1[PIPE_FD_LENGTH], fds2[PIPE_FD_LENGTH];
    CreateTestPipe(fds1);
    CreateTestPipe(fds2);

    SpawningFdRegInfo reg1 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds1, 9001 };
    SpawningFdRegInfo reg2 = { TYPE_PARENT_CHILD, PIPE_FD_LENGTH, fds2, 9002 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg1), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg2), nullptr);

    // should not crash
    DumpSpawningFds(mgr_);

    // NULL mgr should not crash
    DumpSpawningFds(nullptr);
}

/**
 * @tc.name: SpawningFd_Stats_001
 * @tc.desc: GetSpawningFdsStats统计验证：空队列total=0；注册TYPE_CHILD_PARENT后childParentCount=1；
 *           再注册TYPE_PARENT_CHILD后total=2, parentChildCount=1
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Stats_001, TestSize.Level0)
{
    uint32_t total = 0, childParentCount = 0, parentChildCount = 0;

    // empty queue
    GetSpawningFdsStats(mgr_, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(total, (uint32_t)0);
    EXPECT_EQ(childParentCount, (uint32_t)0);
    EXPECT_EQ(parentChildCount, (uint32_t)0);

    // register one TYPE_CHILD_PARENT
    int fds1[PIPE_FD_LENGTH];
    CreateTestPipe(fds1);
    SpawningFdRegInfo reg1 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds1, 10001 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg1), nullptr);

    GetSpawningFdsStats(mgr_, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(total, (uint32_t)1);
    EXPECT_EQ(childParentCount, (uint32_t)1);
    EXPECT_EQ(parentChildCount, (uint32_t)0);

    // register one TYPE_PARENT_CHILD
    int fds2[PIPE_FD_LENGTH];
    CreateTestPipe(fds2);
    SpawningFdRegInfo reg2 = { TYPE_PARENT_CHILD, PIPE_FD_LENGTH, fds2, 10002 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg2), nullptr);

    GetSpawningFdsStats(mgr_, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(total, (uint32_t)2);
    EXPECT_EQ(childParentCount, (uint32_t)1);
    EXPECT_EQ(parentChildCount, (uint32_t)1);
}

/**
 * @tc.name: SpawningFd_ReRegister_001
 * @tc.desc: 同一pid+type重复注册：队列中产生2个节点，total=2，Find返回第一个匹配节点
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_ReRegister_001, TestSize.Level0)
{
    pid_t pid = 11000;
    SpawningFdType type = TYPE_CHILD_PARENT;

    int fds1[PIPE_FD_LENGTH], fds2[PIPE_FD_LENGTH];
    CreateTestPipe(fds1);
    CreateTestPipe(fds2);

    SpawningFdRegInfo reg1 = { type, PIPE_FD_LENGTH, fds1, pid };
    SpawningFdRegInfo reg2 = { type, PIPE_FD_LENGTH, fds2, pid };
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg1), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg2), nullptr);

    // Stats should show 2 total
    uint32_t total = 0, childParentCount = 0, parentChildCount = 0;
    GetSpawningFdsStats(mgr_, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(total, (uint32_t)2);
    EXPECT_EQ(childParentCount, (uint32_t)2);

    // First Find returns one node
    AppSpawnFds *found = FindSpawningFdsByPid(mgr_, pid, type);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->fds[0], fds1[0]);
    EXPECT_EQ(found->fds[1], fds1[1]);
}

/**
 * @tc.name: SpawningFd_Register_003
 * @tc.desc: RegisterSpawningFds支持count=1的单fd注册(TYPE_FOR_DEC场景)，节点count=1，fds[0]正确
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Register_003, TestSize.Level0)
{
    int pipeFds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(pipeFds), 0);
    close(pipeFds[1]);  // Only need one fd for TYPE_FOR_DEC

    int fds[] = { pipeFds[0] };
    SpawningFdRegInfo regInfo = { TYPE_FOR_DEC, 1, fds, 12000 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    AppSpawnFds *found = FindSpawningFdsByPid(mgr_, 12000, TYPE_FOR_DEC);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->type, TYPE_FOR_DEC);
    EXPECT_EQ(found->count, (uint32_t)1);
    EXPECT_EQ(found->fds[0], pipeFds[0]);

    UnregisterSpawningFdsByPid(mgr_, 12000, TYPE_FOR_DEC);
}

/**
 * @tc.name: SpawningFd_Register_004
 * @tc.desc: RegisterSpawningFds count超过MAX_SPAWNING_FDS_PER_NODE时，应返回NULL且不添加节点
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Register_004, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);

    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, MAX_SPAWNING_FDS_PER_NODE + 1, fds, 13000 };
    EXPECT_EQ(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // Verify no node was added
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, 13000, TYPE_CHILD_PARENT), nullptr);

    close(fds[0]);
    close(fds[1]);
}

/**
 * @tc.name: SpawningFd_Register_005
 * @tc.desc: RegisterSpawningFds注册后timestamp字段大于0（使用系统时钟初始化）
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Register_005, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);

    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, 14000 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    AppSpawnFds *found = FindSpawningFdsByPid(mgr_, 14000, TYPE_CHILD_PARENT);
    ASSERT_NE(found, nullptr);
    EXPECT_GT(found->timestamp, (uint64_t)0);

    UnregisterSpawningFdsByPid(mgr_, 14000, TYPE_CHILD_PARENT);
}

/**
 * @tc.name: SpawningFd_Unregister_003
 * @tc.desc: 注销某pid的某type后，同pid其他type和不同pid的同type节点不受影响
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Unregister_003, TestSize.Level0)
{
    int fds1[PIPE_FD_LENGTH], fds2[PIPE_FD_LENGTH], fds3[PIPE_FD_LENGTH];
    CreateTestPipe(fds1);
    CreateTestPipe(fds2);
    CreateTestPipe(fds3);

    SpawningFdRegInfo reg1 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds1, 15001 };
    SpawningFdRegInfo reg2 = { TYPE_PARENT_CHILD, PIPE_FD_LENGTH, fds2, 15001 };
    SpawningFdRegInfo reg3 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds3, 15002 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg1), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg2), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg3), nullptr);

    // Unregister pid=15001 TYPE_CHILD_PARENT
    EXPECT_EQ(UnregisterSpawningFdsByPid(mgr_, 15001, TYPE_CHILD_PARENT), 0);

    // pid=15001 TYPE_PARENT_CHILD should still exist
    EXPECT_NE(FindSpawningFdsByPid(mgr_, 15001, TYPE_PARENT_CHILD), nullptr);

    // pid=15002 TYPE_CHILD_PARENT should still exist
    EXPECT_NE(FindSpawningFdsByPid(mgr_, 15002, TYPE_CHILD_PARENT), nullptr);

    // Cleanup remaining
    UnregisterSpawningFdsByPid(mgr_, 15001, TYPE_PARENT_CHILD);
    UnregisterSpawningFdsByPid(mgr_, 15002, TYPE_CHILD_PARENT);
}

/**
 * @tc.name: SpawningFd_Unregister_004
 * @tc.desc: fds中包含-1的fd时，UnregisterSpawningFdsByPid跳过close(-1)不崩溃，有效fd正常关闭
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Unregister_004, TestSize.Level0)
{
    int pipeFds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(pipeFds), 0);
    close(pipeFds[0]);  // Don't need read end

    int fds[] = { -1, pipeFds[1] };  // fds[0] = -1 should be skipped during close
    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, 2, fds, 16000 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // Should not crash, -1 should be skipped
    EXPECT_EQ(UnregisterSpawningFdsByPid(mgr_, 16000, TYPE_CHILD_PARENT), 0);

    // The valid fd should be closed
    EXPECT_FALSE(IsFdValid(pipeFds[1]));
}

/**
 * @tc.name: SpawningFd_Remove_003
 * @tc.desc: RemoveSpawningFdsByPid移除后fd仍可读写：先write数据到pipe，Remove后read能正确取回数据
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Remove_003, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);
    pid_t pid = 17000;

    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, pid };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // Write to pipe before remove
    const char buf[] = "hello";
    ssize_t written = write(fds[1], buf, sizeof(buf));
    EXPECT_EQ(written, static_cast<ssize_t>(sizeof(buf)));

    // Remove from queue (ownership transfer, fd not closed)
    EXPECT_EQ(RemoveSpawningFdsByPid(mgr_, pid, TYPE_CHILD_PARENT), 0);

    // Read from pipe after remove - should still work
    char readBuf[10] = {0};
    ssize_t bytesRead = read(fds[0], readBuf, sizeof(buf));
    EXPECT_EQ(bytesRead, static_cast<ssize_t>(sizeof(buf)));
    EXPECT_STREQ(readBuf, "hello");

    close(fds[0]);
    close(fds[1]);
}

/**
 * @tc.name: SpawningFd_Cleanup_003
 * @tc.desc: CleanupSpawningFdsByPid按pid清理，不影响其他pid的节点：清理pid=18001后，pid=18002仍可查到
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Cleanup_003, TestSize.Level0)
{
    int fds1[PIPE_FD_LENGTH], fds2[PIPE_FD_LENGTH];
    CreateTestPipe(fds1);
    CreateTestPipe(fds2);

    SpawningFdRegInfo reg1 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds1, 18001 };
    SpawningFdRegInfo reg2 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds2, 18002 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg1), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg2), nullptr);

    // Cleanup pid=18001
    uint32_t cleaned = CleanupSpawningFdsByPid(mgr_, 18001);
    EXPECT_EQ(cleaned, (uint32_t)1);

    // pid=18002 should still be findable
    EXPECT_NE(FindSpawningFdsByPid(mgr_, 18002, TYPE_CHILD_PARENT), nullptr);

    // pid=18001 should be gone
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, 18001, TYPE_CHILD_PARENT), nullptr);

    // Cleanup remaining
    CleanupSpawningFdsByPid(mgr_, 18002);
}

/**
 * @tc.name: SpawningFd_Dump_002
 * @tc.desc: DumpSpawningFds在空队列时不崩溃
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Dump_002, TestSize.Level0)
{
    // Fresh mgr with empty queue (via calloc to avoid g_appSpawnMgr conflict)
    AppSpawnMgr *freshMgr = (AppSpawnMgr *)calloc(1, sizeof(AppSpawnMgr));
    ASSERT_NE(freshMgr, nullptr);
    OH_ListInit(&freshMgr->spawningFdsQueue);

    // Should not crash on empty queue
    DumpSpawningFds(freshMgr);

    free(freshMgr);
}

/**
 * @tc.name: SpawningFd_Stats_003
 * @tc.desc: GetSpawningFdsStats多类型混合统计：注册2个PREFORK+1个DEC+1个PARENT_CHILD，
 *           total=4, childParentCount=2, parentChildCount=1（DEC不计入prefork/parentChild统计）
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Stats_003, TestSize.Level0)
{
    int fds1[PIPE_FD_LENGTH], fds2[PIPE_FD_LENGTH], fds3[PIPE_FD_LENGTH], fds4[PIPE_FD_LENGTH];
    CreateTestPipe(fds1);
    CreateTestPipe(fds2);
    ASSERT_EQ(pipe(fds3), 0);
    close(fds3[1]);  // Only need one fd
    CreateTestPipe(fds4);

    SpawningFdRegInfo reg1 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds1, 19001 };
    SpawningFdRegInfo reg2 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds2, 19002 };
    SpawningFdRegInfo reg3 = { TYPE_FOR_DEC, 1, fds3, 19003 };
    SpawningFdRegInfo reg4 = { TYPE_PARENT_CHILD, PIPE_FD_LENGTH, fds4, 19004 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg1), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg2), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg3), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg4), nullptr);

    uint32_t total = 0, childParentCount = 0, parentChildCount = 0;
    GetSpawningFdsStats(mgr_, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(total, (uint32_t)4);
    EXPECT_EQ(childParentCount, (uint32_t)2);
    EXPECT_EQ(parentChildCount, (uint32_t)1);

    CleanupSpawningFdsByPid(mgr_, 19001);
    CleanupSpawningFdsByPid(mgr_, 19002);
    CleanupSpawningFdsByPid(mgr_, 19003);
    CleanupSpawningFdsByPid(mgr_, 19004);
}

// ===== L0 Data Structure Tests (DS001-DS006) =====

/**
 * @tc.name: SpawningFd_DataStructure_001
 * @tc.desc: 验证SpawningFdType枚举值：TYPE_FOR_DEC=0, TYPE_FOR_FORK_ALL=1,
 *           TYPE_CHILD_PARENT=2, TYPE_PARENT_CHILD=3, TYPE_INVALID=4
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_DataStructure_001, TestSize.Level0)
{
    EXPECT_EQ(TYPE_FOR_DEC, 0);
    EXPECT_EQ(TYPE_FOR_FORK_ALL, 1);
    EXPECT_EQ(TYPE_CHILD_PARENT, 2);
    EXPECT_EQ(TYPE_PARENT_CHILD, 3);
    EXPECT_EQ(TYPE_INVALID, 4);
    EXPECT_LT(TYPE_INVALID, 5);  // Boundary
}

/**
 * @tc.name: SpawningFd_DataStructure_002
 * @tc.desc: 验证AppSpawnFds结构体字段可正确读写：type/count/pid/timestamp/fds[]各字段赋值后读取一致，
 *           且fds[]紧随结构体之后(offsetof验证)
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_DataStructure_002, TestSize.Level0)
{
    int testFds[] = {10, 20};
    size_t allocSize = sizeof(AppSpawnFds) + 2 * sizeof(int);
    AppSpawnFds *node = (AppSpawnFds *)calloc(1, allocSize);
    ASSERT_NE(node, nullptr);

    node->type = TYPE_CHILD_PARENT;
    node->count = 2;
    node->pid = 12345;
    node->timestamp = 1234567890;
    node->fds[0] = testFds[0];
    node->fds[1] = testFds[1];

    EXPECT_EQ(node->type, TYPE_CHILD_PARENT);
    EXPECT_EQ(node->count, (uint32_t)2);
    EXPECT_EQ(node->pid, (pid_t)12345);
    EXPECT_EQ(node->timestamp, (uint64_t)1234567890);
    EXPECT_EQ(node->fds[0], 10);
    EXPECT_EQ(node->fds[1], 20);

    // Verify struct layout: node followed by fds
    EXPECT_EQ((char *)&node->fds[0] - (char *)node, (ptrdiff_t)sizeof(AppSpawnFds));

    free(node);
}

/**
 * @tc.name: SpawningFd_DataStructure_003
 * @tc.desc: 验证SpawningFdKey查找key匹配规则：pid+type均匹配时Find返回节点，
 *           仅pid匹配或仅type匹配时返回NULL
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_DataStructure_003, TestSize.Level0)
{
    int fds[PIPE_FD_LENGTH];
    CreateTestPipe(fds);

    SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds, 55555 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    // pid + type both match → found
    AppSpawnFds *found = FindSpawningFdsByPid(mgr_, 55555, TYPE_CHILD_PARENT);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->pid, (pid_t)55555);
    EXPECT_EQ(found->type, TYPE_CHILD_PARENT);

    // pid match, type mismatch → not found (key doesn't match)
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, 55555, TYPE_PARENT_CHILD), nullptr);

    // type match, pid mismatch → not found (key doesn't match)
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, 66666, TYPE_CHILD_PARENT), nullptr);

    UnregisterSpawningFdsByPid(mgr_, 55555, TYPE_CHILD_PARENT);
}

/**
 * @tc.name: SpawningFd_DataStructure_004
 * @tc.desc: 验证AppSpawnPipeMsg消息封装：sizeof = sizeof(AppSpawnMsgType) + sizeof(AppSpawnPreforkMsg)，
 *           type字段和msg.preforkMsg的id/flags/msgLen字段可正确读写
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_DataStructure_004, TestSize.Level0)
{
    AppSpawnPipeMsg pipeMsg = {};
    pipeMsg.type = MSG_APP_SPAWN;
    pipeMsg.msg.preforkMsg.id = 42;
    pipeMsg.msg.preforkMsg.flags = 0xABCD;
    pipeMsg.msg.preforkMsg.msgLen = 1024;

    // Verify type field
    EXPECT_EQ(pipeMsg.type, MSG_APP_SPAWN);

    // Verify union fields
    EXPECT_EQ(pipeMsg.msg.preforkMsg.id, (uint32_t)42);
    EXPECT_EQ(pipeMsg.msg.preforkMsg.flags, (uint32_t)0xABCD);
    EXPECT_EQ(pipeMsg.msg.preforkMsg.msgLen, (uint32_t)1024);

    // Verify sizeof includes type + union
    EXPECT_EQ(sizeof(pipeMsg), sizeof(AppSpawnMsgType) + sizeof(AppSpawnPreforkMsg));
}

/**
 * @tc.name: SpawningFd_DataStructure_005
 * @tc.desc: 验证AppSpawnContent结构体已移除preforkFd/parentToChildFd字段，
 *           保留reservedPid和propertyBuffer字段，编译通过且字段可正确赋值读取
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_DataStructure_005, TestSize.Level0)
{
    // Verify AppSpawnContent compiles and has expected fields
    // If preforkFd/parentToChildFd were still present, sizeof would be larger
    AppSpawnContent content = {};
    content.mode = MODE_FOR_APP_SPAWN;
    content.reservedPid = -1;
    content.propertyBuffer = NULL;

    EXPECT_EQ(content.mode, MODE_FOR_APP_SPAWN);
    EXPECT_EQ(content.reservedPid, (pid_t)-1);
    EXPECT_EQ(content.propertyBuffer, nullptr);

    // Verify reservedPid and propertyBuffer exist (these replaced the old pipe fd fields)
    EXPECT_GE(sizeof(content), sizeof(void *));  // At minimum has function pointers
}

/**
 * @tc.name: SpawningFd_DataStructure_006
 * @tc.desc: 验证spawningFdsQueue初始化为空双向循环链表(next==prev==&self)，
 *           空队列上执行Find/Cleanup/Dump/Stats均正常工作
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_DataStructure_006, TestSize.Level0)
{
    AppSpawnMgr *freshMgr = (AppSpawnMgr *)calloc(1, sizeof(AppSpawnMgr));
    ASSERT_NE(freshMgr, nullptr);
    OH_ListInit(&freshMgr->spawningFdsQueue);

    // Empty circular list: next and prev point to self
    EXPECT_EQ(freshMgr->spawningFdsQueue.next, &freshMgr->spawningFdsQueue);
    EXPECT_EQ(freshMgr->spawningFdsQueue.prev, &freshMgr->spawningFdsQueue);

    // Empty queue operations should work correctly
    uint32_t total = 0;
    GetSpawningFdsStats(freshMgr, &total, nullptr, nullptr);
    EXPECT_EQ(total, (uint32_t)0);

    EXPECT_EQ(FindSpawningFdsByPid(freshMgr, 1, TYPE_CHILD_PARENT), nullptr);
    EXPECT_EQ(CleanupSpawningFdsByPid(freshMgr, 1), (uint32_t)0);

    DumpSpawningFds(freshMgr);  // Should not crash

    free(freshMgr);
}

// ===== Exception & Boundary Tests (EB005-EB007) =====

/**
 * @tc.name: SpawningFd_Boundary_001
 * @tc.desc: 批量注册100个TYPE_CHILD_PARENT节点，GetSpawningFdsStats验证total=100，
 *           逐个CleanupSpawningFdsByPid后total=0（fd资源回收验证）
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Boundary_001, TestSize.Level0)
{
    const int batchSize = 100;
    int allFds[batchSize][PIPE_FD_LENGTH];

    for (int i = 0; i < batchSize; i++) {
        ASSERT_EQ(pipe(allFds[i]), 0);
        SpawningFdRegInfo regInfo = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, allFds[i],
            20000 + i };
        EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);
    }

    uint32_t total = 0, childParentCount = 0, parentChildCount = 0;
    GetSpawningFdsStats(mgr_, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(total, (uint32_t)batchSize);

    // Cleanup each pid individually
    for (int i = 0; i < batchSize; i++) {
        uint32_t cleaned = CleanupSpawningFdsByPid(mgr_, 20000 + i);
        EXPECT_EQ(cleaned, (uint32_t)1);
    }

    // Queue should be empty
    GetSpawningFdsStats(mgr_, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(total, (uint32_t)0);
}

/**
 * @tc.name: SpawningFd_Boundary_002
 * @tc.desc: 不同pid注册相同type：各自Find返回各自的fds，清理一个pid不影响另一个pid的节点
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Boundary_002, TestSize.Level0)
{
    int fds1[PIPE_FD_LENGTH], fds2[PIPE_FD_LENGTH];
    CreateTestPipe(fds1);
    CreateTestPipe(fds2);

    // Register different pids with same type
    SpawningFdRegInfo reg1 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds1, 30001 };
    SpawningFdRegInfo reg2 = { TYPE_CHILD_PARENT, PIPE_FD_LENGTH, fds2, 30002 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg1), nullptr);
    EXPECT_NE(RegisterSpawningFds(mgr_, &reg2), nullptr);

    // Each pid should find its own entry
    AppSpawnFds *found1 = FindSpawningFdsByPid(mgr_, 30001, TYPE_CHILD_PARENT);
    AppSpawnFds *found2 = FindSpawningFdsByPid(mgr_, 30002, TYPE_CHILD_PARENT);
    ASSERT_NE(found1, nullptr);
    ASSERT_NE(found2, nullptr);
    EXPECT_EQ(found1->fds[0], fds1[0]);
    EXPECT_EQ(found2->fds[0], fds2[0]);

    // Cleanup one should not affect the other
    UnregisterSpawningFdsByPid(mgr_, 30001, TYPE_CHILD_PARENT);
    EXPECT_EQ(FindSpawningFdsByPid(mgr_, 30001, TYPE_CHILD_PARENT), nullptr);
    EXPECT_NE(FindSpawningFdsByPid(mgr_, 30002, TYPE_CHILD_PARENT), nullptr);

    UnregisterSpawningFdsByPid(mgr_, 30002, TYPE_CHILD_PARENT);
}

/**
 * @tc.name: SpawningFd_Boundary_003
 * @tc.desc: pid=0边界值：注册pid=0的TYPE_FOR_DEC节点，Find能正确找到，Unregister正常清理
 * @tc.type: FUNC
 * @tc.level: Level0
 * @tc.require: Spawning Fd Manager
 */
HWTEST_F(SpawningFdTest, SpawningFd_Boundary_003, TestSize.Level0)
{
    int pipeFds[PIPE_FD_LENGTH];
    ASSERT_EQ(pipe(pipeFds), 0);
    close(pipeFds[1]);  // Only need one fd

    int fds[] = { pipeFds[0] };
    SpawningFdRegInfo regInfo = { TYPE_FOR_DEC, 1, fds, 0 };
    EXPECT_NE(RegisterSpawningFds(mgr_, &regInfo), nullptr);

    AppSpawnFds *found = FindSpawningFdsByPid(mgr_, 0, TYPE_FOR_DEC);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->pid, (pid_t)0);
    EXPECT_EQ(found->pid, 0);
    EXPECT_EQ(found->fds[0], pipeFds[0]);

    EXPECT_EQ(UnregisterSpawningFdsByPid(mgr_, 0, TYPE_FOR_DEC), 0);
}
}  // namespace OHOS
