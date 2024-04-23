/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstdio>
#include <fcntl.h>
#include <random>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "securec.h"

#include "appspawn_test_cmder.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace AppSpawn {
namespace {
static const int32_t DEFAULT_PID = 0;
static const int32_t FILE_PATH_SIZE = 50;
static const int32_t CMD_SIZE = 50;
static const int32_t BUFFER_SIZE = 512;
static const int32_t BASE_TYPE = 10;
static const int32_t CONNECT_RETRY_DELAY = 50 * 1000;
static const int32_t CONNECT_RETRY_MAX_TIMES = 5;
static const int32_t UID_POSITION_MOVE = 5;
static const int32_t GID_POSITION_MOVE = 5;
static const int32_t GROUPS_POSITION_MOVE = 8;
static const char *DELIMITER_SPACE = " ";
static const char *DELIMITER_NEWLINE = "\n";
static char g_buffer[BUFFER_SIZE + 1] = {};
}  // namespace

bool CheckFileIsExists(const char *filepath)
{
    int32_t retryCount = 0;
    while ((access(filepath, F_OK) != 0) && (retryCount < CONNECT_RETRY_MAX_TIMES)) {
        usleep(CONNECT_RETRY_DELAY);
        retryCount++;
    }
    GTEST_LOG_(INFO) << "retryCount :" << retryCount << ".";
    if (retryCount < CONNECT_RETRY_MAX_TIMES) {
        return true;
    }
    return false;
}

bool CheckFileIsNotExists(const char *filepath)
{
    int32_t retryCount = 0;
    while ((access(filepath, F_OK) == 0) && (retryCount < CONNECT_RETRY_MAX_TIMES)) {
        usleep(CONNECT_RETRY_DELAY);
        retryCount++;
    }
    GTEST_LOG_(INFO) << "retryCount :" << retryCount << ".";
    if (retryCount < CONNECT_RETRY_MAX_TIMES) {
        return true;
    }
    return false;
}

bool ReadFileInfo(char *buffer, const int32_t &pid, const char *fileName)
{
    // Set file path
    char filePath[FILE_PATH_SIZE];
    if (sprintf_s(filePath, sizeof(filePath), "/proc/%d/%s", pid, fileName) <= 0) {
        HILOG_ERROR(LOG_CORE, "filePath sprintf_s fail .");
        return false;
    }
    GTEST_LOG_(INFO) << "ReadFileInfo :" << filePath;
    if (!CheckFileIsExists(filePath)) {
        HILOG_ERROR(LOG_CORE, "file %{public}s is not exists .", fileName);
        return false;
    }
    // Open file
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        HILOG_ERROR(LOG_CORE, "file %{public}s open failed . error:%{public}s", fileName, strerror(errno));
        return false;
    }
    // Read file
    int t = read(fd, buffer, BUFFER_SIZE);
    if (t <= 0 || buffer == nullptr || t > BUFFER_SIZE) {
        HILOG_INFO(LOG_CORE, "read proc status file failed.");
        close(fd);
        return false;
    }
    buffer[t] = '\0';
    GTEST_LOG_(INFO) << "ReadFileInfo buffer :" << buffer;
    HILOG_INFO(LOG_CORE, "buffer:\n %{public}s", buffer);
    close(fd);
    return true;
}

bool CheckUid(const int32_t &pid, const int newUid)
{
    if (ReadFileInfo(g_buffer, pid, "status")) {
        GTEST_LOG_(INFO) << "CheckUid pid " << pid << " buffer :" << g_buffer;

        // Move to Uid position
        char *uidPtr = strstr(g_buffer, "Uid:");
        if (uidPtr == nullptr) {
            HILOG_ERROR(LOG_CORE, "get Uid info failed.");
            return false;
        }
        if (strlen(uidPtr) > UID_POSITION_MOVE) {
            uidPtr = uidPtr + UID_POSITION_MOVE;
        }
        int32_t uid = static_cast<int32_t>(strtol(uidPtr, NULL, BASE_TYPE));
        HILOG_INFO(LOG_CORE, "new proc(%{public}d) uid = %{public}d, setUid=%{public}d.", pid, uid, newUid);
        if (uid == newUid) {
            return true;
        }
    }
    return false;
}

bool CheckGid(const int32_t &pid, const int newGid)
{
    if (ReadFileInfo(g_buffer, pid, "status")) {
        GTEST_LOG_(INFO) << "CheckGid pid " << pid << " buffer :" << g_buffer;

        // Move to Gid position
        char *gidPtr = strstr(g_buffer, "Gid:");
        if (gidPtr == nullptr) {
            HILOG_ERROR(LOG_CORE, "get Gid info failed.");
            return false;
        }
        if (strlen(gidPtr) > GID_POSITION_MOVE) {
            gidPtr = gidPtr + GID_POSITION_MOVE;
        }
        if (gidPtr == nullptr) {
            HILOG_ERROR(LOG_CORE, "get Gid info failed.");
            return false;
        }
        int32_t gid = static_cast<int32_t>(strtol(gidPtr, NULL, BASE_TYPE));
        HILOG_INFO(LOG_CORE, "new proc(%{public}d) gid = %{public}d, setGid=%{public}d.", pid, gid, newGid);
        if (gid == newGid) {
            return true;
        }
    }
    return false;
}

std::size_t GetGids(const int32_t &pid, std::vector<int32_t> &gids)
{
    if (ReadFileInfo(g_buffer, pid, "status")) {
        GTEST_LOG_(INFO) << "GetGids pid " << pid << " buffer :" << g_buffer;
        // Move to Groups position
        char *groupsPtr = strstr(g_buffer, "Groups");
        if (groupsPtr == nullptr || strlen(groupsPtr) > BUFFER_SIZE) {
            HILOG_ERROR(LOG_CORE, "get Groups info failed.");
            return false;
        }
        if (strlen(groupsPtr) > GROUPS_POSITION_MOVE) {
            groupsPtr = groupsPtr + GROUPS_POSITION_MOVE;
        }
        // Get the row content of Groups
        char *savePtr = nullptr;
        if (groupsPtr == nullptr || strlen(groupsPtr) > BUFFER_SIZE) {
            HILOG_ERROR(LOG_CORE, "get Groups info failed.");
            return false;
        }
        char *line = strtok_r(groupsPtr, DELIMITER_NEWLINE, &savePtr);
        if (line == nullptr || strlen(line) > BUFFER_SIZE) {
            HILOG_ERROR(LOG_CORE, "get Groups line info failed.");
            return false;
        }
        // Get each gid and insert into vector
        char *gid = strtok_r(line, DELIMITER_SPACE, &savePtr);
        while (gid != nullptr) {
            gids.push_back(atoi(gid));
            gid = strtok_r(nullptr, DELIMITER_SPACE, &savePtr);
        }
    }
    return gids.size();
}

bool CheckGids(const int32_t &pid, const std::vector<int32_t> newGids)
{
    // Get Gids
    std::vector<int32_t> gids;
    std::size_t gCount = GetGids(pid, gids);
    if ((gCount == newGids.size()) && (gids == newGids)) {
        return true;
    }
    return false;
}

bool CheckGidsCount(const int32_t &pid, const std::vector<int32_t> newGids)
{
    // Get GidsCount
    std::vector<int32_t> gids;
    std::size_t gCount = GetGids(pid, gids);
    if (gCount == newGids.size()) {
        return true;
    }
    return false;
}

bool CheckProcName(const int32_t &pid, const std::string &newProcessName)
{
    FILE *fp = nullptr;
    char cmd[CMD_SIZE];
    if (sprintf_s(cmd, sizeof(cmd), "ps -o ARGS=CMD -p %d |grep -v CMD", pid) <= 0) {
        HILOG_ERROR(LOG_CORE, "cmd sprintf_s fail .");
        return false;
    }
    if (strlen(cmd) > CMD_SIZE) {
        HILOG_ERROR(LOG_CORE, " cmd length is too long  .");
        return false;
    }
    fp = popen(cmd, "r");
    if (fp == nullptr) {
        HILOG_ERROR(LOG_CORE, " popen function call failed .");
        return false;
    }
    if (fgets(g_buffer, sizeof(g_buffer), fp) != nullptr) {
        for (unsigned int i = 0; i < sizeof(g_buffer); i++) {
            if (g_buffer[i] == '\n') {
                g_buffer[i] = '\0';
                break;
            }
        }
        GTEST_LOG_(INFO) << "procName " << " :" << g_buffer << ".";
        if (newProcessName.compare(0, newProcessName.size(), g_buffer, newProcessName.size()) == 0) {
            pclose(fp);
            return true;
        }
        HILOG_ERROR(LOG_CORE, " procName=%{public}s, newProcessName=%{public}s.", g_buffer, newProcessName.c_str());
    } else {
        HILOG_ERROR(LOG_CORE, "Getting procName failed.");
    }
    pclose(fp);
    return false;
}

bool CheckProcessIsDestroyed(const int32_t &pid)
{
    char filePath[FILE_PATH_SIZE];
    if (sprintf_s(filePath, sizeof(filePath), "/proc/%d", pid) <= 0) {
        HILOG_ERROR(LOG_CORE, "filePath sprintf_s fail .");
        return false;
    }
    return CheckFileIsNotExists(filePath);
}

bool CheckAppspawnPID()
{
    FILE *fp = nullptr;
    fp = popen("pidof appspawn", "r");
    if (fp == nullptr) {
        HILOG_ERROR(LOG_CORE, " popen function call failed.");
        return false;
    }
    if (fgets(g_buffer, sizeof(g_buffer), fp) != nullptr) {
        pclose(fp);
        return true;
    }
    HILOG_ERROR(LOG_CORE, "Getting Pid failed.");
    pclose(fp);
    return false;
}

bool StartAppspawn()
{
    HILOG_ERROR(LOG_CORE, " StartAppspawn ");
    FILE *fp = nullptr;
    fp = popen("/system/bin/appspawn&", "r");
    if (fp == nullptr) {
        HILOG_ERROR(LOG_CORE, " popen function call failed.");
        return false;
    }
    pclose(fp);
    return true;
}

bool StopAppspawn()
{
    HILOG_ERROR(LOG_CORE, " StopAppspawn ");
    FILE *fp = nullptr;
    fp = popen("kill -9 $(pidof appspawn)", "r");
    if (fp == nullptr) {
        HILOG_ERROR(LOG_CORE, " popen function call failed.");
        return false;
    }
    pclose(fp);
    return true;
}

static const std::string defaultAppInfo1 = "{ \
    \"msg-type\": \"MSG_APP_SPAWN\", \
    \"msg-flags\": [ 13 ], \
    \"process-name\" : \"com.example.myapplication\", \
    \"dac-info\" : { \
            \"uid\" : 20010041, \
            \"gid\" : 20010041,\
            \"gid-table\" : [1008],\
            \"user-name\" : \"\" \
    },\
    \"access-token\" : {\
            \"accessTokenIdEx\" : 537854093\
    },\
    \"permission\" : [\
    ],\
    \"internet-permission\" : {\
            \"set-allow-internet\" : 0,\
            \"allow-internet\" : 0\
    },\
    \"bundle-info\" : {\
            \"bundle-index\" : 0,\
            \"bundle-name\" : \"com.example.myapplication\" \
    },\
    \"owner-id\" : \"\",\
    \"render-cmd\" : \"1234567890\",\
    \"domain-info\" : {\
            \"hap-flags\" : 0,\
            \"apl\" : \"system_core\"\
    },\
    \"ext-info\" : [\
            {\
                    \"name\" : \"test\",\
                    \"value\" : \"4444444444444444444\" \
            } \
    ]\
}";

static const std::string defaultAppInfo2 = "{ \
    \"msg-type\": \"MSG_SPAWN_NATIVE_PROCESS\", \
    \"msg-flags\": [ 13 ], \
    \"process-name\" : \"com.example.myapplication\", \
    \"dac-info\" : { \
            \"uid\" : 20010043, \
            \"gid\" : 20010043,\
            \"gid-table\" : [],\
            \"user-name\" : \"\" \
    },\
    \"access-token\" : {\
            \"accessTokenIdEx\" : 537854093\
    },\
    \"permission\" : [\
    ],\
    \"internet-permission\" : {\
            \"set-allow-internet\" : 0,\
            \"allow-internet\" : 0\
    },\
    \"bundle-info\" : {\
            \"bundle-index\" : 0,\
            \"bundle-name\" : \"com.example.myapplication\" \
    },\
    \"owner-id\" : \"\",\
    \"render-cmd\" : \"1234567890\",\
    \"domain-info\" : {\
            \"hap-flags\" : 0,\
            \"apl\" : \"system_core\"\
    },\
    \"ext-info\" : [\
            {\
                    \"name\" : \"test\",\
                    \"value\" : \"4444444444444444444\" \
            } \
    ]\
}";

static const std::string defaultAppInfo3 = "{ \
    \"msg-type\": \"MSG_SPAWN_NATIVE_PROCESS\", \
    \"msg-flags\": [ 13 ], \
    \"process-name\" : \"com.example.myapplication\", \
    \"dac-info\" : { \
            \"uid\" : 20010045, \
            \"gid\" : 20010045,\
            \"gid-table\" : [ 20010045, 20010046 ],\
            \"user-name\" : \"\" \
    },\
    \"access-token\" : {\
            \"accessTokenIdEx\" : 537854093\
    },\
    \"permission\" : [\
    ],\
    \"internet-permission\" : {\
            \"set-allow-internet\" : 0,\
            \"allow-internet\" : 0\
    },\
    \"bundle-info\" : {\
            \"bundle-index\" : 0,\
            \"bundle-name\" : \"com.example.myapplication\" \
    },\
    \"owner-id\" : \"\",\
    \"render-cmd\" : \"1234567890\",\
    \"domain-info\" : {\
            \"hap-flags\" : 0,\
            \"apl\" : \"system_core\"\
    },\
    \"ext-info\" : [\
            {\
                    \"name\" : \"test\",\
                    \"value\" : \"4444444444444444444\" \
            } \
    ]\
}";

static const std::string defaultAppInfoNoInternetPermission = "{ \
    \"msg-type\": \"MSG_APP_SPAWN\", \
    \"msg-flags\": [ 13 ], \
    \"process-name\" : \"com.example.myapplication\", \
    \"dac-info\" : { \
            \"uid\" : 20010045, \
            \"gid\" : 20010045,\
            \"gid-table\" : [ 20010045, 20010046 ],\
            \"user-name\" : \"\" \
    },\
    \"access-token\" : {\
            \"accessTokenIdEx\" : 537854093\
    },\
    \"permission\" : [\
    ],\
    \"bundle-info\" : {\
            \"bundle-index\" : 0,\
            \"bundle-name\" : \"com.example.myapplication\" \
    },\
    \"owner-id\" : \"\",\
    \"render-cmd\" : \"1234567890\",\
    \"domain-info\" : {\
            \"hap-flags\" : 0,\
            \"apl\" : \"system_core\"\
    },\
    \"ext-info\" : [\
            {\
                    \"name\" : \"test\",\
                    \"value\" : \"4444444444444444444\" \
            } \
    ]\
}";

static const std::string defaultAppInfoNoDac = "{ \
    \"msg-type\": \"MSG_APP_SPAWN\", \
    \"msg-flags\": [ 13 ], \
    \"process-name\" : \"com.example.myapplication\", \
    \"access-token\" : {\
            \"accessTokenIdEx\" : 537854093\
    },\
    \"permission\" : [\
    ],\
    \"internet-permission\" : {\
            \"set-allow-internet\" : 0,\
            \"allow-internet\" : 0\
    },\
    \"bundle-info\" : {\
            \"bundle-index\" : 0,\
            \"bundle-name\" : \"com.example.myapplication\" \
    },\
    \"owner-id\" : \"\",\
    \"render-cmd\" : \"1234567890\",\
    \"domain-info\" : {\
            \"hap-flags\" : 0,\
            \"apl\" : \"system_core\"\
    },\
    \"ext-info\" : [\
            {\
                    \"name\" : \"test\",\
                    \"value\" : \"4444444444444444444\" \
            } \
    ]\
}";

static const std::string defaultAppInfoNoToken = "{ \
    \"msg-type\": \"MSG_APP_SPAWN\", \
    \"msg-flags\": [ 13 ], \
    \"process-name\" : \"com.example.myapplication\", \
    \"dac-info\" : { \
            \"uid\" : 20010043, \
            \"gid\" : 20010043,\
            \"gid-table\" : [ 20010043 ],\
            \"user-name\" : \"\" \
    },\
    \"permission\" : [\
    ],\
    \"internet-permission\" : {\
            \"set-allow-internet\" : 0,\
            \"allow-internet\" : 0\
    },\
    \"bundle-info\" : {\
            \"bundle-index\" : 0,\
            \"bundle-name\" : \"com.example.myapplication\" \
    },\
    \"owner-id\" : \"\",\
    \"render-cmd\" : \"1234567890\",\
    \"domain-info\" : {\
            \"hap-flags\" : 0,\
            \"apl\" : \"system_core\"\
    },\
    \"ext-info\" : [\
            {\
                    \"name\" : \"test\",\
                    \"value\" : \"4444444444444444444\" \
            } \
    ]\
}";

static const std::string defaultAppInfoNoBundleInfo = "{ \
    \"msg-type\": \"MSG_APP_SPAWN\", \
    \"msg-flags\": [ 13 ], \
    \"process-name\" : \"com.example.myapplication\", \
    \"dac-info\" : { \
            \"uid\" : 20010043, \
            \"gid\" : 20010043,\
            \"gid-table\" : [ 20010043 ],\
            \"user-name\" : \"\" \
    },\
    \"access-token\" : {\
            \"accessTokenIdEx\" : 537854093\
    },\
    \"permission\" : [\
    ],\
    \"internet-permission\" : {\
            \"set-allow-internet\" : 0,\
            \"allow-internet\" : 0\
    },\
    \"owner-id\" : \"\",\
    \"render-cmd\" : \"1234567890\",\
    \"domain-info\" : {\
            \"hap-flags\" : 0,\
            \"apl\" : \"system_core\"\
    },\
    \"ext-info\" : [\
            {\
                    \"name\" : \"test\",\
                    \"value\" : \"4444444444444444444\" \
            } \
    ]\
}";

static const std::string defaultWebInfo1 = "{ \
    \"msg-type\": \"MSG_APP_SPAWN\", \
    \"msg-flags\": [ 13 ], \
    \"process-name\" : \"com.example.myapplication\", \
    \"dac-info\" : { \
            \"uid\" : 1000001, \
            \"gid\" : 1000001,\
            \"gid-table\" : [1097, 1098],\
            \"user-name\" : \"\" \
    },\
    \"access-token\" : {\
            \"accessTokenIdEx\" : 537854093\
    },\
    \"permission\" : [\
    ],\
    \"internet-permission\" : {\
            \"set-allow-internet\" : 0,\
            \"allow-internet\" : 0\
    },\
    \"bundle-info\" : {\
            \"bundle-index\" : 0,\
            \"bundle-name\" : \"com.example.myapplication\" \
    },\
    \"owner-id\" : \"\",\
    \"render-cmd\" : \"1234567890\",\
    \"domain-info\" : {\
            \"hap-flags\" : 0,\
            \"apl\" : \"system_core\"\
    },\
    \"ext-info\" : [\
            {\
                    \"name\" : \"test\",\
                    \"value\" : \"4444444444444444444\" \
            } \
    ]\
}";

class AppSpawnModuleTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/*
    * Feature: AppSpawn
    * Function: Listen
    * SubFunction: Message listener
    * FunctionPoints: Process start message monitoring
    * EnvConditions: AppSpawn main process has started.
    *                 The socket server has been established.
    * CaseDescription: 1. Query the process of appspawn through the ps command
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_listen_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_listen_001 start");

    EXPECT_EQ(true, CheckAppspawnPID());

    HILOG_INFO(LOG_CORE, "AppSpawn_HF_listen_001 end");
}

/*
    * Feature: AppSpawn
    * Function: Listen
    * SubFunction: Message listener
    * FunctionPoints: Process start message monitoring.
    * EnvConditions: AppSpawn main process has started.
    *                The socket server has been established.
    * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_fork_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_fork_001 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_fork_001 end");
}

/*
    * Feature: AppSpawn
    * Function: Fork
    * SubFunction: fork process
    * FunctionPoints: Fork the process and run the App object.
    * EnvConditions: AppSpawn main process has started.
    *                The socket server has been established.
    * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
    *                  2. Send the message and the message format is correct, the message type is APP_TYPE_DEFAULT
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_fork_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_fork_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_fork_002 end");
}

/*
    * Feature: AppSpawn
    * Function: Fork
    * SubFunction: fork process
    * FunctionPoints: Fork the process and run the App object.
    * EnvConditions: AppSpawn main process has started.
    *                The socket server has been established.
    * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
    *                  2. Send the message and the message format is correct, the message type is APP_TYPE_NATIVE
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_fork_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_fork_003 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_fork_003 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_Native_Fork_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Native_Fork_001 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_Native_Fork_001 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_Native_Fork_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Native_Fork_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // for nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_Native_Fork_002 end");
}

/*
    * Feature: AppSpawn
    * Function: SetUid
    * SubFunction: Set child process permissions
    * FunctionPoints: Set the permissions of the child process to increase the priority of the new process
    * EnvConditions: AppSpawn main process has started.
    *                The socket server has been established.
    * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
    *                  2. Send the message and the message format is correct, the message type is APP_TYPE_DEFAULT
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setUid_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_001 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckUid(result.pid, 20010041));  // 20010041 test
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_001 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setUid_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckUid(result.pid, 20010043));  // 20010043 test

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_002 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setUid_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_003 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckUid(result.pid, 20010043));  // 20010043 test

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_003 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setUid_004, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_004 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckUid(result.pid, 1000001));  // 1000001 test

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_004 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setUid_005, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_005 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckUid(result.pid, 1000001));  // 1000001 test

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_005 end");
}

/*
    * Feature: AppSpawn
    * Function: CheckGid
    * SubFunction: Set child process permissions
    * FunctionPoints: Set the permissions of the child process to increase the priority of the new process
    * EnvConditions: AppSpawn main process has started.
    *                The socket server has been established.
    * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
    *                  2. Send the message and the message format is correct, the message type is APP_TYPE_DEFAULT
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGid_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_001 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckGid(result.pid, 20010043));  // 20010043 test gid

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_001 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGid_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);

    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckGid(result.pid, 20010041));  // 20010041 test gid

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_002 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGid_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_003 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);

    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckGid(result.pid, 20010041));  // 20010041 test gid

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_003 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGid_004, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_004 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // for nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);

    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckGid(result.pid, 1000001));  // 1000001 test gid

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_004 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGid_005, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_005 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // for nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);

    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(true, CheckGid(result.pid, 1000001));  // 1000001 test gid

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_005 end");
}

/*
    * Feature: AppSpawn
    * Function: CheckGids
    * SubFunction: Set child process permissions
    * FunctionPoints: Set the permissions of the child process to increase the priority of the new process
    * EnvConditions: AppSpawn main process has started.
    *                The socket server has been established.
    * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
    *                  2. Send the message and the message format is correct, the message type is APP_TYPE_DEFAULT
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGids_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_001 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    std::vector<int32_t> gids = {1008};  // 1008 gids
    EXPECT_EQ(true, CheckGids(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_001 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGids_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo3.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    std::vector<int32_t> gids = {20010045, 20010046};  // 20010045, 20010046 test gids
    EXPECT_EQ(true, CheckGids(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_002 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGids_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_003 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo3.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    std::vector<int32_t> gids = {20010045, 20010046};  // 20010045, 20010046 test gids
    EXPECT_EQ(true, CheckGids(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_003 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGids_004, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_004 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // for nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    std::vector<int32_t> gids = {1097, 1098};  // 1097, 1098 test gids
    EXPECT_EQ(true, CheckGids(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_004 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGids_005, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_005 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // for nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    std::vector<int32_t> gids = {1097, 1098};  // 1097, 1098 test gids
    EXPECT_EQ(true, CheckGids(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_003 end");
}

/*
    * Feature: AppSpawn
    * Function: setProcName
    * SubFunction: Set process name
    * FunctionPoints: Set process information .
    * EnvConditions: AppSpawn main process has started.
    *                The socket server has been established.
    * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
    *                  2. Send the message and the message format is correct, the message type is APP_TYPE_DEFAULT
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setProcName_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_001 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    // Check new app proc name
    EXPECT_EQ(true, CheckProcName(result.pid, "com.example.myapplication"));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_001 end");
}

/*
    * Feature: AppSpawn
    * Function: setProcName
    * SubFunction: Set process name
    * FunctionPoints: Set process information .
    * EnvConditions: AppSpawn main process has started.
    *                The socket server has been established.
    * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
    *                  2. Send the message and the message format is correct, the message type is APP_TYPE_NATIVE
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setProcName_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_002 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str());  // native start, can run
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_002 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setProcName_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_003 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    // do not check, native run fail
    // Failed to launch a native process with execvp: No such file or directory
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_003 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setProcName_004, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_004 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // for nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    // Check new app proc name
    EXPECT_EQ(true, CheckProcName(result.pid, "com.example.myapplication"));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_004 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setProcName_005, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_005 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // for nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultWebInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    // Check new app proc name
    EXPECT_EQ(true, CheckProcName(result.pid, "com.example.myapplication"));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_005 end");
}

/*
    * Feature: AppSpawn
    * Function: recycleProc
    * SubFunction: Recycling process
    * FunctionPoints: Recycling zombie processes.
    * EnvConditions: Start a js ability
    * CaseDescription: 1. Use the command kill to kill the process pid of the ability
    */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_recycleProc_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_001 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }
    // Check Process Is Destroyed
    EXPECT_EQ(true, CheckProcessIsDestroyed(result.pid));
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_001 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_recycleProc_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_002 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }
    // Check Process Is Destroyed
    EXPECT_EQ(true, CheckProcessIsDestroyed(result.pid));
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_002 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_recycleProc_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_003 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }
    // Check Process Is Destroyed
    EXPECT_EQ(true, CheckProcessIsDestroyed(result.pid));
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_003 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_recycleProc_006, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_006 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }

    // Check Process Is Destroyed
    EXPECT_EQ(true, CheckProcessIsDestroyed(result.pid));
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_006 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_recycleProc_007, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_007 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }

    // Check Process Is Destroyed
    EXPECT_EQ(true, CheckProcessIsDestroyed(result.pid));
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_007 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_recycleProc_008, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_008 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 62);
    printf("number: %d\n", dis(gen));
    AppFlagsIndex number = static_cast<AppFlagsIndex>(dis(gen));

    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnReqMsgSetAppFlag(reqHandle, number);
    ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }
    // Check Process Is Destroyed
    EXPECT_EQ(true, CheckProcessIsDestroyed(result.pid));
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_008 end");
}

/**
 * @brief
 *  no internet permission
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_001 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoInternetPermission.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_001 end");
}

/**
 * @brief
 *  no dac
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_001 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoDac.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_001 end");
}

/**
 * @brief
 *  no access token
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_003 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoToken.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);

    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_003 end");
}

/**
 * @brief
 *  no bundle info
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_004, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_004 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoBundleInfo.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_004 end");
}

/**
 * @brief
 *  no internet permission
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_005, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_005 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoInternetPermission.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_005 end");
}

/**
 * @brief
 *  no dac
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_006, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_006 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoDac.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_006 end");
}

/**
 * @brief
 *  no access token
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_007, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_007 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoToken.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);

    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_007 end");
}

/**
 * @brief
 *  no bundle info
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_008, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_008 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoBundleInfo.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_008 end");
}

/**
 * @brief
 *  no internet permission
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_009, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_009 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoInternetPermission.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_009 end");
}

/**
 * @brief
 *  no dac
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_010, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_010 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoDac.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_010 end");
}

/**
 * @brief
 *  no access token
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_011, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_011 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoToken.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);

    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_011 end");
}

/**
 * @brief
 *  no bundle info
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_Invalid_Msg_012, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_012 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfoNoBundleInfo.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(APPSPAWN_MSG_INVALID, result.result);
    HILOG_INFO(LOG_CORE, "AppSpawn_Invalid_Msg_012 end");
}

/**
 * @brief app flags
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_App_Flags_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_App_Flags_001 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 62);
    printf("number: %d\n", dis(gen));
    AppFlagsIndex number = static_cast<AppFlagsIndex>(dis(gen));
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    APPSPAWN_LOGI("number: %{public}d", number);
    int ret = AppSpawnReqMsgSetAppFlag(reqHandle, number);
    ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_App_Flags_001 end");
}

/**
 * @brief app flags
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_App_Flags_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_App_Flags_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 62);
    printf("number: %d\n", dis(gen));
    AppFlagsIndex number = static_cast<AppFlagsIndex>(dis(gen));
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    APPSPAWN_LOGI("number: %{public}d", number);
    int ret = AppSpawnReqMsgSetAppFlag(reqHandle, number);
    ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_App_Flags_002 end");
}

/**
 * @brief app flags
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_App_Flags_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_App_Flags_003 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 62);
    printf("number: %d\n", dis(gen));
    AppFlagsIndex number = static_cast<AppFlagsIndex>(dis(gen));
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    APPSPAWN_LOGI("number: %{public}d", number);
    int ret = AppSpawnReqMsgSetAppFlag(reqHandle, number);
    ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_App_Flags_003 end");
}

/**
 * @brief app flags
 *
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_App_Flags_004, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_App_Flags_004 start");
    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 62);
    printf("number: %d\n", dis(gen));
    AppFlagsIndex number = static_cast<AppFlagsIndex>(dis(gen));
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str(), MSG_SPAWN_NATIVE_PROCESS);
    APPSPAWN_LOGI("number: %{public}d", number);
    int ret = AppSpawnReqMsgSetAppFlag(reqHandle, number);
    ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_App_Flags_004 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_Msg_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Msg_001 start");

    static const std::string dumpInfo = "{ \
        \"msg-type\": \"MSG_DUMP\", \
        \"msg-flags\": [ 13 ], \
        \"process-name\" : \"com.example.myapplication\" \
    }";

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, dumpInfo.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    HILOG_INFO(LOG_CORE, "AppSpawn_Msg_001 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_Msg_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Msg_002 start");
    static const std::string appInfo = "{ \
        \"msg-type\": \"MSG_GET_RENDER_TERMINATION_STATUS\", \
        \"msg-flags\": [ 13 ], \
        \"process-name\" : \"com.example.myapplication\" \
    }";

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    commander.CreateMsg(reqHandle, appInfo.c_str(), MSG_GET_RENDER_TERMINATION_STATUS);
    ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_NE(0, result.result);
    HILOG_INFO(LOG_CORE, "AppSpawn_Msg_002 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_Msg_003, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Msg_003 start");
    static const std::string appInfo = "{ \
        \"msg-type\": \"MSG_GET_RENDER_TERMINATION_STATUS\", \
        \"msg-flags\": [ 13 ], \
        \"process-name\" : \"com.example.myapplication\" \
    }";

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    commander.CreateMsg(reqHandle, appInfo.c_str(), MSG_GET_RENDER_TERMINATION_STATUS);
    ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    HILOG_INFO(LOG_CORE, "AppSpawn_Msg_003 end");
}

HWTEST_F(AppSpawnModuleTest, AppSpawn_Msg_004, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_Msg_004 start");
    static const std::string appInfo = "{ \
        \"msg-type\": \"MSG_GET_RENDER_TERMINATION_STATUS\", \
        \"msg-flags\": [ 13 ], \
        \"process-name\" : \"com.example.myapplication\" \
    }";

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander(0);  // nwebspawn
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo1.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    commander.CreateMsg(reqHandle, appInfo.c_str(), MSG_GET_RENDER_TERMINATION_STATUS);
    ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_Msg_004 end");
}
}  // namespace AppSpawn
}  // namespace OHOS
