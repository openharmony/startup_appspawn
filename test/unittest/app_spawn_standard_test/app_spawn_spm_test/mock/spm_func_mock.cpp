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

#include "spm_func_mock.h"

// 静态成员初始化
namespace OHOS {
namespace AppSpawn {
std::shared_ptr<SpmFuncMock> SpmFuncMock::spmFuncMock_ = nullptr;
}
}

using namespace OHOS::AppSpawn;

// ============================================================================
// C wrapper functions - redirect calls to C++ mock object
// ============================================================================

extern "C" {
SpmData* SpmDataNew(uint32_t maxPermSize, uint32_t maxExtendPermSize, uint32_t maxBundleNameLen)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->SpmDataNew(maxPermSize, maxExtendPermSize, maxBundleNameLen);
    }
    // Fallback: return nullptr if mock not set
    return nullptr;
}

void SpmDataFree(SpmData *spmData)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        SpmFuncMock::spmFuncMock_->SpmDataFree(spmData);
    }
}

int32_t SpmGetEntry(uint32_t tokenid, SpmData *spmData)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->SpmGetEntry(tokenid, spmData);
    }
    // Fallback: return error if mock not set
    return -1;
}

int32_t SpmIncTokenidRefCnt(uint32_t tokenid, uint32_t spawnId)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->SpmIncTokenidRefCnt(tokenid, spawnId);
    }
    return -1;
}

int32_t SpmDecTokenidRefCnt(uint32_t tokenid, uint32_t spawnId)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->SpmDecTokenidRefCnt(tokenid, spawnId);
    }
    return -1;
}

int32_t SpmIncUidRefCnt(uint32_t uid, uint32_t spawnId)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->SpmIncUidRefCnt(uid, spawnId);
    }
    return -1;
}

int32_t SpmDecUidRefCnt(uint32_t uid, uint32_t spawnId)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->SpmDecUidRefCnt(uid, spawnId);
    }
    return -1;
}

int32_t SpmGetUidRefCnt(uint32_t uid, uint64_t *refCnt)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->SpmGetUidRefCnt(uid, refCnt);
    }
    return -1;
}

int32_t SpmClearSpawnidRefCnt(uint32_t spawnId)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->SpmClearSpawnidRefCnt(spawnId);
    }
    return -1;
}

// Permission functions from perm_setproc_c.h
int32_t GetPermissionsFromKernel(uint32_t tokenId, uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint32_t permsSize)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->GetPermissionsFromKernel(tokenId, perms, permsSize);
    }
    return -1;
}

int32_t FilterKernelPermissions(uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint16_t *kernelPerms, uint32_t* permSize)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->FilterKernelPermissions(perms, kernelPerms, permSize);
    }
    return -1;
}

bool TransferOpCodeToPermission(uint32_t opCode, char *permissionName, uint32_t nameSize)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->TransferOpCodeToPermission(opCode, permissionName, nameSize);
    }
    return false;
}

// Permission functions from spm_setproc.h
int32_t TransferSpmExternPerms(SpmBlob *data, PermsWithValue *valueList, uint32_t *listSize)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->TransferSpmExternPerms(data, valueList, listSize);
    }
    return -1;
}

void* GetAppSpawnMsgInfo(const AppSpawnMsgNode *message, int type)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->GetAppSpawnMsgInfo(message, type);
    }
    return nullptr;
}

int CheckAppSpawnMsgFlag(const AppSpawnMsgNode *message, uint32_t type, uint32_t index)
{
    if (SpmFuncMock::spmFuncMock_ != nullptr) {
        return SpmFuncMock::spmFuncMock_->CheckAppSpawnMsgFlag(message, type, index);
    }
    return 0;
}

} // extern "C"
