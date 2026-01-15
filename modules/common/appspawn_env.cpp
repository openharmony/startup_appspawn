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


#include <string>
#include <fstream>
#include <climits>

#include "securec.h"
#include "appspawn_utils.h"
#include "appspawn_msg.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"

constexpr const char *ENV_FILE_PATH = "/etc/environment";
constexpr size_t MIN_QUOTED_VALUE_LENGTH = 2; // Minimum length for a quoted value (e.g., "x")
constexpr size_t QUOTE_LENGTH = 1; // Length of a single quote character
constexpr size_t TWO_QUOTE_LENGTH = 2 * QUOTE_LENGTH;
constexpr size_t MAX_ENV_LINE_LENGTH = 4096; // Maximum length for a single line
constexpr size_t MAX_ENV_FILE_LINES = 1000; // Maximum number of lines in the file

APPSPAWN_STATIC std::string_view TrimWhitespaceView(std::string_view str)
{
    const char *ws = " \t\n\r";
    auto start = str.find_first_not_of(ws);
    if (start == std::string::npos) {
        return std::string_view();
    }
    auto end = str.find_last_not_of(ws);
    return std::string_view(str.data() + start, end - start + 1);
}

APPSPAWN_STATIC int32_t ParseEnvLine(const std::string &line, std::string &envName, std::string &envValue)
{
    if (line.length() > MAX_ENV_LINE_LENGTH) {
        APPSPAWN_LOGW("Environment line too long: %{public}zu bytes (max: %{public}zu)",
            line.length(), MAX_ENV_LINE_LENGTH);
        return APPSPAWN_ENV_FILE_FORMAT_ERROR;
    }

    std::string_view lineView(line);
    lineView = TrimWhitespaceView(lineView);
    if (lineView.empty() || lineView[0] == '#') {
        return APPSPAWN_ENV_FILE_EMPTY;
    }

    size_t equalsPos = lineView.find('=');
    if (equalsPos == std::string_view::npos) {
        APPSPAWN_LOGW("Invalid environment variable format: %{public}s (missing '=')", line.c_str());
        return APPSPAWN_ENV_FILE_FORMAT_ERROR;
    }

    std::string_view rawName = lineView.substr(0, equalsPos);
    envName = std::string(TrimWhitespaceView(rawName));
    if (envName.empty()) {
        APPSPAWN_LOGW("Invalid environment variable: empty name");
        return APPSPAWN_ENV_FILE_FORMAT_ERROR;
    }
    std::string_view rawValue = lineView.substr(equalsPos + 1);
    std::string trimmedValue = std::string(TrimWhitespaceView(rawValue));
    size_t valueLen = trimmedValue.length();
    if (valueLen >= MIN_QUOTED_VALUE_LENGTH && (trimmedValue[0] == '"' || trimmedValue[0] == '\'') &&
        trimmedValue[valueLen - 1] == trimmedValue[0]) {
        envValue = trimmedValue.substr(QUOTE_LENGTH, valueLen - TWO_QUOTE_LENGTH);
    } else {
        envValue = trimmedValue;
    }
    return APPSPAWN_OK;
}

APPSPAWN_STATIC int32_t LoadCustomSandboxEnv(const char* envFilePath)
{
    APPSPAWN_CHECK(envFilePath != nullptr, return APPSPAWN_ARG_INVALID, "envFilePath is nullptr.");
    char normalizedPath[PATH_MAX] = {0};
    int savedErrno = 0;
    char *tmpPath = realpath(envFilePath, normalizedPath);
    savedErrno = (tmpPath == nullptr) ? errno : 0;
    APPSPAWN_CHECK(savedErrno != ENOENT, return APPSPAWN_OK, "Environment file %{public}s does not exist.",
        envFilePath);
    APPSPAWN_CHECK(tmpPath, return APPSPAWN_ARG_INVALID, "Failed to normalize path %{public}s, errno: %{public}d",
        envFilePath, savedErrno);

    std::ifstream file(normalizedPath);
    bool isOpen = file.is_open();
    savedErrno = (isOpen == false) ? errno : 0;
    APPSPAWN_CHECK(savedErrno != ENOENT, return APPSPAWN_OK, "Environment file %{public}s does not exist.",
        normalizedPath);
    APPSPAWN_CHECK(isOpen, return APPSPAWN_ENV_FILE_READ_ERROR, "Failed to open %{public}s, errno: %{public}d",
        normalizedPath, savedErrno);

    std::string line;
    std::string envName;
    std::string envValue;
    size_t lineCount = 0;
    while (std::getline(file, line)) {
        if (++lineCount > MAX_ENV_FILE_LINES) {
            APPSPAWN_LOGE("Environment file exceeds maximum line count: %{public}zu (max: %{public}zu)",
                lineCount, MAX_ENV_FILE_LINES);
            return APPSPAWN_ENV_FILE_READ_ERROR;
        }

        int32_t parseRet = ParseEnvLine(line, envName, envValue);
        if (parseRet != APPSPAWN_OK) {
            continue;
        }

        // Set environment variable
        int ret = setenv(envName.c_str(), envValue.c_str(), 1);
        APPSPAWN_CHECK_LOGV(ret == 0, continue, "Failed to setenv %{public}s=%{public}s, errno: %{public}d",
            envName.c_str(), envValue.c_str(), errno);
    }

    APPSPAWN_CHECK(!(file.fail() && !file.eof()), return APPSPAWN_ENV_FILE_READ_ERROR,
        "Error reading %{public}s, errno: %{public}d", normalizedPath, errno);
    return APPSPAWN_OK;
}

APPSPAWN_STATIC int SpawnSetCustomSandboxEnv(AppSpawnMgr *content, AppSpawningCtx *property)
{
    APPSPAWN_LOGV("Spawning: set SpawnSetCustomSandboxEnv.");
    if (CheckAppMsgFlagsSet(property, APP_FLAGS_CUSTOM_SANDBOX)) {
        int32_t ret = LoadCustomSandboxEnv(ENV_FILE_PATH);
        APPSPAWN_LOGV("Finished LoadCustomSandboxEnv result: %{public}d", ret);
    }
    return APPSPAWN_OK;
}

MODULE_CONSTRUCTOR(void)
{
    const int32_t priority = HOOK_PRIO_COMMON + 2; // after SpawnSetAppEnv
    AddAppSpawnHook(STAGE_CHILD_PRE_COLDBOOT, priority, SpawnSetCustomSandboxEnv);
}
