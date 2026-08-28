/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SANDBOX_DEC_H
#define SANDBOX_DEC_H

#include <sys/ioctl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "appspawn_hook.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define DEV_DEC_MINOR 0x25
#define HM_DEC_IOCTL_BASE 's'
#define HM_SET_POLICY_ID 1
#define HM_DEL_POLICY_ID 2
#define HM_QUERY_POLICY_ID 3
#define HM_CHECK_POLICY_ID 4
#define HM_DESTORY_POLICY_ID 5
#define HM_CONSTRAINT_POLICY_ID 6
#define HM_DENY_POLICY_ID 7
#define HM_SET_PREFIX_ID 8
#define HM_SET_DEC_IGNORE_CASE_ID 12

// All ioctl cmd macros are defined internally in sandbox_dec.c (based on IoctlDecPolicyBatch),
// external code must use the public functions instead of calling ioctl directly.

#define MAX_POLICY_NUM 256
// Kernel ABI: paths per single ioctl (IoctlDecPolicyBatch.path[]). Do NOT modify.
#define KERNEL_BATCH_SIZE 8
// Max rules collected per config (independent of KERNEL_BATCH_SIZE, adjustable >= 1).
#define MAX_CONFIG_POLICY_NUM 32
#define SANDBOX_MODE_READ  0x00000001
#define SANDBOX_MODE_WRITE (SANDBOX_MODE_READ << 1)
#define DEC_MODE_DENY_INHERIT (1 << 9)

#define DEC_POLICY_HEADER_RESERVED 64

typedef struct PathInfo {
    char *path;
    uint32_t pathLen;
    uint32_t mode;
    bool flag;
} PathInfo;

typedef struct DecPolicyInfo {
    uint64_t tokenId;
    uint64_t timestamp;
    PathInfo path[MAX_CONFIG_POLICY_NUM];
    uint32_t pathNum;
    int32_t userId;
    uint64_t reserved[DEC_POLICY_HEADER_RESERVED];
    bool flag;
} DecPolicyInfo;

typedef struct GlobalDecPolicyInfo {
    uint64_t tokenId;
    uint64_t timestamp;
    PathInfo path[MAX_POLICY_NUM];
    uint32_t pathNum;
    int32_t userId;
    uint64_t reserved[DEC_POLICY_HEADER_RESERVED];
    bool flag;
} GlobalDecPolicyInfo;

typedef struct DecDenyPathTemplate {
    const char *permission;
    const char *decPath;
} DecDenyPathTemplate;

void SetDecPolicyInfos(DecPolicyInfo *decPolicyInfos);
void DestroyDecPolicyInfos(GlobalDecPolicyInfo *globalDecPolicyInfos);
void SetDecPolicy(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
