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

#include <cerrno>
#include <cstring>
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn_service.h"
#include "lib_wrapper.h"
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

class AppSpawnImgLoaderTest : public testing::Test {
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
 * @tc.name: App_ImgLoader_GetSpawningFd_001
 * @tc.desc: Test GetSpawningFd with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_GetSpawningFd_001, TestSize.Level0)
{
    AppSpawnFds *result = GetSpawningFd(nullptr, TYPE_FOR_FORK_ALL);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name: App_ImgLoader_GetSpawningFd_002
 * @tc.desc: Test GetSpawningFd with valid content but no fd added
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_GetSpawningFd_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnFds *result = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    EXPECT_EQ(result, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_GetSpawningFd_003
 * @tc.desc: Test GetSpawningFd after adding fd
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_GetSpawningFd_003, TestSize.Level0)
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
 * @tc.name: App_ImgLoader_AddSpawningFds_001
 * @tc.desc: Test AddSpawningFds with invalid type
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_AddSpawningFds_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int fds[1] = {123};
    int ret = AddSpawningFds(mgr, TYPE_INVALID, 1, fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_AddSpawningFds_002
 * @tc.desc: Test AddSpawningFds with invalid count (zero)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_AddSpawningFds_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int fds[1] = {123};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 0, fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_AddSpawningFds_003
 * @tc.desc: Test AddSpawningFds with null fds
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_AddSpawningFds_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_AddSpawningFds_004
 * @tc.desc: Test AddSpawningFds with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_AddSpawningFds_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int fds[3] = {100, 200, 300};
    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 3, fds);
    EXPECT_EQ(ret, 0);

    AppSpawnFds *spawnFds = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(spawnFds, nullptr);
    EXPECT_EQ(spawnFds->type, TYPE_FOR_FORK_ALL);
    EXPECT_EQ(spawnFds->count, 3);
    EXPECT_EQ(spawnFds->fds[0], 100);
    EXPECT_EQ(spawnFds->fds[1], 200);
    EXPECT_EQ(spawnFds->fds[2], 300);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// AddSpawningCheckPointInfo 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_AddCheckPointInfo_001
 * @tc.desc: Test AddSpawningCheckPointInfo with invalid checkpointId (0)
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_AddCheckPointInfo_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node = AddSpawningCheckPointInfo(mgr, 0, "test_process", 0, 0);
    EXPECT_EQ(node, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_AddCheckPointInfo_002
 * @tc.desc: Test AddSpawningCheckPointInfo with empty process name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_AddCheckPointInfo_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node = AddSpawningCheckPointInfo(mgr, 1234, "", 0, 0);
    EXPECT_EQ(node, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_AddCheckPointInfo_003
 * @tc.desc: Test AddSpawningCheckPointInfo with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_AddCheckPointInfo_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node = AddSpawningCheckPointInfo(mgr, 1234, "test_process", 1, 1000);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->checkPointId, 1234);
    EXPECT_EQ(strcmp(node->name, "test_process"), 0);
    EXPECT_EQ(node->appIndex, 1);
    EXPECT_EQ(node->uid, 1000);

    // Verify it was added to the queue
    AppSpawnedCheckPointProcesses *foundNode = GetSpawningImgInfoByName(mgr, "test_process");
    ASSERT_NE(foundNode, nullptr);
    EXPECT_EQ(foundNode->checkPointId, 1234);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// GetSpawningImgInfoByName 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_GetByName_001
 * @tc.desc: Test GetSpawningImgInfoByName with null name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_GetByName_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node = GetSpawningImgInfoByName(mgr, nullptr);
    EXPECT_EQ(node, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_GetByName_002
 * @tc.desc: Test GetSpawningImgInfoByName with non-existent name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_GetByName_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnedCheckPointProcesses *node = GetSpawningImgInfoByName(mgr, "non_existent");
    EXPECT_EQ(node, nullptr);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_GetByName_003
 * @tc.desc: Test GetSpawningImgInfoByName with existing name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_GetByName_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Add first
    AppSpawnedCheckPointProcesses *node1 = AddSpawningCheckPointInfo(mgr, 1111, "process1", 0, 1000);
    ASSERT_NE(node1, nullptr);

    AppSpawnedCheckPointProcesses *node2 = AddSpawningCheckPointInfo(mgr, 2222, "process2", 1, 2000);
    ASSERT_NE(node2, nullptr);

    // Find
    AppSpawnedCheckPointProcesses *foundNode = GetSpawningImgInfoByName(mgr, "process1");
    ASSERT_NE(foundNode, nullptr);
    EXPECT_EQ(foundNode->checkPointId, 1111);
    EXPECT_EQ(foundNode->uid, 1000);

    foundNode = GetSpawningImgInfoByName(mgr, "process2");
    ASSERT_NE(foundNode, nullptr);
    EXPECT_EQ(foundNode->checkPointId, 2222);
    EXPECT_EQ(foundNode->uid, 2000);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// TraversalImgProcess 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_Traversal_001
 * @tc.desc: Test TraversalImgProcess with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_Traversal_001, TestSize.Level0)
{
    int count = 0;
    auto countTraversal = [](AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data) {
        int *count = static_cast<int *>(data);
        (*count)++;
    };

    TraversalImgProcess(countTraversal, nullptr, &count);
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: App_ImgLoader_Traversal_002
 * @tc.desc: Test TraversalImgProcess with null traversal function
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_Traversal_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int count = 0;
    TraversalImgProcess(nullptr, mgr, &count);
    EXPECT_EQ(count, 0);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_Traversal_003
 * @tc.desc: Test TraversalImgProcess with multiple checkpoint processes
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_Traversal_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Add multiple img processes
    AppSpawnedCheckPointProcesses *node1 = AddSpawningCheckPointInfo(mgr, 1001, "process_a", 0, 1000);
    ASSERT_NE(node1, nullptr);
    AppSpawnedCheckPointProcesses *node2 = AddSpawningCheckPointInfo(mgr, 1002, "process_b", 1, 1001);
    ASSERT_NE(node2, nullptr);
    AppSpawnedCheckPointProcesses *node3 = AddSpawningCheckPointInfo(mgr, 1003, "process_c", 2, 1002);
    ASSERT_NE(node3, nullptr);

    // Count processes during traversal
    int count = 0;
    auto countTraversal = [](AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data) {
        int *count = static_cast<int *>(data);
        (*count)++;
    };

    TraversalImgProcess(countTraversal, mgr, &count);
    EXPECT_EQ(count, 3);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// SpawnDestoryImgQue 函数测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_DestroyImgQue_001
 * @tc.desc: Test SpawnDestoryImgQue with null content
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_DestroyImgQue_001, TestSize.Level0)
{
    int result = SpawnDestoryImgQue(nullptr);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: App_ImgLoader_DestroyImgQue_002
 * @tc.desc: Test SpawnDestoryImgQue with wrong mode
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_DestroyImgQue_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int result = SpawnDestoryImgQue(mgr);
    EXPECT_EQ(result, 0);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_DestroyImgQue_003
 * @tc.desc: Test SpawnDestoryImgQue with no fd added
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_DestroyImgQue_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AddSpawningCheckPointInfo(mgr, 1001, "destroy_test1", 0, 1000);

    int result = SpawnDestoryImgQue(mgr);
    EXPECT_EQ(result, 0);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: App_ImgLoader_DestroyImgQue_004
 * @tc.desc: Test SpawnDestoryImgQue with valid parameters
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_DestroyImgQue_004, TestSize.Level0)
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
 * @tc.name: App_ImgLoader_DestroyImgQue_005
 * @tc.desc: Test SpawnDestoryImgQue with invalid fd
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_DestroyImgQue_005, TestSize.Level0)
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
// 多进程测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_MultiProcess_001
 * @tc.desc: Test multiple processes with different checkpointIds
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_MultiProcess_001, TestSize.Level0)
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
// 多 fd 类型测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_MultiFdType_001
 * @tc.desc: Test multiple fd types in the queue
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_MultiFdType_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int fds1[1] = {100};
    int ret = AddSpawningFds(mgr, TYPE_FOR_DEC, 1, fds1);
    EXPECT_EQ(ret, 0);

    int fds2[1] = {200};
    ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, 1, fds2);
    EXPECT_EQ(ret, 0);

    // Get DEC fd
    AppSpawnFds *decFds = GetSpawningFd(mgr, TYPE_FOR_DEC);
    ASSERT_NE(decFds, nullptr);
    EXPECT_EQ(decFds->fds[0], 100);

    // Get FORK_ALL fd
    AppSpawnFds *forkAllFds = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(forkAllFds, nullptr);
    EXPECT_EQ(forkAllFds->fds[0], 200);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 大量 fd 测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_LargeFdCount_001
 * @tc.desc: Test large number of fds
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_LargeFdCount_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    const int fdCount = 10;
    int fds[fdCount];
    for (int i = 0; i < fdCount; i++) {
        fds[i] = 1000 + i;
    }

    int ret = AddSpawningFds(mgr, TYPE_FOR_FORK_ALL, fdCount, fds);
    EXPECT_EQ(ret, 0);

    AppSpawnFds *spawnFds = GetSpawningFd(mgr, TYPE_FOR_FORK_ALL);
    ASSERT_NE(spawnFds, nullptr);
    EXPECT_EQ(spawnFds->count, fdCount);
    for (int i = 0; i < fdCount; i++) {
        EXPECT_EQ(spawnFds->fds[i], 1000 + i);
    }

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 长进程名测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_LongProcessName_001
 * @tc.desc: Test long process name handling
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_LongProcessName_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Create a long process name
    std::string longName = "very_long_test_process_name_that_exceeds_standard_length";
    AppSpawnedCheckPointProcesses *node = AddSpawningCheckPointInfo(mgr, 9999, longName.c_str(), 5, 3000);
    ASSERT_NE(node, nullptr);

    AppSpawnedCheckPointProcesses *foundNode = GetSpawningImgInfoByName(mgr, longName.c_str());
    ASSERT_NE(foundNode, nullptr);
    EXPECT_EQ(foundNode->checkPointId, 9999);
    EXPECT_EQ(foundNode->appIndex, 5);
    EXPECT_EQ(foundNode->uid, 3000);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 生命周期测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_Lifecycle_001
 * @tc.desc: Test complete lifecycle: add, get, destroy checkpoint processes
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_Lifecycle_001, TestSize.Level0)
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
// 压力测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_Stress_001
 * @tc.desc: Stress test with many checkpoint processes
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_Stress_001, TestSize.Level1)
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

// ============================================================================
// 大数值 checkpointId 测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_LargeCheckpointId_001
 * @tc.desc: Test with very large checkpointId
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_LargeCheckpointId_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Test with maximum uint64_t value
    uint64_t maxCheckpointId = 0xFFFFFFFFFFFFFFFF;
    AppSpawnedCheckPointProcesses *node = AddSpawningCheckPointInfo(mgr, maxCheckpointId, "max_cp_test", 0, 5000);
    ASSERT_NE(node, nullptr);
    EXPECT_EQ(node->checkPointId, maxCheckpointId);

    // Verify can be retrieved
    AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, "max_cp_test");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->checkPointId, maxCheckpointId);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 特殊字符进程名测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_SpecialCharProcessName_001
 * @tc.desc: Test img info with special characters in process name
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_SpecialCharProcessName_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Test process names with numbers and underscore (valid)
    const char *validNames[] = {"process_123", "app_test_001", "com_example_app"};

    for (size_t i = 0; i < sizeof(validNames) / sizeof(validNames[0]); i++) {
        AppSpawnedCheckPointProcesses *node = AddSpawningCheckPointInfo(mgr, 2000 + i, validNames[i], 0, 1000 + i);
        ASSERT_NE(node, nullptr) << "Failed to add process: " << validNames[i];

        AppSpawnedCheckPointProcesses *found = GetSpawningImgInfoByName(mgr, validNames[i]);
        ASSERT_NE(found, nullptr);
        EXPECT_EQ(found->checkPointId, 2000 + i);
    }

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 排序测试
// ============================================================================

/**
 * @tc.name: App_ImgLoader_SortedOrder_001
 * @tc.desc: Test img process queue maintains sorted order
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnImgLoaderTest, App_ImgLoader_SortedOrder_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // Add in non-sequential order
    AppSpawnedCheckPointProcesses *node3 = AddSpawningCheckPointInfo(mgr, 3000, "process_3000", 0, 3000);
    ASSERT_NE(node3, nullptr);

    AppSpawnedCheckPointProcesses *node1 = AddSpawningCheckPointInfo(mgr, 1000, "process_1000", 0, 1000);
    ASSERT_NE(node1, nullptr);

    AppSpawnedCheckPointProcesses *node2 = AddSpawningCheckPointInfo(mgr, 2000, "process_2000", 0, 2000);
    ASSERT_NE(node2, nullptr);

    // Verify sorted order by traversal
    std::vector<uint64_t> observed;
    auto collectIds = [](AppSpawnMgr *mgr, AppSpawnedCheckPointProcesses *imgInfo, void *data) {
        std::vector<uint64_t> *ids = static_cast<std::vector<uint64_t> *>(data);
        ids->push_back(imgInfo->checkPointId);
    };
    std::vector<uint64_t> checkpointIds;
    TraversalImgProcess(collectIds, mgr, &checkpointIds);

    // Should be sorted: 1000, 2000, 3000
    ASSERT_EQ(checkpointIds.size(), 3);
    EXPECT_EQ(checkpointIds[0], 1000);
    EXPECT_EQ(checkpointIds[1], 2000);
    EXPECT_EQ(checkpointIds[2], 3000);

    DeleteAppSpawnMgr(mgr);
}

}  // namespace OHOS
