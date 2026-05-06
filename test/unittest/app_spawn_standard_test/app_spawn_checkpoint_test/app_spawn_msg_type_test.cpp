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
#include <string>
#include <gtest/gtest.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_service.h"
#include "lib_wrapper.h"
#include "checkpoint_test_helper.h"
#include "securec.h"


using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
// static AppSpawnTestHelper g_testHelper;

class AppSpawnMsgTypeTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// ============================================================================
// 消息类型创建测试
// ============================================================================

/**
 * @brief Test creating message with MSG_SPAWN_IMAGE_PROCESS type
 * @note 验证MSG_SPAWN_IMAGE_PROCESS消息类型创建
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Type_Image_Process_Create_001, TestSize.Level0)
{
    const char *processName = "com.test.imgapp";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    // Create MSG_SPAWN_IMAGE_PROCESS request
    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(reqHandle, nullptr);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating message with MSG_SPAWN_WORKER_PROCESS type
 * @note 验证MSG_SPAWN_WORKER_PROCESS消息类型创建
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Type_Worker_Process_Create_001, TestSize.Level0)
{
    const char *processName = "com.test.workerapp";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    // Create MSG_SPAWN_WORKER_PROCESS request
    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_WORKER_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(reqHandle, nullptr);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating message with invalid process name (empty string)
 * @note 验证空进程名创建消息失败
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Type_Invalid_Name_001, TestSize.Level0)
{
    const char *emptyName = "";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, emptyName, &reqHandle);
    // Empty string should fail validation
    EXPECT_NE(ret, 0);
}

// ============================================================================
// AppSpawnReqMsgSetCheckpointInfo 测试
// ============================================================================

/**
 * @brief Test AppSpawnReqMsgSetCheckpointInfo with valid parameters
 * @note 验证设置镜像进程信息成功
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_Img_Info_Valid_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    // Create a message first
    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(reqHandle, nullptr);

    // Set img info
    pid_t imgPid = 12345;
    uint64_t checkPointId = 9876543210ULL;

    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, imgPid, checkPointId, nullptr);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test AppSpawnReqMsgSetCheckpointInfo with null handle
 * @note 验证空句柄设置镜像进程信息失败
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_Img_Info_Null_Handle_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    pid_t imgPid = 12345;
    uint64_t checkPointId = 9876543210ULL;

    int ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, imgPid, checkPointId, nullptr);
    EXPECT_NE(ret, 0);
}

/**
 * @brief Test AppSpawnReqMsgSetCheckpointInfo with zero imgPid
 * @note 验证imgPid为0时设置失败
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_Img_Info_Zero_Pid_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 0, 1000, nullptr);
    EXPECT_NE(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test AppSpawnReqMsgSetCheckpointInfo with zero checkPointId
 * @note 验证checkPointId为0时设置成功
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_Img_Info_Zero_Checkpoint_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 12345, 0, nullptr);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

// ============================================================================
// 消息标志测试
// ============================================================================

/**
 * @brief Test message creation with APP_FLAGS_SPAWN_IMG_INFO flag
 * @note 验证设置镜像启动标志的消息创建
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Flags_Spawn_Img_Info_001, TestSize.Level0)
{
    const char *processName = "com.test.imgspawn";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set APP_FLAGS_SPAWN_IMG_INFO flag
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_SPAWN_IMAGE_PROCESS);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating MSG_SPAWN_WORKER_PROCESS with img info
 * @note 验证WORKER_PROCESS消息配合镜像信息创建
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Worker_With_Img_Info_001, TestSize.Level0)
{
    const char *processName = "com.test.workerimg";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_WORKER_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set img info
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 54321, 12345678, nullptr);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test multiple img info settings on same message
 * @note 验证同一消息多次设置镜像信息
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Multi_Set_Img_Info_001, TestSize.Level1)
{
    const char *processName = "com.test.multiset";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // First set
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 11111, 1000, nullptr);
    EXPECT_EQ(ret, 0);

    // Second set - should update previous values
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 22222, 2000, nullptr);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating bundle info for worker process message
 * @note 验证WORKER_PROCESS消息设置bundle信息
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Worker_Bundle_Info_001, TestSize.Level0)
{
    const char *processName = "com.test.workerbundle";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_WORKER_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set bundle info
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 1, "com.test.bundle");
    EXPECT_EQ(ret, 0);

    // Set img info
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 33333, 3000, nullptr);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test message creation with special process name characters
 * @note 验证特殊字符进程名处理
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Special_Chars_In_Name_001, TestSize.Level1)
{
    // Process names with numbers and dots (typical bundle format)
    const char *validNames[] = {
        "com.example.app123",
        "com.test.app_version2",
        "app.simple.name"
    };

    for (size_t i = 0; i < sizeof(validNames) / sizeof(validNames[0]); i++) {
        AppSpawnReqMsgHandle reqHandle = nullptr;
        int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, validNames[i], &reqHandle);
        EXPECT_EQ(ret, 0) << "Failed to create msg for: " << validNames[i];
        if (ret == 0) {
            AppSpawnReqMsgFree(reqHandle);
        }
    }
}

/**
 * @brief Test AppSpawnReqMsgSetAppFlag for image-related flags
 * @note 验证镜像相关标志设置
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Img_Related_Flags_001, TestSize.Level1)
{
    const char *processName = "com.test.flags";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set SPAWN_IMAGE_PROCESS flag
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_SPAWN_IMAGE_PROCESS);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test creating MSG_SPAWN_WORKER_PROCESS without img info
 * @note 验证WORKER_PROCESS消息不设置img信息时行为
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Worker_Without_Img_Info_001, TestSize.Level1)
{
    const char *processName = "com.test.workernoimg";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_WORKER_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Do NOT set img info - this is the test scenario

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test message with MSG_SPAWN_IMAGE_PROCESS and missing img info
 * @note 验证IMAGE_PROCESS消息缺少镜像信息时的行为
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Image_Missing_Info_001, TestSize.Level1)
{
    const char *processName = "com.test.imgmiss";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Do not call AppSpawnReqMsgSetCheckpointInfo - missing imgPid/checkPointId

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test combination of MSG_SPAWN_WORKER_PROCESS with APP_FLAGS_COLD_BOOT
 * @note 验证WORKER_PROCESS与冷启动标志组合
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Worker_With_Cold_Boot_001, TestSize.Level1)
{
    const char *processName = "com.test.workercold";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_WORKER_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Set cold boot flag (should be allowed)
    ret = AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_COLD_BOOT);
    EXPECT_EQ(ret, 0);

    // Set img info
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 99999, 88888, nullptr);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test message type routing logic verification
 * @note 验证不同消息类型处理路径差异
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Type_Routing_Diff_001, TestSize.Level1)
{
    const char *processName = "com.test.routing";
    AppSpawnReqMsgHandle reqHandleImage = nullptr;
    AppSpawnReqMsgHandle reqHandleWorker = nullptr;
    AppSpawnReqMsgHandle reqHandleSpawn = nullptr;

    // Create different types
    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandleImage);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgCreate(MSG_SPAWN_WORKER_PROCESS, processName, &reqHandleWorker);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, processName, &reqHandleSpawn);
    EXPECT_EQ(ret, 0);

    // All three message types should be created successfully with different types
    // The msgType differentiation is handled internally by AppSpawnReqMsgCreate
    EXPECT_NE(reqHandleImage, nullptr);
    EXPECT_NE(reqHandleWorker, nullptr);
    EXPECT_NE(reqHandleSpawn, nullptr);

    AppSpawnReqMsgFree(reqHandleImage);
    AppSpawnReqMsgFree(reqHandleWorker);
    AppSpawnReqMsgFree(reqHandleSpawn);
}

// ============================================================================
// imgName 参数测试（新增功能）
// ============================================================================

/**
 * @brief Test AppSpawnReqMsgSetCheckpointInfo with custom imgName
 * @note 验证设置自定义镜像进程名成功
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_CheckPoint_With_ProcessName_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(reqHandle, nullptr);

    // Set checkpoint info with custom imgName
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 12345, 1001, "CustomImageProcess");
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test AppSpawnReqMsgSetCheckpointInfo with NULL imgName (fallback to bundleName)
 * @note 验证imgName为NULL时使用bundleName
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_CheckPoint_Null_ProcessName_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // NULL imgName - should use bundleName as fallback
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 12345, 1001, nullptr);
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test AppSpawnReqMsgSetCheckpointInfo with long imgName that exceeds limit
 * @note 验证超长imgName时返回错误
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_CheckPoint_Long_ProcessName_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // Create an imgName longer than APP_CHECKPOINT_NAME_LEN (256)
    std::string longName(300, 'A');
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 12345, 1001, longName.c_str());
    // strcpy_s should fail when source exceeds destination size
    EXPECT_NE(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief Test AppSpawnReqMsgSetCheckpointInfo with imgName at max length
 * @note 验证imgName恰好为最大长度时设置成功
 */
HWTEST_F(AppSpawnMsgTypeTest, App_Spawn_Msg_Set_CheckPoint_MaxLen_ProcessName_001, TestSize.Level0)
{
    const char *processName = "com.test.app";
    AppSpawnReqMsgHandle reqHandle = nullptr;

    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_IMAGE_PROCESS, processName, &reqHandle);
    EXPECT_EQ(ret, 0);

    // imgName with exactly APP_CHECKPOINT_NAME_LEN - 1 characters (must fit with null terminator)
    std::string maxName(APP_CHECKPOINT_NAME_LEN - 1, 'B');
    ret = AppSpawnReqMsgSetCheckpointInfo(reqHandle, 12345, 1001, maxName.c_str());
    EXPECT_EQ(ret, 0);

    AppSpawnReqMsgFree(reqHandle);
}

}   // namespace OHOS