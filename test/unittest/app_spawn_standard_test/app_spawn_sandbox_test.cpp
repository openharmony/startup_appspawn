/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "sandbox_utils.h"
#include "appspawn_service.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::AppSpawn;
using nlohmann::json;

namespace OHOS {
class AppSpawnSandboxTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppSpawnSandboxTest::SetUpTestCase()
{}

void AppSpawnSandboxTest::TearDownTestCase()
{}

void AppSpawnSandboxTest::SetUp()
{}

void AppSpawnSandboxTest::TearDown()
{}

static ClientSocket::AppProperty *GetAppProperty(void)
{
    static AppSpawnClientExt client;
    return &client.property;
}

HWTEST(AppSpawnSandboxTest, App_Spawn_Sandbox_33, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "App_Spawn_Sandbox_33 start";
    ClientSocket::AppProperty *m_appProperty = GetAppProperty();
    m_appProperty->uid = 1000;
    m_appProperty->gid = 1000;

    const char *srcPath = "/data/app/el1";
    std::string path = srcPath;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path);

    const char *srcPath1 = "/data/app/el2/100/base/ohos.test.bundle1";
    std::string path1 = srcPath1;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path1);

    const char *srcPath2 = "/data/app/el2/100/database/ohos.test.bundle2";
    std::string path2 = srcPath2;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path2);

    const char *srcPath3 = "/data/app/el2/100/test123/ohos.test.bundle3";
    std::string path3 = srcPath3;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path3);

    const char *srcPath4 = "/data/serivce/el2/100/hmdfs/account/data/ohos.test.bundle4";
    std::string path4 = srcPath4;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path4);

    const char *srcPath5 = "/data/serivce/el2/100/hmdfs/non_account/data/ohos.test.bundle5";
    std::string path5 = srcPath5;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path5);

    const char *srcPath6 = "/data/serivce/el2/100/hmdfs/non_account/test/ohos.test.bundle6";
    std::string path6 = srcPath6;
    OHOS::AppSpawn::SandboxUtils::CheckAndPrepareSrcPath(m_appProperty, path6);
    SUCCEED();
}
} // namespace OHOS