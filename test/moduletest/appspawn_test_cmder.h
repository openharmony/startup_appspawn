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
#ifndef APPSPAWN_TEST_CMD_H
#define APPSPAWN_TEST_CMD_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "appspawn.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "cJSON.h"
#include "securec.h"
#include "thread_manager.h"

typedef struct TagThreadContext {
} ThreadContext;

__attribute__((always_inline)) inline char *GetStringFromJsonObj(const cJSON *json, const char *key)
{
    APPSPAWN_CHECK_ONLY_EXPER(key != NULL && json != NULL, NULL);
    APPSPAWN_CHECK(cJSON_IsObject(json), return NULL, "json is not object %{public}s %s", key, cJSON_Print(json));
    cJSON *obj = cJSON_GetObjectItemCaseSensitive(json, key);
    APPSPAWN_CHECK_ONLY_EXPER(obj != NULL, return NULL);
    APPSPAWN_CHECK(cJSON_IsString(obj), return NULL, "json is not string %{public}s %s", key, cJSON_Print(obj));
    return cJSON_GetStringValue(obj);
}

__attribute__((always_inline)) inline bool GetBoolValueFromJsonObj(const cJSON *json, const char *key, bool def)
{
    char *value = GetStringFromJsonObj(json, key);
    APPSPAWN_CHECK_ONLY_EXPER(value != NULL, return def);

    if (strcmp(value, "true") == 0 || strcmp(value, "ON") == 0 || strcmp(value, "True") == 0) {
        return true;
    }
    return false;
}

__attribute__((always_inline)) inline uint32_t GetIntValueFromJsonObj(const cJSON *json, const char *key, uint32_t def)
{
    APPSPAWN_CHECK(json != NULL, return def, "Invalid json");
    APPSPAWN_CHECK(cJSON_IsObject(json), return def, "json is not object.");
    return cJSON_GetNumberValue(cJSON_GetObjectItemCaseSensitive(json, key));
}

namespace OHOS {
namespace AppSpawnModuleTest {
class AppSpawnTestCommander : public ThreadContext {
public:
    AppSpawnTestCommander()
    {
        exit_ = 0;
        appSpawn_ = 1;
        dumpFlags = 0;
        msgType_ = 0;
    }
    ~AppSpawnTestCommander()
    {
        if (ptyFd_ != -1) {
            (void)close(ptyFd_);
            ptyFd_ = -1;
        }
        if (appInfoConfig_) {
            cJSON_Delete(appInfoConfig_);
            appInfoConfig_ = nullptr;
        }
    }

    int ProcessArgs(int argc, char *const argv[]);
    int Run();

    int CreateOtherMsg(AppSpawnReqMsgHandle &reqHandle, pid_t pid);
    int CreateMsg(AppSpawnReqMsgHandle &reqHandle, const char *defaultConfig);
    int StartSendMsg();
    int SendMsg();
    AppSpawnClientHandle GetClientHandle() { return clientHandle_; }
private:
    std::vector<std::string> split(const std::string &str, const std::string &pattern);
    int InitPtyInterface();
    int ProcessInputCmd(std::string &cmd);
    int AddExtTlv(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle);
    int BuildMsgFromJson(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle);
    int AddBundleInfoFromJson(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle);
    int GetDacInfoFromJson(const cJSON *appInfoConfig, AppDacInfo &info);
    int GetInternetPermissionInfoFromJson(const cJSON *appInfoConfig, AppSpawnMsgInternetInfo &info);
    int GetAccessTokenFromJson(const cJSON *appInfoConfig, AppSpawnMsgAccessToken &info);
    int AddDomainInfoFromJson(const cJSON *appInfoConfig, AppSpawnReqMsgHandle reqHandle);
    uint32_t GetUint32ArrayFromJson(const cJSON *json, const char *name, uint32_t dataArray[], uint32_t maxCount);
    static AppSpawnTestCommander *ConvertTo(const ThreadContext *context)
    {
        return const_cast<AppSpawnTestCommander *>(reinterpret_cast<const AppSpawnTestCommander *>(context));
    }
    static void TaskExecutorProc(ThreadTaskHandle handle, const ThreadContext *context);
    static void SendTaskFinish(ThreadTaskHandle handle, const ThreadContext *context);
    static void InputThread(ThreadTaskHandle handle, const ThreadContext *context);
    static void DumpThread(ThreadTaskHandle handle, const ThreadContext *context);

    int ptyFd_{-1};
    uint32_t dumpFlags : 1;
    uint32_t exit_ : 1;
    uint32_t appSpawn_ : 1;
    uint32_t msgType_;
    pid_t terminatePid_;
    uint32_t threadCount_{1};
    std::string ptyName_{};
    std::string testFileName_{};
    std::string processName_{};
    cJSON *appInfoConfig_ { nullptr};
    AppSpawnClientHandle clientHandle_{nullptr};
    ThreadMgr threadMgr_{nullptr};
    ThreadTaskHandle inputHandle_{0};
    ThreadTaskHandle dumpHandle_{0};
};
}  // namespace AppSpawnModuleTest
}  // namespace OHOS
#endif  // APPSPAWN_TEST_CMD_H