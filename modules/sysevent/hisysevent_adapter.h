/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef APPSPAWN_HISYSEVENT_ADAPTER_H
#define APPSPAWN_HISYSEVENT_ADAPTER_H

#include "appspawn_utils.h"
#include "loop_event.h"

#ifdef __cplusplus
extern "C" {
#endif


// 错误码定义规范：子系统ID（28-21位）| 模块ID（20-16位）| 具体错误ID（15-0位）
#define APPSPAWN_HISYSEVENT_REPORT_TIME (24 * 60 * 60 * 1000)   // 24h
#define SUBSYS_STARTUP_ID   39
#define SUBSYSTEM_BIT_NUM   21
#define MODULE_BIT_NUM  16
#define ERR_MODULE_APPSPAWN 0x00
#define ERR_MODULE_INIT 0x10
#define ERR_APPSPAWN_BASE   ((SUBSYS_STARTUP_ID << SUBSYSTEM_BIT_NUM) | (ERR_MODULE_APPSPAWN << MODULE_BIT_NUM))
#define ERR_INIT_BASE   ((SUBSYS_STARTUP_ID << SUBSYSTEM_BIT_NUM) | (ERR_MODULE_INIT << MODULE_BIT_NUM))

typedef enum {
    // errcode for handle msg, reserved 128, 0x0001-0x0080
    ERR_APPSPAWN_MSG_TOO_LONG = ERR_APPSPAWN_BASE + 0x0001, // 81788929
    ERR_APPSPAWN_MSG_INVALID_HANDLE,
    ERR_APPSPAWN_MSG_CREATE_MSG_FAILED,
    ERR_APPSPAWN_COPY_MSG_FAILED,
    ERR_APPSPAWN_ALLOC_MEMORY_FAILED,
    ERR_APPSPAWN_MSG_DECODE_MSG_FAILED,

    // errcode for hnp, reserved 128, 0x0081 - 0x0100
    ERR_APPSPAWN_HNP = ERR_APPSPAWN_BASE + 0x0081,

    // errcode for device_dug, reserved 128, 0x0101 - 0x0180
    ERR_APPSPAWN_DEVICE_DUG = ERR_APPSPAWN_BASE + 0x0101,

    // errcode for process_exit, reserved 128, 0x0181 - 0x0200
    ERR_APPSPAWN_PROCESS_EXIT = ERR_APPSPAWN_BASE + 0x0181,

    // errcode for prefork, reserved 128, 0x0201 - 0x0280
    ERR_APPSPAWN_PREFORK = ERR_APPSPAWN_BASE + 0x0201,

    // errcode for sandbox, reserved 128, 0x0281 - 0x0300
    ERR_APPSPAWN_SANDBOX = ERR_APPSPAWN_BASE + 0x0281,

    // errcode for set permission, reserved 128, 0x0301 - 0x0380
    ERR_APPSPAWN_SET_PERMISSION = ERR_APPSPAWN_BASE + 0x0301,
    ERR_APPSPAWN_SET_INTE_PERMISSION_FAIL,
    ERR_APPSPAWN_SET_PROPERTIES_FAIL_SET_KEEP_CAP_FAIL,
    ERR_APPSPAWN_SET_PROPERTIES_FAIL_SET_XPM_FAIL,
    ERR_APPSPAWN_SET_PROPERTIES_FAIL_SET_PROCESSNAME_FAIL,
    ERR_APPSPAWN_SET_PROPERTIES_FAIL_SET_UIDGID_FAIL,
    ERR_APPSPAWN_SET_PROPERTIES_FAIL_SET_FD_FAIL,
    ERR_APPSPAWN_SET_PROPERTIES_FAIL_SET_CAP_FAIL,
    ERR_APPSPAWN_SET_PROPERTIES_FAIL_SET_SELINUXCON_FAIL,

    // errorcode for spawn child process fail, reserved 128, 0x0381 - 0x0400
    ERR_APPSPAWN_SPAWN_FAIL = ERR_APPSPAWN_BASE + 0x0381,
    ERR_APPSPAWN_SPAWN_TIMEOUT,
    ERR_APPSPAWN_CHILD_CRASH,
    ERR_APPSPAWN_MAX_FAILURES_EXCEEDED,
} AppSpawnHisysErrorCode;

typedef struct {
    uint32_t eventCount;
    uint32_t maxDuration;
    uint32_t minDuration;
    uint32_t totalDuration;
} AppSpawnHisysevent;

typedef struct {
    AppSpawnHisysevent bootEvent; // bootStage
    AppSpawnHisysevent manualEvent; // bootFinished
} AppSpawnHisyseventInfo;

AppSpawnHisyseventInfo *InitHisyseventTimer(void);
AppSpawnHisyseventInfo *GetAppSpawnHisyseventInfo(void);
void AddStatisticEventInfo(AppSpawnHisyseventInfo *hisyseventInfo, uint32_t duration, bool stage);
void DeleteHisyseventInfo(AppSpawnHisyseventInfo *hisyseventInfo);

void ReportSpawnChildProcessFail(const char* processName, int32_t errorCode);
void ReportSpawnStatisticDuration(const TimerHandle taskHandle, void* content);

#ifdef __cplusplus
}
#endif
#endif // APPSPAWN_HISYSEVENT_ADAPTER_H