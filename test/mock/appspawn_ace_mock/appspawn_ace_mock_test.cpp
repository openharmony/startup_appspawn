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

static uint32_t g_preloadParamResult = 0;
static uint32_t g_preloadEtsParamResult = 0;
static uint32_t g_spawnUnifiedParamResult = 0;

void SetBoolParamResult(const char *key, bool flag)
{
    if (strcmp(key, "persist.appspawn.preload") == 0) {
        flag ? (g_preloadParamResult = true) : (g_preloadParamResult = false);
    }
    if (strcmp(key, "persist.appspawn.preloadets") == 0) {
        flag ? (g_preloadEtsParamResult = true) : (g_preloadEtsParamResult = false);
    }
    if (strcmp(key, "persist.appspawn.hybridspawn.unified") == 0) {
        flag ? (g_spawnUnifiedParamResult = true) : (g_spawnUnifiedParamResult = false);
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
