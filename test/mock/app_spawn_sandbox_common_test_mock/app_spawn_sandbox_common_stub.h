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

uint32_t GetSandboxNsFlags(bool isNweb);
bool AppSandboxPidNsIsSupport(void);
int32_t HandleArrayForeach(cJSON *arrayJson, ArrayItemProcessor processor);
int FreeAppSandboxConfigCJson(AppSpawnMgr *content);
int LoadAppSandboxConfigCJson(AppSpawnMgr *content);
std::vector<cJSON *> &GetCJsonConfig(SandboxCommonDef::SandboxConfigType type);
std::string GetExtraInfoByType(const AppSpawningCtx *appProperty, const std::string &type);
std::string GetSandboxRootPath(const AppSpawningCtx *appProperty, cJSON *config);
int CreateDirRecursive(const std::string &path, mode_t mode);
void CreateDirRecursiveWithClock(const std::string &path, mode_t mode);
bool VerifyDirRecursive(const std::string &path);
void CreateFileIfNotExist(const char *file);
void SetSandboxPathChmod(cJSON *jsonConfig, std::string &sandboxRoot);
unsigned long GetMountFlagsFromConfig(const std::vector<std::string> &vec);
bool IsDacOverrideEnabled(cJSON *config);
bool GetSwitchStatus(cJSON *config);
uint32_t ConvertFlagStr(const std::string &flagStr);
unsigned long GetMountFlags(cJSON *config);
std::string GetFsType(cJSON *config);
std::string GetOptions(const AppSpawningCtx *appProperty, cJSON *config);
std::vector<std::string> GetDecPath(const AppSpawningCtx *appProperty, cJSON *config);
bool IsCreateSandboxPathEnabled(cJSON *json, std::string srcPath);
bool IsTotalSandboxEnabled(const AppSpawningCtx *appProperty);
bool IsAppSandboxEnabled(const AppSpawningCtx *appProperty);
void GetSandboxMountConfig(const AppSpawningCtx *appProperty, const std::string &section,
    cJSON *mntPoint, SandboxMountConfig &mountConfig);
bool IsNeededCheckPathStatus(const AppSpawningCtx *appProperty, const char *path);
void CheckMountStatus(const std::string &path);
bool HasPrivateInBundleName(const std::string &bundleName);
bool IsMountSuccessful(cJSON *mntPoint);
int CheckBundleName(const std::string &bundleName);
int32_t CheckAppFullMountEnable();
bool IsPrivateSharedStatus(const std::string &bundleName, AppSpawningCtx *appProperty);
bool IsValidMountConfig(cJSON *mntPoint, const AppSpawningCtx *appProperty, bool checkFlag);
std::string ReplaceAllVariables(std::string str, const std::string& from, const std::string& to);
std::vector<std::string> SplitString(std::string &str, const std::string &delimiter);
void MakeAtomicServiceDir(const AppSpawningCtx *appProperty, std::string path,
    std::string variablePackageName);
std::string ReplaceVariablePackageName(const AppSpawningCtx *appProperty, const std::string &path);
std::string ReplaceHostUserId(const AppSpawningCtx *appProperty, const std::string &path);
std::string ReplaceClonePackageName(const AppSpawningCtx *appProperty, const std::string &path);
std::string &GetArkWebPackageName(void);
std::string &SandboxCommon::GetDevModel(void);
std::string ConvertToRealPathWithPermission(const AppSpawningCtx *appProperty, std::string path);
std::string ConvertToRealPath(const AppSpawningCtx *appProperty, std::string path);
void WriteMountInfo();
int32_t DoAppSandboxMountOnce(const AppSpawningCtx *appProperty, const SharedMountArgs *arg);
#ifdef __cplusplus
}
#endif

#endif // APPSPAWN_TEST_STUB_H
