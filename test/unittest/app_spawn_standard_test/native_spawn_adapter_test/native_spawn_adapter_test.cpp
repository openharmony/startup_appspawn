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
#include "native_adapter_mock_test.h"

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
}   // namespace OHOS
