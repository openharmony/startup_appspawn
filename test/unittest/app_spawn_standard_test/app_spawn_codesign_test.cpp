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
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <gtest/gtest.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "appspawn.h"
#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"
#include "code_sign_attr_utils.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
static AppSpawnTestHelper g_testHelper;
static const char *APP_SIGN_TYPE_ENTERPRISE_RESIGN = "enterpriseReSign";

using MsgConfigFunc = int (*)(AppSpawnReqMsgHandle reqHandle, const void *context);

struct StringInfoConfig {
    const char *name;
    const char *value;
};

struct StringInfoConfigList {
    const StringInfoConfig *configs;
    size_t count;
};

template <size_t N>
static StringInfoConfigList MakeStringInfoConfigList(const StringInfoConfig (&configs)[N])
{
    return { configs, N };
}

static const char *const APP_DISTRIBUTION_TYPES[] = {
    XPM_DISTRIBUTION_STR_NONE,
    XPM_DISTRIBUTION_STR_APP_GALLERY,
    XPM_DISTRIBUTION_STR_ENTERPRISE,
    XPM_DISTRIBUTION_STR_ENTERPRISE_NORMAL,
    XPM_DISTRIBUTION_STR_ENTERPRISE_MDM,
    XPM_DISTRIBUTION_STR_INTERNALTESTING,
    XPM_DISTRIBUTION_STR_OS_INTEGRATION,
    XPM_DISTRIBUTION_STR_CROWDTESTING,
};

class AppSpawnCodeSignTest : public testing::Test {
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

static int AddStringInfoConfigs(AppSpawnReqMsgHandle reqHandle, const void *context)
{
    const StringInfoConfigList *stringInfoList = static_cast<const StringInfoConfigList *>(context);
    APPSPAWN_CHECK_ONLY_EXPER(
        stringInfoList != nullptr && stringInfoList->configs != nullptr,
        return APPSPAWN_ARG_INVALID);
    for (size_t index = 0; index < stringInfoList->count; ++index) {
        int ret = AppSpawnReqMsgAddStringInfo(reqHandle,
            stringInfoList->configs[index].name, stringInfoList->configs[index].value);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add string info %{public}zu", index);
    }
    return 0;
}

static int SetAppFlagConfig(AppSpawnReqMsgHandle reqHandle, const void *context)
{
    const AppFlagsIndex *appFlag = static_cast<const AppFlagsIndex *>(context);
    AppSpawnReqMsgSetAppFlag(reqHandle, *appFlag);
    return 0;
}

static int RunSetXpmConfigTest(MsgConfigFunc config = nullptr, const void *context = nullptr)
{
    AppSpawnClientHandle clientHandle = nullptr;
    AppSpawnReqMsgHandle reqHandle = 0;
    AppSpawningCtx *property = nullptr;
    AppSpawnMgr *mgr = nullptr;
    int ret = -1;
    do {
        mgr = CreateAppSpawnMgr(MODE_FOR_APP_SPAWN);
        APPSPAWN_CHECK(mgr != nullptr, break, "Failed to create mgr %{public}s", APPSPAWN_SERVER_NAME);
        ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to create reqMgr %{public}s", APPSPAWN_SERVER_NAME);
        ret = -1;
        reqHandle = g_testHelper.CreateMsg(clientHandle, MSG_APP_SPAWN, 0);
        APPSPAWN_CHECK(reqHandle != INVALID_REQ_HANDLE, break,
            "Failed to create req %{public}s", APPSPAWN_SERVER_NAME);
        if (config != nullptr) {
            ret = config(reqHandle, context);
            APPSPAWN_CHECK(ret == 0, break, "Failed to configure req %{public}s", APPSPAWN_SERVER_NAME);
        }

        ret = -1;
        property = g_testHelper.GetAppProperty(clientHandle, reqHandle);
        APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, break);
        ret = SetXpmConfig(mgr, property);
        EXPECT_EQ(ret, 0);
    } while (0);
    DeleteAppSpawningCtx(property);
    AppSpawnClientDestroy(clientHandle);
    DeleteAppSpawnMgr(mgr);
    return ret;
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_001, TestSize.Level0)
{
    ASSERT_EQ(RunSetXpmConfigTest(), 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_002, TestSize.Level0)
{
    const StringInfoConfig appSignType[] = {
        { MSG_EXT_NAME_APP_SIGN_TYPE, "none" },
    };
    const StringInfoConfigList appSignTypeList = MakeStringInfoConfigList(appSignType);
    ASSERT_EQ(RunSetXpmConfigTest(AddStringInfoConfigs, &appSignTypeList), 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_003, TestSize.Level0)
{
    const StringInfoConfig appSignType[] = {
        { MSG_EXT_NAME_APP_SIGN_TYPE, APP_SIGN_TYPE_ENTERPRISE_RESIGN },
    };
    const StringInfoConfigList appSignTypeList = MakeStringInfoConfigList(appSignType);
    ASSERT_EQ(RunSetXpmConfigTest(AddStringInfoConfigs, &appSignTypeList), 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_004, TestSize.Level0)
{
    const StringInfoConfig provisionType[] = {
        { MSG_EXT_NAME_PROVISION_TYPE, "debug" },
    };
    const StringInfoConfigList provisionTypeList = MakeStringInfoConfigList(provisionType);
    ASSERT_EQ(RunSetXpmConfigTest(AddStringInfoConfigs, &provisionTypeList), 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_005, TestSize.Level0)
{
    const AppFlagsIndex appFlag = APP_FLAGS_TEMP_JIT;
    ASSERT_EQ(RunSetXpmConfigTest(SetAppFlagConfig, &appFlag), 0);
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_006, TestSize.Level0)
{
    for (const char *distributionType : APP_DISTRIBUTION_TYPES) {
        SCOPED_TRACE(distributionType);
        const StringInfoConfig appDistributionType[] = {
            { MSG_EXT_NAME_APP_DISTRIBUTION_TYPE, distributionType },
        };
        const StringInfoConfigList appDistributionTypeList = MakeStringInfoConfigList(appDistributionType);
        ASSERT_EQ(RunSetXpmConfigTest(AddStringInfoConfigs, &appDistributionTypeList), 0);
    }
}

HWTEST_F(AppSpawnCodeSignTest, App_Spawn_SetXpmConfig_007, TestSize.Level0)
{
    for (const char *distributionType : APP_DISTRIBUTION_TYPES) {
        SCOPED_TRACE(distributionType);
        const StringInfoConfig appSignAndDistributionType[] = {
            { MSG_EXT_NAME_APP_SIGN_TYPE, APP_SIGN_TYPE_ENTERPRISE_RESIGN },
            { MSG_EXT_NAME_APP_DISTRIBUTION_TYPE, distributionType },
        };
        const StringInfoConfigList appSignAndDistributionTypeList =
            MakeStringInfoConfigList(appSignAndDistributionType);
        ASSERT_EQ(RunSetXpmConfigTest(AddStringInfoConfigs, &appSignAndDistributionTypeList), 0);
    }
}
}  // namespace OHOS
