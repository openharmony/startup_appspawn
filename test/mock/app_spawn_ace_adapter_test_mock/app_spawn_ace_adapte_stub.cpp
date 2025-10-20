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


#ifdef __cplusplus
extern "C" {
#endif

void GetNames(const cJSON *root, std::set<std::string> &names, std::string key)
{
    if (names == nullptr) {
        printf("GetNames is null");
    }
}

int GetNameSet(const cJSON *root, ParseJsonContext *context)
{
    return 0;
}

void PreloadModule(bool isHybrid)
{
    printf("PreloadModule isHybrid %d", isHybrid);
}

void LoadExtendLib(bool isHybrid)
{
    printf("LoadExtendLib isHybrid %d", isHybrid);
}

void PreloadCJLibs(void)
{
    printf("PreloadCJLibs");
}

void LoadExtendCJLib(void)
{
    printf("LoadExtendCJLib");
}

int BuildFdInfoMap(const AppSpawnMsgNode *message, std::map<std::string, int> &fdMap, int isColdRun)
{
    return 0;
}

void ClearEnvAndReturnSuccess(AppSpawnContent *content, AppSpawnClient *client)
{
    if (content == nullptr) {
        printf("ClearEnvAndReturnSuccess is null");
    }
}

int RunChildThread(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    return 0;
}

int RunChildByRenderCmd(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    return 0;
}

int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    return 0;
}

int PreLoadAppSpawn(AppSpawnMgr *content)
{
    return 0;
}

int DoDlopenLibs(const cJSON *root, ParseJsonContext *context)
{
    return 0;
}

int DlopenAppSpawn(AppSpawnMgr *content)
{
    return 0;
}

int ProcessSpawnDlopenMsg(AppSpawnMgr *content)
{
    return 0;
}

int ProcessSpawnDlcloseMsg(AppSpawnMgr *content)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
