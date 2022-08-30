/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "appspawn_message.h"
#include "appspawn_server.h"

#ifdef OHOS_DEBUG
#include <errno.h>
#include <time.h>
#endif  // OHOS_DEBUG

#include "iproxy_server.h"
#include "iunknown.h"
#include "ipc_skeleton.h"
#include "message.h"
#include "ohos_errno.h"
#include "ohos_init.h"
#include "samgr_lite.h"
#include "service.h"
#include "securec.h"

static const int INVALID_PID = -1;
static const int CLIENT_ID = 100;

typedef struct AppSpawnFeatureApi {
    INHERIT_SERVER_IPROXY;
} AppSpawnFeatureApi;

typedef struct AppSpawnService {
    INHERIT_SERVICE;
    INHERIT_IUNKNOWNENTRY(AppSpawnFeatureApi);
    Identity identity;
} AppSpawnService;

static const char *GetName(Service *service)
{
    UNUSED(service);
    APPSPAWN_LOGI("[appspawn] get service name %s.", APPSPAWN_SERVICE_NAME);
    return APPSPAWN_SERVICE_NAME;
}

static BOOL Initialize(Service *service, Identity identity)
{
    if (service == NULL) {
        APPSPAWN_LOGE("[appspawn] initialize, service NULL!");
        return FALSE;
    }

    AppSpawnService *spawnService = (AppSpawnService *)service;
    spawnService->identity = identity;

    APPSPAWN_LOGI("[appspawn] initialize, identity<%d, %d, %p>", \
        identity.serviceId, identity.featureId, identity.queueId);
    return TRUE;
}

static BOOL MessageHandle(Service *service, Request *msg)
{
    UNUSED(service);
    UNUSED(msg);
    APPSPAWN_LOGE("[appspawn] message handle not support yet!");
    return FALSE;
}

static TaskConfig GetTaskConfig(Service *service)
{
    UNUSED(service);
    TaskConfig config = {LEVEL_HIGH, PRI_BELOW_NORMAL, 0x800, 20, SHARED_TASK};
    return config;
}

static int GetMessageSt(MessageSt *msgSt, IpcIo *req)
{
    if (msgSt == NULL || req == NULL) {
        return EC_FAILURE;
    }

    size_t len = 0;
    char* str = ReadString(req, &len);
    if (str == NULL || len == 0) {
        APPSPAWN_LOGE("[appspawn] invoke, get data failed.");
        return EC_FAILURE;
    }

    int ret = SplitMessage(str, len, msgSt);  // after split message, str no need to free(linux version)
    return ret;
}

static AppSpawnContentLite *g_appSpawnContentLite = NULL;
AppSpawnContent *AppSpawnCreateContent(const char *socketName, char *longProcName, uint32_t longProcNameLen, int cold)
{
    UNUSED(longProcName);
    UNUSED(longProcNameLen);
    APPSPAWN_LOGI("AppSpawnCreateContent %s", socketName);
    AppSpawnContentLite *appSpawnContent = (AppSpawnContentLite *)malloc(sizeof(AppSpawnContentLite));
    APPSPAWN_CHECK(appSpawnContent != NULL, return NULL, "Failed to alloc memory for appspawn");
    int ret = memset_s(appSpawnContent, sizeof(AppSpawnContentLite), 0, sizeof(AppSpawnContentLite));
    APPSPAWN_CHECK(ret == 0, free(appSpawnContent);
        return NULL, "Failed to memset content");
    appSpawnContent->content.longProcName = NULL;
    appSpawnContent->content.longProcNameLen = 0;
    g_appSpawnContentLite = appSpawnContent;
    return appSpawnContent;
}

static int Invoke(IServerProxy *iProxy, int funcId, void *origin, IpcIo *req, IpcIo *reply)
{
#ifdef OHOS_DEBUG
    struct timespec tmStart = {0};
    GetCurTime(&tmStart);
#endif  // OHOS_DEBUG

    UNUSED(iProxy);
    UNUSED(origin);

    if (reply == NULL || funcId != ID_CALL_CREATE_SERVICE || req == NULL) {
        APPSPAWN_LOGE("[appspawn] invoke, funcId %d invalid, reply %d.", funcId, INVALID_PID);
        WriteInt64(reply, INVALID_PID);
        return EC_BADPTR;
    }
    APPSPAWN_LOGI("[appspawn] invoke.");
    AppSpawnClientLite client = {};
    client.client.id = CLIENT_ID;
    client.client.flags = 0;
    client.client.cloneFlags = 0;
    if (GetMessageSt(&client.message, req) != EC_SUCCESS) {
        APPSPAWN_LOGE("[appspawn] invoke, parse failed! reply %d.", INVALID_PID);
        WriteInt64(reply, INVALID_PID);
        return EC_FAILURE;
    }

    APPSPAWN_LOGI("[appspawn] invoke, msg<%s,%s,%d,%d %d>", client.message.bundleName, client.message.identityID,
        client.message.uID, client.message.gID, client.message.capsCnt);
    /* Clone support only one parameter, so need to package application parameters */
    AppSandboxArg *sandboxArg = (AppSandboxArg *)malloc(sizeof(AppSandboxArg));
    if (sandboxArg == NULL) {
        WriteInt64(reply, INVALID_PID);
        return EC_FAILURE;
    }
    (void)memset_s(sandboxArg, sizeof(AppSandboxArg), 0, sizeof(AppSandboxArg));

    pid_t newPid = 0;
    sandboxArg->content = &g_appSpawnContentLite->content;
    sandboxArg->client = &client.client;
    int ret = AppSpawnProcessMsg(sandboxArg, &newPid);
    if (ret != 0) {
        newPid = -1;
    }
    FreeMessageSt(&client.message);
    WriteInt64(reply, newPid);
    free(sandboxArg);

#ifdef OHOS_DEBUG
    struct timespec tmEnd = {0};
    GetCurTime(&tmEnd);
    // 1s = 1000000000ns
    long timeUsed = (tmEnd.tv_sec - tmStart.tv_sec) * 1000000000L + (tmEnd.tv_nsec - tmStart.tv_nsec);
    APPSPAWN_LOGI("[appspawn] invoke, reply pid %d, timeused %ld ns.", newPid, timeUsed);
#else
    APPSPAWN_LOGI("[appspawn] invoke, reply pid %d.", newPid);
#endif  // OHOS_DEBUG

    return ((newPid > 0) ? EC_SUCCESS : EC_FAILURE);
}

static AppSpawnService g_appSpawnService = {
    .GetName = GetName,
    .Initialize = Initialize,
    .MessageHandle = MessageHandle,
    .GetTaskConfig = GetTaskConfig,
    SERVER_IPROXY_IMPL_BEGIN,
    .Invoke = Invoke,
    IPROXY_END,
};

void AppSpawnInit(void)
{
    if (SAMGR_GetInstance()->RegisterService((Service *)&g_appSpawnService) != TRUE) {
        APPSPAWN_LOGE("[appspawn] register service failed!");
        return;
    }

    APPSPAWN_LOGI("[appspawn] register service succeed. %p.", &g_appSpawnService);

    if (SAMGR_GetInstance()->RegisterDefaultFeatureApi(APPSPAWN_SERVICE_NAME, \
        GET_IUNKNOWN(g_appSpawnService)) != TRUE) {
        (void)SAMGR_GetInstance()->UnregisterService(APPSPAWN_SERVICE_NAME);
        APPSPAWN_LOGE("[appspawn] register featureapi failed!");
        return;
    }

    APPSPAWN_LOGI("[appspawn] register featureapi succeed.");
}

SYSEX_SERVICE_INIT(AppSpawnInit);
