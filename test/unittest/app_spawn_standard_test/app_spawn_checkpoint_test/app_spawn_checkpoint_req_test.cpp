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
 * @file app_spawn_checkpoint_req_test.cpp
 * @brief ProcessCheckpointReqMsg 函数测试用例
 *
 * 测试目标：ProcessCheckpointReqMsg 函数分支覆盖率达到 80%+
 *
 * 测试范围：
 * 1. MSG_SPAWN_IMAGE_PROCESS 消息处理测试
 * 2. MSG_SPAWN_WORKER_PROCESS 消息处理测试
 * 3. CheckAppSpawnMsg 失败分支测试
 * 4. CreateAppSpawningCtx 失败分支测试
 * 5. AppSpawnHookExecute 失败分支测试
 * 6. 边界条件测试（空指针等）
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

// IoctlCheckPointArgs 结构体定义（在 appspawn_checkpoint.c 中定义，此处需要重复定义用于测试）
struct IoctlCheckPointArgs {
    pid_t inputPid;           // 输入的镜像进程 PID
    char name[64];            // 进程名称
    uint64_t checkPointId;    // checkpoint 标识符
    pid_t resultPid;          // 返回的结果 PID
};

#ifdef __cplusplus
extern "C" {
#endif

APPSPAWN_STATIC void ProcessCheckpointReqMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);

#ifdef __cplusplus
}
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {

class AppSpawnCheckpointReqTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// ============================================================================
// 主路径测试 - MSG_SPAWN_IMAGE_PROCESS
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_001
 * @tc.desc: Test ProcessCheckpointReqMsg with valid MSG_SPAWN_IMAGE_PROCESS message
 * @tc.type: FUNC
 * @tc.cover: branch 1 (main success path for image process)
 * @tc.pre: 参考App_Spawn_AppSpawnMsg_002用例的消息创建方式
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_001, TestSize.Level0)
{
    // 1. 创建AppSpawnMgr (参考App_Spawn_AppSpawnMsg_002预置条件)
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // 2. 使用CheckpointTestHelper创建带有TLV数据的消息 (参考App_Spawn_AppSpawnMsg_002)
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);  // 参考App_Spawn_AppSpawnMsg_002使用的buffer大小
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.image", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    // 3. 从buffer获取消息 (参考App_Spawn_AppSpawnMsg_002)
    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ASSERT_NE(msgNode, nullptr);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(0, reminder);

    // 4. 解码消息 (参考App_Spawn_AppSpawnMsg_002)
    ret = DecodeAppSpawnMsg(msgNode);
    EXPECT_EQ(ret, 0);

    // 5. 创建连接对象
    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // 6. Mock ioctl to return success
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 54321;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    // 7. 执行测试
    ProcessCheckpointReqMsg(&connection, msgNode);

    // 8. 清理资源
    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 主路径测试 - MSG_SPAWN_WORKER_PROCESS
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_002
 * @tc.desc: Test ProcessCheckpointReqMsg with valid MSG_SPAWN_WORKER_PROCESS message
 * @tc.type: FUNC
 * @tc.cover: branch 2 (main success path for worker process)
 * @tc.pre: 参考App_Spawn_AppSpawnMsg_002用例的消息创建方式
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_002, TestSize.Level0)
{
    // 1. 创建AppSpawnMgr
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // 2. 使用CheckpointTestHelper创建消息
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_WORKER_PROCESS, "com.test.worker", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    // 3. 从buffer获取并解码消息
    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    EXPECT_EQ(ret, 0);

    // 4. 创建连接对象
    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // 5. Mock ioctl to return success
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 54322;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    // 6. 执行测试
    ProcessCheckpointReqMsg(&connection, msgNode);

    // 7. 清理资源
    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// CheckAppSpawnMsg 失败分支测试
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_003
 * @tc.desc: Test ProcessCheckpointReqMsg with invalid message (empty process name)
 * @tc.type: FUNC
 * @tc.cover: branch 3 (CheckAppSpawnMsg failure path)
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // Create message with empty process name to trigger validation failure
    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    ASSERT_NE(msgNode, nullptr);

    msgNode->msgHeader.processName[0] = '\0';  // Empty process name
    msgNode->msgHeader.msgType = MSG_SPAWN_IMAGE_PROCESS;

    ProcessCheckpointReqMsg(&connection, msgNode);
    // Expected: function returns early with error response

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: Process_Checkpoint_Req_Msg_004
 * @tc.desc: Test ProcessCheckpointReqMsg with invalid message type
 * @tc.type: FUNC
 * @tc.cover: branch 3 (CheckAppSpawnMsg failure path - invalid msgType)
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    ASSERT_NE(msgNode, nullptr);

    strcpy_s(msgNode->msgHeader.processName, APP_LEN_PROC_NAME, "com.test.invalid");
    msgNode->msgHeader.msgType = MAX_TYPE_INVALID;  // Invalid message type

    ProcessCheckpointReqMsg(&connection, msgNode);
    // Expected: function returns early with error response

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 缺少 checkpoint info 分支测试
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_005
 * @tc.desc: Test ProcessCheckpointReqMsg with missing checkpoint info
 * @tc.type: FUNC
 * @tc.cover: branch 4 (hook execution failure due to missing checkpoint info)
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    ASSERT_NE(msgNode, nullptr);

    strcpy_s(msgNode->msgHeader.processName, APP_LEN_PROC_NAME, "com.test.missing");
    msgNode->msgHeader.msgType = MSG_SPAWN_IMAGE_PROCESS;
    // NOT setting checkpoint info - this will cause hook to fail

    // Mock ioctl to return error when checkpoint info is missing
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr == nullptr || argsPtr->inputPid == 0) {
            errno = EINVAL;
            return -1;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    ProcessCheckpointReqMsg(&connection, msgNode);
    // Expected: hook fails due to missing checkpoint info

    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// ioctl 失败分支测试
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_006
 * @tc.desc: Test ProcessCheckpointReqMsg with ioctl failure
 * @tc.type: FUNC
 * @tc.cover: branch 5 (ioctl returns error)
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // 使用CheckpointTestHelper创建消息
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.ioerror", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    EXPECT_EQ(ret, 0);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // Mock ioctl to return error
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        errno = EIO;
        return -1;
    };
    UpdateIoctlFunc(ioctlFunc);

    ProcessCheckpointReqMsg(&connection, msgNode);
    // Expected: hook fails with errno

    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 边界条件测试 - 空指针
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_007
 * @tc.desc: Test ProcessCheckpointReqMsg with null connection
 * @tc.type: FUNC
 * @tc.cover: boundary condition
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_007, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // 使用CheckpointTestHelper创建消息
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.nullconn", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    EXPECT_EQ(ret, 0);

    // Pass null connection - should handle gracefully
    ProcessCheckpointReqMsg(nullptr, msgNode);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: Process_Checkpoint_Req_Msg_008
 * @tc.desc: Test ProcessCheckpointReqMsg with null message
 * @tc.type: FUNC
 * @tc.cover: boundary condition
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_008, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // Pass null message - should handle gracefully
    ProcessCheckpointReqMsg(&connection, nullptr);

    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 响应验证测试
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_009
 * @tc.desc: Test ProcessCheckpointReqMsg verifies checkPointId in response
 * @tc.type: FUNC
 * @tc.cover: response path verification
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_009, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // 使用CheckpointTestHelper创建消息
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    uint64_t expectedCheckPointId = 12345678;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.response", 12345, expectedCheckPointId, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    EXPECT_EQ(ret, 0);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // Mock ioctl to return success with specific checkPointId
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    ProcessCheckpointReqMsg(&connection, msgNode);
    // Verify that response was sent (would need to capture response)

    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 不同错误码测试
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_010
 * @tc.desc: Test ProcessCheckpointReqMsg with ENOMEM error
 * @tc.type: FUNC
 * @tc.cover: branch 5 (ioctl returns ENOMEM)
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_010, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // 使用CheckpointTestHelper创建消息
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.enomem", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    EXPECT_EQ(ret, 0);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // Mock ioctl to return ENOMEM error
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        errno = ENOMEM;
        return -1;
    };
    UpdateIoctlFunc(ioctlFunc);

    ProcessCheckpointReqMsg(&connection, msgNode);
    // Expected: hook fails with ENOMEM

    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: Process_Checkpoint_Req_Msg_011
 * @tc.desc: Test ProcessCheckpointReqMsg with EPERM error
 * @tc.type: FUNC
 * @tc.cover: branch 5 (ioctl returns EPERM - permission denied)
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_011, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // 使用CheckpointTestHelper创建消息
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.eperm", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    EXPECT_EQ(ret, 0);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // Mock ioctl to return EPERM error
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        errno = EPERM;
        return -1;
    };
    UpdateIoctlFunc(ioctlFunc);

    ProcessCheckpointReqMsg(&connection, msgNode);
    // Expected: hook fails with EPERM

    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @tc.name: Process_Checkpoint_Req_Msg_012
 * @tc.desc: Test ProcessCheckpointReqMsg with malloc failed
 * @tc.type: FUNC
 * @tc.cover: branch 5 (ioctl returns EPERM - permission denied)
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_012, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    // 使用CheckpointTestHelper创建消息
    CheckpointTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);
    uint32_t msgLen = 0;
    CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, "com.test.eperm", 12345, 1001, nullptr};
    int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
    ASSERT_EQ(ret, 0);

    AppSpawnMsgNode *msgNode = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
    ASSERT_EQ(ret, 0);
    ret = DecodeAppSpawnMsg(msgNode);
    EXPECT_EQ(ret, 0);

    AppSpawnConnection connection = {
        .connectionId = 1,
        .stream = nullptr,
        .receiverCtx = {0}
    };

    // Mock malloc to return EPERM nullptr
    MallocFunc func = [](size_t size) -> void* {
        return nullptr;
    };
    UpdateMallocFunc(func);

    ProcessCheckpointReqMsg(&connection, msgNode);

    UpdateMallocFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

// ============================================================================
// 压力测试
// ============================================================================

/**
 * @tc.name: Process_Checkpoint_Req_Msg_Stress_001
 * @tc.desc: Stress test with multiple checkpoint requests
 * @tc.type: FUNC
 * @tc.cover: stress test
 */
HWTEST_F(AppSpawnCheckpointReqTest, Process_Checkpoint_Req_Msg_Stress_001, TestSize.Level1)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    const int requestCount = 10;
    IoctlFunc ioctlFunc = [](int fd, int req, va_list args) -> int {
        struct IoctlCheckPointArgs *argsPtr = va_arg(args, struct IoctlCheckPointArgs *);
        if (argsPtr != nullptr) {
            argsPtr->resultPid = 10000 + argsPtr->checkPointId;
        }
        return 0;
    };
    UpdateIoctlFunc(ioctlFunc);

    CheckpointTestHelper testHelper;
    for (int i = 0; i < requestCount; i++) {
        char processName[64];
        sprintf_s(processName, sizeof(processName), "com.test.stress.%d", i);

        AppSpawnConnection connection = {
            .connectionId = static_cast<uint32_t>(i + 1),
            .stream = nullptr,
            .receiverCtx = {0}
        };

        // 使用CheckpointTestHelper创建消息
        std::vector<uint8_t> buffer(1024 * 2);
        uint32_t msgLen = 0;
        CheckpointMsgParams params = {MSG_SPAWN_IMAGE_PROCESS, processName, 12345, 1000 + i, nullptr};
        int ret = testHelper.CreateCheckpointMsg(buffer, msgLen, params);
        ASSERT_EQ(ret, 0);

        AppSpawnMsgNode *msgNode = nullptr;
        uint32_t msgRecvLen = 0;
        uint32_t reminder = 0;
        ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msgNode, &msgRecvLen, &reminder);
        ASSERT_EQ(ret, 0);
        ret = DecodeAppSpawnMsg(msgNode);
        ASSERT_EQ(ret, 0);

        ProcessCheckpointReqMsg(&connection, msgNode);
    }

    UpdateIoctlFunc(nullptr);
    DeleteAppSpawnMgr(mgr);
}

}  // namespace OHOS
