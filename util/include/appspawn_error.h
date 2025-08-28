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

#ifndef APPSPAWN_ERROR_H
#define APPSPAWN_ERROR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// 错误码定义规范：子系统ID（27-21位）| 特性(20位) 0:appspawn,1:init| 模块ID（19-16位）| 具体错误ID（15-0位）
#define OHOS_SUBSYS_STARTUP_ID   39
#define OHOS_SUBSYSTEM_BIT_NUM   21
#define OHOS_MODULE_BIT_NUM      16
#define OHOS_SUBMODULE_BIT_NUM   12

// appspawn特性中模块码定义
#define DECLARE_APPSPAWN_ERRORCODE_BASE(module) \
        ((uint32_t)(((module) & 0x0f) << OHOS_MODULE_BIT_NUM) | (OHOS_SUBSYS_STARTUP_ID << OHOS_SUBSYSTEM_BIT_NUM))

#define DECLARE_APPSPAWN_ERRORCODE_SUBMODULE_BASE(module, submodule) \
        ((uint32_t)(DECLARE_APPSPAWN_ERRORCODE_BASE(module) | (((submodule) & 0x0f) << OHOS_SUBMODULE_BIT_NUM)))

#define DECLARE_APPSPAWN_ERRORCODE(module, submodule, error) \
        ((uint32_t)(DECLARE_APPSPAWN_ERRORCODE_SUBMODULE_BASE((module), (submodule)) | ((error) & 0x0fff)))

/* 错误码与模块映射关系：
 * APPSPAWN_UTILS:        0x04E00000 ~ 0x04E0FFFF appspawn公共调试日志错误码范围
 * APPSPAWN_SPAWNER:      0x04E10000 ~ 0x04E1FFFF 孵化器调试日志错误码范围
 * APPSPAWN_SANDBOX:      0x04E20000 ~ 0x04E2FFFF 沙箱调试日志错误码范围
 * APPSPAWN_HNP:          0x04E30000 ~ 0x04E3FFFF hnp调试日志错误码范围
 * APPSPAWN_DEVICE_DEBUG: 0x04E40000 ~ 0x04E4FFFF devicedebug调试日志错误码范围
*/

typedef enum {
    APPSPAWN_UTILS = 0,       // 公共模块
    APPSPAWN_SPAWNER,         // spawner模块
    APPSPAWN_SANDBOX,         // 沙箱模块
    APPSPAWN_HNP,             // hnp模块
    APPSPAWN_DEVICE_DEBUG,    // devicedebug模块
} AppSpawnErrorCodeModuleType;

typedef enum {
    APPSPAWN_UTILS_COMMON = 0,        // utils-common 模块
    APPSPAWN_UTILS_ARCH,
    APPSPAWN_SUB_MODULE_UTILS_COUNT
} AppSpawnSubModuleUtils;

typedef enum {
    APPSPAWN_SPAWNER_COMMON = 0,        // spawner-common 模块
    APPSPAWN_SUB_MODULE_SPAWNER_COUNT
} AppSpawnSubModuleSpawner;

typedef enum {
    APPSPAWN_SANDBOX_COMMON = 0,        // sandbox-common 模块
    APPSPAWN_SUB_MODULE_SANDBOX_COUNT
} AppSpawnSubModuleSandbox;

typedef enum {
    APPSPAWN_HNP_COMMON = 0,        // hnp-common 模块
    APPSPAWN_SUB_MODULE_HNP_COUNT
} AppSpawnSubModuleHnp;

typedef enum {
    APPSPAWN_DEVICEDEBUG_COMMON = 0,        // devicedebug-common 模块
    APPSPAWN_SUB_MODULE_DEVICEDEBUG_COUNT
} AppSpawnSubModuleDeviceDebug;

typedef enum {
    APPSPAWN_ERROR_MSG_TO_LONG = DECLARE_APPSPAWN_ERRORCODE(APPSPAWN_SPAWNER, APPSPAWN_UTILS_COMMON, 0x0000),
    APPSPAWN_ERROR_MSG_INVALID,
} SpawnerErrorCode;

typedef enum {
    SANDBOX_ERROR_ARGS_INVALID = DECLARE_APPSPAWN_ERRORCODE(APPSPAWN_SANDBOX, APPSPAWN_SANDBOX_COMMON, 0x0000),
    SANDBOX_ERROR_MSG_INVALID,
} SandboxErrorCode;

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // APPSPAWN_ERROR_H