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

#include <cstdarg>
#include <gtest/gtest.h>

#include "command_lexer.h"
#include "third_party/externals/harfbuzz/src/hb-unicode.hh"

#define SPACE_STR               " "
#define SINGLE_QUOTE_STR        "'"
#define DOUBLE_QUOTE_STR        "\""

using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace AppSpawn {
namespace {

class CommandLexerTest : public Test {
protected:
    static std::string WrapWith(const char *s, const char *appendage)
    {
        std::string wrappedStr;
        wrappedStr += appendage;
        wrappedStr += s;
        wrappedStr += appendage;
        return wrappedStr;
    }

    static std::string Join(const char *separator, ...)
    {
        bool firstArg = true;
        va_list args;
        va_start(args, separator);
        const char *arg = nullptr;
        std::string joinedStr;

        while ((arg = va_arg(args, const char*))) {
            if (!firstArg) {
                joinedStr += separator;
            } else {
                firstArg = false;
            }
            joinedStr += arg;
        }

        va_end(args);
        return joinedStr;
    }

    std::vector<std::string> args_ {};
};

HWTEST_F(CommandLexerTest, GetAllArguments, TestSize.Level0)
{
    const char *filePath = "/data/local/tmp/lldb-server";
    const char *serverMode = "platform";
    const char *listenOpt = "--listen";
    const char *connUrl = "unix-abstract:///lldb/platform.sock";
    std::string command = Join(SPACE_STR,
        filePath, serverMode, listenOpt, connUrl, nullptr);
    CommandLexer lexer(command);

    ASSERT_TRUE(lexer.GetAllArguments(args_));
    ASSERT_FALSE(args_.empty());
    ASSERT_EQ(args_.size(), (size_t) 4);
    ASSERT_EQ(args_[0], filePath);
    ASSERT_EQ(args_[1], serverMode);
    ASSERT_EQ(args_[2], listenOpt);
    ASSERT_EQ(args_[3], connUrl);
}

HWTEST_F(CommandLexerTest, GetAllArgumentsWithSingleQuote, TestSize.Level0)
{
    const char *fileName = "echo";
    const char *message = " aaa ";
    std::string command = Join(SPACE_STR,
        fileName, WrapWith(message, SINGLE_QUOTE_STR).c_str(), nullptr);
    CommandLexer lexer(command);

    ASSERT_TRUE(lexer.GetAllArguments(args_));
    ASSERT_FALSE(args_.empty());
    ASSERT_EQ(args_.size(), (size_t) 2);
    ASSERT_EQ(args_[0], fileName);
    ASSERT_EQ(args_[1], message);
}

HWTEST_F(CommandLexerTest, GetAllArgumentsWithDoubleQuote, TestSize.Level0)
{
    const char *filePath = "/data/local/tmp/lldb-server";
    const char *logOpt = "--log-channel";
    const char *logChannels = "lldb process thread:gdb-remote packets";
    std::string command = Join(SPACE_STR,
        filePath, logOpt, WrapWith(logChannels, DOUBLE_QUOTE_STR).c_str(),
        nullptr);
    CommandLexer lexer(command);

    ASSERT_TRUE(lexer.GetAllArguments(args_));
    ASSERT_FALSE(args_.empty());
    ASSERT_EQ(args_.size(), (size_t) 3);
    ASSERT_EQ(args_[0], filePath);
    ASSERT_EQ(args_[1], logOpt);
    ASSERT_EQ(args_[2], logChannels);
}

HWTEST_F(CommandLexerTest, GetAllArgumentsFromEmptyStr, TestSize.Level0)
{
    std::string command;
    CommandLexer lexer(command);

    ASSERT_TRUE(lexer.GetAllArguments(args_));
    ASSERT_TRUE(args_.empty());
}

HWTEST_F(CommandLexerTest, GetAllArgumentsFromSpaces, TestSize.Level0)
{
    std::string command = "   ";
    CommandLexer lexer(command);

    ASSERT_TRUE(lexer.GetAllArguments(args_));
    ASSERT_TRUE(args_.empty());
}

HWTEST_F(CommandLexerTest, GetAllArgumentsWithMissingQuote, TestSize.Level0)
{
    const char *fileName = "echo";
    const char *message = DOUBLE_QUOTE_STR " aa";
    std::string command = Join(SPACE_STR, fileName, message, nullptr);
    CommandLexer lexer(command);

    ASSERT_FALSE(lexer.GetAllArguments(args_));
    ASSERT_TRUE(args_.empty());
}

HWTEST_F(CommandLexerTest, GetAllArgumentsWithNonLeadingQuote, TestSize.Level0)
{
    const char *fileName = "mkdir";
    const char *opt = "-p";
    const char *dirName = "lldb-" DOUBLE_QUOTE_STR "server" DOUBLE_QUOTE_STR;
    std::string command = Join(SPACE_STR, fileName, opt, dirName, nullptr);
    CommandLexer lexer(command);

    ASSERT_TRUE(lexer.GetAllArguments(args_));
    ASSERT_FALSE(args_.empty());
    ASSERT_EQ(args_.size(), (size_t) 3);
    ASSERT_EQ(args_[0], fileName);
    ASSERT_EQ(args_[1], opt);
    ASSERT_EQ(args_[2], "lldb-server");
}

} // namespace
} // namespace AppSpawn
} // namespace OHOS
