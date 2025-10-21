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

#ifndef APPSPAWN_TEST_STUB_H
#define APPSPAWN_TEST_STUB_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "cJSON.h"
#include "appspawn_client.h"
#include "appspawn_hook.h"
#include "appspawn_encaps.h"

#ifdef __cplusplus
extern "C" {
#endif

PathMountNode *CreatePathMountNode(uint32_t type, uint32_t hasDemandInfo);
SymbolLinkNode *CreateSymbolLinkNode(void);
SandboxPackageNameNode *CreateSandboxPackageNameNode(const char *name);
SandboxFlagsNode *CreateSandboxFlagsNode(const char *name);
SandboxNameGroupNode *CreateSandboxNameGroupNode(const char *name);
SandboxPermissionNode *CreateSandboxPermissionNode(const char *name);
bool GetBoolParameter(const char *param, bool value);
bool AppSandboxPidNsIsSupport(void);
bool CheckAppFullMountEnable(void);
long GetMountModeFromConfig(const cJSON *config, const char *key, unsigned long def);
uint32_t GetNameGroupTypeFromConfig(const cJSON *config, const char *key, unsigned long def);
uint32_t GetSandboxNsFlags(const cJSON *appConfig);
int SetMode(const char *str, void *context);
mode_t GetChmodFromJson(const cJSON *config);
uint32_t GetFlagIndexFromJson(const cJSON *config);
int32_t DecodeDecPolicyPaths(const cJSON *config, PathMountNode *sandboxNode);
PathMountNode *DecodeMountPathConfig(const SandboxSection *section, const cJSON *config, uint32_t type);
int ParseMountPathsConfig(AppSpawnSandboxCfg *sandbox,
    const cJSON *mountConfigs, SandboxSection *section, uint32_t type);
SymbolLinkNode *DecodeSymbolLinksConfig(const SandboxSection *section, const cJSON *config);
int ParseSymbolLinksConfig(AppSpawnSandboxCfg *sandbox, const cJSON *symbolLinkConfigs,
    SandboxSection *section);
int ParseGidTableConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, SandboxSection *section);
int ParseMountGroupsConfig(AppSpawnSandboxCfg *sandbox, const cJSON *groupConfig, SandboxSection *section);
int ParseBaseConfig(AppSpawnSandboxCfg *sandbox, SandboxSection *section, const cJSON *configs);
int ParsePackageNameConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *packageNameConfigs);
int ParseSpawnFlagsConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *flagsConfig);
int ParsePermissionConfig(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *permissionConfig);
int AddSpawnerPermissionNode(AppSpawnSandboxCfg *sandbox);
SandboxNameGroupNode *ParseNameGroup(AppSpawnSandboxCfg *sandbox, const cJSON *groupConfig);
int ParseNameGroupsConfig(AppSpawnSandboxCfg *sandbox, const cJSON *root);
int ParseConditionalConfig(AppSpawnSandboxCfg *sandbox, const cJSON *configs, const char *configName,
    int (*parseConfig)(AppSpawnSandboxCfg *sandbox, const char *name, const cJSON *configs));
int ParseGlobalSandboxConfig(AppSpawnSandboxCfg *sandbox, const cJSON *root);
int ParseAppSandboxConfig(const cJSON *root, ParseJsonContext *context);
char *GetSandboxNameByType(ExtDataType type);
int LoadAppSandboxConfig(AppSpawnSandboxCfg *sandbox, ExtDataType type);
#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
