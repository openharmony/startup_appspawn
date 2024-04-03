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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "securec.h"

#include "hnp_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

static int AddHnpCfgFileToZip(char *zipPath, const char *hnpSrcPath, HnpCfgInfo *hnpCfg)
{
    int ret;
    char *strPtr;
    int offset;
    char hnpCfgFile[MAX_FILE_PATH_LEN];
    char *buff;

    // zip压缩文件内只保存相对路径，不保存绝对路径信息，偏移到压缩文件夹位置
    strPtr = strrchr(hnpSrcPath, DIR_SPLIT_SYMBOL);
    if (strPtr == NULL) {
        offset = 0;
    } else {
        offset = strPtr - hnpSrcPath + 1;
    }

    // zip函数根据后缀是否'/'区分目录还是文件
    ret = sprintf_s(hnpCfgFile, MAX_FILE_PATH_LEN, "%s%c"HNP_CFG_FILE_NAME, hnpSrcPath + offset, DIR_SPLIT_SYMBOL);
    if (ret < 0) {
        HNP_LOGE("sprintf unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    // 根据配置信息生成hnp.json内容
    ret = GetHnpJsonBuff(hnpCfg, &buff);
    if (ret != 0) {
        HNP_LOGE("get hnp json content by cfg info unsuccess.");
        return ret;
    }
    // 将hnp.json文件写入到.hnp压缩文件中
    ret = HnpAddFileToZip(zipPath, hnpCfgFile, buff, strlen(buff) + 1);
    free(buff);
    if (ret != 0) {
        HNP_LOGE("add file to zip failed.zip=%s, file=%s", zipPath, hnpCfgFile);
        return ret;
    }

    return 0;
}

static int PackHnp(const char *hnpSrcPath, const char *hnpDstPath, HnpPackInfo *hnpPack)
{
    int ret;
    char hnp_file_path[MAX_FILE_PATH_LEN];
    HnpCfgInfo *hnpCfg = &hnpPack->cfgInfo;

    HNP_LOGI("PackHnp start. srcPath=%s, hnpName=%s, hnpVer=%s, hnpDstPath=%s ",
        hnpSrcPath, hnpCfg->name, hnpCfg->version, hnpDstPath);

    /* 拼接hnp文件名 */
    ret = sprintf_s(hnp_file_path, MAX_FILE_PATH_LEN, "%s/%s.hnp", hnpDstPath, hnpCfg->name);
    if (ret < 0) {
        HNP_LOGE("sprintf unsuccess.");
        return HNP_ERRNO_PACK_GET_HNP_PATH_FAILED;
    }

    /* 将软件包压缩成独立的.hnp文件 */
    ret = HnpZip(hnpSrcPath, hnp_file_path);
    if (ret != 0) {
        HNP_LOGE("zip dir unsuccess! srcPath=%s, hnpName=%s, hnpVer=%s, hnpDstPath=%s ret=%d",
            hnpSrcPath, hnpCfg->name, hnpCfg->version, hnpDstPath, ret);
        return HNP_ERRNO_PACK_ZIP_DIR_FAILED;
    }

    /* 如果软件包中不存在hnp.json文件，则需要在hnp压缩文件中添加 */
    if (hnpPack->hnpCfgExist == 0) {
        ret = AddHnpCfgFileToZip(hnp_file_path, hnpSrcPath, &hnpPack->cfgInfo);
        if (ret != 0) {
            HNP_LOGE("add file to zip failed ret=%d. zip=%s, src=%s",
                ret, hnp_file_path, hnpSrcPath);
            return ret;
        }
    }

    HNP_LOGI("PackHnp end. srcPath=%s, hnpName=%s, hnpVer=%s, hnpDstPath=%s, linkNum=%d, ret=%d",
        hnpSrcPath, hnpCfg->name, hnpCfg->version, hnpDstPath, hnpCfg->linkNum, ret);

    return ret;
}

static int GetHnpCfgInfo(const char *hnpCfgPath, const char *sourcePath, HnpCfgInfo *hnpCfg)
{
    NativeBinLink *linkArr = NULL;
    char linksource[MAX_FILE_PATH_LEN] = {0};

    int ret = ParseHnpCfgFile(hnpCfgPath, hnpCfg);
    if (ret != 0) {
        HNP_LOGE("parse hnp cfg[%s] unsuccess! ret=%d", hnpCfgPath, ret);
        return ret;
    }
    /* 校验软连接的source文件是否存在 */
    linkArr = hnpCfg->links;
    for (unsigned int i = 0; i < hnpCfg->linkNum; i++, linkArr++) {
        int ret = sprintf_s(linksource, MAX_FILE_PATH_LEN, "%s/%s", sourcePath, linkArr->source);
        if (ret < 0) {
            free(hnpCfg->links);
            hnpCfg->links = NULL;
            HNP_LOGE("sprintf unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        if (access(linksource, F_OK) != 0) {
            free(hnpCfg->links);
            hnpCfg->links = NULL;
            HNP_LOGE("links source[%s] not exist.", linksource);
            return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
        }
    }
    return 0;
}

static int ParsePackArgs(HnpPackArgv *packArgv, HnpPackInfo *packInfo)
{
    char cfgPath[MAX_FILE_PATH_LEN];

    if (packArgv->source == NULL) {
        HNP_LOGE("source dir is null.");
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }
    if (GetRealPath(packArgv->source, packInfo->source) != 0) {
        HNP_LOGE("source dir path=%s is invalid.", packArgv->source);
        return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
    }
    if (packArgv->output == NULL) {
        packArgv->output = ".";
    }

    if (GetRealPath(packArgv->output, packInfo->output) != 0) {
        HNP_LOGE("output dir path=%s is invalid.", packArgv->output);
        return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
    }
    /* 确认hnp.json文件是否存在，存在则对hnp.json文件进行解析并校验内容是否正确 */
    int ret = sprintf_s(cfgPath, MAX_FILE_PATH_LEN, "%s/"HNP_CFG_FILE_NAME, packInfo->source);
    if (ret < 0) {
        HNP_LOGE("sprintf unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    if (access(cfgPath, F_OK) != 0) {
        /* hnp.json文件不存在则要求用户传入name和version信息 */
        if ((packArgv->name == NULL) || (packArgv->version == NULL)) {
            HNP_LOGE("name or version argv is miss.");
            return HNP_ERRNO_OPERATOR_ARGV_MISS;
        }
        if (strcpy_s(packInfo->cfgInfo.name, MAX_FILE_PATH_LEN, packArgv->name) != EOK) {
            HNP_LOGE("strcpy name argv unsuccess.");
            return HNP_ERRNO_BASE_COPY_FAILED;
        }
        if (strcpy_s(packInfo->cfgInfo.version, HNP_VERSION_LEN, packArgv->version) != EOK) {
            HNP_LOGE("strcpy version argv unsuccess.");
            return HNP_ERRNO_BASE_COPY_FAILED;
        }
        packInfo->hnpCfgExist = 0;
    } else {
        ret = GetHnpCfgInfo(cfgPath, packInfo->source, &packInfo->cfgInfo);
        if (ret != 0) {
            return ret;
        }
        packInfo->hnpCfgExist = 1;
    }
    return 0;
}

int HnpCmdPack(int argc, char *argv[])
{
    HnpPackArgv packArgv = {0};
    HnpPackInfo packInfo = {0};
    int opt;

    optind = 1; // 从头开始遍历参数
    while ((opt = getopt_long(argc, argv, "hi:o:n:v:", NULL, NULL)) != -1) {
        switch (opt) {
            case 'h' :
                return HNP_ERRNO_OPERATOR_ARGV_MISS;
            case 'i' :
                packArgv.source = optarg;
                break;
            case 'o' :
                packArgv.output = optarg;
                break;
            case 'n' :
                packArgv.name = optarg;
                break;
            case 'v' :
                packArgv.version = optarg;
                break;
            default:
                break;
        }
    }

    // 解析参数并生成打包信息
    int ret = ParsePackArgs(&packArgv, &packInfo);
    if (ret != 0) {
        return ret;
    }

    // 根据打包信息进行打包操作
    ret = PackHnp(packInfo.source, packInfo.output, &packInfo);

    // 释放软链接占用的内存
    if (packInfo.cfgInfo.links != NULL) {
        free(packInfo.cfgInfo.links);
        packInfo.cfgInfo.links = NULL;
    }

    return ret;
}

#ifdef __cplusplus
}
#endif