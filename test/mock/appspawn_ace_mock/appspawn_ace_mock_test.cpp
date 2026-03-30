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

#include "appspawn_ace_mock_test.h"

#include <string>

#include "appspawn_utils.h"
#include "parameter.h"
#include "securec.h"

static bool g_preloadParamResult = false;
static bool g_preloadEtsParamResult = false;
static bool g_spawnUnifiedParamResult = false;
void SetBoolParamResult(const char *key, bool flag)
{
    if (strcmp(key, "persist.appspawn.preload") == 0) {
        g_preloadParamResult = flag;
    } else if (strcmp(key, "persist.appspawn.preloadets") == 0) {
        g_preloadEtsParamResult = flag;
    } else if (strcmp(key, "persist.appspawn.hybridspawn.unified") == 0) {
        g_spawnUnifiedParamResult = flag;
    }
}

static bool g_startupPrelinkExist = false;
static bool g_startupPrelinkEnable = true;
int SetParameter(const char *key, const char *value)
{
    if (strcmp(key, "const.startup.prelink.enable") == 0) {
        g_startupPrelinkExist = true;
        g_startupPrelinkEnable = strcmp(value, "true") == 0 ? true : false;
    }

    return 0;
}

namespace OHOS {
namespace system {
    bool GetBoolParameter(const std::string &key, bool def)
    {
        if (strcmp(key.c_str(), "persist.appspawn.preload") == 0) {
            return g_preloadParamResult;
        } else if (strcmp(key.c_str(), "persist.appspawn.preloadets") == 0) {
            return g_preloadEtsParamResult;
        } else if (strcmp(key.c_str(), "persist.appspawn.hybridspawn.unified") == 0) {
            return g_spawnUnifiedParamResult;
        }
        return def;
    }
}  // namespace system
}  // namespace OHOS

static int GetParameterForPrelink(char *value, uint32_t len)
{
    if (!g_startupPrelinkExist) {
        return -1;
    }

    if (g_startupPrelinkEnable) {
        return strcpy_s(value, len, "true") == 0 ? strlen("true") : -1;
    }

    return strcpy_s(value, len, "false") == 0 ? strlen("false") : -1;
}

int GetParameter(const char *key, const char *def, char *value, uint32_t len)
{
    if (strcmp(key, "const.startup.prelink.enable") == 0) {
        return GetParameterForPrelink(value, len);
    }

    return -1;
}

static bool g_mockDlprelinkReserveMemFailed   = false;
static bool g_mockDlprelinkRecordFailed       = false;
static bool g_isExecutedDlprelinkRegister     = false;
void SetMockDlprelinkReserveMemFailed(bool v)
{
    g_mockDlprelinkReserveMemFailed = v;
}

void SetMockDlprelinkRecordFailed(bool v)
{
    g_mockDlprelinkRecordFailed = v;
}

bool GetIsExecutedDlprelinkRegister(void)
{
    return g_isExecutedDlprelinkRegister;
}

void ClearIsExecutedDlprelinkRegister(void)
{
    g_isExecutedDlprelinkRegister = false;
}

int dlprelink_reserve_mem(void)
{
    return g_mockDlprelinkReserveMemFailed ? -1 : 0;
}

int dlprelink_record(int memfd, const char *listPath)
{
    return g_mockDlprelinkRecordFailed ? -1 : 0;
}

int dlprelink_register(int fd)
{
    g_isExecutedDlprelinkRegister = true;
    return 0;
}
