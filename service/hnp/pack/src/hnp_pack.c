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

#include "securec.h"
#include "cJSON.h"

#include "hnp_pack.h"

#ifdef __cplusplus
extern "C" {
#endif

static int PackHnp(const char *hnpSrcPath, const char *hnpDstPath, const char *hnpName, NativeHnpHead *hnpHead)
{
    int ret;
    char hnp_file_path[MAX_FILE_PATH_LEN];

    HNP_LOGI("PackHnp start. srcPath=%s, hnpName=%s, hnpVer=%s, hnpDstPath=%s ",
        hnpSrcPath, hnpName, hnpHead->hnpVersion, hnpDstPath);
    
    /* 拼接hnp文件名 */
    ret = sprintf_s(hnp_file_path, MAX_FILE_PATH_LEN, "%s/%s.hnp", hnpDstPath, hnpName);
    if (ret < 0) {
        HNP_LOGE("sprintf unsuccess.");
        return HNP_ERRNO_PACK_GET_HNP_PATH_FAILED;
    }

    /* 将软件包压缩成独立的.hnp文件 */
    ret = HnpZip(hnpSrcPath, hnp_file_path);
    if (ret != 0) {
        HNP_LOGE("zip dir unsuccess! srcPath=%s, hnpName=%s, hnpVer=%s, hnpDstPath=%s ret=%d",
            hnpSrcPath, hnpName, hnpHead->hnpVersion, hnpDstPath, ret);
        return HNP_ERRNO_PACK_ZIP_DIR_FAILED;
    }

    hnpHead->magic = HNP_HEAD_MAGIC;
    hnpHead->version = HNP_HEAD_VERSION;
    hnpHead->headLen = sizeof(NativeHnpHead) + hnpHead->linkNum * sizeof(NativeBinLink);

    /* 向生成的.hnp文件头写入配置信息 */
    ret = HnpWriteToZipHead(hnp_file_path, (char*)hnpHead, hnpHead->headLen);

    HNP_LOGI("PackHnp end. srcPath=%s, hnpName=%s, hnpVer=%s, hnpDstPath=%s headlen=%d, linkNum=%d, ret=%d",
        hnpSrcPath, hnpName, hnpHead->hnpVersion, hnpDstPath, hnpHead->headLen, hnpHead->linkNum, ret);

    return ret;
}

static int ParseLinksJsonToHnpHead(cJSON *linksItem, NativeHnpHead *hnpHead, NativeBinLink **linkArr)
{
    NativeBinLink *linkArray = NULL;
    int i;

    int linkArrayNum = cJSON_GetArraySize(linksItem);
    if (linkArrayNum != 0) {
        hnpHead->linkNum = linkArrayNum;
        linkArray = (NativeBinLink*)malloc(sizeof(NativeBinLink) * linkArrayNum);
        if (linkArray == NULL) {
            HNP_LOGE("malloc unsuccess.");
            return HNP_ERRNO_NOMEM;
        }
        for (i = 0; i < linkArrayNum; i++) {
            cJSON *link = cJSON_GetArrayItem(linksItem, i);
            if (link == NULL) {
                free(linkArray);
                return HNP_ERRNO_PACK_GET_ARRAY_ITRM_FAILED;
            }
            cJSON *sourceItem = cJSON_GetObjectItem(link, "source");
            if (sourceItem == NULL) {
                HNP_LOGE("get source info in cfg unsuccess.");
                free(linkArray);
                return HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND;
            }
            if (strcpy_s(linkArray[i].source, MAX_FILE_PATH_LEN, sourceItem->valuestring) != EOK) {
                HNP_LOGE("strcpy unsuccess.");
                free(linkArray);
                return HNP_ERRNO_BASE_COPY_FAILED;
            }
            cJSON *targetItem = cJSON_GetObjectItem(link, "target");
            if (targetItem == NULL) {
                HNP_LOGE("get target info in cfg unsuccess.");
                free(linkArray);
                return HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND;
            }
            if (strcpy_s(linkArray[i].target, MAX_FILE_PATH_LEN, targetItem->valuestring) != EOK) {
                HNP_LOGE("strcpy unsuccess.");
                free(linkArray);
                return HNP_ERRNO_BASE_COPY_FAILED;
            }
        }
        *linkArr = linkArray;
    } else {
        hnpHead->linkNum = 0;
    }
    return 0;
}

static int ParseJsonStreamToHnpHead(cJSON *json, char *name, NativeHnpHead *hnpHead, NativeBinLink **linkArr)
{
    int ret;

    cJSON *typeItem = cJSON_GetObjectItem(json, "type");
    if (typeItem == NULL) {
        HNP_LOGE("get type info in cfg unsuccess.");
        return HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND;
    }
    if (strcmp(typeItem->valuestring, "hnp-config") != 0) {
        HNP_LOGE("type info not match.type=%s", typeItem->valuestring);
        return HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND;
    }
    cJSON *nameItem = cJSON_GetObjectItem(json, "name");
    if (nameItem == NULL) {
        HNP_LOGE("get name info in cfg unsuccess.");
        return HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND;
    }
    ret = strcpy_s(name, MAX_FILE_PATH_LEN, nameItem->valuestring);
    if (ret != EOK) {
        HNP_LOGE("strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }
    cJSON *versionItem = cJSON_GetObjectItem(json, "version");
    if (versionItem == NULL) {
        HNP_LOGE("get version info in cfg unsuccess.");
        return HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND;
    }
    ret = strcpy_s(hnpHead->hnpVersion, HNP_VERSION_LEN, versionItem->valuestring);
    if (ret != EOK) {
        HNP_LOGE("strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }
    cJSON *installItem = cJSON_GetObjectItem(json, "install");
    if (installItem == NULL) {
        HNP_LOGE("get install info in cfg unsuccess.");
        return HNP_ERRNO_PACK_PARSE_ITEM_NO_FOUND;
    }
    cJSON *linksItem = cJSON_GetObjectItem(installItem, "links");
    if (linksItem != NULL) {
        ret = ParseLinksJsonToHnpHead(linksItem, hnpHead, linkArr);
        if (ret != 0) {
            return ret;
        }
    } else {
        hnpHead->linkNum = 0;
    }
    return 0;
}

static int ParseHnpCfgToHead(const char *hnpCfgPath, char *name, NativeHnpHead *hnpHead, NativeBinLink **linkArr)
{
    int ret;
    char *cfgStream = NULL;
    cJSON *json;
    int size;

    ret = ReadFileToStream(hnpCfgPath, &cfgStream, &size);
    if (ret != 0) {
        HNP_LOGE("read cfg file[%s] unsuccess.", hnpCfgPath);
        return HNP_ERRNO_PACK_READ_FILE_STREAM_FAILED;
    }
    json = cJSON_Parse(cfgStream);
    free(cfgStream);
    if (json == NULL) {
        HNP_LOGE("parse json file[%s] unsuccess.", hnpCfgPath);
        return HNP_ERRNO_PACK_PARSE_JSON_FAILED;
    }
    ret = ParseJsonStreamToHnpHead(json, name, hnpHead, linkArr);
    cJSON_Delete(json);

    return ret;
}

static int PackHnpWithCfg(const char *hnpSrcPath, const char *hnpDstPath, const char *hnpCfgPath)
{
    NativeHnpHead head;
    NativeHnpHead *hnpHead;
    NativeBinLink *linkArr = NULL;
    char name[MAX_FILE_PATH_LEN] = {0};
    int ret;

    HNP_LOGI("pack hnp start. srcPath=%s, hnpCfg=%s, hnpDstPath=%s",
        hnpSrcPath, hnpCfgPath, hnpDstPath);

    ret = ParseHnpCfgToHead(hnpCfgPath, name, &head, &linkArr);
    if (ret != 0) {
        HNP_LOGE("parse hnp cfg[%s] unsuccess! ret=%d", hnpCfgPath, ret);
        return ret;
    }
    int headLen = sizeof(NativeHnpHead) + head.linkNum * sizeof(NativeBinLink);

    if (head.linkNum != 0) {
        hnpHead = (NativeHnpHead*)malloc(headLen);
        if (hnpHead == NULL) {
            free(linkArr);
            return HNP_ERRNO_NOMEM;
        }
        (void)memcpy_s(hnpHead, headLen, &head, sizeof(NativeHnpHead));
        (void)memcpy_s(hnpHead->links, sizeof(NativeBinLink) * head.linkNum,
            linkArr, sizeof(NativeBinLink) * head.linkNum);
    } else {
        hnpHead = &head;
    }

    HNP_LOGI("pack hnp end. ret=%d, hnpName=%s, version=%s, linksNum=%d",
        ret, name, hnpHead->hnpVersion, hnpHead->linkNum);

    ret = PackHnp(hnpSrcPath, hnpDstPath, name, hnpHead);

    if (head.linkNum != 0) {
        free(linkArr);
        free(hnpHead);
    }
    
    return ret;
}

int HnpCmdPack(int argc, char *argv[])
{
    int index = 0;
    char srcPath[MAX_FILE_PATH_LEN];
    char dstPath[MAX_FILE_PATH_LEN];
    char cfgPath[MAX_FILE_PATH_LEN] = {0};
    char *name = NULL;
    char *version = NULL;

    if (argc < HNP_INDEX_4) {
        HNP_LOGE("pack args num[%u] unsuccess!", argc);
        return HNP_ERRNO_PACK_ARGV_NUM_INVALID;
    }

    if ((GetRealPath(argv[HNP_INDEX_2], srcPath) != 0) || (GetRealPath(argv[HNP_INDEX_3], dstPath) != 0)) {
        HNP_LOGE("pack path invalid! srcPath=%s, dstPath=%s", argv[HNP_INDEX_2], argv[HNP_INDEX_3]);
        return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
    }

    index = HNP_INDEX_4;
    while (index < argc) {
        if ((!strcmp(argv[index], "-name")) && (index + 1 < argc)) {
            name = argv[++index];
        } else if ((!strcmp(argv[index], "-v")) && (index + 1 < argc)) {
            version = argv[++index];
        } else if ((!strcmp(argv[index], "-cfg")) && (index + 1 < argc)) {
            if (GetRealPath(argv[++index], cfgPath) != 0) {
                HNP_LOGE("cfg path[%s] invalid!", argv[index]);
                return HNP_ERRNO_PACK_GET_REALPATH_FAILED;
            }
        }
        index++;
    }

    if ((name != NULL) && (version != NULL)) {
        NativeHnpHead hnpHead;
        hnpHead.linkNum = 0;
        if (strcpy_s(hnpHead.hnpVersion, HNP_VERSION_LEN, version) != EOK) {
            HNP_LOGE("strcpy unsuccess.");
            return HNP_ERRNO_BASE_COPY_FAILED;
        }
        return PackHnp(srcPath, dstPath, name, &hnpHead);
    }

    if (cfgPath[0] != '\0') {
        return PackHnpWithCfg(srcPath, dstPath, cfgPath);
    }

    HNP_LOGE("pack args parse miss! \r\n");

    return HNP_ERRNO_PACK_MISS_OPERATOR_PARAM;
}

#ifdef __cplusplus
}
#endif