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

#ifndef SPM_FUNC_MOCK_H
#define SPM_FUNC_MOCK_H

#include <gmock/gmock.h>
#include <memory>
#include <cstdint>
#include <cstring>

// Include actual appspawn headers first to get type definitions
#include "appspawn_manager.h"

// Include mock SPM headers (these replace real access_token headers for testing)
#include "spm_setproc.h"
#include "perm_setproc_c.h"

// SpawnId definitions for different spawn modes
#define SPAWN_ID_APP      1   // Application spawn
#define SPAWN_ID_NWEB     2   // NWeb spawn
#define SPAWN_ID_NATIVE   3   // Native spawn
#define SPAWN_ID_CJAPP    4   // CJApp spawn
#define SPAWN_ID_HYBRID   5   // Hybrid spawn
#define SPAWN_ID_DEFAULT  SPAWN_ID_APP

#ifdef __cplusplus
extern "C" {
#endif

// C wrapper function declarations (will be called from C code)
SpmData* SpmDataNew(uint32_t maxPermSize, uint32_t maxExtendPermSize, uint32_t maxBundleNameLen);
void SpmDataFree(SpmData *spmData);
int32_t SpmGetEntry(uint32_t tokenid, SpmData *spmData);
int32_t SpmIncTokenidRefCnt(uint32_t tokenid, uint32_t spawnId);
int32_t SpmDecTokenidRefCnt(uint32_t tokenid, uint32_t spawnId);
int32_t SpmIncUidRefCnt(uint32_t uid, uint32_t spawnId);
int32_t SpmDecUidRefCnt(uint32_t uid, uint32_t spawnId);
int32_t SpmGetUidRefCnt(uint32_t uid, uint64_t *refCnt);
int32_t SpmClearSpawnidRefCnt(uint32_t spawnId);
// Note: Permission functions (GetPermissionsFromKernel, FilterKernelPermissions,
// TransferSpmExternPerms, TransferOpCodeToPermission) are declared in perm_setproc_c.h
// and spm_setproc.h, not redeclared here to avoid conflicts

// AppSpawn message functions (mocked for testing)
void* GetAppSpawnMsgInfo(const AppSpawnMsgNode *message, int type);
int CheckAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index);

#ifdef __cplusplus
}
#endif

namespace OHOS {
namespace AppSpawn {

/**
 * @brief SPM 函数 Mock 接口类
 *
 * 继承此类并实现 MOCK_METHOD
 */
class SpmFuncMock {
public:
    virtual ~SpmFuncMock() = default;

    // SPM 数据管理
    virtual SpmData* SpmDataNew(uint32_t maxPermSize, uint32_t maxExtendPermSize, uint32_t maxBundleNameLen) = 0;
    virtual void SpmDataFree(SpmData *spmData) = 0;

    // SPM 数据获取
    virtual int32_t SpmGetEntry(uint32_t tokenid, SpmData *spmData) = 0;

    // TokenId 引用计数管理
    virtual int32_t SpmIncTokenidRefCnt(uint32_t tokenid, uint32_t spawnId) = 0;
    virtual int32_t SpmDecTokenidRefCnt(uint32_t tokenid, uint32_t spawnId) = 0;

    // UID 引用计数管理
    virtual int32_t SpmIncUidRefCnt(uint32_t uid, uint32_t spawnId) = 0;
    virtual int32_t SpmDecUidRefCnt(uint32_t uid, uint32_t spawnId) = 0;
    virtual int32_t SpmGetUidRefCnt(uint32_t uid, uint64_t *refCnt) = 0;

    // 清理操作
    virtual int32_t SpmClearSpawnidRefCnt(uint32_t spawnId) = 0;

    // 权限相关接口 (for perm_setproc_c functions)
    virtual int32_t GetPermissionsFromKernel(uint32_t tokenId,
        uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint32_t permsSize) = 0;
    virtual int32_t FilterKernelPermissions(uint32_t perms[MAX_PERM_BIT_MAP_SIZE],
        uint16_t *kernelPerms, uint32_t* permSize) = 0;
    virtual bool TransferOpCodeToPermission(uint32_t opCode, char *permissionName, uint32_t nameSize) = 0;

    // SPM permission interface (for spm_setproc functions)
    virtual int32_t TransferSpmExternPerms(SpmBlob *data, PermsWithValue *valueList, uint32_t *listSize) = 0;

    // AppSpawn 消息函数
    virtual void* GetAppSpawnMsgInfo(const AppSpawnMsgNode *message, int type) = 0;
    virtual int CheckAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index) = 0;

    // 静态指针，指向实际的 mock 对象
    static std::shared_ptr<SpmFuncMock> spmFuncMock_;
};

/**
 * @brief SPM 函数 Mock 实现类（使用 GMOCK）
 */
class SpmFuncMockImpl : public SpmFuncMock {
public:
    MOCK_METHOD(SpmData*, SpmDataNew, (uint32_t maxPermSize, uint32_t maxExtendPermSize, uint32_t maxBundleNameLen));
    MOCK_METHOD(void, SpmDataFree, (SpmData *spmData));
    MOCK_METHOD(int32_t, SpmGetEntry, (uint32_t tokenid, SpmData *spmData));
    MOCK_METHOD(int32_t, SpmIncTokenidRefCnt, (uint32_t tokenid, uint32_t spawnId));
    MOCK_METHOD(int32_t, SpmDecTokenidRefCnt, (uint32_t tokenid, uint32_t spawnId));
    MOCK_METHOD(int32_t, SpmIncUidRefCnt, (uint32_t uid, uint32_t spawnId));
    MOCK_METHOD(int32_t, SpmDecUidRefCnt, (uint32_t uid, uint32_t spawnId));
    MOCK_METHOD(int32_t, SpmGetUidRefCnt, (uint32_t uid, uint64_t *refCnt));
    MOCK_METHOD(int32_t, SpmClearSpawnidRefCnt, (uint32_t spawnId));
    // Permission-related MOCK_METHODs
    MOCK_METHOD(int32_t, GetPermissionsFromKernel,
        (uint32_t tokenId, uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint32_t permsSize));
    MOCK_METHOD(int32_t, FilterKernelPermissions,
        (uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint16_t *kernelPerms, uint32_t* permSize));
    MOCK_METHOD(bool, TransferOpCodeToPermission, (uint32_t opCode, char *permissionName, uint32_t nameSize));
    MOCK_METHOD(int32_t, TransferSpmExternPerms, (SpmBlob *data, PermsWithValue *valueList, uint32_t *listSize));
    MOCK_METHOD(void*, GetAppSpawnMsgInfo, (const AppSpawnMsgNode *message, int type));
    MOCK_METHOD(int, CheckAppSpawnMsgFlag, (const AppSpawnMsgNode *message, uint32_t type, uint32_t index));
};

} // namespace AppSpawn
} // namespace OHOS

#endif // SPM_FUNC_MOCK_H
