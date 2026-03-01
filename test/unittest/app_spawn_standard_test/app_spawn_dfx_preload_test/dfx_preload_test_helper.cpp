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

#include "dfx_preload_test_helper.h"

#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

#include "securec.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn_client.h"

namespace OHOS {

static const int32_t DEFAULT_TEST_UID = 20010029;
static const int32_t DEFAULT_TEST_GID = 20010029;
static const int32_t DEFAULT_TEST_GID_GROUP = 20010029;
static const int32_t DEFAULT_TEST_GID_COUNT = 2;

void DfxPreloadTestHelper::DfxPreloadTestSetDefaultData(void)
{
    processName_ = std::string("com.example.myapplication");
    defaultTestUid_ = DEFAULT_TEST_UID;
    defaultTestGid_ = DEFAULT_TEST_GID;
    defaultTestGidGroup_ = DEFAULT_TEST_GID_GROUP;
    defaultTestBundleIndex_ = 1;
    defaultApl_ = std::string("system_core");
    defaultMsgFlags_ = 0;
}

int DfxPreloadTestHelper::DfxPreloadTestAddDacInfo(AppSpawnReqMsgHandle &reqHandle)
{
    AppDacInfo dacInfo = {};
    dacInfo.uid = defaultTestUid_;
    dacInfo.gid = defaultTestGid_;
    dacInfo.gidCount = DEFAULT_TEST_GID_COUNT;
    dacInfo.gidTable[0] = defaultTestGidGroup_;
    dacInfo.gidTable[1] = defaultTestGidGroup_ + 1;
    APPSPAWN_CHECK_ONLY_EXPER(strcpy_s(dacInfo.userName, sizeof(dacInfo.userName), "test-app-name") == 0,
        return APPSPAWN_ARG_INVALID);
    return AppSpawnReqMsgSetAppDacInfo(reqHandle, &dacInfo);
}

int DfxPreloadTestHelper::DfxPreloadTestAddFdInfo(AppSpawnReqMsgHandle &reqHandle)
{
    int fdArg = open("/dev/random", O_RDONLY);
    APPSPAWN_CHECK(fdArg >= 0, return -1, "open fd failed ");
    APPSPAWN_LOGV("Add fd info %{public}d", fdArg);
    return AppSpawnReqMsgAddFd(reqHandle, "fdname", fdArg);
}

int DfxPreloadTestHelper::DfxPreloadTestSetMsgFlags(AppSpawnReqMsgHandle reqHandle, uint32_t tlv, uint32_t flags)
{
    AppSpawnReqMsgNode *reqNode = (AppSpawnReqMsgNode *)reqHandle;
    APPSPAWN_CHECK_ONLY_EXPER(reqNode != nullptr, return APPSPAWN_ARG_INVALID);
    if (tlv == TLV_MSG_FLAGS) {
        *(uint32_t *)reqNode->msgFlags->flags = flags;
    } else if (tlv == TLV_PERMISSION) {
        *(uint32_t *)reqNode->permissionFlags->flags = flags;
    }
    return 0;
}

AppSpawnMsgNode *DfxPreloadTestHelper::DfxPreloadTestCreateAppSpawnMsg(AppSpawnMsg *msg)
{
    AppSpawnMsgNode *msgNode = static_cast<AppSpawnMsgNode *>(calloc(1, sizeof(AppSpawnMsgNode)));
    APPSPAWN_CHECK(msgNode != nullptr, return nullptr, "Failed to create receiver");
    int ret = memcpy_s(&msgNode->msgHeader, sizeof(msgNode->msgHeader), msg, sizeof(msgNode->msgHeader));
    if (ret != 0) {
        APPSPAWN_LOGE("Failed to memcpy msg");
        free(msgNode);
        return nullptr;
    }
    msgNode->buffer = static_cast<uint8_t *>(malloc(msg->msgLen));
    APPSPAWN_CHECK(msgNode->buffer != nullptr, free(msgNode);
        return nullptr, "Failed to memcpy msg");
    uint32_t totalCount = msg->tlvCount + TLV_MAX;
    msgNode->tlvOffset = static_cast<uint32_t *>(malloc(totalCount * sizeof(uint32_t)));
    APPSPAWN_CHECK(msgNode->tlvOffset != nullptr, free(msgNode);
        return nullptr, "Failed to alloc memory for recv message");
    for (uint32_t i = 0; i < totalCount; i++) {
        msgNode->tlvOffset[i] = INVALID_OFFSET;
    }
    return msgNode;
}

AppSpawnReqMsgHandle DfxPreloadTestHelper::DfxPreloadTestCreateMsg(AppSpawnClientHandle handle, uint32_t msgType)
{
    AppSpawnReqMsgHandle reqHandle = 0;
    int ret = AppSpawnReqMsgCreate(static_cast<AppSpawnMsgType>(msgType), processName_.c_str(), &reqHandle);
    APPSPAWN_CHECK(ret == 0, return INVALID_REQ_HANDLE, "Failed to create req %{public}s",
                   processName_.c_str());
    APPSPAWN_CHECK_ONLY_EXPER(msgType == MSG_APP_SPAWN || msgType == MSG_SPAWN_NATIVE_PROCESS, return reqHandle);
    do {
        ret = DfxPreloadTestAddFdInfo(reqHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add fd %{public}s", processName_.c_str());
        ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 100, processName_.c_str());  // 100 test index
        APPSPAWN_CHECK(ret == 0, break, "Failed to add bundle info req %{public}s", processName_.c_str());
        ret = DfxPreloadTestAddDacInfo(reqHandle);
        APPSPAWN_CHECK(ret == 0, break, "Failed to add dac %{public}s", processName_.c_str());
        ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);  // 12345678
        APPSPAWN_CHECK(ret == 0, break, "Failed to add access token %{public}s", processName_.c_str());

        if (defaultMsgFlags_ != 0) {
            (void)DfxPreloadTestSetMsgFlags(reqHandle, TLV_MSG_FLAGS, defaultMsgFlags_);
        }

        const char *testData = "ssssssssssssss sssssssss ssssssss";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, "tlv-name-1",
            reinterpret_cast<uint8_t *>(const_cast<char *>(testData)), strlen(testData));
        APPSPAWN_CHECK(ret == 0, break, "Failed to ext tlv %{public}s", processName_.c_str());
        size_t count = permissions_.size();
        for (size_t i = 0; i < count; i++) {
            ret = AppSpawnReqMsgAddPermission(reqHandle, permissions_[i]);
            APPSPAWN_CHECK(ret == 0, break, "Failed to permission %{public}s", permissions_[i]);
        }

        ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 1, 0);
        APPSPAWN_CHECK(ret == 0, break, "Failed to internet info %{public}s", processName_.c_str());

        ret = AppSpawnReqMsgSetAppOwnerId(reqHandle, "ohos.permission.FILE_ACCESS_MANAGER");
        APPSPAWN_CHECK(ret == 0, break, "Failed to ownerid %{public}s", processName_.c_str());
        const char *renderCmd = "/system/bin/sh ls -l ";
        ret = AppSpawnReqMsgAddExtInfo(reqHandle, MSG_EXT_NAME_RENDER_CMD,
            reinterpret_cast<const uint8_t *>(renderCmd), strlen(renderCmd));
        APPSPAWN_CHECK(ret == 0, break, "Failed to render cmd %{public}s", processName_.c_str());
        ret = AppSpawnReqMsgSetAppDomainInfo(reqHandle, 1, defaultApl_.c_str());
        APPSPAWN_CHECK(ret == 0, break, "Failed to domain info %{public}s", processName_.c_str());
        return reqHandle;
    } while (0);
    AppSpawnReqMsgFree(reqHandle);
    return INVALID_REQ_HANDLE;
}

AppSpawningCtx *DfxPreloadTestHelper::DfxPreloadTestGetAppProperty(AppSpawnClientHandle handle,
    AppSpawnReqMsgHandle reqHandle)
{
    AppSpawnReqMsgNode *reqNode = static_cast<AppSpawnReqMsgNode *>(reqHandle);
    APPSPAWN_CHECK(reqNode != nullptr && reqNode->msg != nullptr, AppSpawnReqMsgFree(reqHandle);
        return nullptr, "Invalid reqNode");

    AppSpawnMsgNode *msgNode = DfxPreloadTestCreateAppSpawnMsg(reqNode->msg);
    APPSPAWN_CHECK(msgNode != nullptr, return nullptr, "Failed to alloc for msg");

    uint32_t bufferSize = reqNode->msg->msgLen;
    uint32_t currIndex = 0;
    uint32_t bufferStart = sizeof(AppSpawnMsg);
    ListNode *node = reqNode->msgBlocks.next;
    while (node != &reqNode->msgBlocks) {
        AppSpawnMsgBlock *block = ListEntry(node, AppSpawnMsgBlock, node);
        int ret = memcpy_s(msgNode->buffer + currIndex, bufferSize - currIndex,
            block->buffer + bufferStart, block->currentIndex - bufferStart);
        if (ret != 0) {
            AppSpawnReqMsgFree(reqHandle);
            DeleteAppSpawnMsg(&msgNode);
            return nullptr;
        }
        currIndex += block->currentIndex - bufferStart;
        bufferStart = 0;
        node = node->next;
    }
    APPSPAWN_LOGV("GetAppProperty header magic 0x%{public}x type %{public}u id %{public}u len %{public}u %{public}s",
        msgNode->msgHeader.magic, msgNode->msgHeader.msgType,
        msgNode->msgHeader.msgId, msgNode->msgHeader.msgLen, msgNode->msgHeader.processName);

    // delete reqHandle
    AppSpawnReqMsgFree(reqHandle);
    int ret = DecodeAppSpawnMsg(msgNode);
    APPSPAWN_CHECK(ret == 0, DeleteAppSpawnMsg(&msgNode);
        return nullptr, "Decode msg fail");
    AppSpawningCtx *property = CreateAppSpawningCtx();
    APPSPAWN_CHECK_ONLY_EXPER(property != nullptr, DeleteAppSpawnMsg(&msgNode);
        return nullptr);
    property->message = msgNode;
    return property;
}
}  // namespace OHOS
