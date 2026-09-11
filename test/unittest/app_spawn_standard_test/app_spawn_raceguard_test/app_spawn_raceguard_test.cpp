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
#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <string>

#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn.h"
#include "securec.h"

#ifdef __cplusplus
extern "C" {
#endif

// APPSPAWN_STATIC 展开为空(APPSPAWN_TEST), 模块内静态函数可直接调用
int SetRaceGuardPreloadEnv(AppSpawnMgr *content, AppSpawningCtx *property);
int BuildRaceGuardPreloadValue(const char *curValue, char *out, size_t outLen);

#ifdef __cplusplus
}
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace {
constexpr const char *RACEGUARD_SO_PATH = "/system/lib64/libraceguard.z.so";
constexpr const char *OTHER_SO_PATH = "/system/lib64/libother.z.so";
constexpr size_t ENV_BUFFER_LEN = 512;  // 与 RACEGUARD_ENV_BUFFER 一致

// --wrap=access 桩: 控制 raceguard so 的存在性, 其余路径透传真实 access
bool g_raceguardSoExists = false;

struct TestSpawnCtx {
    AppSpawnMgr content = {};
    AppSpawningCtx property = {};
    AppSpawnMsgNode msgNode = {};
};

void InitTestCtx(TestSpawnCtx &ctx, RunMode mode, AppSpawnMsgType msgType, uint32_t flags)
{
    ctx.content.content.mode = mode;
    ctx.msgNode.msgHeader.msgType = msgType;
    (void)strcpy_s(ctx.msgNode.msgHeader.processName, sizeof(ctx.msgNode.msgHeader.processName),
        "raceguard_ut_process");
    ctx.property.message = &ctx.msgNode;
    ctx.property.client.flags = flags;
}

void ResetTestEnv()
{
    g_raceguardSoExists = false;
    unsetenv("LD_PRELOAD");
}
}  // namespace

extern "C" int __real_access(const char *pathname, int mode);

extern "C" int __wrap_access(const char *pathname, int mode)
{
    if (pathname != nullptr && strcmp(pathname, RACEGUARD_SO_PATH) == 0) {
        return g_raceguardSoExists ? 0 : -1;
    }
    return __real_access(pathname, mode);
}

namespace OHOS {
class AppSpawnRaceGuardTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        ResetTestEnv();
    }
    void TearDown()
    {
        ResetTestEnv();
    }
};

/**
 * @tc.name: RaceGuardModeGuard01
 * @tc.desc: module loaded by non-appspawn server (nweb mode), skip inject
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, RaceGuardModeGuard01, TestSize.Level1)
{
    TestSpawnCtx ctx;
    InitTestCtx(ctx, MODE_FOR_NWEB_SPAWN, MSG_APP_SPAWN, 0);
    g_raceguardSoExists = true;

    int ret = SetRaceGuardPreloadEnv(&ctx.content, &ctx.property);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.property.client.flags & APP_COLD_START, 0u);
    EXPECT_EQ(getenv("LD_PRELOAD"), nullptr);
}

/**
 * @tc.name: RaceGuardNativeMsgTypeSkip01
 * @tc.desc: native process spawn msg, skip inject
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, RaceGuardNativeMsgTypeSkip01, TestSize.Level1)
{
    TestSpawnCtx ctx;
    InitTestCtx(ctx, MODE_FOR_APP_SPAWN, MSG_SPAWN_NATIVE_PROCESS, 0);
    g_raceguardSoExists = true;

    int ret = SetRaceGuardPreloadEnv(&ctx.content, &ctx.property);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.property.client.flags & APP_COLD_START, 0u);
    EXPECT_EQ(getenv("LD_PRELOAD"), nullptr);
}

/**
 * @tc.name: RaceGuardColdStartAlreadySet01
 * @tc.desc: cold start flag already set (asan/dfx first), skip and keep env
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, RaceGuardColdStartAlreadySet01, TestSize.Level1)
{
    TestSpawnCtx ctx;
    InitTestCtx(ctx, MODE_FOR_APP_SPAWN, MSG_APP_SPAWN, APP_COLD_START);
    g_raceguardSoExists = true;
    setenv("LD_PRELOAD", "/system/lib64/libclang_rt.asan.so", 1);

    int ret = SetRaceGuardPreloadEnv(&ctx.content, &ctx.property);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.property.client.flags, static_cast<uint32_t>(APP_COLD_START));
    EXPECT_STREQ(getenv("LD_PRELOAD"), "/system/lib64/libclang_rt.asan.so");
}

/**
 * @tc.name: RaceGuardSoMissingDegrade01
 * @tc.desc: raceguard so missing, degrade and keep normal spawn
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, RaceGuardSoMissingDegrade01, TestSize.Level1)
{
    TestSpawnCtx ctx;
    InitTestCtx(ctx, MODE_FOR_APP_SPAWN, MSG_APP_SPAWN, 0);
    g_raceguardSoExists = false;  // wrap access returns -1

    int ret = SetRaceGuardPreloadEnv(&ctx.content, &ctx.property);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.property.client.flags & APP_COLD_START, 0u);
    EXPECT_EQ(getenv("LD_PRELOAD"), nullptr);
}

/**
 * @tc.name: RaceGuardPreloadInject01
 * @tc.desc: so exists, inject and set APP_COLD_START flag
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, RaceGuardPreloadInject01, TestSize.Level1)
{
    TestSpawnCtx ctx;
    InitTestCtx(ctx, MODE_FOR_APP_SPAWN, MSG_APP_SPAWN, 0);
    g_raceguardSoExists = true;

    int ret = SetRaceGuardPreloadEnv(&ctx.content, &ctx.property);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(ctx.property.client.flags & APP_COLD_START, 0u);
    EXPECT_STREQ(getenv("LD_PRELOAD"), RACEGUARD_SO_PATH);
}

/**
 * @tc.name: RaceGuardPreloadAppend01
 * @tc.desc: LD_PRELOAD already set, append with colon not overwrite
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, RaceGuardPreloadAppend01, TestSize.Level1)
{
    TestSpawnCtx ctx;
    InitTestCtx(ctx, MODE_FOR_APP_SPAWN, MSG_APP_SPAWN, 0);
    g_raceguardSoExists = true;
    setenv("LD_PRELOAD", OTHER_SO_PATH, 1);

    int ret = SetRaceGuardPreloadEnv(&ctx.content, &ctx.property);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(ctx.property.client.flags & APP_COLD_START, 0u);
    std::string expected = std::string(OTHER_SO_PATH) + ":" + RACEGUARD_SO_PATH;
    EXPECT_STREQ(getenv("LD_PRELOAD"), expected.c_str());
}

/**
 * @tc.name: RaceGuardPreloadIdempotent01
 * @tc.desc: LD_PRELOAD already contains raceguard so, no duplicate append
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, RaceGuardPreloadIdempotent01, TestSize.Level1)
{
    TestSpawnCtx ctx;
    InitTestCtx(ctx, MODE_FOR_APP_SPAWN, MSG_APP_SPAWN, 0);
    g_raceguardSoExists = true;
    std::string pre = std::string(OTHER_SO_PATH) + ":" + RACEGUARD_SO_PATH;
    setenv("LD_PRELOAD", pre.c_str(), 1);

    int ret = SetRaceGuardPreloadEnv(&ctx.content, &ctx.property);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.property.client.flags & APP_COLD_START, 0u);
    EXPECT_STREQ(getenv("LD_PRELOAD"), pre.c_str());
}

/**
 * @tc.name: RaceGuardNullPointerGuard01
 * @tc.desc: null content/property pointer, return 0 no crash
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, RaceGuardNullPointerGuard01, TestSize.Level1)
{
    g_raceguardSoExists = true;
    EXPECT_EQ(SetRaceGuardPreloadEnv(nullptr, nullptr), 0);
}

/**
 * @tc.name: BuildRaceGuardPreloadValue01
 * @tc.desc: null/empty current value, write constant path only
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, BuildRaceGuardPreloadValue01, TestSize.Level1)
{
    char out[ENV_BUFFER_LEN] = {0};
    EXPECT_EQ(BuildRaceGuardPreloadValue(nullptr, out, sizeof(out)), 0);
    EXPECT_STREQ(out, RACEGUARD_SO_PATH);

    (void)memset_s(out, sizeof(out), 0, sizeof(out));
    EXPECT_EQ(BuildRaceGuardPreloadValue("", out, sizeof(out)), 0);
    EXPECT_STREQ(out, RACEGUARD_SO_PATH);
}

/**
 * @tc.name: BuildRaceGuardPreloadValue02
 * @tc.desc: current value set, colon append; already contains, idempotent skip
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, BuildRaceGuardPreloadValue02, TestSize.Level1)
{
    char out[ENV_BUFFER_LEN] = {0};
    EXPECT_EQ(BuildRaceGuardPreloadValue(OTHER_SO_PATH, out, sizeof(out)), 0);
    EXPECT_STREQ(out, (std::string(OTHER_SO_PATH) + ":" + RACEGUARD_SO_PATH).c_str());

    EXPECT_EQ(BuildRaceGuardPreloadValue(RACEGUARD_SO_PATH, out, sizeof(out)), -1);
    EXPECT_EQ(BuildRaceGuardPreloadValue(
        (std::string(OTHER_SO_PATH) + ":" + RACEGUARD_SO_PATH).c_str(), out, sizeof(out)), -1);
}

/**
 * @tc.name: BuildRaceGuardPreloadValue03
 * @tc.desc: current value too long, sprintf_s truncation rejected, no overflow
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnRaceGuardTest, BuildRaceGuardPreloadValue03, TestSize.Level1)
{
    char out[ENV_BUFFER_LEN] = {0};
    std::string tooLong(ENV_BUFFER_LEN + 1, 'a');
    EXPECT_EQ(BuildRaceGuardPreloadValue(tooLong.c_str(), out, sizeof(out)), -1);

    // 拼接后恰好超出缓冲区: cur 500 字符 + 分隔符 + so 路径 > 512
    std::string nearOverflow(ENV_BUFFER_LEN - 12, 'a');
    EXPECT_EQ(BuildRaceGuardPreloadValue(nearOverflow.c_str(), out, sizeof(out)), -1);
    EXPECT_EQ(out[0], '\0');  // 失败路径不写入非法内容
}
}  // namespace OHOS
