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
#ifndef SANDBOX_LOCK_BUNDLE_TEST_H
#define SANDBOX_LOCK_BUNDLE_TEST_H

#include <cstdint>
#include <map>
#include <string>

// LockBundleInfo structure definition (matches sandbox_shared_mount.cpp)
struct LockBundleInfo {
    uint32_t refCount;
    std::string lockPath;
};

// Global map for managing lock bundle info (exported via APPSPAWN_TEST macro)
extern std::map<std::string, LockBundleInfo> g_lockBundleMap;

// Functions for lock bundle reference management (exported via APPSPAWN_TEST macro)
int AddLockBundleRef(const std::string &lockPath);
void ReleaseLockBundleRef(const std::string &lockPath);

#endif  // SANDBOX_LOCK_BUNDLE_TEST_H
