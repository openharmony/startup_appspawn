/**
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "appspawn_server.h"
#include "json_utils.h"
#include "sandbox_controlled_app.h"
#include "parameters.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;

namespace OHOS {

static AppSpawnTestHelper g_testHelper;

static const char* CONTROLLED_APPS_JSON_PATH =
    "/data/service/el1/public/dlp_credential_service/ControlledAppList.json";
static constexpr mode_t DIR_PERMISSION = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
static const char* CONTROLLED_APPS_DIR =
    "/data/service/el1/public/dlp_credential_service";

class AppSpawnControlledAppTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        (void)mkdir("/data/service", DIR_PERMISSION);
        (void)mkdir("/data/service/el1", DIR_PERMISSION);
        (void)mkdir("/data/service/el1/public", DIR_PERMISSION);
        (void)mkdir(CONTROLLED_APPS_DIR, DIR_PERMISSION);
    }

    static void TearDownTestCase()
    {
        (void)remove(CONTROLLED_APPS_JSON_PATH);
    }

    void SetUp()
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        ControlledAppCache &cache = ControlledAppCache::GetInstance();
        cache.controlledApps_.clear();
        cache.cacheLoaded = false;
        (void)remove(CONTROLLED_APPS_JSON_PATH);
        system::SetParameter("security.dlp.transparent.crypto.status", "0");
        system::SetParameter("startup.appspawn.dlp_errorcode", "0");
    }

    void TearDown()
    {
        ControlledAppCache &cache = ControlledAppCache::GetInstance();
        cache.controlledApps_.clear();
        cache.cacheLoaded = false;
        (void)remove(CONTROLLED_APPS_JSON_PATH);
        system::SetParameter("security.dlp.transparent.crypto.status", "0");
        system::SetParameter("startup.appspawn.dlp_errorcode", "0");
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
    }

    bool WriteControlledAppJson(const std::string &content)
    {
        std::ofstream ofs(CONTROLLED_APPS_JSON_PATH);
        if (!ofs.is_open()) {
            return false;
        }
        ofs << content;
        ofs.close();
        return true;
    }
};

HWTEST_F(AppSpawnControlledAppTest, GetInstance_Returns_Same_Object, TestSize.Level0)
{
    ControlledAppCache &ins1 = ControlledAppCache::GetInstance();
    ControlledAppCache &ins2 = ControlledAppCache::GetInstance();
    EXPECT_EQ(&ins1, &ins2);
}

HWTEST_F(AppSpawnControlledAppTest, IsControlled_EmptyCache_ReturnsFalse, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    EXPECT_FALSE(cache.IsControlled(100, "com.example.app"));
    EXPECT_FALSE(cache.IsControlled(0, "any"));
}

HWTEST_F(AppSpawnControlledAppTest, IsControlled_UserIdNotFound_ReturnsFalse, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.x");
    cache.cacheLoaded = true;

    EXPECT_FALSE(cache.IsControlled(200, "com.app.x"));
}

HWTEST_F(AppSpawnControlledAppTest, IsControlled_OwnerIdNotFound_ReturnsFalse, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.x");
    cache.cacheLoaded = true;

    EXPECT_FALSE(cache.IsControlled(100, "com.app.y"));
}

HWTEST_F(AppSpawnControlledAppTest, IsControlled_Match_ReturnsTrue, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.match");
    cache.cacheLoaded = true;

    EXPECT_TRUE(cache.IsControlled(100, "com.app.match"));
}

HWTEST_F(AppSpawnControlledAppTest, IsControlled_MultipleUsersAndOwners, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.a");
    cache.controlledApps_["100"].insert("com.app.b");
    cache.controlledApps_["200"].insert("com.app.c");
    cache.cacheLoaded = true;

    EXPECT_TRUE(cache.IsControlled(100, "com.app.a"));
    EXPECT_TRUE(cache.IsControlled(100, "com.app.b"));
    EXPECT_TRUE(cache.IsControlled(200, "com.app.c"));
    EXPECT_FALSE(cache.IsControlled(100, "com.app.c"));
    EXPECT_FALSE(cache.IsControlled(200, "com.app.a"));
}

HWTEST_F(AppSpawnControlledAppTest, IsControlled_LargeUserId, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    uint32_t largeUid = 4294967295u; // UINT32_MAX
    cache.controlledApps_[std::to_string(largeUid)].insert("com.app.large");
    cache.cacheLoaded = true;

    EXPECT_TRUE(cache.IsControlled(largeUid, "com.app.large"));
}

HWTEST_F(AppSpawnControlledAppTest, IsControlled_ZeroUserId, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["0"].insert("com.app.zero");
    cache.cacheLoaded = true;

    EXPECT_TRUE(cache.IsControlled(0, "com.app.zero"));
}

HWTEST_F(AppSpawnControlledAppTest, IsControlled_EmptyOwnerId, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("");
    cache.cacheLoaded = true;

    EXPECT_TRUE(cache.IsControlled(100, ""));
}

HWTEST_F(AppSpawnControlledAppTest, EnsureCacheLoaded_Ready_CacheValid_NoOp, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.valid");
    cache.cacheLoaded = true;

    int32_t ret = cache.EnsureCacheLoaded(CRYPTO_READY);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(cache.cacheLoaded);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.valid"));
}

HWTEST_F(AppSpawnControlledAppTest, EnsureCacheLoaded_Ready_CacheLost_ReloadFails, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = false;

    int32_t ret = cache.EnsureCacheLoaded(CRYPTO_READY);
    EXPECT_EQ(ret, -1);
    EXPECT_FALSE(cache.cacheLoaded);
}

HWTEST_F(AppSpawnControlledAppTest, EnsureCacheLoaded_Update_ReloadFails, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = false;

    int32_t ret = cache.EnsureCacheLoaded(CRYPTO_UPDATE);
    EXPECT_EQ(ret, -1);
    EXPECT_FALSE(cache.cacheLoaded);
}

HWTEST_F(AppSpawnControlledAppTest, EnsureCacheLoaded_Update_EvenWhenCacheLoaded, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.old");
    cache.cacheLoaded = true;

    int32_t ret = cache.EnsureCacheLoaded(CRYPTO_UPDATE);
    EXPECT_EQ(ret, -1);  // LoadFromJsonLocked 失败
    EXPECT_FALSE(cache.cacheLoaded); // 失败时设为 false
}

HWTEST_F(AppSpawnControlledAppTest, EnsureCacheLoaded_UnknownStatus_CacheLost_ReloadFails, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = false;

    int32_t ret = cache.EnsureCacheLoaded(99);
    EXPECT_EQ(ret, -1);
    EXPECT_FALSE(cache.cacheLoaded);
}

HWTEST_F(AppSpawnControlledAppTest, EnsureCacheLoaded_UnknownStatus_CacheValid_NoOp, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.x");
    cache.cacheLoaded = true;

    int32_t ret = cache.EnsureCacheLoaded(99);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(cache.cacheLoaded);
}

HWTEST_F(AppSpawnControlledAppTest, EnsureCacheLoaded_Idempotent_MultipleCalls, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["200"].insert("com.app.idem");
    cache.cacheLoaded = true;

    EXPECT_EQ(cache.EnsureCacheLoaded(CRYPTO_READY), 0);
    EXPECT_EQ(cache.EnsureCacheLoaded(CRYPTO_READY), 0);
    EXPECT_EQ(cache.EnsureCacheLoaded(CRYPTO_READY), 0);
    EXPECT_TRUE(cache.IsControlled(200, "com.app.idem"));
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_CryptoUnavailable_Skip, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = true;
    cache.controlledApps_["100"].insert("com.skip.test");

    int32_t result = cache.ComputeForSpawn(property);
    EXPECT_GE(result, -1); // 不应小于 -1
    EXPECT_LE(result, 1);  // 正常返回 0 或 1
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_ClearsFlag44, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    EXPECT_EQ(SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_CONTROLLED_APP), 0);
    EXPECT_TRUE(CheckAppMsgFlagsSet(property, APP_FLAGS_CONTROLLED_APP));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = true;
    cache.controlledApps_["100"].insert("com.flag.test");

    cache.ComputeForSpawn(property);

    EXPECT_FALSE(CheckAppMsgFlagsSet(property, APP_FLAGS_CONTROLLED_APP));
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_ValidMsgFlags, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    AppSpawnMsgFlags *msgFlags =
        (AppSpawnMsgFlags *)GetAppSpawnMsgInfo(property->message, TLV_MSG_FLAGS);
    ASSERT_NE(msgFlags, nullptr);

    EXPECT_GT(msgFlags->count, APP_FLAGS_CONTROLLED_APP / 32);

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = true;
    cache.controlledApps_["100"].insert("com.msgf.test");

    int32_t result = cache.ComputeForSpawn(property);
    EXPECT_GE(result, -1);
    EXPECT_LE(result, 1);
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_HasDacAndOwnerInfo, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    AppSpawnMsgDacInfo *dacInfo =
        reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    ASSERT_NE(dacInfo, nullptr);
    EXPECT_GT(dacInfo->uid, 0u);

    AppSpawnMsgOwnerId *ownerInfo =
        reinterpret_cast<AppSpawnMsgOwnerId *>(GetAppProperty(property, TLV_OWNER_INFO));
    ASSERT_NE(ownerInfo, nullptr);
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_ControlledApp_MatchInCache, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    AppSpawnMsgDacInfo *dacInfo =
        reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    ASSERT_NE(dacInfo, nullptr);
    uint32_t userId = dacInfo->uid / UID_BASE;

    AppSpawnMsgOwnerId *ownerInfo =
        reinterpret_cast<AppSpawnMsgOwnerId *>(GetAppProperty(property, TLV_OWNER_INFO));
    ASSERT_NE(ownerInfo, nullptr);
    std::string ownerId(ownerInfo->ownerId);

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_[std::to_string(userId)].insert(ownerId);
    cache.cacheLoaded = true;

    EXPECT_TRUE(cache.IsControlled(userId, ownerId));

    EXPECT_EQ(
        SetAppSpawnMsgFlag(property->message, TLV_MSG_FLAGS, APP_FLAGS_CONTROLLED_APP), 0);
    EXPECT_TRUE(CheckAppMsgFlagsSet(property, APP_FLAGS_CONTROLLED_APP));
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_NotControlled_NoFlag, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    AppSpawnMsgDacInfo *dacInfo =
        reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    ASSERT_NE(dacInfo, nullptr);
    uint32_t userId = dacInfo->uid / UID_BASE;

    AppSpawnMsgOwnerId *ownerInfo =
        reinterpret_cast<AppSpawnMsgOwnerId *>(GetAppProperty(property, TLV_OWNER_INFO));
    ASSERT_NE(ownerInfo, nullptr);

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = true;
    cache.controlledApps_.clear();

    EXPECT_FALSE(cache.IsControlled(userId, std::string(ownerInfo->ownerId)));
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_CacheLoadFails_DegradedMount, TestSize.Level0)
{
    system::SetParameter("security.dlp.transparent.crypto.status", "1");

    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = false;
    cache.controlledApps_.clear();
    (void)remove(CONTROLLED_APPS_JSON_PATH);

    int32_t result = cache.ComputeForSpawn(property);
    EXPECT_EQ(result, 0);

    std::string errCode = system::GetParameter("startup.appspawn.dlp_errorcode", "0");
    EXPECT_EQ(errCode, "-1");
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_SecurityContextMissing_OwnerInfoNull, TestSize.Level0)
{
    system::SetParameter("security.dlp.transparent.crypto.status", "1");

    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    AppSpawnMsgOwnerId *ownerInfo =
        reinterpret_cast<AppSpawnMsgOwnerId *>(GetAppProperty(property, TLV_OWNER_INFO));
    EXPECT_EQ(ownerInfo, nullptr);

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = true;
    cache.controlledApps_["100"].insert("com.example.myapplication");

    int32_t result = cache.ComputeForSpawn(property);
    EXPECT_EQ(result, -1);
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_SecurityContextPresent_NormalPath, TestSize.Level0)
{
    system::SetParameter("security.dlp.transparent.crypto.status", "1");

    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    AppSpawnMsgDacInfo *dacInfo =
        reinterpret_cast<AppSpawnMsgDacInfo *>(GetAppProperty(property, TLV_DAC_INFO));
    AppSpawnMsgOwnerId *ownerInfo =
        reinterpret_cast<AppSpawnMsgOwnerId *>(GetAppProperty(property, TLV_OWNER_INFO));
    EXPECT_NE(dacInfo, nullptr);
    EXPECT_NE(ownerInfo, nullptr);

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = true;
    cache.controlledApps_["999"].insert("not.matching.app");

    int32_t result = cache.ComputeForSpawn(property);
    EXPECT_EQ(result, 0);
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_SecurityContextMissing_CryptoUpdate, TestSize.Level0)
{
    system::SetParameter("security.dlp.transparent.crypto.status", "2");

    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    std::string json = "{\"100\": [\"com.example.myapplication\"]}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = false;

    int32_t result = cache.ComputeForSpawn(property);
    EXPECT_EQ(result, -1);
}

HWTEST_F(AppSpawnControlledAppTest, ComputeForSpawn_CryptoUnavailable_NoSecurityCheck, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle = nullptr;
    ASSERT_EQ(AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle), 0);
    AppSpawnReqMsgHandle reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 1);
    ASSERT_NE(reqHandle, INVALID_REQ_HANDLE);
    AppSpawningCtx *property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
    ASSERT_NE(property, nullptr);

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    int32_t result = cache.ComputeForSpawn(property);
    EXPECT_EQ(result, 0);
}

HWTEST_F(AppSpawnControlledAppTest, LoadFromJson_FileNotFound_ReturnsFalse, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJson();
    EXPECT_FALSE(result);
}

HWTEST_F(AppSpawnControlledAppTest, LoadFromJson_MutexReleased_AfterCall, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.pre");
    cache.cacheLoaded = true;

    cache.LoadFromJson();
    EXPECT_TRUE(cache.IsControlled(100, "com.app.pre"));
}

HWTEST_F(AppSpawnControlledAppTest, LoadFromJsonLocked_DirectCall_FileNotFound, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_FALSE(result);
}

HWTEST_F(AppSpawnControlledAppTest, EdgeCase_InitialState_CacheEmpty, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    EXPECT_FALSE(cache.cacheLoaded);
    EXPECT_TRUE(cache.controlledApps_.empty());
}

HWTEST_F(AppSpawnControlledAppTest, EdgeCase_SpecialCharOwnerId, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.with-dots.more.parts");
    cache.controlledApps_["100"].insert("com.app.with_underscore");
    cache.controlledApps_["100"].insert("MixedCase.App.Name");
    cache.cacheLoaded = true;

    EXPECT_TRUE(cache.IsControlled(100, "com.app.with-dots.more.parts"));
    EXPECT_TRUE(cache.IsControlled(100, "com.app.with_underscore"));
    EXPECT_TRUE(cache.IsControlled(100, "MixedCase.App.Name"));
}

HWTEST_F(AppSpawnControlledAppTest, EdgeCase_ConcurrentRead_IsControlledUnderLock, TestSize.Level0)
{
    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.controlledApps_["100"].insert("com.app.stable");
    cache.cacheLoaded = true;

    for (int i = 0; i < 100; i++) {
        EXPECT_TRUE(cache.IsControlled(100, "com.app.stable"));
        EXPECT_FALSE(cache.IsControlled(100, "com.app.other"));
    }
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_ValidJson_Success, TestSize.Level0)
{
    std::string json = "{"
        "\"100\": [\"com.app.one\", \"com.app.two\"],"
        "\"200\": [\"com.app.three\"]"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_TRUE(result);

    EXPECT_TRUE(cache.IsControlled(100, "com.app.one"));
    EXPECT_TRUE(cache.IsControlled(100, "com.app.two"));
    EXPECT_TRUE(cache.IsControlled(200, "com.app.three"));
    EXPECT_FALSE(cache.IsControlled(100, "com.app.three"));
    EXPECT_FALSE(cache.IsControlled(300, "com.app.one"));
}

HWTEST_F(AppSpawnControlledAppTest, File_LoaFromJsonLocked_EmptyJson_Success, TestSize.Level0)
{
    ASSERT_TRUE(WriteControlledAppJson("{}"));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_TRUE(result);
    EXPECT_TRUE(cache.controlledApps_.empty());
    EXPECT_FALSE(cache.IsControlled(100, "any"));
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_NotObject_Fails, TestSize.Level0)
{
    ASSERT_TRUE(WriteControlledAppJson("[\"not\", \"an\", \"object\"]"));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_FALSE(result);
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_NonArrayValue_Skipped, TestSize.Level0)
{
    std::string json = "{"
        "\"100\": [\"com.app.one\"],"
        "\"200\": \"not_an_array_value\""
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_TRUE(result);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.one"));
    EXPECT_FALSE(cache.IsControlled(200, "any"));
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_InvalidUserId_Skipped, TestSize.Level0)
{
    std::string json = "{"
        "\"100\": [\"com.app.one\"],"
        "\"abc_user\": [\"com.app.invalid\"]"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_TRUE(result);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.one"));
    EXPECT_EQ(cache.controlledApps_.size(), 1u);
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_NullItem_Skipped, TestSize.Level0)
{
    std::string json = "{"
        "\"100\": [\"com.app.one\"]"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_TRUE(result);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.one"));
    EXPECT_EQ(cache.controlledApps_["100"].size(), 1u);
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_ViaLoadFromJson, TestSize.Level0)
{
    std::string json = "{"
        "\"100\": [\"com.app.wrapped\"]"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJson(); // public wrapper with mutex
    EXPECT_TRUE(result);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.wrapped"));
}

HWTEST_F(AppSpawnControlledAppTest, File_EnsureCacheLoaded_Ready_CacheLost_Success, TestSize.Level0)
{
    std::string json = "{"
        "\"100\": [\"com.app.reload\"]"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = false;

    int32_t ret = cache.EnsureCacheLoaded(CRYPTO_READY);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(cache.cacheLoaded);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.reload"));
}

HWTEST_F(AppSpawnControlledAppTest, File_EnsureCacheLoaded_Update_Success, TestSize.Level0)
{
    std::string json = "{"
        "\"200\": [\"com.app.update\"]"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    cache.cacheLoaded = false;

    int32_t ret = cache.EnsureCacheLoaded(CRYPTO_UPDATE);
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(cache.cacheLoaded);
    EXPECT_TRUE(cache.IsControlled(200, "com.app.update"));
}

HWTEST_F(AppSpawnControlledAppTest, File_EnsureCacheLoaded_Update_OverwritesOldCache, TestSize.Level0)
{
    std::string oldJson = "{\"100\": [\"old.app\"]}";
    ASSERT_TRUE(WriteControlledAppJson(oldJson));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool loaded = cache.LoadFromJsonLocked();
    ASSERT_TRUE(loaded);
    EXPECT_TRUE(cache.IsControlled(100, "old.app"));

    std::string newJson = "{\"200\": [\"new.app\"]}";
    ASSERT_TRUE(WriteControlledAppJson(newJson));

    int32_t ret = cache.EnsureCacheLoaded(CRYPTO_UPDATE);
    EXPECT_EQ(ret, 0);
    EXPECT_FALSE(cache.IsControlled(100, "old.app"));
    EXPECT_TRUE(cache.IsControlled(200, "new.app"));
}

HWTEST_F(AppSpawnControlledAppTest, File_IsControlled_Negative_AfterLoad, TestSize.Level0)
{
    std::string json = "{\"100\": [\"com.match\"]}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    ASSERT_TRUE(cache.LoadFromJsonLocked());

    EXPECT_FALSE(cache.IsControlled(100, "com.nomatch"));
    EXPECT_FALSE(cache.IsControlled(200, "com.match"));
    EXPECT_FALSE(cache.IsControlled(200, "com.nomatch"));
    EXPECT_FALSE(cache.IsControlled(999, "com.match"));
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_EmptyUserId_Skipped, TestSize.Level0)
{
    std::string json = "{"
        "\"\": [\"com.app.empty.key\"],"
        "\"100\": [\"com.app.one\"]"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_TRUE(result);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.one"));
    // empty userId should be skipped, not stored
    EXPECT_EQ(cache.controlledApps_.size(), 1u);
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_NonStringItem_Skipped, TestSize.Level0)
{
    std::string json = "{"
        "\"100\": [123, true, 3.14, \"com.app.valid\"]"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_TRUE(result);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.valid"));
    // non-string items should be skipped
    EXPECT_EQ(cache.controlledApps_["100"].size(), 1u);
}

HWTEST_F(AppSpawnControlledAppTest, File_LoadFromJsonLocked_NullValue_NonArray_Skipped, TestSize.Level0)
{
    std::string json = "{"
        "\"100\": [\"com.app.one\"],"
        "\"200\": null"
        "}";
    ASSERT_TRUE(WriteControlledAppJson(json));

    ControlledAppCache &cache = ControlledAppCache::GetInstance();
    bool result = cache.LoadFromJsonLocked();
    EXPECT_TRUE(result);
    EXPECT_TRUE(cache.IsControlled(100, "com.app.one"));
    // null value (not array) should be skipped
    EXPECT_EQ(cache.controlledApps_.size(), 1u);
}

} // namespace OHOS
