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

/**
 * @file spm.c
 * @brief SPM (Security Process Manager) module for appspawn
 *
 * This module provides SPM integration for appspawn:
 * 1. Socket message validation and rebuild from kernel SPM
 * 2. Reference count management (tokenid/uid) for spawned processes
 * 3. Automatic hooks registration on module load
 *
 * SpawnId is a fixed value per runMode, mapped by RunModeToSpawnId().
 * All apps in the same spawn service instance share the same spawnId.
 */

#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "cJSON.h"
#include "securec.h"
#include "spm.h"
#include <inttypes.h>

#include "spm_setproc.h"
#include "perm_setproc_c.h"
#include "appspawn_mount_permission.h"
#include "hisysevent_adapter.h"
#include "spm_permission.h"
#include "tlv_builder.h"
/**
 * @brief Create a new message structure (internal implementation)
 * @return Pointer to new message or NULL on failure
 */
static AppSpawnMsgNode *CreateAppSpawnMsgInternal(void)
{
    AppSpawnMsgNode *message = (AppSpawnMsgNode *)calloc(1, sizeof(AppSpawnMsgNode));
    if (message == NULL) {
        APPSPAWN_LOGE("CreateAppSpawnMsgInternal: Failed to allocate memory");
        return NULL;
    }
    message->buffer = NULL;
    message->tlvOffset = NULL;
    return message;
}

/**
 * @brief Delete a message structure (internal implementation)
 * @param msgNode Pointer to message pointer (will be set to NULL)
 */
static void DeleteAppSpawnMsgInternal(AppSpawnMsgNode **msgNode)
{
    if (msgNode == NULL || *msgNode == NULL) {
        return;
    }
    if ((*msgNode)->buffer) {
        free((*msgNode)->buffer);
        (*msgNode)->buffer = NULL;
    }
    if ((*msgNode)->tlvOffset) {
        free((*msgNode)->tlvOffset);
        (*msgNode)->tlvOffset = NULL;
    }
    free(*msgNode);
    *msgNode = NULL;
}

// Note: Other SPM functions are declared in included headers:
// - SpmGetEntry, SpmDataFree, reference count functions (via spm_setproc.h / perm_setproc_c.h)

// Global spawnId (will be initialized in SpmPreloadHook)
static uint32_t g_spawnId = 0;

// SpawnId definitions for different spawn modes
#define SPAWN_ID_APP       1   // Application spawn
#define SPAWN_ID_NWEB      2   // NWeb spawn
#define SPAWN_ID_NATIVE    3   // Native spawn
#define SPAWN_ID_CJAPP     4   // CJApp spawn
#define SPAWN_ID_HYBRID    5   // Hybrid spawn
#define SPAWN_ID_DEFAULT   SPAWN_ID_APP
#define BITS_PER_WORD      32

// APL level string constants
#define APL_NAME_NORMAL       "normal"
#define APL_NAME_SYSTEM_BASIC "system_basic"
#define APL_NAME_SYSTEM_CORE  "system_core"

// APL level to string mapping (index matches AplLevel enum value)
static const char * const APL_NAME_MAP[] = {
    APL_NAME_NORMAL,       // APL_INVALID (0) -> fallback to "normal"
    APL_NAME_NORMAL,       // APL_NORMAL (1)
    APL_NAME_SYSTEM_BASIC, // APL_SYSTEM_BASIC (2)
    APL_NAME_SYSTEM_CORE,  // APL_SYSTEM_CORE (3)
};
#define APL_NAME_MAP_SIZE (sizeof(APL_NAME_MAP) / sizeof(APL_NAME_MAP[0]))

// Distribution type to string mapping
static const char * const DISTRIBUTION_TYPE_MAP[] = {
    "none",              // 0
    "app_gallery",       // 1
    "enterprise",        // 2
    "os_integration",    // 3
    "crowdtesting",      // 4
    "enterprise_normal", // 5
    "enterprise_mdm",    // 6
    "internaltesting"    // 7
};
#define DISTRIBUTION_TYPE_MAP_SIZE (sizeof(DISTRIBUTION_TYPE_MAP) / sizeof(DISTRIBUTION_TYPE_MAP[0]))

// Refcount context: encapsulates accessTokenIdEx and uid for refcount operations
typedef struct {
    uint64_t accessTokenIdEx;  // 64-bit: [tokenidattr(32-bit) | tokenid(32-bit)]
    uint32_t uid;              // UID for refcount operations
} RefcountContext;

uint32_t GetSpawnId(void)
{
    return g_spawnId;
}

/**
 * @brief Convert runMode to fixed spawnId
 *
 * Each spawn mode maps to a distinct fixed spawnId value.
 */
// SpawnId mapping table: runMode -> spawnId
static const struct {
    uint32_t runMode;
    uint32_t spawnId;
} SPAWN_ID_MAP[] = {
    {MODE_FOR_APP_SPAWN, SPAWN_ID_APP},
    {MODE_FOR_APP_COLD_RUN, SPAWN_ID_APP},
    {MODE_FOR_NWEB_SPAWN, SPAWN_ID_NWEB},
    {MODE_FOR_NWEB_COLD_RUN, SPAWN_ID_NWEB},
    {MODE_FOR_NATIVE_SPAWN, SPAWN_ID_NATIVE},
    {MODE_FOR_NATIVE_COLD_RUN, SPAWN_ID_NATIVE},
    {MODE_FOR_CJAPP_SPAWN, SPAWN_ID_CJAPP},
    {MODE_FOR_CJAPP_COLD_RUN, SPAWN_ID_CJAPP},
    {MODE_FOR_HYBRID_SPAWN, SPAWN_ID_HYBRID},
    {MODE_FOR_HYBRID_COLD_RUN, SPAWN_ID_HYBRID},
};
#define SPAWN_ID_MAP_SIZE (sizeof(SPAWN_ID_MAP) / sizeof(SPAWN_ID_MAP[0]))

static uint32_t RunModeToSpawnId(uint32_t runMode)
{
    for (uint32_t i = 0; i < SPAWN_ID_MAP_SIZE; i++) {
        if (SPAWN_ID_MAP[i].runMode == runMode) {
            return SPAWN_ID_MAP[i].spawnId;
        }
    }
    // Default to APP spawnId if not found
    return SPAWN_ID_DEFAULT;
}

int CleanupStaleSpawns(void)
{
    uint32_t spawnId = GetSpawnId();

    APPSPAWN_LOGI("CleanupStaleSpawns: spawnId=%{public}u", spawnId);

    int ret = SpmClearSpawnidRefCnt(spawnId);
    if (ret != 0) {
        APPSPAWN_LOGW("CleanupStaleSpawns failed: spawnId=%{public}u, ret=%{public}d", spawnId, ret);
    }

    return 0;
}

// ============================================================================
// Message Rebuild from SPM (Two-Phase Approach)
// ============================================================================

// TLV builder functions are now in tlv_builder.c

/**
 * @brief Collect DAC_INFO TLV descriptor from SPM data and add to list
 *
 * Note: SPM DAC data is only used when:
 * 1. Not in nweb mode (MODE_FOR_NWEB_SPAWN or MODE_FOR_NWEB_COLD_RUN)
 * 2. APP_FLAGS_ISOLATED_SANDBOX_TYPE flag is not set
 *
 * @param mgr AppSpawn manager instance (to check runMode)
 * @param ctx AppSpawning context (to check flags)
 * @param oldMsg Old message node (to check if DAC_INFO exists)
 * @param spmData SPM data from kernel
 * @param head List head to append entry to
 * @return 1 if entry was added, 0 if skipped, -1 on error
 */
static int CollectDacInfoFromSPM(const AppSpawnMgr *mgr, const AppSpawningCtx *ctx,
    const AppSpawnMsgNode *oldMsg, const SpmData *spmData, ListNode *head)
{
    if (spmData == NULL || head == NULL || mgr == NULL || ctx == NULL) {
        return -1;
    }

    // Check if we should use SPM DAC data
    // Only use SPM DAC when: 1) not nweb mode AND 2) no APP_FLAGS_ISOLATED_SANDBOX_TYPE flag
    uint32_t runMode = mgr->content.mode;
    bool isNWebMode = (runMode == MODE_FOR_NWEB_SPAWN || runMode == MODE_FOR_NWEB_COLD_RUN);
    bool hasIsolatedSandboxFlag = CheckAppMsgFlagsSet(ctx, APP_FLAGS_ISOLATED_SANDBOX_TYPE);
    if (isNWebMode || hasIsolatedSandboxFlag) {
        // Skip SPM DAC data, just copy from old message if exists
        APPSPAWN_LOGI("CollectDacInfoFromSPM: Skip SPM DAC (isNWeb=%{public}d, hasIsolatedFlag=%{public}d)",
                      isNWebMode, hasIsolatedSandboxFlag);

        // Copy DAC from old message if exists, otherwise skip
        if (oldMsg != NULL && oldMsg->tlvOffset[TLV_DAC_INFO] != INVALID_OFFSET) {
            return CopyStandardTlv(oldMsg, TLV_DAC_INFO, head);
        }
        return 0;  // DAC_INFO not present, skip successfully
    }

    uint32_t dataLen = sizeof(AppDacInfo);  // Raw length, AddStandardTlv will align

    uint8_t *data = (uint8_t *)calloc(1, dataLen);
    APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate DAC info data");

    AppDacInfo *dacInfo = (AppDacInfo *)data;

    // Copy from old message if exists
    if (oldMsg != NULL && oldMsg->tlvOffset[TLV_DAC_INFO] != INVALID_OFFSET) {
        AppSpawnTlv *oldTlv = (AppSpawnTlv *)(oldMsg->buffer + oldMsg->tlvOffset[TLV_DAC_INFO]);
        AppDacInfo *oldDacInfo = (AppDacInfo *)(oldTlv + 1);
        int cpRet = memcpy_s(data, sizeof(AppDacInfo), oldDacInfo, sizeof(AppDacInfo));
        APPSPAWN_CHECK(cpRet == 0, free(data); return -1,
            "Failed to copy DAC info from old message (ret=%{public}d)", cpRet);
    }

    // Update uid from SPM (always override)
    dacInfo->uid = spmData->uid;

    APPSPAWN_LOGI("CollectDacInfoFromSPM: uid=%{public}u, gid=%{public}u, gidCount=%{public}u",
        dacInfo->uid, dacInfo->gid, dacInfo->gidCount);
    int ret = AddStandardTlv(head, TLV_DAC_INFO, data, dataLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1, "Failed to add DAC_INFO TLV");

    return 0;
}

/**
 * @brief Compute permission flag indexes from kernel permission bitmap
 *
 * @param tokenid Token ID to query kernel for permission bitmap
 * @param flagIndexes Output array for computed spawn flag indexes
 * @param maxCount Output number of bitmap words (flagIndexes array size)
 * @return 0 on success, -1 on failure
 */
static int ComputePermissionFlags(const AppSpawnMgr *mgr,
    uint64_t tokenid, uint32_t *flagIndexes, uint32_t *maxCount)
{
    APPSPAWN_CHECK_ONLY_EXPER(mgr!= NULL && flagIndexes != NULL && maxCount != NULL, return -1);
    // Get permission bitmap from kernel
    uint32_t perms[MAX_PERM_BIT_MAP_SIZE];
    int32_t ret = GetPermissionsFromKernel(tokenid, perms, MAX_PERM_BIT_MAP_SIZE);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get permissions from kernel");

    // Log permission bitmap (first 4 words)
    APPSPAWN_LOGI("ComputePermissionFlags: tokenid=%{public}" PRIu64 ", bitmap[0-3]=0x%{public}x "
        "0x%{public}x 0x%{public}x 0x%{public}x",
        tokenid, perms[0], perms[1], perms[2], perms[3]);

    // Get permission queue directly from AppSpawnContent (avoid struct copy)
    const SandboxQueue *permQueue = mgr->content.permissionQueue;
    APPSPAWN_CHECK(permQueue != NULL, return -1,
        "Failed to get permission queue from AppSpawnContent");

    // Compute maxCount from permQueue length: ceil(queueLen / 32) bitmap words
    uint32_t queueLen = (uint32_t)OH_ListGetCnt(&permQueue->front);
    *maxCount = (queueLen + BITS_PER_WORD - 1) / BITS_PER_WORD;

    // Compute spawn flag indexes from kernel permission bitmap
    ret = GetSpawnFlagIndexesFromPermissionBitmap(permQueue, perms, flagIndexes, *maxCount);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get spawn flag indexes");

    APPSPAWN_LOGI("ComputePermissionFlags: queueLen=%{public}u, maxCount=%{public}u",
                  queueLen, *maxCount);
    return 0;
}

/**
 * @brief Collect PERMISSION TLV descriptor from SPM kernel data and add to list
 *
 * @param oldMsg Old message node (to check if PERMISSION exists, for logging)
 * @param spmData SPM data from kernel
 * @param head List head to append entry to
 * @return 1 if entry was added, 0 if skipped, -1 on error
 */
static int CollectPermissionFromSPM(const AppSpawnMgr *mgr, const SpmData *spmData, ListNode *head)
{
    APPSPAWN_CHECK(spmData != NULL && head != NULL && mgr != NULL, return -1, "invalid param");

    uint32_t flagIndexes[MAX_GRANTED_PERMISSIONS] = {0};
    uint32_t maxCount = 0;
    int ret = ComputePermissionFlags(mgr, spmData->tokenid, flagIndexes, &maxCount);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to compute permission flags");

    // Always create TLV_PERMISSION, even with count=0
    uint32_t flagsDataLen = sizeof(uint32_t) + maxCount * sizeof(uint32_t);  // Raw length

    uint8_t *data = (uint8_t *)calloc(1, flagsDataLen);
    APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate permission data");

    *((uint32_t *)data) = (uint32_t)maxCount;
    for (uint32_t i = 0; i < maxCount; i++) {
        ((uint32_t *)(data + sizeof(uint32_t)))[i] = flagIndexes[i];
    }

    APPSPAWN_LOGV("CollectPermissionFromSPM: %{public}u bitmap words", maxCount);
    ret = AddStandardTlv(head, TLV_PERMISSION, data, flagsDataLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1, "Failed to add PERMISSION TLV");

    return 0;
}

/**
 * @brief Collect BUNDLE_INFO TLV descriptor from SPM data and add to list
 *
 * @param oldMsg Old message node (to check if BUNDLE_INFO exists)
 * @param spmData SPM data from kernel
 * @param head List head to append entry to
 * @return 1 if entry was added, 0 if skipped, -1 on error
 */
static int CollectBundleInfoFromSPM(const AppSpawnMsgNode *oldMsg,
    const SpmData *spmData, ListNode *head)
{
    if (spmData == NULL || head == NULL || spmData->name.buf == NULL) {
        return -1;
    }

    // BUNDLE_INFO structure: [AppSpawnMsgBundleInfo] [bundleName string]
    // 使用实际字符串长度，而不是缓冲区大小
    uint32_t nameLen = strlen(spmData->name.buf);
    // 限制最大长度
    if (nameLen >= APP_LEN_BUNDLE_NAME) {
        nameLen = APP_LEN_BUNDLE_NAME - 1;
    }

    // Total length: struct + string + '\0' (raw length)
    uint32_t dataLen = sizeof(AppSpawnMsgBundleInfo) + nameLen + 1;

    uint8_t *data = (uint8_t *)calloc(1, dataLen);
    APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate bundle info data");

    AppSpawnMsgBundleInfo *bundleInfo = (AppSpawnMsgBundleInfo *)data;
    bundleInfo->bundleIndex = spmData->index;

    char *bundleName = (char *)(bundleInfo + 1);
    int ret = memcpy_s(bundleName, dataLen - sizeof(AppSpawnMsgBundleInfo), spmData->name.buf, nameLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1, "Failed to copy bundle name");

    APPSPAWN_LOGI("CollectBundleInfoFromSPM: index=%{public}u", bundleInfo->bundleIndex);
    ret = AddStandardTlv(head, TLV_BUNDLE_INFO, data, dataLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1, "Failed to add BUNDLE_INFO TLV");

    return 0;
}

/**
 * @brief Collect DOMAIN_INFO TLV descriptor from SPM data and add to list
 *
 * @param oldMsg Old message node (to check if DOMAIN_INFO exists and get hapFlags)
 * @param spmData SPM data from kernel
 * @param head List head to append entry to
 * @return 1 if entry was added, 0 if skipped, -1 on error
 */
static int CollectDomainInfoFromSPM(const AppSpawnMsgNode *oldMsg,
    const SpmData *spmData, ListNode *head)
{
    if (spmData == NULL || head == NULL) {
        return -1;
    }

    // Get hapFlags from old message if available (not from SPM, must preserve)
    uint32_t hapFlags = 0;
    if (oldMsg != NULL && oldMsg->tlvOffset[TLV_DOMAIN_INFO] != INVALID_OFFSET) {
        AppSpawnTlv *oldTlv = (AppSpawnTlv *)(oldMsg->buffer + oldMsg->tlvOffset[TLV_DOMAIN_INFO]);
        AppSpawnMsgDomainInfo *oldDomainInfo = (AppSpawnMsgDomainInfo *)(oldTlv + 1);
        hapFlags = oldDomainInfo->hapFlags;
    }

    // Convert apl to string via lookup table
    const char *aplStr = APL_NAME_NORMAL;
    APPSPAWN_ONLY_EXPER(spmData->apl < APL_NAME_MAP_SIZE, aplStr = APL_NAME_MAP[spmData->apl]);

    uint32_t aplLen = strlen(aplStr);
    uint32_t dataLen = sizeof(AppSpawnMsgDomainInfo) + aplLen + 1;

    uint8_t *data = (uint8_t *)calloc(1, dataLen);
    APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate domain info data");

    AppSpawnMsgDomainInfo *domainInfo = (AppSpawnMsgDomainInfo *)data;
    domainInfo->hapFlags = hapFlags;

    char *apl = (char *)(domainInfo + 1);
    int cpRet = memcpy_s(apl, aplLen + 1, aplStr, aplLen);
    APPSPAWN_CHECK(cpRet == 0, free(data); return -1, "Failed to copy apl string");

    APPSPAWN_LOGI("CollectDomainInfoFromSPM: apl=%{public}s", apl);
    int ret = AddStandardTlv(head, TLV_DOMAIN_INFO, data, dataLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1, "Failed to add DOMAIN_INFO TLV");

    return 0;
}

/**
 * @brief Collect OWNER_INFO TLV descriptor from SPM data and add to list
 *
 * @param oldMsg Old message node (to check if OWNER_INFO exists)
 * @param spmData SPM data from kernel
 * @param head List head to append entry to
 * @return 1 if entry was added, 0 if skipped, -1 on error
 */
static int CollectOwnerInfoFromSPM(const SpmData *spmData, ListNode *head)
{
    if (spmData == NULL || head == NULL) {
        return -1;
    }

    char ownerIdStr[32] = {0};
    int ret = snprintf_s(ownerIdStr, sizeof(ownerIdStr), sizeof(ownerIdStr) - 1, "%llu",
                         (unsigned long long)spmData->ownerid);
    APPSPAWN_CHECK(ret > 0, return -1, "Failed to convert ownerid to string");

    uint32_t ownerIdLen = strlen(ownerIdStr) + 1;  // +1 for '\0'
    uint32_t dataLen = sizeof(AppSpawnMsgOwnerId) + ownerIdLen;

    uint8_t *data = (uint8_t *)calloc(1, dataLen);
    APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate owner info data");

    // Copy string to flexible array position (skip struct header)
    char *ownerId = (char *)(data + sizeof(AppSpawnMsgOwnerId));
    int cpRet = memcpy_s(ownerId, ownerIdLen, ownerIdStr, strlen(ownerIdStr));
    APPSPAWN_CHECK(cpRet == 0, free(data); return -1, "Failed to copy ownerId string");

    APPSPAWN_LOGI("CollectOwnerInfoFromSPM: ownerid=%{public}s", ownerIdStr);
    ret = AddStandardTlv(head, TLV_OWNER_INFO, data, dataLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1, "Failed to add OWNER_INFO TLV");

    return 0;
}

/**
 * @brief Collect XPM_ID_TYPE extended TLV descriptor from SPM data and add to list
 *
 * @param spmData SPM data from kernel
 * @param head List head to append entry to
 * @return 1 if entry was added, 0 if skipped, -1 on error
 */
static int CollectXpmIdTypeFromSPM(const SpmData *spmData, ListNode *head)
{
    if (spmData == NULL || head == NULL) {
        return -1;
    }

    // XPM_ID_TYPE 作为扩展 TLV 存储，避免占用标准 TLV 的 enum 值
    uint32_t dataLen = sizeof(uint32_t);  // Raw length, AddExtTlv will align it
    uint8_t *data = (uint8_t *)calloc(1, dataLen);
    APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate xpm id type data");

    *((uint32_t *)data) = spmData->idType;

    APPSPAWN_LOGI("CollectXpmIdTypeFromSPM: idType=%{public}u", spmData->idType);
    int ret = AddExtTlv(head, MSG_EXT_NAME_XPM_ID_TYPE, 0, data, dataLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1, "Failed to add XPM_ID_TYPE TLV");

    return 0;
}

/**
 * @brief Validate SPM data consistency with original message
 *
 * @param spmData SPM data from kernel (already fetched)
 * @param accessTokenIdEx Access token ID from message (for tokenidAttr validation)
 * @param uidFromMsg UID from original message (for UID consistency check)
 * @param mgr AppSpawn manager instance (to check runMode for nweb/isolated)
 * @param ctx AppSpawning context (to check flags for isolated)
 * @return 0 on success, error code on failure
 */
static int ValidateSpmDataConsistency(const SpmData *spmData,
    uint64_t accessTokenIdEx, uint32_t uidFromMsg, const AppSpawnMgr *mgr,
    const AppSpawningCtx *ctx)
{
    if (spmData == NULL) {
        APPSPAWN_LOGE("ValidateSpmDataConsistency: Invalid parameters");
        return SPM_ERROR_INVALID_PARAM;
    }

    // 1. Validate tokenidAttr consistency
    // accessTokenIdEx is 64-bit: [tokenidattr(32-bit) | tokenid(32-bit)]
    uint32_t tokenidattrFromMsg = (uint32_t)(accessTokenIdEx >> 32);
    uint32_t tokenidattrFromSPM = spmData->tokenidAttr;
    if (tokenidattrFromMsg != tokenidattrFromSPM) {
        APPSPAWN_LOGE("ValidateSpmDataConsistency: TokenidAttr mismatch! msg=0x%{public}x, spm=0x%{public}x",
                      tokenidattrFromMsg, tokenidattrFromSPM);
        return SPM_ERROR_TOKENID_ATTR_MISMATCH;  // Abort spawn: tokenidAttr mismatch is a critical security error
    }

    APPSPAWN_LOGI("ValidateSpmDataConsistency: TokenidAttr match: 0x%{public}x", tokenidattrFromMsg);

    // 2. Validate UID consistency between original message and SPM data
    // - For normal processes: UID must be consistent (security check)
    // - For nweb/isolated processes: UID mismatch is allowed
    // Determine if this is a special process type (nweb or isolated)
    bool isSpecialProcess = false;
    if (mgr != NULL && ctx != NULL) {
        uint32_t runMode = mgr->content.mode;
        bool isNWebMode = (runMode == MODE_FOR_NWEB_SPAWN || runMode == MODE_FOR_NWEB_COLD_RUN);
        bool hasIsolatedSandboxFlag = CheckAppMsgFlagsSet(ctx, APP_FLAGS_ISOLATED_SANDBOX_TYPE);
        isSpecialProcess = isNWebMode || hasIsolatedSandboxFlag;
    }

    // Check UID consistency
    if (spmData->uid != uidFromMsg) {
        if (!isSpecialProcess) {
            // Normal process: UID mismatch is a security error
            APPSPAWN_LOGE("ValidateSpmDataConsistency: UID mismatch! msg_uid=%{public}u, spm_uid=%{public}u. ",
                uidFromMsg, spmData->uid);
            return SPM_ERROR_UID_MISMATCH;  // Abort spawn: UID mismatch is a critical security error
        }
    }

    APPSPAWN_LOGI("ValidateSpmDataConsistency: SPM data validated: uid=%{public}u, accessTokenId=%{public}u, "
                  "apl=%{public}u, ownerid=%{public}llu, idType=%{public}u, distributionType=%{public}u",
                  spmData->uid, spmData->tokenid, spmData->apl,
                  (unsigned long long)spmData->ownerid, spmData->idType, spmData->distributionType);

    return 0;
}

// ============================================================================
// JIT_PERMISSIONS TLV Rebuild Functions
// ============================================================================

/**
 * @brief Get kernel permissions and extended permissions for JIT JSON
 *
 * @param spmData SPM data from kernel
 * @param kernelPerms Output array for kernel permission opcodes
 * @param kernelCount Output number of kernel permissions
 * @param extPerms Output array for extended permissions (with values)
 * @param extPermCount Output number of extended permissions
 * @return 0 on success, -1 on failure
 */
static int GetKernelPermissionsForJson(const SpmData *spmData,
    uint16_t kernelPerms[], uint32_t *kernelCount, PermsWithValue extPerms[],
    uint32_t *extPermCount)
{
    if (spmData == NULL || kernelPerms == NULL || kernelCount == NULL ||
        extPerms == NULL || extPermCount == NULL) {
        APPSPAWN_LOGE("GetKernelPermissionsForJson: Invalid parameters");
        return -1;
    }

    // 1. Get complete permission bitmap from kernel
    uint32_t perms[MAX_PERM_BIT_MAP_SIZE];
    int32_t ret = GetPermissionsFromKernel(spmData->tokenid, perms, MAX_PERM_BIT_MAP_SIZE);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get permissions from kernel");

    // 2. Filter kernel permissions
    *kernelCount = MAX_KERNEL_PERMISSIONS;
    ret = FilterKernelPermissions(perms, kernelPerms, kernelCount);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to filter kernel permissions");

    // 3. Get extended permissions (with values)
    *extPermCount = MAX_EXT_PERMISSIONS;
    if (strlen(spmData->extendPerms.buf) > 0) {
        ret = TransferSpmExternPerms((SpmBlob *)&spmData->extendPerms, extPerms, extPermCount);
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to transfer external permissions");
    } else {
        *extPermCount = 0;
    }

    return 0;
}

/**
 * @brief Add a single permission item to the JSON permissions array
 *
 * @param permissionsArray cJSON array to add item to
 * @param opcode Permission opcode
 * @param extPerms Array of extended permissions (may contain values)
 * @param extPermCount Number of extended permissions
 * @return 0 on success, -1 on failure
 */
static int AddPermissionItemToJson(cJSON *permissionsArray, uint16_t opcode,
    const PermsWithValue extPerms[], uint32_t extPermCount)
{
    if (permissionsArray == NULL) {
        return -1;
    }

    // Convert opcode to permission name
    char permName[256] = {0};
    if (!TransferOpCodeToPermission(opcode, permName, sizeof(permName))) {
        APPSPAWN_LOGW("AddPermissionItemToJson: failed to get name for opcode %{public}u", opcode);
        return -1;
    }

    // Create permission item object
    cJSON *permItem = cJSON_CreateObject();
    if (permItem == NULL) {
        APPSPAWN_LOGW("AddPermissionItemToJson: failed to create perm item for %{public}s", permName);
        return -1;
    }

    // Check if there's an extra value in extPerms
    const char *extraValue = NULL;
    for (uint32_t j = 0; j < extPermCount; j++) {
        if (extPerms[j].code == opcode) {
            extraValue = extPerms[j].value;
            break;
        }
    }

    // Add permission to JSON based on whether it has a value
    if (extraValue != NULL && strlen(extraValue) > 0) {
        cJSON_AddStringToObject(permItem, permName, extraValue);
    } else {
        cJSON_AddTrueToObject(permItem, permName);
    }

    cJSON_AddItemToArray(permissionsArray, permItem);
    return 0;
}

/**
 * @brief 构建 JIT Permissions JSON 字符串
 *
 * 从 SPM 数据提取权限信息，使用 cJSON 构建符合格式要求的 JSON 字符串
 *
 * @param spmData SPM 数据
 * @param jsonOut 输出 JSON 字符串指针（调用方负责释放，使用 cJSON_free）
 * @param jsonLen 输出 JSON 字符串长度
 * @return 0 on success, error code on failure
 */
static int BuildJITPermissionsJson(const SpmData *spmData, char **jsonOut,
    uint32_t *jsonLen)
{
    APPSPAWN_CHECK(spmData != NULL && jsonOut != NULL && jsonLen != NULL, return -1, "Invalid parameters");

    // 1. Get kernel permissions and extended permissions
    uint16_t kernelPerms[MAX_KERNEL_PERMISSIONS];
    uint32_t kernelCount = 0;
    PermsWithValue extPerms[MAX_EXT_PERMISSIONS];
    uint32_t extPermCount = 0;

    int ret = GetKernelPermissionsForJson(spmData, kernelPerms, &kernelCount, extPerms, &extPermCount);
    APPSPAWN_CHECK(ret == 0, return -1, "Failed to get kernel permissions");

    // 2. Build JSON structure
    cJSON *root = cJSON_CreateObject();
    APPSPAWN_CHECK(root != NULL, return -1, "Failed to create JSON root");

    cJSON_AddStringToObject(root, "name", "Permissions");
    cJSON_AddNumberToObject(root, "ohos.encaps.count", kernelCount);

    cJSON *permissionsArray = cJSON_CreateArray();
    APPSPAWN_CHECK(permissionsArray != NULL, cJSON_Delete(root); return -1,
                  "Failed to create JSON permissionsArray");
    cJSON_AddItemToObject(root, "permissions", permissionsArray);

    // 3. Fill permission items
    for (uint32_t i = 0; i < kernelCount; i++) {
        AddPermissionItemToJson(permissionsArray, kernelPerms[i], extPerms, extPermCount);
    }

    // 4. Convert to JSON string
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    APPSPAWN_CHECK(json != NULL, return -1, "Failed to print JSON");

    *jsonOut = json;
    *jsonLen = (uint32_t)strlen(json);

    APPSPAWN_LOGI("BuildJITPermissionsJson: built JSON with %{public}u permissions, len=%{public}u",
                  kernelCount, *jsonLen);
    return 0;
}

/**
 * @brief Collect JIT_PERMISSIONS extended TLV descriptor from SPM data and add to list
 *
 * @param spmData SPM data from kernel
 * @param head List head to append entry to
 * @return 1 if entry was added, 0 if skipped, -1 on error
 */
static int CollectJitPermissionsFromSPM(const SpmData *spmData, ListNode *head)
{
    if (spmData == NULL || head == NULL) {
        return -1;
    }

    // Build JSON string
    char *json = NULL;
    uint32_t jsonLen = 0;
    int32_t ret = BuildJITPermissionsJson(spmData, &json, &jsonLen);
    APPSPAWN_CHECK(ret == 0 && json != NULL, return -1, "Failed to build JIT permissions JSON");

    APPSPAWN_LOGI("CollectJitPermissionsFromSPM: JSON len=%{public}u", jsonLen);
    APPSPAWN_LOGI("CollectJitPermissionsFromSPM: JSON content=%{public}s", json);

    // Copy JSON string to calloc-allocated memory to match free() in DestroyTlvEntry
    uint8_t *jsonData = (uint8_t *)calloc(1, jsonLen + 1);
    APPSPAWN_CHECK(jsonData != NULL, cJSON_free(json); return -1, "Failed to allocate JSON data");

    int cpRet = memcpy_s(jsonData, jsonLen + 1, json, jsonLen);
    cJSON_free(json);  // Free cJSON-allocated memory immediately
    APPSPAWN_CHECK(cpRet == 0, free(jsonData); return -1, "Failed to copy JSON string");

    int result = AddExtTlv(head, MSG_EXT_NAME_JIT_PERMISSIONS, DATA_TYPE_STRING, jsonData, jsonLen);
    APPSPAWN_CHECK(result == 0, free(jsonData); return -1, "Failed to add JIT_PERMISSIONS TLV");

    return 0;
}

/**
 * @brief Collect a single extended TLV by dispatching on tlvName
 *
 * Handles extended TLVs (tlvType == TLV_MAX) based on tlvName:
 *   - JIT_PERMISSIONS / APP_DISTRIBUTION_TYPE: rebuild from SPM data
 *   - Others: copy from old message
 *
 * @param tlvExt     Pointer to the extended TLV header in old message buffer
 * @param spmData    SPM data from kernel
 * @param head       List head to append entry to
 * @return 0 on success, -1 on failure
 */

/**
 * @brief Rebuild APP_DISTRIBUTION_TYPE extended TLV from SPM distribution type
 *
 * @param distributionType Distribution type enum value from SPM
 * @param head List head to append entry to
 * @return 0 on success, -1 on failure
 */
static int RebuildDistributionTypeTlv(uint32_t distributionType, ListNode *head)
{
    const char *distStr = "none";
    if (distributionType < DISTRIBUTION_TYPE_MAP_SIZE) {
        distStr = DISTRIBUTION_TYPE_MAP[distributionType];
    }

    uint32_t distLen = strlen(distStr);  // Raw length, AddExtTlv will align for STRING
    uint32_t allocLen = (distLen > 0) ? distLen : 1;
    uint8_t *data = (uint8_t *)calloc(1, allocLen);
    APPSPAWN_CHECK(data != NULL, return -1, "Failed to allocate distribution data");

    int cpRet = memcpy_s(data, allocLen, distStr, distLen);
    APPSPAWN_CHECK(cpRet == 0, free(data); return -1, "Failed to copy distribution string");

    APPSPAWN_LOGI("RebuildDistributionTypeTlv: Rebuilt: %{public}s", distStr);
    int ret = AddExtTlv(head, MSG_EXT_NAME_APP_DISTRIBUTION_TYPE, DATA_TYPE_STRING, data, distLen);
    APPSPAWN_CHECK(ret == 0, free(data); return -1,
                  "Failed to add APP_DISTRIBUTION_TYPE TLV");
    return 0;
}

/**
 * @brief Copy other extended TLV as-is from old message
 *
 * Moved to tlv_builder.c as CopyOtherExtTlv
 */

static int CollectExtTlvByName(const AppSpawnTlvExt *tlvExt,
    const SpmData *spmData, ListNode *head)
{
    if (tlvExt == NULL || spmData == NULL || head == NULL) {
        return -1;
    }

    // JIT_PERMISSIONS: rebuild from SPM
    if (strcmp(tlvExt->tlvName, MSG_EXT_NAME_JIT_PERMISSIONS) == 0) {
        return CollectJitPermissionsFromSPM(spmData, head);
    }

    // APP_DISTRIBUTION_TYPE: rebuild from SPM
    if (strcmp(tlvExt->tlvName, MSG_EXT_NAME_APP_DISTRIBUTION_TYPE) == 0) {
        return RebuildDistributionTypeTlv(spmData->distributionType, head);
    }

    // Other extended TLVs: copy as-is
    return CopyOtherExtTlv(tlvExt, head);
}

/**
 * @brief Collect all TLVs by walking the original message buffer sequentially
 *
 * Reads AppSpawnTlv/AppSpawnTlvExt headers directly from oldMsg->buffer,
 * preserving the exact original order. Uses a unified switch on tlvType:
 *   - Standard sensitive (BUNDLE_INFO, DAC_INFO, etc.): rebuild from SPM data
 *   - Standard non-sensitive: copy from old message
 *   - Extended (TLV_MAX): dispatch by tlvName for rebuild or copy
 *
 * TLV_XPM_ID_TYPE is newly added and not present in original client messages,
 * so it is appended after the buffer walk.
 *
 * @param mgr AppSpawn manager instance
 * @param ctx AppSpawning context (for flags checking)
 * @param oldMsg Old message node
 * @param spmData SPM data from kernel
 * @param head List head to append entries to
 * @return 0 on success, -1 on failure
 */
static int CollectTlvsInOrder(const AppSpawnMgr *mgr, const AppSpawningCtx *ctx,
    const AppSpawnMsgNode *oldMsg, const SpmData *spmData, ListNode *head)
{
    APPSPAWN_CHECK(oldMsg != NULL && spmData != NULL && head != NULL, return -1,
        "Invalid parameters: oldMsg=%{public}p, spmData=%{public}p, head=%{public}p", oldMsg, spmData, head);

    uint32_t bufferLen = oldMsg->msgHeader.msgLen - sizeof(AppSpawnMsg);
    uint32_t currLen = 0;

    while (currLen < bufferLen) {
        APPSPAWN_CHECK(currLen + sizeof(AppSpawnTlv) <= bufferLen, return -1,
            "truncated TLV header at offset %{public}u", currLen);
        AppSpawnTlv *tlv = (AppSpawnTlv *)(oldMsg->buffer + currLen);
        APPSPAWN_CHECK(tlv->tlvLen >= sizeof(AppSpawnTlv) && currLen + tlv->tlvLen <= bufferLen,
            return -1, "invalid tlvLen=%{public}u at offset %{public}u", tlv->tlvLen, currLen);
        int result = 0;
        switch (tlv->tlvType) {
            case TLV_BUNDLE_INFO:
                result = CollectBundleInfoFromSPM(oldMsg, spmData, head);
                break;
            case TLV_DAC_INFO:
                result = CollectDacInfoFromSPM(mgr, ctx, oldMsg, spmData, head);
                break;
            case TLV_DOMAIN_INFO:
                result = CollectDomainInfoFromSPM(oldMsg, spmData, head);
                break;
            case TLV_OWNER_INFO:
                result = CollectOwnerInfoFromSPM(spmData, head);
                break;
            case TLV_PERMISSION:
                result = CollectPermissionFromSPM(mgr, spmData, head);
                break;
            case TLV_MAX: {
                result = CollectExtTlvByName((AppSpawnTlvExt *)(oldMsg->buffer + currLen), spmData, head);
                break;
            }
            default:
                result = CopyStandardTlv(oldMsg, tlv->tlvType, head);
                break;
        }
        APPSPAWN_CHECK(result >= 0, return -1, "Failed to collect TLV %{public}u", tlv->tlvType);
        currLen += tlv->tlvLen;
    }

    int ret = CollectXpmIdTypeFromSPM(spmData, head);
    APPSPAWN_CHECK(ret >= 0, return -1, "Failed to collect TLV_XPM_ID_TYPE");
    return 0;
}

/**
 * @brief Increment SPM reference counts for a context
 *
 * Increments tokenid and uid refcounts, tracking which ones succeed
 * so they can be rolled back precisely on failure.
 *
 * @param mgr AppSpawn manager instance (for spawn ID)
 * @param ctx AppSpawning context (will set spmRefAdded)
 * @param refCtx Refcount context containing accessTokenIdEx and uid
 * @return 0 on success, error code on failure
 */
static int IncRefCountsForContext(const AppSpawnMgr *mgr, AppSpawningCtx *ctx,
    const RefcountContext *refCtx)
{
    APPSPAWN_CHECK(ctx != NULL && mgr != NULL && refCtx != NULL, return SPM_ERROR_INVALID_PARAM,
        "Invalid parameters: ctx=%{public}p, mgr=%{public}p, refCtx=%{public}p", ctx, mgr, refCtx);

    uint32_t spawnId = GetSpawnId();
    uint8_t refAdded = SPM_REF_NONE;

    // Increment tokenid refcount
    int incRet = SpmIncTokenidRefCnt(refCtx->accessTokenIdEx & 0xFFFFFFFF, spawnId);
    APPSPAWN_CHECK(incRet == 0,
        ctx->spmRefAdded = refAdded;
        return SPM_ERROR_REF_COUNT_INC_FAILED,
        "SpmIncTokenidRefCnt failed (ret=%{public}d), tokenid=%{public}u, spawnId=%{public}u",
        incRet, (uint32_t)(refCtx->accessTokenIdEx & 0xFFFFFFFF), spawnId);
    refAdded |= SPM_REF_TOKENID;

    // Increment uid refcount
    incRet = SpmIncUidRefCnt(refCtx->uid, spawnId);
    APPSPAWN_CHECK(incRet == 0,
        ctx->spmRefAdded = refAdded;
        return SPM_ERROR_REF_COUNT_INC_FAILED,
        "SpmIncUidRefCnt failed (ret=%{public}d), uid=%{public}u, spawnId=%{public}u", incRet, refCtx->uid, spawnId);
    refAdded |= SPM_REF_UID;
    ctx->spmRefAdded = refAdded;

    APPSPAWN_LOGI("IncRefCountsForContext: refcount bitmap=0x%{public}02x, tokenid=%{public}" PRIu64
        ", uid=%{public}u, spawnId=%{public}u",
        refAdded, refCtx->accessTokenIdEx, refCtx->uid, spawnId);
    uint32_t runMode = mgr->content.mode;
    bool isNWebMode = (runMode == MODE_FOR_NWEB_SPAWN || runMode == MODE_FOR_NWEB_COLD_RUN);
    bool hasIsolatedSandboxFlag = CheckAppMsgFlagsSet(ctx, APP_FLAGS_ISOLATED_SANDBOX_TYPE);

    APPSPAWN_ONLY_EXPER(!isNWebMode && !hasIsolatedSandboxFlag, return 0);

    uint64_t uidRefCnt = 0;
    int getRet = SpmGetUidRefCnt(refCtx->uid, &uidRefCnt);
    APPSPAWN_CHECK(getRet == 0 && uidRefCnt == 1, return SPM_ERROR_GET_SPM_FAILED,
        "get spm uid ref failed %{public}d %{public}" PRIu64, getRet, uidRefCnt);
    return 0;
}

/**
 * @brief Get tokenid and UID from message for refcount operations
 *
 * @param message Message containing ACCESS_TOKEN_INFO and DAC_INFO
 * @param refCtx Output refcount context (will be filled with accessTokenIdEx and uid)
 * @return 0 on success, error code on failure
 */
static int GetTokenidAndUidFromMsg(const AppSpawnMsgNode *message,
    RefcountContext *refCtx)
{
    if (message == NULL || refCtx == NULL) {
        APPSPAWN_LOGE("GetTokenidAndUidFromMsg: Invalid parameters");
        return SPM_ERROR_INVALID_PARAM;
    }

    const AppSpawnMsgAccessToken *tokenInfo =
        (const AppSpawnMsgAccessToken *)GetAppSpawnMsgInfo(message, TLV_ACCESS_TOKEN_INFO);
    if (tokenInfo == NULL) {
        APPSPAWN_LOGE("GetTokenidAndUidFromMsg: No ACCESS_TOKEN_INFO in message");
        return SPM_ERROR_MISSING_MSG_DATA;
    }
    refCtx->accessTokenIdEx = tokenInfo->accessTokenIdEx;

    const AppDacInfo *dacInfo = (const AppDacInfo *)GetAppSpawnMsgInfo(message, TLV_DAC_INFO);
    if (dacInfo == NULL) {
        APPSPAWN_LOGE("GetTokenidAndUidFromMsg: No DAC_INFO in message");
        return SPM_ERROR_MISSING_MSG_DATA;
    }
    refCtx->uid = dacInfo->uid;

    return 0;
}

/**
 * @brief Get and validate SPM data from kernel
 *
 * This function allocates SPM data, retrieves it from kernel, and validates consistency.
 * On failure, it returns an error code and the caller should use original message.
 *
 * @param oldMsg Original message containing ACCESS_TOKEN_INFO
 * @param accessTokenIdEx Access token ID (for SpmGetEntry)
 * @param uidForRefcount UID from original message (for validation)
 * @param mgr AppSpawn manager instance (for process type check)
 * @param ctx AppSpawning context (for process type check)
 * @param spmOut Output pointer for SPM data (caller owns, must be freed on failure)
 * @return 0 on success, error code on failure
 */
static int GetAndValidateSpmData(const AppSpawnMsgNode *oldMsg,
    const RefcountContext *refCtx, const AppSpawnMgr *mgr,
    const AppSpawningCtx *ctx, SpmData **spmOut)
{
    // 1. Allocate SPM data
    SpmData *spmData = SpmDataNew(MAX_PERM_SIZE, MAX_EXTEND_PERM_SIZE, MAX_BUNDLE_NAME_LEN);
    APPSPAWN_CHECK(spmData != NULL, return SPM_ERROR_NO_MEMORY,
        "GetAndValidateSpmData: Failed to allocate SpmData");

    // 2. Get SPM data from kernel
    int ret = SpmGetEntry(refCtx->accessTokenIdEx & 0xFFFFFFFF, spmData);
    APPSPAWN_CHECK(ret == 0, SpmDataFree(spmData);
        return SPM_ERROR_GET_SPM_FAILED,
        "Failed to get SPM data for tokenid=%{public}" PRIu64 ", ret=%{public}d", refCtx->accessTokenIdEx, ret);

    // 3. Validate consistency
    ret = ValidateSpmDataConsistency(spmData, refCtx->accessTokenIdEx, refCtx->uid, mgr, ctx);
    APPSPAWN_CHECK(ret == 0, SpmDataFree(spmData);
        return ret, "GetAndValidateSpmData: SPM data validation failed (ret=%{public}d)", ret);

    *spmOut = spmData;
    return 0;
}

/**
 * @brief Use original message for spawn after incrementing refcounts
 *
 * This function is called when SPM data is not available (due to allocation failure,
 * SpmGetEntry failure, or TLV build failure). It increments refcounts for the original
 * message and returns success to continue spawn with the original message.
 *
 * @param mgr AppSpawn manager instance
 * @param ctx AppSpawning context (will set spmRefAdded)
 * @param refCtx Refcount context containing accessTokenIdEx and uid
 * @return 0 on success (continue with original message), error code on failure (abort spawn)
 */
static int UseOriginalMsgForSpawn(const AppSpawnMgr *mgr,
    AppSpawningCtx *ctx, const RefcountContext *refCtx)
{
    // Increment refcounts for original message (must do this before using original message)
    int ret = IncRefCountsForContext(mgr, ctx, refCtx);
    if (ret != 0) {
        APPSPAWN_LOGE("UseOriginalMsgForSpawn: Failed to increment refcounts (ret=%{public}d), aborting spawn", ret);
        return ret;  // Must abort: refcount increment failed
    }

    // Continue spawn with original message
    APPSPAWN_LOGW("UseOriginalMsgForSpawn: Using original message for spawn");
    return 0;  // Continue with original message
}

/**
 * @brief Rollback SPM reference counts on failure
 *
 * Rolls back tokenid and uid refcounts that were successfully incremented.
 *
 * @param ctx AppSpawning context containing spmRefAdded bitmap
 * @param refCtx Refcount context containing accessTokenIdEx and uid
 */
static void RollbackRefCounts(AppSpawningCtx *ctx, const RefcountContext *refCtx)
{
    if (ctx == NULL || ctx->spmRefAdded == SPM_REF_NONE) {
        return;
    }

    uint32_t spawnId = GetSpawnId();

    // Rollback tokenid refcount
    if (ctx->spmRefAdded & SPM_REF_TOKENID) {
        SpmDecTokenidRefCnt(refCtx->accessTokenIdEx & 0xFFFFFFFF, spawnId);
        APPSPAWN_LOGI("RollbackRefCounts: Rolled back tokenid refcount");
    }

    // Rollback uid refcount
    if (ctx->spmRefAdded & SPM_REF_UID) {
        SpmDecUidRefCnt(refCtx->uid, spawnId);
        APPSPAWN_LOGI("RollbackRefCounts: Rolled back uid refcount (uid=%{public}u)", refCtx->uid);
    }

    ctx->spmRefAdded = SPM_REF_NONE;
}

/**
 * @brief Phase 2: Calculate buffer size, allocate, and write all TLVs
 *
 * @param oldMsg Old message (for header copy)
 * @param entryList List of TLV descriptors from Phase 1
 * @param newMsgOut Output pointer for new message (caller owns)
 * @return 0 on success, -1 on failure
 */
static int WriteTlvsToNewMsg(const AppSpawnMsgNode *oldMsg,
    ListNode *entryList, AppSpawnMsgNode **newMsgOut)
{
    APPSPAWN_CHECK(oldMsg != NULL && entryList != NULL && newMsgOut != NULL, return -1,
                   "Invalid parameters: oldMsg=%{public}p, entryList=%{public}p, newMsgOut=%{public}p",
                   oldMsg, entryList, newMsgOut);

    // 1. Calculate exact buffer size and count extended TLVs
    uint32_t exactBufSize = 0;
    int extTlvCount = 0;
    ListNode *node = NULL;
    ForEachListEntry(entryList, node) {
        RebuildTlvEntry *entry = ListEntry(node, RebuildTlvEntry, node);
        exactBufSize += entry->totalLen;
        if (entry->entryType == TLV_ENTRY_EXTENDED) {
            extTlvCount++;
        }
    }

    APPSPAWN_LOGI("WriteTlvsToNewMsg: Extended TLV count=%{public}d, exact buffer size=%{public}u",
                  extTlvCount, exactBufSize);

    // 2. Create new message
    AppSpawnMsgNode *newMsg = CreateAppSpawnMsgInternal();
    APPSPAWN_CHECK(newMsg != NULL, return SPM_ERROR_NO_MEMORY, "Failed to create new message");

    // 3. Copy message header and connection
    newMsg->msgHeader = oldMsg->msgHeader;
    newMsg->connection = oldMsg->connection;

    // 4. Initialize tlvOffset array (TLV_MAX standard + extTlvCount extended)
    newMsg->tlvOffset = (uint32_t *)calloc(TLV_MAX + extTlvCount, sizeof(uint32_t));
    APPSPAWN_CHECK(newMsg->tlvOffset != NULL, DeleteAppSpawnMsgInternal(&newMsg); return SPM_ERROR_NO_MEMORY,
                   "Failed to allocate tlvOffset array");
    for (int i = 0; i < TLV_MAX + extTlvCount; i++) {
        newMsg->tlvOffset[i] = INVALID_OFFSET;
    }

    // 5. Allocate exact-size buffer
    if (exactBufSize > 0) {
        newMsg->buffer = (uint8_t *)calloc(1, exactBufSize);
        APPSPAWN_CHECK(newMsg->buffer != NULL,
                       free(newMsg->tlvOffset); newMsg->tlvOffset = NULL;
                       DeleteAppSpawnMsgInternal(&newMsg); return SPM_ERROR_NO_MEMORY,
                       "Failed to allocate message buffer (%{public}u bytes)", exactBufSize);
    }

    // 6. Write all TLV entries to buffer
    newMsg->tlvCount = 0;
    uint32_t currentOffset = 0;
    int ret = WriteTlvEntriesToBuffer(newMsg, entryList, &currentOffset);
    APPSPAWN_CHECK(ret == 0, DeleteAppSpawnMsgInternal(&newMsg); return SPM_ERROR_TLV_BUILD_FAILED,
                   "Failed to write TLV entries to buffer");

    // 7. Update message length (msgLen includes header size)
    newMsg->msgHeader.msgLen = sizeof(AppSpawnMsg) + currentOffset;

    *newMsgOut = newMsg;
    return 0;
}

/**
 * @brief Build and replace message with TLV-based approach
 *
 * This function performs the TLV-based message rebuild process. If TLV build fails,
 * it returns success (0) to allow using the original message (refcounts already incremented).
 *
 * @param oldMsg Original message (will be deleted on success)
 * @param spmData SPM data containing permissions and extensions
 * @param mgr AppSpawn manager instance
 * @param ctx AppSpawning context (message pointer will be updated on success)
 * @return 0 on success (always returns 0, uses original message on TLV build failure)
 */
static void BuildAndReplaceMessage(AppSpawnMsgNode *oldMsg,
    const SpmData *spmData, const AppSpawnMgr *mgr, AppSpawningCtx *ctx)
{
    ListNode entryList;
    OH_ListInit(&entryList);

    // Collect TLVs
    int ret = CollectTlvsInOrder(mgr, ctx, oldMsg, spmData, &entryList);
    APPSPAWN_CHECK(ret == 0,
                   FreeTlvEntries(&entryList);
                   return,
                   "BuildAndReplaceMessage: Failed to collect TLVs (ret=%{public}d), using original message", ret);

    int entryCount = OH_ListGetCnt(&entryList);
    APPSPAWN_LOGI("BuildAndReplaceMessage: Phase 1 complete, collected %{public}d TLV entries", entryCount);

    // Write new message
    AppSpawnMsgNode *newMsg = NULL;
    ret = WriteTlvsToNewMsg(oldMsg, &entryList, &newMsg);
    APPSPAWN_CHECK(ret == 0, FreeTlvEntries(&entryList);
        return, "BuildAndReplaceMessage: Failed to write TLVs (ret=%{public}d), using original message", ret);

    // Replace message
    DeleteAppSpawnMsgInternal(&oldMsg);
    ctx->message = newMsg;

    FreeTlvEntries(&entryList);
    APPSPAWN_LOGV("BuildAndReplaceMessage: Message successfully replaced with SPM-rebuilt message");
}

/**
 * @brief Rebuild message from SPM data (two-phase approach with linked list)
 *
 * Phase 1 — Collect: Gather all TLV descriptors into a linked list
 * Phase 2 — Write: Calculate exact size, allocate buffer, write all TLVs
 *
 * @param mgr AppSpawnMgr
 * @param ctx AppSpawningCtx（包含消息指针）
 * @return 0 on success, error code on failure
 */
static int RebuildMessageFromSPM(const AppSpawnMgr *mgr, AppSpawningCtx *ctx)
{
    APPSPAWN_CHECK(mgr != NULL && ctx != NULL && ctx->message != NULL, return SPM_ERROR_INVALID_PARAM,
        "Invalid parameters: mgr=%{public}p, ctx=%{public}p, ctx->message=%{public}p", mgr, ctx, ctx->message);

    AppSpawnMsgNode *oldMsg = ctx->message;
    APPSPAWN_LOGI("RebuildMessageFromSPM: Rebuilding message for %{public}s, msgId=%{public}u",
        oldMsg->msgHeader.processName, oldMsg->msgHeader.msgId);

    // 1. Get tokenid and UID from original message
    RefcountContext refCtx = {0};
    int ret = GetTokenidAndUidFromMsg(oldMsg, &refCtx);
    APPSPAWN_CHECK(ret == 0, return ret,
                   "RebuildMessageFromSPM: Failed to get tokenid/UID "
                   "(ret=%{public}d), aborting spawn", ret);

    // 2. Get and validate SPM data from kernel
    SpmData *spmData = NULL;
    ret = GetAndValidateSpmData(oldMsg, &refCtx, mgr, ctx, &spmData);
    if (ret != 0) {
        // SPM data validation failed (security error) or get failed (resource error)
        // For security errors, abort spawn. For resource errors, use original message.
        if (ret == SPM_ERROR_TOKENID_ATTR_MISMATCH || ret == SPM_ERROR_UID_MISMATCH) {
            APPSPAWN_LOGE("RebuildMessageFromSPM: SPM data validation failed (ret=%{public}d), aborting spawn", ret);
            return ret;  // Abort spawn: security error
        } else {
            APPSPAWN_LOGW("RebuildMessageFromSPM: (ret=%{public}d), using original message", ret);
            return UseOriginalMsgForSpawn(mgr, ctx, &refCtx);
        }
    }

    // 3. Increment refcounts using uid/tokenid from original message
    ret = IncRefCountsForContext(mgr, ctx, &refCtx);
    APPSPAWN_CHECK(ret == 0, RollbackRefCounts(ctx, &refCtx);
        SpmDataFree(spmData);
        return ret,
        "RebuildMessageFromSPM: Failed to increment refcounts (ret=%{public}d), aborting spawn", ret);

    // 4. Build and replace message
    BuildAndReplaceMessage(oldMsg, spmData, mgr, ctx);

    // Cleanup
    SpmDataFree(spmData);
    return 0;  // Always return 0 (BuildAndReplaceMessage handles failures by using original message)
}

int OnMessageRebuildFromSPM(AppSpawnMgr *mgr, AppSpawningCtx *ctx)
{
    if (mgr == NULL || ctx == NULL) {
        APPSPAWN_LOGE("OnMessageRebuildFromSPM: invalid parameters");
        return SPM_ERROR_INVALID_PARAM;
    }

    // Skip SPM operations on Linux platform
    APPSPAWN_CHECK_LOGV(!mgr->content.isLinux, return 0, "OnMessageRebuildFromSPM: skip on Linux platform");

    // Rebuild message from SPM (this will free the old message and replace the pointer)
    int ret = RebuildMessageFromSPM(mgr, ctx);
    if (ret != 0) {
        APPSPAWN_LOGE("OnMessageRebuildFromSPM: Failed to rebuild message from SPM, ret=%{public}d", ret);
        return ret;  // Fail the spawn
    }

    APPSPAWN_LOGI("OnMessageRebuildFromSPM: Message rebuilt successfully for %{public}s",
                  ctx->message->msgHeader.processName);
    return 0;
}

/**
 * @brief Hook for spawn abort reference count update
 *
 * This hook is called when app spawn fails (before app is queued).
 * Only decrements refcounts tracked in spmRefAdded bitmap.
 * Stage: STAGE_SERVER_SPAWN_ABORT (priority: HOOK_PRIO_HIGHEST to run first)
 */
int OnSpawnAbortUpdateRefCount(AppSpawnMgr *mgr, AppSpawningCtx *ctx)
{
    if (mgr == NULL || ctx == NULL) {
        return -1;
    }

    // Skip SPM operations on Linux platform
    APPSPAWN_CHECK_LOGV(!mgr->content.isLinux, return 0, "OnSpawnAbortUpdateRefCount: skip on Linux platform");

    // Check if any refcounts were incremented during message rebuild
    APPSPAWN_CHECK(ctx->spmRefAdded != SPM_REF_NONE, return 0, "spmRefAdded is 0, skip refcount decrement");

    AppSpawnMsgNode *msg = ctx->message;
    if (msg == NULL) {
        APPSPAWN_LOGE("OnSpawnAbortUpdateRefCount: msg is NULL");
        return -1;
    }

    uint32_t spawnId = GetSpawnId();

    // Decrement tokenid refcount if it was incremented
    if (ctx->spmRefAdded & SPM_REF_TOKENID) {
        const AppSpawnMsgAccessToken *tokenInfo =
            (const AppSpawnMsgAccessToken *)GetAppSpawnMsgInfo(msg, TLV_ACCESS_TOKEN_INFO);
        if (tokenInfo != NULL) {
            int ret = SpmDecTokenidRefCnt(tokenInfo->accessTokenIdEx & 0xFFFFFFFF, spawnId);
            if (ret != 0) {
                APPSPAWN_LOGW("OnSpawnAbortUpdateRefCount: SpmDecTokenidRefCnt failed (ret=%{public}d)", ret);
            }
        }
    }

    // Decrement uid refcount if it was incremented
    if (ctx->spmRefAdded & SPM_REF_UID) {
        const AppDacInfo *dacInfo = (const AppDacInfo *)GetAppSpawnMsgInfo(msg, TLV_DAC_INFO);
        if (dacInfo != NULL) {
            int ret = SpmDecUidRefCnt(dacInfo->uid, spawnId);
            if (ret != 0) {
                APPSPAWN_LOGW("OnSpawnAbortUpdateRefCount: SpmDecUidRefCnt failed (ret=%{public}d)", ret);
            }
        }
    }

    APPSPAWN_LOGI("OnSpawnAbortUpdateRefCount: bitmap=0x%{public}02x, pid=%{public}d, spawnId=%{public}u",
                  ctx->spmRefAdded, ctx->pid, spawnId);

    ctx->spmRefAdded = SPM_REF_NONE;  // Clear bitmap to prevent double release

    return 0;
}

int OnAppExitUpdateRefCount(const AppSpawnMgr *mgr, const AppSpawnedProcess *appInfo)
{
    if (mgr == NULL || appInfo == NULL) {
        return -1;
    }

    // Skip SPM operations on Linux platform
    APPSPAWN_CHECK_LOGV(!mgr->content.isLinux, return 0, "OnAppExitUpdateRefCount: skip on Linux platform");

    // Check if any refcounts were incremented
    APPSPAWN_CHECK(appInfo->spmRefAdded != SPM_REF_NONE, return 0, "spmRefAdded is 0, skip refcount decrement");

    uint32_t spawnId = GetSpawnId();

    // Decrement tokenid refcount if it was incremented
    if (appInfo->spmRefAdded & SPM_REF_TOKENID) {
        int ret = SpmDecTokenidRefCnt(appInfo->tokenid & 0xFFFFFFFF, spawnId);
        if (ret != 0) {
            APPSPAWN_LOGW("OnAppExitUpdateRefCount: SpmDecTokenidRefCnt failed (ret=%{public}d)", ret);
        }
    }

    // Decrement uid refcount if it was incremented
    if (appInfo->spmRefAdded & SPM_REF_UID) {
        int ret = SpmDecUidRefCnt(appInfo->uid, spawnId);
        if (ret != 0) {
            APPSPAWN_LOGW("OnAppExitUpdateRefCount: SpmDecUidRefCnt failed (ret=%{public}d)", ret);
        }
    }

    APPSPAWN_LOGI("OnAppExitUpdateRefCount: bitmap=0x%{public}02x, tokenid=%{public}" PRIu64
        ", uid=%{public}u, pid=%{public}d, spawnId=%{public}u",
         appInfo->spmRefAdded, appInfo->tokenid, appInfo->uid, appInfo->pid, spawnId);

    return 0;
}

static int SpmPreloadHook(AppSpawnMgr *mgr)
{
    if (mgr == NULL) {
        APPSPAWN_LOGE("SpmPreloadHook: invalid mgr");
        return -1;
    }

    // Skip SPM operations on Linux platform
    APPSPAWN_CHECK_LOGV(!mgr->content.isLinux, return 0, "SpmPreloadHook: skip on Linux platform");

    APPSPAWN_LOGI("SPM module initializing...");

    uint32_t runMode = mgr->content.mode;
    g_spawnId = RunModeToSpawnId(runMode);

    // Log spawn info
    APPSPAWN_LOGI("Initialized spawnId: runMode=%{public}u, spawnId=%{public}u",
        runMode, g_spawnId);

    int ret = SpmClearSpawnidRefCnt(g_spawnId);
    if (ret != 0) {
        APPSPAWN_LOGW("CleanupStaleSpawns failed (ret=%{public}d)", ret);
    }

    APPSPAWN_LOGI("SPM module initialized successfully");
    return 0;
}

MODULE_CONSTRUCTOR(void)
{
    APPSPAWN_LOGI("SPM module loading...");
    (void)AddServerStageHook(STAGE_SERVER_PRELOAD, HOOK_PRIO_COMMON, SpmPreloadHook);
    (void)AddAppSpawnHook(STAGE_PARENT_MSG_DECODE, HOOK_PRIO_HIGHEST, OnMessageRebuildFromSPM);
    (void)AddAppSpawnHook(STAGE_SERVER_SPAWN_ABORT, HOOK_PRIO_HIGHEST, OnSpawnAbortUpdateRefCount);
    (void)AddProcessMgrHook(STAGE_SERVER_APP_CLEANUP, HOOK_PRIO_HIGHEST, OnAppExitUpdateRefCount);
    APPSPAWN_LOGI("SPM module loaded successfully, hooks registered");
}