/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "appmgr_test_helper.h"

#include <csignal>
#include <cstring>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include <gtest/gtest.h>
#include "securec.h"

#include "appspawn.h"
#include "appspawn_adapter.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_utils.h"

#include "appspawn_server.h"
#include "appspawn_service.h"
#include "cJSON.h"

using namespace testing;
using namespace testing::ext;

//CloseFdArgsFromConnection is declared APPSPAWN_STATIC in appspawn_service.c, only visible
//to the test binary (APPSPAWN_TEST strips static ). Forward-declare here for UT access.
extern "C" {
void CloseFdArgsFromConnection(AppSpawnConnection *connection);
void ProcessSignal(const struct signalfd_siginfo *siginfo);
int WriteMsgToChild(AppSpawningCtx *property, RunMode mode);
int SetPreforkProcessName(AppSpawnContent *content);
void ClearPipeFd(int pipe[], int length);
void ClearMMAP(int clientId, uint32_t memSize);
void ClearPreforkInfo(AppSpawningCtx *property);
int WritePreforkMsg(AppSpawningCtx *property, uint32_t memSize);
void ProcessCheckpointReqMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);
int AppspawpnDevicedebugKill(int pid, cJSON *args);
int AppspawnDevicedebugDeal(const char *op, int pid, cJSON *args);
int ProcessAppSpawnDeviceDebugMsg(AppSpawnMsgNode *message);
int AppSpawnReqMsgFdGet(AppSpawnConnection *connection, AppSpawnMsgNode *message, const char *fdName, int *fd);
void ProcessObserveProcessSignalMsg(AppSpawnConnection *connection, AppSpawnMsgNode *message);
int SendUnlockMsgToPrefork(AppSpawnContent *content, int uid);
int SendPipeMsgToChild(AppSpawnMgr *mgr, pid_t childPid, AppSpawnPipeMsg *pipeMsg);
void CleanupPreforkChild(AppSpawnMgr *mgr, pid_t childPid);
int TransferPreforkFdToForkCtx(AppSpawnMgr *mgr, pid_t childPid, AppSpawningCtx *property);
int AppSpawnColdStartApp(struct AppSpawnContent *content, AppSpawnClient *client);
}

namespace OHOS {
constexpr uint32_t TEST_MSG_BUFFER_SIZE = 1024;  // buffer size for building a spawn message TLV payload
class AppSpawnAppMgrTest : public testing::Test {
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

typedef struct TagAppSpawnedProcess AppSpawnedProcessInfo;

/**
 * @brief AppSpawnMgr
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMgr_001, TestSize.Level0)
{
    for (int i = 0; i < MODE_INVALID; i++) {
        AppSpawnMgr *mgr = CreateAppSpawnMgr(i);
        EXPECT_EQ(mgr != nullptr, 1);

        AppSpawnContent *content = GetAppSpawnContent();
        EXPECT_EQ(content != nullptr, 1);
        EXPECT_EQ(content->mode == static_cast<RunMode>(i), 1);

        if (i == MODE_FOR_APP_SPAWN || i == MODE_FOR_APP_COLD_RUN) {
            EXPECT_EQ(1, IsAppSpawnMode(mgr));  //  true
        } else {
            EXPECT_EQ(0, IsAppSpawnMode(mgr));  //  false
        }

        if (i == MODE_FOR_NWEB_SPAWN || i == MODE_FOR_NWEB_COLD_RUN) {
            EXPECT_EQ(1, IsNWebSpawnMode(mgr)); //  true
        } else {
            EXPECT_EQ(0, IsNWebSpawnMode(mgr)); //  false
        }

        if (i == MODE_FOR_NATIVE_SPAWN || i == MODE_FOR_NATIVE_COLD_RUN) {
            EXPECT_EQ(1, IsNativeSpawnMode(mgr));   //  true
        } else {
            EXPECT_EQ(0, IsNativeSpawnMode(mgr));   //  false
        }

        if (i == MODE_FOR_HYBRID_SPAWN || i == MODE_FOR_HYBRID_COLD_RUN) {
            EXPECT_EQ(1, IsHybridSpawnMode(mgr));   //  true
        } else {
            EXPECT_EQ(0, IsHybridSpawnMode(mgr));   //  false
        }

        if (i == MODE_FOR_APP_COLD_RUN || i == MODE_FOR_NWEB_COLD_RUN || i == MODE_FOR_HYBRID_COLD_RUN ||
            i == MODE_FOR_CJAPP_COLD_RUN || i == MODE_FOR_NATIVE_COLD_RUN) {
            EXPECT_EQ(1, IsColdRunMode(mgr));   //  true
        } else {
            EXPECT_EQ(0, IsColdRunMode(mgr));   //  false
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

        EXPECT_EQ(0, IsColdRunMode(mgr));   //  false
        EXPECT_EQ(0, IsAppSpawnMode(mgr));  //  false
        EXPECT_EQ(0, IsNWebSpawnMode(mgr)); //  false
        EXPECT_EQ(0, IsNativeSpawnMode(mgr));   //  false
        EXPECT_EQ(0, IsHybridSpawnMode(mgr));   //  false

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
    APPSPAWN_LOGI("TraversalSpawnedProcess test");
}
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnedProcess_001, TestSize.Level0)
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
            AppSpawnedProcess *app = AddSpawnedProcess(pidInput[j], processNameInput[i], 0, false, 0);
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnedProcess_002, TestSize.Level0)
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
            AppSpawnedProcess *app = AddSpawnedProcess(pidInput[j], processNameInput[i], 0, false, 0);
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnedProcess_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    const char *processNameInput[] = {"1", "22", "333", "4444", "55555", "6666"};
    // GetSpawnedProcessByName
    size_t processNameCount = ARRAY_LENGTH(processNameInput);
    for (size_t i = 0; i < processNameCount; i++) {
        AppSpawnedProcess *app = AddSpawnedProcess(1000, processNameInput[i], 0, false, 0); // 10000
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
    APPSPAWN_LOGI("AppSpawningCtxTraversal test");
}
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_001, TestSize.Level0)
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_002, TestSize.Level0)
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
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_001, TestSize.Level0)
{
    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    EXPECT_EQ(msgNode != nullptr, 1);
    int ret = CheckAppSpawnMsg(msgNode);
    EXPECT_NE(0, ret);  // check fail

    // delete
    DeleteAppSpawnMsg(&msgNode);
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
                DeleteAppSpawnMsg(&outMsg);
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_002, TestSize.Level0)
{
    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 + sizeof(AppSpawnMsg));  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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
    DeleteAppSpawnMsg(&outMsg);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_003, TestSize.Level0)
{
    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 + sizeof(AppSpawnMsg));  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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
    DeleteAppSpawnMsg(&outMsg);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_004, TestSize.Level0)
{
    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {});
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
    DeleteAppSpawnMsg(&outMsg);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_GET_RENDER_TERMINATION_STATUS, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddRenderTerminationTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);

    AppSpawnedProcess *app = AddSpawnedProcess(9999999, "aaaa", 0, false, 0); // 9999999 test
    EXPECT_EQ(app != nullptr, 1);
    TerminateSpawnedProcess(app);
    AppSpawnExtData extData;
    OH_ListAddTail(&(mgr->extData), &(extData.node));
    ProcessAppSpawnDumpMsg(outMsg);

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
    DeleteAppSpawnMsg(&outMsg);

    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_GET_RENDER_TERMINATION_STATUS, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddRenderTerminationTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(msgLen, msgRecvLen);
    EXPECT_EQ(memcmp(buffer.data() + sizeof(AppSpawnMsg), outMsg->buffer, msgLen - sizeof(AppSpawnMsg)), 0);
    EXPECT_EQ(0, reminder);

    AppSpawnedProcess *app = AddSpawnedProcess(9999999, "aaaa", 0, false, 0); // 9999999 test
    EXPECT_EQ(app != nullptr, 1);

    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);
    AppSpawnResult result = {};
    // die app not exist
    ret = ProcessTerminationStatusMsg(outMsg, &result);
    EXPECT_EQ(0, ret);

    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_007, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_GET_RENDER_TERMINATION_STATUS, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddRenderTerminationTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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

    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_008, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_DUMP, msgLen, {});
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
    outMsg->tlvOffset = nullptr;
    ProcessAppSpawnDumpMsg(outMsg);

    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_009, TestSize.Level0)
{
    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    msgNode->buffer = static_cast<uint8_t *>(malloc(255));;
    msgNode->tlvOffset = static_cast<uint32_t *>(malloc(128));
    EXPECT_EQ(msgNode != nullptr, 1);
    DeleteAppSpawnMsg(&msgNode);
    EXPECT_EQ(msgNode, NULL);
    DeleteAppSpawnMsg(&msgNode);
    EXPECT_EQ(msgNode, NULL);
    msgNode = CreateAppSpawnMsg();
    EXPECT_NE(msgNode, NULL);
    DeleteAppSpawnMsg(&msgNode);
    EXPECT_EQ(msgNode, NULL);
    DeleteAppSpawnMsg(nullptr);
}

/**
 * @brief 消息内容操作接口
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsg_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsg_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);  // 1024 * 2  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        },
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddExtTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsg_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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
        for (int j = 0; j < MAX_FLAGS_INDEX; j++) {
            ret = SetAppSpawnMsgFlag(nullptr, inputType[i], j);
            EXPECT_EQ(0, ret == 0);
            ret = CheckAppSpawnMsgFlag(nullptr, inputType[i], j);
            EXPECT_EQ(0, ret);
        }
    }
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsg_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief DumpFailedAppspawnMsg tlvOffset NULL boundary
* @note 预期结果：message 非空但 tlvOffset 为 NULL 时，打印 msgHeader 后返回，不发生崩溃
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_DumpFailedAppspawnMsg_002, TestSize.Level0)
{
    AppSpawnMsgNode *msg = CreateAppSpawnMsg();
    EXPECT_EQ(msg != nullptr, 1);
    //CreateAppSpawnMsg creates message with tlvOffset=NULL, buffer=NULL
    DumpFailedAppspawnMsg(msg, 0);
    DumpFailedAppspawnMsg(msg, -1);
    DeleteAppSpawnMsg(&msg);
}

/**
* @brief DumpFailedAppspawnMsg normal message with base TLV, ret = 0
* @note 预期结果：能正确打印 msgHeader 和扩展 TLV 信息（包含 base tlv 但无 ext tlv）
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_DumpFailedAppspawnMsg_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  //1024 max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(
        buffer, MSG_APP_SPAWN, msgLen, {AppMgrTestHelper::AppMgrTestAddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    //dump failed msg with ret=0 (no ext tlv in base case)
    DumpFailedAppspawnMsg(outMsg, 0);
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief DumpFailedAppspawnMsg normal message with error ret
* @note 预期结果：ret 为错误码（< 0）时也能正确打印失败诊断信息
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_DumpFailedAppspawnMsg_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024 max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen,
        {AppMgrTestHelper::AppMgrTestAddBaseTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    //dump failed msg with various error codes
    DumpFailedAppspawnMsg(outMsg, APPSPAWN_ARG_INVALID);
    DumpFailedAppspawnMsg(outMsg, APPSPAWN_MSG_INVALID);
    DumpFailedAppspawnMsg(outMsg, -1);
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief DumpFailedAppspawnMsg message with string ext TLV
* @note 预期结果：包含 dataType=DATA_TYPE_STRING 的 ext tlv 时，正确打印 key=value 信息
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_DumpFailedAppspawnMsg_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(2048); // 2048 buffer
    uint32_t msgLen = 0;
    //append an ext TLV with string data type after base TLV
    auto addStringExtTlv = [](uint8_t *buf, uint32_t bufLen, uint32_t &realLen, uint32_t &tlvCount) ->int {
        const char *value = "dump-ext-string-value";
        AppSpawnTlvExt tlv = {};
        tlv.tlvType = TLV_MAX;
        tlv.dataType = DATA_TYPE_STRING;
        tlv.dataLen = strlen(value) + 1;
        tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
        if (tlv.tlvLen > bufLen)
        {
            return -1;
        }
        int ret = strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), "dump-ext-str");
        if (ret != 0)
        {
            return -1;
        }
        ret = memcpy_s(buf, bufLen, &tlv, sizeof(tlv));
        if (ret != 0)
        {
            return -1;
        }
        ret = memcpy_s(buf + sizeof(tlv), bufLen - sizeof(tlv), value, tlv.dataLen);
        if (ret != 0)
        {
            return -1;
        }
        realLen = tlv.tlvLen;
        tlvCount = 1;
        return 0;
    };
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen,
        {AppMgrTestHelper::AppMgrTestAddBaseTlv, addStringExtTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    //covers tlv->dataType == DATA_TYPE_STRING branch
    DumpFailedAppspawnMsg(outMsg, APPSPAWN_ARG_INVALID);
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief DumpFailedAppspawnMsg message with non-string ext TLV
* @note 预期结果：包含 dataType!=DATA_TYPE_STRING 的 ext tlv 时，正确打印 key+len+type 信息
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_DumpFailedAppspawnMsg_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(2048); //2048 buffer
    uint32_t msgLen = 0;
    //append an ext TLV with non-string (binary) data type
    auto addBinaryExtTlv = [](uint8_t *buf, uint32_t bufLen, uint32_t &realLen, uint32_t &tlvCount) ->int {
        uint8_t value[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34, 0x56, 0x78};  // 8 bytes binary
        AppSpawnTlvExt tlv = {};
        tlv.tlvType = TLV_MAX;
        tlv.dataType = 0;  // non-string
        tlv.dataLen = sizeof(value);
        tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);
        if (tlv.tlvLen > bufLen) {
            return -1;
        }
        int ret = strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), "dump-ext-bin");
        if (ret != 0) {
            return -1;
        }
        ret = memcpy_s(buf, bufLen, &tlv, sizeof(tlv));
        if (ret != 0) {
            return -1;
        }
        ret = memcpy_s(buf + sizeof(tlv), bufLen - sizeof(tlv), value, tlv.dataLen);
        if (ret != 0) {
            return -1;
        }
        realLen = tlv.tlvLen;
        tlvCount = 1;
        return 0;
    };
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen,
    {AppMgrTestHelper::AppMgrTestAddBaseTlv, addBinaryExtTlv});
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    ret = DecodeAppSpawnMsg(outMsg);
    EXPECT_EQ(0, ret);

    // covers tlv->dataType != DATA_TYPE_STRING branch
    DumpFailedAppspawnMsg(outMsg, APPSPAWN_ARG_INVALID);
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief CloseFdArgsFromConnection empty fd list (fdCount <= 0)
* @note 预期结果：fdCount = 0时直接驳回，不关闭任何 fd
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_CloseFdArgsFromConnection_002, TestSize.Level0)
{
    AppSpawnConnection conn = {};
    conn.receiverCtx.fdCount = 0;
    CloseFdArgsFromConnection(&conn);
    EXPECT_EQ(conn.receiverCtx.fdCount, 0);

    // negative fdCount also safe (early return)
    conn.receiverCtx.fdCount = -1;
    CloseFdArgsFromConnection(&conn);
    EXPECT_EQ(conn.receiverCtx.fdCount, -1);  // unchanged because early returns
}

/**
* @brief CloseFdArgsFromConnection closes valid fds and resets fdCount
* @note 预期结果：fdCount > 0 时关闭所有 >0 的fd，并将 fdCount 置0
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_CloseFdArgsFromConnection_003, TestSize.Level0)
{
    int pipefd1[2] = {-1, -1};
    int pipefd2[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd1));
    ASSERT_EQ(0, pipe(pipefd2));

    AppSpawnConnection conn = {};
    conn.receiverCtx.fdCount = 2;
    conn.receiverCtx.fds[0] = pipefd1[0];
    conn.receiverCtx.fds[1] = pipefd2[0];

    // fds should be alive before close
    char c = 'x';
    EXPECT_EQ(1, write(pipefd1[1], &c, 1));

    CloseFdArgsFromConnection(&conn);

    // fdCount reset to 0
    EXPECT_EQ(conn.receiverCtx.fdCount, 0);
    // fds are closed and marked -1
    EXPECT_EQ(conn.receiverCtx.fds[0], -1);
    EXPECT_EQ(conn.receiverCtx.fds[1], -1);
    // peer ends still valid; close them to release resources
    close(pipefd1[1]);
    close(pipefd2[1]);
}

/**
* @brief CloseFdArgsFromConnection skips non-positive fds
* @note 预期结果：fd[i] <= 0 的项被跳过（不调用 close），但仍将其设为 -1，
*                 最后 fdCount 重置为 0
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_CloseFdArgsFromConnection_004, TestSize.Level0)
{
    int pipefd[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd));

    AppSpawnConnection conn = {};
    conn.receiverCtx.fdCount = 3;  //3 entries: one valid, one 0, one -2
    conn.receiverCtx.fds[0] = pipefd[0];
    conn.receiverCtx.fds[1] = 0;   // skipped (fd > 0 check fails)
    conn.receiverCtx.fds[2] = -2;  // skipped

    CloseFdArgsFromConnection(&conn);

    EXPECT_EQ(conn.receiverCtx.fdCount, 0);
    // all entries rewritten to -1 (only valid one was closed; invalid ones
    // remain unchanged in their slots per the source logic, but the loop
    // does fds[i] = -1 only after close, so skipped entries keep their value)
    EXPECT_EQ(conn.receiverCtx.fds[0], -1);
    // fds[1] and fds[2] were skipped, values unchanged
    EXPECT_EQ(conn.receiverCtx.fds[1], 0);
    EXPECT_EQ(conn.receiverCtx.fds[2], -2);
    close(pipefd[1]);
}

/**
* @brief CloseFdArgsFromConnection idempotency after prior close
* @note 预期结果：第二次调用时 fdCount 已为 0，直接返回（无副作用）
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_CloseFdArgsFromConnection_005, TestSize.Level0)
{
    int pipefd[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd));

    AppSpawnConnection conn = {};
    conn.receiverCtx.fdCount = 1;
    conn.receiverCtx.fds[0] = pipefd[0];

    CloseFdArgsFromConnection(&conn);
    EXPECT_EQ(conn.receiverCtx.fdCount, 0);

    // second call: fdCount=0, early return
    CloseFdArgsFromConnection(&conn);
    EXPECT_EQ(conn.receiverCtx.fdCount, 0);

    close(pipefd[1]);
}

//APPSPAWN_STATIC functions in appspawn_service.c are only visible to the test
//binary (APPSPAWN_TEST strips static). Forward-declare here for UT access.
extern "C" bool OnConnectionUserCheck(uid_t uid);
extern "C" char *GetSpawnNameByRunMode(RunMode mode);
extern "C" void WriteSignalInfoToFd(AppSpawnedProcess *appInfo, AppSpawnContent *content, int signal);
extern "C" void HandleDiedPid(pid_t pid, uid_t uid, int status);

/**
* @brief RegisterSpawningFds with invalid and valid reg info
* @note 预期结果：mgr/regInfo 为空、type 非法、count 为 0、fds 为空、count 超上限时
*                 返回 NULL；合法输入注册成功且 type/count/pid/fd 拷贝正确、可被查找
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_RegisterSpawningFds_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    int pipefd[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd));
    int fds[2] = {pipefd[0], pipefd[1]};
    SpawningFdRegInfo regInfo = {TYPE_CHILD_PARENT, 2, fds, 1234};  // 1234 test pid
    EXPECT_EQ(RegisterSpawningFds(nullptr, &regInfo), nullptr);
    EXPECT_EQ(RegisterSpawningFds(mgr, nullptr), nullptr);

    SpawningFdRegInfo invalid = regInfo;
    invalid.type = TYPE_INVALID;
    EXPECT_EQ(RegisterSpawningFds(mgr, &invalid), nullptr);
    invalid = regInfo;
    invalid.count = 0;
    EXPECT_EQ(RegisterSpawningFds(mgr, &invalid), nullptr);
    invalid = regInfo;
    invalid.fds = nullptr;
    EXPECT_EQ(RegisterSpawningFds(mgr, &invalid), nullptr);
    invalid = regInfo;
    invalid.count = MAX_SPAWNING_FDS_PER_NODE + 1;  // 9 exceed max 8
    EXPECT_EQ(RegisterSpawningFds(mgr, &invalid), nullptr);

    AppSpawnFds *spawnFds = RegisterSpawningFds(mgr, &regInfo);
    EXPECT_EQ(spawnFds != nullptr, 1);
    EXPECT_EQ(spawnFds->type, TYPE_CHILD_PARENT);
    EXPECT_EQ(spawnFds->count, 2);
    EXPECT_EQ(spawnFds->pid, 1234);
    EXPECT_EQ(spawnFds->fds[0], pipefd[0]);
    EXPECT_EQ(spawnFds->fds[1], pipefd[1]);
    EXPECT_EQ(FindSpawningFdsByPid(mgr, 1234, TYPE_CHILD_PARENT), spawnFds);

    //unregister closes both registered fds
    EXPECT_EQ(0, UnregisterSpawningFdsByPid(mgr, 1234, TYPE_CHILD_PARENT));
    char c = 0;
    EXPECT_EQ(-1, read(pipefd[0], &c, 1));
    EXPECT_EQ(-1, write(pipefd[1], &c, 1));
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief UnregisterSpawningFdsByPid with invalid and repeat calls
* @note 预期结果：mgr 为空或节点不存在时返回 APPSPAWN_ARG_INVALID；
*                 成功后 fd 被关闭、
*                 节点被移除，再次注销返回 APPSPAWN_ARG_INVALID
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_UnregisterSpawningFdsByPid_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    EXPECT_EQ(APPSPAWN_ARG_INVALID, UnregisterSpawningFdsByPid(mgr, 100, TYPE_CHILD_PARENT));
    EXPECT_EQ(APPSPAWN_ARG_INVALID, UnregisterSpawningFdsByPid(nullptr, 100, TYPE_CHILD_PARENT));

    int pipefd[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd));
    int fds[1] = {pipefd[0]};
    SpawningFdRegInfo regInfo = {TYPE_CHILD_PARENT, 1, fds, 100};  // 100 test pid
    EXPECT_EQ(RegisterSpawningFds(mgr, &regInfo) != nullptr, 1);
    EXPECT_EQ(FindSpawningFdsByPid(mgr, 100, TYPE_PARENT_CHILD), nullptr);  // type mismatch

    EXPECT_EQ(0, UnregisterSpawningFdsByPid(mgr, 100, TYPE_CHILD_PARENT));
    EXPECT_EQ(FindSpawningFdsByPid(mgr, 100, TYPE_CHILD_PARENT), nullptr);
    char c = 0;
    EXPECT_EQ(-1, read(pipefd[0], &c, 1));  // closed by unregister
    EXPECT_EQ(APPSPAWN_ARG_INVALID, UnregisterSpawningFdsByPid(mgr, 100, TYPE_CHILD_PARENT));
    close(pipefd[1]);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief RemoveSpawningFdsByPid and DeleteSpawningFds keep fds open
* @note 预期结果：按 pid 移除或按指针删除节点时 fd 不被关闭（所有权转移），
*                 pipe 仍然可用；DeleteSpawningFds 对空指针安全
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_RemoveSpawningFdsByPid_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    EXPECT_EQ(APPSPAWN_ARG_INVALID, RemoveSpawningFdsByPid(mgr, 100, TYPE_PARENT_CHILD));
    DeleteSpawningFds(nullptr);

    int pipefd[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd));
    int fds[1] = {pipefd[0]};
    SpawningFdRegInfo regInfo = {TYPE_PARENT_CHILD, 1, fds, 100};  // 100 test pid
    AppSpawnFds *spawnFds = RegisterSpawningFds(mgr, &regInfo);
    EXPECT_EQ(spawnFds != nullptr, 1);
    EXPECT_EQ(0, RemoveSpawningFdsByPid(mgr, 100, TYPE_PARENT_CHILD));
    EXPECT_EQ(FindSpawningFdsByPid(mgr, 100, TYPE_PARENT_CHILD), nullptr);

    //remove does not close fd, pipe is still usable
    char c = 'r';
    EXPECT_EQ(1, write(pipefd[1], &c, 1));
    EXPECT_EQ(1, read(pipefd[0], &c, 1));

    //delete by pointer also keeps fd open
    spawnFds = RegisterSpawningFds(mgr, &regInfo);
    EXPECT_EQ(spawnFds != nullptr, 1);
    AppSpawnFds *nullFds = nullptr;
    DeleteSpawningFds(&nullFds);
    DeleteSpawningFds(&spawnFds);
    EXPECT_EQ(spawnFds, nullptr);
    EXPECT_EQ(1, write(pipefd[1], &c, 1));
    close(pipefd[0]);
    close(pipefd[1]);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief CleanupSpawningFdsByPid cleans all nodes of target pid
* @note 预期结果：仅清理目标 pid 的全部节点并关闭 fd，其他 pid 的节点保留；
*                 GetSpawningFdsStats/DumpSpawningFds 统计与打印正确
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_CleanupSpawningFdsByPid_001, TestSize.Level0)
{
    EXPECT_EQ(0U, CleanupSpawningFdsByPid(nullptr, 100));
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    EXPECT_EQ(0U, CleanupSpawningFdsByPid(mgr, 100));  // empty queue

    int pipefd1[2] = {-1, -1};
    int pipefd2[2] = {-1, -1};
    int pipefd3[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd1));
    ASSERT_EQ(0, pipe(pipefd2));
    ASSERT_EQ(0, pipe(pipefd3));
    int fds1[1] = {pipefd1[0]};
    int fds2[1] = {pipefd2[0]};
    int fds3[1] = {pipefd3[0]};
    SpawningFdRegInfo regInfo1 = {TYPE_CHILD_PARENT, 1, fds1, 100};
    SpawningFdRegInfo regInfo2 = {TYPE_PARENT_CHILD, 1, fds2, 100};
    SpawningFdRegInfo regInfo3 = {TYPE_KILL_REASON_FD, 1, fds3, 200};  // 200 other pid
    EXPECT_EQ(RegisterSpawningFds(mgr, &regInfo1) != nullptr, 1);
    EXPECT_EQ(RegisterSpawningFds(mgr, &regInfo2) != nullptr, 1);
    EXPECT_EQ(RegisterSpawningFds(mgr, &regInfo3) != nullptr, 1);

    EXPECT_EQ(2U, CleanupSpawningFdsByPid(mgr, 100));  // 2 nodes belong to pid 100
    EXPECT_EQ(FindSpawningFdsByPid(mgr, 100, TYPE_CHILD_PARENT), nullptr);
    EXPECT_EQ(FindSpawningFdsByPid(mgr, 100, TYPE_PARENT_CHILD), nullptr);
    EXPECT_EQ(FindSpawningFdsByPid(mgr, 200, TYPE_KILL_REASON_FD) != nullptr, 1);
    char c = 0;
    EXPECT_EQ(-1, read(pipefd1[0], &c, 1));  // closed by cleanup
    EXPECT_EQ(-1, read(pipefd2[0], &c, 1));
    EXPECT_EQ(0U, CleanupSpawningFdsByPid(mgr, 100));  // no node left

    //stats and dump with the remaining node
    uint32_t total = 0;
    uint32_t childParentCount = 0;
    uint32_t parentChildCount = 0;
    GetSpawningFdsStats(mgr, &total, &childParentCount, &parentChildCount);
    EXPECT_EQ(1U, total);
    EXPECT_EQ(0U, childParentCount);
    EXPECT_EQ(0U, parentChildCount);
    GetSpawningFdsStats(mgr, nullptr, &childParentCount, &parentChildCount);  // null param safe
    DumpSpawningFds(nullptr);
    DumpSpawningFds(mgr);

    close(pipefd1[1]);
    close(pipefd2[1]);
    close(pipefd3[0]);
    close(pipefd3[1]);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief OnConnectionUserCheck with allowed and denied uids
* @note 预期结果：白名单 uid（root 0/app_fwk_update 3350/foundation 5523/
*                 storage_manager 1090）允许连接，其余 uid 拒绝
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_OnConnectionUserCheck_001, TestSize.Level0)
{
    const int allowCount = 4;
    const uid_t allowUid[allowCount] = {0, 3350, 5523, 1090};
    for (int i = 0; i < allowCount; i++) {
        EXPECT_EQ(true, OnConnectionUserCheck(allowUid[i]));
    }

    const int denyCount = 3;
    const uid_t denyUid[denyCount] = {1, 1234, 99999};  // not in allow list
    for (int i = 0; i < denyCount; i++) {
        EXPECT_EQ(false, OnConnectionUserCheck(denyUid[i]));
    }
}

/**
* @brief GetSpawnNameByRunMode for all run modes
* @note 预期结果：每种运行模式映射到对应的服务名，非法模式返回空串
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_GetSpawnNameByRunMode_001, TestSize.Level0)
{
    const int modeCount = MODE_INVALID;
    const char *expectName[modeCount] = {
        APPSPAWN_SERVER_NAME, NWEBSPAWN_SERVER_NAME,       // MODE_FOR_APP_SPAWN, MODE_FOR_NWEB_SPAWN
        APPSPAWN_SERVER_NAME, NWEBSPAWN_SERVER_NAME,       // MODE_FOR_APP_COLD_RUN, MODE_FOR_NWEB_COLD_RUN
        NATIVESPAWN_SERVER_NAME, NATIVESPAWN_SERVER_NAME,  // MODE_FOR_NATIVE_SPAWN, MODE_FOR_NATIVE_COLD_RUN
        CJAPPSPAWN_SERVER_NAME, CJAPPSPAWN_SERVER_NAME,    // MODE_FOR_CJAPP_SPAWN, MODE_FOR_CJAPP_COLD_RUN
        HYBRIDSPAWN_SERVER_NAME, HYBRIDSPAWN_SERVER_NAME   // MODE_FOR_HYBRID_SPAWN, MODE_FOR_HYBRID_COLD_RUN
    };
    for (int i = 0; i < modeCount; i++) {
        EXPECT_EQ(0, strcmp(GetSpawnNameByRunMode(static_cast<RunMode>(i)), expectName[i]));
    }
    EXPECT_EQ(0, strcmp(GetSpawnNameByRunMode(MODE_INVALID), ""));
    EXPECT_EQ(0, strcmp(GetSpawnNameByRunMode(static_cast<RunMode>(-1)), ""));
    EXPECT_EQ(0, strcmp(GetSpawnNameByRunMode(static_cast<RunMode>(MODE_INVALID + 1)), ""));
}

/**
* @brief WriteSignalInfoToFd with invalid args and normal json write
* @note 预期结果：signalFd/pid/uid 非法时直接返回不写 fd；合法时向 signalFd 写入
*                 包含 pid/uid/signal/bundleName 的 json 信息
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_WriteSignalInfoToFd_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    AppSpawnContent *content = GetAppSpawnContent();
    ASSERT_NE(nullptr, content);

    const size_t nameLen = 32;  // 32 test name len
    AppSpawnedProcess *appInfo = (AppSpawnedProcess *)malloc(sizeof(AppSpawnedProcess) + nameLen);
    ASSERT_NE(nullptr, appInfo);
    appInfo->pid = 4242;     // 4242 test pid
    appInfo->uid = 3000123;  // 3000123 test uid
    ASSERT_EQ(0, strcpy_s(appInfo->name, nameLen, "write.signal.app"));

    int pipefd[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd));
    //invalid signal fd
    content->signalFd = 0;
    WriteSignalInfoToFd(appInfo, content, 9);  // 9 test signal
    //invalid pid
    content->signalFd = pipefd[1];
    appInfo->pid = 0;
    WriteSignalInfoToFd(appInfo, content, 9);
    //invalid uid, if wrongly written, json would contain "uid":0
    appInfo->pid = 4242;
    appInfo->uid = 0;
    WriteSignalInfoToFd(appInfo, content, 9);

    //normal case, json info is written into signal fd
    appInfo->uid = 3000123;
    WriteSignalInfoToFd(appInfo, content, 9);
    char buffer[256] = {0};  // 256 max json len
    ssize_t readLen = read(pipefd[0], buffer, sizeof(buffer) - 1);
    EXPECT_GT(readLen, 0);
    EXPECT_NE(nullptr, strstr(buffer, "\"pid\":\t4242"));
    EXPECT_NE(nullptr, strstr(buffer, "\"uid\":\t3000123"));
    EXPECT_NE(nullptr, strstr(buffer, "\"bundleName\":\t\"write.signal.app\""));
    EXPECT_EQ(nullptr, strstr(buffer, "\"uid\":\t0"));
    close(pipefd[0]);
    close(pipefd[1]);
    free(appInfo);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief HandleDiedPid with no mgr, unknown pid and tracked process
* @note 预期结果：无 mgr 或 pid 未注册时仅打印状态不崩溃；已注册进程被移入死亡队列，
*                 reservedPid 命中时被清零
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_HandleDiedPid_001, TestSize.Level0)
{
    //no mgr created, return directly
    HandleDiedPid(424242, 0, 0);

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    //unknown pid, not in app queue
    HandleDiedPid(424242, 0, 0);
    EXPECT_EQ(0U, mgr->diedAppCount);

    AppSpawnedProcess *app = AddSpawnedProcess(424242, "died.test.app", 0, false, 0);
    EXPECT_EQ(app != nullptr, 1);
    mgr->content.reservedPid = 424242;  // hit reserved pid branch
    HandleDiedPid(424242, 0, 0);
    EXPECT_EQ(0, mgr->content.reservedPid);
    EXPECT_EQ(GetSpawnedProcess(424242), nullptr);  // removed from app queue
    EXPECT_EQ(1U, mgr->diedAppCount);               // moved to died queue

    app = AddSpawnedProcess(424243, "died.test.app2", 0, false, 0);
    EXPECT_EQ(app != nullptr, 1);
    HandleDiedPid(424243, 0, 0);
    EXPECT_EQ(2U, mgr->diedAppCount);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief HandleDiedPid sets killReason=REASON_SIGNAL_KILL before STAGE_SERVER_APP_CLEANUP
 * @note 预期结果：进程被信号杀死（WIFSIGNALED）时，移入 diedQueue 的进程
 *                 killReason 被置为 REASON_SIGNAL_KILL
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_HandleDiedPid_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnedProcess *app = AddSpawnedProcess(424252, "died.signal.app", 0, false, 0);
    EXPECT_EQ(app != nullptr, 1);

    // waitpid status: WIFSIGNALED == true, WTERMSIG == SIGKILL(9)
    int status = SIGKILL;
    HandleDiedPid(424252, 0, status);
    EXPECT_EQ(GetSpawnedProcess(424252), nullptr);

    AppSpawnedProcess *diedApp = nullptr;
    ListNode *node = mgr->diedQueue.next;
    while (node != &mgr->diedQueue) {
        AppSpawnedProcess *info = ListEntry(node, AppSpawnedProcess, node);
        if (info->pid == 424252) {
            diedApp = info;
            break;
        }
        node = node->next;
    }
    ASSERT_NE(diedApp, nullptr);
    EXPECT_EQ(diedApp->killReason, REASON_SIGNAL_KILL);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief HandleDiedPid sets killReason=REASON_SIGNAL_EXIT before STAGE_SERVER_APP_CLEANUP
 * @note 预期结果：进程正常退出（WIFEXITED）时，移入 diedQueue 的进程
 *                 killReason 被置为 REASON_SIGNAL_EXIT
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_HandleDiedPid_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    AppSpawnedProcess *app = AddSpawnedProcess(424253, "died.exit.app", 0, false, 0);
    EXPECT_EQ(app != nullptr, 1);

    // waitpid status: WIFEXITED == true, WEXITSTATUS == 1
    int status = 1 << 8;
    HandleDiedPid(424253, 0, status);
    EXPECT_EQ(GetSpawnedProcess(424253), nullptr);

    AppSpawnedProcess *diedApp = nullptr;
    ListNode *node = mgr->diedQueue.next;
    while (node != &mgr->diedQueue) {
        AppSpawnedProcess *info = ListEntry(node, AppSpawnedProcess, node);
        if (info->pid == 424253) {
            diedApp = info;
            break;
        }
        node = node->next;
    }
    ASSERT_NE(diedApp, nullptr);
    EXPECT_EQ(diedApp->killReason, REASON_SIGNAL_EXIT);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief GetAppSpawnMsgFromBuffer with invalid msg header
* @note 预期结果：magic 非法、msgLen 超上限或小于头长度、tlvCount 超上限或超过
*                 msgLen 允许的上限时消息重建失败返回 -1
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnMsgNode_010, TestSize.Level0)
{
    AppSpawnMsg header = {};
    header.magic = APPSPAWN_MSG_MAGIC;
    header.msgType = MSG_APP_SPAWN;
    header.msgId = 1;
    ASSERT_EQ(0, strcpy_s(header.processName, sizeof(header.processName), "header.check.app"));

    const int caseCount = 5;
    for (int i = 0; i < caseCount; i++) {
        header.msgLen = sizeof(AppSpawnMsg);
        header.tlvCount = 0;
        if (i == 0) {
            header.magic = 0;  // invalid magic
        } else if (i == 1) {
            header.msgLen = MAX_MSG_TOTAL_LENGTH;  // exceed max msg len
        } else if (i == 2) {
            header.msgLen = sizeof(AppSpawnMsg) - 4;  // 4 less than msg header
        } else if (i == 3) {
            header.msgLen = sizeof(AppSpawnMsg) + 8;  // 8 test len
            header.tlvCount = MAX_TLV_COUNT;          // exceed max tlv count
        } else {
            header.msgLen = sizeof(AppSpawnMsg) + 8;  // 8 test len
            header.tlvCount = 71;                     // 71 >= msgLen / sizeof(AppSpawnTlv)
        }

        AppSpawnMsgNode *outMsg = nullptr;
        uint32_t msgRecvLen = 0;
        uint32_t reminder = 0;
        int ret = GetAppSpawnMsgFromBuffer(reinterpret_cast<uint8_t *>(&header), sizeof(AppSpawnMsg),
            &outMsg, &msgRecvLen, &reminder);
        EXPECT_EQ(-1, ret);
        DeleteAppSpawnMsg(&outMsg);
        header.magic = APPSPAWN_MSG_MAGIC;
    }
}

/**
* @brief DecodeAppSpawnMsg with invalid tlv length
* @note 预期结果：tlvLen 为 0 或超出消息剩余长度时解码失败，返回 APPSPAWN_MSG_INVALID
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_DecodeAppSpawnMsg_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    AppMgrTestHelper testHelper;

    const int caseCount = 2;
    for (int i = 0; i < caseCount; i++) {
        std::vector<uint8_t> buffer(1024);  // 1024 max buffer
        uint32_t msgLen = 0;
        int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
            AppMgrTestHelper::AppMgrTestAddBaseTlv,
            [&](uint8_t *buf, uint32_t bufLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
                //only tlv header is written, tlvLen is 0 or beyond remaining buffer
                AppSpawnTlv tlv = {};
                tlv.tlvType = TLV_MSG_FLAGS;
                tlv.tlvLen = (i == 0) ? 0 : 64;  // 0 len or 64 beyond
                if (sizeof(tlv) > bufLen) {
                    return -1;
                }
                int ret = memcpy_s(buf, bufLen, &tlv, sizeof(tlv));
                if (ret != 0) {
                    return -1;
                }
                realLen = sizeof(tlv);
                tlvCount = 1;
                return 0;
            }
        });
        EXPECT_EQ(0, ret);

        AppSpawnMsgNode *outMsg = nullptr;
        uint32_t msgRecvLen = 0;
        uint32_t reminder = 0;
        ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
        EXPECT_EQ(0, ret);
        EXPECT_NE(0, DecodeAppSpawnMsg(outMsg));
        DeleteAppSpawnMsg(&outMsg);
    }
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief DecodeAppSpawnMsg with inconsistent ext tlv info
* @note 预期结果：ext tlv 的 dataLen 大于 tlvLen 减去扩展头长度时校验失败，
*                 解码返回 APPSPAWN_MSG_INVALID
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_DecodeAppSpawnMsg_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024 max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        AppMgrTestHelper::AppMgrTestAddBaseTlv,
        [&](uint8_t *buf, uint32_t bufLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            //ext tlv whose dataLen is much bigger than real data len
            AppSpawnTlvExt tlv = {};
            tlv.tlvType = TLV_MAX;
            tlv.dataType = DATA_TYPE_STRING;
            tlv.dataLen = 64;  // 64 fake data len
            tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(4);  // 4 real data len
            int ret = strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), "decode-ext-bad");
            if (ret != 0 || tlv.tlvLen > bufLen) {
                return -1;
            }
            ret = memcpy_s(buf, bufLen, &tlv, sizeof(tlv));
            if (ret != 0) {
                return -1;
            }
            uint32_t dummy = 0;
            ret = memcpy_s(buf + sizeof(tlv), bufLen - sizeof(tlv), &dummy, sizeof(dummy));
            if (ret != 0) {
                return -1;
            }
            realLen = tlv.tlvLen;
            tlvCount = 1;
            return 0;
        }
    });
    EXPECT_EQ(0, ret);

    AppSpawnMsgNode *outMsg = nullptr;
    uint32_t msgRecvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &outMsg, &msgRecvLen, &reminder);
    EXPECT_EQ(0, ret);
    EXPECT_NE(0, DecodeAppSpawnMsg(outMsg));
    DeleteAppSpawnMsg(&outMsg);
    DeleteAppSpawnMgr(mgr);
}

/**
* @brief TerminateSpawnedProcess died queue eviction in nweb mode
* @note 预期结果：nweb 模式下进程终止后移入死亡队列，队列最多保留
*                 MAX_DIED_PROCESS_COUNT 个节点，超出后淘汰最老节点
*
*/
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_TerminateSpawnedProcess_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    const int appCount = MAX_DIED_PROCESS_COUNT + 2;  // 7 apps, more than max died count 5
    char name[APP_LEN_PROC_NAME] = {0};
    for (int i = 0; i < appCount; i++) {
        int ret = sprintf_s(name, sizeof(name), "died.app.%d", i);
        EXPECT_GT(ret, 0);
        AppSpawnedProcess *app = AddSpawnedProcess(3000 + i, name, 0, false, 0);
        EXPECT_EQ(app != nullptr, 1);
        TerminateSpawnedProcess(app);
        uint32_t expectCount = (static_cast<uint32_t>(i) >= MAX_DIED_PROCESS_COUNT) ?
            MAX_DIED_PROCESS_COUNT : static_cast<uint32_t>(i) + 1;
        EXPECT_EQ(expectCount, mgr->diedAppCount);
        EXPECT_EQ(GetSpawnedProcess(3000 + i), nullptr);  // removed from app queue
    }
    EXPECT_EQ(static_cast<uint32_t>(MAX_DIED_PROCESS_COUNT), mgr->diedAppCount);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AppSpawningCtx AppSpawnMsg
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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
    for (int j = 0; j < MAX_FLAGS_INDEX; j++) {
        ret = SetAppPermissionFlags(nullptr, j);
        EXPECT_NE(0, ret);
        ret = CheckAppPermissionFlagSet(nullptr, j);
        EXPECT_EQ(0, ret);
    }
    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024 * 2);  // 1024 * 2  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        },
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddExtTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_006, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    EXPECT_EQ(mgr != nullptr, 1);

    // get from buffer
    AppMgrTestHelper testHelper;
    std::vector<uint8_t> buffer(1024);  // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = testHelper.AppMgrTestCreateSendMsg(buffer, MSG_APP_SPAWN, msgLen, {
        [&](uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount) -> int {
            return testHelper.AppMgrTestAddBaseTlv(buffer, bufferLen, realLen, tlvCount);
        }
    });
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

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtx_Msg_007, TestSize.Level0)
{
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    EXPECT_EQ(appCtx != nullptr, 1);

    // IsDeveloperModeOn
    int ret = IsDeveloperModeOn(appCtx);
    EXPECT_EQ(ret, 0);
    appCtx->client.flags |= APP_DEVELOPER_MODE;
    ret = IsDeveloperModeOn(appCtx);
    EXPECT_EQ(ret, 1);
    ret = IsDeveloperModeOn(nullptr);
    EXPECT_EQ(ret, 0);

    //IsJitFortModeOn
    ret = IsJitFortModeOn(appCtx);
    EXPECT_EQ(ret, 0);
    appCtx->client.flags |= APP_JITFORT_MODE;
    ret = IsJitFortModeOn(appCtx);
    EXPECT_EQ(ret, 1);
    ret = IsJitFortModeOn(nullptr);
    EXPECT_EQ(ret, 0);

    DeleteAppSpawningCtx(appCtx);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_RebuildAppSpawnMsgNode, TestSize.Level0)
{
    AppSpawnMsgNode *msgNode = CreateAppSpawnMsg();
    EXPECT_EQ(msgNode != nullptr, 1);
    int ret = CheckAppSpawnMsg(msgNode);
    EXPECT_NE(0, ret);  // check fail
    AppSpawnedProcess *app = (AppSpawnedProcess *)malloc(sizeof(AppSpawnedProcess) + sizeof(char) * 10);
    EXPECT_EQ(app != nullptr, 1);
    app->message = (AppSpawnMsgNode *)malloc(sizeof(AppSpawnMsgNode));
    EXPECT_EQ(app->message != nullptr, 1);
    app->message->msgHeader.tlvCount = 10; // 10 is tlvCount
    app->message->msgHeader.msgLen = 200; // 200 is msgLen
    ret = strcpy_s(app->message->msgHeader.processName, APP_LEN_PROC_NAME, "test.xxx");
    EXPECT_EQ(ret, 0);
    ret = strcpy_s(app->name, 10, "test.xxx"); // 10 is appNmae length
    EXPECT_EQ(ret, 0);
    RebuildAppSpawnMsgNode(msgNode, app);
    free(app->message);
    free(app);
}

HWTEST_F(AppSpawnAppMgrTest, App_Spawn_KillAndWaitStatus, TestSize.Level0)
{
    pid_t pid = -1;
    int sig = SIGTERM;
    int exitStatus;
    int ret = KillAndWaitStatus(pid, sig, &exitStatus);
    EXPECT_EQ(0, ret);
    ret = KillAndWaitStatus(pid, sig, nullptr);
    EXPECT_EQ(0, ret);
    pid = getpid(); //test pid
    signal(SIGTERM, AppMgrTestHelper::SignalHandle);
    ret = KillAndWaitStatus(pid, sig, &exitStatus);
    EXPECT_EQ(-1, ret);
}

static void TraversalCount(const AppSpawnMgr *mgr, AppSpawnedProcess *appInfo, void *data)
{
    (void)mgr;
    (void)appInfo;
    int *count = static_cast<int *>(data);
    if (count != nullptr) {
        (*count)++;
    }
}

static void CtxTraversalCount(const AppSpawnMgr *mgr, AppSpawningCtx *ctx, void *data)
{
    (void)mgr;
    (void)ctx;
    int *count = static_cast<int *>(data);
    if (count != nullptr) {
        (*count)++;
    }
}

static AppSpawnMsgNode *CreateDecodedTestMsg(uint32_t msgType)
{
    AppMgrTestHelper helper;
    std::vector<uint8_t> buffer(TEST_MSG_BUFFER_SIZE + sizeof(AppSpawnMsg));  // base TLV 需容纳完整消息
    uint32_t msgLen = 0;
    int ret = helper.AppMgrTestCreateSendMsg(buffer, msgType, msgLen, {
        [&](uint8_t *b, uint32_t bl, uint32_t &rl, uint32_t &tc) -> int {
            return helper.AppMgrTestAddBaseTlv(b, bl, rl, tc);
        }
    });
    if (ret != 0) {
        return nullptr;
    }
    AppSpawnMsgNode *msg = nullptr;
    uint32_t recvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msg, &recvLen, &reminder);
    if (ret != 0 || msg == nullptr) {
        return nullptr;
    }
    if (DecodeAppSpawnMsg(msg) != 0) {
        DeleteAppSpawnMsg(&msg);
        return nullptr;
    }
    return msg;
}

/**
 * @brief TraversalSpawnedProcess：无 mgr / 空队列 / 多 app 遍历计数
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_TraversalSpawnedProcess_001, TestSize.Level0)
{
    int count = 0;
    TraversalSpawnedProcess(TraversalCount, &count);  // 无 mgr，直接返回
    EXPECT_EQ(0, count);

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    TraversalSpawnedProcess(nullptr, &count);  // traversal 为空，直接返回
    EXPECT_EQ(0, count);

    TraversalSpawnedProcess(TraversalCount, &count);  // 空队列
    EXPECT_EQ(0, count);

    EXPECT_NE(nullptr, AddSpawnedProcess(3001, "traverse.a", 0, false, 0));
    EXPECT_NE(nullptr, AddSpawnedProcess(3002, "traverse.b", 0, false, 0));
    EXPECT_NE(nullptr, AddSpawnedProcess(3003, "traverse.c", 0, false, 0));
    count = 0;
    TraversalSpawnedProcess(TraversalCount, &count);
    EXPECT_EQ(3, count);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AppSpawningCtxTraversal：无 mgr / traversal 为空 / 有 ctx 遍历计数
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawningCtxTraversal_001, TestSize.Level0)
{
    int count = 0;
    AppSpawningCtxTraversal(CtxTraversalCount, &count);  // 无 mgr
    EXPECT_EQ(0, count);

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtxTraversal(nullptr, &count);  // traversal 为空
    EXPECT_EQ(0, count);

    AppSpawningCtxTraversal(CtxTraversalCount, &count);  // 空队列
    EXPECT_EQ(0, count);

    EXPECT_NE(nullptr, CreateAppSpawningCtx());
    EXPECT_NE(nullptr, CreateAppSpawningCtx());
    count = 0;
    AppSpawningCtxTraversal(CtxTraversalCount, &count);
    EXPECT_EQ(2, count);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ProcessAppSpawnDumpMsg：无 mgr / message 为空 / 空队列 dump 不崩溃
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ProcessAppSpawnDumpMsg_001, TestSize.Level0)
{
    ProcessAppSpawnDumpMsg(nullptr);  // 无 mgr，直接返回

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    ProcessAppSpawnDumpMsg(nullptr);  // message 为空，直接返回

    AppSpawnMsgNode *msg = CreateDecodedTestMsg(MSG_DUMP);
    ASSERT_NE(nullptr, msg);
    ProcessAppSpawnDumpMsg(msg);  // 空队列 + 无 pty-name，走 stdout 分支

    DeleteAppSpawnMsg(&msg);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ProcessTerminationStatusMsg：无 mgr / 非 nweb / result 为空 / 无 pid TLV
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ProcessTerminationStatusMsg_001, TestSize.Level0)
{
    AppSpawnResult result = {0};

    EXPECT_EQ(-1, ProcessTerminationStatusMsg(nullptr, &result));  // no mgr

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    // message is NULL -> return -1 (message NULL check precedes the mode check)
    EXPECT_EQ(-1, ProcessTerminationStatusMsg(nullptr, &result));
    // result is NULL -> return -1
    EXPECT_EQ(-1, ProcessTerminationStatusMsg(nullptr, nullptr));

    // non-nwebspawn mode with a valid message -> APPSPAWN_MSG_INVALID
    AppSpawnMsgNode *msg = CreateDecodedTestMsg(MSG_GET_RENDER_TERMINATION_STATUS);
    ASSERT_NE(nullptr, msg);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, ProcessTerminationStatusMsg(msg, &result));
    DeleteAppSpawnMsg(&msg);

    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ProcessTerminationStatusMsg：nweb 模式 + message 无 render termination TLV
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ProcessTerminationStatusMsg_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnMsgNode *msg = CreateDecodedTestMsg(MSG_GET_RENDER_TERMINATION_STATUS);
    ASSERT_NE(nullptr, msg);

    AppSpawnResult result = {0};
    // 无 TLV_RENDER_TERMINATION_INFO，pid 读取失败
    EXPECT_EQ(-1, ProcessTerminationStatusMsg(msg, &result));

    DeleteAppSpawnMsg(&msg);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ProcessTerminationStatusMsg：nweb 模式 + render termination TLV 正路径
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ProcessTerminationStatusMsg_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppMgrTestHelper helper;
    std::vector<uint8_t> buffer(1024); // 1024  max buffer
    uint32_t msgLen = 0;
    int ret = helper.AppMgrTestCreateSendMsg(buffer, MSG_GET_RENDER_TERMINATION_STATUS, msgLen, {
        [&](uint8_t *b, uint32_t bl, uint32_t &rl, uint32_t &tc) -> int {
            return helper.AppMgrTestAddRenderTerminationTlv(b, bl, rl, tc);
        }
    });
    ASSERT_EQ(0, ret);

    AppSpawnMsgNode *msg = nullptr;
    uint32_t recvLen = 0;
    uint32_t reminder = 0;
    ret = GetAppSpawnMsgFromBuffer(buffer.data(), msgLen, &msg, &recvLen, &reminder);
    ASSERT_EQ(0, ret);
    ASSERT_EQ(0, DecodeAppSpawnMsg(msg));

    AppSpawnResult result = {0};
    // 正路径：pid 未注册，GetProcessTerminationStatus 返回 -1，但消息处理成功
    EXPECT_EQ(0, ProcessTerminationStatusMsg(msg, &result));
    EXPECT_EQ(9999999, result.pid);
    EXPECT_EQ(-1, result.result);

    DeleteAppSpawnMsg(&msg);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ProcessSignal：SIGCHLD（无子进程）与默认信号分支不崩溃
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ProcessSignal_001, TestSize.Level0)
{
    struct signalfd_siginfo siginfo = {};
    siginfo.ssi_signo = SIGCHLD;
    siginfo.ssi_uid = 0;
    siginfo.ssi_pid = 0;
    ProcessSignal(&siginfo);  // 无子进程，waitpid 返回 0，不进入回收集合

    siginfo.ssi_signo = SIGUSR1;  // 默认分支
    ProcessSignal(&siginfo);
}

/**
 * @brief WriteMsgToChild：property 为空 / message 为空
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_WriteMsgToChild_001, TestSize.Level0)
{
    EXPECT_EQ(APPSPAWN_MSG_INVALID, WriteMsgToChild(nullptr, MODE_FOR_APP_SPAWN));

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(nullptr, property);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, WriteMsgToChild(property, MODE_FOR_APP_SPAWN));  // message 为空

    DeleteAppSpawningCtx(property);
}

/**
 * @brief SetPreforkProcessName：longProcName 有效时设置成功
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_SetPreforkProcessName_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawnContent *content = GetAppSpawnContent();
    ASSERT_NE(content, nullptr);

    char procName[16] = {0};
    content->longProcName = procName;
    content->longProcNameLen = static_cast<uint32_t>(sizeof(procName));

    int ret = SetPreforkProcessName(content);
    EXPECT_EQ(0, ret);
    EXPECT_STREQ("apppool", content->longProcName);

    content->longProcName = nullptr;
    content->longProcNameLen = 0;
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ClearPipeFd：正常 fd 关闭并置 -1，非正数 fd 不受影响
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ClearPipeFd_001, TestSize.Level0)
{
    int pipefd[2] = {-1, -1};
    ASSERT_EQ(0, pipe(pipefd));

    int fds[4] = {pipefd[0], pipefd[1], 0, -1};  // 0/-1 不应被 close
    ClearPipeFd(fds, 4);
    EXPECT_EQ(-1, fds[0]);
    EXPECT_EQ(-1, fds[1]);
    EXPECT_EQ(0, fds[2]);
    EXPECT_EQ(-1, fds[3]);
}

/**
 * @brief ClearPipeFd：length 为 0 时无操作
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ClearPipeFd_002, TestSize.Level0)
{
    int fds[2] = {10, 20};  // 占位值，length=0 不应被修改
    ClearPipeFd(fds, 0);
    EXPECT_EQ(10, fds[0]);
    EXPECT_EQ(20, fds[1]);
}

/**
 * @brief ClearMMAP：无 mgr / propertyBuffer 为空时不崩溃
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ClearMMAP_001, TestSize.Level0)
{
    ClearMMAP(424242, 4096);  // 无 mgr

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    ClearMMAP(424242, 4096);  // propertyBuffer 为空
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ClearPreforkInfo：property 为空 / childMsg 为空时不崩溃
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ClearPreforkInfo_001, TestSize.Level0)
{
    ClearPreforkInfo(nullptr);  // property 为空

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(nullptr, property);
    ClearPreforkInfo(property);  // childMsg 为空

    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief WritePreforkMsg：无 mgr / propertyBuffer 为空返回 -1
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_WritePreforkMsg_001, TestSize.Level0)
{
    EXPECT_EQ(-1, WritePreforkMsg(nullptr, 4096));  // 无 mgr，content 为空

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    EXPECT_EQ(-1, WritePreforkMsg(nullptr, 4096));  // propertyBuffer 为空
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief ProcessCheckpointReqMsg：连接/消息参数为空的提前返回分支
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ProcessCheckpointReqMsg_001, TestSize.Level0)
{
    ProcessCheckpointReqMsg(nullptr, nullptr);  // 连接与消息均为空

    AppSpawnConnection conn = {};
    ProcessCheckpointReqMsg(&conn, nullptr);  // 消息为空
}

/**
 * @brief AppspawpnDevicedebugKill：args 为空 / signal 缺失 / 非数字
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppspawpnDevicedebugKill_001, TestSize.Level0)
{
    EXPECT_EQ(-1, AppspawpnDevicedebugKill(424242, nullptr));  // args 为空

    cJSON *args = cJSON_CreateObject();
    ASSERT_NE(nullptr, args);
    EXPECT_EQ(-1, AppspawpnDevicedebugKill(424242, args));  // 无 signal 字段

    cJSON_AddStringToObject(args, "signal", "nine");  // signal 非数字
    EXPECT_EQ(-1, AppspawpnDevicedebugKill(424242, args));

    cJSON_Delete(args);
}

/**
 * @brief AppspawpnDevicedebugKill：pid 未注册 / 进程不可调试
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppspawpnDevicedebugKill_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    cJSON *args = cJSON_CreateObject();
    ASSERT_NE(nullptr, args);
    cJSON_AddNumberToObject(args, "signal", 9);
    // pid 未注册
    EXPECT_EQ(APPSPAWN_DEVICEDEBUG_ERROR_APP_NOT_EXIST, AppspawpnDevicedebugKill(424242, args));

    // pid 已注册但不可调试
    ASSERT_NE(nullptr, AddSpawnedProcess(424242, "dbg.app", 0, false, 0));
    EXPECT_EQ(APPSPAWN_DEVICEDEBUG_ERROR_APP_NOT_DEBUGGABLE, AppspawpnDevicedebugKill(424242, args));

    cJSON_Delete(args);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AppspawnDevicedebugDeal：非法 op 返回 -1
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppspawnDevicedebugDeal_001, TestSize.Level0)
{
    cJSON *args = cJSON_CreateObject();
    ASSERT_NE(nullptr, args);

    EXPECT_EQ(-1, AppspawnDevicedebugDeal("invalid-op", 424242, args));
    EXPECT_EQ(-1, AppspawnDevicedebugDeal("kill", 424242, nullptr));  // 透传 args 为空

    cJSON_Delete(args);
}

/**
 * @brief ProcessAppSpawnDeviceDebugMsg：消息为空 / 无 devicedebug 扩展信息
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ProcessAppSpawnDeviceDebugMsg_001, TestSize.Level0)
{
    EXPECT_EQ(-1, ProcessAppSpawnDeviceDebugMsg(nullptr));  // 消息为空

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawnMsgNode *msg = CreateDecodedTestMsg(MSG_DEVICE_DEBUG);
    ASSERT_NE(nullptr, msg);
    EXPECT_EQ(-1, ProcessAppSpawnDeviceDebugMsg(msg));  // 无 devicedebug extInfo

    DeleteAppSpawnMsg(&msg);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AppSpawnReqMsgFdGet：连接/消息/偏移数组为空的提前返回分支
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnReqMsgFdGet_001, TestSize.Level0)
{
    int fd = -1;
    EXPECT_EQ(-1, AppSpawnReqMsgFdGet(nullptr, nullptr, "x", &fd));  // 连接为空

    AppSpawnConnection conn = {};
    EXPECT_EQ(-1, AppSpawnReqMsgFdGet(&conn, nullptr, "x", &fd));  // 消息为空

    AppSpawnMsgNode msg = {};
    uint8_t buf[16] = {0};
    msg.buffer = buf;        // buffer 有效
    msg.tlvOffset = nullptr; // tlvOffset 为空
    EXPECT_EQ(-1, AppSpawnReqMsgFdGet(&conn, &msg, "x", &fd));
}

/**
 * @brief ProcessObserveProcessSignalMsg：消息为空直接返回
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_ProcessObserveProcessSignalMsg_001, TestSize.Level0)
{
    ProcessObserveProcessSignalMsg(nullptr, nullptr);  // 消息为空，直接返回
}

/**
 * @brief SendUnlockMsgToPrefork：content 为空 / 无 parent-child fd
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_SendUnlockMsgToPrefork_001, TestSize.Level0)
{
    EXPECT_EQ(APPSPAWN_ARG_INVALID, SendUnlockMsgToPrefork(nullptr, 100));  // content 为空

    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawnContent *content = GetAppSpawnContent();
    ASSERT_NE(content, nullptr);
    EXPECT_EQ(APPSPAWN_ARG_INVALID, SendUnlockMsgToPrefork(content, 100));  // 无 parent-child fd
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief SendPipeMsgToChild：无 parent-child fd 返回 ARG_INVALID
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_SendPipeMsgToChild_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawnPipeMsg pipeMsg = {};
    EXPECT_EQ(APPSPAWN_ARG_INVALID, SendPipeMsgToChild(mgr, 424242, &pipeMsg));  // 无 fd
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief CleanupPreforkChild：不存在的 pid 清理不崩溃
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_CleanupPreforkChild_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    CleanupPreforkChild(mgr, 424242);  // kill 失败被忽略 + 无 fd 可清理
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief TransferPreforkFdToForkCtx：无 child-parent fd 返回 ARG_INVALID
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_TransferPreforkFdToForkCtx_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(nullptr, property);
    EXPECT_EQ(APPSPAWN_ARG_INVALID, TransferPreforkFdToForkCtx(mgr, 424242, property));  // 无 fd

    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}

/**
 * @brief AppSpawnColdStartApp：进程名为空返回 ARG_INVALID
 *
 */
HWTEST_F(AppSpawnAppMgrTest, App_Spawn_AppSpawnColdStartApp_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawnContent *content = GetAppSpawnContent();
    ASSERT_NE(content, nullptr);

    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(nullptr, property);
    // message 为空 -> GetProcessName 返回 NULL
    EXPECT_EQ(APPSPAWN_ARG_INVALID, AppSpawnColdStartApp(content, &property->client));

    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
}
}  // namespace OHOS
