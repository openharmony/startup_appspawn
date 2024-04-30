/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnAppMgrTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief test for appspawn_manager.h
 *
 */
/**
 * @brief AppSpawnMgr
 *
 */
HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMgr_001, TestSize.Level0)
{
    int ret = 0;
    for (int i = 0; i < MODE_INVALID; i++) {
        AppSpawnMgr *mgr = CreateAppSpawnMgr(i);
        EXPECT_EQ(mgr != nullptr, 1);

        AppSpawnContent *content = GetAppSpawnContent();
        EXPECT_EQ(content != nullptr, 1);
        EXPECT_EQ(content->mode == static_cast<RunMode>(i), 1);

        if (i == MODE_FOR_NWEB_SPAWN || i == MODE_FOR_NWEB_COLD_RUN) {
            ret = IsNWebSpawnMode(mgr);
            EXPECT_EQ(1, ret);  //  true
        } else {
            ret = IsNWebSpawnMode(mgr);
            EXPECT_EQ(0, ret);  //  false
        }

        if (i == MODE_FOR_APP_COLD_RUN || i == MODE_FOR_NWEB_COLD_RUN) {
            ret = IsColdRunMode(mgr);
            EXPECT_EQ(1, ret);  //  true
        } else {
            ret = IsColdRunMode(mgr);
            EXPECT_EQ(0, ret);  //  false
        }

        // get
        mgr = GetAppSpawnMgr();
        EXPECT_EQ(mgr != nullptr, 1);

        // delete
        DeleteAppSpawnMgr(mgr);

        // get not exist
        mgr = GetAppSpawnMgr();
        EXPECT_EQ(mgr == nullptr, 1);

        // get not exist
        content = GetAppSpawnContent();
        EXPECT_EQ(content == nullptr, 1);

        ret = IsColdRunMode(mgr);
        EXPECT_EQ(0, ret);  //  false
        ret = IsNWebSpawnMode(mgr);
        EXPECT_EQ(0, ret);  //  false

        // delete not exist
        DeleteAppSpawnMgr(mgr);
    }
}

/**
 * @brief AppSpawnedProcess
 *
 */
static void TestAppTraversal(const AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data)
{
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnedProcess_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(0);
    EXPECT_EQ(mgr != nullptr, 1);
    const size_t processNameCount = 3;
    const size_t pidCount = 4;
    const size_t resultCount = processNameCount * pidCount;
    const char *processNameInput[processNameCount] = {nullptr, "aaaaaa", ""};  // 3 size
    pid_t pidInput[pidCount] = {0, 100, 1000, -100};                           // 4 size
    int result[resultCount] = {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0};
    for (size_t i = 0; i < processNameCount; i++) {
        for (size_t j = 0; j < pidCount; j++) {
            AppSpawnedProcess *app = AddSpawnedProcess(pidInput[j], processNameInput[i]);
            EXPECT_EQ(app != nullptr, result[i * pidCount + j]);
        }
    }

    // Traversal
    TraversalSpawnedProcess(TestAppTraversal, nullptr);
    TraversalSpawnedProcess(TestAppTraversal, reinterpret_cast<void *>(mgr));
    TraversalSpawnedProcess(nullptr, nullptr);
    TraversalSpawnedProcess(nullptr, reinterpret_cast<void *>(mgr));

    // GetSpawnedProcess
    int resultGet[pidCount] = {0, 1, 1, 0};
    for (size_t j = 0; j < pidCount; j++) {
        AppSpawnedProcess *app = GetSpawnedProcess(pidInput[j]);
        EXPECT_EQ(app != nullptr, resultGet[j]);
    }

    // GetSpawnedProcessByName
    int resultGetByName[processNameCount] = {0, 1, 0};
    for (size_t i = 0; i < processNameCount; i++) {
        AppSpawnedProcess *app = GetSpawnedProcessByName(processNameInput[i]);
        EXPECT_EQ(app != nullptr, resultGetByName[i]);

        // delete app
        TerminateSpawnedProcess(app);
    }
    // delete not exist
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnedProcess_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    const size_t processNameCount = 3;
    const size_t pidCount = 4;
    const size_t resultCount = processNameCount * pidCount;
    const char *processNameInput[processNameCount] = {nullptr, "aaaaaa", ""};  // 3 size
    pid_t pidInput[pidCount] = {0, 100, 1000, -100};                           // 4 size
    int result[resultCount] = {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0};
    for (size_t i = 0; i < processNameCount; i++) {
        for (size_t j = 0; j < pidCount; j++) {
            AppSpawnedProcess *app = AddSpawnedProcess(pidInput[j], processNameInput[i]);
            EXPECT_EQ(app != nullptr, result[i * pidCount + j]);
        }
    }

    // Traversal
    TraversalSpawnedProcess(TestAppTraversal, nullptr);
    TraversalSpawnedProcess(TestAppTraversal, reinterpret_cast<void *>(mgr));
    TraversalSpawnedProcess(nullptr, nullptr);
    TraversalSpawnedProcess(nullptr, reinterpret_cast<void *>(mgr));

    // GetSpawnedProcess
    int resultGet[pidCount] = {0, 1, 1, 0};
    for (size_t j = 0; j < pidCount; j++) {
        AppSpawnedProcess *app = GetSpawnedProcess(pidInput[j]);
        EXPECT_EQ(app != nullptr, resultGet[j]);
    }

    // GetSpawnedProcessByName
    int resultGetByName[processNameCount] = {0, 1, 0};
    for (size_t i = 0; i < processNameCount; i++) {
        AppSpawnedProcess *app = GetSpawnedProcessByName(processNameInput[i]);
        EXPECT_EQ(app != nullptr, resultGetByName[i]);
        // delete app
        TerminateSpawnedProcess(app);
    }
    // delete not exist
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnedProcess_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    const char *processNameInput[] = {"1", "22", "333", "4444", "55555", "6666"};
    // GetSpawnedProcessByName
    size_t processNameCount = ARRAY_LENGTH(processNameInput);
    for (size_t i = 0; i < processNameCount; i++) {
        AppSpawnedProcess *app = AddSpawnedProcess(1000, processNameInput[i]); // 10000
        EXPECT_EQ(app != nullptr, 1);
    }
    for (size_t i = 0; i < processNameCount; i++) {
        AppSpawnedProcess *app = GetSpawnedProcessByName(processNameInput[i]);
        EXPECT_EQ(app != nullptr, 1);
        // delete app
        TerminateSpawnedProcess(app);
    }
    // delete not exist
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AppSpawningCtx
 *
 */
static void TestProcessTraversal(const AppSpawnMgr *mgr, AppSpawningCtx *ctx, void *data)
{
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_001, TestSize.Level0)
{
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawningCtx(nullptr);

    AppSpawnMgr *mgr = CreateAppSpawnMgr(0);
    EXPECT_EQ(mgr != nullptr, 1);
    appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);

    // GetAppSpawningCtxByPid
    appCtx->pid = 100;  // 100 test
    appCtx = GetAppSpawningCtxByPid(0);
    EXPECT_EQ(appCtx == nullptr, 1);
    appCtx = GetAppSpawningCtxByPid(100000);  // 100000 test
    EXPECT_EQ(appCtx == nullptr, 1);
    appCtx = GetAppSpawningCtxByPid(-2);  // -2 test
    EXPECT_EQ(appCtx == nullptr, 1);
    appCtx = GetAppSpawningCtxByPid(100);  // 100 test
    EXPECT_EQ(appCtx != nullptr, 1);

    AppSpawningCtxTraversal(TestProcessTraversal, reinterpret_cast<void *>(appCtx));
    AppSpawningCtxTraversal(nullptr, reinterpret_cast<void *>(appCtx));
    AppSpawningCtxTraversal(TestProcessTraversal, nullptr);
    AppSpawningCtxTraversal(nullptr, nullptr);

    appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    // delete not exist
    DeleteAppSpawnMgr(mgr);
    DeleteAppSpawningCtx(nullptr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_002, TestSize.Level0)
{
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawningCtx(nullptr);

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);

    // GetAppSpawningCtxByPid
    appCtx->pid = 100;  // 100 test
    appCtx = GetAppSpawningCtxByPid(0);
    EXPECT_EQ(appCtx == nullptr, 1);
    appCtx = GetAppSpawningCtxByPid(100000);  // 100000 test
    EXPECT_EQ(appCtx == nullptr, 1);
    appCtx = GetAppSpawningCtxByPid(-2);  // -2 test
    EXPECT_EQ(appCtx == nullptr, 1);
    appCtx = GetAppSpawningCtxByPid(100);  // 100 test
    EXPECT_EQ(appCtx != nullptr, 1);

    AppSpawningCtxTraversal(TestProcessTraversal, reinterpret_cast<void *>(appCtx));
    AppSpawningCtxTraversal(nullptr, reinterpret_cast<void *>(appCtx));
    AppSpawningCtxTraversal(TestProcessTraversal, nullptr);
    AppSpawningCtxTraversal(nullptr, nullptr);

    DeleteAppSpawningCtx(appCtx);
    // delete not exist
    DeleteAppSpawnMgr(mgr);
    DeleteAppSpawningCtx(nullptr);
}

/**
 * @brief AppSpawnMsgNode
 *
 */
HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_001, TestSize.Level0)
{
    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    EXPECT_EQ(msgNode != nullptr, 1);
    int ret = CheckAppSpawnMsg(msgNode);
    EXPECT_NE(0, ret);  // check fail

    // delete
    DeleteAppSpawnMsg(msgNode);
    DeleteAppSpawnMsg(nullptr);

    // get from buffer
    std::vector<uint8_t> buffer(16);  // 16
    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    const int inputCount = 2;                                           // 2 test
    const uint8_t *inputBuffer[inputCount] = {nullptr, buffer.data()};  // 2 test
    uint32_t *inputMsgLen[inputCount] = {nullptr, &msgRecvLen};
    uint32_t *inputReminder[inputCount] = {nullptr, &reminder};
    int result[inputCount * inputCount * inputCount] = {0};
    result[7] = 1;
    for (int i = 0; i < inputCount; i++) {
        for (int j = 0; j < inputCount; j++) {
            for (int k = 0; k < inputCount; k++) {
                ret = GetAppSpawnMsgFromBuffer(inputBuffer[i], buffer.size(),
                    &outMsg, inputMsgLen[j], inputReminder[k]);
                EXPECT_EQ(ret == 0, result[i * inputCount * inputCount + j * inputCount + k]);  // check fail
                DeleteAppSpawnMsg(outMsg);
            }
        }
    }
    for (int i = 0; i < inputCount; i++) {
        for (int j = 0; j < inputCount; j++) {
            for (int k = 0; k < inputCount; k++) {
                ret = GetAppSpawnMsgFromBuffer(inputBuffer[i], buffer.size(),
                    nullptr, inputMsgLen[j], inputReminder[k]);
                EXPECT_NE(0, ret);  // check fail
            }
        }
    }

    ret = DecodeAppSpawnMsg(nullptr);
    EXPECT_NE(0, ret);
    ret = CheckAppSpawnMsg(nullptr);
    EXPECT_NE(0, ret);
    DeleteAppSpawnMsg(nullptr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_002, TestSize.Level0)
{
    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 + sizeof(AppSpawnMsg));  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    // copy msg header
    ret = memcpy_s(buffer.data() + msgLen, sizeof(AppSpawnMsg), buffer.data(), sizeof(AppSpawnMsg));
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    // 测试部分头信息
    // only msg type
    uint32_t currLen = sizeof(uint32_t) + sizeof(uint32_t);
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), currLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(currLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data(), &outMsg->msgHeader, currLen), 0);
    // continue msg
    ret = GetAppSpawnMsgFromBuffer(buffer.data() + currLen, sizeof(uint32_t), &outMsg, &msgRecvLen, &reminder);
    currLen += sizeof(uint32_t);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(currLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data(), &outMsg->msgHeader, currLen), 0);
    EXPECT_EQ(0, reminder);

    // end msg header
    ret = GetAppSpawnMsgFromBuffer(buffer.data() + currLen,
        sizeof(AppSpawnMsg) - currLen, &outMsg, &msgRecvLen, &reminder);
    currLen = sizeof(AppSpawnMsg);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(currLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data(), &outMsg->msgHeader, currLen), 0);
    EXPECT_EQ(0, reminder);

    // reminder msg + next header
    ret = GetAppSpawnMsgFromBuffer(buffer.data() + currLen, msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(sizeof(AppSpawnMsg), reminder);
    DeleteAppSpawnMsg(outMsg);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_003, TestSize.Level0)
{
    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 + sizeof(AppSpawnMsg));  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);
    // copy msg header
    ret = memcpy_s(buffer.data() + msgLen, sizeof(AppSpawnMsg), buffer.data(), sizeof(AppSpawnMsg));
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    // 测试部分头信息
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen + sizeof(AppSpawnMsg), &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(sizeof(AppSpawnMsg), reminder);

    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    ret = CheckAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    DeleteAppSpawnMsg(outMsg);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_004, TestSize.Level0)
{
    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;

    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);

    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    ret = CheckAppSpawnMsg(outMsg);
    EXPECT_NE(0, ret);
    DeleteAppSpawnMsg(outMsg);
}

static int AddRenderTerminationTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount)
{
    // add app flage
    uint32_t currLen = 0;
    AppSpawnTlv tlv = {};
    tlv.tlvType = TLV_RENDER_TERMINATION_INFO;
    pid_t pid = 9999999; // 9999999 test
    tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(pid);

    int ret = memcpy_s(buffer, bufferLen, &tlv, sizeof(tlv));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    ret = memcpy_s(buffer + sizeof(tlv), bufferLen - sizeof(tlv), &pid, tlv.tlvLen - sizeof(tlv));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    currLen += tlv.tlvLen;
    tlvCount++;
    realLen = currLen;
    return 0;
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_GET_RENDER_TERMINATION_STATUS, msgLen, {AddRenderTerminationTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);

    AppSpawnedProcess *app = AddSpawnedProcess(9999999, "aaaa"); // 9999999 test
    EXPECT_EQ(app != nullptr, 1);
    TerminateSpawnedProcess(app);

    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    AppSpawnResult result = {};
    // app exist
    ret = ProcessTerminationStatusMsg(outMsg, &result);
    EXPECT_EQ(0, ret);

    ret = ProcessTerminationStatusMsg(nullptr, &result);
    EXPECT_NE(0, ret);

    ret = ProcessTerminationStatusMsg(outMsg, nullptr);
    EXPECT_NE(0, ret);

    ret = ProcessTerminationStatusMsg(nullptr, nullptr);
    EXPECT_NE(0, ret);
    DeleteAppSpawnMsg(outMsg);

    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_GET_RENDER_TERMINATION_STATUS, msgLen, {AddRenderTerminationTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);

    AppSpawnedProcess *app = AddSpawnedProcess(9999999, "aaaa"); // 9999999 test
    EXPECT_EQ(app != nullptr, 1);

    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    AppSpawnResult result = {};
    // die app not exist
    ret = ProcessTerminationStatusMsg(outMsg, &result);
    EXPECT_EQ(0, ret);

    DeleteAppSpawnMsg(outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_007, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_GET_RENDER_TERMINATION_STATUS, msgLen, {AddRenderTerminationTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);

    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    AppSpawnResult result = {};
    // app not exist
    ret = ProcessTerminationStatusMsg(outMsg, &result);
    EXPECT_EQ(0, ret);

    DeleteAppSpawnMsg(outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_008, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_DUMP, msgLen, {});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);

    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    ProcessAppSpawnDumpMsg(outMsg);
    ProcessAppSpawnDumpMsg(nullptr);

    DeleteAppSpawnMsg(outMsg);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief 消息内容操作接口
 *
 */
HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsg_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    // get msg info
    int inputTlv[13] = {0, 1, 2, 3, 4, 5, 6, 7, 8, TLV_MAX, TLV_MAX + 1, TLV_MAX + 2, -1}; // 13 test
    int result[13] = {1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0}; // 13 test
    for (size_t i = 0; i < ARRAY_LENGTH(inputTlv); i++) {
        void *info = GetAppSpawnMsgInfo(outMsg, i);
        EXPECT_EQ(info != nullptr, result[i]);
    }

    for (size_t i = 0; i < ARRAY_LENGTH(inputTlv); i++) {
        void *info = GetAppSpawnMsgInfo(nullptr, i);
        EXPECT_EQ(info == nullptr, 1);
    }
    DeleteAppSpawnMsg(outMsg);
    DeleteAppSpawnMgr(mgr);
}

static int AddTest001ExtTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount)
{
    const char *testData = "555555555555555555555555555";
    uint32_t currLen = 0;
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    int ret = strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), "test-001");
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy");
    tlv.dataLen = strlen(testData) + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);

    ret = memcpy_s(buffer, bufferLen, &tlv, sizeof(tlv));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    ret = memcpy_s(buffer + sizeof(tlv), bufferLen - sizeof(tlv), testData, tlv.dataLen + 1);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    currLen += tlv.tlvLen;
    tlvCount++;
    realLen = currLen;
    return 0;
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsg_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);  // 1024 * 2  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer,
        MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv, AddTest001ExtTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    // get msg ext info
    const int inputCount = 5;
    const char *inputName[inputCount] = {nullptr, "1", "22", "test-001", ""};
    int result[inputCount] = {0, 0, 0, 1, 0 };
    for (int i = 0; i < inputCount; i++) {
        uint32_t len = 0;
        void *info = GetAppSpawnMsgExtInfo(outMsg, inputName[i], &len);
        EXPECT_EQ(info != nullptr, result[i]);
    }
    for (int i = 0; i < inputCount; i++) {
        void *info = GetAppSpawnMsgExtInfo(outMsg, inputName[i], nullptr);
        EXPECT_EQ(info != nullptr, result[i]);
    }
    for (int i = 0; i < inputCount; i++) {
        uint32_t len = 0;
        void *info = GetAppSpawnMsgExtInfo(nullptr, inputName[i], &len);
        EXPECT_EQ(info == nullptr, 1);
    }
    DeleteAppSpawnMsg(outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsg_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    const int inputCount = 4;
    uint32_t inputType[inputCount] = {0, TLV_MSG_FLAGS, TLV_PERMISSION, TLV_MAX};
    int result[inputCount] = {0, 1, 1, 0};

    for (int i = 0; i < inputCount; i++) {
        for (int j = 0; j < 32; j++) { // max index 32
            ret = SetAppSpawnMsgFlag(outMsg, inputType[i], j);
            EXPECT_EQ(result[i], ret == 0);
            ret = CheckAppSpawnMsgFlag(outMsg, inputType[i], j);
            EXPECT_EQ(result[i], ret);
        }
    }
    for (int i = 0; i < inputCount; i++) {
        for (int j = 32; j < MAX_FLAGS_INDEX + 5; j++) { // 5 test
            ret = SetAppSpawnMsgFlag(outMsg, inputType[i], j);
            EXPECT_EQ(0, ret == 0);
            ret = CheckAppSpawnMsgFlag(outMsg, inputType[i], j);
            EXPECT_EQ(0, ret);
        }
    }
    for (int i = 0; i < inputCount; i++) {
        for (int j = 0; j < MAX_FLAGS_INDEX; j++) {
            ret = SetAppSpawnMsgFlag(nullptr, inputType[i], j);
            EXPECT_EQ(0, ret == 0);
            ret = CheckAppSpawnMsgFlag(nullptr, inputType[i], j);
            EXPECT_EQ(0, ret);
        }
    }
    DeleteAppSpawnMsg(outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsg_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    // dump msg
    DumpAppSpawnMsg(outMsg);
    DumpAppSpawnMsg(nullptr);
    DeleteAppSpawnMsg(outMsg);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AppSpawningCtx AppSpawnMsg
 *
 */
HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    appCtx->message = outMsg;
    int msgType = GetAppSpawnMsgType(appCtx);
    EXPECT_EQ(msgType, MSG_APP_SPAWN);
    outMsg->msgHeader.msgType = MSG_GET_RENDER_TERMINATION_STATUS;
    msgType = GetAppSpawnMsgType(appCtx);
    EXPECT_EQ(msgType, MSG_GET_RENDER_TERMINATION_STATUS);
    outMsg->msgHeader.msgType = MSG_SPAWN_NATIVE_PROCESS;
    msgType = GetAppSpawnMsgType(appCtx);
    EXPECT_EQ(msgType, MSG_SPAWN_NATIVE_PROCESS);
    msgType = GetAppSpawnMsgType(nullptr);
    EXPECT_EQ(msgType, MAX_TYPE_INVALID);

    // GetBundleName
    const char *bundleName = GetBundleName(appCtx);
    EXPECT_NE(nullptr, bundleName);
    bundleName = GetBundleName(nullptr);
    EXPECT_EQ(nullptr, bundleName);

    // IsDeveloperModeOn
    ret = IsDeveloperModeOn(appCtx);
    EXPECT_EQ(ret, 0);
    appCtx->client.flags |= APP_DEVELOPER_MODE;
    ret = IsDeveloperModeOn(appCtx);
    EXPECT_EQ(ret, 1);
    ret = IsDeveloperModeOn(nullptr);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    appCtx->message = outMsg;

    // GetBundleName
    const char *name = GetProcessName(appCtx);
    EXPECT_NE(nullptr, name);
    name = GetProcessName(nullptr);
    EXPECT_EQ(nullptr, name);

    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    appCtx->message = outMsg;

    for (int j = 0; j < 32; j++) { // max index 32
        ret = SetAppPermissionFlags(appCtx, j);
        EXPECT_EQ(1, ret == 0);
        ret = CheckAppPermissionFlagSet(appCtx, j);
        EXPECT_EQ(1, ret);
    }
    for (int j = 32; j < MAX_FLAGS_INDEX + 5; j++) { // 32 5 test
        ret = SetAppPermissionFlags(appCtx, j);
        EXPECT_NE(0, ret);
        ret = CheckAppPermissionFlagSet(appCtx, j);
        EXPECT_EQ(0, ret);
    }
    for (int j = 0; j < MAX_FLAGS_INDEX; j++) {
        ret = SetAppPermissionFlags(nullptr, j);
        EXPECT_NE(0, ret);
        ret = CheckAppPermissionFlagSet(nullptr, j);
        EXPECT_EQ(0, ret);
    }
    uint32_t result[MAX_FLAGS_INDEX] = {0};
    result[0] = 0;
    result[1] = 1;
    result[3] = 1;
    for (int j = 0; j < MAX_FLAGS_INDEX; j++) {
        ret = CheckAppMsgFlagsSet(appCtx, j);
        EXPECT_EQ(result[j], ret);
    }
    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    appCtx->message = outMsg;

    // get msg info
    int inputTlv[13] = {0, 1, 2, 3, 4, 5, 6, 7, 8, TLV_MAX, TLV_MAX + 1, TLV_MAX + 2, -1}; // 13 test
    int result[13] = {1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0}; // 13 test
    for (size_t i = 0; i < ARRAY_LENGTH(inputTlv); i++) {
        void *info = GetAppProperty(appCtx, i);
        EXPECT_EQ(info != nullptr, result[i]);
    }

    for (size_t i = 0; i < ARRAY_LENGTH(inputTlv); i++) {
        void *info = GetAppProperty(nullptr, i);
        EXPECT_EQ(info == nullptr, 1);
    }
    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);  // 1024 * 2  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer,
        MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv, AddTest001ExtTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    appCtx->message = outMsg;

    // get msg ext info
    const int inputCount = 5;
    const char *inputName[inputCount] = {nullptr, "1", "22", "test-001", ""};
    int result[inputCount] = {0, 0, 0, 1, 0 };
    for (int i = 0; i < inputCount; i++) {
        uint32_t len = 0;
        void *info = GetAppPropertyExt(appCtx, inputName[i], &len);
        EXPECT_EQ(info != nullptr, result[i]);
    }
    for (int i = 0; i < inputCount; i++) {
        void *info = GetAppPropertyExt(appCtx, inputName[i], nullptr);
        EXPECT_EQ(info != nullptr, result[i]);
    }
    for (int i = 0; i < inputCount; i++) {
        uint32_t len = 0;
        void *info = GetAppPropertyExt(nullptr, inputName[i], &len);
        EXPECT_EQ(info == nullptr, 1);
    }
    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}

HWTEST(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppSpawnTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.CreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {AppSpawnTestHelper::AddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);
    appCtx->message = outMsg;

    EXPECT_EQ(CheckAppSpawnMsgFlag(outMsg, TLV_MSG_FLAGS, APP_FLAGS_DEVELOPER_MODE), 0);
    EXPECT_EQ(SetAppSpawnMsgFlag(outMsg, TLV_MSG_FLAGS, APP_FLAGS_DEVELOPER_MODE), 0);
    EXPECT_EQ(CheckAppSpawnMsgFlag(outMsg, TLV_MSG_FLAGS, APP_FLAGS_DEVELOPER_MODE), 1);

    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}
}  // namespace OHOS
