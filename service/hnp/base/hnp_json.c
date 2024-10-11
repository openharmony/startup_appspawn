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
        hnpCfg->linkNum = (size_t)linkArrayNum;
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
        HNP_LOGE("type info not match.type=%{public}s", typeItem->valuestring);
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
        HNP_LOGE("read cfg file[%{public}s] unsuccess.", hnpCfgPath);
        return HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED;
    }
    json = cJSON_Parse(cfgStream);
    free(cfgStream);
    if (json == NULL) {
        HNP_LOGE("parse json file[%{public}s] unsuccess.", hnpCfgPath);
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

static bool HnpInstallHapExistCheck(const char *hnpPackageName, cJSON *json, cJSON **hapItemOut, int *hapIndex)
{
    cJSON *hapItem = NULL;
    bool hapExist = false;

    for (int i = 0; i < cJSON_GetArraySize(json); i++) {
        hapItem = cJSON_GetArrayItem(json, i);
        if ((cJSON_GetObjectItem(hapItem, "hap") != NULL) &&
            (strcmp(cJSON_GetObjectItem(hapItem, "hap")->valuestring, hnpPackageName) == 0)) {
            hapExist = true;
            *hapItemOut = hapItem;
            *hapIndex = i;
            break;
        }
    }

    return hapExist;
}

static bool HnpInstallHnpExistCheck(cJSON *hnpItemArr, const char *name, cJSON **hnpItemOut, int *hnpIndex,
    const char *version)
{
    if (hnpItemArr == NULL) {
        return false;
    }

    for (int i = 0; i < cJSON_GetArraySize(hnpItemArr); i++) {
        cJSON *hnpItem = cJSON_GetArrayItem(hnpItemArr, i);
        cJSON *nameJson = cJSON_GetObjectItem(hnpItem, "name");
        cJSON *versionJson = cJSON_GetObjectItem(hnpItem, "current_version");
        if ((nameJson != NULL) && (strcmp(nameJson->valuestring, name) == 0)) {
            *hnpItemOut = hnpItem;
            *hnpIndex = i;
            if (version == NULL) {
                return true;
            }
            if ((versionJson != NULL) && (strcmp(versionJson->valuestring, version) == 0)) {
                return true;
            }
        }
    }

    return false;
}

static void HnpPackageVersionUpdateAll(cJSON *json, const HnpCfgInfo *hnpCfg)
{
    for (int i = 0; i < cJSON_GetArraySize(json); i++) {
        cJSON *hapItem = cJSON_GetArrayItem(json, i);
        cJSON *hnpItemArr = cJSON_GetObjectItem(hapItem, "hnp");
        for (int j = 0; j < cJSON_GetArraySize(hnpItemArr); j++) {
            cJSON *hnpItem = cJSON_GetArrayItem(hnpItemArr, j);
            if ((cJSON_GetObjectItem(hnpItem, "name") == NULL) ||
                (strcmp(cJSON_GetObjectItem(hnpItem, "name")->valuestring, hnpCfg->name) != 0)) {
                continue;
            }
            cJSON *version = cJSON_GetObjectItem(hnpItem, "current_version");
            if (version == NULL) {
                break;
            }
            cJSON_SetValuestring(version, hnpCfg->version);
            break;
        }
    }

    return;
}

static int HnpHapJsonWrite(cJSON *json)
{
    FILE *fp = fopen(HNP_PACKAGE_INFO_JSON_FILE_PATH, "wb");
    if (fp == NULL) {
        HNP_LOGE("open file:%{public}s unsuccess!", HNP_PACKAGE_INFO_JSON_FILE_PATH);
        return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
    }
    char *jsonStr = cJSON_Print(json);
    size_t writeLen = fwrite(jsonStr, strlen(jsonStr), sizeof(char), fp);
    (void)fclose(fp);
    free(jsonStr);
    if (writeLen == 0) {
        HNP_LOGE("package info write file:%{public}s unsuccess!", HNP_PACKAGE_INFO_JSON_FILE_PATH);
        return HNP_ERRNO_BASE_FILE_WRITE_FAILED;
    }

    return 0;
}

static int HnpHapJsonHnpAdd(bool hapExist, cJSON *json, cJSON *hapItem, const char *hnpPackageName,
    const HnpCfgInfo *hnpCfg)
{
    cJSON *hnpItemArr = NULL;
    int ret;

    if (hapExist) {
        hnpItemArr = cJSON_GetObjectItem(hapItem, "hnp");
        if (hnpItemArr == NULL) {
            HNP_LOGE("hnp item array get unsuccess");
            return HNP_ERRNO_BASE_PARSE_ITEM_NO_FOUND;
        }
    } else {
        hapItem = cJSON_CreateObject();
        if (hapItem == NULL) {
            HNP_LOGE("hnp json write create hap object unsuccess");
            return HNP_ERRNO_BASE_JSON_ARRAY_CREATE_FAILED;
        }
        cJSON_AddStringToObject(hapItem, "hap", hnpPackageName);
        hnpItemArr = cJSON_CreateArray();
        if (hnpItemArr == NULL) {
            HNP_LOGE("hnp json write array create unsuccess");
            return HNP_ERRNO_BASE_JSON_ARRAY_CREATE_FAILED;
        }
        cJSON_AddItemToObject(hapItem, "hnp", hnpItemArr);
        cJSON_AddItemToArray(json, hapItem);
    }
    cJSON *hnpItem = cJSON_CreateObject();
    if (hnpItem == NULL) {
        HNP_LOGE("hnp json write create hnp object unsuccess");
        return HNP_ERRNO_BASE_JSON_ARRAY_CREATE_FAILED;
    }
    cJSON_AddItemToObject(hnpItem, "name", cJSON_CreateString(hnpCfg->name));
    cJSON_AddItemToObject(hnpItem, "current_version", cJSON_CreateString(hnpCfg->version));
    if (hnpCfg->isInstall) {
        cJSON_AddItemToObject(hnpItem, "install_version", cJSON_CreateString(hnpCfg->version));
    } else {
        cJSON_AddItemToObject(hnpItem, "install_version", cJSON_CreateString("none"));
    }
    cJSON_AddItemToArray(hnpItemArr, hnpItem);

    HnpPackageVersionUpdateAll(json, hnpCfg);

    ret = HnpHapJsonWrite(json);
    return ret;
}

int HnpInstallInfoJsonWrite(const char *hapPackageName, const HnpCfgInfo *hnpCfg)
{
    bool hapExist = false;
    int hapIndex = 0;
    int hnpIndex = 0;
    char *infoStream;
    int size;
    cJSON *hapItem = NULL;
    cJSON *hnpItem = NULL;
    cJSON *json = NULL;

    if ((hapPackageName == NULL) || (hnpCfg == NULL)) {
        return HNP_ERRNO_BASE_PARAMS_INVALID;
    }

    int ret = ReadFileToStream(HNP_PACKAGE_INFO_JSON_FILE_PATH, &infoStream, &size);
    if (ret != 0) {
        if ((ret == HNP_ERRNO_BASE_FILE_OPEN_FAILED) || (ret == HNP_ERRNO_BASE_GET_FILE_LEN_NULL)) {
            if ((json = cJSON_CreateArray()) == NULL) {
                HNP_LOGE("hnp json write array create unsuccess");
                return HNP_ERRNO_BASE_JSON_ARRAY_CREATE_FAILED;
            }
        } else {
            HNP_LOGE("hnp json write read hnp info file unsuccess");
            return HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED;
        }
    } else {
        json = cJSON_Parse(infoStream);
        free(infoStream);
        if (json == NULL) {
            HNP_LOGE("hnp json write parse json file unsuccess.");
            return HNP_ERRNO_BASE_PARSE_JSON_FAILED;
        }
        hapExist = HnpInstallHapExistCheck(hapPackageName, json, &hapItem, &hapIndex);
    }

    if (hapExist) {
        cJSON *hnpItemArr = cJSON_GetObjectItem(hapItem, "hnp");
        bool hnpExist = HnpInstallHnpExistCheck(hnpItemArr, hnpCfg->name, &hnpItem, &hnpIndex, NULL);
        if (hnpExist) {
            cJSON *versionJson = cJSON_GetObjectItem(hnpItem, "current_version");
            if (versionJson != NULL) { // 当前版本存在，即非新增版本，仅更新current_version即可，无需更新install_version
                cJSON_SetValuestring(versionJson, hnpCfg->version);
                HnpPackageVersionUpdateAll(json, hnpCfg);
                ret = HnpHapJsonWrite(json);
                cJSON_Delete(json);
                return ret;
            }
        }
    }

    ret = HnpHapJsonHnpAdd(hapExist, json, hapItem, hapPackageName, hnpCfg);
    cJSON_Delete(json);
    return ret;
}

static bool HnpOtherPackageInstallCheck(const char *name, const char *version, int packageIndex, cJSON *json)
{
    bool hnpExist = false;
    int hnpIndex = 0;

    for (int i = 0; i < cJSON_GetArraySize(json); i++) {
        if (i == packageIndex) {
            continue;
        }
        cJSON *hapItem = cJSON_GetArrayItem(json, i);
        cJSON *hnpItemArr = cJSON_GetObjectItem(hapItem, "hnp");
        cJSON *hnpItem = NULL;
        hnpExist = HnpInstallHnpExistCheck(hnpItemArr, name, &hnpItem, &hnpIndex, version);
        if (hnpExist) {
            return true;
        }
    }

    return false;
}

static int HnpPackageInfoGetOut(HnpPackageInfo *packageInfos, int sum, HnpPackageInfo **packageInfoOut, int *count)
{
    HnpPackageInfo *ptr;

    if (sum == 0) {
        return 0;
    }

    ptr = malloc(sizeof(HnpPackageInfo) * sum);
    if (ptr == NULL) {
        HNP_LOGE("malloc hnp info unsuccess.");
        return HNP_ERRNO_NOMEM;
    }

    if (memcpy_s(ptr, sizeof(HnpPackageInfo) * sum, packageInfos, sizeof(HnpPackageInfo) * sum) != 0) {
        free(ptr);
        HNP_LOGE("memcpy hnp info unsuccess.");
        return HNP_ERRNO_BASE_MEMCPY_FAILED;
    }

    *packageInfoOut = ptr;
    *count = sum;
    return 0;
}

static int HnpPackageJsonGet(cJSON **pJson)
{
    char *infoStream;
    int size;

    int ret = ReadFileToStream(HNP_PACKAGE_INFO_JSON_FILE_PATH, &infoStream, &size);
    if (ret != 0) {
        if (ret == HNP_ERRNO_BASE_FILE_OPEN_FAILED || ret == HNP_ERRNO_BASE_GET_FILE_LEN_NULL) {
            return 0;
        }
        HNP_LOGE("package info get read hnp info file unsuccess");
        return HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED;
    }

    cJSON *json = cJSON_Parse(infoStream);
    free(infoStream);
    if (json == NULL) {
        HNP_LOGE("package info get parse json file unsuccess.");
        return HNP_ERRNO_BASE_PARSE_JSON_FAILED;
    }

    *pJson = json;

    return 0;
}

int HnpPackageInfoGet(const char *packageName, HnpPackageInfo **packageInfoOut, int *count)
{
    bool hnpExist = false;
    int hapIndex = 0;
    HnpPackageInfo packageInfos[MAX_PACKAGE_HNP_NUM] = {0};
    int sum = 0;
    cJSON *json = NULL;

    int ret = HnpPackageJsonGet(&json);
    if (ret != 0 || json == NULL) {
        return ret;
    }

    cJSON *hapItem = NULL;
    if (HnpInstallHapExistCheck(packageName, json, &hapItem, &hapIndex) == false) {
        cJSON_Delete(json);
        return 0;
    }

    cJSON *hnpItemArr = cJSON_GetObjectItem(hapItem, "hnp");
    for (int j = 0; j < cJSON_GetArraySize(hnpItemArr); j++) {
        cJSON *hnpItem = cJSON_GetArrayItem(hnpItemArr, j);
        cJSON *name = cJSON_GetObjectItem(hnpItem, "name");
        cJSON *version = cJSON_GetObjectItem(hnpItem, "current_version");
        cJSON *installVersion = cJSON_GetObjectItem(hnpItem, "install_version");
        if (name == NULL || version == NULL || installVersion == NULL) {
            continue;
        }
        hnpExist = HnpOtherPackageInstallCheck(name->valuestring, version->valuestring, hapIndex, json);
        // 当卸载当前版本未被其他hap使用或者存在安装版本的时候，需要卸载对应的当前版本或者安装版本
        if (!hnpExist || strcmp(installVersion->valuestring, "none") != 0) {
            if ((strcpy_s(packageInfos[sum].name, MAX_FILE_PATH_LEN, name->valuestring) != EOK) ||
                (strcpy_s(packageInfos[sum].currentVersion, HNP_VERSION_LEN, version->valuestring) != EOK) ||
                (strcpy_s(packageInfos[sum].installVersion, HNP_VERSION_LEN, installVersion->valuestring) != EOK)) {
                HNP_LOGE("strcpy hnp info name[%{public}s],version[%{public}s],install version[%{public}s] unsuccess.",
                    name->valuestring, version->valuestring, installVersion->valuestring);
                cJSON_Delete(json);
                return HNP_ERRNO_BASE_COPY_FAILED;
            }
            packageInfos[sum].hnpExist = hnpExist;
            sum++;
        }
    }
    cJSON_Delete(json);

    return HnpPackageInfoGetOut(packageInfos, sum, packageInfoOut, count);
}

int HnpPackageInfoHnpDelete(const char *packageName, const char *name, const char *version)
{
    char *infoStream;
    int size;
    cJSON *hapItem = NULL;
    cJSON *hnpItem = NULL;
    int hapIndex = 0;
    bool hapExist = false;
    int hnpIndex = 0;
    bool hnpExist = false;

    int ret = ReadFileToStream(HNP_PACKAGE_INFO_JSON_FILE_PATH, &infoStream, &size);
    if (ret != 0) {
        if (ret == HNP_ERRNO_BASE_FILE_OPEN_FAILED || ret == HNP_ERRNO_BASE_GET_FILE_LEN_NULL) {
            return 0;
        } else {
            HNP_LOGE("hnp delete read hnp info file unsuccess");
            return HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED;
        }
    }

    cJSON *json = cJSON_Parse(infoStream);
    free(infoStream);
    if (json == NULL) {
        HNP_LOGE("hnp delete parse json file unsuccess.");
        return HNP_ERRNO_BASE_PARSE_JSON_FAILED;
    }

    hapExist = HnpInstallHapExistCheck(packageName, json, &hapItem, &hapIndex);
    if (!hapExist) {
        cJSON_Delete(json);
        return 0;
    }

    cJSON *hnpItemArr = cJSON_GetObjectItem(hapItem, "hnp");
    hnpExist = HnpInstallHnpExistCheck(hnpItemArr, name, &hnpItem, &hnpIndex, version);
    if (hnpExist) {
        cJSON_DeleteItemFromArray(hnpItemArr, hnpIndex);
    }

    ret = HnpHapJsonWrite(json);
    cJSON_Delete(json);
    return ret;
}

int HnpPackageInfoDelete(const char *packageName)
{
    char *infoStream;
    int size;
    cJSON *hapItem = NULL;
    int hapIndex = 0;
    bool hapExist = false;

    int ret = ReadFileToStream(HNP_PACKAGE_INFO_JSON_FILE_PATH, &infoStream, &size);
    if (ret != 0) {
        if (ret == HNP_ERRNO_BASE_FILE_OPEN_FAILED || ret == HNP_ERRNO_BASE_GET_FILE_LEN_NULL) {
            return 0;
        }
        HNP_LOGE("package info delete read hnp info file unsuccess");
        return HNP_ERRNO_BASE_READ_FILE_STREAM_FAILED;
    }

    cJSON *json = cJSON_Parse(infoStream);
    free(infoStream);
    if (json == NULL) {
        HNP_LOGE("package info delete parse json file unsuccess.");
        return HNP_ERRNO_BASE_PARSE_JSON_FAILED;
    }

    hapExist = HnpInstallHapExistCheck(packageName, json, &hapItem, &hapIndex);
    if (hapExist) {
        cJSON_DeleteItemFromArray(json, hapIndex);
    }

    ret = HnpHapJsonWrite(json);
    cJSON_Delete(json);
    return ret;
}

static char *HnpNeedUnInstallHnpVersionGet(cJSON *hnpItemArr, const char *name)
{
    if (hnpItemArr == NULL) {
        return NULL;
    }

    for (int i = 0; i < cJSON_GetArraySize(hnpItemArr); i++) {
        cJSON *hnpItem = cJSON_GetArrayItem(hnpItemArr, i);
        if ((cJSON_GetObjectItem(hnpItem, "name") != NULL) &&
            (cJSON_GetObjectItem(hnpItem, "current_version") != NULL) &&
            (cJSON_GetObjectItem(hnpItem, "install_version") != NULL) &&
            (strcmp(cJSON_GetObjectItem(hnpItem, "name")->valuestring, name) == 0) &&
            (strcmp(cJSON_GetObjectItem(hnpItem, "current_version")->valuestring,
            cJSON_GetObjectItem(hnpItem, "install_version")->valuestring) == 0)) {
            return cJSON_GetObjectItem(hnpItem, "current_version")->valuestring;
        }
    }

    return NULL;
}

char *HnpCurrentVersionGet(const char *name)
{
    char *infoStream;
    int size;
    cJSON *hapItem = NULL;

    int ret = ReadFileToStream(HNP_PACKAGE_INFO_JSON_FILE_PATH, &infoStream, &size);
    if (ret != 0) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(infoStream);
    free(infoStream);
    if (json == NULL) {
        HNP_LOGE("hnp delete parse json file unsuccess.");
        return NULL;
    }

    for (int i = 0; i < cJSON_GetArraySize(json); i++) {
        hapItem = cJSON_GetArrayItem(json, i);
        cJSON *hnpItemArr = cJSON_GetObjectItem(hapItem, "hnp");
        if (hnpItemArr == NULL) {
            return NULL;
        }

        for (int i = 0; i < cJSON_GetArraySize(hnpItemArr); i++) {
            cJSON *hnpItem = cJSON_GetArrayItem(hnpItemArr, i);
            if ((cJSON_GetObjectItem(hnpItem, "name") != NULL) &&
                (cJSON_GetObjectItem(hnpItem, "current_version") != NULL) &&
                (strcmp(cJSON_GetObjectItem(hnpItem, "name")->valuestring, name) == 0)) {
                return cJSON_GetObjectItem(hnpItem, "current_version")->valuestring;
            }
        }
    }

    cJSON_Delete(json);
    return NULL;
}

char *HnpCurrentVersionUninstallCheck(const char *name)
{
    char *infoStream;
    int size;
    cJSON *hapItem = NULL;
    char *version = NULL;

    int ret = ReadFileToStream(HNP_PACKAGE_INFO_JSON_FILE_PATH, &infoStream, &size);
    if (ret != 0) {
        return NULL;
    }

    cJSON *json = cJSON_Parse(infoStream);
    free(infoStream);
    if (json == NULL) {
        HNP_LOGE("hnp delete parse json file unsuccess.");
        return NULL;
    }

    for (int i = 0; i < cJSON_GetArraySize(json); i++) {
        hapItem = cJSON_GetArrayItem(json, i);
        cJSON *hnpItemArr = cJSON_GetObjectItem(hapItem, "hnp");
        version = HnpNeedUnInstallHnpVersionGet(hnpItemArr, name);
        if (version != NULL) {
            break;
        }
    }

    cJSON_Delete(json);
    return version;
}

#ifdef __cplusplus
}
#endif