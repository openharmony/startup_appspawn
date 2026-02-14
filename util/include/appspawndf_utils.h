/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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
#ifndef APPSPAWNDF_UTILS_H
#define APPSPAWNDF_UTILS_H

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include "appspawn_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*AppSpawnClientInitPtr)(const char *, AppSpawnClientHandle *);

/* 使能判断：接收注入的函数指针 */
bool AppSpawndfIsServiceEnabled(AppSpawnClientInitPtr initFunc);

/* 获取实例句柄 */
AppSpawnClientHandle AppSpawndfGetHandle(void);

/* 逻辑路由判断 */
bool AppSpawndfIsAppInWhiteList(const char *processName);
bool AppSpawndfIsAppEnableGWPASan(const char *name, AppSpawnMsgFlags* msgFlags);
bool AppSpawndfIsBroadcastMsg(uint32_t msgType);

/* 广播结果合并逻辑 */
void AppSpawndfMergeBroadcastResult(uint32_t msgType, AppSpawnResult *mainRes, AppSpawnResult *dfRes);

/* 设置消息标志位 */
int SetAppSpawnMsgFlags(AppSpawnMsgFlags *msgFlags, uint32_t index);

#ifdef __cplusplus
}
#endif

#endif // APPSPAWNDF_UTILS_H