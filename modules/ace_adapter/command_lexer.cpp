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


#include "appspawn_utils.h"
#include "command_lexer.h"

using namespace OHOS::AppSpawn;

enum class ParsingState {
    INIT,
    IN_WHITESPACE,
    IN_ARGUMENT,
    IN_SINGLE_QUOTE,
    IN_DOUBLE_QUOTE,
};

bool CommandLexer::GetAllArguments(std::vector<std::string> &args)
{
    constexpr char singleQuote = '\'';
    constexpr char doubleQuote = '"';
    ParsingState state = ParsingState::INIT;
    std::string lastArg;

    for (size_t i = 0; i < str_.size(); i++) {
        switch (state) {
            case ParsingState::INIT:
                if (isspace(str_[i])) {
                    state = ParsingState::IN_WHITESPACE;
                } else if (str_[i] == singleQuote) {
                    state = ParsingState::IN_SINGLE_QUOTE;
                } else if (str_[i] == doubleQuote) {
                    state = ParsingState::IN_DOUBLE_QUOTE;
                } else {
                    state = ParsingState::IN_ARGUMENT;
                    lastArg += str_[i];
                }
                break;
            case ParsingState::IN_WHITESPACE:
                if (str_[i] == singleQuote) {
                    state = ParsingState::IN_SINGLE_QUOTE;
                } else if (str_[i] == doubleQuote) {
                    state = ParsingState::IN_DOUBLE_QUOTE;
                } else if (!isspace(str_[i])) {
                    state = ParsingState::IN_ARGUMENT;
                    lastArg += str_[i];
                }
                break;
            case ParsingState::IN_ARGUMENT:
                if (isspace(str_[i])) {
                    args.push_back(std::move(lastArg));
                    // Whether a moved string is empty depends on the
                    // implementation of C++ std library, so clear() is called.
                    lastArg.clear();
                    state = ParsingState::IN_WHITESPACE;
                } else if (str_[i] == singleQuote) {
                    state = ParsingState::IN_SINGLE_QUOTE;
                } else if (str_[i] == doubleQuote) {
                    state = ParsingState::IN_DOUBLE_QUOTE;
                } else {
                    lastArg += str_[i];
                }
                break;
            case ParsingState::IN_SINGLE_QUOTE:
                if (str_[i] == singleQuote) {
                    state = ParsingState::IN_ARGUMENT;
                } else {
                    lastArg += str_[i];
                }
                break;
            case ParsingState::IN_DOUBLE_QUOTE:
                if (str_[i] == doubleQuote) {
                    state = ParsingState::IN_ARGUMENT;
                } else {
                    lastArg += str_[i];
                }
                break;
        }
    }

    if (state == ParsingState::IN_ARGUMENT) {
        args.push_back(std::move(lastArg));
    } else if (state == ParsingState::IN_SINGLE_QUOTE ||
        state == ParsingState::IN_DOUBLE_QUOTE) {
        APPSPAWN_LOGE("Parsing arguments failed: missing terminated quote");
        args.clear();
        return false;
    }

    return true;
}
