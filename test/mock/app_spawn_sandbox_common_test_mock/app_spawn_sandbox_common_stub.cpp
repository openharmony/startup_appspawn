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
    if (isNweb == true) {
        printf("GetSandboxNsFlags isNweb is true");
    }

    return 0;
}

bool AppSandboxPidNsIsSupport(void)
{
    return true;
}

int32_t HandleArrayForeach(cJSON *arrayJson, ArrayItemProcessor processor)
{
    if (arrayJson == nullptr) {
        printf("HandleArrayForeach arrayJson is null");
    }

    if (processor == nullptr) {
        printf("HandleArrayForeach processor is null");
    }

    return 0;
}

int LoadAppSandboxConfigCJson(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("LoadAppSandboxConfigCJson content is null");
    }

    return 0;
}

int FreeAppSandboxConfigCJson(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("FreeAppSandboxConfigCJson content is null");
    }

    return 0;
}

std::vector<cJSON *> &GetCJsonConfig(SandboxCommonDef::SandboxConfigType type)
{
    if (type == nullptr) {
        printf("GetCJsonConfig type is null");
    }

    return nullptr;
}

std::string GetExtraInfoByType(const AppSpawningCtx *appProperty, const std::string &type)
{
    if (appProperty == nullptr) {
        printf("GetExtraInfoByType appProperty is null");
    }

    if (type == nullptr) {
        printf("GetExtraInfoByType type is null");
    }

    return "";
}

std::string GetSandboxRootPath(const AppSpawningCtx *appProperty, cJSON *config)
{
    if (appProperty == nullptr) {
        printf("GetSandboxRootPath appProperty is null");
    }

    if (config == nullptr) {
        printf("GetSandboxRootPath config is null");
    }

    return "";
}

int CreateDirRecursive(const std::string &path, mode_t mode)
{
    if (path == "") {
        printf("CreateDirRecursive path is null");
    }

    if (mode == nullptr) {
        printf("CreateDirRecursive mode is null");
    }

    return 0;
}

void CreateDirRecursiveWithClock(const std::string &path, mode_t mode)
{
    if (path == "") {
        printf("CreateDirRecursiveWithClock path is null");
    }

    if (mode == nullptr) {
        printf("CreateDirRecursiveWithClock mode is nullptr");
    }
}

bool VerifyDirRecursive(const std::string &path)
{
    if (path == nullptr) {
        printf("VerifyDirRecursive path is null");
    }

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
    if (vec == nullptr) {
        printf("GetMountFlagsFromConfig vec is null");
    }

    return 0;
}

bool IsDacOverrideEnabled(cJSON *config)
{
    if (config == nullptr) {
        printf("IsDacOverrideEnabled config is null");
    }

    return true;
}

bool GetSwitchStatus(cJSON *config)
{
    if (config == nullptr) {
        printf("GetSwitchStatus config is null");
    }

    return true;
}

uint32_t ConvertFlagStr(const std::string &flagStr)
{
    if (flagStr == "") {
        printf("ConvertFlagStr flagStr is null");
    }

    return 0;
}

unsigned long GetMountFlags(cJSON *config)
{
    if (config == nullptr) {
        printf("GetMountFlags config is null");
    }

    return 0;
}

std::string GetFsType(cJSON *config)
{
    if (config == nullptr) {
        printf("GetFsType config is null");
    }

    return "";
}

std::string GetOptions(const AppSpawningCtx *appProperty, cJSON *config)
{
    if (appProperty == nullptr) {
        printf("GetOptions appProperty is null");
    }

    if (config == nullptr) {
        printf("GetOptions config is null");
    }

    return "";
}

std::vector<std::string> GetDecPath(const AppSpawningCtx *appProperty, cJSON *config)
{
    if (appProperty == nullptr) {
        printf("GetDecPath appProperty is null");
    }

    if (config == nullptr) {
        printf("GetDecPath config is null");
    }

    return nullptr;
}

bool IsCreateSandboxPathEnabled(cJSON *json, std::string srcPath)
{
    if (json == nullptr) {
        printf("IsCreateSandboxPathEnabled json is null");
    }

    if (srcPath == "") {
        printf("IsCreateSandboxPathEnabled srcPath is null");
    }

    return true;
}

bool IsTotalSandboxEnabled(const AppSpawningCtx *appProperty)
{
    if (appProperty == nullptr) {
        printf("IsTotalSandboxEnabled appProperty is null");
    }

    return true;
}

bool IsAppSandboxEnabled(const AppSpawningCtx *appProperty)
{
    if (appProperty == nullptr) {
        printf("IsAppSandboxEnabled appProperty is null");
    }

    return true;
}

void GetSandboxMountConfig(const AppSpawningCtx *appProperty, const std::string &section,
    cJSON *mntPoint, SandboxMountConfig &mountConfig)
{
    if (appProperty == nullptr) {
        printf("GetSandboxMountConfig appProperty is nullptr");
    }

    if (section == nullptr) {
        printf("GetSandboxMountConfig section is null");
    }

    if (mntPoint == nullptr) {
        printf("GetSandboxMountConfig mntPoint is null");
    }

    if (mountConfig == nullptr) {
        printf("GetSandboxMountConfig mountConfig is null");
    }
}

bool IsNeededCheckPathStatus(const AppSpawningCtx *appProperty, const char *path)
{
    if (appProperty == nullptr) {
        printf("IsNeededCheckPathStatus appProperty is null");
    }

    if (path == nullptr) {
        printf("IsNeededCheckPathStatus path is null");
    }

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
    if (bundleName == "") {
        printf("HasPrivateInBundleName bundleName is null");
    }

    return true;
}

bool IsMountSuccessful(cJSON *mntPoint)
{
    if (mntPoin == nullptr) {
        printf("IsMountSuccessful mntPoin is null");
    }

    return 0;
}

int CheckBundleName(const std::string &bundleName)
{
    if (bundleName == "") {
        printf("CheckBundleName bundleName is null");
    }

    return 0;
}

int32_t CheckAppFullMountEnable()
{
    return 0;
}

bool IsPrivateSharedStatus(const std::string &bundleName, AppSpawningCtx *appProperty)
{
    if (bundleName == "") {
        printf("IsPrivateSharedStatus bundleName is null");
    }

    if (appProperty == nullptr) {
        printf("IsPrivateSharedStatus appProperty is null");
    }

    return true;
}

bool IsValidMountConfig(cJSON *mntPoint, const AppSpawningCtx *appProperty, bool checkFlag)
{
    if (mntPoint == nullptr) {
        printf("IsValidMountConfig mntPoint is null");
    }

    if (appProperty == nullptr) {
        printf("IsValidMountConfig appProperty is null");
    }

    if (checkFlag == true) {
        printf("IsValidMountConfig checkFlag is true");
    }

    return true;
}

std::string ReplaceAllVariables(std::string str, const std::string& from, const std::string& to)
{
    if (str == "") {
        printf("ReplaceAllVariables str is null");
    }

    if (from == nullptr) {
        printf("ReplaceAllVariables from is null");
    }

    if (to == nullptr) {
        printf("ReplaceAllVariables to is null");
    }

    return "";
}

std::vector<std::string> SplitString(std::string &str, const std::string &delimiter)
{
    if (str == "") {
        printf("SplitString str is null");
    }

    if (delimiter == "") {
        printf("SplitString delimiter is null");
    }

    return "";
}

void MakeAtomicServiceDir(const AppSpawningCtx *appProperty, std::string path,
    std::string variablePackageName)
{
    if (path == "") {
        printf("MakeAtomicServiceDir path is nullptr");
    }

    if (variablePackageName == "") {
        printf("MakeAtomicServiceDir variablePackageName is null");
    }

    if (appProperty == nullptr) {
        printf("MakeAtomicServiceDir appProperty is null");
    }
}

std::string ReplaceVariablePackageName(const AppSpawningCtx *appProperty, const std::string &path)
{
    if (appProperty == nullptr) {
        printf("ReplaceVariablePackageName appProperty is null");
    }

    if (path == nullptr) {
        printf("ReplaceVariablePackageName path is null");
    }

    return "";
}

std::string ReplaceHostUserId(const AppSpawningCtx *appProperty, const std::string &path)
{
    if (appProperty == nullptr) {
        printf("ReplaceHostUserId appProperty is null");
    }

    if (path == nullptr) {
        printf("ReplaceHostUserId path is null");
    }

    return "";
}

std::string ReplaceClonePackageName(const AppSpawningCtx *appProperty, const std::string &path)
{
    if (appProperty == nullptr) {
        printf("ReplaceClonePackageName appProperty is null");
    }

    if (path == nullptr) {
        printf("ReplaceClonePackageName path is null");
    }

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
    if (appProperty == nullptr) {
        printf("ConvertToRealPathWithPermission appProperty is null");
    }

    if (path == "") {
        printf("ConvertToRealPathWithPermission path is null");
    }

    return "";
}

std::string ConvertToRealPath(const AppSpawningCtx *appProperty, std::string path)
{
    if (appProperty == nullptr) {
        printf("ConvertToRealPath appProperty is null");
    }

    if (path == "") {
        printf("ConvertToRealPath path is null");
    }

    return "";
}

void WriteMountInfo()
{
    printf("WriteMountInfo");
}

int32_t DoAppSandboxMountOnce(const AppSpawningCtx *appProperty, const SharedMountArgs *arg)
{
    if (appProperty == nullptr) {
        printf("DoAppSandboxMountOnce appProperty is null");
    }

    if (arg == nullptr) {
        printf("DoAppSandboxMountOnce arg is null");
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
