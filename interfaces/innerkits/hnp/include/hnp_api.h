/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef HNP_API_H
#define HNP_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    OPTION_INDEX_FORCE = 0,     /* installed forcely */
    OPTION_INDEX_BUTT
} HnpInstallOptionIndex;

#define HNP_API_ERRNO_BASE 0x2000

// 0x2001 参数非法
#define HNP_API_ERRNO_PARAM_INVALID             (HNP_API_ERRNO_BASE + 0x1)

// 0x2002 fork子进程失败
#define HNP_API_ERRNO_FORK_FAILED               (HNP_API_ERRNO_BASE + 0x2)

// 0x2003 等待子进程退出失败
#define HNP_API_WAIT_PID_FAILED                 (HNP_API_ERRNO_BASE + 0x3)

// 0x2004 非开发者模式
#define HNP_API_NOT_IN_DEVELOPER_MODE           (HNP_API_ERRNO_BASE + 0x4)

// 0x2005 创建管道失败
#define HNP_API_ERRNO_PIPE_CREATED_FAILED       (HNP_API_ERRNO_BASE + 0x5)

// 0x2006 读取管道失败
#define HNP_API_ERRNO_PIPE_READ_FAILED          (HNP_API_ERRNO_BASE + 0x6)

// 0x2007 获取返回码失败
#define HNP_API_ERRNO_RETURN_VALUE_GET_FAILED   (HNP_API_ERRNO_BASE + 0x7)

// 0x2008 移动内存失败
#define HNP_API_ERRNO_MEMMOVE_FAILED            (HNP_API_ERRNO_BASE + 0x8)

#define PACK_NAME_LENTH 256
#define HAP_PATH_LENTH 256
#define ABI_LENTH 128

typedef struct HapInfo {
    char packageName[PACK_NAME_LENTH]; // package name
    char hapPath[HAP_PATH_LENTH];      // hap file path
    char abi[ABI_LENTH];               // system abi
} HapInfo;

/**
 * Install native software package.
 *
 * @param userId Indicates id of user.
 * @param hnpRootPath  Indicates the root path of hnp packages
 * @param hapInfo Indicates the information of HAP.
 * @param installOptions Indicates install options.
 *
 * @return 0:success;other means failure.
 */
int NativeInstallHnp(const char *userId, const char *hnpRootPath, const HapInfo *hapInfo, int installOptions);

/**
 * Uninstall native software package.
 *
 * @param userId Indicates id of user.
 * @param packageName Indicates the packageName of HAP.
 *
 * @return 0:success;other means failure.
 */
int NativeUnInstallHnp(const char *userId, const char *packageName);

#ifdef __cplusplus
}
#endif

#endif