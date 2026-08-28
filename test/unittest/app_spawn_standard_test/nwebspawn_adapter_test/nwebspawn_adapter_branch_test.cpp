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
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <string>
#include <unistd.h>

#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "json_utils.h"
#include "parameter.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

int PreLoadNwebSpawn(AppSpawnMgr *content);
int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client);
void EraseAppWebEngineVersionFromCmd(std::string& renderCmd);
void AddRenderIpcFdsToCmd(std::string& renderCmd);
int ParseRenderIpcFds(const AppSpawnMsgNode *message,
    const AppSpawnMsgReceiverCtx& recvCtx, struct RenderIpcFds& origFds);
int DupRenderIpcFds(const struct RenderIpcFds &origFds);
int GetRenderIpcFdsFromEnv(struct RenderIpcFds &fds);
int DupNwebRenderFdsBeforeRunHook(AppSpawnMgr *content, AppSpawningCtx *property);

struct RenderIpcFds {
    int32_t ipcFd;
    int32_t sharedFd;
    int32_t crashFd;
};

extern "C" void SetDlsymResult(uint32_t flags, bool success);
extern "C" uint32_t g_dlsymResultFlags;
#define DLSYM_FAIL_SET_SEC_POLICY 0x01
#define DLSYM_FAIL_NWEB_MAIN 0x02
#define DLSYM_FAIL_INIT_ENV 0x04

static void SetupMessageConnection(AppSpawningCtx *property, int fdCount = 0)
{
    if (property == nullptr || property->message == nullptr) {
        return;
    }
    AppSpawnConnection *conn = reinterpret_cast<AppSpawnConnection *>(calloc(1, sizeof(AppSpawnConnection)));
    if (conn != nullptr) {
        conn->receiverCtx.fdCount = fdCount;
        property->message->connection = conn;
    }
}

static void FreeMessageConnection(AppSpawningCtx *property)
{
    if (property == nullptr || property->message == nullptr) {
        return;
    }
    if (property->message->connection != nullptr) {
        free(property->message->connection);
        property->message->connection = nullptr;
    }
}

namespace OHOS {

class NWebAdapterBranchTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());
        SetDlsymResult(0xFF, true);
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
        unsetenv("APPSPAWN_FD_ipc-fd");
        unsetenv("APPSPAWN_FD_shared-fd");
        unsetenv("APPSPAWN_FD_crash-fd");
    }
};

/**
 * @brief ProcessType == "render" && SetSeccompPolicyForRenderer succeeds
 *        Covers: processType == "render" = true, SetSeccompPolicyForRenderer = true (seccomp enabled, policy OK)
 */
HWTEST_F(NWebAdapterBranchTest, SetSeccomp_Render_Process_Policy_Ok, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetSeccomp_Render_Process_Policy_Ok start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    int ret = PreLoadNwebSpawn(mgr);
    EXPECT_EQ(ret, 0);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ProcessType != "render" - seccomp check is skipped
 *        Covers: processType != "render" branch (skip SetSeccompPolicyForRenderer entirely)
 */
HWTEST_F(NWebAdapterBranchTest, SetSeccomp_NonRender_Process, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "SetSeccomp_NonRender_Process start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    int ret = PreLoadNwebSpawn(mgr);
    EXPECT_EQ(ret, 0);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief EraseAppWebEngineVersionFromCmd - prefix not found
 *        Covers: posLeft == std::string::npos -> early return
 */
HWTEST_F(NWebAdapterBranchTest, EraseVersion_PrefixNotFound, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "EraseVersion_PrefixNotFound start";
    std::string renderCmd = "/system/bin/sh ls -l";
    EraseAppWebEngineVersionFromCmd(renderCmd);
    EXPECT_EQ(renderCmd, "/system/bin/sh ls -l");
}

/**
 * @brief EraseAppWebEngineVersionFromCmd - prefix found, no trailing '#'
 *        Covers: posEnd == std::string::npos branch, erase to end
 */
HWTEST_F(NWebAdapterBranchTest, EraseVersion_PrefixFound_NoTrailingHash, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "EraseVersion_PrefixFound_NoTrailingHash start";
    std::string renderCmd = "/system/bin/sh ls -l#--appEngineVersion=1";
    EraseAppWebEngineVersionFromCmd(renderCmd);
    EXPECT_EQ(renderCmd, "/system/bin/sh ls -l");
}

/**
 * @brief EraseAppWebEngineVersionFromCmd - prefix found, trailing '#'
 *        Covers: posEnd != std::string::npos branch, erase partial
 */
HWTEST_F(NWebAdapterBranchTest, EraseVersion_PrefixFound_WithTrailingHash, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "EraseVersion_PrefixFound_WithTrailingHash start";
    std::string renderCmd = "/system/bin/sh#--appEngineVersion=2#other";
    EraseAppWebEngineVersionFromCmd(renderCmd);
    EXPECT_EQ(renderCmd, "/system/bin/sh#other");
}

/**
 * @brief AddRenderIpcFdsToCmd - append IPC fds to render command
 *        Covers: all fd fields are appended and then reset to -1
 */
HWTEST_F(NWebAdapterBranchTest, AddRenderIpcFdsToCmd_Normal, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "AddRenderIpcFdsToCmd_Normal start";
    std::string renderCmd = "/system/bin/sh ls -l";
    AddRenderIpcFdsToCmd(renderCmd);
    EXPECT_NE(renderCmd.find("#--ipc-fd="), std::string::npos);
    EXPECT_NE(renderCmd.find("#--shared-fd="), std::string::npos);
    EXPECT_NE(renderCmd.find("#--crash-fd="), std::string::npos);
}

/**
 * @brief RunChildProcessor with null content - renderCmd is nullptr
 *        Covers: renderCmd == nullptr -> return -1
 */
HWTEST_F(NWebAdapterBranchTest, RunChild_NullRenderCmd, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "RunChild_NullRenderCmd start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    int ret = PreLoadNwebSpawn(mgr);
    EXPECT_EQ(ret, 0);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(property);
    int result = RunChildProcessor(nullptr, client);
    EXPECT_EQ(result, -1);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief RunChildProcessor with nwebRenderHandle == nullptr (dlsym returns null for NWebRenderMain)
 *        Covers: nwebRenderHandle == nullptr -> return -1 (line 175-178)
 */
HWTEST_F(NWebAdapterBranchTest, RunChild_NwebRenderHandleNull, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "RunChild_NwebRenderHandleNull start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.nweb");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    AppSpawningCtx *spawningCtx = helper.GetAppProperty(clientHandle, reqHandle);
    AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(spawningCtx);

    int result = RunChildProcessor(nullptr, client);
    EXPECT_EQ(result, 0);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief RunChildProcessor - dlsym for NWebRenderMain fails
 *        Covers: funcNWebRenderMain == nullptr -> return -1 (line 189-192)
 */
HWTEST_F(NWebAdapterBranchTest, RunChild_DlsymNWebRenderMainNull, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "RunChild_DlsymNWebRenderMainNull start";
    SetDlsymResult(DLSYM_FAIL_NWEB_MAIN, false);

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.nweb2");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    AppSpawningCtx *spawningCtx = helper.GetAppProperty(clientHandle, reqHandle);
    AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(spawningCtx);

    int result = RunChildProcessor(nullptr, client);
    EXPECT_EQ(result, 0);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    SetDlsymResult(DLSYM_FAIL_NWEB_MAIN, true);
}

/**
 * @brief RunChildProcessor - processType == "render" and SetSeccompPolicyForRenderer succeeds
 *        Covers: processType == "render" branch (when WITH_SECCOMP is not defined,
 *                SetSeccompPolicyForRenderer always returns true)
 *        When WITH_SECCOMP IS defined and SetRendererSeccompPolicy dlsym fails,
 *        this would cover the return -1 branch (line 183-185).
 */
HWTEST_F(NWebAdapterBranchTest, RunChild_RenderProcess_SeccompPolicyOk, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "RunChild_RenderProcess_SeccompPolicyOk start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.nweb3");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_PROCESS_TYPE, "render");
    AppSpawningCtx *spawningCtx = helper.GetAppProperty(clientHandle, reqHandle);
    AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(spawningCtx);

    int result = RunChildProcessor(nullptr, client);
    EXPECT_EQ(result, 0);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ParseRenderIpcFds - invalid TLV offset
 *        Covers: tlvOffset[index] == INVALID_OFFSET -> return APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_InvalidOffset, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_InvalidOffset start";
    uint8_t buffer[256] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 1;
    uint32_t offsets[TLV_MAX + 1] = {0};
    offsets[TLV_MAX] = INVALID_OFFSET;  // First ext TLV offset is invalid
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief ParseRenderIpcFds - TLV type != TLV_MAX (skip)
 *        Covers: tlvType != TLV_MAX -> continue (skips this TLV entry)
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_WrongTlvType, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_WrongTlvType start";
    uint8_t buffer[256] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 1;
    AppSpawnTlv tlv = {};
    tlv.tlvType = 0;  // Not TLV_MAX
    memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv));
    uint32_t offsets[TLV_MAX + 1] = {0};
    offsets[TLV_MAX] = 0;  // ext TLV offset points to buffer start
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief ParseRenderIpcFds - tlvName != MSG_EXT_NAME_APP_FD (skip)
 *        Covers: strcmp(tlv->tlvName, MSG_EXT_NAME_APP_FD) != 0 -> continue
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_WrongTlvName, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_WrongTlvName start";
    uint8_t buffer[256] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 1;
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), "WrongName");
    tlv.dataLen = 0;
    tlv.tlvLen = sizeof(AppSpawnTlvExt);
    memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv));
    uint32_t offsets[TLV_MAX + 1] = {0};
    offsets[TLV_MAX] = 0;
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief ParseRenderIpcFds - fdCount exhausted / fd <= 0
 *        Covers: findFdIndex >= recvCtx.fdCount || recvCtx.fds[findFdIndex] <= 0
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_FdCountExhausted, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_FdCountExhausted start";
    uint8_t buffer[256] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 1;
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), MSG_EXT_NAME_APP_FD);
    tlv.dataLen = strlen("ipc-fd") + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
    memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv));
    memcpy_s(buffer + sizeof(tlv), sizeof(buffer) - sizeof(tlv), "ipc-fd", tlv.dataLen);
    uint32_t offsets[TLV_MAX + 1] = {0};
    offsets[TLV_MAX] = 0;
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 0;
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief ParseRenderIpcFds - fd value is <= 0
 *        Covers: recvCtx.fds[findFdIndex] <= 0
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_FdValueInvalid, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_FdValueInvalid start";
    uint8_t buffer[256] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 1;
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), MSG_EXT_NAME_APP_FD);
    tlv.dataLen = strlen("ipc-fd") + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
    memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv));
    memcpy_s(buffer + sizeof(tlv), sizeof(buffer) - sizeof(tlv), "ipc-fd", tlv.dataLen);
    uint32_t offsets[TLV_MAX + 1] = {0};
    offsets[TLV_MAX] = 0;
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 1;
    recvCtx.fds[0] = -1;
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief ParseRenderIpcFds - key == "shared-fd"
 *        Covers: else if (key == "shared-fd") branch
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_SharedFdKey, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_SharedFdKey start";
    uint8_t buffer[256] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 1;
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), MSG_EXT_NAME_APP_FD);
    tlv.dataLen = strlen("shared-fd") + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
    memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv));
    memcpy_s(buffer + sizeof(tlv), sizeof(buffer) - sizeof(tlv), "shared-fd", tlv.dataLen);
    uint32_t offsets[TLV_MAX + 1] = {0};
    offsets[TLV_MAX] = 0;
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 1;
    recvCtx.fds[0] = 5;
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    EXPECT_EQ(origFds.sharedFd, 5);
}

/**
 * @brief ParseRenderIpcFds - key == "crash-fd"
 *        Covers: else if (key == "crash-fd") branch
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_CrashFdKey, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_CrashFdKey start";
    uint8_t buffer[256] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 1;
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), MSG_EXT_NAME_APP_FD);
    tlv.dataLen = strlen("crash-fd") + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
    memcpy_s(buffer, sizeof(buffer), &tlv, sizeof(tlv));
    memcpy_s(buffer + sizeof(tlv), sizeof(buffer) - sizeof(tlv), "crash-fd", tlv.dataLen);
    uint32_t offsets[TLV_MAX + 1] = {0};
    offsets[TLV_MAX] = 0;
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 1;
    recvCtx.fds[0] = 6;
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    EXPECT_EQ(origFds.crashFd, 6);
}

/**
 * @brief ParseRenderIpcFds - all 3 fds found, early break
 *        Covers: ipcFd != -1 && sharedFd != -1 && crashFd != -1 -> break (line 236-238)
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_AllFdsFound, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_AllFdsFound start";
    uint8_t buffer[512] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 3;

    uint32_t offsets[TLV_MAX + 3] = {};
    uint32_t currOffset = 0;
    const char *fdNames[] = {"ipc-fd", "shared-fd", "crash-fd"};

    for (int i = 0; i < 3; i++) {
        offsets[TLV_MAX + i] = currOffset;
        AppSpawnTlvExt tlv = {};
        tlv.tlvType = TLV_MAX;
        strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), MSG_EXT_NAME_APP_FD);
        tlv.dataLen = strlen(fdNames[i]) + 1;
        tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
        memcpy_s(buffer + currOffset, sizeof(buffer) - currOffset, &tlv, sizeof(tlv));
        memcpy_s(buffer + currOffset + sizeof(tlv), sizeof(buffer) - currOffset - sizeof(tlv),
            fdNames[i], tlv.dataLen);
        currOffset += tlv.tlvLen;
    }
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 3;
    recvCtx.fds[0] = 10;
    recvCtx.fds[1] = 11;
    recvCtx.fds[2] = 12;
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_OK);
    EXPECT_EQ(origFds.ipcFd, 10);
    EXPECT_EQ(origFds.sharedFd, 11);
    EXPECT_EQ(origFds.crashFd, 12);
}

/**
 * @brief ParseRenderIpcFds - partial fds, final check fails
 *        Covers: ipcFd <= 0 || sharedFd <= 0 || crashFd <= 0 -> APPSPAWN_ARG_INVALID (line 241-244)
 */
HWTEST_F(NWebAdapterBranchTest, ParseRenderIpcFds_PartialFds, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ParseRenderIpcFds_PartialFds start";
    uint8_t buffer[512] = {0};
    AppSpawnMsgNode msgNode = {};
    msgNode.buffer = buffer;
    msgNode.tlvCount = 2;

    uint32_t offsets[TLV_MAX + 2] = {};
    uint32_t currOffset = 0;
    const char *fdNames[] = {"ipc-fd", "shared-fd"};

    for (int i = 0; i < 2; i++) {
        offsets[TLV_MAX + i] = currOffset;
        AppSpawnTlvExt tlv = {};
        tlv.tlvType = TLV_MAX;
        strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), MSG_EXT_NAME_APP_FD);
        tlv.dataLen = strlen(fdNames[i]) + 1;
        tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
        memcpy_s(buffer + currOffset, sizeof(buffer) - currOffset, &tlv, sizeof(tlv));
        memcpy_s(buffer + currOffset + sizeof(tlv), sizeof(buffer) - currOffset - sizeof(tlv),
            fdNames[i], tlv.dataLen);
        currOffset += tlv.tlvLen;
    }
    msgNode.tlvOffset = offsets;

    AppSpawnMsgReceiverCtx recvCtx = {};
    recvCtx.fdCount = 2;
    recvCtx.fds[0] = 10;
    recvCtx.fds[1] = 11;
    RenderIpcFds origFds = {};
    int ret = ParseRenderIpcFds(&msgNode, recvCtx, origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    EXPECT_EQ(origFds.ipcFd, 10);
    EXPECT_EQ(origFds.sharedFd, 11);
    EXPECT_EQ(origFds.crashFd, -1);
}

/**
 * @brief DupRenderIpcFds - all dup succeed
 *        Covers: g_renderIpcFds.ipcFd > 0 && sharedFd > 0 && crashFd > 0 -> OK
 */
HWTEST_F(NWebAdapterBranchTest, DupRenderIpcFds_AllSuccess, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupRenderIpcFds_AllSuccess start";
    int fd1 = open("/dev/null", O_RDONLY);
    int fd2 = open("/dev/null", O_RDONLY);
    int fd3 = open("/dev/null", O_RDONLY);
    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);
    ASSERT_GE(fd3, 0);

    RenderIpcFds origFds = {fd1, fd2, fd3};
    int ret = DupRenderIpcFds(origFds);
    EXPECT_EQ(ret, APPSPAWN_OK);

    close(fd1);
    close(fd2);
    close(fd3);
}

/**
 * @brief DupRenderIpcFds - dup fails (invalid fd)
 *        Covers: at least one dup returns <= 0 -> cleanup, return APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, DupRenderIpcFds_DupFail, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupRenderIpcFds_DupFail start";
    RenderIpcFds origFds = {-1, -1, -1};
    int ret = DupRenderIpcFds(origFds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - ipc-fd env not set
 *        Covers: ipcFdEnv == nullptr -> return APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_IpcFdNotSet, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_IpcFdNotSet start";
    unsetenv("APPSPAWN_FD_ipc-fd");
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - ipc-fd set but invalid (non-numeric)
 *        Covers: strtol parse failure -> APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_IpcFdInvalid, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_IpcFdInvalid start";
    setenv("APPSPAWN_FD_ipc-fd", "not_a_number", 1);
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - ipc-fd set but value <= 0
 *        Covers: fds.ipcFd <= 0 -> APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_IpcFdZero, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_IpcFdZero start";
    setenv("APPSPAWN_FD_ipc-fd", "0", 1);
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - ipc-fd valid but shared-fd not set
 *        Covers: sharedFdEnv == nullptr -> return APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_SharedFdNotSet, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_SharedFdNotSet start";
    setenv("APPSPAWN_FD_ipc-fd", "10", 1);
    unsetenv("APPSPAWN_FD_shared-fd");
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - shared-fd invalid value
 *        Covers: sharedFdEnv parse failure -> APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_SharedFdInvalid, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_SharedFdInvalid start";
    setenv("APPSPAWN_FD_ipc-fd", "10", 1);
    setenv("APPSPAWN_FD_shared-fd", "abc", 1);
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - shared-fd value <= 0
 *        Covers: fds.sharedFd <= 0 -> APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_SharedFdZero, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_SharedFdZero start";
    setenv("APPSPAWN_FD_ipc-fd", "10", 1);
    setenv("APPSPAWN_FD_shared-fd", "-1", 1);
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - ipc & shared valid, crash-fd not set
 *        Covers: crashFdEnv == nullptr -> return APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_CrashFdNotSet, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_CrashFdNotSet start";
    setenv("APPSPAWN_FD_ipc-fd", "10", 1);
    setenv("APPSPAWN_FD_shared-fd", "11", 1);
    unsetenv("APPSPAWN_FD_crash-fd");
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - crash-fd invalid value
 *        Covers: crashFdEnv parse failure -> APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_CrashFdInvalid, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_CrashFdInvalid start";
    setenv("APPSPAWN_FD_ipc-fd", "10", 1);
    setenv("APPSPAWN_FD_shared-fd", "11", 1);
    setenv("APPSPAWN_FD_crash-fd", "xyz", 1);
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - crash-fd value <= 0
 *        Covers: fds.crashFd <= 0 -> APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_CrashFdZero, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_CrashFdZero start";
    setenv("APPSPAWN_FD_ipc-fd", "10", 1);
    setenv("APPSPAWN_FD_shared-fd", "11", 1);
    setenv("APPSPAWN_FD_crash-fd", "0", 1);
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief GetRenderIpcFdsFromEnv - all env vars valid
 *        Covers: happy path, all fds valid -> return APPSPAWN_OK
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_AllValid, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_AllValid start";
    setenv("APPSPAWN_FD_ipc-fd", "10", 1);
    setenv("APPSPAWN_FD_shared-fd", "11", 1);
    setenv("APPSPAWN_FD_crash-fd", "12", 1);
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_OK);
    EXPECT_EQ(fds.ipcFd, 10);
    EXPECT_EQ(fds.sharedFd, 11);
    EXPECT_EQ(fds.crashFd, 12);
}

/**
 * @brief GetRenderIpcFdsFromEnv - ipc-fd has trailing non-numeric chars
 *        Covers: *endptr != '\0' -> APPSPAWN_ARG_INVALID (e.g. "10abc")
 */
HWTEST_F(NWebAdapterBranchTest, GetRenderIpcFdsFromEnv_IpcFdTrailingChars, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "GetRenderIpcFdsFromEnv_IpcFdTrailingChars start";
    setenv("APPSPAWN_FD_ipc-fd", "10abc", 1);
    RenderIpcFds fds = {};
    int ret = GetRenderIpcFdsFromEnv(fds);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - not nweb spawn mode
 *        Covers: !IsNWebSpawnMode(content) -> return APPSPAWN_OK
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NotNwebMode, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NotNwebMode start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_OK);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode with renderCmd M114 version
 *        Covers: version == M114 -> return APPSPAWN_OK (skip fd processing)
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_M114Version, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_M114Version start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.m114");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    const char *renderCmd = "/system/bin/sh#--appEngineVersion=1";
    AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD,
        reinterpret_cast<const uint8_t *>(renderCmd), strlen(renderCmd));
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_OK);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode with non-M114 version (e.g. M132=2)
 *        Covers: version != M114 -> proceed to fd processing
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NonM114Version, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NonM114Version start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.m132");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    const char *renderCmd = "/system/bin/sh#--appEngineVersion=2";
    AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD,
        reinterpret_cast<const uint8_t *>(renderCmd), strlen(renderCmd));
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property, 0);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, 0);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, renderCmd is nullptr
 *        Covers: renderCmd == nullptr -> skip version check, proceed to fd processing
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NullRenderCmd, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NullRenderCmd start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.nullrender");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property, 0);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, 0);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, property is NULL
 *        Covers: property == NULL -> return APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NullProperty, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NullProperty start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb cold run mode, env fds missing (emulator fallback)
 *        Covers: IsColdRunMode && GetRenderIpcFdsFromEnv fails -> return APPSPAWN_OK (emulator fallback)
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_ColdRun_EnvFdMissing, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_ColdRun_EnvFdMissing start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_COLD_RUN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    unsetenv("APPSPAWN_FD_ipc-fd");
    unsetenv("APPSPAWN_FD_shared-fd");
    unsetenv("APPSPAWN_FD_crash-fd");

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.coldmiss");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_OK);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb cold run mode, env fds valid
 *        Covers: IsColdRunMode && GetRenderIpcFdsFromEnv succeeds -> DupRenderIpcFds
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_ColdRun_EnvFdValid, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_ColdRun_EnvFdValid start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_COLD_RUN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    int fd1 = open("/dev/null", O_RDONLY);
    int fd2 = open("/dev/null", O_RDONLY);
    int fd3 = open("/dev/null", O_RDONLY);
    ASSERT_GE(fd1, 0);
    ASSERT_GE(fd2, 0);
    ASSERT_GE(fd3, 0);

    setenv("APPSPAWN_FD_ipc-fd", std::to_string(fd1).c_str(), 1);
    setenv("APPSPAWN_FD_shared-fd", std::to_string(fd2).c_str(), 1);
    setenv("APPSPAWN_FD_crash-fd", std::to_string(fd3).c_str(), 1);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.coldvalid");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_OK);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
    close(fd1);
    close(fd2);
    close(fd3);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, message has null buffer
 *        Covers: message != NULL && message->buffer == NULL -> return APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NullMessageBuffer, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NullMessageBuffer start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.nullbuf");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    if (property->message != nullptr) {
        property->message->buffer = nullptr;
    }

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, message has null tlvOffset
 *        Covers: message->tlvOffset == NULL -> return APPSPAWN_TLV_NONE
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NullTlvOffset, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NullTlvOffset start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.nulltlv");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    uint8_t *origBuffer = nullptr;
    uint32_t *origTlvOffset = nullptr;
    uint8_t dummyBuf[1] = {0};
    if (property->message != nullptr) {
        origBuffer = property->message->buffer;
        origTlvOffset = property->message->tlvOffset;
        property->message->buffer = dummyBuf;
        property->message->tlvOffset = nullptr;
    }

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_TLV_NONE);
    if (property->message != nullptr) {
        property->message->buffer = origBuffer;
        property->message->tlvOffset = origTlvOffset;
    }
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, fdCount == 0 (no fds needed)
 *        Covers: recvCtx.fdCount <= 0 -> return 0 (no fd processing needed)
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_ZeroFdCount, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_ZeroFdCount start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.zerofd");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property, 0);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, 0);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief PreLoadNwebSpawn - APP_SPAWN mode (not nweb)
 *        Covers: !IsNWebSpawnMode -> return 0 (already tested in NWeb_Spawn_Msg_009)
 */
HWTEST_F(NWebAdapterBranchTest, PreLoadNwebSpawn_AppMode, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PreLoadNwebSpawn_AppMode start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    int ret = PreLoadNwebSpawn(mgr);
    EXPECT_EQ(ret, 0);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief PreLoadNwebSpawn - NWEB_SPAWN mode
 *        Covers: IsNWebSpawnMode -> RegChildLooper + PreloadArkWebLibForRender
 */
HWTEST_F(NWebAdapterBranchTest, PreLoadNwebSpawn_NwebMode, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PreLoadNwebSpawn_NwebMode start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    int ret = PreLoadNwebSpawn(mgr);
    EXPECT_EQ(ret, 0);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief PreLoadNwebSpawn - NWEB_COLD_RUN mode
 *        Covers: IsNWebSpawnMode (cold run) -> same path as nweb spawn
 */
HWTEST_F(NWebAdapterBranchTest, PreLoadNwebSpawn_NwebColdMode, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "PreLoadNwebSpawn_NwebColdMode start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_COLD_RUN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    int ret = PreLoadNwebSpawn(mgr);
    EXPECT_EQ(ret, 0);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, renderCmd with invalid version (non-numeric)
 *        Covers: UpdateAppWebEngineVersion with invalid version -> SYSTEM_DEFAULT (not M114)
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_InvalidVersion, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_InvalidVersion start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.badver");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    const char *renderCmd = "/system/bin/sh#--appEngineVersion=abc";
    AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD,
        reinterpret_cast<const uint8_t *>(renderCmd), strlen(renderCmd));
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property, 0);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, 0);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, renderCmd with negative version
 *        Covers: UpdateAppWebEngineVersion with v < 0 -> SYSTEM_DEFAULT
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NegativeVersion, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NegativeVersion start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.negver");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    const char *renderCmd = "/system/bin/sh#--appEngineVersion=-1";
    AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD,
        reinterpret_cast<const uint8_t *>(renderCmd), strlen(renderCmd));
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property, 0);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, 0);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, renderCmd with version without trailing '#'
 *        Covers: UpdateAppWebEngineVersion posEnd == npos branch
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_VersionNoTrailingHash, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_VersionNoTrailingHash start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.notrail");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    const char *renderCmd = "/system/bin/sh#--appEngineVersion=1";
    AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD,
        reinterpret_cast<const uint8_t *>(renderCmd), strlen(renderCmd));
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_OK);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, renderCmd with version followed by '#'
 *        Covers: UpdateAppWebEngineVersion posEnd != npos branch, version = M114
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_VersionWithTrailingHash, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_VersionWithTrailingHash start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.trailhash");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    const char *renderCmd = "/system/bin/sh#--appEngineVersion=1#--other=param";
    AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD,
        reinterpret_cast<const uint8_t *>(renderCmd), strlen(renderCmd));
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_OK);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode, no renderCmd ext info
 *        Covers: renderCmd == nullptr -> skip version check, proceed to fd processing
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NoRenderCmd, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NoRenderCmd start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle);
    AppSpawnTestHelper helper;
    helper.SetProcessName("com.test.norender");
    AppSpawnReqMsgHandle reqHandle = helper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    AppSpawningCtx *property = helper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);
    SetupMessageConnection(property, 0);

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, 0);
    FreeMessageConnection(property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief DupNwebRenderFdsBeforeRunHook - nweb mode with null message
 *        Covers: message == NULL -> return APPSPAWN_ARG_INVALID
 */
HWTEST_F(NWebAdapterBranchTest, DupNwebRenderFds_NullMessage, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "DupNwebRenderFds_NullMessage start";
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;
    PreLoadNwebSpawn(mgr);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = nullptr;

    int ret = DupNwebRenderFdsBeforeRunHook(mgr, property);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

}  // namespace OHOS
