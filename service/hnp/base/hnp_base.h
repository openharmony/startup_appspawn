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
#include <stdbool.h>

#include "securec.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILE_PATH_LEN PATH_MAX

#define HNP_VERSION_LEN 32
#define BUFFER_SIZE 1024
#define HNP_COMMAND_LEN 128
#define MAX_PROCESSES 32
#define MAX_SOFTWARE_NUM 32

#define HNP_CFG_FILE_NAME "hnp.json"

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

/* hnp配置文件信息 */
typedef struct HnpCfgInfoStru {
    char name[MAX_FILE_PATH_LEN];
    char version[HNP_VERSION_LEN];    // Native软件包版本号
    unsigned int linkNum;   // 软链接配置个数
    NativeBinLink *links;
} HnpCfgInfo;

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

// 0x801002 缺少必要的操作参数
#define HNP_ERRNO_OPERATOR_ARGV_MISS            HNP_ERRNO_COMMON(HNP_MID_MAIN, 0x2)

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

// 0x80110e 解压缩打开文件失败
#define HNP_ERRNO_BASE_UNZIP_OPEN_FAILED        HNP_ERRNO_COMMON(HNP_MID_BASE, 0xe)

// 0x80110f 解压缩获取文件信息失败
#define HNP_ERRNO_BASE_UNZIP_GET_INFO_FAILED    HNP_ERRNO_COMMON(HNP_MID_BASE, 0xf)

// 0x801110 解压缩获取文件信息失败
#define HNP_ERRNO_BASE_UNZIP_READ_FAILED        HNP_ERRNO_COMMON(HNP_MID_BASE, 0x10)

// 0x801111 生成软链接失败
#define HNP_ERRNO_GENERATE_SOFT_LINK_FAILED     HNP_ERRNO_COMMON(HNP_MID_BASE, 0x11)

// 0x801112 进程正在运行
#define HNP_ERRNO_PROCESS_RUNNING               HNP_ERRNO_COMMON(HNP_MID_BASE, 0x12)

// 0x801113 入参失败
#define HNP_ERRNO_BASE_PARAMS_INVALID           HNP_ERRNO_COMMON(HNP_MID_BASE, 0x13)

// 0x801114 strdup失败
#define HNP_ERRNO_BASE_STRDUP_FAILED            HNP_ERRNO_COMMON(HNP_MID_BASE, 0x14)

// 0x801115 设置权限失败
#define HNP_ERRNO_BASE_CHMOD_FAILED             HNP_ERRNO_COMMON(HNP_MID_BASE, 0x15)

// 0x801116 删除目录失败
#define HNP_ERRNO_BASE_UNLINK_FAILED            HNP_ERRNO_COMMON(HNP_MID_BASE, 0x16)

// 0x801117 对应进程不存在
#define HNP_ERRNO_BASE_PROCESS_NOT_FOUND        HNP_ERRNO_COMMON(HNP_MID_BASE, 0x17)

// 0x801118 创建路径失败
#define HNP_ERRNO_BASE_MKDIR_PATH_FAILED        HNP_ERRNO_COMMON(HNP_MID_BASE, 0x18)

// 0x801119 读取配置文件流失败
#define HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED  HNP_ERRNO_COMMON(HNP_MID_BASE, 0x19)

// 0x80111a 解析json信息失败
#define HNP_ERRNO_BASE_PARSE_JSON_FAILED        HNP_ERRNO_COMMON(HNP_MID_BASE, 0x1a)

// 0x80111b 未找到json项
#define HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND      HNP_ERRNO_COMMON(HNP_MID_BASE, 0x1b)

// 0x80111c 解析json数组失败
#define HNP_ERRNO_BASE_GET_ARRAY_ITRM_FAILED    HNP_ERRNO_COMMON(HNP_MID_BASE, 0x1c)

int GetFileSizeByHandle(FILE *file, int *size);

int ReadFileToStream(const char *filePath, char **stream, int *streamLen);

int ReadFileToStreamBySize(const char *filePath, char **stream, int readSize);

int GetRealPath(char *srcPath, char *realPath);

int HnpZip(const char *inputDir, const char *outputFile);

int HnpUnZip(const char *inputFile, const char *outputDir);

int HnpAddFileToZip(char *zipfile, char *filename, char *buff, int size);

void HnpLogPrintf(int logLevel, char *module, const char *format, ...);

int HnpCfgGetFromZip(const char *inputFile, HnpCfgInfo *hnpCfg);

int HnpSymlink(const char *srcFile, const char *dstFile);

int HnpProcessRunCheck(const char *runPath);

int HnpDeleteFolder(const char *path);

int HnpCreateFolder(const char* path);

int HnpWriteInfoToFile(const char* filePath, char *buff, int len);

int ParseHnpCfgFile(const char *hnpCfgPath, HnpCfgInfo *hnpCfg);

int GetHnpJsonBuff(HnpCfgInfo *hnpCfg, char **buff);

int HnpCfgGetFromSteam(char *cfgStream, HnpCfgInfo *hnpCfg);

#define HNP_LOGI(args...) \
    HnpLogPrintf(HNP_LOG_INFO, "HNP", ##args)

#define HNP_LOGE(args...) \
    HnpLogPrintf(HNP_LOG_ERROR, "HNP", ##args)

#ifdef __cplusplus
}
#endif

#endif