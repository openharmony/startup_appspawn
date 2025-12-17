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

    if (root == nullptr) {
        printf("GetNames root is null");
    }

    if (key.empty()) {
        printf("GetNames key is null");
    }
}

int GetNameSet(const cJSON *root, ParseJsonContext *context)
{
    if (root == nullptr) {
        printf("root is null");
    }

    if (context == nullptr) {
        printf("context is null");
    }
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
    if (message == nullptr) {
        printf("BuildFdInfoMap message is null");
    }

    if (fdMap == nullptr) {
        printf("BuildFdInfoMap fdMap is null");
    }

    if (isColdRun == 0) {
        printf("BuildFdInfoMap isColdRun is 0");
    }

    return 0;
}

void ClearEnvAndReturnSuccess(AppSpawnContent *content, AppSpawnClient *client)
{
    if (content == nullptr) {
        printf("ClearEnvAndReturnSuccess content is null");
    }

    if (client == nullptr) {
        printf("ClearEnvAndReturnSuccess client is null");
    }
}

int RunChildThread(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("RunChildThread content is null");
    }

    if (property == nullptr) {
        printf("RunChildThread property is null");
    }

    return 0;
}

int RunChildByRenderCmd(const AppSpawnMgr *content, const AppSpawningCtx *property)
{
    if (content == nullptr) {
        printf("RunChildByRenderCmd content is null");
    }

    if (property == nullptr) {
        printf("RunChildByRenderCmd property is null");
    }
    return 0;
}

int RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    if (content == nullptr) {
        printf("RunChildProcessor content is null");
    }

    if (client == nullptr) {
        printf("RunChildProcessor client is null");
    }

    return 0;
}

int PreLoadAppSpawn(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("PreLoadAppSpawn content is null");
    }

    return 0;
}

int DoDlopenLibs(const cJSON *root, ParseJsonContext *context)
{
    if (root == nullptr) {
        printf("DoDlopenLibs root is null");
    }

    if (context == nullptr) {
        printf("ParseJsonContext context is null");
    }
    return 0;
}

int DlopenAppSpawn(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("DlopenAppSpawn content is null");
    }

    return 0;
}

int ProcessSpawnDlopenMsg(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("ProcessSpawnDlopenMsg content is null");
    }

    return 0;
}

int ProcessSpawnDlcloseMsg(AppSpawnMgr *content)
{
    if (content == nullptr) {
        printf("ProcessSpawnDlcloseMsg content is null");
    }

    return 0;
}

#ifdef __cplusplus
}
#endif
