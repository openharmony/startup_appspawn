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
#ifdef APPSPAWN_SUPPORT_NOSHAREFS
#define DEC_IGNORE_CASE_LIST \
    {"/storage/Users/currentUser", DEC_MODE_IGNORE_CASE}, \
    {"/storage/Users/currentUser/appdata", DEC_MODE_NOT_IGNORE_CASE}
#else
#define DEC_IGNORE_CASE_LIST
#endif
#ifdef __cplusplus
}
#endif

#endif // DEC_CONFIG_H
