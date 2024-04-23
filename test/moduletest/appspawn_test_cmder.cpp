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

#include "appspawn_test_cmder.h"

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <termios.h>
#include <unistd.h>

#include <sys/stat.h>

#include "appspawn.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "cJSON.h"
#include "command_lexer.h"
#include "json_utils.h"
#include "securec.h"
#include "thread_manager.h"

#define MAX_THREAD 10
#define MAX_SEND 200
#define PTY_PATH_SIZE 128

namespace OHOS {
namespace AppSpawnModuleTest {
static const std::string g_defaultAppInfo = "{ \
    \"msg-type\": \"MSG_APP_SPAWN\", \
    \"msg-flags\": [1, 2 ], \
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
            \"ohos.permission.READ_IMAGEVIDEO\",\
            \"ohos.permission.FILE_CROSS_APP\",\
            \"ohos.permission.ACTIVATE_THEME_PACKAGE\"\
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

static const char *APPSPAWN_TEST_USAGE = "usage: AppSpawnTest <options> \n"
    "options list:\n"
    "  --help                   list available commands\n"
    "  --file xx                file path with app info\n"
    "  --thread xx              use multi-thread to send message\n"
    "  --type xx                send msg type \n"
    "  --pid xx                 render terminate pid\n"
    "  --mode nwebspawn         send message to nwebspawn service\n";

int AppSpawnTestCommander::ProcessArgs(int argc, char *const argv[])
{
    int sendMsg = 0;
    msgType_ = MAX_TYPE_INVALID;
    for (int32_t i = 0; i < argc; i++) {
        if (argv[i] == nullptr) {
            continue;
        }
        if (strcmp(argv[i], "--file") == 0 && ((i + 1) < argc)) {  // test file
            i++;
            testFileName_ = argv[i];
            sendMsg = 1;
        } else if (strcmp(argv[i], "--thread") == 0 && ((i + 1) < argc)) {  // use thread
            i++;
            threadCount_ = atoi(argv[i]);
            if (threadCount_ > MAX_THREAD) {
                threadCount_ = MAX_THREAD;
            }
            sendMsg = 1;
        } else if (strcmp(argv[i], "--mode") == 0 && ((i + 1) < argc)) {
            i++;
            appSpawn_ = strcmp(argv[i], "nwebspawn") == 0 ? 0 : 1;
            sendMsg = 1;
        } else if (strcmp(argv[i], "--type") == 0 && ((i + 1) < argc)) {
            i++;
            msgType_ = atoi(argv[i]);
            sendMsg = 1;
        } else if (strcmp(argv[i], "--pid") == 0 && ((i + 1) < argc)) {
            i++;
            msgType_ = MSG_GET_RENDER_TERMINATION_STATUS;
            terminatePid_ = atoi(argv[i]);
            sendMsg = 1;
        } else if (strcmp(argv[i], "--help") == 0) {
            printf("%s\n", APPSPAWN_TEST_USAGE);
            return 1;
        } else if (strcmp(argv[i], "--send") == 0 || strcmp(argv[i], "send") == 0) {
            sendMsg = 1;
        }
    }
    if (sendMsg == 0) {
        printf("%s\n", APPSPAWN_TEST_USAGE);
        return 1;
    }
    return 0;
}

uint32_t AppSpawnTestCommander::GetUint32ArrayFromJson(const cJSON *json,
    const char *name, uint32_t dataArray[], uint32_t maxCount)
{
    APPSPAWN_CHECK(json != NULL, return 0, "Invalid json");
    APPSPAWN_CHECK(name != NULL, return 0, "Invalid name");
    APPSPAWN_CHECK(dataArray != NULL, return 0, "Invalid dataArray");
    APPSPAWN_CHECK(cJSON_IsObject(json), return 0, "json is not object.");
    cJSON *array = cJSON_GetObjectItemCaseSensitive(json, name);
    APPSPAWN_CHECK_ONLY_EXPER(array != NULL, return 0);
    APPSPAWN_CHECK(cJSON_IsArray(array), return 0, "json is not object.");

    uint32_t count = 0;
    uint32_t arrayLen = cJSON_GetArraySize(array);
    for (int i = 0; i < arrayLen; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        uint32_t value = (uint32_t)cJSON_GetNumberValue(item);
        if (count < maxCount) {
            dataArray[count++] = value;
        }
    }
    return count;
}

int AppSpawnTestCommander::AddBundleInfoFromJson(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle)
{
    cJSON *config = cJSON_GetObjectItemCaseSensitive(appInfoConfig, "bundle-info");
    APPSPAWN_CHECK_ONLY_EXPER(config, return 0);

    uint32_t bundleIndex = GetIntValueFromJsonObj(config, "bundle-index", 0);
    char *bundleName = GetStringFromJsonObj(config, "bundle-name");
    int ret = AppSpawnReqMsgSetBundleInfo(reqHandle, bundleIndex, bundleName);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add bundle info req %{public}s", bundleName);
    return 0;
}

int AppSpawnTestCommander::AddDacInfoFromJson(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle)
{
    cJSON *config = cJSON_GetObjectItemCaseSensitive(appInfoConfig, "dac-info");
    APPSPAWN_CHECK_ONLY_EXPER(config, return 0);

    AppDacInfo info = {};
    info.uid = GetIntValueFromJsonObj(config, "uid", 0);
    info.gid = GetIntValueFromJsonObj(config, "gid", 0);
    info.gidCount = GetUint32ArrayFromJson(config, "gid-table", info.gidTable, APP_MAX_GIDS);
    char *userName = GetStringFromJsonObj(config, "user-name");
    if (userName != nullptr) {
        int ret = strcpy_s(info.userName, sizeof(info.userName), userName);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add userName info req %{public}s", userName);
    }
    return AppSpawnReqMsgSetAppDacInfo(reqHandle, &info);
}

int AppSpawnTestCommander::AddInternetPermissionInfoFromJson(
    const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle)
{
    cJSON *config = cJSON_GetObjectItemCaseSensitive(appInfoConfig, "internet-permission");
    APPSPAWN_CHECK_ONLY_EXPER(config, return 0);

    uint8_t setAllowInternet = GetIntValueFromJsonObj(config, "set-allow-internet", 0);
    uint8_t allowInternet = GetIntValueFromJsonObj(config, "allow-internet", 0);
    return AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, allowInternet, setAllowInternet);
}

int AppSpawnTestCommander::AddAccessTokenFromJson(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle)
{
    cJSON *config = cJSON_GetObjectItemCaseSensitive(appInfoConfig, "access-token");
    APPSPAWN_CHECK_ONLY_EXPER(config, return 0);

    uint64_t accessTokenIdEx = GetIntValueFromJsonObj(config, "accessTokenIdEx", 0);
    return AppSpawnReqMsgSetAppAccessToken(reqHandle, accessTokenIdEx);
}

int AppSpawnTestCommander::AddDomainInfoFromJson(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle)
{
    cJSON *config = cJSON_GetObjectItemCaseSensitive(appInfoConfig, "domain-info");
    APPSPAWN_CHECK_ONLY_EXPER(config, return 0);

    uint32_t hapFlags = GetIntValueFromJsonObj(config, "hap-flags", 0);
    char *apl = GetStringFromJsonObj(config, "apl");
    int ret = AppSpawnReqMsgSetAppDomainInfo(reqHandle, hapFlags, apl);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to domain info");
    return 0;
}

int AppSpawnTestCommander::AddExtTlv(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle)
{
    cJSON *configs = cJSON_GetObjectItemCaseSensitive(appInfoConfig, "ext-info");
    APPSPAWN_CHECK_ONLY_EXPER(configs != nullptr, return 0);

    int ret = 0;
    uint32_t count = cJSON_GetArraySize(configs);
    for (unsigned int j = 0; j < count; j++) {
        cJSON *config = cJSON_GetArrayItem(configs, j);

        char *name = GetStringFromJsonObj(config, "name");
        char *value = GetStringFromJsonObj(config, "value");
        APPSPAWN_LOGV("ext-info %{public}s %{public}s", name, value);
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, name, value);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add ext name %{public}s", name);
    }

    // 添加一个二进制的扩展元素
    AppDacInfo dacInfo{};
    dacInfo.uid = 101;          // 101 test data
    dacInfo.gid = 101;          // 101 test data
    dacInfo.gidTable[0] = 101;  // 101 test data
    dacInfo.gidCount = 1;
    (void)strcpy_s(dacInfo.userName, sizeof(dacInfo.userName), processName_.c_str());
    ret = AppSpawnReqMsgAddExtInfo(reqHandle,
        "app-dac-info", reinterpret_cast<uint8_t *>(&dacInfo), sizeof(dacInfo));
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add ext name app-info");
    return ret;
}

int AppSpawnTestCommander::BuildMsgFromJson(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle)
{
    int ret = AddBundleInfoFromJson(appInfoConfig, reqHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add dac %{public}s", processName_.c_str());

    ret = AddDomainInfoFromJson(appInfoConfig, reqHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add dac %{public}s", processName_.c_str());

    ret = AddDacInfoFromJson(appInfoConfig, reqHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add dac %{public}s", processName_.c_str());

    ret = AddAccessTokenFromJson(appInfoConfig, reqHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to add access token %{public}s", processName_.c_str());

    cJSON *obj = cJSON_GetObjectItemCaseSensitive(appInfoConfig, "permission");
    if (obj != nullptr && cJSON_IsArray(obj)) {
        int count = cJSON_GetArraySize(obj);
        for (int i = 0; i < count; i++) {
            char *value = cJSON_GetStringValue(cJSON_GetArrayItem(obj, i));
            APPSPAWN_LOGV("permission %{public}s ", value);
            ret = AppSpawnReqMsgAddPermission(reqHandle, value);
            APPSPAWN_CHECK(ret == 0, return ret, "Failed to permission %{public}s", value);
        }
    }

    ret = AddInternetPermissionInfoFromJson(appInfoConfig, reqHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to internet info %{public}s", processName_.c_str());

    std::string ownerId = GetStringFromJsonObj(appInfoConfig, "owner-id");
    if (!ownerId.empty()) {
        ret = AppSpawnReqMsgSetAppOwnerId(reqHandle, ownerId.c_str());
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to ownerid %{public}s", processName_.c_str());
    }

    std::string renderCmd = GetStringFromJsonObj(appInfoConfig, "render-cmd");
    if (!renderCmd.empty()) {
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD, renderCmd.c_str());
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to add renderCmd %{public}s", renderCmd.c_str());
    }
    return AddExtTlv(appInfoConfig, reqHandle);
}

int AppSpawnTestCommander::CreateOtherMsg(AppSpawnReqMsgHandle &reqHandle, pid_t pid)
{
    if (msgType_ == MSG_GET_RENDER_TERMINATION_STATUS) {
        int ret = AppSpawnTerminateMsgCreate(pid, &reqHandle);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to termination message req %{public}s", processName_.c_str());
    }
    if (msgType_ == MSG_DUMP) {
        int ret = AppSpawnReqMsgCreate(static_cast<AppSpawnMsgType>(msgType_), processName_.c_str(), &reqHandle);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to dump req %{public}s", processName_.c_str());
        ret = AppSpawnReqMsgAddStringInfo(reqHandle, "pty-name", ptyName_.c_str());
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to add ptyName_ %{public}s", ptyName_.c_str());
    }
    return 0;
}

static uint32_t GetMsgTypeFromJson(const cJSON *json)
{
    const char *msgType = GetStringFromJsonObj(json, "msg-type");
    if (msgType == nullptr) {
        return MSG_APP_SPAWN;
    }
    if (strcmp(msgType, "MSG_SPAWN_NATIVE_PROCESS") == 0) {
        return MSG_SPAWN_NATIVE_PROCESS;
    }
    if (strcmp(msgType, "MSG_GET_RENDER_TERMINATION_STATUS") == 0) {
        return MSG_GET_RENDER_TERMINATION_STATUS;
    }
    if (strcmp(msgType, "MSG_DUMP") == 0) {
        return MSG_DUMP;
    }
    return MSG_APP_SPAWN;
}

int AppSpawnTestCommander::CreateMsg(AppSpawnReqMsgHandle &reqHandle,
    const char *defaultConfig, uint32_t defMsgType)
{
    int ret = APPSPAWN_SYSTEM_ERROR;
    if (clientHandle_ == NULL) {
        ret = AppSpawnClientInit(appSpawn_ ? APPSPAWN_SERVER_NAME : NWEBSPAWN_SERVER_NAME, &clientHandle_);
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to create client %{public}d", appSpawn_);
    }
    reqHandle = INVALID_REQ_HANDLE;
    if (appInfoConfig_) {
        cJSON_Delete(appInfoConfig_);
        appInfoConfig_ = nullptr;
    }
    if (!testFileName_.empty()) {
        appInfoConfig_ = GetJsonObjFromFile(testFileName_.c_str());
        if (appInfoConfig_ == nullptr) {
            printf("Failed to load file %s, so use default info \n", testFileName_.c_str());
        }
    }
    if (appInfoConfig_ == nullptr) {
        appInfoConfig_ = cJSON_Parse(defaultConfig);
    }
    if (appInfoConfig_ == nullptr) {
        printf("Invalid app info \n");
        return APPSPAWN_SYSTEM_ERROR;
    }
    processName_ = GetStringFromJsonObj(appInfoConfig_, "process-name");
    if (processName_.empty()) {
        processName_ = "com.example.myapplication";
    }
    msgType_ = (msgType_ == MAX_TYPE_INVALID) ? GetMsgTypeFromJson(appInfoConfig_) : msgType_;
    msgType_ = (defMsgType != MAX_TYPE_INVALID) ? defMsgType : msgType_;
    if (msgType_ == MSG_DUMP) {
        return CreateOtherMsg(reqHandle, 0);
    } else if (msgType_ == MSG_GET_RENDER_TERMINATION_STATUS) {
        pid_t pid = GetIntValueFromJsonObj(appInfoConfig_, "pid", 0);
        return CreateOtherMsg(reqHandle, pid);
    }
    ret = AppSpawnReqMsgCreate(static_cast<AppSpawnMsgType>(msgType_), processName_.c_str(), &reqHandle);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to create req %{public}s", processName_.c_str());

    uint32_t msgFlags[64] = {};  // 64
    uint32_t count = GetUint32ArrayFromJson(appInfoConfig_, "msg-flags", msgFlags, ARRAY_LENGTH(msgFlags));
    for (uint32_t j = 0; j < count; j++) {
        (void)AppSpawnReqMsgSetAppFlag(reqHandle, static_cast<AppFlagsIndex>(msgFlags[j]));
    }
    (void)AppSpawnReqMsgSetAppFlag(reqHandle, APP_FLAGS_IGNORE_SANDBOX);
    ret = BuildMsgFromJson(appInfoConfig_, reqHandle);
    APPSPAWN_CHECK(ret == 0, AppSpawnReqMsgFree(reqHandle);
        return ret, "Failed to build req %{public}s", processName_.c_str());
    return ret;
}

int AppSpawnTestCommander::SendMsg()
{
    const char *server = appSpawn_ ? APPSPAWN_SERVER_NAME : NWEBSPAWN_SERVER_NAME;
    printf("Send msg to server '%s' \n", server);
    AppSpawnReqMsgHandle reqHandle = INVALID_REQ_HANDLE;
    int ret = 0;
    if (msgType_ == MSG_DUMP) {
        while (!dumpFlags) {
            usleep(20000);  // 20000
        }
        ret = CreateOtherMsg(reqHandle, 0);
    } else if (msgType_ == MSG_GET_RENDER_TERMINATION_STATUS) {
        ret = CreateOtherMsg(reqHandle, terminatePid_);
    } else {
        ret = CreateMsg(reqHandle, g_defaultAppInfo.c_str());
    }
    AppSpawnResult result = {ret, 0};
    if (ret == 0) {
        ret = AppSpawnClientSendMsg(clientHandle_, reqHandle, &result);
    }
    switch (msgType_) {
        case MSG_APP_SPAWN:
            if (result.result == 0) {
                printf("Spawn app %s success, pid %d \n", processName_.c_str(), result.pid);
            } else {
                printf("Spawn app %s fail, result 0x%x \n", processName_.c_str(), result.result);
            }
            break;
        case MSG_SPAWN_NATIVE_PROCESS:
            if (result.result == 0) {
                printf("Spawn native app %s success, pid %d \n", processName_.c_str(), result.pid);
            } else {
                printf("Spawn native app %s fail, result 0x%x \n", processName_.c_str(), result.result);
            }
            break;
        case MSG_GET_RENDER_TERMINATION_STATUS:
            printf("Terminate app %s success, pid %d status 0x%x \n",
                processName_.c_str(), result.pid, result.result);
            break;
        default:
            printf("Dump server %s result %d \n", server, ret);
            break;
    }
    msgType_ = MAX_TYPE_INVALID;
    terminatePid_ = 0;
    printf("Please input cmd: \n");
    return 0;
}

int AppSpawnTestCommander::StartSendMsg()
{
    int ret = 0;
    printf("Start send msg thread count %d file name %s \n", threadCount_, testFileName_.c_str());
    if (threadCount_ == 1) {
        SendMsg();
    } else {
        ThreadTaskHandle taskHandle = 0;
        ret = ThreadMgrAddTask(threadMgr_, &taskHandle);
        APPSPAWN_CHECK(ret == 0, return 0, "Failed to add task ");
        for (uint32_t index = 0; index < threadCount_; index++) {
            ThreadMgrAddExecutor(threadMgr_, taskHandle, TaskExecutorProc, reinterpret_cast<ThreadContext *>(this));
        }
        TaskSyncExecute(threadMgr_, taskHandle);
    }
    return 0;
}

void AppSpawnTestCommander::TaskExecutorProc(ThreadTaskHandle handle, const ThreadContext *context)
{
    AppSpawnTestCommander *testCmder = AppSpawnTestCommander::ConvertTo(context);
    testCmder->SendMsg();
}

void AppSpawnTestCommander::SendTaskFinish(ThreadTaskHandle handle, const ThreadContext *context)
{
    APPSPAWN_LOGV("SendTaskFinish %{public}u \n", handle);
}

static std::vector<std::string> g_args;
static int HandleSplitString(const char *str, void *context)
{
    APPSPAWN_LOGV("HandleSplitString %{public}s ", str);
    std::string value = str;
    g_args.push_back(value);
    return 0;
}

int AppSpawnTestCommander::ProcessInputCmd(std::string &cmd)
{
    g_args.clear();
    int ret = StringSplit(cmd.c_str(), " ", nullptr, HandleSplitString);
    std::vector<char *> options;
    for (const auto &arg : g_args) {
        if (!arg.empty()) {
            options.push_back(const_cast<char *>(arg.c_str()));
        }
    }
    (void)ProcessArgs(options.size(), options.data());
    StartSendMsg();
    return ret;
}

void AppSpawnTestCommander::InputThread(ThreadTaskHandle handle, const ThreadContext *context)
{
    AppSpawnTestCommander *testCmder = AppSpawnTestCommander::ConvertTo(context);
    char buffer[1024] = {0};  // 1024 test buffer max len
    fd_set fds;
    printf("Please input cmd: \n");
    while (1) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        int ret = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, nullptr);
        if (ret <= 0) {
            if (testCmder->exit_) {
                break;
            }
            continue;
        }
        ssize_t rlen = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
        if (rlen <= 1) {
            continue;
        }
        buffer[rlen - 1] = 0;
        printf("Recv command: '%s' \n", buffer);
        if (strncmp("quit", buffer, strlen("quit")) == 0) {
            testCmder->exit_ = 1;
            break;
        }
        if (strncmp("send", buffer, 4) == 0) {  // 4 strlen("send")
            std::string cmd(buffer);
            testCmder->ProcessInputCmd(cmd);
            printf("Please input cmd: \n");
        }
    }
}

void AppSpawnTestCommander::DumpThread(ThreadTaskHandle handle, const ThreadContext *context)
{
    AppSpawnTestCommander *testCmder = AppSpawnTestCommander::ConvertTo(context);
    printf("Start dump thread \n");
    char buffer[10240] = {0};  // 1024 test buffer max len
    fd_set fds;
    while (1) {
        testCmder->dumpFlags = 1;
        FD_ZERO(&fds);
        FD_SET(testCmder->ptyFd_, &fds);
        int ret = select(testCmder->ptyFd_ + 1, &fds, nullptr, nullptr, nullptr);
        if (ret <= 0) {
            if (testCmder->exit_) {
                break;
            }
            continue;
        }
        if (!FD_ISSET(testCmder->ptyFd_, &fds)) {
            continue;
        }
        ssize_t rlen = read(testCmder->ptyFd_, buffer, sizeof(buffer) - 1);
        while (rlen > 0) {
            buffer[rlen] = '\0';
            printf("%s", buffer);
            fflush(stdout);
            rlen = read(testCmder->ptyFd_, buffer, sizeof(buffer) - 1);
        }
    }
}

int AppSpawnTestCommander::Run()
{
    int ret = 0;
    const char *name = appSpawn_ ? APPSPAWN_SERVER_NAME : NWEBSPAWN_SERVER_NAME;
    if (clientHandle_ == NULL) {
        ret = AppSpawnClientInit(name, &clientHandle_);
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to create client %{public}s", name);
    }

    InitPtyInterface();

    ret = CreateThreadMgr(5, &threadMgr_);  // 5 max thread
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to create thread manager");

    ret = ThreadMgrAddTask(threadMgr_, &inputHandle_);
    APPSPAWN_CHECK(ret == 0, return 0, "Failed to add task for thread ");
    ThreadMgrAddExecutor(threadMgr_, inputHandle_, InputThread, this);
    TaskExecute(threadMgr_, inputHandle_, SendTaskFinish, this);

    ret = ThreadMgrAddTask(threadMgr_, &dumpHandle_);
    APPSPAWN_CHECK(ret == 0, return 0, "Failed to add task for thread ");
    ThreadMgrAddExecutor(threadMgr_, dumpHandle_, DumpThread, this);
    TaskExecute(threadMgr_, dumpHandle_, SendTaskFinish, this);

    StartSendMsg();

    APPSPAWN_LOGV("Finish send msg \n");
    while (!exit_) {
        usleep(200000);  // 200000 200ms
    }
    ThreadMgrCancelTask(threadMgr_, inputHandle_);
    ThreadMgrCancelTask(threadMgr_, dumpHandle_);
    DestroyThreadMgr(threadMgr_);
    threadMgr_ = nullptr;
    inputHandle_ = 0;
    dumpHandle_ = 0;
    AppSpawnClientDestroy(clientHandle_);
    clientHandle_ = nullptr;
    return 0;
}

int AppSpawnTestCommander::InitPtyInterface()
{
    // open master pty and get slave pty
    int pfd = open("/dev/ptmx", O_RDWR | O_CLOEXEC | O_NOCTTY | O_NONBLOCK);
    APPSPAWN_CHECK(pfd >= 0, return -1, "Failed open pty err=%d", errno);
    APPSPAWN_CHECK(grantpt(pfd) >= 0, close(pfd); return -1, "Failed to call grantpt");
    APPSPAWN_CHECK(unlockpt(pfd) >= 0, close(pfd); return -1, "Failed to call unlockpt");
    char ptsbuffer[PTY_PATH_SIZE] = {0};
    int ret = ptsname_r(pfd, ptsbuffer, sizeof(ptsbuffer));
    APPSPAWN_CHECK(ret >= 0, close(pfd);
        return -1, "Failed to get pts name err=%d", errno);
    APPSPAWN_LOGI("ptsbuffer is %s", ptsbuffer);
    APPSPAWN_CHECK(chmod(ptsbuffer, S_IRWXU | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) == 0, close(pfd);
        return -1, "Failed to chmod %s, err=%d", ptsbuffer, errno);
    ptyFd_ = pfd;
    ptyName_ = std::string(ptsbuffer);
    return 0;
}
}  // namespace AppSpawnModuleTest
}  // namespace OHOS
