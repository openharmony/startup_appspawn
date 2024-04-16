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
#include <stdlib.h>

#include "cJSON.h"
#include "securec.h"

#include "hnp_base.h"

#ifdef __cplusplus
extern "C" {
#endif

static int ParseLinksJsonToCfgInfo(cJSON *linksItem, HnpCfgInfo *hnpCfg)
{
    NativeBinLink *linkArray = NULL;

    int linkArrayNum = cJSON_GetArraySize(linksItem);
    if (linkArrayNum > 0) {
        hnpCfg->linkNum = linkArrayNum;
        linkArray = (NativeBinLink*)malloc(sizeof(NativeBinLink) * linkArrayNum);
        if (linkArray == NULL) {
            HNP_LOGE("malloc unsuccess.");
            return HNP_ERRNO_NOMEM;
        }
        for (int i = 0; i < linkArrayNum; i++) {
            cJSON *link = cJSON_GetArrayItem(linksItem, i);
            if (link == NULL) {
                free(linkArray);
                return HNP_ERRNO_BASE_GET_ARRAY_ITRM_FAILED;
            }
            cJSON *sourceItem = cJSON_GetObjectItem(link, "source");
            if ((sourceItem == NULL) || (sourceItem->valuestring == NULL)) {
                HNP_LOGE("get source info in cfg unsuccess.");
                free(linkArray);
                return HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND;
            }
            if (strcpy_s(linkArray[i].source, MAX_FILE_PATH_LEN, sourceItem->valuestring) != EOK) {
                HNP_LOGE("strcpy unsuccess.");
                free(linkArray);
                return HNP_ERRNO_BASE_COPY_FAILED;
            }
            linkArray[i].target[0] = '\0';  //允许target不填，软链接默认使用原二进制名称
            cJSON *targetItem = cJSON_GetObjectItem(link, "target");
            if ((targetItem != NULL) && (targetItem->valuestring != NULL) &&
                (strcpy_s(linkArray[i].target, MAX_FILE_PATH_LEN, targetItem->valuestring) != EOK)) {
                HNP_LOGE("strcpy unsuccess.");
                free(linkArray);
                return HNP_ERRNO_BASE_COPY_FAILED;
            }
        }
        hnpCfg->links = linkArray;
    } else {
        hnpCfg->linkNum = 0;
    }
    return 0;
}

static int ParseJsonStreamToHnpCfgInfo(cJSON *json, HnpCfgInfo *hnpCfg)
{
    int ret;

    cJSON *typeItem = cJSON_GetObjectItem(json, "type");
    if ((typeItem == NULL) || (typeItem->valuestring == NULL)) {
        HNP_LOGE("get type info in cfg unsuccess.");
        return HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND;
    }
    if (strcmp(typeItem->valuestring, "hnp-config") != 0) {
        HNP_LOGE("type info not match.type=%s", typeItem->valuestring);
        return HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND;
    }
    cJSON *nameItem = cJSON_GetObjectItem(json, "name");
    if ((nameItem == NULL) || (nameItem->valuestring == NULL)) {
        HNP_LOGE("get name info in cfg unsuccess.");
        return HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND;
    }
    ret = strcpy_s(hnpCfg->name, MAX_FILE_PATH_LEN, nameItem->valuestring);
    if (ret != EOK) {
        HNP_LOGE("strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }
    cJSON *versionItem = cJSON_GetObjectItem(json, "version");
    if ((versionItem == NULL) || (versionItem->valuestring == NULL)) {
        HNP_LOGE("get version info in cfg unsuccess.");
        return HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND;
    }
    ret = strcpy_s(hnpCfg->version, HNP_VERSION_LEN, versionItem->valuestring);
    if (ret != EOK) {
        HNP_LOGE("strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }
    cJSON *installItem = cJSON_GetObjectItem(json, "install");
    if (installItem == NULL) {
        HNP_LOGE("get install info in cfg unsuccess.");
        return HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND;
    }
    cJSON *linksItem = cJSON_GetObjectItem(installItem, "links");
    if (linksItem != NULL) {
        ret = ParseLinksJsonToCfgInfo(linksItem, hnpCfg);
        if (ret != 0) {
            return ret;
        }
    } else {
        hnpCfg->linkNum = 0;
    }
    return 0;
}

int ParseHnpCfgFile(const char *hnpCfgPath, HnpCfgInfo *hnpCfg)
{
    int ret;
    char *cfgStream = NULL;
    cJSON *json;
    int size;

    ret = ReadFileToStream(hnpCfgPath, &cfgStream, &size);
    if (ret != 0) {
        HNP_LOGE("read cfg file[%s] unsuccess.", hnpCfgPath);
        return HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED;
    }
    json = cJSON_Parse(cfgStream);
    free(cfgStream);
    if (json == NULL) {
        HNP_LOGE("parse json file[%s] unsuccess.", hnpCfgPath);
        return HNP_ERRNO_BASE_PARSE_JSON_FAILED;
    }
    ret = ParseJsonStreamToHnpCfgInfo(json, hnpCfg);
    cJSON_Delete(json);

    return ret;
}

int HnpCfgGetFromSteam(char *cfgStream, HnpCfgInfo *hnpCfg)
{
    cJSON *json;
    int ret;

    if (cfgStream == NULL) {
        HNP_LOGE("hnp cfg file not found.");
        return HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED;
    }

    json = cJSON_Parse(cfgStream);
    if (json == NULL) {
        HNP_LOGE("parse json file unsuccess.");
        return HNP_ERRNO_BASE_PARSE_JSON_FAILED;
    }
    ret = ParseJsonStreamToHnpCfgInfo(json, hnpCfg);
    cJSON_Delete(json);

    return ret;
}
int GetHnpJsonBuff(HnpCfgInfo *hnpCfg, char **buff)
{
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "type", "hnp-config");
    cJSON_AddStringToObject(root, "name", hnpCfg->name);
    cJSON_AddStringToObject(root, "version", hnpCfg->version);
    cJSON_AddObjectToObject(root, "install");
    char *str = cJSON_Print(root);
    cJSON_Delete(root);

    *buff = str;
    return 0;
}

#ifdef __cplusplus
}
#endif