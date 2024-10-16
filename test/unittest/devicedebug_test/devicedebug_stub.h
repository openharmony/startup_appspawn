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

#ifndef DEVICEDEBUG_STUB_H
#define DEVICEDEBUG_STUB_H

#ifdef __cplusplus
extern "C" {
#endif

void DeveloperModeOpenSet(int developerMode);

void AppSpawnClientInitRetSet(int appSpawnClientInitRet);

void AppSpawnReqMsgCreateRetSet(int appSpawnClientSendMsgRet);

void AppSpawnReqMsgAddExtInfoRetSet(int appSpawnReqMsgAddExtInfoRet);

void AppSpawnClientSendMsgResultSet(int appSpawnClientSendMsgResult);

void AppSpawnClientSendMsgRetSet(int appSpawnClientSendMsgRet);

#ifdef __cplusplus
}
#endif

#endif