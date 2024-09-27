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

#include "devicedebug_stub.h"

#include "appspawn.h"

#ifdef __cplusplus
extern "C" {
#endif

int g_isDeveloperModeOpen = 0;

void DeveloperModeOpenSet(int developerMode)
{
    g_isDeveloperModeOpen = developerMode;
}

int IsDeveloperModeOpenStub()
{
    return g_isDeveloperModeOpen;
}

int g_appSpawnClientInitRet = 0;

void AppSpawnClientInitRetSet(int appSpawnClientInitRet)
{
    g_appSpawnClientInitRet = appSpawnClientInitRet;
}

int AppSpawnClientInitStub(const char *serviceName, AppSpawnClientHandle *handle)
{
    (void)serviceName;
    (void)handle;

    return g_appSpawnClientInitRet;
}

int g_appSpawnReqMsgCreateRet = 0;

void AppSpawnReqMsgCreateRetSet(int appSpawnReqMsgCreateRet)
{
    g_appSpawnReqMsgCreateRet = appSpawnReqMsgCreateRet;
}

int AppSpawnReqMsgCreateStub(AppSpawnMsgType msgType, const char *processName, AppSpawnReqMsgHandle *reqHandle)
{
    (void)msgType;
    (void)processName;
    (void)reqHandle;

    return g_appSpawnReqMsgCreateRet;
}

int g_appSpawnReqMsgAddStringInfoRet = 0;

void AppSpawnReqMsgAddStringInfoRetSet(int appSpawnReqMsgAddStringInfoRet)
{
    g_appSpawnReqMsgAddStringInfoRet = appSpawnReqMsgAddStringInfoRet;
}

int AppSpawnReqMsgAddStringInfoStub(AppSpawnReqMsgHandle reqHandle, const char *name, const char *value)
{
    (void)reqHandle;
    (void)name;
    (void)value;

    return g_appSpawnReqMsgAddStringInfoRet;
}

int g_appSpawnClientSendMsgResulte = 0;
int g_appSpawnClientSendMsgRet = 0;

void AppSpawnClientSendMsgResultSet(int appSpawnClientSendMsgResult)
{
    g_appSpawnClientSendMsgResulte = appSpawnClientSendMsgResult;
}

void AppSpawnClientSendMsgRetSet(int appSpawnClientSendMsgRet)
{
    g_appSpawnClientSendMsgRet = appSpawnClientSendMsgRet;
}

int AppSpawnClientSendMsgStub(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle, AppSpawnResult *result)
{
    (void)handle;
    (void)reqHandle;
    result->result = g_appSpawnClientSendMsgResulte;

    return g_appSpawnClientSendMsgRet;
}

#ifdef __cplusplus
}
#endif
