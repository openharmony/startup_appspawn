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

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_msg.h"

#include "appspawn_utils.h"
#include "hilog/log.h"
#include "securec.h"

#include "appspawn_test_cmder.h"
#include <gtest/gtest.h>

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace AppSpawn {
#undef LOG_DOMAIN
#define LOG_DOMAIN 0xD002C11
#undef LOG_TAG
#define LOG_TAG "AppSpawnMST"

namespace {
    const bool CHECK_OK = true;
    const bool CHECK_ERROR = false;
    const int32_t DEFAULT_PID = 0;
    const int32_t FILE_PATH_SIZE = 50;
    const int32_t CMD_SIZE = 50;
    const int32_t BUFFER_SIZE = 512;
    const int32_t BASE_TYPE = 10;
    const int32_t CONNECT_RETRY_DELAY = 50 * 1000;
    const int32_t CONNECT_RETRY_MAX_TIMES = 5;
    const int32_t UID_POSITION_MOVE = 5;
    const int32_t GID_POSITION_MOVE = 5;
    const int32_t GROUPS_POSITION_MOVE = 8;
    const char *DELIMITER_SPACE = " ";
    const char *DELIMITER_NEWLINE = "\n";

    char buffer[BUFFER_SIZE] = {"\0"};
    int32_t retryCount = 0;
}  // namespace

bool CheckFileIsExists(const char *filepath)
{
    retryCount = 0;
    while ((access(filepath, F_OK) != 0) && (retryCount < CONNECT_RETRY_MAX_TIMES)) {
        usleep(CONNECT_RETRY_DELAY);
        retryCount++;
    }
    GTEST_LOG_(INFO) << "retryCount :" << retryCount << ".";
    if (retryCount < CONNECT_RETRY_MAX_TIMES) {
        return CHECK_OK;
    }
    return CHECK_ERROR;
}

bool ReadFileInfo(char *buffer, const int32_t &pid, const char *fileName)
{
    // Set file path
    char filePath[FILE_PATH_SIZE];
    if (sprintf_s(filePath, sizeof(filePath), "/proc/%d/%s", pid, fileName) <= 0) {
        HILOG_ERROR(LOG_CORE, "filePath sprintf_s fail .");
        return CHECK_ERROR;
    }
    if (!CheckFileIsExists(filePath)) {
        HILOG_ERROR(LOG_CORE, "file %{public}s is not exists .", fileName);
        return CHECK_ERROR;
    }
    // Open file
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) {
        HILOG_ERROR(LOG_CORE, "file %{public}s open failed . error:%{publid}s", fileName, strerror(errno));
        return CHECK_ERROR;
    }
    // Read file
    int t = read(fd, buffer, BUFFER_SIZE);
    if (t <= 0 || buffer == nullptr) {
        HILOG_INFO(LOG_CORE, "read proc status file failed.");
        close(fd);
        return CHECK_ERROR;
    }
    HILOG_INFO(LOG_CORE, "buffer:\n %{public}s", buffer);
    close(fd);

    return CHECK_OK;
}

bool CheckUid(const int32_t &pid, const int newUid)
{
    if (ReadFileInfo(buffer, pid, "status")) {
        // Move to Uid position
        char *uidPtr = strstr(buffer, "Uid:");
        if (uidPtr == nullptr) {
            HILOG_ERROR(LOG_CORE, "get Uid info failed.");
            return CHECK_ERROR;
        }
        if (strlen(uidPtr) > UID_POSITION_MOVE) {
            uidPtr = uidPtr + UID_POSITION_MOVE;
        }
        int32_t uid = static_cast<int32_t>(strtol(uidPtr, NULL, BASE_TYPE));
        HILOG_INFO(LOG_CORE, "new proc(%{public}d) uid = %{public}d, setUid=%{public}d.", pid, uid, newUid);
        if (uid == newUid) {
            return CHECK_OK;
        }
    }
    return CHECK_ERROR;
}

bool CheckGid(const int32_t &pid, const int newGid)
{
    if (ReadFileInfo(buffer, pid, "status")) {
        // Move to Gid position
        char *gidPtr = strstr(buffer, "Gid:");
        if (gidPtr == nullptr) {
            HILOG_ERROR(LOG_CORE, "get Gid info failed.");
            return CHECK_ERROR;
        }
        if (strlen(gidPtr) > GID_POSITION_MOVE) {
            gidPtr = gidPtr + GID_POSITION_MOVE;
        }
        if (gidPtr == nullptr) {
            HILOG_ERROR(LOG_CORE, "get Gid info failed.");
            return CHECK_ERROR;
        }
        int32_t gid = static_cast<int32_t>(strtol(gidPtr, NULL, BASE_TYPE));
        HILOG_INFO(LOG_CORE, "new proc(%{public}d) gid = %{public}d, setGid=%{public}d.", pid, gid, newGid);
        if (gid == newGid) {
            return CHECK_OK;
        }
    }
    return CHECK_ERROR;
}

std::size_t GetGids(const int32_t &pid, std::vector<int32_t> &gids)
{
    if (ReadFileInfo(buffer, pid, "status")) {
        // Move to Groups position
        char *groupsPtr = strstr(buffer, "Groups");
        if (groupsPtr == nullptr || strlen(groupsPtr) > BUFFER_SIZE) {
            HILOG_ERROR(LOG_CORE, "get Groups info failed.");
            return CHECK_ERROR;
        }
        if (strlen(groupsPtr) > GROUPS_POSITION_MOVE) {
            groupsPtr = groupsPtr + GROUPS_POSITION_MOVE;
        }
        // Get the row content of Groups
        char *saveptr = nullptr;
        if (groupsPtr == nullptr || strlen(groupsPtr) > BUFFER_SIZE) {
            HILOG_ERROR(LOG_CORE, "get Groups info failed.");
            return CHECK_ERROR;
        }
        char *line = strtok_r(groupsPtr, DELIMITER_NEWLINE, &saveptr);
        if (line == nullptr || strlen(line) > BUFFER_SIZE) {
            HILOG_ERROR(LOG_CORE, "get Groups line info failed.");
            return CHECK_ERROR;
        }
        // Get each gid and insert into vector
        char *gid = strtok_r(line, DELIMITER_SPACE, &saveptr);
        while (gid != nullptr) {
            gids.push_back(atoi(gid));
            gid = strtok_r(nullptr, DELIMITER_SPACE, &saveptr);
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
        return CHECK_OK;
    }

    return CHECK_ERROR;
}

bool CheckGidsCount(const int32_t &pid, const std::vector<int32_t> newGids)
{
    // Get GidsCount
    std::vector<int32_t> gids;
    std::size_t gCount = GetGids(pid, gids);
    if (gCount == newGids.size()) {
        return CHECK_OK;
    }

    return CHECK_ERROR;
}

bool CheckProcName(const int32_t &pid, const std::string &newProcessName)
{
    FILE *fp = nullptr;
    char cmd[CMD_SIZE];
    if (sprintf_s(cmd, sizeof(cmd), "ps -o ARGS=CMD -p %d |grep -v CMD", pid) <= 0) {
        HILOG_ERROR(LOG_CORE, "cmd sprintf_s fail .");
        return CHECK_ERROR;
    }
    if (strlen(cmd) > CMD_SIZE) {
        HILOG_ERROR(LOG_CORE, " cmd length is too long  .");
        return CHECK_ERROR;
    }
    fp = popen(cmd, "r");
    if (fp == nullptr) {
        HILOG_ERROR(LOG_CORE, " popen function call failed .");
        return CHECK_ERROR;
    }
    char procName[BUFFER_SIZE];
    if (fgets(procName, sizeof(procName), fp) != nullptr) {
        for (unsigned int i = 0; i < sizeof(procName); i++) {
            if (procName[i] == '\n') {
                procName[i] = '\0';
                break;
            }
        }
        GTEST_LOG_(INFO) << "strcmp" << " :" << strcmp(newProcessName.c_str(), procName) << ".";

        if (newProcessName.compare(0, newProcessName.size(), procName, newProcessName.size()) == 0) {
            pclose(fp);
            return CHECK_OK;
        }
        HILOG_ERROR(LOG_CORE, " procName=%{public}s, newProcessName=%{public}s.", procName, newProcessName.c_str());
    } else {
        HILOG_ERROR(LOG_CORE, "Getting procName failed.");
    }
    pclose(fp);

    return CHECK_ERROR;
}

bool CheckProcessIsDestroyed(const int32_t &pid)
{
    char filePath[FILE_PATH_SIZE];
    if (sprintf_s(filePath, sizeof(filePath), "/proc/%d", pid) <= 0) {
        HILOG_ERROR(LOG_CORE, "filePath sprintf_s fail .");
        return CHECK_ERROR;
    }

    if (CheckFileIsExists(filePath)) {
        HILOG_ERROR(LOG_CORE, "File %{public}d is not exists .", pid);
        return CHECK_ERROR;
    }

    return CHECK_OK;
}

bool CheckAppspawnPID()
{
    FILE *fp = nullptr;
    fp = popen("pidof appspawn", "r");
    if (fp == nullptr) {
        HILOG_ERROR(LOG_CORE, " popen function call failed.");
        return CHECK_ERROR;
    }
    char pid[BUFFER_SIZE];
    if (fgets(pid, sizeof(pid), fp) != nullptr) {
        pclose(fp);
        return CHECK_OK;
    }

    HILOG_ERROR(LOG_CORE, "Getting Pid failed.");

    pclose(fp);

    return CHECK_ERROR;
}

bool startAppspawn()
{
    FILE *fp = nullptr;
    fp = popen("/system/bin/appspawn&", "r");
    if (fp == nullptr) {
        HILOG_ERROR(LOG_CORE, " popen function call failed.");
        return CHECK_ERROR;
    }

    pclose(fp);

    return CHECK_OK;
}

bool stopAppspawn()
{
    FILE *fp = nullptr;
    fp = popen("kill -9 $(pidof appspawn)", "r");
    if (fp == nullptr) {
        HILOG_ERROR(LOG_CORE, " popen function call failed.");
        return CHECK_ERROR;
    }

    pclose(fp);

    return CHECK_OK;
}

class AppSpawnModuleTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();

    const std::string defaultAppInfo1 = "{ \
        \"msg-type\": 0, \
        \"msg-flags\": [ 3 ], \
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

    const std::string defaultAppInfo2 = "{ \
        \"msg-type\": 0, \
        \"msg-flags\": [ 3 ], \
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

    const std::string defaultAppInfo3 = "{ \
        \"msg-type\": 1, \
        \"msg-flags\": [ 3 ], \
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

    const std::string defaultAppInfo4 = "{ \
        \"msg-type\": 1, \
        \"msg-flags\": [ 3 ], \
        \"process-name\" : \"com.example.myapplication\", \
        \"dac-info\" : { \
                \"uid\" : 20010043, \
                \"gid\" : 20010043,\
                \"gid-table\" : [ 20010043, 20010044 ],\
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

    const std::string defaultAppInfo5 = "{ \
        \"msg-type\": 1, \
        \"msg-flags\": [ 3 ], \
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
};

void AppSpawnModuleTest::SetUpTestCase()
{
    if (!CheckAppspawnPID()) {
        EXPECT_EQ(startAppspawn(), CHECK_OK);
    }
}

void AppSpawnModuleTest::TearDownTestCase()
{
    if (CheckAppspawnPID()) {
        EXPECT_EQ(stopAppspawn(), CHECK_OK);
    }
}

void AppSpawnModuleTest::SetUp()
{
    auto ret = memset_s(buffer, sizeof(buffer), 0x00, BUFFER_SIZE);
    if (ret != EOK) {
        HILOG_ERROR(LOG_CORE, "memset_s is failed.");
    }
}

void AppSpawnModuleTest::TearDown() {}

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

    EXPECT_EQ(CHECK_OK, CheckAppspawnPID());

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
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_listen_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_listen_002 start");


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
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_listen_002 end");
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
 *                  2. Send the message and the message format is correct, the message type is APP_TYPE_NATIVE
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_fork_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_fork_002 start");


    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo3.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_fork_002 end");
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
    EXPECT_EQ(CHECK_OK, CheckUid(result.pid, 20010043));
    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_001 end");
}

/*
 * Feature: AppSpawn
 * Function: SetUid
 * SubFunction: Set child process permissions
 * FunctionPoints: Set the permissions of the child process to increase the priority of the new process
 * EnvConditions: AppSpawn main process has started.
 *                The socket server has been established.
 * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
 *                  2. Send the message and the message format is correct, the message type is APP_TYPE_NATIVE
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_setUid_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo3.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(CHECK_OK, CheckGid(result.pid, 20010043));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_002 end");
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
    commander.CreateMsg(reqHandle, defaultAppInfo4.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(CHECK_OK, CheckGid(result.pid, 20010043));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_001 end");
}

/*
 * Feature: AppSpawn
 * Function: CheckGid
 * SubFunction: Set child process permissions
 * FunctionPoints: Set the permissions of the child process to increase the priority of the new process
 * EnvConditions: AppSpawn main process has started.
 *                The socket server has been established.
 * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
 *                  2. Send the message and the message format is correct, the message type is APP_TYPE_NATIVE
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGid_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);

    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    EXPECT_EQ(CHECK_OK, CheckGid(result.pid, 20010043));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGid_002 end");
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
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    std::vector<int32_t> gids = {20010043};
    EXPECT_EQ(CHECK_OK, CheckGids(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setUid_001 end");
}

/*
 * Feature: AppSpawn
 * Function: CheckGids
 * SubFunction: Set child process permissions
 * FunctionPoints: Set the permissions of the child process to increase the priority of the new process
 * EnvConditions: AppSpawn main process has started.
 *                The socket server has been established.
 * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
 *                  2. Send the message and the message format is correct, the message type is APP_TYPE_NATIVE
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGids_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo5.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    std::vector<int32_t> gids = { 20010043 };
    EXPECT_EQ(CHECK_OK, CheckGids(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGids_002 end");
}

/*
 * Feature: AppSpawn
 * Function: CheckGidsCount
 * SubFunction: Set child process permissions
 * FunctionPoints: Set the permissions of the child process to increase the priority of the new process
 * EnvConditions: AppSpawn main process has started.
 *                The socket server has been established.
 * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
 *                  2. Send the message and the message format is correct, the message type is APP_TYPE_DEFAULT
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGidsCount_001, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGidsCount_001 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo2.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    std::vector<int32_t> gids = {20010043 };
    EXPECT_EQ(CHECK_OK, CheckGidsCount(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGidsCount_001 end");
}

/*
 * Feature: AppSpawn
 * Function: CheckGidsCount
 * SubFunction: Set child process permissions
 * FunctionPoints: Set the permissions of the child process to increase the priority of the new process
 * EnvConditions: AppSpawn main process has started.
 *                The socket server has been established.
 * CaseDescription: 1. Establish a socket client and connect with the Appspawn server
 *                  2. Send the message and the message format is correct, the message type is APP_TYPE_NATIVE
 */
HWTEST_F(AppSpawnModuleTest, AppSpawn_HF_checkGidsCount_002, TestSize.Level0)
{
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGidsCount_002 start");

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    AppSpawnReqMsgHandle reqHandle;
    AppSpawnResult result;
    commander.CreateMsg(reqHandle, defaultAppInfo5.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    std::vector<int32_t> gids = {20010043};
    EXPECT_EQ(CHECK_OK, CheckGidsCount(result.pid, gids));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_checkGidsCount_002 end");
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
    EXPECT_EQ(CHECK_OK, CheckProcName(result.pid, "com.example.myapplication"));

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
    commander.CreateMsg(reqHandle, defaultAppInfo4.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";
    // Check new app proc name
    EXPECT_EQ(CHECK_OK, CheckProcName(result.pid, "com.example.myapplication"));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_setProcName_002 end");
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
    commander.CreateMsg(reqHandle, defaultAppInfo4.c_str());
    int ret = AppSpawnClientSendMsg(commander.GetClientHandle(), reqHandle, &result);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, result.result);
    EXPECT_NE(0, result.pid);
    GTEST_LOG_(INFO) << "newPid :" << result.pid << ".";

    EXPECT_EQ(0, kill(result.pid, SIGKILL));
    result.pid = DEFAULT_PID;

    // Check Process Is Destroyed
    EXPECT_EQ(CHECK_OK, CheckProcessIsDestroyed(result.pid));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_001 end");
}

/*
 * Feature: AppSpawn
 * Function: recycleProc
 * SubFunction: Recycling process
 * FunctionPoints: Recycling zombie processes
 * EnvConditions: Start a native ability .
 * CaseDescription: 1. Use the command kill to kill the process pid of the ability
 */
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

    EXPECT_EQ(0, kill(result.pid, SIGKILL));
    result.pid = DEFAULT_PID;
    // Check Process Is Destroyed
    EXPECT_EQ(CHECK_OK, CheckProcessIsDestroyed(result.pid));

    if (result.pid > 0) {
        EXPECT_EQ(0, kill(result.pid, SIGKILL));
        result.pid = DEFAULT_PID;
    }
    HILOG_INFO(LOG_CORE, "AppSpawn_HF_recycleProc_002 end");
}
}  // namespace AppSpawn
}  // namespace OHOS
