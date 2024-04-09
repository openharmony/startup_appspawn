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

#ifndef HNP_INSTALLER_H
#define HNP_INSTALLER_H

#include "hnp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeHnpPathStru {
    char hnpSoftwareName[MAX_FILE_PATH_LEN];
    char hnpBasePath[MAX_FILE_PATH_LEN];
    char hnpSoftwarePath[MAX_FILE_PATH_LEN];
    char hnpVersionPath[MAX_FILE_PATH_LEN];
} NativeHnpPath;

// 0x801301 组装安装路径失败
#define HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED            HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x1)

// 0x801302 获取安装绝对路径失败
#define HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED            HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x2)

// 0x801303 获取Hnp安装包名称失败
#define HNP_ERRNO_INSTALLER_GET_HNP_NAME_FAILED            HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x3)

// 0x801304 安装的包已存在
#define HNP_ERRNO_INSTALLER_PATH_IS_EXIST                  HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x4)

// 0x801305 获取卸载路径失败
#define HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST           HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x5)

// 0x801306 安装命令参数uid错误
#define HNP_ERRNO_INSTALLER_ARGV_UID_INVALID               HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x6)

// 0x801307 获取版本目录失败
#define HNP_ERRNO_INSTALLER_VERSION_FILE_GET_FAILED        HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x7)

// 0x801308 安装包超过最大值
#define HNP_ERRNO_INSTALLER_SOFTWARE_NUM_OVERSIZE          HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x8)

#define HNP_DEFAULT_INSTALL_ROOT_PATH "/data/app/el1/bundle"

int HnpCmdInstall(int argc, char *argv[]);

int HnpCmdUnInstall(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif