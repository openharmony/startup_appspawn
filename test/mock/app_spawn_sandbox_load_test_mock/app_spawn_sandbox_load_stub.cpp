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
    return nullptr;
}

SymbolLinkNode *CreateSymbolLinkNode(void)
{
    return nullptr;
}

SandboxPackageNameNode *CreateSandboxPackageNameNode(const char *name)
{
    return nullptr;
}

SandboxFlagsNode *CreateSandboxFlagsNode(const char *name)
{
    return nullptr;
}

SandboxNameGroupNode *CreateSandboxNameGroupNode(const char *name)
{
    return nullptr;
}

SandboxPermissionNode *CreateSandboxPermissionNode(const char *name)
{
    return nullptr;
}

bool GetBoolParameter(const char *param, bool value)
{
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
    return 0;
}

uint32_t GetNameGroupTypeFromConfig(const cJSON *config, const char *key, unsigned long def)
{
    return 0;
}

uint32_t GetSandboxNsFlags(const cJSON *appConfig)
{
    return 0;
}

int SetMode(const char *str, void *context)
{
    return 0;
}

mode_t GetChmodFromJson(const cJSON *config)
{
    return nullptr;
}

uint32_t GetFlagIndexFromJson(const cJSON *config)
{
    return 0;
}

void FillPathDemandInfo(const cJSON *config, PathMountNode *sandboxNode)
{
    if (config == nullptr) {
        printf("FillPathDemandInfo config is nullptr");
    }
}

int32_t DecodeDecPolicyPaths(const cJSON *config, PathMountNode *sandboxNode)
{
    return 0;
}

PathMountNode *DecodeMountPathConfig(const SandboxSection *section, const cJSON *config, uint32_t type)
{
    return nullptr;
}

int ParseMountPathsConfig(AppSpawnSandboxCfg *sandbox,
    const cJSON *mountConfigs, SandboxSection *section, uint32_t type)
{
    return 0;
}

SymbolLinkNode *DecodeSymbolLinksConfig(const SandboxSection *section, const cJSON *config)
{
    return nullptr;
}

int ParseSymbolLinksConfig(AppSpawnSandboxCfg *sandbox, const cJSON *symbolLinkConfigs,
    SandboxSection *section)
{
    return 0;
}

int ParseGidTableConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, SandboxSection *section)
{
    return 0;
}

int ParseMountGroupsConfig(AppSpawnSandboxCfg *sandbox, const cJSON *groupConfig, SandboxSection *section)
{
    return 0;
}

int ParseBaseConfig(AppSpawnSandboxCfg *sandbox, SandboxSection *section, const cJSON *configs)
{
    return 0;
}

int ParsePackageNameConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *packageNameConfigs)
{
    return 0;
}

int ParseSpawnFlagsConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *flagsConfig)
{
    return 0;
}

int ParsePermissionConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *permissionConfig)
{
    return 0;
}

int AddSpawnerPermissionNode(AppSpawnSandboxCfg *sandbox)
{
    return 0;
}

SandboxNameGroupNode *ParseNameGroup(AppSpawnSandboxCfg *sandbox, const cJSON *groupConfig)
{
    return nullptr;
}

int ParseNameGroupsConfig(AppSpawnSandboxCfg *sandbox, const cJSON *root)
{
    return 0;
}

int ParseConditionalConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, const char *configName,
    int (*parseConfig)(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *configs))
{
    return 0;
}

int ParseGlobalSandboxConfig(AppSpawnSandboxCfg *sandbox, const cJSON *root)
{
    return 0;
}

int ParseAppSandboxConfig(const cJSON *root, ParseJsonContext *context)
{
    return 0;
}

char *GetSandboxNameByType(ExtDataType type)
{
    return nullptr;
}

int LoadAppSandboxConfig(AppSpawnSandboxCfg *sandbox, ExtDataType type)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
