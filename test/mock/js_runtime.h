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

#ifndef STUB_JS_RUNTIME_H
#define STUB_JS_RUNTIME_H

#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace OHOS {
namespace AbilityRuntime {
    class Runtime {
    public:
        enum class Language {
            JS = 0,
        };

        struct Options {
            Language lang = Language::JS;
            std::string bundleName;
            std::string moduleName;
            bool loadAce = true;
            bool preload = false;
            bool isBundle = true;
            bool isDebugVersion = false;
            bool isJsFramework = false;
            bool isStageModel = true;
            bool isTestFramework = false;
            int32_t uid = -1;
            // ArkTsCard start
            bool isUnique = false;
        };

        static std::unique_ptr<Runtime> Create(const Options &options)
        {
            return std::make_unique<Runtime>();
        }
        static void SavePreloaded(std::unique_ptr<Runtime> &&instance) {}

        Runtime() {};
        ~Runtime() {};

        void PreloadSystemModule(const std::string &moduleName)
        {
            return;
        }

        Runtime(const Runtime &) = delete;
        Runtime(Runtime &&) = delete;
        Runtime &operator=(const Runtime &) = delete;
        Runtime &operator=(Runtime &&) = delete;
    };
}  // namespace AbilityRuntime
namespace Ace {
    class AceForwardCompatibility {
    public:
        AceForwardCompatibility() {}
        ~AceForwardCompatibility() {}

        static const char *GetAceLibName()
        {
            return "";
        }
    };
}  // namespace Ace

namespace AppExecFwk {
    class MainThread {
    public:
        MainThread() {}
        ~MainThread() {}

        static void PreloadExtensionPlugin() {}
        static void Start() {}
    };
}  // namespace AppExecFwk

}  // namespace OHOS
#endif  // STUB_JS_RUNTIME_H