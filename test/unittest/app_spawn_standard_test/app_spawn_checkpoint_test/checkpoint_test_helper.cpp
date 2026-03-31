/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "checkpoint_test_helper.h"

#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

#include "securec.h"
#include "appspawn.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"

namespace OHOS {

// 内联函数：添加单个TLV（参考appmgr_test_helper.cpp中的AddOneTlv）
static int inline CheckpointAddOneTlv(uint8_t *buffer, uint32_t bufferLen, const AppSpawnTlv &tlv, const uint8_t *data)
{
    if (tlv.tlvLen > bufferLen) {
        return -1;
    }
    int ret = memcpy_s(buffer, bufferLen, &tlv, sizeof(tlv));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    ret = memcpy_s(buffer + sizeof(tlv), bufferLen - sizeof(tlv), data, tlv.tlvLen - sizeof(tlv));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    return 0;
}

// 宏：添加TLV（参考APP_MGR_ADD_TLV宏）
#define CHECKPOINT_ADD_TLV(type, value, currLen, tlvCount)  \
do {    \
    AppSpawnTlv d_tlv = {}; \
    d_tlv.tlvType = (type);    \
    d_tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(value);   \
    ret = CheckpointAddOneTlv(buffer + (currLen), bufferLen - (currLen), d_tlv, (uint8_t *)&(value));   \
    APPSPAWN_CHECK(ret == 0, return -1, "Failed add tlv %{public}u", d_tlv.tlvType);  \
    (currLen) += d_tlv.tlvLen;    \
    (tlvCount)++;   \
} while (0)

int CheckpointTestHelper::CheckpointTestAddBaseTlv(uint8_t *buffer, uint32_t bufferLen,
    uint32_t &realLen, uint32_t &tlvCount)
{
    // add app flags (参考AppMgrTestAddBaseTlv)
    uint32_t currLen = 0;
    uint32_t flags[3] = {2, 0b1010, 0};
    AppSpawnTlv tlv = {};
    tlv.tlvType = TLV_MSG_FLAGS;
    tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(flags);
    int ret = CheckpointAddOneTlv(buffer + currLen, bufferLen - currLen, tlv, (uint8_t *)flags);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed add tlv %{public}u", tlv.tlvType);
    currLen += tlv.tlvLen;
    tlvCount++;

    tlv.tlvType = TLV_PERMISSION;
    tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(flags);
    ret = CheckpointAddOneTlv(buffer + currLen, bufferLen - currLen, tlv, (uint8_t *)flags);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed add tlv %{public}u", tlv.tlvType);
    currLen += tlv.tlvLen;
    tlvCount++;

    // add domain info
    typedef struct {
        uint32_t hapFlags;
        char apl[32];  // APP_APL_MAX_LEN
    } CheckpointTestAppDomainInfo;
    CheckpointTestAppDomainInfo domainInfo = {0, "normal"};
    CHECKPOINT_ADD_TLV(TLV_DOMAIN_INFO, domainInfo, currLen, tlvCount);

    // add access token
    AppSpawnMsgAccessToken token = {12345678};
    CHECKPOINT_ADD_TLV(TLV_ACCESS_TOKEN_INFO, token, currLen, tlvCount);

    // add internet info
    AppSpawnMsgInternetInfo internetInfo = {0, 1};
    CHECKPOINT_ADD_TLV(TLV_INTERNET_INFO, internetInfo, currLen, tlvCount);

    // add bundle info
    typedef struct {
        int32_t bundleIndex;
        char bundleName[256];  // APP_LEN_BUNDLE_NAME
    } CheckpointTestAppBundleInfo;
    CheckpointTestAppBundleInfo info = {};
    APPSPAWN_CHECK_ONLY_EXPER(strcpy_s(info.bundleName, sizeof(info.bundleName), "test-bundleName") == 0,
        return -1);
    info.bundleIndex = 100;
    CHECKPOINT_ADD_TLV(TLV_BUNDLE_INFO, info, currLen, tlvCount);

    // add dac info (使用 AppDacInfo)
    AppDacInfo dacInfo = {};
    dacInfo.uid = 20010029;
    dacInfo.gid = 20010029;
    dacInfo.gidCount = 2;
    dacInfo.gidTable[0] = 20010029;
    dacInfo.gidTable[1] = 20010030;
    CHECKPOINT_ADD_TLV(TLV_DAC_INFO, dacInfo, currLen, tlvCount);

    realLen = currLen;
    return 0;
}

int CheckpointTestHelper::CheckpointTestAddCheckpointTlv(uint8_t *buffer, uint32_t bufferLen,
    uint32_t &realLen, uint32_t &tlvCount, const CheckpointMsgParams &params)
{
    uint32_t currLen = 0;
    AppSpawnTlv tlv = {};
    tlv.tlvType = TLV_CHECK_POINT_INFO;
    tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(AppSpawnCheckpointInfo);

    AppSpawnCheckpointInfo checkpointInfo = {};
    checkpointInfo.imgPid = params.imagePid;
    checkpointInfo.checkPointId = params.checkPointId;

    int ret = CheckpointAddOneTlv(buffer + currLen, bufferLen - currLen, tlv, (uint8_t *)&checkpointInfo);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed add tlv %{public}u", tlv.tlvType);
    currLen += tlv.tlvLen;
    tlvCount++;

    realLen = currLen;
    return 0;
}

int CheckpointTestHelper::CreateCheckpointMsg(std::vector<uint8_t> &buffer, uint32_t &msgLen,
    const CheckpointMsgParams &params)
{
    if (params.processName != nullptr) {
        processName_ = std::string(params.processName);
    }

    // 使用 lambda 函数绑定参数
    auto addBaseTlv = [this](uint8_t *buf, uint32_t bufLen, uint32_t &realLen, uint32_t &count) -> int {
        return CheckpointTestAddBaseTlv(buf, bufLen, realLen, count);
    };

    auto addCheckpointTlv = [this, params](uint8_t *buf, uint32_t bufLen,
        uint32_t &realLen, uint32_t &count) -> int {
        return CheckpointTestAddCheckpointTlv(buf, bufLen, realLen, count, params);
    };

    std::vector<AddTlvFunction> addTlvFuncs = {addBaseTlv, addCheckpointTlv};
    return CheckpointTestCreateSendMsg(buffer, params.msgType, msgLen, addTlvFuncs);
}

int CheckpointTestHelper::CheckpointTestCreateSendMsg(std::vector<uint8_t> &buffer, uint32_t msgType,
    uint32_t &msgLen, const std::vector<AddTlvFunction> &addTlvFuncs)
{
    if (buffer.size() < sizeof(AppSpawnMsg)) {
        return -1;
    }
    AppSpawnMsg *msg = reinterpret_cast<AppSpawnMsg *>(buffer.data());
    msg->magic = APPSPAWN_MSG_MAGIC;
    msg->msgType = msgType;
    msg->msgLen = sizeof(AppSpawnMsg);
    msg->msgId = 1;
    msg->tlvCount = 0;
    APPSPAWN_CHECK_ONLY_EXPER(strcpy_s(msg->processName, sizeof(msg->processName), processName_.c_str()) == 0,
        return -1);

    // add tlv (参考AppMgrTestCreateSendMsg)
    uint32_t currLen = sizeof(AppSpawnMsg);
    for (auto addTlvFunc : addTlvFuncs) {
        uint32_t realLen = 0;
        uint32_t tlvCount = 0;
        int ret = addTlvFunc(buffer.data() + currLen, buffer.size() - currLen, realLen, tlvCount);
        APPSPAWN_CHECK(ret == 0 && (currLen + realLen) < buffer.size(),
            return -1, "Failed add tlv to msg %{public}s", processName_.c_str());
        msg->msgLen += realLen;
        currLen += realLen;
        msg->tlvCount += tlvCount;
    }
    msgLen = msg->msgLen;
    APPSPAWN_LOGV("CheckpointTestCreateSendMsg msgLen %{public}d", msgLen);
    return 0;
}

}  // namespace OHOS
