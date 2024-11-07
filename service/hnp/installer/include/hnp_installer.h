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

// 0x801301 组装安装路径失败
#define HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED            HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x1)

// 0x801302 获取安装绝对路径失败
#define HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED            HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x2)

// 0x801303 ELF文件验签失败
#define HNP_ERRNO_INSTALLER_CODE_SIGN_APP_FAILED           HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x3)

// 0x801304 安装的包已存在
#define HNP_ERRNO_INSTALLER_PATH_IS_EXIST                  HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x4)

// 0x801305 获取卸载路径失败
#define HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST           HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x5)

// 0x801306 安装命令参数uid错误
#define HNP_ERRNO_INSTALLER_ARGV_UID_INVALID               HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x6)

// 0x801307 restorecon 安装目录失败
#define HNP_ERRNO_INSTALLER_RESTORECON_HNP_PATH_FAIL       HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x7)

// 0x801308 安装包中的二进制文件过多
#define HNP_ERRNO_INSTALLER_HAP_FILE_COUNT_OVER            HNP_ERRNO_COMMON(HNP_MID_INSTALLER, 0x8)

#define HNP_DEFAULT_INSTALL_ROOT_PATH "/data/app/el1/bundle"
#define HNP_SANDBOX_BASE_PATH "/data/service/hnp"

/* hap安装信息 */
typedef struct HapInstallInfoStru {
    int uid;                                  // 用户id
    char *hapPackageName;                     // app名称
    char *hnpRootPath;                        // hnp安装目录
    char *hapPath;                            // hap目录
    char *abi;                                // 系统abi路径
    bool isForce;                             // 是否强制安装
} HapInstallInfo;

/* hnp安装信息 */
typedef struct HnpInstallInfoStru {
    HapInstallInfo *hapInstallInfo;           // hap安装信息
    bool isPublic;                            // 是否公有
    char hnpBasePath[MAX_FILE_PATH_LEN];      // hnp安装基础路径,public为 xxx/{uid}/hnppublic,private为xxx/{uid}/hnp/{hap}
    char hnpSoftwarePath[MAX_FILE_PATH_LEN];  // 软件安装路径，为hnpBasePath/{name}.org/
    char hnpVersionPath[MAX_FILE_PATH_LEN];   // 软件安装版本路径，为hnpBasePath/{name}.org/{name}_{version}
    char hnpSignKeyPrefix[MAX_FILE_PATH_LEN]; // hnp包验签前缀,hnp/{abi}/xxxx/xxx.hnp
} HnpInstallInfo;

int HnpCmdInstall(int argc, char *argv[]);

int HnpCmdUnInstall(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif