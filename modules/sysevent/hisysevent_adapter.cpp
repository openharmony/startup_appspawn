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
// fail event
constexpr const char* SPAWN_CHILD_PROCESS_FAIL = "SPAWN_CHILD_PROCESS_FAIL";

// behavior event
constexpr const char* SPAWN_KEY_EVENT = "SPAWN_KEY_EVENT";
constexpr const char* SPAWN_ABNORMAL_DURATION = "SPAWN_ABNORMAL_DURATION";

// statistic event
constexpr const char* SPAWN_PROCESS_DURATION = "SPAWN_PROCESS_DURATION";

// param
constexpr const char* PROCESS_NAME = "PROCESS_NAME";
constexpr const char* ERROR_CODE = "ERROR_CODE";
constexpr const char* SPAWN_RESULT = "SPAWN_RESULT";
constexpr const char* SRC_PATH = "SRC_PATH";
constexpr const char* TARGET_PATH = "TARGET_PATH";

constexpr const char* EVENT_NAME = "EVENT_NAME";

constexpr const char* SCENE_NAME = "SCENE_NAME";
constexpr const char* DURATION = "DURATION";

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

AppSpawnHisyseventInfo *GetAppSpawnHisyseventInfo(void)
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

AppSpawnHisyseventInfo *InitHisyseventTimer(void)
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

void ReportSpawnChildProcessFail(const char* processName, int32_t errorCode, int32_t spawnResult)
{
    if (spawnResult == APPSPAWN_SANDBOX_MOUNT_FAIL) {
        return;
    }
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_CHILD_PROCESS_FAIL,
        HiSysEvent::EventType::FAULT,
        PROCESS_NAME, processName,
        ERROR_CODE, errorCode,
        SPAWN_RESULT, spawnResult);
    if (ret != 0) {
        APPSPAWN_LOGE("ReportSpawnChildProcessFail error, ret: %{public}d", ret);
    }
}

void ReportMountFail(const char* bundleName, const char* srcPath, const char* targetPath,
    int32_t spawnResult)
{
    if (srcPath == nullptr || (strstr(srcPath, "data/app/el1/") == nullptr &&
        strstr(srcPath, "data/app/el2/") == nullptr)) {
        return;
    }
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_CHILD_PROCESS_FAIL,
        HiSysEvent::EventType::FAULT,
        PROCESS_NAME, bundleName,
        ERROR_CODE, ERR_APPSPAWN_CHILD_MOUNT_FAILED,
        SRC_PATH, srcPath,
        TARGET_PATH, targetPath,
        SPAWN_RESULT, spawnResult);
    if (ret != 0) {
        APPSPAWN_LOGE("ReportMountFail error, ret: %{public}d", ret);
    }
}

void ReportKeyEvent(const char *eventName)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_KEY_EVENT,
        HiSysEvent::EventType::BEHAVIOR,
        EVENT_NAME, eventName);
    if (ret != 0) {
        APPSPAWN_LOGE("ReportKeyEvent error, ret: %{public}d", ret);
    }
}

void ReportAbnormalDuration(const char* scene, uint64_t duration)
{
    APPSPAWN_LOGI("ReportAbnormalDuration %{public}d with %{public}s  %{public}" PRId64 " us",
        getpid(), scene, duration);
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_ABNORMAL_DURATION,
        HiSysEvent::EventType::BEHAVIOR,
        SCENE_NAME, scene,
        DURATION, duration);
    if (ret != 0) {
        APPSPAWN_LOGE("ReportAbnormalDuration error, ret: %{public}d", ret);
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
        APPSPAWN_LOGE("ReportSpawnProcessDuration error, ret: %{public}d", ret);
    }
}

void ReportSpawnStatisticDuration(const TimerHandle taskHandle, void *content)
{
    AppSpawnHisyseventInfo *hisyseventInfo = static_cast<AppSpawnHisyseventInfo *>(content);
    ReportSpawnProcessDuration(&hisyseventInfo->bootEvent, BOOTSTAGE);
    ReportSpawnProcessDuration(&hisyseventInfo->manualEvent, BOOTFINISHEDSTAGE);

    InitStatisticEventInfo(hisyseventInfo);
}