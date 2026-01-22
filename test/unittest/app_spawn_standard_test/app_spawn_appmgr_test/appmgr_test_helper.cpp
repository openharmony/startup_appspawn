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

#include "appmgr_test_helper.h"

#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>

#include "securec.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"

namespace OHOS {
typedef struct {
    int32_t bundleIndex;
    char bundleName[APP_LEN_BUNDLE_NAME];  // bundle name
} AppMgrTestAppBundleInfo;

typedef struct {
    uint32_t hapFlags;
    char apl[APP_APL_MAX_LEN];
} AppMgrTestAppDomainInfo;

static int inline AddOneTlv(uint8_t *buffer, uint32_t bufferLen, const AppSpawnTlv &tlv, const uint8_t *data)
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

#define APP_MGR_ADD_TLV(type, value, currLen, tlvCount)  \
do {    \
    AppSpawnTlv d_tlv = {}; \
    d_tlv.tlvType = (type);    \
    d_tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(value);   \
    ret = AddOneTlv(buffer + (currLen), bufferLen - (currLen), d_tlv, (uint8_t *)&(value));   \
    APPSPAWN_CHECK(ret == 0, return -1, "Failed add tlv %{public}u", d_tlv.tlvType);  \
    (currLen) += d_tlv.tlvLen;    \
    (tlvCount)++;   \
} while (0)

int AppMgrTestHelper::AppMgrTestAddBaseTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount)
{
    // add app flags
    uint32_t currLen = 0;
    uint32_t flags[3] = {2, 0b1010, 0};
    AppSpawnTlv tlv = {};
    tlv.tlvType = TLV_MSG_FLAGS;
    tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(flags);
    int ret = AddOneTlv(buffer + currLen, bufferLen - currLen, tlv, (uint8_t *)flags);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed add tlv %{public}u", tlv.tlvType);
    currLen += tlv.tlvLen;
    tlvCount++;

    tlv.tlvType = TLV_PERMISSION;
    tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(flags);
    ret = AddOneTlv(buffer + currLen, bufferLen - currLen, tlv, (uint8_t *)flags);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed add tlv %{public}u", tlv.tlvType);
    currLen += tlv.tlvLen;
    tlvCount++;

    AppMgrTestAppDomainInfo domainInfo = {0, "normal"};
    APP_MGR_ADD_TLV(TLV_DOMAIN_INFO, domainInfo, currLen, tlvCount);

    AppSpawnMsgAccessToken token = {12345678};  // 12345678
    APP_MGR_ADD_TLV(TLV_ACCESS_TOKEN_INFO, token, currLen, tlvCount);

    AppSpawnMsgInternetInfo internetInfo = {0, 1};
    APP_MGR_ADD_TLV(TLV_INTERNET_INFO, internetInfo, currLen, tlvCount);

    // add bundle info
    AppMgrTestAppBundleInfo info = {};
    APPSPAWN_CHECK_ONLY_EXPER(strcpy_s(info.bundleName, sizeof(info.bundleName), "test-bundleName") == 0,
        return -1);
    info.bundleIndex = 100;  // 100 test index
    APP_MGR_ADD_TLV(TLV_BUNDLE_INFO, info, currLen, tlvCount);

    // add dac
    AppDacInfo  dacInfo = {};
    dacInfo.uid = 20010029; // 20010029 test uid
    dacInfo.gid = 20010029; // 20010029 test gid
    dacInfo.gidCount = 2; // 2 count
    dacInfo.gidTable[0] = 20010029; // 20010029 test gid
    dacInfo.gidTable[1] = 20010030; // 20010030 test gid
    APP_MGR_ADD_TLV(TLV_DAC_INFO, dacInfo, currLen, tlvCount);
    realLen = currLen;
    return 0;
}

int AppMgrTestHelper::AppMgrTestAddRenderTerminationTlv(uint8_t *buffer, uint32_t bufferLen,
                                                        uint32_t &realLen, uint32_t &tlvCount)
{
    // add app flags
    uint32_t currLen = 0;
    AppSpawnTlv tlv = {};
    tlv.tlvType = TLV_RENDER_TERMINATION_INFO;
    pid_t pid = 9999999; // 9999999 test
    tlv.tlvLen = sizeof(AppSpawnTlv) + sizeof(pid);

    int ret = memcpy_s(buffer, bufferLen, &tlv, sizeof(tlv));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    ret = memcpy_s(buffer + sizeof(tlv), bufferLen - sizeof(tlv), &pid, tlv.tlvLen - sizeof(tlv));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    currLen += tlv.tlvLen;
    tlvCount++;
    realLen = currLen;
    return 0;
}

int AppMgrTestHelper::AppMgrTestAddExtTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount)
{
    const char *testData = "555555555555555555555555555";
    uint32_t currLen = 0;
    AppSpawnTlvExt tlv = {};
    tlv.tlvType = TLV_MAX;
    int ret = strcpy_s(tlv.tlvName, sizeof(tlv.tlvName), "test-001");
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to strcpy");
    tlv.dataLen = strlen(testData) + 1;
    tlv.tlvLen = sizeof(AppSpawnTlvExt) + APPSPAWN_ALIGN(tlv.dataLen);

    ret = memcpy_s(buffer, bufferLen, &tlv, sizeof(tlv));
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    ret = memcpy_s(buffer + sizeof(tlv), bufferLen - sizeof(tlv), testData, tlv.dataLen + 1);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to memcpy_s bufferSize");
    currLen += tlv.tlvLen;
    tlvCount++;
    realLen = currLen;
    return 0;
}

void AppMgrTestHelper::AppMgrTestSetDefaultData()
{
    processName_ = std::string("com.example.myapplication");
}

void AppMgrTestHelper::SignalHandle(int sig)
{
    std::cout<<"signal is: "<<sig<<std::endl;
}

int AppMgrTestHelper::AppMgrTestCreateSendMsg(std::vector<uint8_t> &buffer, uint32_t msgType, uint32_t &msgLen,
                                              const std::vector<AddTlvFunction> &addTlvFuncs)
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
    // add tlv
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
    APPSPAWN_LOGV("AppMgrTestCreateSendMsg msgLen %{public}d", msgLen);
    return 0;
}
}  // namespace OHOS
