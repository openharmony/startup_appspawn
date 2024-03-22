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

#ifndef HNP_BASE_H
#define HNP_BASE_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "securec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILE_PATH_LEN PATH_MAX

#define HNP_HEAD_MAGIC 0x12345678
#define HNP_HEAD_VERSION 1
#define HNP_VERSION_LEN 32

#ifdef _WIN32
#define DIR_SPLIT_SYMBOL '\\'
#else
#define DIR_SPLIT_SYMBOL '/'
#endif

/* Native软件二进制软链接配置 */
typedef struct NativeBinLinkStru {
    char source[MAX_FILE_PATH_LEN];
    char target[MAX_FILE_PATH_LEN];
} NativeBinLink;

/* hnp文件头结构 */
typedef struct NativeHnpHeadStru {
    unsigned int magic;     // 魔术字校验
    unsigned int version;   // 版本号
    unsigned int headLen;   // hnp结构头大小
    unsigned int reserve;   // 预留字段
    char hnpVersion[HNP_VERSION_LEN];    // Native软件包版本号
    unsigned int linkNum;   // 软链接配置个数
    NativeBinLink links[0];
} NativeHnpHead;

/* 日志级别 */
typedef enum  {
    HNP_LOG_INFO    = 0,
    HNP_LOG_WARN    = 1,
    HNP_LOG_ERROR   = 2,
    HNP_LOG_DEBUG   = 3,
    HNP_LOG_BUTT
} HNP_LOG_LEVEL_E;

/* 数字索引 */
enum {
    HNP_INDEX_0 = 0,
    HNP_INDEX_1,
    HNP_INDEX_2,
    HNP_INDEX_3,
    HNP_INDEX_4,
    HNP_INDEX_5,
    HNP_INDEX_6,
    HNP_INDEX_7
};


// 错误码生成
#define HNP_ERRNO_HNP_MID               0x80
#define HNP_ERRNO_HIGH16_MAKE()  (HNP_ERRNO_HNP_MID << 16)
#define HNP_ERRNO_LOW16_MAKE(Mid, Errno)  (((Mid) << 8) + (Errno))
#define HNP_ERRNO_COMMON(Mid, Errno) (HNP_ERRNO_HIGH16_MAKE() | HNP_ERRNO_LOW16_MAKE(Mid, Errno))

#define HNP_ERRNO_PARAM_INVALID     0x22
#define HNP_ERRNO_NOMEM             0x23

enum {
    HNP_MID_MAIN        = 0x10,
    HNP_MID_BASE        = 0x11,
    HNP_MID_PACK        = 0x12,
    HNP_MID_INSTALLER   = 0x13
};

/* hnp_main模块*/
// 0x801001 操作类型非法
#define HNP_ERRNO_OPERATOR_TYPE_INVALID         HNP_ERRNO_COMMON(HNP_MID_MAIN, 0x1)

/* hnp_base模块*/
// 0x801101 打开文件失败
#define HNP_ERRNO_BASE_FILE_OPEN_FAILED         HNP_ERRNO_COMMON(HNP_MID_BASE, 0x1)

// 0x801102 读取文件失败
#define HNP_ERRNO_BASE_FILE_READ_FAILED         HNP_ERRNO_COMMON(HNP_MID_BASE, 0x2)

// 0x801103 fseek设置失败
#define HNP_ERRNO_BASE_FILE_SEEK_FAILED         HNP_ERRNO_COMMON(HNP_MID_BASE, 0x3)

// 0x801104 ftell设置失败
#define HNP_ERRNO_BASE_FILE_TELL_FAILED         HNP_ERRNO_COMMON(HNP_MID_BASE, 0x4)

// 0x801105 realpath失败
#define HNP_ERRNO_BASE_REALPATHL_FAILED         HNP_ERRNO_COMMON(HNP_MID_BASE, 0x5)

// 0x801106 获取文件大小为0
#define HNP_ERRNO_BASE_GET_FILE_LEN_NULL        HNP_ERRNO_COMMON(HNP_MID_BASE, 0x6)

// 0x801107 字符串大小超出限制
#define HNP_ERRNO_BASE_STRING_LEN_OVER_LIMIT    HNP_ERRNO_COMMON(HNP_MID_BASE, 0x7)

// 0x801108 目录打开失败
#define HNP_ERRNO_BASE_DIR_OPEN_FAILED          HNP_ERRNO_COMMON(HNP_MID_BASE, 0x8)

// 0x801109 sprintf拼装失败
#define HNP_ERRNO_BASE_SPRINTF_FAILED           HNP_ERRNO_COMMON(HNP_MID_BASE, 0x9)

// 0x80110a 生成压缩文件失败
#define HNP_ERRNO_BASE_CREATE_ZIP_FAILED        HNP_ERRNO_COMMON(HNP_MID_BASE, 0xa)

// 0x80110b 写文件失败
#define HNP_ERRNO_BASE_FILE_WRITE_FAILED        HNP_ERRNO_COMMON(HNP_MID_BASE, 0xb)

// 0x80110c 拷贝失败
#define HNP_ERRNO_BASE_COPY_FAILED              HNP_ERRNO_COMMON(HNP_MID_BASE, 0xc)

// 0x80110d 获取文件属性失败
#define HNP_ERRNO_GET_FILE_ATTR_FAILED          HNP_ERRNO_COMMON(HNP_MID_BASE, 0xd)

int GetFileSizeByHandle(FILE *file, int *size);

int ReadFileToStream(const char *filePath, char **stream, int *streamLen);

int GetRealPath(char *srcPath, char *realPath);

int HnpZip(const char *inputDir, const char *outputFile);

int HnpUnZip(const char *inputFile, const char *outputDir);

int HnpWriteToZipHead(const char *zipFile, char *buff, int len);

void HnpLogPrintf(int logLevel, char *module, const char *format, ...);

#define HNP_LOGI(args...) \
    HnpLogPrintf(HNP_LOG_INFO, "HNP", ##args)

#define HNP_LOGE(args...) \
    HnpLogPrintf(HNP_LOG_ERROR, "HNP", ##args)

#ifdef __cplusplus
}
#endif

#endif