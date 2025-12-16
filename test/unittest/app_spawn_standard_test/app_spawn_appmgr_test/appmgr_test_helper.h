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

#ifndef APPMGR_TEST_HELPER_H
#define APPMGR_TEST_HELPER_H

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <unistd.h>
#include <vector>

namespace OHOS {

using AddTlvFunction = std::function<int(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount)>;

class AppMgrTestHelper {
public:
    AppMgrTestHelper()
    {
        AppMgrTestSetDefaultData();
    }
    ~AppMgrTestHelper() {}

    int AppMgrTestCreateSendMsg(std::vector<uint8_t> &buffer, uint32_t msgType, uint32_t &msgLen,
        const std::vector<AddTlvFunction> &addTlvFuncs);
    int AppMgrTestAddBaseTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount);
    int AppMgrTestAddRenderTerminationTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount);
    int AppMgrTestAddExtTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount);
    static void SignalHandle(int sig);

public:
    std::string processName_;
    void AppMgrTestSetDefaultData();
};
}  // namespace OHOS
#endif  // APPMGR_TEST_HELPER_H
