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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "securec.h"

void *GetAppSpawnMsgInfo(const AppSpawnMsgNode *message, int type)
{
    APPSPAWN_CHECK(type < TLV_MAX, return NULL, "Invalid tlv type %{public}u", type);
    APPSPAWN_CHECK_ONLY_EXPER(message != NULL && message->buffer != NULL, return NULL);
    APPSPAWN_CHECK_ONLY_EXPER(message->tlvOffset != NULL, return NULL);
    APPSPAWN_CHECK_ONLY_EXPER(message->tlvOffset[type] != INVALID_OFFSET, return NULL);
    return (void *)(message->buffer + message->tlvOffset[type] + sizeof(AppSpawnTlv));
}

void *GetAppSpawnMsgExtInfo(const AppSpawnMsgNode *message, const char *name, uint32_t *len)
{
    APPSPAWN_CHECK(name != NULL, return NULL, "Invalid name ");
    APPSPAWN_CHECK_ONLY_EXPER(message != NULL && message->buffer != NULL, return NULL);
    APPSPAWN_CHECK_ONLY_EXPER(message->tlvOffset != NULL, return NULL);

    APPSPAWN_LOGV("GetAppSpawnMsgExtInfo tlvCount %{public}d name %{public}s", message->tlvCount, name);
    for (uint32_t index = TLV_MAX; index < (TLV_MAX + message->tlvCount); index++) {
        if (message->tlvOffset[index] == INVALID_OFFSET) {
            return NULL;
        }
        uint8_t *data = message->buffer + message->tlvOffset[index];
        if (((AppSpawnTlv *)data)->tlvType != TLV_MAX) {
            continue;
        }
        AppSpawnTlvExt *tlv = (AppSpawnTlvExt *)data;
        if (strcmp(tlv->tlvName, name) != 0) {
            continue;
        }
        if (len != NULL) {
            *len = tlv->dataLen;
        }
        return data + sizeof(AppSpawnTlvExt);
    }
    return NULL;
}

int CheckAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index)
{
    APPSPAWN_CHECK(type == TLV_MSG_FLAGS || type == TLV_PERMISSION, return 0, "Invalid tlv %{public}u ", type);
    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppSpawnMsgInfo(message, type);
    APPSPAWN_CHECK(msgFlags != NULL, return 0, "No tlv %{public}u in msg", type);
    uint32_t blockIndex = index / 32;  // 32 max bit in int
    uint32_t bitIndex = index % 32;    // 32 max bit in int
    APPSPAWN_CHECK(blockIndex < msgFlags->count, return 0,
        "Invalid index %{public}d max: %{public}d", index, msgFlags->count);
    return CHECK_FLAGS_BY_INDEX(msgFlags->flags[blockIndex], bitIndex);
}

static inline int SetSpawnMsgFlags(AppSpawnMsgFlags *msgFlags, uint32_t index)
{
    uint32_t blockIndex = index / 32;  // 32 max bit in int
    uint32_t bitIndex = index % 32;    // 32 max bit in int
    APPSPAWN_CHECK(blockIndex < msgFlags->count, return APPSPAWN_ARG_INVALID,
        "Invalid index %{public}u blockIndex %{public}u %{public}u ", index, blockIndex, msgFlags->count);
    msgFlags->flags[blockIndex] |= (1 << bitIndex);
    return 0;
}

int SetAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index)
{
    APPSPAWN_CHECK(type == TLV_MSG_FLAGS || type == TLV_PERMISSION,
        return APPSPAWN_ARG_INVALID, "Invalid tlv %{public}u ", type);
    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppSpawnMsgInfo(message, type);
    APPSPAWN_CHECK(msgFlags != NULL, return APPSPAWN_ARG_INVALID, "No tlv %{public}d in msg", type);
    return SetSpawnMsgFlags(msgFlags, index);
}

AppSpawnMsgNode *CreateAppSpawnMsg(void)
{
    AppSpawnMsgNode *message = (AppSpawnMsgNode *)calloc(1, sizeof(AppSpawnMsgNode));
    APPSPAWN_CHECK(message != NULL, return NULL, "Failed to create message");
    message->buffer = NULL;
    message->tlvOffset = NULL;
    return message;
}

void DeleteAppSpawnMsg(AppSpawnMsgNode *msgNode)
{
    if (msgNode == NULL) {
        return;
    }
    if (msgNode->buffer) {
        free(msgNode->buffer);
        msgNode->buffer = NULL;
    }
    if (msgNode->tlvOffset) {
        free(msgNode->tlvOffset);
        msgNode->tlvOffset = NULL;
    }
    free(msgNode);
}

static inline int CheckRecvMsg(const AppSpawnMsg *msg)
{
    APPSPAWN_CHECK(msg != NULL, return -1, "Invalid msg");
    APPSPAWN_CHECK(msg->magic == APPSPAWN_MSG_MAGIC, return -1, "Invalid magic 0x%{public}x", msg->magic);
    APPSPAWN_CHECK(msg->msgLen < MAX_MSG_TOTAL_LENGTH, return -1, "Message too long %{public}u", msg->msgLen);
    APPSPAWN_CHECK(msg->msgLen >= sizeof(AppSpawnMsg), return -1, "Message too long %{public}u", msg->msgLen);
    APPSPAWN_CHECK(msg->tlvCount < MAX_TLV_COUNT, return -1, "Message too long %{public}u", msg->tlvCount);
    APPSPAWN_CHECK(msg->tlvCount < (msg->msgLen / sizeof(AppSpawnTlv)),
        return -1, "Message too long %{public}u", msg->tlvCount);
    return 0;
}

static int AppSpawnMsgRebuild(AppSpawnMsgNode *message, const AppSpawnMsg *msg)
{
    APPSPAWN_CHECK_ONLY_EXPER(CheckRecvMsg(&message->msgHeader) == 0, return APPSPAWN_MSG_INVALID);
    if (msg->msgLen == sizeof(message->msgHeader)) {  // only has msg header
        return 0;
    }
    if (message->buffer == NULL) {
        message->buffer = calloc(1, msg->msgLen - sizeof(message->msgHeader));
        APPSPAWN_CHECK(message->buffer != NULL, return -1, "Failed to alloc memory for recv message");
    }
    if (message->tlvOffset == NULL) {
        uint32_t totalCount = msg->tlvCount + TLV_MAX;
        message->tlvOffset = malloc(totalCount * sizeof(uint32_t));
        APPSPAWN_CHECK(message->tlvOffset != NULL, return -1, "Failed to alloc memory for recv message");
        for (uint32_t i = 0; i < totalCount; i++) {
            message->tlvOffset[i] = INVALID_OFFSET;
        }
    }
    return 0;
}

AppSpawnMsgNode *RebuildAppSpawnMsgNode(AppSpawnMsgNode *message, AppSpawnedProcess *appInfo)
{
#ifdef DEBUG_BEGETCTL_BOOT
    APPSPAWN_CHECK(message != NULL && appInfo != NULL, return NULL, "params is null");
    uint32_t bufferLen = 0;
    AppSpawnMsgNode *node = CreateAppSpawnMsg();
    APPSPAWN_CHECK(message != NULL, return NULL, "Failed to create AppSpawnMsgNode");
    int ret = memcpy_s(&node->msgHeader, sizeof(AppSpawnMsg), &message->msgHeader, sizeof(AppSpawnMsg));
    APPSPAWN_CHECK(ret == 0, return NULL, "Failed to memcpy_s node->msgHeader");
    bufferLen = message->msgHeader.msgLen + appInfo->message->msgHeader.msgLen - sizeof(AppSpawnMsg);
    node->msgHeader.msgLen = bufferLen;
    node->msgHeader.msgType = MSG_SPAWN_NATIVE_PROCESS;
    node->msgHeader.tlvCount += message->msgHeader.tlvCount;
    ret = AppSpawnMsgRebuild(node, &node->msgHeader);
    APPSPAWN_CHECK(ret == 0, return NULL, "Failed to alloc memory for recv message");
    uint32_t appInfoBufLen = appInfo->message->msgHeader.msgLen - sizeof(AppSpawnMsg);
    uint32_t msgBufLen = message->msgHeader.msgLen - sizeof(AppSpawnMsg);
    ret = memcpy_s(node->buffer, bufferLen, appInfo->message->buffer, appInfoBufLen);
    APPSPAWN_CHECK(ret == 0, return NULL, "Failed to memcpy_s appInfo buffer");
    ret = memcpy_s(node->buffer + appInfoBufLen, bufferLen - appInfoBufLen, message->buffer, msgBufLen);
    APPSPAWN_CHECK(ret == 0, return NULL, "Failed to memcpy_s message->buffer");
    return node;
#endif
    return NULL;
}

int CheckAppSpawnMsg(const AppSpawnMsgNode *message)
{
    APPSPAWN_CHECK_ONLY_EXPER(message != NULL, return APPSPAWN_MSG_INVALID);
    APPSPAWN_CHECK(strlen(message->msgHeader.processName) > 0,
        return APPSPAWN_MSG_INVALID, "Invalid property processName %{public}s", message->msgHeader.processName);
    APPSPAWN_CHECK(message->tlvOffset != NULL,
        return APPSPAWN_MSG_INVALID, "Invalid property tlv offset for %{public}s", message->msgHeader.processName);
    APPSPAWN_CHECK(message->buffer != NULL,
        return APPSPAWN_MSG_INVALID, "Invalid property buffer for %{public}s", message->msgHeader.processName);

    if (message->tlvOffset[TLV_BUNDLE_INFO] == INVALID_OFFSET ||
        message->tlvOffset[TLV_MSG_FLAGS] == INVALID_OFFSET ||
        message->tlvOffset[TLV_ACCESS_TOKEN_INFO] == INVALID_OFFSET ||
        message->tlvOffset[TLV_DOMAIN_INFO] == INVALID_OFFSET ||
        message->tlvOffset[TLV_DAC_INFO] == INVALID_OFFSET) {
        APPSPAWN_LOGE("No must tlv bundle: %{public}u flags: %{public}u token: %{public}u domain %{public}u %{public}u",
            message->tlvOffset[TLV_BUNDLE_INFO], message->tlvOffset[TLV_MSG_FLAGS],
            message->tlvOffset[TLV_ACCESS_TOKEN_INFO],
            message->tlvOffset[TLV_DOMAIN_INFO], message->tlvOffset[TLV_DAC_INFO]);
        return APPSPAWN_MSG_INVALID;
    }
    AppSpawnMsgBundleInfo *bundleInfo = (AppSpawnMsgBundleInfo *)GetAppSpawnMsgInfo(message, TLV_BUNDLE_INFO);
    if (bundleInfo != NULL) {
        if (strstr(bundleInfo->bundleName, "\\") != NULL || strstr(bundleInfo->bundleName, "/") != NULL) {
            APPSPAWN_LOGE("Invalid bundle name %{public}s", bundleInfo->bundleName);
            return APPSPAWN_MSG_INVALID;
        }
    }
    return 0;
}

static int CheckExtTlvInfo(const AppSpawnTlv *tlv, uint32_t remainLen)
{
    AppSpawnTlvExt *tlvExt = (AppSpawnTlvExt *)(tlv);
    APPSPAWN_LOGV("Recv type [%{public}s %{public}u] real len: %{public}u",
        tlvExt->tlvName, tlvExt->tlvLen, tlvExt->dataLen);
    if (tlvExt->dataLen > tlvExt->tlvLen - sizeof(AppSpawnTlvExt)) {
        APPSPAWN_LOGE("Invalid tlv [%{public}s %{public}u] real len: %{public}u %{public}zu",
            tlvExt->tlvName, tlvExt->tlvLen, tlvExt->dataLen, sizeof(AppSpawnTlvExt));
        return APPSPAWN_MSG_INVALID;
    }
    return 0;
}

static int CheckMsgTlv(const AppSpawnTlv *tlv, uint32_t remainLen)
{
    uint32_t tlvLen = 0;
    switch (tlv->tlvType) {
        case TLV_MSG_FLAGS:
            tlvLen = ((AppSpawnMsgFlags *)(tlv + 1))->count * sizeof(uint32_t);
            break;
        case TLV_ACCESS_TOKEN_INFO:
            tlvLen = sizeof(AppSpawnMsgAccessToken);
            break;
        case TLV_DAC_INFO:
            tlvLen = sizeof(AppSpawnMsgDacInfo);
            break;
        case TLV_BUNDLE_INFO:
            APPSPAWN_CHECK((tlv->tlvLen - sizeof(AppSpawnTlv)) <= (sizeof(AppSpawnMsgBundleInfo) + APP_LEN_BUNDLE_NAME),
                return APPSPAWN_MSG_INVALID, "Invalid property tlv %{public}d %{public}d ", tlv->tlvType, tlv->tlvLen);
            break;
        case TLV_OWNER_INFO:
            APPSPAWN_CHECK((tlv->tlvLen - sizeof(AppSpawnTlv)) <= APP_OWNER_ID_LEN,
                return APPSPAWN_MSG_INVALID, "Invalid property tlv %{public}d %{public}d ", tlv->tlvType, tlv->tlvLen);
            break;
        case TLV_DOMAIN_INFO:
            APPSPAWN_CHECK((tlv->tlvLen - sizeof(AppSpawnTlv)) <= (APP_APL_MAX_LEN + sizeof(AppSpawnMsgDomainInfo)),
                return APPSPAWN_MSG_INVALID, "Invalid property tlv %{public}d %{public}d ", tlv->tlvType, tlv->tlvLen);
            break;
        case TLV_MAX:
            return CheckExtTlvInfo(tlv, remainLen);
        default:
            break;
    }
    APPSPAWN_CHECK(tlvLen <= tlv->tlvLen,
        return APPSPAWN_MSG_INVALID, "Invalid property tlv %{public}d %{public}d ", tlv->tlvType, tlv->tlvLen);
    return 0;
}

int DecodeAppSpawnMsg(AppSpawnMsgNode *message)
{
    APPSPAWN_CHECK_ONLY_EXPER(message != NULL, return APPSPAWN_MSG_INVALID);
    int ret = 0;
    uint32_t tlvCount = 0;
    uint32_t bufferLen = message->msgHeader.msgLen - sizeof(AppSpawnMsg);
    uint32_t currLen = 0;
    while (currLen < bufferLen) {
        AppSpawnTlv *tlv = (AppSpawnTlv *)(message->buffer + currLen);
        APPSPAWN_CHECK(tlv->tlvLen <= (bufferLen - currLen), break,
            "Invalid tlv [%{public}d %{public}d] curr: %{public}zu",
            tlv->tlvType, tlv->tlvLen, currLen + sizeof(AppSpawnMsg));

        APPSPAWN_LOGV("DecodeAppSpawnMsg tlv %{public}u %{public}u start: %{public}zu ",
            tlv->tlvType, tlv->tlvLen, currLen + sizeof(AppSpawnMsg)); // show in msg offset
        ret = CheckMsgTlv(tlv, bufferLen - currLen);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, break);

        if (tlv->tlvType < TLV_MAX) {  // normal
            message->tlvOffset[tlv->tlvType] = currLen;
            currLen += tlv->tlvLen;
        } else {
            APPSPAWN_CHECK(tlvCount < message->msgHeader.tlvCount, break,
                "Invalid tlv number tlv %{public}d tlvCount: %{public}d", tlv->tlvType, tlvCount);
            message->tlvOffset[TLV_MAX + tlvCount] = currLen;
            tlvCount++;
            currLen += tlv->tlvLen;
        }
    }
    APPSPAWN_CHECK_ONLY_EXPER(currLen >= bufferLen, return APPSPAWN_MSG_INVALID);
    // save real ext tlv count
    message->tlvCount = tlvCount;
    return 0;
}

int GetAppSpawnMsgFromBuffer(const uint8_t *buffer, uint32_t bufferLen,
    AppSpawnMsgNode **outMsg, uint32_t *msgRecvLen, uint32_t *reminder)
{
    APPSPAWN_CHECK_ONLY_EXPER(buffer != NULL && outMsg != NULL, return APPSPAWN_MSG_INVALID);
    APPSPAWN_CHECK_ONLY_EXPER(msgRecvLen != NULL && reminder != NULL, return APPSPAWN_MSG_INVALID);
    *reminder = 0;
    AppSpawnMsgNode *message = *outMsg;
    if (message == NULL) {
        message = CreateAppSpawnMsg();
        APPSPAWN_CHECK(message != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to create message");
        *outMsg = message;
    }

    uint32_t reminderLen = bufferLen;
    const uint8_t *reminderBuffer = buffer;
    if (*msgRecvLen < sizeof(AppSpawnMsg)) {  // recv partial message
        if ((bufferLen + *msgRecvLen) >= sizeof(AppSpawnMsg)) {
            int ret = memcpy_s(((uint8_t *)&message->msgHeader) + *msgRecvLen,
                sizeof(message->msgHeader) - *msgRecvLen,
                buffer, sizeof(message->msgHeader) - *msgRecvLen);
            APPSPAWN_CHECK(ret == EOK, return -1, "Failed to copy recv buffer");

            ret = AppSpawnMsgRebuild(message, &message->msgHeader);
            APPSPAWN_CHECK(ret == 0, return -1, "Failed to alloc buffer for receive msg");
            reminderLen = bufferLen - (sizeof(message->msgHeader) - *msgRecvLen);
            reminderBuffer = buffer + sizeof(message->msgHeader) - *msgRecvLen;
            *msgRecvLen = sizeof(message->msgHeader);
        } else {
            int ret = memcpy_s(((uint8_t *)&message->msgHeader) + *msgRecvLen,
                sizeof(message->msgHeader) - *msgRecvLen, buffer, bufferLen);
            APPSPAWN_CHECK(ret == EOK, return -1, "Failed to copy recv buffer");
            *msgRecvLen += bufferLen;
            return 0;
        }
    }
    // do not copy msg header
    uint32_t realCopy = (reminderLen + *msgRecvLen) > message->msgHeader.msgLen ?
        message->msgHeader.msgLen - *msgRecvLen : reminderLen;
    if (message->buffer == NULL) {  // only has msg header
        return 0;
    }
    APPSPAWN_LOGV("HandleRecvBuffer msgRecvLen: %{public}u reminderLen %{public}u realCopy %{public}u",
        *msgRecvLen, reminderLen, realCopy);
    int ret = memcpy_s(message->buffer + *msgRecvLen - sizeof(message->msgHeader),
        message->msgHeader.msgLen - *msgRecvLen, reminderBuffer, realCopy);
    APPSPAWN_CHECK(ret == EOK, return -1, "Failed to copy recv buffer");
    *msgRecvLen += realCopy;
    if (realCopy < reminderLen) {
        *reminder = reminderLen - realCopy;
    }
    return 0;
}

static inline void DumpMsgFlags(const char *info, const AppSpawnMsgFlags *msgFlags)
{
    APPSPAPWN_DUMP("%{public}s count: %{public}u ", info, msgFlags->count);
    for (uint32_t i = 0; i < msgFlags->count; i++) {
        APPSPAPWN_DUMP("%{public}s flags: 0x%{public}x", info, msgFlags->flags[i]);
    }
}

static inline void DumpMsgExtInfo(const AppSpawnTlv *tlv)
{
    if (tlv->tlvType != TLV_MAX) {
        APPSPAPWN_DUMP("App tlv info: [%{public}d %{public}d]", tlv->tlvType, tlv->tlvLen);
        return;
    }
    AppSpawnTlvExt *tlvExt = (AppSpawnTlvExt *)(tlv);
    APPSPAPWN_DUMP("App extend info name: %{public}s len: %{public}u", tlvExt->tlvName, tlvExt->dataLen);
    if (tlvExt->dataType == DATA_TYPE_STRING) {
        APPSPAPWN_DUMP("App extend info value: '%{public}s'", (char *)(tlvExt + 1));
    }
}

void DumpAppSpawnMsg(const AppSpawnMsgNode *message)
{
    APPSPAWN_CHECK_ONLY_EXPER(message != NULL, return);
    APPSPAPWN_DUMP("App spawn msg msgId: %{public}u msgLen: %{public}u tlvCount: %{public}u",
        message->msgHeader.msgId, message->msgHeader.msgLen, message->tlvCount);
    APPSPAPWN_DUMP("App spawn msg process name: %{public}s", message->msgHeader.processName);

    AppSpawnMsgFlags *msgFlags = (AppSpawnMsgFlags *)GetAppSpawnMsgInfo(message, TLV_MSG_FLAGS);
    APPSPAWN_ONLY_EXPER(msgFlags != NULL, DumpMsgFlags("App flags", msgFlags));
    msgFlags = (AppSpawnMsgFlags *)GetAppSpawnMsgInfo(message, TLV_PERMISSION);
    APPSPAWN_ONLY_EXPER(msgFlags != NULL, DumpMsgFlags("App permission bits", msgFlags));

    AppSpawnMsgDacInfo *dacInfo = (AppSpawnMsgDacInfo *)GetAppSpawnMsgInfo(message, TLV_DAC_INFO);
    if (dacInfo != NULL) {
        APPSPAPWN_DUMP("App dac info uid: %{public}d gid: %{public}d count: %{public}d",
            dacInfo->uid, dacInfo->gid, dacInfo->gidCount);
        for (uint32_t i = 0; i < dacInfo->gidCount; i++) {
            APPSPAPWN_DUMP("gid group[%{public}d]: %{public}d", i, dacInfo->gidTable[i]);
        }
    }
    AppSpawnMsgBundleInfo *bundleInfo = (AppSpawnMsgBundleInfo *)GetAppSpawnMsgInfo(message, TLV_BUNDLE_INFO);
    APPSPAWN_ONLY_EXPER(bundleInfo != NULL,
        APPSPAPWN_DUMP("App bundle info name: \"%{public}s\" index: %{public}d",
        bundleInfo->bundleName, bundleInfo->bundleIndex));

    AppSpawnMsgDomainInfo *domainInfo = (AppSpawnMsgDomainInfo *)GetAppSpawnMsgInfo(message, TLV_DOMAIN_INFO);
    APPSPAWN_ONLY_EXPER(domainInfo != NULL,
        APPSPAPWN_DUMP("App domain info hap: 0x%{public}x apl: \"%{public}s\"", domainInfo->hapFlags, domainInfo->apl));

    AppSpawnMsgOwnerId *owner = (AppSpawnMsgOwnerId *)GetAppSpawnMsgInfo(message, TLV_OWNER_INFO);
    APPSPAWN_ONLY_EXPER(owner != NULL, APPSPAPWN_DUMP("App owner info: \"%{public}s\" ", owner->ownerId));

    AppSpawnMsgAccessToken *t = (AppSpawnMsgAccessToken *)GetAppSpawnMsgInfo(message, TLV_ACCESS_TOKEN_INFO);
    APPSPAWN_ONLY_EXPER(t != NULL,
        APPSPAPWN_DUMP("App access token info: %{public}" PRId64 "", t->accessTokenIdEx));

    AppSpawnMsgInternetInfo *info = (AppSpawnMsgInternetInfo *)GetAppSpawnMsgInfo(message, TLV_INTERNET_INFO);
    APPSPAWN_ONLY_EXPER(info != NULL,
        APPSPAPWN_DUMP("App internet permission info [%{public}d %{public}d]",
        info->setAllowInternet, info->allowInternet));

    for (uint32_t i = TLV_MAX; i < TLV_MAX + message->tlvCount; i++) {
        if (message->tlvOffset[i] == INVALID_OFFSET) {
            continue;
        }
        DumpMsgExtInfo((AppSpawnTlv *)(message->buffer + message->tlvOffset[i]));
    }
}