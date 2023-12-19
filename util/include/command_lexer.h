/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef COMMAND_LEXER_H
#define COMMAND_LEXER_H

#include <string>
#include <vector>

namespace OHOS {
namespace AppSpawn {

class CommandLexer {
public:
    explicit CommandLexer(const std::string &str) : str_(str) {}
    // Disable copy and move semantics to avoid extension of the lifecyle of the
    // resource not owned by this class.
    CommandLexer(const CommandLexer &other) = delete;
    CommandLexer(CommandLexer &&other) = delete;
    CommandLexer &operator=(const CommandLexer &other) = delete;
    CommandLexer &operator=(CommandLexer &&other) = delete;
    // Return true, and args will be populated with all argument. Otherwise,
    // false is returned, and args will be cleared.
    bool GetAllArguments(std::vector<std::string> &args);

private:
    const std::string &str_;
};

} // namespace AppSpawn
} // namespace OHOS

#endif // COMMAND_LEXER_H
