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
#include <chrono>

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

// cpu event
static constexpr char PERFORMANCE_DOMAIN[] = "PERFORMANCE";
constexpr const char* CPU_SCENE_ENTRY = "CPU_SCENE_ENTRY";
constexpr const char* PACKAGE_NAME = "PACKAGE_NAME";
constexpr const char* SCENE_ID = "SCENE_ID";
constexpr const char* HAPPEN_TIME = "HAPPEN_TIME";
constexpr const char* APPSPAWN = "APPSPAWN";

// for Mount Full
constexpr const char* SPAWN_MOUNT_FULL = "SPAWN_MOUNT_FULL";
constexpr const char* NAMESPACE_MOUNT_COUNT = "NAMESPACE_MOUNT_COUNT";
constexpr const char* DEVICE_MOUNT_COUNT = "DEVICE_MOUNT_COUNT";

// for Mount count
constexpr const char* SPAWN_MOUNT_COUNT = "SPAWN_MOUNT_COUNT";
constexpr const char* MAX_NAMESPACE_MOUNT_COUNT = "MAX_NAMESPACE_MOUNT_COUNT";
constexpr const char* MAX_DEVICE_MOUNT_COUNT = "MAX_DEVICE_MOUNT_COUNT";
constexpr const char* AVG_NAMESPACE_MOUNT_COUNT = "AVG_NAMESPACE_MOUNT_COUNT";
constexpr const char* AVG_DEVICE_MOUNT_COUNT = "AVG_DEVICE_MOUNT_COUNT";
}

void AppSpawnHiSysEventWrite()
{
    auto now = std::chrono::system_clock::now();
    int64_t timeStamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    int32_t sceneld = 0;
    int ret = HiSysEventWrite(PERFORMANCE_DOMAIN, CPU_SCENE_ENTRY,
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        PACKAGE_NAME, APPSPAWN,
        SCENE_ID, std::to_string(sceneld).c_str(),
        HAPPEN_TIME, timeStamp);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "HiSysEventWrite error,ret = %{public}d", ret);
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
    event->avgDeviceMountCount = 0;
    event->avgNsMountCount = 0;
    event->maxDeviceMountCount = 0;
    event->maxNsMountCount = 0;
}

static void InitStatisticEventInfo(AppSpawnHisyseventInfo *appSpawnHisysInfo)
{
    InitStatisticEvent(&appSpawnHisysInfo->bootEvent);
    InitStatisticEvent(&appSpawnHisysInfo->manualEvent);
}

static int StartTimer(LE_ProcessTimer processTimer, void *context, uint64_t timeout, uint64_t repeat)
{
    LoopHandle loop = LE_GetDefaultLoop();
    APPSPAWN_CHECK(loop != NULL, return -1, "getDeafult loop failed");

    TimerHandle timer = NULL;
    int ret = LE_CreateTimer(loop, &timer, processTimer, context);
    APPSPAWN_CHECK(ret == 0, return ret, "LE_CreateTimer failed %{public}d", ret);
    ret = LE_StartTimer(loop, timer, timeout, repeat);
    APPSPAWN_CHECK(ret == 0, LE_StopTimer(LE_GetDefaultLoop(), timer);
        return ret, "LE_StartTimer failed %{public}d", ret);
    return ret;
}

void ReportMountCount(AppSpawnHisyseventInfo *hisysEvent)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_MOUNT_COUNT,
        HiSysEvent::EventType::STATISTIC,
        MAX_NAMESPACE_MOUNT_COUNT, hisysEvent->manualEvent.maxNsMountCount,
        MAX_DEVICE_MOUNT_COUNT, hisysEvent->manualEvent.maxDeviceMountCount,
        AVG_NAMESPACE_MOUNT_COUNT, hisysEvent->manualEvent.avgNsMountCount,
        AVG_DEVICE_MOUNT_COUNT, hisysEvent->manualEvent.avgDeviceMountCount);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportMountCount error, ret: %{public}d", ret);
}

static void UpdateMountCountInfo(const TimerHandle taskHandle, void *content)
{
    APPSPAWN_LOGI("Start Update Mount Behavior info");
    AppSpawnHisyseventInfo *hisysEvent = static_cast<AppSpawnHisyseventInfo *>(content);
    APPSPAWN_CHECK(hisysEvent != nullptr, return, "invalid hisys info");
    // get mountinfo from proc
    // update mount info
    ReportMountCount(hisysEvent);
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

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportSpawnProcessDuration error, ret: %{public}d", ret);
}

void ReportSpawnStatisticDuration(const TimerHandle taskHandle, void *content)
{
    AppSpawnHisyseventInfo *hisyseventInfo = static_cast<AppSpawnHisyseventInfo *>(content);
    ReportSpawnProcessDuration(&hisyseventInfo->bootEvent, BOOTSTAGE);
    ReportSpawnProcessDuration(&hisyseventInfo->manualEvent, BOOTFINISHEDSTAGE);

    InitStatisticEventInfo(hisyseventInfo);
}

APPSPAWN_STATIC int CreateHisysTimerLoop(AppSpawnHisyseventInfo *hisyseventInfo)
{
    APPSPAWN_CHECK(hisyseventInfo != NULL, return -1, "invalid hisysevent info");
    int ret = StartTimer(ReportSpawnStatisticDuration, (void *)hisyseventInfo, DURATION_TIMER_TIMEOUT, INT64_MAX);
    APPSPAWN_CHECK(ret == 0, return -1, "fail to start statistic timer");
    ret = StartTimer(UpdateMountCountInfo, (void *)hisyseventInfo, MOUNT_TIMER_TIMEOUT, INT64_MAX);
    APPSPAWN_CHECK(ret == 0, return -1, "fail to start mount timer");
    return 0;
}

AppSpawnHisyseventInfo *GetAppSpawnHisyseventInfo(void)
{
    AppSpawnHisyseventInfo *hisyseventInfo =
        static_cast<AppSpawnHisyseventInfo *>(calloc(1, sizeof(AppSpawnHisyseventInfo)));
    APPSPAWN_CHECK(hisyseventInfo != NULL, return NULL, "fail to alloc memory for hisyseventInfo");
    InitStatisticEventInfo(hisyseventInfo);
    return hisyseventInfo;
}

void DeleteHisyseventInfo(AppSpawnHisyseventInfo *hisyseventInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(hisyseventInfo != NULL, return);
    free(hisyseventInfo);
    hisyseventInfo = nullptr;
}

AppSpawnHisyseventInfo *InitHisyseventTimer(void)
{
    AppSpawnHisyseventInfo *hisyseventInfo = GetAppSpawnHisyseventInfo();
    APPSPAWN_CHECK(hisyseventInfo != NULL, return NULL, "fail to init hisyseventInfo");
    int ret = CreateHisysTimerLoop(hisyseventInfo);
    if (ret != 0) {
        DeleteHisyseventInfo(hisyseventInfo);
        hisyseventInfo = nullptr;
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

void ReportMountFull(int32_t errCode, int32_t nsMountCount, int32_t deviceMountCount, int32_t spawnResult)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_MOUNT_FULL,
        HiSysEvent::EventType::FAULT,
        ERROR_CODE, errCode,
        NAMESPACE_MOUNT_COUNT, nsMountCount,
        DEVICE_MOUNT_COUNT, deviceMountCount,
        SPAWN_RESULT, spawnResult);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportMountFull error, ret: %{public}d", ret);
}

