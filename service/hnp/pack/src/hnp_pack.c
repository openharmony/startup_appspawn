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

#include "securec.h"

#include "hnp_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

static int PackHnp(const char *hnpSrcPath, const char *hnpDstPath, HnpCfgInfo *hnpCfg)
{
    int ret;
    char hnp_file_path[MAX_FILE_PATH_LEN];

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

    HNP_LOGI("PackHnp end. srcPath=%s, hnpName=%s, hnpVer=%s, hnpDstPath=%s, linkNum=%d, ret=%d",
        hnpSrcPath, hnpCfg->name, hnpCfg->version, hnpDstPath, hnpCfg->linkNum, ret);

    return ret;
}

int GetHnpCfgInfo(const char *hnpCfgPath, const char *sourcePath, HnpCfgInfo *hnpCfg)
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
            return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
        }
    }
    return 0;
}

int ParsePackArgs(HnpPackArgv *hnpPackArgv, HnpPackInfo *hnpPackInfo)
{
    char cfgPath[MAX_FILE_PATH_LEN];

    if (hnpPackArgv->source == NULL) {
        HNP_LOGE("source dir is null.");
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }
    if (GetRealPath(hnpPackArgv->source, hnpPackInfo->source) != 0) {
        HNP_LOGE("source dir path=%s is invalid.", hnpPackArgv->source);
        return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
    }
    if (hnpPackArgv->output != NULL) {
        if (GetRealPath(hnpPackArgv->output, hnpPackInfo->output) != 0) {
            HNP_LOGE("output dir path=%s is invalid.", hnpPackArgv->output);
            return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
        }
    } else {
        if (GetRealPath(".", hnpPackInfo->output) != 0) {
            HNP_LOGE("output dir path=. is invalid.");
            return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
        }
    }

    int ret = sprintf_s(cfgPath, MAX_FILE_PATH_LEN, "%s/hnp.json", hnpPackInfo->source);
    if (ret < 0) {
        HNP_LOGE("sprintf unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    if (access(cfgPath, F_OK) != 0) {
        if ((hnpPackArgv->name == NULL) || (hnpPackArgv->version == NULL)) {
            HNP_LOGE("name or version argv is miss.");
            return HNP_ERRNO_OPERATOR_ARGV_MISS;
        }
        strcpy_s(hnpPackInfo->cfgInfo.name, MAX_FILE_PATH_LEN, hnpPackArgv->name);
        strcpy_s(hnpPackInfo->cfgInfo.version, HNP_VERSION_LEN, hnpPackArgv->version);
        ret = CreateHnpJsonFile(cfgPath, &hnpPackInfo->cfgInfo);
        if (ret != 0) {
            return ret;
        }
    } else {
        ret = GetHnpCfgInfo(cfgPath, hnpPackInfo->source, &hnpPackInfo->cfgInfo);
        if (ret != 0) {
            return ret;
        }
    }
    return 0;
}

int HnpCmdPack(int argc, char *argv[])
{
    HnpPackArgv hnpPackArgv = {0};
    HnpPackInfo hnpPackInfo = {0};
    int opt;

    optind = 1;  // 从头开始遍历参数
    while ((opt = getopt(argc, argv, "hi:o:n:v:")) != -1) {
        switch (opt) {
            case 'h' :
                return HNP_ERRNO_OPERATOR_ARGV_MISS;
            case 'i' :
                hnpPackArgv.source = optarg;
                break;
            case 'o' :
                hnpPackArgv.output = optarg;
                break;
            case 'n' :
                hnpPackArgv.name = optarg;
                break;
            case 'v' :
                hnpPackArgv.version = optarg;
                break;
            default:
                break;
        }
    }
    // 解析参数并生成打包信息
    int ret = ParsePackArgs(&hnpPackArgv, &hnpPackInfo);
    if (ret != 0) {
        return ret;
    }

    // 根据打包信息进行打包操作
    ret = PackHnp(hnpPackInfo.source, hnpPackInfo.output, &hnpPackInfo.cfgInfo);
    if (ret != 0) {
        return ret;
    }

    // 释放软链接占用的内存
    if (hnpPackInfo.cfgInfo.links != NULL) {
        free(hnpPackInfo.cfgInfo.links);
        hnpPackInfo.cfgInfo.links = NULL;
    }
    return 0;
}

#ifdef __cplusplus
}
#endif