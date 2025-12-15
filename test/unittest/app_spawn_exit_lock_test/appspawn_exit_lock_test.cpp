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

#include <gtest/gtest.h>
#include <csignal>

#include "appspawn_exit_lock_stub.h"
#include "appspawn_server.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppspawnExitLockTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppspawnExitLockTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "AppspawnExitLockTest SetUpTestCase";
}

void AppspawnExitLockTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "AppspawnExitLockTest TearDownTestCase";
}

void AppspawnExitLockTest::SetUp()
{
    GTEST_LOG_(INFO) << "AppspawnExitLockTest SetUp";
}

void AppspawnExitLockTest::TearDown()
{
    GTEST_LOG_(INFO) << "AppspawnExitLockTest TearDown";
}

HWTEST_F(AppspawnExitLockTest, AppspawnExitLockTest_001, TestSize.Level0)
{
    int code = 0;
    ProcessExit(code);
    EXPECT_EQ(code, 0);
    code = 1;
    ProcessExit(code);
    EXPECT_EQ(code, 1);
}

HWTEST_F(AppspawnExitLockTest, AppspawnExitLockTest_002, TestSize.Level0)
{
    bool ret = IsUnlockStatus(100 * 200000);
    EXPECT_EQ(ret, true);
    ret = IsUnlockStatus(101 * 200000);
    EXPECT_NE(ret, true);
    ret = IsUnlockStatus(102 * 200000);
    EXPECT_NE(ret, true);
    for (int i = 0; i <= LOCK_STATUS_UNLOCK_MAX; i++) {
        ret = IsUnlockStatus(100 * 200000);
        EXPECT_EQ(ret, true);
    }
}

HWTEST_F(AppspawnExitLockTest, ClearEnvAndReturnSuccess_001, TestSize.Level0)
{
    AppSpawningCtx *appCtx = CreateAppSpawningCtx();
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawnContent *content = reinterpret_cast<AppSpawnContent *>(mgr);
    appCtx->forkCtx.fd[1] = 1;
    AppSpawnClient *client = reinterpret_cast<AppSpawnClient *>(appCtx);

    ClearEnvAndReturnSuccess(content, client);
    DeleteAppSpawningCtx(appCtx);
    DeleteAppSpawnMgr(mgr);
}

}