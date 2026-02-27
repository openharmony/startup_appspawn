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
#include <memory>
#include <string>
#include <unistd.h>

#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "appspawn.h"

#include "dfx_preload_test_helper.h"

APPSPAWN_STATIC int SetDfxPreloadEnableEnv(AppSpawnMgr *content, AppSpawningCtx *property);

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
class AppSpawnDfxPreloadTest : public testing::Test {
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

/**
 * @brief 携带APP_FLAGS_DEBUGGABLE标记的debug应用测试SetDfxPreloadEnableEnv
 *
 */
HWTEST_F(AppSpawnDfxPreloadTest, SetDfxPreloadEnableEnv_01, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        ASSERT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        DfxPreloadTestHelper dfxPreloadTestHelper;
        AppSpawnReqMsgHandle reqHandle = dfxPreloadTestHelper.DfxPreloadTestCreateMsg(clientHandle, MSG_APP_SPAWN);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_DEBUGGABLE);

        property = dfxPreloadTestHelper.DfxPreloadTestGetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_DEBUGGABLE), 0);

        ret = SetDfxPreloadEnableEnv(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

/**
 * @brief 已标记coldstart的asan应用测试SetDfxPreloadEnableEnv
 *
 */
HWTEST_F(AppSpawnDfxPreloadTest, SetDfxPreloadEnableEnv_02, TestSize.Level0)
{
    int ret = -1;
    AppSpawnMgr *mgr = nullptr;
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawningCtx *property = nullptr;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        ASSERT_NE(mgr, nullptr);

        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create client %{public}s", APPSPAWN_SERVER_NAME);

        DfxPreloadTestHelper dfxPreloadTestHelper;
        AppSpawnReqMsgHandle reqHandle = dfxPreloadTestHelper.DfxPreloadTestCreateMsg(clientHandle, MSG_APP_SPAWN);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break, "Failed to create msg type %{public}d", MSG_APP_SPAWN);
        AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_ASANENABLED);

        property = dfxPreloadTestHelper.DfxPreloadTestGetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK(property != nullptr, break, "Failed to get app property");
        EXPECT_NE(CheckAppMsgFlagsSet(property, APP_FLAGS_ASANENABLED), 0);
        property->client.flags |= APP_COLD_START;

        ret = SetDfxPreloadEnableEnv(mgr, property);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    ASSERT_EQ(ret, 0);
}

}   // namespace OHOS
