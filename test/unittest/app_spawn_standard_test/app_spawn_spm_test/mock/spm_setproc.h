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

#ifndef SPM_SETPROC_MOCK_H
#define SPM_SETPROC_MOCK_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Buffer descriptor used to pass variable-length data to/from kernel SPM ioctls.
 */
typedef struct {
    char *buf;        /**< Pointer to the data buffer. */
    uint32_t bufSize; /**< Size of buf in bytes. */
} SpmBlob;

/**
 * @brief Describes a single SPM (Signed Permission Monitor) entry.
 */
typedef struct {
    uint32_t uid;
    uint32_t tokenid;
    uint32_t tokenidAttr;
    uint32_t index;
    uint16_t apl;
    uint16_t distributionType;
    uint32_t idType;
    uint64_t ownerid;
    SpmBlob name;
    SpmBlob perms;
    SpmBlob extendPerms;
} SpmData;

/**
 * @brief Parsed view of one external permission item in extendPerms.
 */
typedef struct {
    uint32_t code; /**< Permission opcode. */
    char *value;   /**< Pointer to the value bytes of this permission item. */
} PermsWithValue;

// SPM reference count bitmap flags (for spmRefAdded field)
#define SPM_REF_NONE      0x00  // No refcounts incremented
#define SPM_REF_TOKENID   0x01  // bit 0: tokenid refcount incremented
#define SPM_REF_UID       0x02  // bit 1: uid refcount incremented

// SPM data allocation constants
#define MAX_PERM_SIZE        8192   // Maximum permission bitmap size (bytes)
#define MAX_EXTEND_PERM_SIZE 2048   // Maximum extended permission size (bytes)
#define MAX_BUNDLE_NAME_LEN  256    // Maximum bundle name length (bytes)

/**
 * @brief Allocate and zero-init SpmData with SpmBlob buffers.
 */
SpmData* SpmDataNew(uint32_t permBufSize, uint32_t extendPermBufSize, uint32_t nameBufSize);

/**
 * @brief Free SpmData allocated by SpmDataNew.
 */
void SpmDataFree(SpmData *data);

/**
 * @brief Query SPM entry by tokenid.
 */
int SpmGetEntry(uint32_t tokenid, SpmData *entry);

/**
 * @brief Increment uid active process refcount.
 */
int SpmIncUidRefCnt(uint32_t uid, uint32_t spawnid);

/**
 * @brief Decrement uid active process refcount.
 */
int SpmDecUidRefCnt(uint32_t uid, uint32_t spawnid);

/**
 * @brief Get uid active process refcount.
 */
int SpmGetUidRefCnt(uint32_t uid, uint64_t *refcnt);

/**
 * @brief Increment tokenid active process refcount.
 */
int SpmIncTokenidRefCnt(uint32_t tokenid, uint32_t spawnid);

/**
 * @brief Decrement tokenid active process refcount.
 */
int SpmDecTokenidRefCnt(uint32_t tokenid, uint32_t spawnid);

/**
 * @brief Get tokenid active process refcount.
 */
int SpmGetTokenidRefCnt(uint32_t tokenid, uint64_t *refcnt);

/**
 * @brief Clear spawnid refcount.
 */
int SpmClearSpawnidRefCnt(uint32_t spawnid);

/**
 * @brief Get SPM kernel feature version.
 */
int SpmGetVersion(uint32_t *version);

/**
 * @brief Parse the extendPerms buffer in SpmBlob.
 */
int32_t TransferSpmExternPerms(SpmBlob *data, PermsWithValue *valueList, uint32_t *listSize);

#ifdef __cplusplus
}
#endif

#endif // SPM_SETPROC_MOCK_H
