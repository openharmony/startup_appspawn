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

#include "appspawn_client.h"
#include "appspawn_mount_permission.h"
#include "appspawn_hook.h"
#include "appspawn_utils.h"
#include "parameter.h"
#include "securec.h"

static inline int CalcFlagsUnits(uint32_t maxIndex)
{
    return ((maxIndex / 32) + ((maxIndex % 32 == 0) ? 0 : 1));  // 32 max bit in uint32_t
}

static inline int SetAppSpawnMsgFlags(AppSpawnMsgFlags *msgFlags, uint32_t index)
{
    uint32_t blockIndex = index / 32;  // 32 max bit in int
    uint32_t bitIndex = index % 32;    // 32 max bit in int
    if (blockIndex < msgFlags->count) {
        msgFlags->flags[blockIndex] |= (1 << bitIndex);
    }
    return 0;
}

static inline int CheckMsg(const AppSpawnReqMsgNode *reqNode, const AppSpawnTlv *tlv, const char *name)
{
    if ((reqNode->msg->msgLen + tlv->tlvLen) > MAX_MSG_TOTAL_LENGTH) {
        APPSPAWN_LOGE("The message is too long %{public}s", name);
        return APPSPAWN_MSG_INVALID;
    }
    if (reqNode->msg->msgType == MSG_GET_RENDER_TERMINATION_STATUS) {
        if (tlv->tlvType != TLV_RENDER_TERMINATION_INFO) {
            APPSPAWN_LOGE("Not support tlv %{public}s for message MSG_GET_RENDER_TERMINATION_STATUS", name);
            return APPSPAWN_TLV_NOT_SUPPORT;
        }
    }
    return 0;
}

static inline int CheckInputString(const char *info, const char *value, uint32_t maxLen)
{
    APPSPAWN_CHECK(value != NULL, return APPSPAWN_ARG_INVALID, "Invalid input info for %{public}s ", info);
    uint32_t valueLen = (uint32_t)strlen(value);
    APPSPAWN_CHECK(valueLen > 0 && valueLen < maxLen, return APPSPAWN_ARG_INVALID,
        "Invalid input string length '%{public}s' for '%{public}s'", value, info);
    return 0;
}

static AppSpawnMsgBlock *CreateAppSpawnMsgBlock(AppSpawnReqMsgNode *reqNode)
{
    AppSpawnMsgBlock *block = (AppSpawnMsgBlock *)calloc(1, MAX_MSG_BLOCK_LEN);
    APPSPAWN_CHECK(block != NULL, return NULL, "Failed to create block");
    OH_ListInit(&block->node);
    block->blockSize = MAX_MSG_BLOCK_LEN - sizeof(AppSpawnMsgBlock);
    block->currentIndex = 0;
    OH_ListAddTail(&reqNode->msgBlocks, &block->node);
    return block;
}

static AppSpawnMsgBlock *GetValidMsgBlock(const AppSpawnReqMsgNode *reqNode, uint32_t realLen)
{
    AppSpawnMsgBlock *block = NULL;
    struct ListNode *node = reqNode->msgBlocks.next;
    while (node != &reqNode->msgBlocks) {
        block = ListEntry(node, AppSpawnMsgBlock, node);
        if ((block->blockSize - block->currentIndex) >= realLen) {
            return block;
        }
        node = node->next;
    }
    return NULL;
}

static AppSpawnMsgBlock *GetTailMsgBlock(const AppSpawnReqMsgNode *reqNode)
{
    AppSpawnMsgBlock *block = NULL;
    struct ListNode *node = reqNode->msgBlocks.prev;
    if (node != &reqNode->msgBlocks) {
        block = ListEntry(node, AppSpawnMsgBlock, node);
    }
    return block;
}

static void FreeMsgBlock(ListNode *node)
{
    AppSpawnMsgBlock *block = ListEntry(node, AppSpawnMsgBlock, node);
    OH_ListRemove(node);
    OH_ListInit(node);
    free(block);
}

static int AddAppDataToBlock(AppSpawnMsgBlock *block, const uint8_t *data, uint32_t dataLen, int32_t dataType)
{
    APPSPAWN_CHECK(block->blockSize > block->currentIndex,
        return APPSPAWN_BUFFER_NOT_ENOUGH, "Not enough buffer for data");
    uint32_t reminderLen = block->blockSize - block->currentIndex;
    uint32_t realDataLen = (dataType == DATA_TYPE_STRING) ? APPSPAWN_ALIGN(dataLen + 1) : APPSPAWN_ALIGN(dataLen);
    APPSPAWN_CHECK(reminderLen >= realDataLen, return APPSPAWN_BUFFER_NOT_ENOUGH, "Not enough buffer for data");
    int ret = memcpy_s(block->buffer + block->currentIndex, reminderLen, data, dataLen);
    APPSPAWN_CHECK(ret == EOK, return APPSPAWN_SYSTEM_ERROR, "Failed to copy data");
    if (dataType == DATA_TYPE_STRING) {
        *((char *)block->buffer + block->currentIndex + dataLen) = '\0';
    }
    block->currentIndex += realDataLen;
    return 0;
}

static int AddAppDataToTail(AppSpawnReqMsgNode *reqNode, const uint8_t *data, uint32_t dataLen, int32_t dataType)
{
    uint32_t currLen = 0;
    AppSpawnMsgBlock *block = GetTailMsgBlock(reqNode);
    APPSPAWN_CHECK(block != NULL, return APPSPAWN_BUFFER_NOT_ENOUGH, "Not block info reqNode");
    uint32_t realDataLen = (dataType == DATA_TYPE_STRING) ? dataLen + 1 : dataLen;
    do {
        uint32_t reminderBufferLen = block->blockSize - block->currentIndex;
        uint32_t reminderDataLen = realDataLen - currLen;
        uint32_t realLen = APPSPAWN_ALIGN(reminderDataLen);
        uint32_t realCopy = 0;
        if (reminderBufferLen >= realLen) {  // 足够存储，直接保存
            int ret = memcpy_s(block->buffer + block->currentIndex, reminderBufferLen, data + currLen, reminderDataLen);
            APPSPAWN_CHECK(ret == EOK, return APPSPAWN_SYSTEM_ERROR, "Failed to copy data");
            block->currentIndex += realLen;
            break;
        } else if (reminderBufferLen > 0) {
            realCopy = reminderDataLen > reminderBufferLen ? reminderBufferLen : reminderDataLen;
            int ret = memcpy_s(block->buffer + block->currentIndex, reminderBufferLen, data + currLen, realCopy);
            APPSPAWN_CHECK(ret == EOK, return APPSPAWN_SYSTEM_ERROR, "Failed to copy data");
            block->currentIndex += realCopy;
            currLen += realCopy;
        }
        block = CreateAppSpawnMsgBlock(reqNode);
        APPSPAWN_CHECK(block != NULL, return APPSPAWN_SYSTEM_ERROR, "Not enough buffer for data");
    } while (currLen < realDataLen);
    return 0;
}

static int AddAppDataEx(AppSpawnReqMsgNode *reqNode, const char *name, const AppSpawnAppData *data)
{
    AppSpawnTlvExt tlv = {};
    if (data->dataType == DATA_TYPE_STRING) {
        tlv.tlvLen = APPSPAWN_ALIGN(data->dataLen + 1) + sizeof(AppSpawnTlvExt);
    } else {
        tlv.tlvLen = APPSPAWN_ALIGN(data->dataLen) + sizeof(AppSpawnTlvExt);
    }
    tlv.tlvType = TLV_MAX;
    tlv.dataLen = data->dataLen;
    tlv.dataType = data->dataType;
    int ret = strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), name);
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_SYSTEM_ERROR, "Failed to add data for %{public}s", name);
    ret = CheckMsg(reqNode, (AppSpawnTlv *)&tlv, name);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    APPSPAWN_LOGV("AddAppDataEx tlv [%{public}s %{public}u ] dataLen: %{public}u start: %{public}u",
        name, tlv.tlvLen, data->dataLen, reqNode->msg->msgLen);
    // 获取一个能保存改完整tlv的block
    AppSpawnMsgBlock *block = GetValidMsgBlock(reqNode, tlv.tlvLen);
    if (block != NULL) {
        ret = AddAppDataToBlock(block, (uint8_t *)&tlv, sizeof(tlv), 0);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add tlv for %{public}s", name);
        ret = AddAppDataToBlock(block, data->data, data->dataLen, data->dataType);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add data for %{public}s", name);
    } else {
        // 没有一个可用的block，最队列最后添加数据
        ret = AddAppDataToTail(reqNode, (uint8_t *)&tlv, sizeof(tlv), 0);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add tlv to tail for %{public}s", name);
        ret = AddAppDataToTail(reqNode, data->data, data->dataLen, data->dataType);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add data to tail for %{public}s", name);
    }
    reqNode->msg->tlvCount++;
    reqNode->msg->msgLen += tlv.tlvLen;
    APPSPAWN_LOGV("AddAppDataEx success name '%{public}s' end: %{public}u", name, reqNode->msg->msgLen);
    return 0;
}

static int AddAppData(AppSpawnReqMsgNode *reqNode,
    uint32_t tlvType, const AppSpawnAppData *data, uint32_t count, const char *name)
{
    // 计算实际数据的长度
    uint32_t realLen = sizeof(AppSpawnTlv);
    uint32_t dataLen = 0;
    for (uint32_t index = 0; index < count; index++) {
        dataLen += data[index].dataLen;
        realLen += (data[index].dataType == DATA_TYPE_STRING) ?
            APPSPAWN_ALIGN(data[index].dataLen + 1) : APPSPAWN_ALIGN(data[index].dataLen);
    }
    AppSpawnTlv tlv;
    tlv.tlvLen = realLen;
    tlv.tlvType = tlvType;
    int ret = CheckMsg(reqNode, &tlv, name);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    APPSPAWN_LOGV("AddAppData tlv [%{public}s %{public}u] dataLen: %{public}u start: %{public}u",
        name, tlv.tlvLen, dataLen, reqNode->msg->msgLen);
    // 获取一个能保存改完整tlv的block
    AppSpawnMsgBlock *block = GetValidMsgBlock(reqNode, tlv.tlvLen);
    if (block != NULL) {
        ret = AddAppDataToBlock(block, (uint8_t *)&tlv, sizeof(tlv), 0);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add tlv for %{public}d", tlvType);

        for (uint32_t index = 0; index < count; index++) {
            ret = AddAppDataToBlock(block, (uint8_t *)data[index].data, data[index].dataLen, data[index].dataType);
            APPSPAWN_CHECK(ret == 0, return ret, "Failed to add data for %{public}d", tlvType);
        }
    } else {
        // 没有一个可用的block，最队列最后添加数据
        ret = AddAppDataToTail(reqNode, (uint8_t *)&tlv, sizeof(tlv), 0);
        APPSPAWN_CHECK(ret == 0, return ret, "Failed to add tlv to tail for %{public}d", tlvType);
        // 添加tlv信息
        for (uint32_t index = 0; index < count; index++) {
            ret = AddAppDataToTail(reqNode, (uint8_t *)data[index].data, data[index].dataLen, data[index].dataType);
            APPSPAWN_CHECK(ret == 0, return ret, "Failed to add data for %{public}d", tlvType);
        }
    }
    reqNode->msg->msgLen += tlv.tlvLen;
    APPSPAWN_LOGV("AddAppData success tlvType %{public}s end: %{public}u", name, reqNode->msg->msgLen);
    return 0;
}

static int SetFlagsTlv(AppSpawnReqMsgNode *reqNode,
    AppSpawnMsgBlock *block, AppSpawnMsgFlags **msgFlags, int type, int maxCount)
{
    uint32_t units = CalcFlagsUnits(maxCount);
    APPSPAWN_LOGV("SetFlagsTlv maxCount %{public}d type %{public}d units %{public}d", maxCount, type, units);
    uint32_t flagsLen = sizeof(AppSpawnTlv) + sizeof(AppSpawnMsgFlags) + sizeof(uint32_t) * units;
    APPSPAWN_CHECK((block->blockSize - block->currentIndex) > flagsLen,
        return APPSPAWN_BUFFER_NOT_ENOUGH, "Invalid block to set flags tlv type %{public}d", type);

    AppSpawnTlv *tlv = (AppSpawnTlv *)(block->buffer + block->currentIndex);
    tlv->tlvLen = flagsLen;
    tlv->tlvType = type;
    *msgFlags = (AppSpawnMsgFlags *)(block->buffer + block->currentIndex + sizeof(AppSpawnTlv));
    (*msgFlags)->count = units;
    block->currentIndex += flagsLen;
    reqNode->msg->msgLen += flagsLen;
    reqNode->msg->tlvCount++;
    return 0;
}

static int CreateBaseMsg(AppSpawnReqMsgNode *reqNode, uint32_t msgType, const char *processName)
{
    AppSpawnMsgBlock *block = CreateAppSpawnMsgBlock(reqNode);
    APPSPAWN_CHECK(block != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to create block for %{public}s", processName);

    // 保留消息头的大小
    reqNode->msg = (AppSpawnMsg *)(block->buffer + block->currentIndex);
    reqNode->msg->magic = APPSPAWN_MSG_MAGIC;
    reqNode->msg->msgId = 0;
    reqNode->msg->msgType = msgType;
    reqNode->msg->msgLen = sizeof(AppSpawnMsg);
    reqNode->msg->tlvCount = 0;
    int ret = strcpy_s(reqNode->msg->processName, sizeof(reqNode->msg->processName), processName);
    APPSPAWN_CHECK(ret == 0, return APPSPAWN_SYSTEM_ERROR, "Failed to create block for %{public}s", processName);
    block->currentIndex = sizeof(AppSpawnMsg);
    ret = SetFlagsTlv(reqNode, block, &reqNode->msgFlags, TLV_MSG_FLAGS, MAX_FLAGS_INDEX);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    APPSPAWN_CHECK_ONLY_EXPER(msgType == MSG_APP_SPAWN || msgType == MSG_SPAWN_NATIVE_PROCESS, return 0);
    int maxCount = GetPermissionMaxCount();
    APPSPAWN_CHECK(maxCount > 0, return APPSPAWN_SYSTEM_ERROR, "Invalid max for permission %{public}s", processName);
    ret = SetFlagsTlv(reqNode, block, &reqNode->permissionFlags, TLV_PERMISSION, maxCount);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    APPSPAWN_LOGV("CreateBaseMsg msgLen: %{public}u %{public}u", reqNode->msg->msgLen, block->currentIndex);
    return 0;
}

static void DeleteAppSpawnReqMsg(AppSpawnReqMsgNode *reqNode)
{
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return);
    APPSPAWN_LOGV("DeleteAppSpawnReqMsg reqId: %{public}u", reqNode->reqId);
    reqNode->msgFlags = NULL;
    reqNode->permissionFlags = NULL;
    reqNode->msg = NULL;
    // 释放block
    OH_ListRemoveAll(&reqNode->msgBlocks, FreeMsgBlock);
    free(reqNode);
}

static AppSpawnReqMsgNode *CreateAppSpawnReqMsg(uint32_t msgType, const char *processName)
{
    static uint32_t reqId = 0;
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)malloc(sizeof(AppSpawnReqMsgNode));
    APPSPAWN_CHECK(reqNode != NULL, return NULL, "Failed to create msg node for %{public}s", processName);

    OH_ListInit(&reqNode->node);
    OH_ListInit(&reqNode->msgBlocks);
    reqNode->reqId = ++reqId;
    reqNode->msg = NULL;
    reqNode->msgFlags = NULL;
    reqNode->permissionFlags = NULL;
    int ret = CreateBaseMsg(reqNode, msgType, processName);
    APPSPAWN_CHECK(ret == 0, DeleteAppSpawnReqMsg(reqNode);
         return NULL, "Failed to create base msg for %{public}s", processName);
    APPSPAWN_LOGV("CreateAppSpawnReqMsg reqId: %{public}d msg type: %{public}u processName: %{public}s",
        reqNode->reqId, msgType, processName);
    return reqNode;
}

static void GetSpecialGid(const char *bundleName, gid_t gidTable[], uint32_t *gidCount)
{
    // special handle bundle name medialibrary and scanner
    const char *specialBundleNames[] = {
        "com.ohos.medialibrary.medialibrarydata", "com.ohos.medialibrary.medialibrarydata:backup"
    };

    for (size_t i = 0; i < sizeof(specialBundleNames) / sizeof(specialBundleNames[0]); i++) {
        if (strcmp(bundleName, specialBundleNames[i]) == 0) {
            if (*gidCount < APP_MAX_GIDS) {
                gidTable[(*gidCount)++] = GID_USER_DATA_RW;
                gidTable[(*gidCount)++] = GID_FILE_ACCESS;
            }
            break;
        }
    }
}

int AppSpawnReqMsgCreate(AppSpawnMsgType msgType, const char *processName, AppSpawnReqMsgHandle *reqHandle)
{
    APPSPAWN_CHECK(reqHandle != NULL, return APPSPAWN_ARG_INVALID, "Invalid request handle");
    APPSPAWN_CHECK(msgType < MAX_TYPE_INVALID,
        return APPSPAWN_MSG_INVALID, "Invalid message type %{public}u %{public}s", msgType, processName);
    int ret = CheckInputString("processName", processName, APP_LEN_PROC_NAME);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    AppSpawnReqMsgNode *reqNode = CreateAppSpawnReqMsg(msgType, processName);
    APPSPAWN_CHECK(reqNode != NULL, return APPSPAWN_SYSTEM_ERROR,
        "Failed to create msg node for %{public}s", processName);
    *reqHandle = (AppSpawnReqMsgHandle)(reqNode);
    return 0;
}

void AppSpawnReqMsgFree(AppSpawnReqMsgHandle reqHandle)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return);
    DeleteAppSpawnReqMsg(reqNode);
}

int AppSpawnReqMsgSetAppDacInfo(AppSpawnReqMsgHandle reqHandle, const AppDacInfo *dacInfo)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_CHECK(dacInfo != NULL, return APPSPAWN_ARG_INVALID, "Invalid dacInfo ");

    AppDacInfo tmpDacInfo = {0};
    (void)memcpy_s(&tmpDacInfo, sizeof(tmpDacInfo), dacInfo, sizeof(tmpDacInfo));
    GetSpecialGid(reqNode->msg->processName, tmpDacInfo.gidTable, &tmpDacInfo.gidCount);

    AppSpawnAppData data[MAX_DATA_IN_TLV] = {};
    data[0].data = (uint8_t *)&tmpDacInfo;
    data[0].dataLen = sizeof(AppSpawnMsgDacInfo);
    return AddAppData(reqNode, TLV_DAC_INFO, data, 1, "TLV_DAC_INFO");
}

int AppSpawnReqMsgSetBundleInfo(AppSpawnReqMsgHandle reqHandle, uint32_t bundleIndex, const char *bundleName)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    int ret = CheckInputString("TLV_BUNDLE_INFO", bundleName, APP_LEN_BUNDLE_NAME);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    AppSpawnMsgBundleInfo info = {};
    info.bundleIndex = bundleIndex;
    AppSpawnAppData data[MAX_DATA_IN_TLV] = {};
    data[0].data = (uint8_t *)&info;
    data[0].dataLen = sizeof(AppSpawnMsgBundleInfo);
    data[1].data = (uint8_t *)bundleName;
    data[1].dataLen = strlen(bundleName);
    data[1].dataType = DATA_TYPE_STRING;
    return AddAppData(reqNode, TLV_BUNDLE_INFO, data, MAX_DATA_IN_TLV, "TLV_BUNDLE_INFO");
}

int AppSpawnReqMsgSetAppFlag(AppSpawnReqMsgHandle reqHandle, AppFlagsIndex flagIndex)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_CHECK(reqNode->msgFlags != NULL, return APPSPAWN_ARG_INVALID, "No msg flags tlv ");
    APPSPAWN_CHECK(flagIndex < MAX_FLAGS_INDEX, return APPSPAWN_ARG_INVALID,
        "Invalid msg app flags %{public}d", flagIndex);
    return SetAppSpawnMsgFlags(reqNode->msgFlags, flagIndex);
}

int AppSpawnReqMsgAddExtInfo(AppSpawnReqMsgHandle reqHandle, const char *name, const uint8_t *value, uint32_t valueLen)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    int ret = CheckInputString("check name", name, APPSPAWN_TLV_NAME_LEN);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    APPSPAWN_CHECK(value != NULL && valueLen <= EXTRAINFO_TOTAL_LENGTH_MAX && valueLen > 0,
        return APPSPAWN_ARG_INVALID, "Invalid ext value ");

    APPSPAWN_LOGV("AppSpawnReqMsgAddExtInfo name %{public}s", name);
    AppSpawnAppData data[1] = {};  // 1 max data count
    data[0].data = (uint8_t *)value;
    data[0].dataLen = valueLen;
    return AddAppDataEx(reqNode, name, data);  // 2 max count
}

int AppSpawnReqMsgAddStringInfo(AppSpawnReqMsgHandle reqHandle, const char *name, const char *value)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    int ret = CheckInputString("check name", name, APPSPAWN_TLV_NAME_LEN);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    ret = CheckInputString(name, value, EXTRAINFO_TOTAL_LENGTH_MAX);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    APPSPAWN_LOGV("AppSpawnReqMsgAddStringInfo name %{public}s", name);
    AppSpawnAppData data[1] = {};  // 1 max data count
    data[0].data = (uint8_t *)value;
    data[0].dataLen = strlen(value);
    data[0].dataType = DATA_TYPE_STRING;
    return AddAppDataEx(reqNode, name, data);  // 2 max count
}

int AppSpawnReqMsgAddPermission(AppSpawnReqMsgHandle reqHandle, const char *permission)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_CHECK(permission != NULL, return APPSPAWN_ARG_INVALID, "Invalid permission ");
    APPSPAWN_CHECK(reqNode->permissionFlags != NULL, return APPSPAWN_ARG_INVALID, "No permission tlv ");

    int32_t maxIndex = GetMaxPermissionIndex(NULL);
    int index = GetPermissionIndex(NULL, permission);
    APPSPAWN_CHECK(index >= 0 && index < maxIndex,
        return APPSPAWN_PERMISSION_NOT_SUPPORT, "Invalid permission %{public}s", permission);
    APPSPAWN_LOGV("AddPermission index %{public}d name %{public}s", index, permission);
    int ret = SetAppSpawnMsgFlags(reqNode->permissionFlags, index);
    APPSPAWN_CHECK(ret == 0, return ret, "Invalid permission %{public}s", permission);
    return 0;
}

int AppSpawnReqMsgSetAppDomainInfo(AppSpawnReqMsgHandle reqHandle, uint32_t hapFlags, const char *apl)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    int ret = CheckInputString("TLV_DOMAIN_INFO", apl, APP_APL_MAX_LEN);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    AppSpawnMsgDomainInfo msgDomainInfo;
    msgDomainInfo.hapFlags = hapFlags;
    AppSpawnAppData data[MAX_DATA_IN_TLV] = {};
    data[0].data = (uint8_t *)&msgDomainInfo;
    data[0].dataLen = sizeof(AppSpawnMsgDomainInfo);
    data[1].data = (uint8_t *)apl;
    data[1].dataLen = strlen(apl);
    data[1].dataType = DATA_TYPE_STRING;
    return AddAppData(reqNode, TLV_DOMAIN_INFO, data, MAX_DATA_IN_TLV, "TLV_DOMAIN_INFO");
}

int AppSpawnReqMsgSetAppInternetPermissionInfo(AppSpawnReqMsgHandle reqHandle, uint8_t allow, uint8_t setAllow)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);

    AppSpawnMsgInternetInfo info = {};
    info.allowInternet = allow;
    info.setAllowInternet = setAllow;
    AppSpawnAppData data[MAX_DATA_IN_TLV] = {};
    data[0].data = (uint8_t *)&info;
    data[0].dataLen = sizeof(AppSpawnMsgInternetInfo);
    return AddAppData(reqNode, TLV_INTERNET_INFO, data, 1, "TLV_INTERNET_INFO");
}

int AppSpawnReqMsgSetAppOwnerId(AppSpawnReqMsgHandle reqHandle, const char *ownerId)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    int ret = CheckInputString("TLV_OWNER_INFO", ownerId, APP_OWNER_ID_LEN);
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);

    AppSpawnAppData data[MAX_DATA_IN_TLV] = {};
    data[0].data = (uint8_t *)ownerId;
    data[0].dataLen = strlen(ownerId);
    data[0].dataType = DATA_TYPE_STRING;
    return AddAppData(reqNode, TLV_OWNER_INFO, data, 1, "TLV_OWNER_INFO");
}

int AppSpawnReqMsgSetAppAccessToken(AppSpawnReqMsgHandle reqHandle, uint64_t accessTokenIdEx)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);

    AppSpawnAppData data[MAX_DATA_IN_TLV] = {};
    data[0].data = (uint8_t *)&accessTokenIdEx;
    data[0].dataLen = sizeof(accessTokenIdEx);
    return AddAppData(reqNode, TLV_ACCESS_TOKEN_INFO, data, 1, "TLV_ACCESS_TOKEN_INFO");
}

int AppSpawnTerminateMsgCreate(pid_t pid, AppSpawnReqMsgHandle *reqHandle)
{
    APPSPAWN_CHECK(reqHandle != NULL, return APPSPAWN_ARG_INVALID, "Invalid request handle");
    AppSpawnReqMsgNode *reqNode = CreateAppSpawnReqMsg(MSG_GET_RENDER_TERMINATION_STATUS, "terminate-process");
    APPSPAWN_CHECK(reqNode != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to create msg node");

    AppSpawnAppData data[MAX_DATA_IN_TLV] = {};
    data[0].data = (uint8_t *)&pid;
    data[0].dataLen = sizeof(pid);
    int ret = AddAppData(reqNode, TLV_RENDER_TERMINATION_INFO, data, 1, "TLV_RENDER_TERMINATION_INFO");
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, DeleteAppSpawnReqMsg(reqNode);
        return ret);
    *reqHandle = (AppSpawnReqMsgHandle)(reqNode);
    return 0;
}

int AppSpawnClientAddPermission(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle, const char *permission)
{
    AppSpawnReqMsgMgr *reqMgr = (AppSpawnReqMsgMgr *)handle;
    APPSPAWN_CHECK(reqMgr != NULL, return APPSPAWN_ARG_INVALID, "Invalid reqMgr");
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != NULL, return APPSPAWN_ARG_INVALID);
    APPSPAWN_CHECK(permission != NULL, return APPSPAWN_ARG_INVALID, "Invalid permission ");
    APPSPAWN_CHECK(reqNode->permissionFlags != NULL, return APPSPAWN_ARG_INVALID, "No permission tlv ");

    int32_t maxIndex = GetMaxPermissionIndex(handle);
    int index = GetPermissionIndex(handle, permission);
    APPSPAWN_CHECK(index >= 0 && index < maxIndex,
        return APPSPAWN_PERMISSION_NOT_SUPPORT, "Invalid permission %{public}s", permission);
    APPSPAWN_LOGV("add permission index %{public}d name %{public}s", index, permission);
    int ret = SetAppSpawnMsgFlags(reqNode->permissionFlags, index);
    APPSPAWN_CHECK(ret == 0, return ret, "Invalid permission %{public}s", permission);
    return 0;
}
