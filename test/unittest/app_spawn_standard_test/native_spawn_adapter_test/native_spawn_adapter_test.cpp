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
#include <map>
#include <gtest/gtest.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_service.h"
#include "native_adapter_mock_test.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

APPSPAWN_STATIC int BuildFdInfoMap(const AppSpawnMsgNode *message, std::map<std::string, int> &fdMap, int isColdRun);
APPSPAWN_STATIC int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client);
APPSPAWN_STATIC int PreLoadNativeSpawn(AppSpawnMgr *content);

namespace OHOS {
class NativeSpawnAdapterTest : public testing::Test {
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

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = CreateAppSpawnMsg();
    ASSERT_NE(property->message, nullptr);

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(property->message, fdMap, false);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
    EXPECT_NE(ret, 0);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_COLD_RUN);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = CreateAppSpawnMsg();
    ASSERT_NE(property->message, nullptr);

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(property->message, fdMap, true);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
    EXPECT_NE(ret, 0);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_RunChildProcessor_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = CreateAppSpawnMsg();
    ASSERT_NE(property->message, nullptr);

    int ret = RunChildProcessor(&mgr->content, &property->client);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_RunChildProcessor_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_COLD_RUN);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = CreateAppSpawnMsg();
    ASSERT_NE(property->message, nullptr);

    int ret = RunChildProcessor(&mgr->content, &property->client);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_PreLoad_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NATIVESPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    int ret = PreLoadNativeSpawn(mgr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_PreLoad_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_COLD_RUN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NATIVESPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    int ret = PreLoadNativeSpawn(mgr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

// ==================== BuildFdInfoMap additional tests ====================

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_003, TestSize.Level0)
{
    // Test null message
    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(nullptr, fdMap, false);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_004, TestSize.Level0)
{
    // Test message with null buffer
    AppSpawnMsgNode msg = {};
    msg.buffer = nullptr;
    msg.tlvOffset = nullptr;
    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, false);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_005, TestSize.Level0)
{
    // Test message with null tlvOffset
    uint8_t buffer[256] = {0};
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = nullptr;
    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, false);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_006, TestSize.Level0)
{
    // Test non-cold-run with null connection returns -1
    uint8_t buffer[256] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1] = {0};
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = nullptr;
    msg.tlvCount = 0;
    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, false);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_007, TestSize.Level0)
{
    // Test non-cold-run with fdCount <= 0 returns 0 (no FDs needed)
    uint8_t buffer[256] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1] = {0};
    AppSpawnConnection connection = {};
    connection.receiverCtx.fdCount = 0;
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = &connection;
    msg.tlvCount = 0;
    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, false);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_008, TestSize.Level0)
{
    // Test TLV entry with INVALID_OFFSET returns -1
    uint8_t buffer[256] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    tlvOffsets[TLV_MAX] = INVALID_OFFSET;  // first extended TLV has invalid offset
    AppSpawnConnection connection = {};
    connection.receiverCtx.fdCount = 0;
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = &connection;
    msg.tlvCount = 1;
    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, true);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_009, TestSize.Level0)
{
    // Test TLV entry with type != TLV_MAX is skipped
    uint8_t buffer[512] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    // Place a TLV at offset 0 with type different from TLV_MAX
    AppSpawnTlv *tlv = reinterpret_cast<AppSpawnTlv *>(buffer);
    tlv->tlvType = TLV_BUNDLE_INFO;  // not TLV_MAX, should be skipped
    tlv->tlvLen = sizeof(AppSpawnTlv);
    tlvOffsets[TLV_MAX] = 0;
    AppSpawnConnection connection = {};
    connection.receiverCtx.fdCount = 0;
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = &connection;
    msg.tlvCount = 1;
    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, true);
    // Should succeed but fdMap should be empty since no AppFd TLV found
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(fdMap.size(), 0u);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_010, TestSize.Level0)
{
    // Test cold-run with valid env variable set
    uint8_t buffer[512] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    // Build an AppFd TLV entry
    AppSpawnTlvExt *tlvExt = reinterpret_cast<AppSpawnTlvExt *>(buffer);
    tlvExt->tlvType = TLV_MAX;
    tlvExt->tlvLen = sizeof(AppSpawnTlvExt) + 4;  // 4 bytes for key name "test"
    tlvExt->dataLen = 0;
    tlvExt->dataType = 0;
    (void)sprintf_s(tlvExt->tlvName, sizeof(tlvExt->tlvName), "%s", MSG_EXT_NAME_APP_FD);
    // key string after AppSpawnTlvExt header
    char *keyPtr = reinterpret_cast<char *>(buffer + sizeof(AppSpawnTlvExt));
    (void)sprintf_s(keyPtr, 5, "%s", "test");
    tlvOffsets[TLV_MAX] = 0;
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = nullptr;
    msg.tlvCount = 1;

    // Set env variable for cold run
    std::string envKey = std::string(APP_FDENV_PREFIX) + "test";
    setenv(envKey.c_str(), "5", 1);

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, true);
    unsetenv(envKey.c_str());
    EXPECT_EQ(ret, 0);
    EXPECT_NE(fdMap.find("test"), fdMap.end());
    EXPECT_EQ(fdMap["test"], 5);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_011, TestSize.Level0)
{
    // Test cold-run with env variable not set (fdChar is NULL)
    uint8_t buffer[512] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    AppSpawnTlvExt *tlvExt = reinterpret_cast<AppSpawnTlvExt *>(buffer);
    tlvExt->tlvType = TLV_MAX;
    tlvExt->tlvLen = sizeof(AppSpawnTlvExt) + 4;
    tlvExt->dataLen = 0;
    tlvExt->dataType = 0;
    (void)sprintf_s(tlvExt->tlvName, sizeof(tlvExt->tlvName), "%s", MSG_EXT_NAME_APP_FD);
    char *keyPtr = reinterpret_cast<char *>(buffer + sizeof(AppSpawnTlvExt));
    (void)sprintf_s(keyPtr, 6, "%s", "noenv");
    tlvOffsets[TLV_MAX] = 0;
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = nullptr;
    msg.tlvCount = 1;

    // Ensure env variable is not set
    std::string envKey = std::string(APP_FDENV_PREFIX) + "noenv";
    unsetenv(envKey.c_str());

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, true);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(fdMap.size(), 0u);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_012, TestSize.Level0)
{
    // Test cold-run with invalid fd value in env (non-numeric)
    uint8_t buffer[512] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    AppSpawnTlvExt *tlvExt = reinterpret_cast<AppSpawnTlvExt *>(buffer);
    tlvExt->tlvType = TLV_MAX;
    tlvExt->tlvLen = sizeof(AppSpawnTlvExt) + 4;
    tlvExt->dataLen = 0;
    tlvExt->dataType = 0;
    (void)sprintf_s(tlvExt->tlvName, sizeof(tlvExt->tlvName), "%s", MSG_EXT_NAME_APP_FD);
    char *keyPtr = reinterpret_cast<char *>(buffer + sizeof(AppSpawnTlvExt));
    (void)sprintf_s(keyPtr, 6, "%s", "badfd");
    tlvOffsets[TLV_MAX] = 0;
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = nullptr;
    msg.tlvCount = 1;

    // Set env with non-numeric value
    std::string envKey = std::string(APP_FDENV_PREFIX) + "badfd";
    setenv(envKey.c_str(), "not_a_number", 1);

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, true);
    unsetenv(envKey.c_str());
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(fdMap.size(), 0u);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_013, TestSize.Level0)
{
    // Test non-cold-run with valid connection and fds
    uint8_t buffer[512] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    AppSpawnTlvExt *tlvExt = reinterpret_cast<AppSpawnTlvExt *>(buffer);
    tlvExt->tlvType = TLV_MAX;
    tlvExt->tlvLen = sizeof(AppSpawnTlvExt) + 4;
    tlvExt->dataLen = 0;
    tlvExt->dataType = 0;
    (void)sprintf_s(tlvExt->tlvName, sizeof(tlvExt->tlvName), "%s", MSG_EXT_NAME_APP_FD);
    char *keyPtr = reinterpret_cast<char *>(buffer + sizeof(AppSpawnTlvExt));
    (void)sprintf_s(keyPtr, 5, "%s", "test");
    tlvOffsets[TLV_MAX] = 0;

    AppSpawnConnection connection = {};
    int testFd = dup(1);  // duplicate stdout to get a valid fd
    ASSERT_GT(testFd, 0);
    connection.receiverCtx.fdCount = 1;
    connection.receiverCtx.fds[0] = testFd;

    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = &connection;
    msg.tlvCount = 1;

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, false);
    close(testFd);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(fdMap.find("test"), fdMap.end());
    EXPECT_EQ(fdMap["test"], testFd);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_BuildFdInfoMap_014, TestSize.Level0)
{
    // Test non-cold-run with fdCount > 0 but invalid fd (fd <= 0) returns -1
    uint8_t buffer[512] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    AppSpawnTlvExt *tlvExt = reinterpret_cast<AppSpawnTlvExt *>(buffer);
    tlvExt->tlvType = TLV_MAX;
    tlvExt->tlvLen = sizeof(AppSpawnTlvExt) + 4;
    tlvExt->dataLen = 0;
    tlvExt->dataType = 0;
    (void)sprintf_s(tlvExt->tlvName, sizeof(tlvExt->tlvName), "%s", MSG_EXT_NAME_APP_FD);
    char *keyPtr = reinterpret_cast<char *>(buffer + sizeof(AppSpawnTlvExt));
    (void)sprintf_s(keyPtr, 5, "%s", "test");
    tlvOffsets[TLV_MAX] = 0;

    AppSpawnConnection connection = {};
    connection.receiverCtx.fdCount = 1;
    connection.receiverCtx.fds[0] = -1;  // invalid fd

    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = &connection;
    msg.tlvCount = 1;

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, false);
    EXPECT_EQ(ret, -1);
}

// ==================== RunChildProcessor additional tests ====================

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_RunChildProcessor_003, TestSize.Level0)
{
    // Test null client returns -1
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NATIVE_SPAWN);
    ASSERT_NE(mgr, nullptr);
    int ret = RunChildProcessor(&mgr->content, nullptr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, -1);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_RunChildProcessor_004, TestSize.Level0)
{
    // Test null content returns -1
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    int ret = RunChildProcessor(nullptr, &property->client);
    DeleteAppSpawningCtx(property);
    EXPECT_EQ(ret, -1);
}

// ==================== PreLoadNativeSpawn additional tests ====================

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_PreLoad_003, TestSize.Level0)
{
    // Test non-native spawn mode (e.g., MODE_FOR_APP_SPAWN) returns 0 without registering
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    int ret = PreLoadNativeSpawn(mgr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

HWTEST_F(NativeSpawnAdapterTest, Native_Spawn_PreLoad_004, TestSize.Level0)
{
    // Test null content for PreLoadNativeSpawn
    int ret = PreLoadNativeSpawn(nullptr);
    EXPECT_EQ(ret, 0);
}
}   // namespace OHOS
