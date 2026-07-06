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

#include <string>
#include <unistd.h>

#include <gtest/gtest.h>

#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_server.h"
#include "appspawn_service.h"
#include "appspawn_utils.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "arkweb_utils.h"

// RenderIpcFds is defined in anonymous namespace in nwebspawn_adapter.cpp.
// Under APPSPAWN_TEST, APPSPAWN_STATIC is empty so the functions are externally
// visible. We must provide a matching struct definition here for the function
// signatures that reference RenderIpcFds by value/reference.
struct RenderIpcFds {
    int32_t ipcFd;
    int32_t sharedFd;
    int32_t crashFd;
};

// C++ function declarations (use std::string or OHOS::ArkWeb, cannot be in extern "C")
// Note: UpdateAppWebEngineVersion is declared as plain "static" in the source file
// (not APPSPAWN_STATIC), so it cannot be linked externally. Test it indirectly
// through DupNwebRenderFdsBeforeRunHook.
APPSPAWN_STATIC void EraseAppWebEngineVersionFromCmd(std::string& renderCmd);
APPSPAWN_STATIC void AddRenderIpcFdsToCmd(std::string& renderCmd);
APPSPAWN_STATIC int ParseRenderIpcFds(const AppSpawnMsgNode *message,
    const AppSpawnMsgReceiverCtx& recvCtx, RenderIpcFds& origFds);
APPSPAWN_STATIC int DupRenderIpcFds(const RenderIpcFds &origFds);
APPSPAWN_STATIC int GetRenderIpcFdsFromEnv(RenderIpcFds &fds);
APPSPAWN_STATIC int DupNwebRenderFdsBeforeRunHook(AppSpawnMgr *content, AppSpawningCtx *property);
APPSPAWN_STATIC int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client);
APPSPAWN_STATIC int PreLoadNwebSpawn(AppSpawnMgr *content);

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
class NwebspawnAdapterTest : public testing::Test {
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

// Helper: Create AppSpawnMgr and explicitly set mode.
// CreateAppSpawnMgr returns a global singleton, so we must override the mode
// after creation to ensure the correct mode for each test.
static AppSpawnMgr *CreateMgrWithMode(RunMode mode, const char *procName)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(mode);
    if (mgr != nullptr) {
        mgr->content.mode = mode;
        mgr->content.longProcName = const_cast<char *>(procName);
        mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    }
    return mgr;
}

// Helper: Write AppFd TLV header and key data into buffer, return new offset.
static uint32_t WriteAppFdTlv(uint8_t *buffer, uint32_t bufferSize, uint32_t offset, const char *key)
{
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    if (sprintf_s(tlv.tlvName, sizeof(tlv.tlvName), "%s", MSG_EXT_NAME_APP_FD) < 0) {
        return offset;
    }
    tlv.dataLen = strlen(key) + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
    if (memcpy_s(buffer + offset, bufferSize - offset, &tlv, sizeof(tlv)) != 0) {
        return offset;
    }
    uint32_t keyOffset = offset + sizeof(AppSpawnTlvExt);
    uint32_t keySize = bufferSize - offset - sizeof(AppSpawnTlvExt);
    if (memcpy_s(buffer + keyOffset, keySize, key, tlv.dataLen) != 0) {
        return offset;
    }
    return offset + tlv.tlvLen;
}

// ==================== EraseAppWebEngineVersionFromCmd tests ====================

/**
 * @brief Test EraseAppWebEngineVersionFromCmd with version in middle
 * @note 验证从命令中间删除版本前缀
 */
HWTEST_F(NwebspawnAdapterTest, EraseAppWebEngineVersionFromCmd_Middle_001, TestSize.Level0)
{
    std::string cmd = "render#--appEngineVersion=1#other";
    EraseAppWebEngineVersionFromCmd(cmd);
    EXPECT_EQ(cmd, "render#other");
}

/**
 * @brief Test EraseAppWebEngineVersionFromCmd with version at end
 * @note 验证从命令末尾删除版本前缀
 */
HWTEST_F(NwebspawnAdapterTest, EraseAppWebEngineVersionFromCmd_AtEnd_001, TestSize.Level0)
{
    std::string cmd = "render#--appEngineVersion=1";
    EraseAppWebEngineVersionFromCmd(cmd);
    EXPECT_EQ(cmd, "render");
}

/**
 * @brief Test EraseAppWebEngineVersionFromCmd without version prefix
 * @note 验证无版本前缀时命令不变
 */
HWTEST_F(NwebspawnAdapterTest, EraseAppWebEngineVersionFromCmd_NoPrefix_001, TestSize.Level0)
{
    std::string cmd = "render_without_version";
    EraseAppWebEngineVersionFromCmd(cmd);
    EXPECT_EQ(cmd, "render_without_version");
}

/**
 * @brief Test EraseAppWebEngineVersionFromCmd with empty string
 * @note 验证空字符串不变
 */
HWTEST_F(NwebspawnAdapterTest, EraseAppWebEngineVersionFromCmd_Empty_001, TestSize.Level0)
{
    std::string cmd = "";
    EraseAppWebEngineVersionFromCmd(cmd);
    EXPECT_EQ(cmd, "");
}

/**
 * @brief Test EraseAppWebEngineVersionFromCmd with only version prefix
 * @note 验证仅包含版本前缀的字符串
 */
HWTEST_F(NwebspawnAdapterTest, EraseAppWebEngineVersionFromCmd_OnlyPrefix_001, TestSize.Level0)
{
    std::string cmd = "#--appEngineVersion=2";
    EraseAppWebEngineVersionFromCmd(cmd);
    EXPECT_EQ(cmd, "");
}

// ==================== AddRenderIpcFdsToCmd tests ====================

/**
 * @brief Test AddRenderIpcFdsToCmd appends fd suffixes to render command
 * @note 验证向渲染命令追加IPC FD后缀
 */
HWTEST_F(NwebspawnAdapterTest, AddRenderIpcFdsToCmd_Valid_001, TestSize.Level0)
{
    // Create temporary fds using pipe
    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);

    int pipeFds2[2] = {-1, -1};
    ret = pipe(pipeFds2);
    ASSERT_EQ(ret, 0);

    // Set up g_renderIpcFds via DupRenderIpcFds
    struct RenderIpcFds origFds;
    origFds.ipcFd = pipeFds[0];
    origFds.sharedFd = pipeFds2[0];
    origFds.crashFd = pipeFds2[1];

    ret = DupRenderIpcFds(origFds);
    EXPECT_EQ(ret, APPSPAWN_OK);

    std::string cmd = "render_cmd";
    AddRenderIpcFdsToCmd(cmd);

    // Verify suffixes are appended
    EXPECT_NE(cmd.find("#--ipc-fd="), std::string::npos);
    EXPECT_NE(cmd.find("#--shared-fd="), std::string::npos);
    EXPECT_NE(cmd.find("#--crash-fd="), std::string::npos);

    // Clean up
    close(pipeFds[0]);
    close(pipeFds[1]);
    close(pipeFds2[0]);
    close(pipeFds2[1]);
}

// ==================== DupRenderIpcFds tests ====================

/**
 * @brief Test DupRenderIpcFds with valid fds
 * @note 验证dup有效的fd成功
 */
HWTEST_F(NwebspawnAdapterTest, DupRenderIpcFds_Valid_001, TestSize.Level0)
{
    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);

    int pipeFds2[2] = {-1, -1};
    ret = pipe(pipeFds2);
    ASSERT_EQ(ret, 0);

    int pipeFds3[2] = {-1, -1};
    ret = pipe(pipeFds3);
    ASSERT_EQ(ret, 0);

    struct RenderIpcFds origFds;
    origFds.ipcFd = pipeFds[0];
    origFds.sharedFd = pipeFds2[0];
    origFds.crashFd = pipeFds3[0];

    ret = DupRenderIpcFds(origFds);
    EXPECT_EQ(ret, APPSPAWN_OK);

    // Clean up
    close(pipeFds[0]);
    close(pipeFds[1]);
    close(pipeFds2[0]);
    close(pipeFds2[1]);
    close(pipeFds3[0]);
    close(pipeFds3[1]);
}

/**
 * @brief Test DupRenderIpcFds with invalid fd (closed fd)
 * @note 验证dup无效fd失败
 */
HWTEST_F(NwebspawnAdapterTest, DupRenderIpcFds_InvalidFd_001, TestSize.Level0)
{
    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);

    int pipeFds2[2] = {-1, -1};
    ret = pipe(pipeFds2);
    ASSERT_EQ(ret, 0);

    struct RenderIpcFds origFds;
    origFds.ipcFd = pipeFds[0];
    origFds.sharedFd = pipeFds2[0];
    origFds.crashFd = -1;  // Invalid fd

    ret = DupRenderIpcFds(origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    close(pipeFds[0]);
    close(pipeFds[1]);
    close(pipeFds2[0]);
    close(pipeFds2[1]);
}

// ==================== GetRenderIpcFdsFromEnv tests ====================

/**
 * @brief Test GetRenderIpcFdsFromEnv with valid environment variables
 * @note 验证从环境变量获取有效fd
 */
HWTEST_F(NwebspawnAdapterTest, GetRenderIpcFdsFromEnv_Valid_001, TestSize.Level0)
{
    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);

    int pipeFds2[2] = {-1, -1};
    ret = pipe(pipeFds2);
    ASSERT_EQ(ret, 0);

    int pipeFds3[2] = {-1, -1};
    ret = pipe(pipeFds3);
    ASSERT_EQ(ret, 0);

    std::string ipcFdEnv = std::to_string(pipeFds[0]);
    std::string sharedFdEnv = std::to_string(pipeFds2[0]);
    std::string crashFdEnv = std::to_string(pipeFds3[0]);

    setenv("APPSPAWN_FD_ipc-fd", ipcFdEnv.c_str(), 1);
    setenv("APPSPAWN_FD_shared-fd", sharedFdEnv.c_str(), 1);
    setenv("APPSPAWN_FD_crash-fd", crashFdEnv.c_str(), 1);

    struct RenderIpcFds fds;
    ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_OK);
    EXPECT_EQ(fds.ipcFd, pipeFds[0]);
    EXPECT_EQ(fds.sharedFd, pipeFds2[0]);
    EXPECT_EQ(fds.crashFd, pipeFds3[0]);

    unsetenv("APPSPAWN_FD_ipc-fd");
    unsetenv("APPSPAWN_FD_shared-fd");
    unsetenv("APPSPAWN_FD_crash-fd");
    close(pipeFds[0]);
    close(pipeFds[1]);
    close(pipeFds2[0]);
    close(pipeFds2[1]);
    close(pipeFds3[0]);
    close(pipeFds3[1]);
}

/**
 * @brief Test GetRenderIpcFdsFromEnv without ipc-fd env variable
 * @note 验证缺少ipc-fd环境变量时返回失败
 */
HWTEST_F(NwebspawnAdapterTest, GetRenderIpcFdsFromEnv_NoIpcFd_001, TestSize.Level0)
{
    unsetenv("APPSPAWN_FD_ipc-fd");
    unsetenv("APPSPAWN_FD_shared-fd");
    unsetenv("APPSPAWN_FD_crash-fd");

    struct RenderIpcFds fds;
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief Test GetRenderIpcFdsFromEnv without shared-fd env variable
 * @note 验证缺少shared-fd环境变量时返回失败
 */
HWTEST_F(NwebspawnAdapterTest, GetRenderIpcFdsFromEnv_NoSharedFd_001, TestSize.Level0)
{
    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);

    std::string ipcFdEnv = std::to_string(pipeFds[0]);
    setenv("APPSPAWN_FD_ipc-fd", ipcFdEnv.c_str(), 1);
    unsetenv("APPSPAWN_FD_shared-fd");
    unsetenv("APPSPAWN_FD_crash-fd");

    struct RenderIpcFds fds;
    ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    unsetenv("APPSPAWN_FD_ipc-fd");
    close(pipeFds[0]);
    close(pipeFds[1]);
}

/**
 * @brief Test GetRenderIpcFdsFromEnv without crash-fd env variable
 * @note 验证缺少crash-fd环境变量时返回失败
 */
HWTEST_F(NwebspawnAdapterTest, GetRenderIpcFdsFromEnv_NoCrashFd_001, TestSize.Level0)
{
    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);

    int pipeFds2[2] = {-1, -1};
    ret = pipe(pipeFds2);
    ASSERT_EQ(ret, 0);

    std::string ipcFdEnv = std::to_string(pipeFds[0]);
    std::string sharedFdEnv = std::to_string(pipeFds2[0]);
    setenv("APPSPAWN_FD_ipc-fd", ipcFdEnv.c_str(), 1);
    setenv("APPSPAWN_FD_shared-fd", sharedFdEnv.c_str(), 1);
    unsetenv("APPSPAWN_FD_crash-fd");

    struct RenderIpcFds fds;
    ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    unsetenv("APPSPAWN_FD_ipc-fd");
    unsetenv("APPSPAWN_FD_shared-fd");
    close(pipeFds[0]);
    close(pipeFds[1]);
    close(pipeFds2[0]);
    close(pipeFds2[1]);
}

/**
 * @brief Test GetRenderIpcFdsFromEnv with invalid (non-numeric) ipc-fd value
 * @note 验证ipc-fd为非数字值时返回失败
 */
HWTEST_F(NwebspawnAdapterTest, GetRenderIpcFdsFromEnv_InvalidIpcFd_001, TestSize.Level0)
{
    setenv("APPSPAWN_FD_ipc-fd", "not_a_number", 1);
    unsetenv("APPSPAWN_FD_shared-fd");
    unsetenv("APPSPAWN_FD_crash-fd");

    struct RenderIpcFds fds;
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    unsetenv("APPSPAWN_FD_ipc-fd");
}

/**
 * @brief Test GetRenderIpcFdsFromEnv with zero ipc-fd value
 * @note 验证ipc-fd为0时返回失败（fd必须大于0）
 */
HWTEST_F(NwebspawnAdapterTest, GetRenderIpcFdsFromEnv_ZeroIpcFd_001, TestSize.Level0)
{
    setenv("APPSPAWN_FD_ipc-fd", "0", 1);
    unsetenv("APPSPAWN_FD_shared-fd");
    unsetenv("APPSPAWN_FD_crash-fd");

    struct RenderIpcFds fds;
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    unsetenv("APPSPAWN_FD_ipc-fd");
}

/**
 * @brief Test GetRenderIpcFdsFromEnv with negative shared-fd value
 * @note 验证shared-fd为负数时返回失败
 */
HWTEST_F(NwebspawnAdapterTest, GetRenderIpcFdsFromEnv_NegativeSharedFd_001, TestSize.Level0)
{
    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);

    std::string ipcFdEnv = std::to_string(pipeFds[0]);
    setenv("APPSPAWN_FD_ipc-fd", ipcFdEnv.c_str(), 1);
    setenv("APPSPAWN_FD_shared-fd", "-1", 1);
    unsetenv("APPSPAWN_FD_crash-fd");

    struct RenderIpcFds fds;
    ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);

    unsetenv("APPSPAWN_FD_ipc-fd");
    unsetenv("APPSPAWN_FD_shared-fd");
    close(pipeFds[0]);
    close(pipeFds[1]);
}

// ==================== PreLoadNwebSpawn tests ====================

/**
 * @brief Test PreLoadNwebSpawn with non-nwebspawn mode
 * @note 验证非nwebspawn模式直接返回0
 */
HWTEST_F(NwebspawnAdapterTest, PreLoadNwebSpawn_NonNwebMode_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_APP_SPAWN, APPSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    int ret = PreLoadNwebSpawn(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief Test PreLoadNwebSpawn with nwebspawn mode
 * @note 验证nwebspawn模式预加载返回0
 */
HWTEST_F(NwebspawnAdapterTest, PreLoadNwebSpawn_NwebMode_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    int ret = PreLoadNwebSpawn(mgr);
    EXPECT_EQ(ret, 0);
}

// ==================== DupNwebRenderFdsBeforeRunHook tests ====================

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with non-nwebspawn mode
 * @note 验证非nwebspawn模式直接返回APPSPAWN_OK
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_NonNwebMode_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_APP_SPAWN, APPSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, nullptr);
    EXPECT_EQ(ret, APPSPAWN_OK);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with null property
 * @note 验证nwebspawn模式下property为空返回失败
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_NullProperty_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, nullptr);
    EXPECT_NE(ret, APPSPAWN_OK);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with null message in property
 * @note 验证nwebspawn模式下message为空返回失败
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_NullMessage_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx property = {};
    property.message = nullptr;

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, &property);
    EXPECT_NE(ret, APPSPAWN_OK);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with M114 version (skip fd processing)
 * @note 验证M114版本跳过fd处理
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_M114Skip_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    // Build a minimal AppSpawningCtx with M114 render-cmd
    // TLV offset array must be large enough: TLV_MAX + tlvCount entries
    uint8_t buffer[1024] = {0};
    uint32_t tlvOffsets[TLV_MAX + 4];
    for (uint32_t i = 0; i < TLV_MAX + 4; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 1;

    // Build TLV for render-cmd with M114 version at buffer offset 0
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    ASSERT_GE(sprintf_s(tlv.tlvName, sizeof(tlv.tlvName), "%s", MSG_EXT_NAME_RENDER_CMD), 0);
    const char *renderCmdData = "render#--appEngineVersion=1";
    tlv.dataLen = strlen(renderCmdData) + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);

    ASSERT_EQ(memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv)), 0);
    ASSERT_EQ(memcpy_s(buffer + sizeof(AppSpawnTlvExt), sizeof(buffer) - sizeof(AppSpawnTlvExt),
             renderCmdData, tlv.dataLen), 0);

    tlvOffsets[TLV_MAX] = 0;  // Extended TLVs start at index TLV_MAX

    AppSpawningCtx property = {};
    property.message = &msgNode;

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, &property);
    EXPECT_EQ(ret, APPSPAWN_OK);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with null buffer in message
 * @note 验证message的buffer为空时返回失败
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_NullBuffer_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX + 2; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = nullptr;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 0;

    AppSpawningCtx property = {};
    property.message = &msgNode;

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, &property);
    EXPECT_NE(ret, APPSPAWN_OK);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with null tlvOffset in message
 * @note 验证message的tlvOffset为空时返回APPSPAWN_TLV_NONE
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_NullTlvOffset_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    uint8_t buffer[64] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = nullptr;
    msgNode.tlvCount = 0;

    AppSpawningCtx property = {};
    property.message = &msgNode;

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, &property);
    EXPECT_EQ(ret, APPSPAWN_TLV_NONE);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with cold start mode (connection is NULL)
 * @note 验证冷启动模式下从环境变量获取fd，无环境变量时失败
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_ColdStartNoEnv_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    uint8_t buffer[1024] = {0};
    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX + 2; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 0;
    msgNode.connection = nullptr;  // Cold start mode

    AppSpawningCtx property = {};
    property.message = &msgNode;

    unsetenv("APPSPAWN_FD_ipc-fd");
    unsetenv("APPSPAWN_FD_shared-fd");
    unsetenv("APPSPAWN_FD_crash-fd");

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, &property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with cold start mode and valid env
 * @note 验证冷启动模式下从环境变量成功获取fd
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_ColdStartWithEnv_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    uint8_t buffer[1024] = {0};
    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX + 2; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 0;
    msgNode.connection = nullptr;  // Cold start mode

    AppSpawningCtx property = {};
    property.message = &msgNode;

    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);
    int pipeFds2[2] = {-1, -1};
    ret = pipe(pipeFds2);
    ASSERT_EQ(ret, 0);
    int pipeFds3[2] = {-1, -1};
    ret = pipe(pipeFds3);
    ASSERT_EQ(ret, 0);

    setenv("APPSPAWN_FD_ipc-fd", std::to_string(pipeFds[0]).c_str(), 1);
    setenv("APPSPAWN_FD_shared-fd", std::to_string(pipeFds2[0]).c_str(), 1);
    setenv("APPSPAWN_FD_crash-fd", std::to_string(pipeFds3[0]).c_str(), 1);

    ret = DupNwebRenderFdsBeforeRunHook(mgr, &property);
    EXPECT_EQ(ret, APPSPAWN_OK);

    unsetenv("APPSPAWN_FD_ipc-fd");
    unsetenv("APPSPAWN_FD_shared-fd");
    unsetenv("APPSPAWN_FD_crash-fd");
    close(pipeFds[0]);
    close(pipeFds[1]);
    close(pipeFds2[0]);
    close(pipeFds2[1]);
    close(pipeFds3[0]);
    close(pipeFds3[1]);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with connection but zero fdCount
 * @note 验证有连接但fdCount为0时返回0
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_ZeroFdCount_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    uint8_t buffer[256] = {0};
    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX + 2; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 0;

    AppSpawnConnection connection = {};
    connection.receiverCtx.fdCount = 0;
    msgNode.connection = &connection;

    AppSpawningCtx property = {};
    property.message = &msgNode;

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, &property);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief Test DupNwebRenderFdsBeforeRunHook with connection but invalid TLV data
 * @note 验证有连接和fdCount但TLV数据无效时返回APPSPAWN_ARG_INVALID
 */
HWTEST_F(NwebspawnAdapterTest, DupNwebRenderFdsBeforeRunHook_InvalidTlvData_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateMgrWithMode(MODE_FOR_NWEB_SPAWN, NWEBSPAWN_SERVER_NAME);
    ASSERT_NE(mgr, nullptr);

    uint8_t buffer[256] = {0};
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    ASSERT_GE(sprintf_s(tlv.tlvName, sizeof(tlv.tlvName), "%s", "OtherName"), 0);
    tlv.dataLen = 5;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
    ASSERT_EQ(memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv)), 0);

    // TLV offsets: extended TLVs start at index TLV_MAX
    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX + 2; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    tlvOffsets[TLV_MAX] = 0;  // First extended TLV at buffer offset 0

    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 1;

    AppSpawnConnection connection = {};
    connection.receiverCtx.fdCount = 3;
    connection.receiverCtx.fds[0] = 10;
    connection.receiverCtx.fds[1] = 11;
    connection.receiverCtx.fds[2] = 12;
    msgNode.connection = &connection;

    AppSpawningCtx property = {};
    property.message = &msgNode;

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, &property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

// ==================== RunChildProcessor tests ====================

/**
 * @brief Test RunChildProcessor with null client
 * @note 验证client为空时返回-1
 */
HWTEST_F(NwebspawnAdapterTest, RunChildProcessor_NullClient_001, TestSize.Level0)
{
    int ret = RunChildProcessor(nullptr, nullptr);
    EXPECT_EQ(ret, -1);
}

// ==================== ParseRenderIpcFds tests ====================

/**
 * @brief Test ParseRenderIpcFds with invalid tlv offset
 * @note 验证无效tlv偏移时返回APPSPAWN_ARG_INVALID
 */
HWTEST_F(NwebspawnAdapterTest, ParseRenderIpcFds_InvalidOffset_001, TestSize.Level0)
{
    // TLV offsets: extended TLVs start at index TLV_MAX
    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX + 2; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    AppSpawnMsgNode msgNode = {};
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 1;
    uint8_t buffer[256] = {0};
    msgNode.buffer = buffer;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 0;

    struct RenderIpcFds fds;
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief Test ParseRenderIpcFds with no AppFd TLV entries
 * @note 验证无AppFd TLV条目时返回APPSPAWN_ARG_INVALID
 */
HWTEST_F(NwebspawnAdapterTest, ParseRenderIpcFds_NoAppFdTlv_001, TestSize.Level0)
{
    uint8_t buffer[256] = {0};
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    ASSERT_GE(sprintf_s(tlv.tlvName, sizeof(tlv.tlvName), "%s", "OtherName"), 0);
    tlv.dataLen = 5;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
    ASSERT_EQ(memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv)), 0);

    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX + 2; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    tlvOffsets[TLV_MAX] = 0;

    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 1;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 3;
    recvCtx.fds[0] = 10;
    recvCtx.fds[1] = 11;
    recvCtx.fds[2] = 12;

    struct RenderIpcFds fds;
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief Test ParseRenderIpcFds with AppFd TLV but insufficient fd count
 * @note 验证AppFd TLV存在但fd数量不足时返回APPSPAWN_ARG_INVALID
 */
HWTEST_F(NwebspawnAdapterTest, ParseRenderIpcFds_InsufficientFdCount_001, TestSize.Level0)
{
    uint8_t buffer[256] = {0};
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    ASSERT_GE(sprintf_s(tlv.tlvName, sizeof(tlv.tlvName), "%s", MSG_EXT_NAME_APP_FD), 0);
    const char *fdKey = "ipc-fd";
    tlv.dataLen = strlen(fdKey) + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
    ASSERT_EQ(memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv)), 0);
    ASSERT_EQ(memcpy_s(buffer + sizeof(AppSpawnTlvExt),
        sizeof(buffer) - sizeof(AppSpawnTlvExt), fdKey, tlv.dataLen), 0);

    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX + 2; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }
    tlvOffsets[TLV_MAX] = 0;

    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 1;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 0;

    struct RenderIpcFds fds;
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief Test ParseRenderIpcFds with valid AppFd TLVs for all three fds
 * @note 验证三个有效的AppFd TLV解析成功
 */
HWTEST_F(NwebspawnAdapterTest, ParseRenderIpcFds_AllThreeFds_001, TestSize.Level0)
{
    int pipeFds[2] = {-1, -1};
    int ret = pipe(pipeFds);
    ASSERT_EQ(ret, 0);
    int pipeFds2[2] = {-1, -1};
    ret = pipe(pipeFds2);
    ASSERT_EQ(ret, 0);
    int pipeFds3[2] = {-1, -1};
    ret = pipe(pipeFds3);
    ASSERT_EQ(ret, 0);

    uint8_t buffer[1024] = {0};
    uint32_t currOffset = 0;
    uint32_t tlvOffsets[TLV_MAX + 4];
    for (uint32_t i = 0; i < TLV_MAX + 4; i++) {
        tlvOffsets[i] = INVALID_OFFSET;
    }

    tlvOffsets[TLV_MAX] = currOffset;
    currOffset = WriteAppFdTlv(buffer, sizeof(buffer), currOffset, "ipc-fd");
    tlvOffsets[TLV_MAX + 1] = currOffset;
    currOffset = WriteAppFdTlv(buffer, sizeof(buffer), currOffset, "shared-fd");
    tlvOffsets[TLV_MAX + 2] = currOffset;
    currOffset = WriteAppFdTlv(buffer, sizeof(buffer), currOffset, "crash-fd");

    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvOffset = tlvOffsets;
    msgNode.tlvCount = 3;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 3;
    recvCtx.fds[0] = pipeFds[0];
    recvCtx.fds[1] = pipeFds2[0];
    recvCtx.fds[2] = pipeFds3[0];

    struct RenderIpcFds fds;
    ret = ParseRenderIpcFds(&msgNode, recvCtx, fds);
    EXPECT_EQ(ret, APPSPAWN_OK);
    EXPECT_EQ(fds.ipcFd, pipeFds[0]);
    EXPECT_EQ(fds.sharedFd, pipeFds2[0]);
    EXPECT_EQ(fds.crashFd, pipeFds3[0]);

    close(pipeFds[0]);
    close(pipeFds[1]);
    close(pipeFds2[0]);
    close(pipeFds2[1]);
    close(pipeFds3[0]);
    close(pipeFds3[1]);
}

}  // namespace OHOS
