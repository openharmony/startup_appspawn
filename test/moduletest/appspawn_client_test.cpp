/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "appspawn_test_client.h"

#include "gtest/gtest.h"
#include "securec.h"

using namespace testing::ext;
using namespace OHOS;

namespace OHOS {
namespace AppSpawn {

class AppSpawnClientTest : public testing::Test, public AppSpawnTestClient {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_AppSpawn_1, TestSize.Level0)
{
    int ret = ClientCreateSocket("/dev/unix/socket/AppSpawn");
    EXPECT_EQ(ret, 0);

    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.clock", "ls -l > aaa.txt");
    request.code = SPAWN_NATIVE_PROCESS;
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    EXPECT_EQ(ret, 0);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_AppSpawn_2, TestSize.Level0)
{
    int ret = ClientCreateSocket("/dev/unix/socket/AppSpawn");
    EXPECT_EQ(ret, 0);

    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.clock", "ls -l > aaa.txt");

    // more gid
    request.gidCount = APP_MAX_GIDS + 1;
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    EXPECT_NE(ret, 0);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
}

// has hsp data,read json from etc
static const std::string g_jsonStr = "{ "
    "\"bundles\": [{ "
        "\"name\":\"test_service\", "
        "\"path\":[\"/data/init_ut/test_service\"], "
        "\"importance\":-20, "
        "\"uid\":\"system\", "
        "\"writepid\":[\"/dev/test_service\"], "
        "\"console\":1, "
        "\"caps\":[\"TEST_ERR\"], "
        "\"gid\":[\"system\"], "
        "\"critical\":1  "
    "}], "
    "\"modules\": [{ "
        "\"name\":\"test_service\", "
        "\"path\":[\"/data/init_ut/test_service\"], "
        "\"importance\":-20, "
        "\"uid\":\"system\", "
        "\"writepid\":[\"/dev/test_service\"], "
        "\"console\":1, "
        "\"caps\":[\"TEST_ERR\"], "
        "\"gid\":[\"system\"], "
        "\"critical\":1  "
    "}], "
    " \"versions\": [{ "
        "\"name\":\"test_service\", "
        "\"path\":[\"/data/init_ut/test_service\"], "
        "\"importance\":-20, "
        "\"uid\":\"system\", "
        "\"writepid\":[\"/dev/test_service\"], "
        "\"console\":1, "
        "\"caps\":[\"TEST_ERR\"], "
        "\"gid\":[\"system\"], "
        "\"critical\":1  "
    "}] "
"}";

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_AppSpawn_3, TestSize.Level0)
{
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_3");
    int ret = ClientCreateSocket("/dev/unix/socket/AppSpawn");
    EXPECT_EQ(ret, 0);

    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.clock", "ls -l > aaa.txt");

    printf("AppSpawn_Client_AppSpawn_3 hsp %zu %s \n", g_jsonStr.size(), g_jsonStr.c_str());
    request.hspList.totalLength = g_jsonStr.size();
    std::vector<char *> data(sizeof(request) + g_jsonStr.size());
    memcpy_s(data.data(), sizeof(request), &request, sizeof(request));
    memcpy_s(data.data() + sizeof(request), g_jsonStr.size(), g_jsonStr.data(), g_jsonStr.size());
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(data.data()), data.size());
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_AppSpawn_4, TestSize.Level0)
{
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_4");
    int ret = ClientCreateSocket("/dev/unix/socket/AppSpawn");
    EXPECT_EQ(ret, 0);

    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.clock", "ls -l > aaa.txt");

    // has hsp data,read json from etc
    const std::string jsonPath("/system/etc/sandbox/system-sandbox.json");
    std::ifstream jsonFileStream;
    jsonFileStream.open(jsonPath.c_str(), std::ios::in);
    EXPECT_EQ(jsonFileStream.is_open() == true, 1);
    std::vector<char> buf;
    char ch;
    while (jsonFileStream.get(ch)) {
        buf.insert(buf.end(), ch);
    }
    jsonFileStream.close();

    printf("AppSpawn_Client_AppSpawn_4 hsp %zu \n", buf.size());
    request.hspList.totalLength = buf.size();
    std::vector<char *> data(sizeof(request) + buf.size());
    memcpy_s(data.data(), sizeof(request), &request, sizeof(request));
    memcpy_s(data.data() + sizeof(request), buf.size(), buf.data(), buf.size());
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(data.data()), data.size());
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_AppSpawn_5, TestSize.Level0)
{
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_5 start");
    int ret = ClientCreateSocket("/dev/unix/socket/AppSpawn");
    EXPECT_EQ(ret, 0);

    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.clock", "ls -l > aaa.txt");

    // more hsp data,read json from etc
    const std::string jsonPath("/system/etc/sandbox/appdata-sandbox.json");
    std::ifstream jsonFileStream;
    jsonFileStream.open(jsonPath.c_str(), std::ios::in);
    EXPECT_EQ(jsonFileStream.is_open() == true, 1);
    std::vector<char> buf;
    char ch;
    while (jsonFileStream.get(ch)) {
        buf.insert(buf.end(), ch);
    }
    jsonFileStream.close();

    request.hspList.totalLength = buf.size();
    std::vector<char *> data(sizeof(request) + buf.size());
    memcpy_s(data.data(), sizeof(request), &request, sizeof(request));
    memcpy_s(data.data() + sizeof(request), buf.size(), buf.data(), buf.size());
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(data.data()), data.size());
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_5 end");
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_AppSpawn_6, TestSize.Level0)
{
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_6 start");
    int ret = ClientCreateSocket("/dev/unix/socket/AppSpawn");
    EXPECT_EQ(ret, 0);

    SetParameter("startup.appspawn.cold.boot", "1");
    SetParameter("persist.appspawn.client.timeout", "10");
    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.clock", "ls -l > /data/aaa.txt");
    request.flags = APP_COLD_BOOT;
    request.code = SPAWN_NATIVE_PROCESS;
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    EXPECT_EQ(ret, 0);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_6 end");
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_AppSpawn_7, TestSize.Level0)
{
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_7 start");
    int ret = ClientCreateSocket("/dev/unix/socket/AppSpawn");
    EXPECT_EQ(ret, 0);

    SetParameter("startup.appspawn.cold.boot", "1");
    SetParameter("persist.appspawn.client.timeout", "10");
    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.2222222222222clock", "ls -l > /data/aaa.txt");
    request.flags = APP_COLD_BOOT | APP_NO_SANDBOX;
    request.code = SPAWN_NATIVE_PROCESS;
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    EXPECT_EQ(ret, 0);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_7 end");
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_AppSpawn_8, TestSize.Level0)
{
    // for clod start, but not in sandbox
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_8 start");
    int ret = ClientCreateSocket("/dev/unix/socket/AppSpawn");
    EXPECT_EQ(ret, 0);

    SetParameter("startup.appspawn.cold.boot", "1");
    SetParameter("persist.appspawn.client.timeout", "10");
    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.2222222222222clock", "ls -l > /data/aaa.txt");
    request.flags = APP_COLD_BOOT | APP_NO_SANDBOX;
    request.code = SPAWN_NATIVE_PROCESS;
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    EXPECT_EQ(ret, 0);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
    APPSPAWN_LOGI("AppSpawn_Client_AppSpawn_8 end");
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_NWebSpawn_1, TestSize.Level0)
{
    int ret = ClientCreateSocket("/dev/unix/socket/NWebSpawn");
    EXPECT_EQ(ret, 0);

    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.clock", "ls -l > aaa.txt");
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    EXPECT_EQ(ret, 0);
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    // close client
    ClientClose();
}

HWTEST_F(AppSpawnClientTest, AppSpawn_Client_NWebSpawn_2, TestSize.Level0)
{
    int ret = ClientCreateSocket("/dev/unix/socket/NWebSpawn");
    EXPECT_EQ(ret, 0);

    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    ClientFillMsg(&request, "ohos.samples.clock", "ls -l > aaa.txt");
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    EXPECT_EQ(ret, 0);
    pid_t pid = 0;
    ret = ClientRecvMsg(pid);
    EXPECT_EQ(ret, 0);
    printf("AppSpawn_Client_NWebSpawn_2 pid %d \n", pid);
    request.code = GET_RENDER_TERMINATION_STATUS;
    request.pid = pid;
    ret = ClientSendMsg(reinterpret_cast<const uint8_t *>(&request), sizeof(request));
    EXPECT_EQ(ret, 0);
    ret = ClientRecvMsg(pid);
    printf("AppSpawn_Client_NWebSpawn_2 result %d \n", ret);
    // close client
    ClientClose();
}
}  // namespace AppSpawn
}  // namespace OHOS