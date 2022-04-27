/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "appspawn_adapter.h"

#include <dlfcn.h>
#include <string>

#ifdef NWEB_SPAWN
#define RENDER_PROCESS_MAX_NUM 16
#define RENDER_PROCESS_ARRAY_IDLE 0

typedef struct {
    int32_t pid;
    int exitStatus;
} RenderProcessNode;

static RenderProcessNode g_renderProcessArray[RENDER_PROCESS_MAX_NUM];
#endif

void *g_nwebHandle = nullptr;

void LoadExtendLib(AppSpawnContent *content)
{
    const std::string LOAD_LIB_DIR = "/data/app/el1/bundle/public/com.ohos.nweb/libs/arm";
#ifdef __MUSL__
    Dl_namespace dlns;
    dlns_init(&dlns, "nweb_ns");
    dlns_create(&dlns, LOAD_LIB_DIR.c_str());
    void *handle = dlopen_ns(&dlns, "libweb_engine.so", RTLD_NOW | RTLD_GLOBAL);
#else
    const std::string ENGINE_LIB_DIR = LOAD_LIB_DIR + "/libweb_engine.so";
    void *handle = dlopen(ENGINE_LIB_DIR.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (handle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libweb_engine.so, [%s]", dlerror());
    } else {
        APPSPAWN_LOGI("Success to dlopen libweb_engine.so");
    }
#ifdef __MUSL__
    g_nwebHandle = dlopen_ns(&dlns, "libnweb_render.so", RTLD_NOW | RTLD_GLOBAL);
#else
    const std::string RENDER_LIB_DIR = LOAD_LIB_DIR + "/libnweb_render.so";
    g_nwebHandle = dlopen(RENDER_LIB_DIR.c_str(), RTLD_NOW | RTLD_GLOBAL);
#endif
    if (g_nwebHandle == nullptr) {
        APPSPAWN_LOGE("Fail to dlopen libnweb_render.so, [%s]", dlerror());
    } else {
        APPSPAWN_LOGI("Success to dlopen libnweb_render.so");
    }
}

void RunChildProcessor(AppSpawnContent *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    using FuncType = void (*)(const char *cmd);
    FuncType funcNWebRenderMain = reinterpret_cast<FuncType>(dlsym(g_nwebHandle, "NWebRenderMain"));
    if (funcNWebRenderMain == nullptr) {
        APPSPAWN_LOGI("webviewspawn dlsym ERROR=%s", dlerror());
        return;
    }
    funcNWebRenderMain(appProperty->property.renderCmd);
}

static void DumpRenderProcessExitedArray()
{
    APPSPAWN_LOGI("dump render process exited array:");
    for (int i = 0; i < RENDER_PROCESS_MAX_NUM; i++) {
        APPSPAWN_LOGI("[pid, exitedStatus] = [%d, %d]",
            g_renderProcessArray[i].pid, g_renderProcessArray[i].exitStatus);
    }
}

void RecordRenderProcessExitedStatus(pid_t pid, int status)
{
    int i = 0;
    for (; i < RENDER_PROCESS_MAX_NUM; i++) {
        if (g_renderProcessArray[i].pid == RENDER_PROCESS_ARRAY_IDLE) {
            g_renderProcessArray[i].exitStatus = status;
            g_renderProcessArray[i].pid = pid;
            break;
        }
    }
    if (i == RENDER_PROCESS_MAX_NUM) {
        APPSPAWN_LOGE("no empty space in render process exited array");
        DumpRenderProcessExitedArray();
    }
}

int GetRenderProcessTerminationStatus(int32_t pid, int *status)
{
    if (status == nullptr) {
        return -1;
    }

    for (int i = 0; i < RENDER_PROCESS_MAX_NUM; i++) {
        if (g_renderProcessArray[i].pid == pid) {
            *status = g_renderProcessArray[i].exitStatus;
            g_renderProcessArray[i].pid = RENDER_PROCESS_ARRAY_IDLE;
            return 0;
        }
    }
    APPSPAWN_LOGE("not find pid[%d] in render process exited arrary", pid);
    DumpRenderProcessExitedArray();
    return -1;
}
