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

uint32_t GetSandboxNsFlags(bool isNweb)
{
    return 0;
}

bool AppSandboxPidNsIsSupport(void)
{
    return true;
}

int32_t HandleArrayForeach(cJSON *arrayJson, ArrayItemProcessor processor)
{
    return 0;
}

int LoadAppSandboxConfigCJson(AppSpawnMgr *content)
{
    return 0;
}

int FreeAppSandboxConfigCJson(AppSpawnMgr *content)
{
    return 0;
}

std::vector<cJSON *> &GetCJsonConfig(SandboxCommonDef::SandboxConfigType type)
{
    return nullptr;
}

std::string GetExtraInfoByType(const AppSpawningCtx *appProperty, const std::string &type)
{
    return "";
}

std::string GetSandboxRootPath(const AppSpawningCtx *appProperty, cJSON *config)
{
    return "";
}

int CreateDirRecursive(const std::string &path, mode_t mode)
{
    return 0;
}

void CreateDirRecursiveWithClock(const std::string &path, mode_t mode)
{
    if (mode == nullptr) {
        printf("CreateDirRecursiveWithClock mode is nullptr");
    }
}

bool VerifyDirRecursive(const std::string &path)
{
    return true;
}

void CreateFileIfNotExist(const char *file)
{
    if (file == nullptr) {
        printf("CreateFileIfNotExist file is nullptr");
    }
    return;
}

void SetSandboxPathChmod(cJSON *jsonConfig, std::string &sandboxRoot)
{
    if (jsonConfig == nullptr) {
        printf("SetSandboxPathChmod jsonConfig is nullptr");
    }
}

unsigned long GetMountFlagsFromConfig(const std::vector<std::string> &vec)
{
    return 0;
}

bool IsDacOverrideEnabled(cJSON *config)
{
    return true;
}

bool GetSwitchStatus(cJSON *config)
{
    return true;
}

uint32_t ConvertFlagStr(const std::string &flagStr)
{
    return 0;
}

unsigned long GetMountFlags(cJSON *config)
{
    return 0;
}

std::string GetFsType(cJSON *config)
{
    return "";
}

std::string GetOptions(const AppSpawningCtx *appProperty, cJSON *config)
{
    return "";
}

std::vector<std::string> GetDecPath(const AppSpawningCtx *appProperty, cJSON *config)
{
    return nullptr;
}

bool IsCreateSandboxPathEnabled(cJSON *json, std::string srcPath)
{
    return true;
}

bool IsTotalSandboxEnabled(const AppSpawningCtx *appProperty)
{
    return true;
}

bool IsAppSandboxEnabled(const AppSpawningCtx *appProperty)
{
    return true;
}

void GetSandboxMountConfig(const AppSpawningCtx *appProperty, const std::string &section,
    cJSON *mntPoint, SandboxMountConfig &mountConfig)
{
    if (appProperty == nullptr) {
        printf("GetSandboxMountConfig appProperty is nullptr");
    }
}

bool IsNeededCheckPathStatus(const AppSpawningCtx *appProperty, const char *path)
{
    return true;
}

void CheckMountStatus(const std::string &path)
{
    if (path == "") {
        printf("CheckMountStatus path is nullptr");
    }
}

bool HasPrivateInBundleName(const std::string &bundleName)
{
    return true;
}

bool IsMountSuccessful(cJSON *mntPoint)
{
    return 0;
}

int CheckBundleName(const std::string &bundleName)
{
    return 0;
}

int32_t CheckAppFullMountEnable()
{
    return 0;
}

bool IsPrivateSharedStatus(const std::string &bundleName, AppSpawningCtx *appProperty)
{
    return true;
}

bool IsValidMountConfig(cJSON *mntPoint, const AppSpawningCtx *appProperty, bool checkFlag)
{
    return true;
}

std::string ReplaceAllVariables(std::string str, const std::string& from, const std::string& to)
{
    return "";
}

std::vector<std::string> SplitString(std::string &str, const std::string &delimiter)
{
    return "";
}

void MakeAtomicServiceDir(const AppSpawningCtx *appProperty, std::string path,
    std::string variablePackageName)
{
    if (path == "") {
        printf("MakeAtomicServiceDir path is nullptr");
    }
}

std::string ReplaceVariablePackageName(const AppSpawningCtx *appProperty, const std::string &path)
{
    return "";
}

std::string ReplaceHostUserId(const AppSpawningCtx *appProperty, const std::string &path)
{
    return "";
}

std::string ReplaceClonePackageName(const AppSpawningCtx *appProperty, const std::string &path)
{
    return "";
}

std::string& GetArkWebPackageName(void)
{
    return "";
}

std::string &SandboxCommon::GetDevModel(void)
{
    return "";
}

std::string ConvertToRealPathWithPermission(const AppSpawningCtx *appProperty, std::string path)
{
    return "";
}

std::string ConvertToRealPath(const AppSpawningCtx *appProperty, std::string path)
{
    return "";
}

void WriteMountInfo()
{
    printf("WriteMountInfo");
}

int32_t DoAppSandboxMountOnce(const AppSpawningCtx *appProperty, const SharedMountArgs *arg)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
