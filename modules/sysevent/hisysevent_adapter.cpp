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
#include "hisysevent_adapter.h"
#include "securec.h"
#include "hisysevent.h"
#include "appspawn_utils.h"

using namespace OHOS::HiviewDFX;
namespace {
// event
constexpr const char* SPAWN_PROCESS_DURATION = "SPAWN_PROCESS_DURATION";
constexpr const char* SPAWN_CHILD_PROCESS_FAIL = "SPAWN_CHILD_PROCESS_FAIL";

// param
constexpr const char* PROCESS_NAME = "PROCESS_NAME";
constexpr const char* ERROR_CODE = "ERROR_CODE";
constexpr const char* MAXDURATION = "MAXDURATION";
constexpr const char* MINDURATION = "MINDURATION";
constexpr const char* TOTALDURATION = "TOTALDURATION";
constexpr const char* EVENTCOUNT = "EVENTCOUNT";
constexpr const char* STAGE = "STAGE";
constexpr const char* BOOTSTAGE = "BOOTSTAGE";
constexpr const char* BOOTFINISHEDSTAGE = "BOOTFINISHEDSTAGE";
}

static void AddStatisticEvent(AppSpawnHisysevent *event, uint32_t duration)
{
    event->eventCount++;
    event->totalDuration += duration;
    if (duration > event->maxDuration) {
        event->maxDuration = duration;
    }
    if (duration < event->minDuration) {
        event->minDuration = duration;
    }
    APPSPAWN_LOGV("event->maxDuration is: %{public}d, event->minDuration is: %{public}d",
        event->maxDuration, event->minDuration);
}

void AddStatisticEventInfo(AppSpawnHisyseventInfo *hisyseventInfo, uint32_t duration, bool stage)
{
    APPSPAWN_CHECK(hisyseventInfo != NULL, return, "fail to get AppSpawnHisyseventInfo");
    if (stage) {
        AddStatisticEvent(&hisyseventInfo->bootEvent, duration);
    } else {
        AddStatisticEvent(&hisyseventInfo->manualEvent, duration);
    }
}

static void InitStatisticEvent(AppSpawnHisysevent *event)
{
    event->eventCount = 0;
    event->maxDuration = 0;
    event->minDuration = UINT32_MAX;
    event->totalDuration = 0;
}

static void InitStatisticEventInfo(AppSpawnHisyseventInfo *appSpawnHisysInfo)
{
    InitStatisticEvent(&appSpawnHisysInfo->bootEvent);
    InitStatisticEvent(&appSpawnHisysInfo->manualEvent);
}

static int CreateHisysTimerLoop(AppSpawnHisyseventInfo *hisyseventInfo)
{
    LoopHandle loop = LE_GetDefaultLoop();
    TimerHandle timer = NULL;
    int ret = LE_CreateTimer(loop, &timer, ReportSpawnStatisticDuration, (void *)hisyseventInfo);
    APPSPAWN_CHECK(ret == 0, return -1, "fail to create HisysTimer, ret is: %{public}d", ret);
    // start a timer to report event every 24h
    ret = LE_StartTimer(loop, timer, APPSPAWN_HISYSEVENT_REPORT_TIME, INT64_MAX);
    APPSPAWN_CHECK(ret == 0, return -1, "fail to start HisysTimer, ret is: %{public}d", ret);
    return ret;
}

AppSpawnHisyseventInfo *GetAppSpawnHisyseventInfo()
{
    AppSpawnHisyseventInfo *hisyseventInfo =
        static_cast<AppSpawnHisyseventInfo *>(malloc(sizeof(AppSpawnHisyseventInfo)));
    APPSPAWN_CHECK(hisyseventInfo != NULL, return NULL, "fail to alloc memory for hisyseventInfo");
    int ret = memset_s(hisyseventInfo, sizeof(AppSpawnHisyseventInfo), 0, sizeof(AppSpawnHisyseventInfo));
    if (ret != 0) {
        free(hisyseventInfo);
        hisyseventInfo = NULL;
        APPSPAWN_LOGE("Failed to memset hisyseventInfo");
        return NULL;
    }
    InitStatisticEventInfo(hisyseventInfo);
    return hisyseventInfo;
}

void DeleteHisyseventInfo(AppSpawnHisyseventInfo *hisyseventInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(hisyseventInfo != NULL, return);
    free(hisyseventInfo);
    hisyseventInfo = NULL;
}

AppSpawnHisyseventInfo *InitHisyseventTimer()
{
    AppSpawnHisyseventInfo *hisyseventInfo = GetAppSpawnHisyseventInfo();
    APPSPAWN_CHECK(hisyseventInfo != NULL, return NULL, "fail to init hisyseventInfo");
    int ret = CreateHisysTimerLoop(hisyseventInfo);
    if (ret != 0) {
        DeleteHisyseventInfo(hisyseventInfo);
        hisyseventInfo = NULL;
        APPSPAWN_LOGE("fail to create hisys timer loop, ret: %{public}d", ret);
    }
    return hisyseventInfo;
}

void ReportSpawnChildProcessFail(const char* processName, int32_t errorCode)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_CHILD_PROCESS_FAIL,
        HiSysEvent::EventType::FAULT,
        PROCESS_NAME, processName,
        ERROR_CODE, errorCode);
    if (ret != 0) {
        APPSPAWN_LOGE("HiSysEventWrite error, ret: %{public}d", ret);
    }
}

void ReportSpawnProcessDuration(AppSpawnHisysevent *hisysevent, const char* stage)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_PROCESS_DURATION,
        HiSysEvent::EventType::STATISTIC,
        MAXDURATION, hisysevent->maxDuration,
        MINDURATION, hisysevent->minDuration,
        TOTALDURATION, hisysevent->totalDuration,
        EVENTCOUNT, hisysevent->eventCount,
        STAGE, stage);
    if (ret != 0) {
        APPSPAWN_LOGE("HiSysEventWrite error, ret: %{public}d", ret);
    }
}

void ReportSpawnStatisticDuration(const TimerHandle taskHandle, void *content)
{
    AppSpawnHisyseventInfo *hisyseventInfo = static_cast<AppSpawnHisyseventInfo *>(content);
    ReportSpawnProcessDuration(&hisyseventInfo->bootEvent, BOOTSTAGE);
    ReportSpawnProcessDuration(&hisyseventInfo->manualEvent, BOOTFINISHEDSTAGE);

    InitStatisticEventInfo(hisyseventInfo);
}