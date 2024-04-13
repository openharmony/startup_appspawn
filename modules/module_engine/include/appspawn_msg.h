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

#ifndef APPSPAWN_CLIENT_MSG_H
#define APPSPAWN_CLIENT_MSG_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "appspawn.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NWEBSPAWN_SOCKET_NAME "NWebSpawn"
#define APPSPAWN_SOCKET_NAME "AppSpawn"
#define KEEPALIVE_NAME "keepalive"

#define APPSPAWN_ALIGN(len) (((len) + 0x03) & (~0x03))
#define APPSPAWN_TLV_NAME_LEN 32
#define MAX_MSG_BLOCK_LEN (4 * 1024)
#define RECV_BLOCK_LEN 1024
#define MAX_MSG_TOTAL_LENGTH (64 * 1024)
#define EXTRAINFO_TOTAL_LENGTH_MAX (32 * 1024)
#define MAX_TLV_COUNT 128
#define APPSPAWN_MSG_MAGIC 0xEF201234

#define APP_LEN_PROC_NAME 256    // process name length
#define APP_LEN_BUNDLE_NAME 256  // bundle name length
#define APP_LEN_SO_PATH 256      // load so lib
#define APP_APL_MAX_LEN 32
#define APP_RENDER_CMD_MAX_LEN 2048
#define APP_OWNER_ID_LEN 64

typedef enum {
    TLV_BUNDLE_INFO = 0,
    TLV_MSG_FLAGS,
    TLV_DAC_INFO,
    TLV_DOMAIN_INFO,
    TLV_OWNER_INFO,
    TLV_ACCESS_TOKEN_INFO,
    TLV_PERMISSION,
    TLV_INTERNET_INFO,
    TLV_RENDER_TERMINATION_INFO,
    TLV_MAX
} AppSpawnMsgTlvType;

#define DATA_TYPE_STRING 1

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wextern-c-compat"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wextern-c-compat"
#elif defined(_MSC_VER)
#    pragma warning(push)
#endif

#pragma pack(4)
typedef AppDacInfo AppSpawnMsgDacInfo;

typedef struct {
    uint16_t tlvLen;
    uint16_t tlvType;
} AppSpawnTlv;

typedef struct {
    uint16_t tlvLen;  // 对齐后的长度
    uint16_t tlvType;
    uint16_t dataLen;  // 数据的长度
    uint16_t dataType;
    char tlvName[APPSPAWN_TLV_NAME_LEN];
} AppSpawnTlvExt;

typedef struct {
    uint32_t count;
    uint32_t flags[0];
} AppSpawnMsgFlags;

typedef struct {
    uint64_t accessTokenIdEx;
} AppSpawnMsgAccessToken;

typedef struct {
    char ownerId[0];  // app identifier id
} AppSpawnMsgOwnerId;

typedef struct {
    uint8_t setAllowInternet;
    uint8_t allowInternet;  // hap sockect allowed
    uint8_t res[2];
} AppSpawnMsgInternetInfo;

typedef struct {
    uint32_t hapFlags;
    char apl[0];
} AppSpawnMsgDomainInfo;

typedef struct {
    uint32_t bundleIndex;
    char bundleName[0];  // process name
} AppSpawnMsgBundleInfo;

typedef struct TagAppSpawnMsg {
    uint32_t magic;
    uint32_t msgType;
    uint32_t msgLen;
    uint32_t msgId;
    uint32_t tlvCount;
    char processName[APP_LEN_PROC_NAME];
} AppSpawnMsg;

typedef struct {
    AppSpawnMsg msgHdr;
    AppSpawnResult result;
} AppSpawnResponseMsg;
#pragma pack()

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif
#endif  // APPSPAWN_CLIENT_MSG_H
