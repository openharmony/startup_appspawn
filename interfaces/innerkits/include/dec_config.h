/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef DEC_CONFIG_H
#define DEC_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEC_MODE_IGNORE_CASE 0
#define DEC_MODE_NOT_IGNORE_CASE 1

typedef struct DecIgnoreCaseInfo {
    const char *path;
    int mode;
} DecIgnoreCaseInfo;

// Ignore case directory configuration
#define DEC_IGNORE_CASE_LIST \
    {"/storage/Users/currentUser", DEC_MODE_IGNORE_CASE}, \
    {"/storage/Users/currentUser/appdata", DEC_MODE_NOT_IGNORE_CASE}, \
    {"/storage/External", DEC_MODE_IGNORE_CASE}

#define DEC_IGNORE_CASE_LIST_SHAREFS \
    {"/storage/Users/currentUser", DEC_MODE_IGNORE_CASE}, \
    {"/storage/Users/currentUser/appdata", DEC_MODE_NOT_IGNORE_CASE}

// Pick the proper ignore-case list based on NoShareFs state.
// noShareFsEnabled == true -> full list; false -> sharefs list.
// Returns pointer to the selected list and writes its element count to *count.
static inline const DecIgnoreCaseInfo *GetDecIgnoreCaseList(bool noShareFsEnabled, uint32_t *count)
{
    static const DecIgnoreCaseInfo noShareList[] = { DEC_IGNORE_CASE_LIST };
    static const DecIgnoreCaseInfo shareList[] = { DEC_IGNORE_CASE_LIST_SHAREFS };
    if (noShareFsEnabled == true) {
        *count = (uint32_t)(sizeof(noShareList) / sizeof(noShareList[0]));
        return noShareList;
    }
    *count = (uint32_t)(sizeof(shareList) / sizeof(shareList[0]));
    return shareList;
}

#ifdef __cplusplus
}
#endif

#endif // DEC_CONFIG_H
