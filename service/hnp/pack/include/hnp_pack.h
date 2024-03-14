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

// 0x801201 打包命令参数错误
#define HNP_ERRNO_PACK_ARGV_NUM_INVALID         HNP_ERRNO_COMMON(HNP_MID_PACK, 0x1)

// 0x801202 获取绝对路径失败
#define HNP_ERRNO_PACK_GET_REALPATH_FAILED      HNP_ERRNO_COMMON(HNP_MID_PACK, 0x2)

// 0x801203 缺少操作参数
#define HNP_ERRNO_PACK_MISS_OPERATOR_PARAM      HNP_ERRNO_COMMON(HNP_MID_PACK, 0x3)

// 0x801204 读取配置文件流失败
#define HNP_ERRNO_PACK_READ_FILE_STREAM_FAILED  HNP_ERRNO_COMMON(HNP_MID_PACK, 0x4)

// 0x801205 解析json信息失败
#define HNP_ERRNO_PACK_PARSE_JSON_FAILED        HNP_ERRNO_COMMON(HNP_MID_PACK, 0x5)

// 0x801206 未找到json项
#define HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND      HNP_ERRNO_COMMON(HNP_MID_PACK, 0x6)

// 0x801207 解析json数组失败
#define HNP_ERRNO_PACK_GET_ARRAY_ITRM_FAILED    HNP_ERRNO_COMMON(HNP_MID_PACK, 0x7)

// 0x801208 组装hnp输出路径失败
#define HNP_ERRNO_PACK_GET_HNP_PATH_FAILED      HNP_ERRNO_COMMON(HNP_MID_PACK, 0x8)

// 0x801209 压缩目录失败
#define HNP_ERRNO_PACK_ZIP_DIR_FAILED           HNP_ERRNO_COMMON(HNP_MID_PACK, 0x9)

int HnpCmdPack(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif