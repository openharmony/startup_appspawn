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
#include <string>
#include <unistd.h>
#include <gtest/gtest.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_service.h"
#include "appspawn_utils.h"
#include "child_process_api.h"
#include "native_adapter_mock_test.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

APPSPAWN_STATIC int BuildFdInfoMap(const AppSpawnMsgNode *message, std::map<std::string, int> &fdMap, int isColdRun);
APPSPAWN_STATIC int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client);
APPSPAWN_STATIC int PreLoadNativeSpawn(AppSpawnMgr *content);

namespace OHOS {
class NativeSpawnAdapterBranchTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());
        SetBoolParamResult("persist.init.debug.checkexit", false);
        OHOS::AppExecFwk::ChildProcessApi::Reset();
    }
    void TearDown()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());
        SetBoolParamResult("persist.init.debug.checkexit", false);
        unsetenv(APPSPAWN_CHECK_EXIT);
    }
};

static void TestChildProcessWithMode(int32_t mode)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(mode);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = CreateAppSpawnMsg();
    ASSERT_NE(property->message, nullptr);

    if (property->message->buffer != nullptr) {
        free(property->message->buffer);
    }
    property->message->buffer = static_cast<uint8_t *>(calloc(64, sizeof(uint8_t)));
    ASSERT_NE(property->message->buffer, nullptr);
    uint32_t totalCount = TLV_MAX + 1;
    if (property->message->tlvOffset != nullptr) {
        free(property->message->tlvOffset);
    }
    property->message->tlvOffset = static_cast<uint32_t *>(calloc(totalCount, sizeof(uint32_t)));
    ASSERT_NE(property->message->tlvOffset, nullptr);
    for (uint32_t i = 0; i < totalCount; i++) {
        property->message->tlvOffset[i] = INVALID_OFFSET;
    }
    AppSpawnTlv *tlv = reinterpret_cast<AppSpawnTlv *>(property->message->buffer);
    tlv->tlvType = TLV_MSG_FLAGS;
    tlv->tlvLen = sizeof(AppSpawnTlv) + sizeof(uint32_t) + sizeof(uint32_t);
    AppSpawnMsgFlags *msgFlags = reinterpret_cast<AppSpawnMsgFlags *>(
        property->message->buffer + sizeof(AppSpawnTlv));
    msgFlags->count = 1;
    msgFlags->flags[0] = (1u << APP_FLAGS_CHILDPROCESS);
    property->message->tlvOffset[TLV_MSG_FLAGS] = 0;
    property->message->tlvCount = 0;

    int ret = RunChildProcessor(&mgr->content, &property->client);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(OHOS::AppExecFwk::ChildProcessApi::WasStartChildCalled());
}

/**
 * @tc.name: Native_Spawn_BuildFdInfoMap_TypeMax_NameMismatch
 * @tc.desc: Test BuildFdInfoMap with tlvType=TLV_MAX and tlvName not AppFd, should skip via continue
 * @tc.type: FUNC
 */
HWTEST_F(NativeSpawnAdapterBranchTest, Native_Spawn_BuildFdInfoMap_TypeMax_NameMismatch, TestSize.Level0)
{
    uint8_t buffer[512] = {0};
    uint32_t tlvOffsets[TLV_MAX + 1];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    AppSpawnTlvExt *tlvExt = reinterpret_cast<AppSpawnTlvExt *>(buffer);
    tlvExt->tlvType = TLV_MAX;
    tlvExt->tlvLen = sizeof(AppSpawnTlvExt) + 8;
    tlvExt->dataLen = 0;
    tlvExt->dataType = 0;
    ASSERT_GT(sprintf_s(tlvExt->tlvName, sizeof(tlvExt->tlvName), "%s", "NotAppFd"), 0);
    char *keyPtr = reinterpret_cast<char *>(buffer + sizeof(AppSpawnTlvExt));
    ASSERT_GT(sprintf_s(keyPtr, 5, "%s", "test"), 0);
    tlvOffsets[TLV_MAX] = 0;
    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = nullptr;
    msg.tlvCount = 1;

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, true);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(fdMap.size(), 0u);
}

/**
 * @tc.name: Native_Spawn_BuildFdInfoMap_MultiFd_NoBreak
 * @tc.desc: Test BuildFdInfoMap with multiple AppFd TLVs and sufficient fds, should not break early
 * @tc.type: FUNC
 */
HWTEST_F(NativeSpawnAdapterBranchTest, Native_Spawn_BuildFdInfoMap_MultiFd_NoBreak, TestSize.Level0)
{
    uint8_t buffer[512] = {0};
    uint32_t tlvOffsets[TLV_MAX + 2];
    for (uint32_t i = 0; i < TLV_MAX; i++) {
        tlvOffsets[i] = 0;
    }
    uint32_t secondOffset = sizeof(AppSpawnTlvExt) + 4;

    AppSpawnTlvExt *tlv1 = reinterpret_cast<AppSpawnTlvExt *>(buffer);
    tlv1->tlvType = TLV_MAX;
    tlv1->tlvLen = sizeof(AppSpawnTlvExt) + 4;
    tlv1->dataLen = 0;
    tlv1->dataType = 0;
    ASSERT_GT(sprintf_s(tlv1->tlvName, sizeof(tlv1->tlvName), "%s", MSG_EXT_NAME_APP_FD), 0);
    char *key1Ptr = reinterpret_cast<char *>(buffer + sizeof(AppSpawnTlvExt));
    ASSERT_GT(sprintf_s(key1Ptr, 4, "%s", "fd1"), 0);

    AppSpawnTlvExt *tlv2 = reinterpret_cast<AppSpawnTlvExt *>(buffer + secondOffset);
    tlv2->tlvType = TLV_MAX;
    tlv2->tlvLen = sizeof(AppSpawnTlvExt) + 4;
    tlv2->dataLen = 0;
    tlv2->dataType = 0;
    ASSERT_GT(sprintf_s(tlv2->tlvName, sizeof(tlv2->tlvName), "%s", MSG_EXT_NAME_APP_FD), 0);
    char *key2Ptr = reinterpret_cast<char *>(buffer + secondOffset + sizeof(AppSpawnTlvExt));
    ASSERT_GT(sprintf_s(key2Ptr, 4, "%s", "fd2"), 0);

    tlvOffsets[TLV_MAX] = 0;
    tlvOffsets[TLV_MAX + 1] = secondOffset;

    AppSpawnConnection connection = {};
    int fd1 = dup(1);
    int fd2 = dup(1);
    int fd3 = dup(1);
    ASSERT_GT(fd1, 0);
    ASSERT_GT(fd2, 0);
    ASSERT_GT(fd3, 0);
    connection.receiverCtx.fdCount = 3;
    connection.receiverCtx.fds[0] = fd1;
    connection.receiverCtx.fds[1] = fd2;
    connection.receiverCtx.fds[2] = fd3;

    AppSpawnMsgNode msg = {};
    msg.buffer = buffer;
    msg.tlvOffset = tlvOffsets;
    msg.connection = &connection;
    msg.tlvCount = 2;

    std::map<std::string, int> fdMap;
    int ret = BuildFdInfoMap(&msg, fdMap, false);
    close(fd1);
    close(fd2);
    close(fd3);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(fdMap.size(), 2u);
    EXPECT_NE(fdMap.find("fd1"), fdMap.end());
    EXPECT_EQ(fdMap["fd1"], fd1);
    EXPECT_NE(fdMap.find("fd2"), fdMap.end());
    EXPECT_EQ(fdMap["fd2"], fd2);
}

/**
 * @tc.name: Native_Spawn_RunChildThread_GetBoolParam_True
 * @tc.desc: Test RunChildThread with GetBoolParameter returning true, should set checkExit
 * @tc.type: FUNC
 */
HWTEST_F(NativeSpawnAdapterBranchTest, Native_Spawn_RunChildThread_GetBoolParam_True, TestSize.Level0)
{
    SetBoolParamResult("persist.init.debug.checkexit", true);
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
    EXPECT_FALSE(OHOS::AppExecFwk::ChildProcessApi::WasStartChildCalled());
}

/**
 * @tc.name: Native_Spawn_RunChildThread_ChildProcess_ColdRun
 * @tc.desc: Test RunChildThread with APP_FLAGS_CHILDPROCESS in cold run mode, should call StartChild
 * @tc.type: FUNC
 */
HWTEST_F(NativeSpawnAdapterBranchTest, Native_Spawn_RunChildThread_ChildProcess_ColdRun, TestSize.Level0)
{
    TestChildProcessWithMode(MODE_FOR_NATIVE_COLD_RUN);
}

/**
 * @tc.name: Native_Spawn_RunChildThread_ChildProcess_NonColdRun
 * @tc.desc: Test RunChildThread with APP_FLAGS_CHILDPROCESS in non-cold run mode, should call StartChild
 * @tc.type: FUNC
 */
HWTEST_F(NativeSpawnAdapterBranchTest, Native_Spawn_RunChildThread_ChildProcess_NonColdRun, TestSize.Level0)
{
    TestChildProcessWithMode(MODE_FOR_NATIVE_SPAWN);
}
}   // namespace OHOS
