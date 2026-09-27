/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "sandbox_clone_app.h"
#include "securec.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_utils.h"
#include "json_utils.h"
#include "sandbox_common.h"
#include "sandbox_core.h"
#include "config_policy_utils.h"
#include <sstream>

#ifdef APPSPAWN_HISYSEVENT
#include "hisysevent_adapter.h"
#endif

namespace OHOS {
namespace AppSpawn {

static const std::string CLONE_APP_GRANT_KEY = "cloneAppGrantDownloadUri";
static const std::string GRANT_URI_KEY = "grantUri";
static const std::string INDEX_MIN_KEY = "indexMin";
static const std::string INDEX_MAX_KEY = "indexMax";

static void DestroyCloneAppGrantUri(ListNode *node)
{
    CloneAppGrantUri *entry = ListEntry(node, CloneAppGrantUri, node);
    OH_ListRemove(&entry->node);
    free(entry);
}

static int CompareExtDataId(ListNode *node, void *data)
{
    AppSpawnExtData *extData = ListEntry(node, AppSpawnExtData, node);
    return extData->dataId - *(uint32_t *)data;
}

static void FreeCloneAppGrantExtData(AppSpawnExtData *data)
{
    APPSPAWN_CHECK_ONLY_EXPER(data != nullptr, return);
    CloneAppGrantExtData *grantData = reinterpret_cast<CloneAppGrantExtData *>(data);
    OH_ListRemove(&grantData->extData.node);
    OH_ListInit(&grantData->extData.node);
    OH_ListRemoveAll(&grantData->grantQueue, DestroyCloneAppGrantUri);
    OH_ListInit(&grantData->grantQueue);
    grantData->grantCount = 0;
    free(grantData);
}

static CloneAppGrantExtData *GetCloneAppGrantExtData(AppSpawnMgr *content)
{
    APPSPAWN_CHECK_ONLY_EXPER(content != nullptr, return nullptr);
    uint32_t dataId = EXT_DATA_CLONE_APP_GRANT;
    ListNode *node = OH_ListFind(&content->extData, &dataId, CompareExtDataId);
    APPSPAWN_CHECK_ONLY_EXPER(node != nullptr, return nullptr);
    AppSpawnExtData *extData = ListEntry(node, AppSpawnExtData, node);
    return reinterpret_cast<CloneAppGrantExtData *>(extData);
}

static CloneAppGrantExtData *CreateCloneAppGrantExtData(AppSpawnMgr *content)
{
    APPSPAWN_CHECK(content != nullptr, return nullptr, "Invalid content");
    CloneAppGrantExtData *grantData = static_cast<CloneAppGrantExtData *>(calloc(1, sizeof(CloneAppGrantExtData)));
    APPSPAWN_CHECK(grantData != nullptr, return nullptr, "Failed to alloc clone app grant ext data");
    OH_ListInit(&grantData->extData.node);
    OH_ListInit(&grantData->grantQueue);
    grantData->configLoaded = false;
    grantData->grantCount = 0;
    grantData->extData.dataId = EXT_DATA_CLONE_APP_GRANT;
    grantData->extData.freeNode = FreeCloneAppGrantExtData;
    grantData->extData.dumpNode = nullptr;
    OH_ListAddTail(&content->extData, &grantData->extData.node);
    return grantData;
}

static int32_t ParseCloneAppGrantItem(cJSON *item, CloneAppGrantExtData *grantData)
{
    APPSPAWN_CHECK_ONLY_EXPER(item != nullptr && cJSON_IsObject(item), return -1);
    cJSON *grantUriNode = cJSON_GetObjectItemCaseSensitive(item, GRANT_URI_KEY.c_str());
    cJSON *indexMinNode = cJSON_GetObjectItemCaseSensitive(item, INDEX_MIN_KEY.c_str());
    cJSON *indexMaxNode = cJSON_GetObjectItemCaseSensitive(item, INDEX_MAX_KEY.c_str());
    if (grantUriNode == nullptr || !cJSON_IsNumber(grantUriNode) ||
        indexMinNode == nullptr || !cJSON_IsNumber(indexMinNode) ||
        indexMaxNode == nullptr || !cJSON_IsNumber(indexMaxNode)) {
        APPSPAWN_LOGW("ParseCloneAppGrantItem: skip item missing or invalid field");
        return -1;
    }

    if (grantUriNode->valueint < 0 || indexMinNode->valueint < 0 || indexMaxNode->valueint < 0) {
        APPSPAWN_LOGW("ParseCloneAppGrantItem: skip item negative value");
        return -1;
    }

    if (indexMinNode->valueint > indexMaxNode->valueint) {
        APPSPAWN_LOGW("ParseCloneAppGrantItem: skip item indexMin > indexMax");
        return -1;
    }
    CloneAppGrantUri *entry = static_cast<CloneAppGrantUri *>(calloc(1, sizeof(CloneAppGrantUri)));
    APPSPAWN_CHECK(entry != nullptr, return 1, "Failed to alloc clone app grant uri");
    OH_ListInit(&entry->node);
    entry->grantUri = static_cast<uint32_t>(grantUriNode->valueint);
    entry->indexMin = static_cast<uint32_t>(indexMinNode->valueint);
    entry->indexMax = static_cast<uint32_t>(indexMaxNode->valueint);
    OH_ListAddTail(&grantData->grantQueue, &entry->node);
    grantData->grantCount++;
    return 0;
}

// JSON格式： {"cloneAppGrantDownloadUri":[{"grantUri": 0 ,"indexMin": 1,"indexMax": 5}]}
static int32_t ReadCloneAppGrantDownloadJson(const std::string &configPath, CloneAppGrantExtData *grantData)
{
    APPSPAWN_CHECK(!configPath.empty() && grantData != nullptr,
        return APPSPAWN_MSG_INVALID, "Invalid configPath or grantData");
    cJSON *grantConfigRoot = GetJsonObjFromFile(configPath.c_str());
    APPSPAWN_CHECK(grantConfigRoot != nullptr, return APPSPAWN_MSG_INVALID,
        "ReadCloneAppGrantDownloadJson: failed to load %{public}s", configPath.c_str());
    APPSPAWN_CHECK_LOGW(cJSON_IsObject(grantConfigRoot),
        cJSON_Delete(grantConfigRoot); return APPSPAWN_MSG_INVALID,
        "ReadCloneAppGrantDownloadJson: root is not an object for %{public}s", configPath.c_str());
    cJSON *grantUriArray = cJSON_GetObjectItemCaseSensitive(grantConfigRoot, CLONE_APP_GRANT_KEY.c_str());
    APPSPAWN_CHECK_LOGW(grantUriArray != nullptr && cJSON_IsArray(grantUriArray),
    cJSON_Delete(grantConfigRoot); return APPSPAWN_MSG_INVALID,
    "ReadCloneAppGrantDownloadJson: cloneAppGrantDownloadUri is not an array");
    int count = cJSON_GetArraySize(grantUriArray);
    for (int i = 0; i < count; i++) {
        cJSON *item = cJSON_GetArrayItem(grantUriArray, i);
        if (ParseCloneAppGrantItem(item, grantData) == 1) {
            break;
        }
    }
    cJSON_Delete(grantConfigRoot);
    return 0;
}

static int32_t MatchGrantUriByAppIndex(const CloneAppGrantExtData *grantData, uint32_t appIndex,
    uint32_t &matchedGrantUri)
{
    ListNode *node = nullptr;
    ForEachListEntry(&grantData->grantQueue, node) {
        CloneAppGrantUri *entry = ListEntry(node, CloneAppGrantUri, node);
        if (appIndex >= entry->indexMin && appIndex <= entry->indexMax) {
            matchedGrantUri = entry->grantUri;
            return 0;
        }
    }
    return -1;
}

int SandboxCore::LoadCloneAppGrantDownloadConfig(AppSpawnMgr *content)
{
    APPSPAWN_CHECK(content != nullptr, return 0, "Invalid content");
    CloneAppGrantExtData *grantData = GetCloneAppGrantExtData(content);
    if (grantData == nullptr) {
        grantData = CreateCloneAppGrantExtData(content);
        APPSPAWN_CHECK(grantData != nullptr, return 0, "Failed to create clone app grant ext data");
    }
    APPSPAWN_CHECK_LOGV(!grantData->configLoaded, return 0,
        "LoadCloneAppGrantDownloadConfig skip reload, rules %{public}u", grantData->grantCount);
    CfgFiles *files = GetCfgFiles(SandboxCommonDef::CLONE_APP_DOWNLOAD_CFG_DIR);
    if (files == nullptr) {
        APPSPAWN_LOGW("LoadCloneAppGrantDownloadConfig: config files not found");
        return 0;
    }
    bool loaded = false;
    for (int i = 0; i < MAX_CFG_POLICY_DIRS_CNT; ++i) {
        APPSPAWN_ONLY_EXPER(files->paths[i] == nullptr, continue);
        std::string path = files->paths[i];
        path += SandboxCommonDef::CLONE_APP_DOWNLOAD_CFG_FILE;
        APPSPAWN_LOGV("LoadCloneAppGrantDownloadConfig %{public}s", path.c_str());
        if (ReadCloneAppGrantDownloadJson(path, grantData) == 0) {
            loaded = true;
        }
    }
    grantData->configLoaded = loaded;
    if (!loaded) {
        APPSPAWN_LOGW("LoadCloneAppGrantDownloadConfig: no valid config file loaded");
#ifdef APPSPAWN_HISYSEVENT
        ReportKeyEvent(CLONE_APP_GRANT_CONFIG_LOAD_FAIL);
#endif
    }
    FreeCfgFiles(files);
    return 0;
}

void SandboxCore::BuildClonePackageName(const AppSpawnMsgBundleInfo *bundleInfo,
    std::ostringstream &clonePackageName)
{
    uint32_t matchedGrantUri = SandboxCommonDef::GRANT_URI_CLONE_NAME;
    CloneAppGrantExtData *grantData = GetCloneAppGrantExtData(GetAppSpawnMgr());
    if (grantData != nullptr && grantData->configLoaded && grantData->grantCount > 0) {
        uint32_t matchIndex = bundleInfo->bundleIndex % SandboxCommonDef::CLONE_APP_INDEX_THRESHOLD;
        int32_t ret = MatchGrantUriByAppIndex(grantData, matchIndex, matchedGrantUri);
        if (ret != 0) {
            APPSPAWN_LOGV("BuildClonePackageName: no matching rule for index %{public}u", matchIndex);
        }
    }
    if (matchedGrantUri == SandboxCommonDef::GRANT_URI_ORIGINAL_NAME) {
        uint32_t floorIndex = bundleInfo->bundleIndex / SandboxCommonDef::CLONE_APP_INDEX_THRESHOLD;
        if (floorIndex > 0) {
            clonePackageName << "+clone-" << (floorIndex * SandboxCommonDef::CLONE_APP_INDEX_THRESHOLD)
                << "+" << bundleInfo->bundleName;
        } else {
            clonePackageName << bundleInfo->bundleName;
        }
    } else {
        clonePackageName << "+clone-" << bundleInfo->bundleIndex << "+" << bundleInfo->bundleName;
    }
}

} // namespace AppSpawn
} // namespace OHOS
