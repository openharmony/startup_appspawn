/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "app_spawn_stub.h"

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstdbool>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <grp.h>

#include <linux/capability.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "access_token.h"
#include "hilog/log.h"
#include "securec.h"
#include "token_setproc.h"
#include "tokenid_kit.h"

#ifdef __cplusplus
extern "C" {
#endif

PathMountNode *CreatePathMountNode(uint32_t type, uint32_t hasDemandInfo)
{
    if (type == 0) {
        printf("CreatePathMountNode type is null");
    }

    if (hasDemandInfo == 0) {
        printf("CreatePathMountNode hasDemandInfo is null");
    }

    return nullptr;
}

SymbolLinkNode *CreateSymbolLinkNode(void)
{
    return nullptr;
}

SandboxPackageNameNode *CreateSandboxPackageNameNode(const char *name)
{
    if (name == nullptr) {
        printf("CreateSandboxPackageNameNode name is null");
    }

    return nullptr;
}

SandboxFlagsNode *CreateSandboxFlagsNode(const char *name)
{
    if (name == nullptr) {
        printf("CreateSandboxFlagsNode name is null");
    }

    return nullptr;
}

SandboxNameGroupNode *CreateSandboxNameGroupNode(const char *name)
{
    if (name == nullptr) {
        printf("CreateSandboxNameGroupNode name is null");
    }

    return nullptr;
}

SandboxPermissionNode *CreateSandboxPermissionNode(const char *name)
{
    if (name == nullptr) {
        printf("CreateSandboxPermissionNode name is null");
    }

    return nullptr;
}

bool GetBoolParameter(const char *param, bool value)
{
    if (param == nullptr) {
        printf("GetBoolParameter param is null");
    }

    if (value == nullptr) {
        printf("GetBoolParameter value is null");
    }

    return true;
}

bool AppSandboxPidNsIsSupport(void)
{
    return true;
}

bool CheckAppFullMountEnable(void)
{
    return true;
}

long GetMountModeFromConfig(const cJSON *config, const char *key, unsigned long def)
{
    if (config == nullptr) {
        printf("GetMountModeFromConfig config is null");
    }

    if (key == nullptr) {
        printf("GetMountModeFromConfig key is null");
    }

    if (def == 0) {
        printf("GetMountModeFromConfig def is 0");
    }

    return 0;
}

uint32_t GetNameGroupTypeFromConfig(const cJSON *config, const char *key, unsigned long def)
{
    if (config) {
        printf("GetNameGroupTypeFromConfig config is null");
    }

    if (key) {
        printf("GetNameGroupTypeFromConfig key is null");
    }

    if (def == 0) {
        printf("GetNameGroupTypeFromConfig def is 0");
    }

    return 0;
}

uint32_t GetSandboxNsFlags(const cJSON *appConfig)
{
    if (appConfig == nullptr) {
        printf("GetSandboxNsFlags appConfig is null");
    }

    return 0;
}

int SetMode(const char *str, void *context)
{
    if (str == nullptr) {
        printf("SetMode str is null");
    }

    if (context == nullptr) {
        printf("SetMode context is null");
    }

    return 0;
}

mode_t GetChmodFromJson(const cJSON *config)
{
    if (config == nullptr) {
        printf("GetChmodFromJson config is null");
    }

    return nullptr;
}

uint32_t GetFlagIndexFromJson(const cJSON *config)
{
    if (config == nullptr) {
        printf("GetFlagIndexFromJson config is null");
    }

    return 0;
}

void FillPathDemandInfo(const cJSON *config, PathMountNode *sandboxNode)
{
    if (config == nullptr) {
        printf("FillPathDemandInfo config is nullptr");
    }

    if (sandboxNode == nullptr) {
        printf("FillPathDemandInfo sandboxNode is null");
    }
}

int32_t DecodeDecPolicyPaths(const cJSON *config, PathMountNode *sandboxNode)
{
    if (config == nullptr) {
        printf("DecodeDecPolicyPaths config is null");
    }

    if (sandboxNode == nullptr) {
        printf("DecodeDecPolicyPaths sandboxNode is null");
    }

    return 0;
}

PathMountNode *DecodeMountPathConfig(const SandboxSection *section, const cJSON *config, uint32_t type)
{
    if (section == nullptr) {
        printf("DecodeMountPathConfig section is null");
    }

    if (config == nullptr) {
        printf("DecodeMountPathConfig config is null");
    }

    if (type == 0) {
        printf("DecodeMountPathConfig type is 0");
    }

    return nullptr;
}

int ParseMountPathsConfig(AppSpawnSandboxCfg *sandbox,
    const cJSON *mountConfigs, SandboxSection *section, uint32_t type)
{
    if (sandbox == nullptr) {
        printf("ParseMountPathsConfig sandbox is null");
    }

    if (mountConfigs == nullptr) {
        printf("ParseMountPathsConfig mountConfigs is null");
    }

    if (section == nullptr) {
        printf("ParseMountPathsConfig section is null");
    }

    if (type == 0) {
        printf("ParseMountPathsConfig type is 0");
    }

    return 0;
}

SymbolLinkNode *DecodeSymbolLinksConfig(const SandboxSection *section, const cJSON *config)
{
    if (section == nullptr) {
        printf("DecodeSymbolLinksConfig section is null");
    }

    if (config == nullptr) {
        printf("DecodeSymbolLinksConfig config is null");
    }

    return nullptr;
}

int ParseSymbolLinksConfig(AppSpawnSandboxCfg *sandbox, const cJSON *symbolLinkConfigs,
    SandboxSection *section)
{
    if (sandbox == nullptr) {
        printf("ParseSymbolLinksConfig sandbox is null");
    }

    if (symbolLinkConfigs == nullptr) {
        printf("ParseSymbolLinksConfig symbolLinkConfigs is null");
    }

    if (section == nullptr) {
        printf("ParseSymbolLinksConfig section is null");
    }

    return 0;
}

int ParseGidTableConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, SandboxSection *section)
{
    if (sandbox == nullptr) {
        printf("ParseGidTableConfig sandbox is null");
    }

    if (configs == nullptr) {
        printf("ParseGidTableConfig configs is null");
    }

    if (section == nullptr) {
        printf("ParseGidTableConfig section is null");
    }

    return 0;
}

int ParseMountGroupsConfig(AppSpawnSandboxCfg *sandbox, const cJSON *groupConfig, SandboxSection *section)
{
    if (sandbox == nullptr) {
        printf("ParseMountGroupsConfig sandbox is null");
    }

    if (groupConfig == nullptr) {
        printf("ParseMountGroupsConfig groupConfig is null");
    }

    if (section == nullptr) {
        printf("ParseMountGroupsConfig section is null");
    }

    return 0;
}

int ParseBaseConfig(AppSpawnSandboxCfg *sandbox, SandboxSection *section, const cJSON *configs)
{
    if (sandbox == nullptr) {
        printf("ParseBaseConfig sandbox is null");
    }

    if (section == nullptr) {
        printf("ParseBaseConfig section is null");
    }

    if (configs == nullptr) {
        printf("ParseBaseConfig configs is null");
    }

    return 0;
}

int ParsePackageNameConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *packageNameConfigs)
{
    if (sandbox == nullptr) {
        printf("ParsePackageNameConfig sandbox is null");
    }

    if (name == nullptr) {
        printf("ParsePackageNameConfig name is null");
    }

    if (packageNameConfigs == nullptr) {
        printf("ParsePackageNameConfig packageNameConfigs is null");
    }

    return 0;
}

int ParseSpawnFlagsConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *flagsConfig)
{
    if (sandbox == nullptr) {
        printf("ParseSpawnFlagsConfig sandbox is null");
    }

    if (name == nullptr) {
        printf("ParseSpawnFlagsConfig name is null");
    }

    if (flagsConfig == nullptr) {
        printf("ParseSpawnFlagsConfig flagsConfig is null");
    }

    return 0;
}

int ParsePermissionConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *permissionConfig)
{
    if (sandbox == nullptr) {
        printf("ParsePermissionConfig sandbox is null");
    }

    if (name == nullptr) {
        printf("ParsePermissionConfig name is null");
    }

    if (permissionConfig == nullptr) {
        printf("ParsePermissionConfig permissionConfig is null");
    }

    return 0;
}

int AddSpawnerPermissionNode(AppSpawnSandboxCfg *sandbox)
{
    if (sandbox == nullptr) {
        printf("AddSpawnerPermissionNode sandbox is null");
    }

    return 0;
}

SandboxNameGroupNode *ParseNameGroup(AppSpawnSandboxCfg *sandbox, const cJSON *groupConfig)
{
    if (sandbox == nullptr) {
        printf("ParseNameGroup sandbox is null");
    }

    if (groupConfig == nullptr) {
        printf("ParseNameGroup groupConfig is null");
    }

    return nullptr;
}

int ParseNameGroupsConfig(AppSpawnSandboxCfg *sandbox, const cJSON *root)
{
    if (sandbox == nullptr) {
        printf("ParseNameGroupsConfig sandbox is null");
    }

    if (root == nullptr) {
        printf("ParseNameGroupsConfig root is null");
    }

    return 0;
}

int ParseConditionalConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, const char *configName,
    int (*parseConfig)(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *configs))
{
    if (sandbox == nullptr) {
        printf("ParseConditionalConfig sandbox is null");
    }

    if (configs == nullptr) {
        printf("ParseConditionalConfig configs is null");
    }

    if (configName == nullptr) {
        printf("ParseConditionalConfig configName is null");
    }

    if (parseConfig == nullptr) {
        printf("ParseConditionalConfig parseConfig is null");
    }

    return 0;
}

int ParseGlobalSandboxConfig(AppSpawnSandboxCfg *sandbox, const cJSON *root)
{
    if (sandbox == nullptr) {
        printf("ParseGlobalSandboxConfig sandbox is null");
    }

    if (root == nullptr) {
        printf("ParseGlobalSandboxConfig root is null");
    }

    return 0;
}

int ParseAppSandboxConfig(const cJSON *root, ParseJsonContext *context)
{
    if (root == nullptr) {
        printf("ParseAppSandboxConfig root is null");
    }

    if (context == nullptr) {
        printf("ParseAppSandboxConfig context is null");
    }

    return 0;
}

char *GetSandboxNameByType(ExtDataType type)
{
    if (type == nullptr) {
        printf("GetSandboxNameByType type is null");
    }

    return nullptr;
}

int LoadAppSandboxConfig(AppSpawnSandboxCfg *sandbox, ExtDataType type)
{
    if (sandbox == nullptr) {
        printf("LoadAppSandboxConfig sandbox is null");
    }

    if (type == nullptr) {
        printf("LoadAppSandboxConfig type is null");
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
