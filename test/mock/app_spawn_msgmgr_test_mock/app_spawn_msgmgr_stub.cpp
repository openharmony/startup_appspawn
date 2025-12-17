/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "app_spawn_stub.h"

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdbool>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>

#include <linux/capability.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "access_token.h"
#include "hilog/log.h"
#include "securec.h"
#include "token_setproc.h"
#include "tokenid_kit.h"

#ifdef WITH_SELINUX
#include "hap_restorecon.h"
#endif
#ifdef WITH_SECCOMP
#include "seccomp_policy.h"
#include <sys/prctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *GetAppSpawnMsgInfo(const AppSpawnMsgNode *message, int type)
{
    if (message == nullptr) {
        printf("GetAppSpawnMsgInfo message is null");
    }

    if (type == 0) {
        printf("GetAppSpawnMsgInfo type is 0");
    }

    return nullptr;
}

void *GetAppSpawnMsgExtInfo(const AppSpawnMsgNode *message, const char *name, uint32_t *len)
{
    if (message == nullptr) {
        printf("GetAppSpawnMsgExtInfo message is null");
    }

    if (name == nullptr) {
        printf("GetAppSpawnMsgExtInfo name is null");
    }

    if (len == nullptr) {
        printf("GetAppSpawnMsgExtInfo len is null");
    }

    return nullptr;
}

int CheckAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index)
{
    if (message == nullptr) {
        printf("CheckAppSpawnMsgFlag message is null");
    }

    if (type == 0) {
        printf("CheckAppSpawnMsgFlag type is 0");
    }

    if (index == 0) {
        printf("CheckAppSpawnMsgFlag index is 0");
    }

    return nullptr;
}

int SetSpawnMsgFlags(AppSpawnMsgFlags *msgFlags, uint32_t index)
{
    if (msgFlags == nullptr) {
        printf("SetSpawnMsgFlags msgFlags is null");
    }

    if (index == 0) {
        printf("SetSpawnMsgFlags index is 0");
    }

    return 0;
}

int SetAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index)
{
    if (message == nullptr) {
        printf("SetAppSpawnMsgFlag message is null");
    }

    if (type == 0) {
        printf("SetAppSpawnMsgFlag type is 0");
    }

    if (index == 0) {
        printf("SetAppSpawnMsgFlag index is 0");
    }

    return 0;
}

AppSpawnMsgNode *CreateAppSpawnMsg(void)
{
    return nullptr;
}

void DeleteAppSpawnMsg(AppSpawnMsgNode **msgNode)
{
    if (message == nullptr) {
        printf("DeleteAppSpawnMsg data is null");
    }
}

int CheckRecvMsg(const AppSpawnMsg *msg)
{
    if (msg == nullptr) {
        printf("CheckRecvMsg msg is null");
    }

    return 0;
}

int AppSpawnMsgRebuild(AppSpawnMsgNode *message, const AppSpawnMsg *msg)
{
    if (message == nullptr) {
        printf("AppSpawnMsgRebuild message is null");
    }

    if (msg == nullptr) {
        printf("AppSpawnMsgRebuild msg is null");
    }

    return 0;
}

AppSpawnMsgNode *RebuildAppSpawnMsgNode(AppSpawnMsgNode *message, AppSpawnedProcess *appInfo)
{
    if (message == nullptr) {
        printf("RebuildAppSpawnMsgNode message is null");
    }

    if (appInfo == nullptr) {
        printf("RebuildAppSpawnMsgNode appInfo is null");
    }

    return nullptr;
}

int CheckAppSpawnMsg(const AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("CheckAppSpawnMsg message is null");
    }

    return 0;
}

int CheckExtTlvInfo(const AppSpawnTlv *tlv, uint32_t remainLen)
{
    if (tlv == nullptr) {
        printf("CheckExtTlvInfo tlv is null");
    }

    if (remainLen == 0) {
        printf("CheckExtTlvInfo remainLen is 0");
    }

    return 0;
}

int CheckMsgTlv(const AppSpawnTlv *tlv, uint32_t remainLen)
{
    if (tlv == nullptr) {
        printf("CheckMsgTlv tlv is null");
    }

    if (remainLen == 0) {
        printf("CheckMsgTlv remainLen is 0");
    }

    return 0;
}

int DecodeAppSpawnMsg(AppSpawnMsgNode *message)
{
    if (message == nullptr) {
        printf("DecodeAppSpawnMsg message is null");
    }

    return 0;
}

int GetAppSpawnMsgFromBuffer(const uint8_t *buffer, uint32_t bufferLen,
    AppSpawnMsgNode **outMsg, uint32_t *msgRecvLen, uint32_t *reminder)
{
    if (buffer == nullptr) {
        printf("GetAppSpawnMsgFromBuffer buffer is null");
    }

    if (bufferLen == 0) {
        printf("GetAppSpawnMsgFromBuffer bufferLen is null");
    }

    if (outMsg == nullptr) {
        printf("GetAppSpawnMsgFromBuffer outMsg is null");
    }

    if (msgRecvLen == nullptr) {
        printf("GetAppSpawnMsgFromBuffer msgRecvLen is null");
    }

    if (reminder == nullptr) {
        printf("GetAppSpawnMsgFromBuffer reminder is null");
    }

    return 0;
}

void DumpMsgFlags(const char *processName, const char *info, const AppSpawnMsgFlags *msgFlags)
{
    if (processName == nullptr) {
        printf("DumpMsgFlags processName is null");
    }

    if (info == nullptr) {
        printf("DumpMsgFlags info is null");
    }

    if (msgFlags == nullptr) {
        printf("DumpMsgFlags msgFlags is null");
    }
}

void DumpMsgExtInfo(const AppSpawnTlv *tlv)
{
    if (tlv == nullptr) {
        printf("DumpMsgExtInfo tlv is null");
    }
}

void DumpAppSpawnMsg(const AppSpawnMsgNode *message)
{
    if (tlv == nullptr) {
        printf("DumpAppSpawnMsg message is null");
    }
}

#ifdef __cplusplus
}
#endif
