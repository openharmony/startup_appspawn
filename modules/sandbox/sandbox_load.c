/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/mount.h>
#include <sys/types.h>

#include "appspawn_msg.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "cJSON.h"
#include "init_utils.h"
#include "json_utils.h"
#include "parameter.h"
#include "securec.h"

static const SandboxFlagInfo NAMESPACE_FLAGS_MAP[] = {
    {"pid", CLONE_NEWPID}, {"net", CLONE_NEWNET}
};

static const SandboxFlagInfo FLAGE_POINT_MAP[] = {
    {"0", 0},
    {"START_FLAGS_BACKUP", (unsigned long)APP_FLAGS_BACKUP_EXTENSION},
    {"DLP_MANAGER", (unsigned long)APP_FLAGS_DLP_MANAGER},
    {"DEVELOPER_MODE", (unsigned long)APP_FLAGS_DEVELOPER_MODE}
};

static const SandboxFlagInfo MOUNT_MODE_MAP[] = {
    {"not-exists", (unsigned long)MOUNT_MODE_NOT_EXIST},
    {"always", (unsigned long)MOUNT_MODE_ALWAYS}
};

static const SandboxFlagInfo NAME_GROUP_TYPE_MAP[] = {
    {"system-const", (unsigned long)SANDBOX_TAG_SYSTEM_CONST},
    {"app-variable", (unsigned long)SANDBOX_TAG_APP_VARIABLE}
};

static inline PathMountNode *CreatePathMountNode(uint32_t type, uint32_t hasDemandInfo)
{
    uint32_t len = hasDemandInfo ? sizeof(PathDemandInfo) : 0;
    return (PathMountNode *)CreateSandboxMountNode(sizeof(PathMountNode) + len, type);
}

static inline SymbolLinkNode *CreateSymbolLinkNode(void)
{
    return (SymbolLinkNode *)CreateSandboxMountNode(sizeof(SymbolLinkNode), SANDBOX_TAG_SYMLINK);
}

static inline SandboxPackageNameNode *CreateSandboxPackageNameNode(const char *name)
{
    return (SandboxPackageNameNode *)CreateSandboxSection(name,
        sizeof(SandboxPackageNameNode), SANDBOX_TAG_PACKAGE_NAME);
}

static inline SandboxFlagsNode *CreateSandboxFlagsNode(const char *name)
{
    return (SandboxFlagsNode *)CreateSandboxSection(name, sizeof(SandboxFlagsNode), SANDBOX_TAG_SPAWN_FLAGS);
}

static inline SandboxNameGroupNode *CreateSandboxNameGroupNode(const char *name)
{
    return (SandboxNameGroupNode *)CreateSandboxSection(name, sizeof(SandboxNameGroupNode), SANDBOX_TAG_NAME_GROUP);
}

static inline SandboxPermissionNode *CreateSandboxPermissionNode(const char *name)
{
    size_t len = sizeof(SandboxPermissionNode);
    SandboxPermissionNode *node = (SandboxPermissionNode *)CreateSandboxSection(name, len, SANDBOX_TAG_PERMISSION);
    APPSPAWN_CHECK(node != NULL, return NULL, "Failed to create permission node");
    node->permissionIndex = 0;
    return node;
}

static inline bool GetBoolParameter(const char *param, bool value)
{
    char tmp[32] = {0};  // 32 max
    int ret = GetParameter(param, "", tmp, sizeof(tmp));
    APPSPAWN_LOGV("GetBoolParameter key %{public}s ret %{public}d result: %{public}s", param, ret, tmp);
    if (ret > 0 && strcmp(tmp, "false") == 0) {
        return false;
    }
    if (ret > 0 && strcmp(tmp, "true") == 0) {
        return true;
    }
    return value;
}

static inline bool AppSandboxPidNsIsSupport(void)
{
    // only set false, return false
    return GetBoolParameter("const.sandbox.pidns.support", true);
}

static inline bool CheckAppFullMountEnable(void)
{
    return GetBoolParameter("const.filemanager.full_mount.enable", false);
}

static unsigned long GetMountModeFromConfig(const cJSON *config, const char *key, unsigned long def)
{
    char *value = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(config, key));
    if (value == NULL) {
        return def;
    }
    const SandboxFlagInfo *info = GetSandboxFlagInfo(value, MOUNT_MODE_MAP, ARRAY_LENGTH(MOUNT_MODE_MAP));
    if (info != NULL) {
        return info->flags;
    }
    return def;
}

static uint32_t GetNameGroupTypeFromConfig(const cJSON *config, const char *key, unsigned long def)
{
    char *value = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(config, key));
    if (value == NULL) {
        return def;
    }
    const SandboxFlagInfo *info = GetSandboxFlagInfo(value, NAME_GROUP_TYPE_MAP, ARRAY_LENGTH(NAME_GROUP_TYPE_MAP));
    if (info != NULL) {
        return (uint32_t)info->flags;
    }
    return def;
}

static uint32_t GetSandboxNsFlags(const cJSON *appConfig)
{
    uint32_t nsFlags = 0;
    cJSON *obj = cJSON_GetObjectItemCaseSensitive(appConfig, "sandbox-ns-flags");
    if (obj == NULL || !cJSON_IsArray(obj)) {
        return nsFlags;
    }
    int count = cJSON_GetArraySize(obj);
    for (int i = 0; i < count; i++) {
        char *value = cJSON_GetStringValue(cJSON_GetArrayItem(obj, i));
        const SandboxFlagInfo *info = GetSandboxFlagInfo(value, NAMESPACE_FLAGS_MAP, ARRAY_LENGTH(NAMESPACE_FLAGS_MAP));
        if (info != NULL) {
            nsFlags |= info->flags;
        }
    }
    return nsFlags;
}

static int SetMode(const char *str, void *context)
{
    mode_t *mode = (mode_t *)context;
    *mode |= GetPathMode(str);
    return 0;
}

static mode_t GetChmodFromJson(const cJSON *config)
{
    mode_t mode = 0;
    char *modeStrs = GetStringFromJsonObj(config, "dest-mode");
    if (modeStrs == NULL) {
        return mode;
    }
    (void)StringSplit(modeStrs, "|", (void *)&mode, SetMode);
    return mode;
}

static uint32_t GetFlagIndexFromJson(const cJSON *config)
{
    char *flagStr = GetStringFromJsonObj(config, "name");
    if (flagStr == NULL) {
        return 0;
    }
    const SandboxFlagInfo *info = GetSandboxFlagInfo(flagStr, FLAGE_POINT_MAP, ARRAY_LENGTH(FLAGE_POINT_MAP));
    if (info != NULL) {
        return info->flags;
    }
    return 0;
}

static void FillPathDemandInfo(const cJSON *config, PathMountNode *sandboxNode)
{
    APPSPAWN_CHECK_ONLY_EXPER(config != NULL, return);
    sandboxNode->demandInfo->uid = GetIntValueFromJsonObj(config, "uid", -1);
    sandboxNode->demandInfo->gid = GetIntValueFromJsonObj(config, "gid", -1);
    sandboxNode->demandInfo->mode = GetIntValueFromJsonObj(config, "ugo", -1);
}

static PathMountNode *DecodeMountPathConfig(const SandboxSection *section, const cJSON *config, uint32_t type)
{
    char *srcPath = GetStringFromJsonObj(config, "src-path");
    char *dstPath = GetStringFromJsonObj(config, "sandbox-path");
    if (srcPath == NULL || dstPath == NULL) {
        return NULL;
    }
    PathMountNode *tmp = GetPathMountNode(section, type, srcPath, dstPath);
    if (tmp != NULL) { // 删除老的节点，保存新的节点
        DeleteSandboxMountNode((SandboxMountNode *)tmp);
        APPSPAWN_LOGW("path %{public}s %{public}s repeat config, delete old", srcPath, dstPath);
    }

    cJSON *demandInfo = cJSON_GetObjectItemCaseSensitive(config, "create-on-demand");
    PathMountNode *sandboxNode = CreatePathMountNode(type, demandInfo != NULL);
    APPSPAWN_CHECK_ONLY_EXPER(sandboxNode != NULL, return NULL);
    sandboxNode->createDemand = demandInfo != NULL;
    sandboxNode->source = strdup(srcPath);
    sandboxNode->target = strdup(dstPath);

    sandboxNode->destMode = GetChmodFromJson(config);
    sandboxNode->mountSharedFlag = GetBoolValueFromJsonObj(config, "mount-shared-flag", false);
    sandboxNode->checkErrorFlag = GetBoolValueFromJsonObj(config, "check-action-status", false);

    sandboxNode->category = GetMountCategory(GetStringFromJsonObj(config, "category"));
    const char *value = GetStringFromJsonObj(config, "app-apl-name");
    if (value != NULL) {
        sandboxNode->appAplName = strdup(value);
    }
    FillPathDemandInfo(demandInfo, sandboxNode);

    if (sandboxNode->source == NULL || sandboxNode->target == NULL) {
        APPSPAWN_LOGE("Failed to get sourc or target path");
        DeleteSandboxMountNode((SandboxMountNode *)sandboxNode);
        return NULL;
    }
    return sandboxNode;
}

static int ParseMountPathsConfig(AppSpawnSandboxCfg *sandbox,
    const cJSON *mountConfigs, SandboxSection *section, uint32_t type)
{
    APPSPAWN_CHECK_ONLY_EXPER(mountConfigs != NULL && cJSON_IsArray(mountConfigs), return -1);

    uint32_t mountPointSize = cJSON_GetArraySize(mountConfigs);
    for (uint32_t i = 0; i < mountPointSize; i++) {
        cJSON *mntJson = cJSON_GetArrayItem(mountConfigs, i);
        if (mntJson == NULL) {
            continue;
        }
        PathMountNode *sandboxNode = DecodeMountPathConfig(section, mntJson, type);
        APPSPAWN_CHECK_ONLY_EXPER(sandboxNode != NULL, continue);
        AddSandboxMountNode(&sandboxNode->sandboxNode, section);
    }
    return 0;
}

static SymbolLinkNode *DecodeSymbolLinksConfig(const SandboxSection *section, const cJSON *config)
{
    const char *target = GetStringFromJsonObj(config, "target-name");
    const char *linkName = GetStringFromJsonObj(config, "link-name");
    if (target == NULL || linkName == NULL) {
        return NULL;
    }

    SymbolLinkNode *tmp = GetSymbolLinkNode(section, target, linkName);
    if (tmp != NULL) { // 删除老的节点，保存新的节点
        DeleteSandboxMountNode((SandboxMountNode *)tmp);
        APPSPAWN_LOGW("SymbolLink %{public}s %{public}s repeat config, delete old", target, linkName);
    }

    SymbolLinkNode *node = CreateSymbolLinkNode();
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return NULL);
    node->destMode = GetChmodFromJson(config);
    node->checkErrorFlag = GetBoolValueFromJsonObj(config, "check-action-status", false);
    node->target = strdup(target);
    node->linkName = strdup(linkName);
    if (node->target == NULL || node->linkName == NULL) {
        APPSPAWN_LOGE("Failed to get sourc or target path");
        DeleteSandboxMountNode((SandboxMountNode *)node);
        return NULL;
    }
    return node;
}

static int ParseSymbolLinksConfig(AppSpawnSandboxCfg *sandbox, const cJSON *symbolLinkConfigs, SandboxSection *section)
{
    APPSPAWN_CHECK_ONLY_EXPER(symbolLinkConfigs != NULL && cJSON_IsArray(symbolLinkConfigs), return -1);
    uint32_t symlinkPointSize = cJSON_GetArraySize(symbolLinkConfigs);
    for (uint32_t i = 0; i < symlinkPointSize; i++) {
        cJSON *symConfig = cJSON_GetArrayItem(symbolLinkConfigs, i);
        if (symConfig == NULL) {
            continue;
        }
        SymbolLinkNode *node = DecodeSymbolLinksConfig(section, symConfig);
        APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return -1);
        AddSandboxMountNode(&node->sandboxNode, section);
    }
    return 0;
}

static int ParseGidTableConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, SandboxSection *section)
{
    APPSPAWN_CHECK(cJSON_IsArray(configs), return 0, "json is not array.");
    uint32_t arrayLen = (uint32_t)cJSON_GetArraySize(configs);
    APPSPAWN_CHECK_ONLY_EXPER(arrayLen > 0, return 0);
    APPSPAWN_CHECK(arrayLen < APP_MAX_GIDS, arrayLen = APP_MAX_GIDS, "More gid in gids json.");

    // 配置存在，以后面的配置为准
    if (section->gidTable) {
        free(section->gidTable);
        section->gidTable = NULL;
        section->gidCount = 0;
    }
    section->gidTable = (gid_t *)calloc(1, sizeof(gid_t) * arrayLen);
    APPSPAWN_CHECK(section->gidTable != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to alloc memory.");

    for (uint32_t i = 0; i < arrayLen; i++) {
        cJSON *item = cJSON_GetArrayItem(configs, i);
        gid_t gid = 0;
        if (cJSON_IsNumber(item)) {
            gid = (gid_t)cJSON_GetNumberValue(item);
        } else {
            char *value = cJSON_GetStringValue(item);
            gid = DecodeGid(value);
        }
        if (gid <= 0) {
            continue;
        }
        section->gidTable[section->gidCount++] = gid;
    }
    return 0;
}

static int ParseMountGroupsConfig(AppSpawnSandboxCfg *sandbox, const cJSON *groupConfig, SandboxSection *section)
{
    APPSPAWN_CHECK(cJSON_IsArray(groupConfig),
        return APPSPAWN_SANDBOX_INVALID, "Invalid mount-groups config %{public}s", section->name);

    // 合并name-group
    uint32_t count = (uint32_t)cJSON_GetArraySize(groupConfig);
    APPSPAWN_LOGV("mount-group in section %{public}s  %{public}u", section->name, count);
    APPSPAWN_CHECK_ONLY_EXPER(count > 0, return 0);
    count += section->number;
    SandboxMountNode **nameGroups = (SandboxMountNode **)calloc(1, sizeof(SandboxMountNode *) * count);
    APPSPAWN_CHECK(nameGroups != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to alloc memory for group name");

    uint32_t j = 0;
    uint32_t number = 0;
    for (j = 0; j < section->number; j++) { // copy old
        if (section->nameGroups[j] == NULL) {
            continue;
        }
        nameGroups[number++] = section->nameGroups[j];
    }

    SandboxNameGroupNode *mountNode = NULL;
    for (uint32_t i = 0; i < count; i++) {
        nameGroups[number] = NULL;

        char *name = cJSON_GetStringValue(cJSON_GetArrayItem(groupConfig, i));
        mountNode = (SandboxNameGroupNode *)GetSandboxSection(&sandbox->nameGroupsQueue, name);
        if (mountNode == NULL) {
            APPSPAWN_LOGE("Can not find name-group %{public}s", name);
            continue;
        }
        // check type
        if (strcmp(section->name, "system-const") == 0 && mountNode->destType != SANDBOX_TAG_SYSTEM_CONST) {
            APPSPAWN_LOGE("Invalid name-group %{public}s", name);
            continue;
        }
        // 过滤重复的节点
        for (j = 0; j < section->number; j++) {
            if (section->nameGroups[j] != NULL && section->nameGroups[j] == (SandboxMountNode *)mountNode) {
                APPSPAWN_LOGE("Name-group %{public}s bas been set", name);
                break;
            }
        }
        if (j < section->number) {
            continue;
        }
        nameGroups[number++] = (SandboxMountNode *)mountNode;
        APPSPAWN_LOGV("Name-group %{public}d %{public}s set", section->number, name);
    }
    if (section->nameGroups != NULL) {
        free(section->nameGroups);
    }
    section->nameGroups = nameGroups;
    section->number = number;
    APPSPAWN_LOGV("mount-group in section %{public}s  %{public}u", section->name, section->number);
    return 0;
}

static int ParseBaseConfig(AppSpawnSandboxCfg *sandbox, SandboxSection *section, const cJSON *configs)
{
    APPSPAWN_CHECK_ONLY_EXPER(configs != NULL, return 0);
    APPSPAWN_CHECK(cJSON_IsObject(configs),
        return APPSPAWN_SANDBOX_INVALID, "Invalid config %{public}s", section->name);
    APPSPAWN_LOGV("Parse sandbox %{public}s", section->name);
    // "sandbox-switch": "ON", default sandbox switch is on
    section->sandboxSwitch = GetBoolValueFromJsonObj(configs, "sandbox-switch", true);
    // "sandbox-shared"
    section->sandboxShared = GetBoolValueFromJsonObj(configs, "sandbox-shared", false);

    int ret = 0;
    cJSON *gidTabJson = cJSON_GetObjectItemCaseSensitive(configs, "gids");
    if (gidTabJson) {
        ret = ParseGidTableConfig(sandbox, gidTabJson, section);
        APPSPAWN_CHECK(ret == 0, return ret, "Parse gids for %{public}s", section->name);
    }
    cJSON *pathConfigs = cJSON_GetObjectItemCaseSensitive(configs, "mount-paths");
    if (pathConfigs != NULL) {  // mount-paths
        ret = ParseMountPathsConfig(sandbox, pathConfigs, section, SANDBOX_TAG_MOUNT_PATH);
        APPSPAWN_CHECK(ret == 0, return ret, "Parse mount-paths for %{public}s", section->name);
    }
    pathConfigs = cJSON_GetObjectItemCaseSensitive(configs, "mount-files");
    if (pathConfigs != NULL) {  // mount-files
        ret = ParseMountPathsConfig(sandbox, pathConfigs, section, SANDBOX_TAG_MOUNT_FILE);
        APPSPAWN_CHECK(ret == 0, return ret, "Parse mount-paths for %{public}s", section->name);
    }
    pathConfigs = cJSON_GetObjectItemCaseSensitive(configs, "symbol-links");
    if (pathConfigs != NULL) {  // symbol-links
        ret = ParseSymbolLinksConfig(sandbox, pathConfigs, section);
        APPSPAWN_CHECK(ret == 0, return ret, "Parse symbol-links for %{public}s", section->name);
    }
    cJSON *groupConfig = cJSON_GetObjectItemCaseSensitive(configs, "mount-groups");
    if (groupConfig != NULL) {
        ret = ParseMountGroupsConfig(sandbox, groupConfig, section);
        APPSPAWN_CHECK(ret == 0, return ret, "Parse mount-groups for %{public}s", section->name);
    }
    return 0;
}

static int ParsePackageNameConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *packageNameConfigs)
{
    APPSPAWN_LOGV("Parse package-name config %{public}s", name);
    SandboxPackageNameNode *node = (SandboxPackageNameNode *)GetSandboxSection(&sandbox->packageNameQueue, name);
    if (node == NULL) {
        node = CreateSandboxPackageNameNode(name);
    }
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return -1);

    int ret = ParseBaseConfig(sandbox, &node->section, packageNameConfigs);
    if (ret != 0) {
        DeleteSandboxSection((SandboxSection *)node);
        return ret;
    }
    // success, insert section
    AddSandboxSection(&node->section, &sandbox->packageNameQueue);
    return 0;
}

static int ParseSpawnFlagsConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *flagsConfig)
{
    uint32_t flagIndex = GetFlagIndexFromJson(flagsConfig);
    APPSPAWN_LOGV("Parse spawn-flags config %{public}s flagIndex %{public}u", name, flagIndex);
    SandboxFlagsNode *node = (SandboxFlagsNode *)GetSandboxSection(&sandbox->spawnFlagsQueue, name);
    if (node == NULL) {
        node = CreateSandboxFlagsNode(name);
    }
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return -1);
    node->flagIndex = flagIndex;

    int ret = ParseBaseConfig(sandbox, &node->section, flagsConfig);
    if (ret != 0) {
        DeleteSandboxSection((SandboxSection *)node);
        return ret;
    }
    // success, insert section
    AddSandboxSection(&node->section, &sandbox->spawnFlagsQueue);
    return 0;
}

static int ParsePermissionConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *permissionConfig)
{
    APPSPAWN_LOGV("Parse permission config %{public}s", name);
    SandboxPermissionNode *node = (SandboxPermissionNode *)GetSandboxSection(&sandbox->permissionQueue, name);
    if (node == NULL) {
        node = CreateSandboxPermissionNode(name);
    }
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return -1);

    int ret = ParseBaseConfig(sandbox, &node->section, permissionConfig);
    if (ret != 0) {
        DeleteSandboxSection((SandboxSection *)node);
        return ret;
    }
    // success, insert section
    AddSandboxSection(&node->section, &sandbox->permissionQueue);
    return 0;
}

static SandboxNameGroupNode *ParseNameGroup(AppSpawnSandboxCfg *sandbox, const cJSON *groupConfig)
{
    char *name = GetStringFromJsonObj(groupConfig, "name");
    APPSPAWN_CHECK(name != NULL, return NULL, "No name in name group config");
    APPSPAWN_LOGV("Parse name-group config %{public}s", name);
    SandboxNameGroupNode *node = (SandboxNameGroupNode *)GetSandboxSection(&sandbox->nameGroupsQueue, name);
    if (node == NULL) {
        node = CreateSandboxNameGroupNode(name);
    }
    APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return NULL);

    cJSON *obj = cJSON_GetObjectItemCaseSensitive(groupConfig, "mount-paths-deps");
    if (obj) {
        if (node->depNode) { // free repeat
            DeleteSandboxMountNode((SandboxMountNode *)node->depNode);
        }
        node->depNode = DecodeMountPathConfig(NULL, obj, SANDBOX_TAG_MOUNT_PATH);
        if (node->depNode == NULL) {
            DeleteSandboxSection((SandboxSection *)node);
            return NULL;
        }
        // "deps-mode": "not-exists"
        node->depMode = GetMountModeFromConfig(groupConfig, "deps-mode", MOUNT_MODE_ALWAYS);
    }

    int ret = ParseBaseConfig(sandbox, &node->section, groupConfig);
    if (ret != 0) {
        DeleteSandboxSection((SandboxSection *)node);
        return NULL;
    }
    // "type": "system-const",
    // "caps": ["shared"],
    node->destType = GetNameGroupTypeFromConfig(groupConfig, "type", SANDBOX_TAG_INVALID);
    // success, insert section
    AddSandboxSection(&node->section, &sandbox->nameGroupsQueue);
    return node;
}

static int ParseNameGroupsConfig(AppSpawnSandboxCfg *sandbox, const cJSON *root)
{
    APPSPAWN_CHECK(root != NULL, return APPSPAWN_SANDBOX_INVALID, "Invalid config ");
    cJSON *configs = cJSON_GetObjectItemCaseSensitive(root, "name-groups");
    APPSPAWN_CHECK_ONLY_EXPER(configs != NULL, return 0);
    APPSPAWN_CHECK(cJSON_IsArray(configs), return APPSPAWN_SANDBOX_INVALID, "Invalid config ");
    int count = cJSON_GetArraySize(configs);
    APPSPAWN_CHECK_ONLY_EXPER(count > 0, return 0);

    sandbox->depNodeCount = 0;
    for (int i = 0; i < count; i++) {
        cJSON *json = cJSON_GetArrayItem(configs, i);
        if (json == NULL) {
            continue;
        }
        SandboxNameGroupNode *node = ParseNameGroup(sandbox, json);
        APPSPAWN_CHECK_ONLY_EXPER(node != NULL, return APPSPAWN_SANDBOX_INVALID);
        if (node->depNode) {
            sandbox->depNodeCount++;
        }
    }
    APPSPAWN_LOGV("ParseNameGroupsConfig depNodeCount %{public}d", sandbox->depNodeCount);
    return 0;
}

static int ParseConditionalConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, const char *configName,
    int (*parseConfig)(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *configs))
{
    APPSPAWN_CHECK_ONLY_EXPER(configs != NULL, return 0);
    APPSPAWN_CHECK(cJSON_IsArray(configs),
        return APPSPAWN_SANDBOX_INVALID, "Invalid config %{public}s", configName);
    int ret = 0;
    int count = cJSON_GetArraySize(configs);
    for (int i = 0; i < count; i++) {
        cJSON *json = cJSON_GetArrayItem(configs, i);
        if (json == NULL) {
            continue;
        }
        char *name = GetStringFromJsonObj(json, "name");
        if (name == NULL) {
            APPSPAWN_LOGE("No name in %{public}s configs", configName);
            continue;
        }
        ret = parseConfig(sandbox, name, json);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }
    return 0;
}

static int ParseGlobalSandboxConfig(AppSpawnSandboxCfg *sandbox, const cJSON *root)
{
    cJSON *json = cJSON_GetObjectItemCaseSensitive(root, "global");
    if (json) {
        sandbox->sandboxNsFlags = GetSandboxNsFlags(json);
        char *rootPath = GetStringFromJsonObj(json, "sandbox-root");
        APPSPAWN_CHECK(rootPath != NULL, return APPSPAWN_SYSTEM_ERROR, "No root path in config");
        if (sandbox->rootPath) {
            free(sandbox->rootPath);
        }
        sandbox->rootPath = strdup(rootPath);
        APPSPAWN_CHECK(sandbox->rootPath != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed to copy root path");
        sandbox->topSandboxSwitch = GetBoolValueFromJsonObj(json, "top-sandbox-switch", true);
    }
    return 0;
}

typedef struct TagParseJsonContext {
    AppSpawnSandboxCfg *sandboxCfg;
}ParseJsonContext;

APPSPAWN_STATIC int ParseAppSandboxConfig(const cJSON *root, ParseJsonContext *context)
{
    APPSPAWN_CHECK(root != NULL && context != NULL && context->sandboxCfg != NULL,
        return APPSPAWN_SYSTEM_ERROR, "Invalid json");
    AppSpawnSandboxCfg *sandbox = context->sandboxCfg;
    int ret = ParseGlobalSandboxConfig(sandbox, root);  // "global":
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return APPSPAWN_SANDBOX_INVALID);
    ret = ParseNameGroupsConfig(sandbox, root);  // name-groups
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return APPSPAWN_SANDBOX_INVALID);

    // "required"
    cJSON *required = cJSON_GetObjectItemCaseSensitive(root, "required");
    if (required) {
        cJSON *config = NULL;
        cJSON_ArrayForEach(config, required)
        {
            APPSPAWN_LOGI("Sandbox required config: %{public}s", config->string);
            SandboxSection *section = GetSandboxSection(&sandbox->requiredQueue, config->string);
            if (section == NULL) {
                section = CreateSandboxSection(config->string, sizeof(SandboxSection), SANDBOX_TAG_REQUIRED);
            }
            APPSPAWN_CHECK_ONLY_EXPER(section != NULL, return -1);

            ret = ParseBaseConfig(sandbox, section, config);
            if (ret != 0) {
                DeleteSandboxSection(section);
                return ret;
            }
            // success, insert section
            AddSandboxSection(section, &sandbox->requiredQueue);
        }
    }

    // conditional
    cJSON *json = cJSON_GetObjectItemCaseSensitive(root, "conditional");
    if (json != NULL) {
        // permission
        cJSON *config = cJSON_GetObjectItemCaseSensitive(json, "permission");
        ret = ParseConditionalConfig(sandbox, config, "permission", ParsePermissionConfig);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
        // spawn-flag
        config = cJSON_GetObjectItemCaseSensitive(json, "spawn-flag");
        ret = ParseConditionalConfig(sandbox, config, "spawn-flag", ParseSpawnFlagsConfig);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
        // package-name
        config = cJSON_GetObjectItemCaseSensitive(json, "package-name");
        ret = ParseConditionalConfig(sandbox, config, "package-name", ParsePackageNameConfig);
        APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    }
    return ret;
}

int LoadAppSandboxConfig(AppSpawnSandboxCfg *sandbox, int nwebSpawn)
{
    APPSPAWN_CHECK_ONLY_EXPER(sandbox != NULL, return APPSPAWN_ARG_INVALID);
    const char *sandboxName = nwebSpawn ? WEB_SANDBOX_FILE_NAME : APP_SANDBOX_FILE_NAME;
    if (sandbox->depGroupNodes != NULL) {
        APPSPAWN_LOGW("Sandbox has been load");
        return 0;
    }
    ParseJsonContext context = {};
    context.sandboxCfg = sandbox;
    int ret = ParseJsonConfig("etc/sandbox", sandboxName, ParseAppSandboxConfig, &context);
    if (ret == APPSPAWN_SANDBOX_NONE) {
        APPSPAWN_LOGW("No sandbox config");
        ret = 0;
    }
    APPSPAWN_CHECK_ONLY_EXPER(ret == 0, return ret);
    sandbox->pidNamespaceSupport = AppSandboxPidNsIsSupport();
    sandbox->appFullMountEnable = CheckAppFullMountEnable();
    APPSPAWN_LOGI("Sandbox pidNamespaceSupport: %{public}d appFullMountEnable: %{public}d",
        sandbox->pidNamespaceSupport, sandbox->appFullMountEnable);

    uint32_t depNodeCount = sandbox->depNodeCount;
    APPSPAWN_CHECK_ONLY_EXPER(depNodeCount > 0, return ret);

    sandbox->depGroupNodes = (SandboxNameGroupNode **)calloc(1, sizeof(SandboxNameGroupNode *) * depNodeCount);
    APPSPAWN_CHECK(sandbox->depGroupNodes != NULL, return APPSPAWN_SYSTEM_ERROR, "Failed alloc memory ");
    sandbox->depNodeCount = 0;
    ListNode *node = sandbox->nameGroupsQueue.front.next;
    while (node != &sandbox->nameGroupsQueue.front) {
        SandboxNameGroupNode *groupNode = (SandboxNameGroupNode *)ListEntry(node, SandboxMountNode, node);
        if (groupNode->depNode) {
            sandbox->depGroupNodes[sandbox->depNodeCount++] = groupNode;
        }
        node = node->next;
    }
    APPSPAWN_LOGI("LoadAppSandboxConfig depNodeCount %{public}d", sandbox->depNodeCount);

    return 0;
}
