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
#include <fstream>
#include <sstream>

#include "init_param.h"
#include "parameter.h"
#include "appspawn_manager.h"
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

// unlock mount param
constexpr const char* UID = "UID";
constexpr const char* APP_COUNT = "APP_COUNT";
constexpr const char* SUCCESS_COUNT = "SUCCESS_COUNT";
constexpr const char* FAIL_COUNT = "FAIL_COUNT";
constexpr const char* BUNDLE_NAME = "BUNDLE_NAME";
constexpr const char* UNLOCK_MOUNT_ALL_DONE = "UNLOCK_MOUNT_ALL_DONE";
constexpr const char* UNLOCK_MOUNT_APP_FAIL = "UNLOCK_MOUNT_APP_FAIL";

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

}

void AppSpawnHiSysEventWrite()
{
    auto now = std::chrono::system_clock::now();
    int64_t timeStamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    int ret = HiSysEventWrite(PERFORMANCE_DOMAIN, CPU_SCENE_ENTRY,
        OHOS::HiviewDFX::HiSysEvent::EventType::BEHAVIOR,
        PACKAGE_NAME, APPSPAWN,
        SCENE_ID, "0",
        HAPPEN_TIME, timeStamp);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "HiSysEventWrite error, ret: %{public}d", ret);
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
    APPSPAWN_CHECK(hisyseventInfo != nullptr, return, "Invalid hisysevent info");
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
    APPSPAWN_CHECK(hisyseventInfo != nullptr, return -1, "Invalid hisysevent info");
    LoopHandle loop = LE_GetDefaultLoop();
    APPSPAWN_CHECK(loop != nullptr, return -1, "LE_GetDefaultLoop failed");

    TimerHandle timer = nullptr;
    int ret = LE_CreateTimer(loop, &timer, ReportSpawnStatisticDuration, (void *)hisyseventInfo);
    APPSPAWN_CHECK(ret == 0, return ret, "Failed to create hisys timer %{public}d", ret);

    // start a timer to report event every 24h
    ret = LE_StartTimer(loop, timer, DURATION_TIMER_TIMEOUT, INT64_MAX);
    APPSPAWN_CHECK(ret == 0, LE_StopTimer(LE_GetDefaultLoop(), timer), "Failed to start hisys timer %{public}d", ret);
    return ret;
}

AppSpawnHisyseventInfo *GetAppSpawnHisyseventInfo(void)
{
    AppSpawnHisyseventInfo *hisyseventInfo =
        static_cast<AppSpawnHisyseventInfo *>(calloc(1, sizeof(AppSpawnHisyseventInfo)));
    APPSPAWN_CHECK(hisyseventInfo != nullptr, return nullptr, "Failed to calloc for hisyseventInfo %{public}d", errno);

    InitStatisticEventInfo(hisyseventInfo);
    return hisyseventInfo;
}

void DeleteHisyseventInfo(AppSpawnHisyseventInfo *hisyseventInfo)
{
    APPSPAWN_CHECK_ONLY_EXPER(hisyseventInfo != nullptr, return);
    free(hisyseventInfo);
    hisyseventInfo = nullptr;
}

AppSpawnHisyseventInfo *InitHisyseventTimer(void)
{
    AppSpawnHisyseventInfo *hisyseventInfo = GetAppSpawnHisyseventInfo();
    APPSPAWN_CHECK(hisyseventInfo != nullptr, return nullptr, "Failed to get hisysevent info");

    int ret = CreateHisysTimerLoop(hisyseventInfo);
    if (ret != 0) {
        DeleteHisyseventInfo(hisyseventInfo);
        hisyseventInfo = nullptr;
        APPSPAWN_LOGE("Failed to create hisys timer loop, ret: %{public}d", ret);
    }

    return hisyseventInfo;
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

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportSpawnChildProcessFail error, ret: %{public}d", ret);
}

void ReportMountFail(const char* bundleName, const char* srcPath, const char* targetPath, int32_t spawnResult)
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

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportMountFail error, ret: %{public}d", ret);
}

void ReportKeyEvent(const char *eventName)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_KEY_EVENT,
        HiSysEvent::EventType::BEHAVIOR,
        EVENT_NAME, eventName);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportKeyEvent error, ret: %{public}d", ret);
}

void ReportAbnormalDuration(const char* scene, uint64_t duration)
{
    APPSPAWN_LOGI("ReportAbnormalDuration %{public}d with %{public}s %{public}" PRId64 " us",
        getpid(), scene, duration);
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_ABNORMAL_DURATION,
        HiSysEvent::EventType::BEHAVIOR,
        SCENE_NAME, scene,
        DURATION, duration);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportAbnormalDuration error, ret: %{public}d", ret);
}

void ReportMountFull(int32_t errCode, int32_t nsMountCount, int32_t deviceMountCount, int32_t spawnResult)
{
    APPSPAWN_LOGI("ReportMountFull %{public}d %{public}d %{public}d %{public}d", errCode, nsMountCount,
        deviceMountCount, spawnResult);
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, SPAWN_MOUNT_FULL,
        HiSysEvent::EventType::FAULT,
        ERROR_CODE, errCode,
        NAMESPACE_MOUNT_COUNT, nsMountCount,
        DEVICE_MOUNT_COUNT, deviceMountCount,
        SPAWN_RESULT, spawnResult);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportMountFull error, ret: %{public}d", ret);
}

void ReportUnlockMountResult(int32_t uid, int32_t totalCount, int32_t successCount, int32_t failCount, int64_t duration)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, UNLOCK_MOUNT_ALL_DONE,
        HiSysEvent::EventType::STATISTIC,
        UID, uid,
        APP_COUNT, totalCount,
        SUCCESS_COUNT, successCount,
        FAIL_COUNT, failCount,
        DURATION, duration);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportUnlockMountResult error, ret: %{public}d", ret);
}

void ReportUnlockMountAppFail(int32_t uid, const char *bundleName,
    const char *srcPath, const char *destPath, int32_t errorCode)
{
    int ret = HiSysEventWrite(HiSysEvent::Domain::APPSPAWN, UNLOCK_MOUNT_APP_FAIL,
        HiSysEvent::EventType::FAULT,
        UID, uid,
        BUNDLE_NAME, bundleName,
        SRC_PATH, srcPath,
        TARGET_PATH, destPath,
        ERROR_CODE, errorCode);

    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "ReportUnlockMountAppFail error, ret: %{public}d", ret);
}

static bool CheckNeedUpdateReport()
{
    char buffer[PARAM_BUFFER_LEN] = {0};
    int ret = GetParameter(PARAM_APPSPAWN_TIME_MOUNTINFO, "0", buffer, PARAM_BUFFER_LEN);
    APPSPAWN_CHECK_LOGW(ret == 0, return true, "WriteMountInfo not exist param, should report %{public}d", ret);

    long lastSecs = strtol(buffer, nullptr, NUM_DEC);
    APPSPAWN_CHECK(lastSecs > 0, return true, "WriteMountInfo last mount secs invalid");

    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);
    long duration = current.tv_sec - lastSecs;
    // 60 * 60s update mountinfo hisysevent
    APPSPAWN_CHECK(duration <= MOUNTINFO_UPDATE_TIMEOUT, return true, "WriteMountInfo timeout");
    return false;
}

static void UpdateLastReportTime()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    char buffer[PARAM_BUFFER_LEN] = {0};
    int ret = snprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, "%ld", ts.tv_sec);
    APPSPAWN_CHECK(ret > 0, return, "Failed to build mountTime errno %{public}d", errno);
    ret = SetParameter(PARAM_APPSPAWN_TIME_MOUNTINFO, buffer);
    APPSPAWN_CHECK_ONLY_LOG(ret == 0, "WriteMountInfo set mountinfo param failed %{public}d", ret);
}

APPSPAWN_STATIC uint32_t GetMountCountForPid(pid_t pid)
{
    std::string path = "/proc/" + std::to_string(pid) + "/mountcount";
    FILE *fp = fopen(path.c_str(), "r");
    APPSPAWN_CHECK(fp != nullptr, return 0, "Failed to fopen %{public}s errno %{public}d", path.c_str(), errno);

    uint32_t count = 0;
    APPSPAWN_ONLY_EXPER(fscanf_s(fp, "%u", &count) != 1, count = 0);

    (void)fclose(fp);
    return count;
}

APPSPAWN_STATIC uint32_t GetAllMountCount(void)
{
    FILE *fp = fopen("/proc/sys/fs/mount-nr", "r");
    APPSPAWN_CHECK(fp != nullptr, return 0, "Failed to fopen /proc/sys/fs/mount-nr errno %{public}d", errno);

    uint32_t count = 0;
    APPSPAWN_ONLY_EXPER(fscanf_s(fp, "%u", &count) != 1, count = 0);

    (void)fclose(fp);
    return count;
}

APPSPAWN_STATIC void WriteMountInfoToFile(pid_t pid, int appCount, uint32_t mainNsMountCount, uint32_t allMountCount)
{
    auto startTime = std::chrono::steady_clock::now();
    std::ofstream logFile("/data/service/el1/startup/log/mountinfo.log");
    APPSPAWN_CHECK(logFile.is_open(), return, "WriteMountInfo open mountinfo.log failed %{public}d", errno);

    logFile << "==================== Mount Point Full Report ====================\n";
    logFile << "All App Count: " << appCount << '\n';
    logFile << "MainNs Mount Count: " << mainNsMountCount << '\n';
    logFile << "All Mount Count: " << allMountCount << '\n';
    logFile << '\n';

    std::ifstream mountinfoFile("/proc/" + std::to_string(pid) + "/mountinfo");
    APPSPAWN_CHECK(mountinfoFile.is_open(), logFile.close();
        return, "WriteMountInfo open mountinfo failed %{public}d", errno);
    std::string line;
    while (std::getline(mountinfoFile, line)) {
        logFile << line << '\n';
    }
    mountinfoFile.close();

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    APPSPAWN_LOGI("WriteMountInfo completed time use %{public}lld ms", duration);

    logFile << "==================== End of Report time use " << duration << " ms ====================\n";
    logFile.close();
}

void ReportMountFullHisysevent(int32_t errCode)
{
    static bool isReport = false;
    APPSPAWN_CHECK_DUMPI(!isReport, return, "WriteMountInfo already report");

    AppSpawnMgr *mgr = GetAppSpawnMgr();
    APPSPAWN_CHECK(mgr != nullptr, return, "WriteMountInfo invalid mgr");
    APPSPAWN_CHECK_DUMPI(IsAppSpawnMode(mgr), return, "WriteMountInfo not appspawn mode");

    bool needReport = CheckNeedUpdateReport();
    APPSPAWN_CHECK_DUMPI(needReport, return, "WriteMountInfo not need report");

    int appCount = OH_ListGetCnt(&mgr->appQueue);
    uint32_t mainNsMountCount = GetMountCountForPid(mgr->servicePid);
    uint32_t allMountCount = GetAllMountCount();

    WriteMountInfoToFile(mgr->servicePid, appCount, mainNsMountCount, allMountCount);
    ReportMountFull(errCode, mainNsMountCount, allMountCount, appCount);

    isReport = true;
    UpdateLastReportTime();
}
