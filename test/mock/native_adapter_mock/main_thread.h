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

/* Mock OHOS::AppExecFwk namespace */

#ifndef STUB_MAIN_THREAD_H
#define STUB_MAIN_THREAD_H

#include <map>
#include <string>

namespace OHOS {
namespace AppExecFwk {
    class MainThread {
    public:
        MainThread() {}
        ~MainThread() {}

        static void Start()
        {
            return;
        }

        static void StartChild(std::map<std::string, int> &map)
        {
            return;
        }
    };
}  // namespace AppExecFwk

}  // namespace OHOS
#endif  // STUB_MAIN_THREAD_H
