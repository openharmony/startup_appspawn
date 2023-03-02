/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef APPSPAWN_TEST_STUB_H
#define APPSPAWN_TEST_STUB_H

#include <iostream>

struct HapDomainInfo {
    std::string apl;
    std::string packageName;
    unsigned int hapFlags = 1;
};

class HapContextStub {
public:
    HapContextStub();
    ~HapContextStub();
    int HapDomainSetcontext(HapDomainInfo& hapDomainInfo);
};
#ifdef __cplusplus
extern "C" {
#endif
pid_t GetTestPid(void);
void SetHapDomainSetcontextResult(int result);
#ifdef __cplusplus
}
#endif
#endif // APPSPAWN_TEST_STUB_H
