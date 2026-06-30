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

#ifndef APPSPAWN_SPM_H
#define APPSPAWN_SPM_H

#include <stdint.h>
#include <sys/types.h>
#include "appspawn_manager.h"
#include "appspawn_mount_permission.h"

// JIT Permissions rebuild constants
#define MAX_KERNEL_PERMISSIONS 256  // 最大 kernel permissions 数量
#define MAX_EXT_PERMISSIONS   64    // 最大扩展权限数量

// Permission flags rebuild constants
#define MAX_GRANTED_PERMISSIONS  256  // 最大已授权权限数量

// SPM data allocation constants
#define MAX_PERM_SIZE        8192   // Maximum permission bitmap size (bytes)
#define MAX_EXTEND_PERM_SIZE 2048   // Maximum extended permission size (bytes)
#define MAX_BUNDLE_NAME_LEN  256    // Maximum bundle name length (bytes)

// SPM reference count bitmap flags (for spmRefAdded field)
#define SPM_REF_NONE      0x00  // No refcounts incremented
#define SPM_REF_TOKENID   0x01  // bit 0: tokenid refcount incremented
#define SPM_REF_UID       0x02  // bit 1: uid refcount incremented

// SPM message rebuild error codes
// Error code format: 0x04E1xxxx (APPSPAWN_SPAWNER module)
typedef enum {
    SPM_SUCCESS = 0,                         // Success
    SPM_ERROR_INVALID_PARAM = -1,            // Invalid parameter
    SPM_ERROR_NO_MEMORY = -2,                // Memory allocation failed
    SPM_ERROR_TOKENID_ATTR_MISMATCH = -3,   // TokenidAttr mismatch (security error, must abort)
    SPM_ERROR_INVALID_DATA = -4,             // Invalid SPM data (security error, must abort)
    SPM_ERROR_REF_COUNT_LIMIT = -5,         // Reference count limit exceeded (resource error)
    SPM_ERROR_REF_COUNT_INC_FAILED = -6,    // Reference count increment failed (resource error)
    SPM_ERROR_GET_SPM_FAILED = -7,          // Failed to get SPM data from kernel
    SPM_ERROR_TLV_BUILD_FAILED = -8,        // TLV building failed (processing error)
    SPM_ERROR_MSG_REBUILD_FAILED = -9,      // Message rebuild failed (processing error)
    SPM_ERROR_UID_MISMATCH = -10,
    SPM_ERROR_MISSING_MSG_DATA = -11,
    SPM_ERROR_SPM_NOSUPP = -12,
} SpmErrorCode;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief APL (Ability Privilege Level) enumeration
 * Must be consistent with access_token module
 */
typedef enum {
    APL_INVALID = 0,
    APL_NORMAL = 1,
    APL_SYSTEM_BASIC = 2,
    APL_SYSTEM_CORE = 3,
} AplLevel;

uint32_t GetSpawnId(void);

int CleanupStaleSpawns(void);

int OnMessageRebuildFromSPM(AppSpawnMgr *mgr, AppSpawningCtx *ctx);

int OnSpawnAbortUpdateRefCount(AppSpawnMgr *mgr, AppSpawningCtx *ctx);

int OnAppExitUpdateRefCount(const AppSpawnMgr *mgr, const AppSpawnedProcess *appInfo);

#ifdef __cplusplus
}
#endif

#endif  // SPM_H
