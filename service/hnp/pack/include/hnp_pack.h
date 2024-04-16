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

#ifndef HNP_PACK_H
#define HNP_PACK_H

#include "hnp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

// 0x801201 获取绝对路径失败
#define HNP_ERRNO_PACK_GET_REALPATH_FAILED      HNP_ERRNO_COMMON(HNP_MID_PACK, 0x1)

// 0x801202 组装hnp输出路径失败
#define HNP_ERRNO_PACK_GET_HNP_PATH_FAILED      HNP_ERRNO_COMMON(HNP_MID_PACK, 0x2)

// 0x801203 压缩目录失败
#define HNP_ERRNO_PACK_ZIP_DIR_FAILED           HNP_ERRNO_COMMON(HNP_MID_PACK, 0x3)

/* hnp打包参数 */
typedef struct HnpPackArgvStru {
    char *source;       // 待打包目录
    char *output;       // 打包后文件存放目录
    char *name;         // 软件包名
    char *version;      // 版本号
} HnpPackArgv;

/* hnp打包信息 */
typedef struct HnpPackInfoStru {
    char source[MAX_FILE_PATH_LEN];     // 待打包目录
    char output[MAX_FILE_PATH_LEN];     // 打包后文件存放目录
    HnpCfgInfo cfgInfo;                 // hnp配置信息
    int hnpCfgExist;                    // 是否存在配置文件
} HnpPackInfo;

int HnpCmdPack(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif