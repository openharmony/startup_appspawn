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

#include <cerrno>
#include <cstring>
#include <gtest/gtest.h>
#include <cJSON.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

APPSPAWN_STATIC int RunChildThread(const AppSpawnMgr *content, const AppSpawningCtx *property);
APPSPAWN_STATIC int RunChildByRenderCmd(const AppSpawnMgr *content, const AppSpawningCtx *property);
APPSPAWN_STATIC int PreLoadAppSpawn(AppSpawnMgr *content);
APPSPAWN_STATIC int DlopenAppSpawn(AppSpawnMgr *content);

namespace OHOS {
static AppSpawnTestHelper g_testHelper;
class AppSpawnAceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief nwebspawn进行预加载
 * @note 预期结果: nwebspawn不支持预加载，直接返回
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_Preload_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(NWEBSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    int ret = PreLoadAppSpawn(mgr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief cjappspawn进行预加载
 * @note 预期结果: cjappspawn通过processName校验，执行LoadExtendCJLib预加载
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_Preload_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_CJAPP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(CJAPPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    int ret = PreLoadAppSpawn(mgr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief 未设置preload参数时（使用参数默认值），appspawn进行预加载
 * @note 预期结果: appspawn根据参数默认值判断是否执行preload预加载
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_Preload_003, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    int ret = PreLoadAppSpawn(mgr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief preload参数设置为true时，测试appspawn进行预加载
 * @note 预期结果: appspawn执行preload预加载
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_Preload_004, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    SetBoolParamResult("presist.appspwn.preload", true);
    int ret = PreLoadAppSpawn(mgr);
    SetBoolParamResult("presist.appspwn.preload", false);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief preload参数和preloadets参数均设置为true时，测试appspawn进行预加载
 * @note 预期结果: appspawn执行preload和preloadets预加载
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_Preload_005, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    mgr->content.longProcName = const_cast<char *>(APPSPAWN_SERVER_NAME);
    mgr->content.longProcNameLen = APP_LEN_PROC_NAME;

    SetBoolParamResult("presist.appspwn.preload", true);
    SetBoolParamResult("presist.appspwn.preloadets", true);
    int ret = PreLoadAppSpawn(mgr);
    SetBoolParamResult("presist.appspwn.preload", false);
    SetBoolParamResult("presist.appspwn.preloadets", false);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief nwebspawn进行加载system libs
 * @note 预期结果: nwebspawn不支持加载system libs，直接返回
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_Dlopen_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_NWEB_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int ret = DlopenAppSpawn(mgr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief appspawn进行加载system libs
 * @note 预期结果: appspawn加载system libs正常
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_Dlopen_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);

    int ret = DlopenAppSpawn(mgr);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief appspawn执行RunChildThread
 * @note 预期结果: 执行正常，分支覆盖
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_RunChild_001, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = CreateAppSpawnMsg();
    ASSERT_NE(property->message, nullptr);

    int ret = RunChildThread(mgr, property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, 0);
}

/**
 * @brief appspawn执行RunChildByRenderCmd
 * @note 预期结果: 执行正常，分支覆盖
 *
 */
HWTEST_F(AppSpawnAceTest, App_Spawn_Ace_RunChild_002, TestSize.Level0)
{
    AppSpawnMgr *mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
    ASSERT_NE(mgr, nullptr);
    AppSpawningCtx *property = CreateAppSpawningCtx();
    ASSERT_NE(property, nullptr);
    property->message = CreateAppSpawnMsg();
    ASSERT_NE(property->message, nullptr);

    int ret = RunChildByRenderCmd(mgr, property);
    DeleteAppSpawningCtx(property);
    DeleteAppSpawnMgr(mgr);
    EXPECT_EQ(ret, -1);
}
}   // namespace OHOS
