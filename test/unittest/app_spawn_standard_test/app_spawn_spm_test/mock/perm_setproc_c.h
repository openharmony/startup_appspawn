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

#ifndef PERM_SETPROC_C_MOCK_H
#define PERM_SETPROC_C_MOCK_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#include <stdbool.h>
#endif

// Forward declare MAX_PERM_BIT_MAP_SIZE if not defined
#ifndef MAX_PERM_BIT_MAP_SIZE
#define MAX_PERM_BIT_MAP_SIZE 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

int32_t AddPermissionToKernel(uint32_t tokenId, const char *perms, uint32_t permsSize);
int32_t RemovePermissionFromKernel(uint32_t tokenID);
int32_t SetPermissionToKernel(uint32_t tokenID, int32_t opCode, bool status);
int32_t GetPermissionFromKernel(uint32_t tokenID, int32_t opCode, bool *isGranted);
int32_t GetPermissionsFromKernel(uint32_t tokenId, uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint32_t permsSize);
int32_t FilterKernelPermissions(uint32_t perms[MAX_PERM_BIT_MAP_SIZE], uint16_t *kernelPerms, uint32_t* permSize);
bool TransferPermissionToOpcode(const char *permissionName, uint32_t *opCode);
bool TransferOpCodeToPermission(uint32_t opCode, char *permissionName, uint32_t nameSize);

#ifdef __cplusplus
}
#endif

#endif // PERM_SETPROC_C_MOCK_H
