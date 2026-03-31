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

/**
 * @file app_spawn_img_info_test.cpp
 * @brief 镜像信息管理测试（优化版）
 *
 * 测试范围：
 * 1. GetSpawningFd 函数测试
 * 2. AddSpawningFds 函数测试
 * 3. AddSpawningCheckPointInfo 函数测试
 * 4. GetSpawningImgInfoByName 函数测试
 * 5. TraversalImgProcess 函数测试
 * 6. SpawnDestoryImgQue 函数测试
 * 7. 镜像进程生命周期管理测试
 */

#include <cerrno>
#include <cstring>
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <vector>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn_service.h"
#include "lib_wrapper.h"
#include "checkpoint_test_helper.h"
#include "securec.h"

#ifdef __cplusplus
extern "C" {
#endif

// 声明源码中使用 APPSPAWN_STATIC 修饰的函数
APPSPAWN_STATIC AppSpawnFds *GetSpawningFd(AppSpawnMgr *content, SpawningFdType fdType);
APPSPAWN_STATIC int32_t AddSpawningFds(AppSpawnMgr *content, SpawningFdType type, uint32_t count, int *fds);
APPSPAWN_STATIC AppSpawnedCheckPointProcesses *AddSpawningCheckPointInfo(
    AppSpawnMgr *content, uint64_t checkpointId, const char *processName, uint32_t appIndex, uint32_t uid);
APPSPAWN_STATIC AppSpawnedCheckPointProcesses *GetSpawningImgInfoByName(AppSpawnMgr *content, const char *name);

typedef void (*CheckPointProcessTraversal)(AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *checkPointInfo, void *data);
APPSPAWN_STATIC void TraversalImgProcess(CheckPointProcessTraversal traversal, AppSpawnMgr *content, void *data);

APPSPAWN_STATIC int SpawnDestoryImgQue(AppSpawnMgr *content);

#ifdef __cplusplus
}
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {

class AppSpawnImgInfoTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// ============================================================================
// GetSpawningFd 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_GetSpawningFd_001
 * @tc.desc: Test GetSpawningFd with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetSpawningFd_001, TestSize.Level0)
{
    AppSpawnFds *result = GetSpawningFd(nullptr, TYPE_FOR_FORK_ALL);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: App_ImgInfo_GetSpawningFd_002
 * @tc.desc: Test GetSpawningFd with valid parameters but no fd added
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetSpawningFd_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnFds *result = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    EXPECT_EQ(result, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_GetSpawningFd_003
 * @tc.desc: Test GetSpawningFd after adding fd
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetSpawningFd_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int fds[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_EQ(ret, 0);

    AppSpawnFds *result = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->type, TYPE_FOR_FORK_ALL);
    EXPECT_EQ(result->count, 1);
    EXPECT_EQ(result->fds[0], 100);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// AddSpawningFds 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_AddSpawningFds_001
 * @tc.desc: Test AddSpawningFds with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_AddSpawningFds_001, TestSize.Level0)
{
    int fds[1] = {100};
    int32_t ret = AddSpawningFds(nullptr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_NE(ret, 0);
}

/**
 * @tc.name: App_ImgInfo_AddSpawningFds_002
 * @tc.desc: Test AddSpawningFds with invalid type
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_AddSpawningFds_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int fds[1] = {100};
    int32_t ret = AddSpawningFds(mgr, TYPE_INVALID, 1, fds);
    EXPECT_NE(ret, 0);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_AddSpawningFds_003
 * @tc.desc: Test AddSpawningFds with null fds
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_AddSpawningFds_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int32_t ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, nullptr);
    EXPECT_NE(ret, 0);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_AddSpawningFds_004
 * @tc.desc: Test AddSpawningFds with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_AddSpawningFds_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int fds[2] = {100, 200};
    int32_t ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 2, fds);
    EXPECT_EQ(ret, 0);

    AppSpawnFds *result = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->count, 2);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// AddSpawningCheckPointInfo 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_AddCheckPointInfo_001
 * @tc.desc: Test AddSpawningCheckPointInfo with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_AddCheckPointInfo_001, TestSize.Level0)
{
    AppSpawnedCheckPointProcesses *result = AddSpawningCheckPointInfo(
        nullptr, 1001, "test_process", 0, 1000);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: App_ImgInfo_AddCheckPointInfo_002
 * @tc.desc: Test AddSpawningCheckPointInfo with invalid checkpointId (0)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_AddCheckPointInfo_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *result = AddSpawningCheckPointInfo(
        mgr, 0, "test_process", 0, 1000);
    EXPECT_EQ(result, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_AddCheckPointInfo_003
 * @tc.desc: Test AddSpawningCheckPointInfo with null processName
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_AddCheckPointInfo_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *result = AddSpawningCheckPointInfo(
        mgr, 1001, nullptr, 0, 1000);
    EXPECT_EQ(result, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_AddCheckPointInfo_004
 * @tc.desc: Test AddSpawningCheckPointInfo with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_AddCheckPointInfo_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *result = AddSpawningCheckPointInfo(
        mgr, 1001, "com.test.app", 0, 1000);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->checkPointId, 1001);
    EXPECT_EQ(result->uid, 1000);
    EXPECT_EQ(result->appIndex, 0);
    EXPECT_STREQ(result->name, "com.test.app");

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// GetSpawningImgInfoByName 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_GetByName_001
 * @tc.desc: Test GetSpawningImgInfoByName with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetByName_001, TestSize.Level0)
{
    AppSpawnedCheckPointProcesses *result = GetSpawningImgInfoByName(nullptr, "test");
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: App_ImgInfo_GetByName_002
 * @tc.desc: Test GetSpawningImgInfoByName with null name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetByName_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *result = GetSpawningImgInfoByName(mgr, nullptr);
    EXPECT_EQ(result, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_GetByName_003
 * @tc.desc: Test GetSpawningImgInfoByName with non-existent name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetByName_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *result = GetSpawningImgInfoByName(mgr, "non_existent");
    EXPECT_EQ(result, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_GetByName_004
 * @tc.desc: Test GetSpawningImgInfoByName after adding checkpoint info
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_GetByName_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *added = AddSpawningCheckPointInfo(
        mgr, 1001, "com.test.find", 0, 1000);
    ASSERT_NE(added, nullptr);

    AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, "com.test.find");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->checkPointId, 1001);
    EXPECT_STREQ(found->name, "com.test.find");

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// TraversalImgProcess 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_Traversal_001
 * @tc.desc: Test TraversalImgProcess with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_Traversal_001, TestSize.Level0)
{
    int count = 0;
    auto countFunc = [](AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *info, void *data) {
        (*static_cast<int *>(data))++;
    };
    TraversalImgProcess(countFunc, nullptr, &count);
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: App_ImgInfo_Traversal_002
 * @tc.desc: Test TraversalImgProcess with null traversal function
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_Traversal_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int count = 0;
    TraversalImgProcess(nullptr, mgr, &count);
    EXPECT_EQ(count, 0);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_Traversal_003
 * @tc.desc: Test TraversalImgProcess with multiple checkpoint processes
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_Traversal_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AddSpawningCheckPointInfo(mgr, 1001, "traversal_a", 0, 1000);
    AddSpawningCheckPointInfo(mgr, 1002, "traversal_b", 0, 1001);
    AddSpawningCheckPointInfo(mgr, 1003, "traversal_c", 0, 1002);

    int count = 0;
    auto countFunc = [](AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *info, void *data) {
        (*static_cast<int *>(data))++;
    };
    TraversalImgProcess(countFunc, mgr, &count);
    EXPECT_EQ(count, 3);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// SpawnDestoryImgQue 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_DestroyImgQue_001
 * @tc.desc: Test SpawnDestoryImgQue with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_DestroyImgQue_001, TestSize.Level0)
{
    int result = SpawnDestoryImgQue(nullptr);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: App_ImgInfo_DestroyImgQue_002
 * @tc.desc: Test SpawnDestoryImgQue with wrong mode
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_DestroyImgQue_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int result = SpawnDestoryImgQue(mgr);
    EXPECT_EQ(result, 0);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_DestroyImgQue_003
 * @tc.desc: Test SpawnDestoryImgQue with no fd added
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_DestroyImgQue_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AddSpawningCheckPointInfo(mgr, 1001, "destroy_test1", 0, 1000);

    int result = SpawnDestoryImgQue(mgr);
    EXPECT_EQ(result, 0);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_DestroyImgQue_004
 * @tc.desc: Test SpawnDestoryImgQue with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_DestroyImgQue_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    AddSpawningCheckPointInfo(mgr, 1001, "destroy_test1", 0, 1000);
    AddSpawningCheckPointInfo(mgr, 1002, "destroy_test2", 0, 1001);

    int validFd = 100;
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, &validFd);
    EXPECT_EQ(ret, 0);

    // Mock ioctl to return success
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    int result = SpawnDestoryImgQue(mgr);
    EXPECT_EQ(result, 0);

    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgInfo_DestroyImgQue_005
 * @tc.desc: Test SpawnDestoryImgQue with invalid fd
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_DestroyImgQue_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    AddSpawningCheckPointInfo(mgr, 1001, "destroy_invalid_fd", 0, 1000);

    int invalidFd = -1;
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, &invalidFd);
    EXPECT_EQ(ret, 0);

    int result = SpawnDestoryImgQue(mgr);
    EXPECT_EQ(result, 0);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 镜像进程生命周期管理测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_Lifecycle_001
 * @tc.desc: Test complete lifecycle: add, get, destroy checkpoint processes
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_Lifecycle_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    // Step 1: Add checkpoint processes
    AppSpawnedCheckPointProcesses *node1 = AddSpawningCheckPointInfo(
        mgr, 1001, "lifecycle_test1", 0, 1000);
    ASSERT_NE(node1, nullptr);
    AppSpawnedCheckPointProcesses *node2 = AddSpawningCheckPointInfo(
        mgr, 1002, "lifecycle_test2", 0, 1001);
    ASSERT_NE(node2, nullptr);

    // Step 2: Verify retrieval
    AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, "lifecycle_test1");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->checkPointId, 1001);

    // Step 3: Add fork all fd
    int fds[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds);
    EXPECT_EQ(ret, 0);

    // Step 4: Get fd and verify
    AppSpawnFds *fdInfo = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(fdInfo, nullptr);
    EXPECT_EQ(fdInfo->fds[0], 100);

    // Step 5: Destroy queue
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int { return 0; };
    UpdateIoctlFunc(ioctlFunc);
    SpawnDestoryImgQue(mgr);
    UpdateIoctlFunc(nullptr);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 多进程测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_MultiProcess_001
 * @tc.desc: Test multiple processes with different checkpointIds
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_MultiProcess_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    const int processCount = 10;
    for (int i = 0; i < processCount; i++) {
        char name[64];
        sprintf_s(name, sizeof(name), "multi_process_%02d", i);
        AppSpawnedCheckPointProcesses *node = AddSpawningCheckPointInfo(
            mgr, 10000 + i, name, 0, 2000 + i);
        ASSERT_NE(node, nullptr);
    }

    // Verify all can be found
    for (int i = 0; i < processCount; i++) {
        char name[64];
        sprintf_s(name, sizeof(name), "multi_process_%02d", i);
        AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, name);
        EXPECT_NE(found, nullptr);
    }

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 压力测试
// ============================================================================

/**
 * @tc.name: App_ImgInfo_Stress_001
 * @tc.desc: Stress test with many checkpoint processes
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgInfoTest, App_ImgInfo_Stress_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    const int processCount = 50;
    for (int i = 0; i < processCount; i++) {
        char name[64];
        sprintf_s(name, sizeof(name), "stress_process_%02d", i);
        AppSpawnedCheckPointProcesses *node = AddSpawningCheckPointInfo(
            mgr, 10000 + i, name, 0, 2000 + i);
        ASSERT_NE(node, nullptr);
    }

    int count = 0;
    auto countFunc = [](AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *info, void *data) {
        (*static_cast<int *>(data))++;
    };
    TraversalImgProcess(countFunc, mgr, &count);
    EXPECT_EQ(count, processCount);

    DeleteAppSpawnMgr(mgr);
}

}  // namespace OHOS