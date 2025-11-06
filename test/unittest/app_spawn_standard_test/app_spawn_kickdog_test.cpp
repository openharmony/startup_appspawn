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

#include "appspawn_kickdog.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnKickDogTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

static int CheckFileContent(const char *filePath, const char *targetStr)
{
    char buf[100];

    if (filePath == nullptr || targetStr == nullptr) {
        printf("para invalid\n");
        return -1;
    }

    int fileFd = open(filePath, O_RDWR);
    if (fileFd == -1) {
        printf("open %s fail,errno:%d\n", filePath, errno);
        return fileFd;
    }

    int readResult = read(fileFd, buf, sizeof(buf) - 1);
    if (readResult <= 0) {
        printf("read %s fail,result:%d\n", filePath, readResult);
        close(fileFd);
        return -1;
    }

    if (strcmp(buf, targetStr) != 0) {
        printf("read buf %s is not euqal target str:%s\n", buf, targetStr);
        close(fileFd);
        return -1;
    } else {
        printf("read buf %s is euqal to target str:%s\n", buf, targetStr);
        ftruncate(fileFd, 0);
    }

    close(fileFd);
    return 0;
}

static bool CheckDeviceInLinux()
{
    struct utsname uts;
    if (uname(&uts) == -1) {
        printf("uname get failed");
        return false;
    }
    if (strcmp(uts.sysname, "Linux") == 0) {
        printf("uname sysname is Linux");
        return true;
    }
    return false;
}

/**
 * @brief watchdog开启及定时kickdog
 *
 */
HWTEST_F(AppSpawnKickDogTest, App_Spawn_AppSpawnKickDog_001, TestSize.Level0)
{
    int fileFd = open(HM_APPSPAWN_WATCHDOG_FILE, O_CREAT);
    EXPECT_EQ(fileFd != -1, 1);
    close(fileFd);

    std::unique_ptr<OHOS::AppSpawnTestServer> testServer =
        std::make_unique<OHOS::AppSpawnTestServer>("appspawn -mode appspawn");
    AddPreloadHook(HOOK_PRIO_COMMON, SpawnKickDogStart);
    testServer->Start(nullptr);
    if (CheckDeviceInLinux()) {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_ON), 0);
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_KICK), -1);
    } else {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_ON), 0);
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_KICK), -1);
    }
    sleep(12); // wait for kick dog(kick every 10 seconds)
    if (CheckDeviceInLinux()) {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_ON), -1);
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_KICK), 0);
    } else {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_ON), -1);
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_KICK), 0);
    }
    sleep(10);
    if (CheckDeviceInLinux()) {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_KICK), 0);
    } else {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_APPSPAWN_WATCHDOG_KICK), 0);
    }
    testServer->Stop();
    EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, nullptr), -1);

    remove(HM_APPSPAWN_WATCHDOG_FILE);
}

/**
 * @brief nwebspawn接入hungtask定时kickdog
 *
 */
HWTEST_F(AppSpawnKickDogTest, App_Spawn_AppSpawnKickDog_002, TestSize.Level0)
{
    int fileFd = open(HM_APPSPAWN_WATCHDOG_FILE, O_CREAT);
    EXPECT_EQ(fileFd != -1, 1);
    close(fileFd);

    std::unique_ptr<OHOS::AppSpawnTestServer> testServer =
        std::make_unique<OHOS::AppSpawnTestServer>("appspawn -mode nwebspawn");
    AddPreloadHook(HOOK_PRIO_COMMON, SpawnKickDogStart);
    testServer->Start(nullptr);
    if (CheckDeviceInLinux()) {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_ON), 0);
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_KICK), -1);
    } else {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_NWEBSPAWN_WATCHDOG_ON), 0);
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_NWEBSPAWN_WATCHDOG_KICK), -1);
    }
    sleep(12); // wait for kick dog(kick every 10 seconds)
    if (CheckDeviceInLinux()) {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_ON), -1);
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_KICK), 0);
    } else {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_NWEBSPAWN_WATCHDOG_ON), -1);
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_NWEBSPAWN_WATCHDOG_KICK), 0);
    }
    sleep(10);
    if (CheckDeviceInLinux()) {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, LINUX_APPSPAWN_WATCHDOG_KICK), 0);
    } else {
        EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, HM_NWEBSPAWN_WATCHDOG_KICK), 0);
    }
    testServer->Stop();
    EXPECT_EQ(CheckFileContent(HM_APPSPAWN_WATCHDOG_FILE, nullptr), -1);

    remove(HM_APPSPAWN_WATCHDOG_FILE);
}
}  // namespace OHOS
