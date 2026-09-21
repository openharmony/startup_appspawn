/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <securec.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "appspawn_thread_gate.h"
#include "appspawn_utils.h"
#ifdef APPSPAWN_HISYSEVENT
#include "hisysevent_adapter.h"
#endif
#include "loop_event.h"
#include "parameter.h"

#define PIDS_ROOT "/dev/pids/"
#define GATE_GROUP_SUFFIX_MAIN "_main"
#define GATE_GROUP_SUFFIX_SPAWNED "_spawned"
#define GATE_PROC_FILE "/cgroup.procs"
#define GATE_PIDS_MAX_FILE "/pids.max"
#define GATE_PIDS_CUR_FILE "/pids.current"

#define GATE_PIPE_MAGIC 0x5A
#define GATE_FRAME_SIZE 6
#define GATE_TABLE_SIZE 32           // in-flight entries, concurrent spawns are far below this
#define GATE_REMAIN_SIZE 32          // leftover pid list for periodic re-migration
#define GATE_PROC_LINE_LEN 32

// Staged factory default of startup.appspawn.thread.limit: P-C1 dry-run uses a
// large numeric value (never the literal "max" - runtime auto paths must not
// write "max"), P-C2 switches to 1 (same-version cutover with the gate wrap).
#define GATE_LIMIT_DRYRUN_DEFAULT 10000
// P-C2 switch anchor: becomes the default once the gate wrap (M10'②③④/M12')
// ships in the same version. Keep it referenced by the switch, do not remove.
#define GATE_LIMIT_DEFAULT 1
#define GATE_MAX_DEFAULT 256
#define GATE_TIMEOUT_DEFAULT 2       // seconds
#define GATE_RETRY_DEFAULT 5         // seconds
#define GATE_FAIL_LIMIT_DEFAULT 5

#define PARAM_THREAD_LIMIT "startup.appspawn.thread.limit"
#define PARAM_GATE_MAX "startup.appspawn.thread.gate.max"
#define PARAM_GATE_TIMEOUT "startup.appspawn.thread.gate.timeout"
#define PARAM_GATE_RETRY "startup.appspawn.thread.gate.retry"
#define PARAM_GATE_FAIL_LIMIT "startup.appspawn.thread.gate.fail.limit"

#define GATE_EVENT_ARM_FAIL "GATE_ARM_FAIL"
#define GATE_EVENT_DEGRADED "GATE_DEGRADED"
#define GATE_EVENT_CHARGE_LEAK "GATE_CHARGE_LEAK"

// Same shape as GetSpawnNameByRunMode (standard/appspawn_service.c): a private
// copy lives here because that helper is file-static in the executable while
// this module is linked into the appspawn_common so.
typedef struct {
    RunMode mode;
    const char *name;
} GateSpawnNameEntry;
static const GateSpawnNameEntry GATE_SPAWN_NAME_MAP[] = {
    {MODE_FOR_APP_SPAWN, "appspawn"},
    {MODE_FOR_NWEB_SPAWN, "nwebspawn"},
    {MODE_FOR_HYBRID_SPAWN, "hybridspawn"},
    {MODE_FOR_NATIVE_SPAWN, "nativespawn"},
    {MODE_FOR_CJAPP_SPAWN, "cjappspawn"},
};

#pragma pack(push, 1)
typedef struct {
    uint8_t magic;
    uint32_t appId;
    uint8_t taskCnt;
} GateFrame;
#pragma pack(pop)
_Static_assert(sizeof(GateFrame) == GATE_FRAME_SIZE, "gate frame must be 6 bytes");

typedef struct {
    pid_t pid;
    uint32_t appId;          // GATE_APPID_NONE = never frame-pairable
    uint8_t framePairable;
} GateEntry;

enum GateState {
    GATE_STATE_IDLE = 0,     // not armed yet / degraded
    GATE_STATE_ARMED,
    GATE_STATE_DEGRADED,     // fail.limit reached, all gate ops no-op
};

typedef struct {
    int state;
    int dryrun;                       // thread.limit is a large staged value, no real constraint
    int syncSection;                  // between Enter() and RegisterPid/EnterFail()
    int gateOpen;                     // pids.max currently holds gateMax
    char mainGroup[64];               // /dev/pids/<spawner>_main
    char spawnedGroup[64];            // /dev/pids/<spawner>_spawned
    uint32_t limitValue;
    uint32_t gateMax;
    uint32_t timeoutSec;
    uint32_t retrySec;
    uint32_t failLimit;
    int pipeFd[2];                    // [0] read end watched, [1] write end held by parent
    WatcherHandle watcher;
    TimerHandle monitorTimer;         // one-shot open-window watchdog (create-per-use)
    TimerHandle retryTimer;           // periodic: degraded close retry + leftover re-migrate
    GateEntry table[GATE_TABLE_SIZE];
    int tableCnt;
    pid_t remainList[GATE_REMAIN_SIZE];
    int remainCnt;
    int closeRetryPending;            // D3-b flag: close-window write failed, retry periodically
    uint32_t failCnt;                 // consecutive failures, any successful cgroup write resets
    // observation counters (M13'-8 frame miss ratio and friends)
    uint64_t frameTotal;
    uint64_t frameMiss;
    uint64_t frameBad;
    uint64_t missLogCounter;
    uint64_t dropLogCounter;          // notify-write failure rate control
    // charge self-check report control: first report + changed-diff only
    // (60s periodic re-report lands with the P3 sampling timer)
    int lastChargeDiff;
    int chargeLeakReported;
} GateContext;

static GateContext g_gate;

// Forward declarations (single translation unit, callbacks reference helpers
// defined later for readability of the state-machine core).
static void GateRetryTimerCb(const TimerHandle taskHandle, void *context);
static void GateCheckFailLimit(void);
static void GateDropTableByIndex(int index);
static void GateAddRemain(pid_t pid);
static void GateEnsureRetryTimer(void);
static int GateCloseWindow(void);
static int GateOpenWindow(void);
static int GateMigrateToSpawned(pid_t pid);

/**
 * @brief Report a gate lifecycle event via hisysevent when built with it.
 *
 * Logging stays at the call sites (they carry errno/context detail); this
 * helper only fans the event name out to the event channel.
 */
static void GateReportEvent(const char *event)
{
#ifdef APPSPAWN_HISYSEVENT
    ReportKeyEvent(event);
#endif
}

static uint32_t GateReadParamUint(const char *key, uint32_t defValue)
{
    char value[PARAM_BUFFER_LEN] = {0};
    int ret = GetParameter(key, "", value, sizeof(value));
    if (ret <= 0) {
        return defValue;
    }
    for (int i = 0; value[i] != '\0'; i++) {
        if (!isdigit((unsigned char)value[i])) {
            return defValue;
        }
    }
    int v = atoi(value);
    return v > 0 ? (uint32_t)v : defValue;
}

/**
 * @brief Write a decimal pid/number string into a cgroup proc file.
 *
 * @param path Absolute cgroup file path.
 * @param value Decimal number to write (pid or pids.max value).
 * @return 0 on success; -1 on open/write failure (errno logged by caller).
 */
static int GateWriteProcFile(const char *path, long value)
{
    char buf[GATE_PROC_LINE_LEN];
    int ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "%ld", value);
    APPSPAWN_CHECK(ret > 0, return -1, "Invalid value %{public}ld", value);
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    APPSPAWN_CHECK(fd >= 0, return -1, "open %{public}s errno %{public}d", path, errno);
    ssize_t wlen = write(fd, buf, strlen(buf));
    close(fd);
    return (wlen == (ssize_t)strlen(buf)) ? 0 : -1;
}

/**
 * @brief Read a whole small cgroup file (pids.current / pids.max / cgroup.procs).
 *
 * @param path File to read.
 * @param buf Output buffer.
 * @param bufLen Buffer size.
 * @return Read length (>0) on success; -1 on failure.
 */
static int GateReadFile(const char *path, char *buf, uint32_t bufLen)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    APPSPAWN_CHECK(fd >= 0, return -1, "open %{public}s errno %{public}d", path, errno);
    ssize_t rlen = read(fd, buf, bufLen - 1);
    close(fd);
    if (rlen <= 0) {
        return -1;
    }
    buf[rlen] = '\0';
    return (int)rlen;
}

/**
 * @brief Hot-path pids.max write: single write, no read-back (one write per spawn).
 */
static int GateWritePidsMaxHot(const char *group, uint32_t value)
{
    char path[96];
    int ret = snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s%s", group, GATE_PIDS_MAX_FILE);
    APPSPAWN_CHECK(ret > 0, return -1, "snprintf failed");
    return GateWriteProcFile(path, (long)value);
}

/**
 * @brief Cold-path pids.max write: write then read back and compare (arm / D3-a retry).
 */
static int GateWritePidsMaxVerify(const char *group, uint32_t value)
{
    char path[96];
    char buf[GATE_PROC_LINE_LEN] = {0};
    int ret = snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s%s", group, GATE_PIDS_MAX_FILE);
    APPSPAWN_CHECK(ret > 0, return -1, "snprintf failed");
    if (GateWriteProcFile(path, (long)value) != 0) {
        return -1;
    }
    if (GateReadFile(path, buf, sizeof(buf)) <= 0) {
        return -1;
    }
    return (atoi(buf) == (int)value) ? 0 : -1;
}

/**
 * @brief Move a pid into the transitional spawned group (thread-group move via cgroup.procs).
 *
 * @return 0 on success; -1 with ESRCH semantics detected via errno by the caller.
 */
static int GateMigrateToSpawned(pid_t pid)
{
    char path[96];
    int ret = snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s%s", g_gate.spawnedGroup, GATE_PROC_FILE);
    APPSPAWN_CHECK(ret > 0, return -1, "snprintf failed");
    if (GateWriteProcFile(path, (long)pid) != 0) {
        APPSPAWN_LOGE("migrate pid %{public}d to %{public}s errno %{public}d", pid, path, errno);
        return -1;
    }
    return 0;
}

static void GateAddRemain(pid_t pid)
{
    for (int i = 0; i < g_gate.remainCnt; i++) {
        if (g_gate.remainList[i] == pid) {
            return;
        }
    }
    if (g_gate.remainCnt < GATE_REMAIN_SIZE) {
        g_gate.remainList[g_gate.remainCnt++] = pid;
    }
}

/**
 * @brief Snapshot thread names under /proc/self/task for the leak self-check report.
 */
static void GateDumpThreadNames(char *buf, uint32_t bufLen)
{
    DIR *dir = opendir("/proc/self/task");
    APPSPAWN_CHECK_ONLY_EXPER(dir != NULL, return);
    uint32_t used = 0;
    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL && used + 32 < bufLen) {
        if (!isdigit((unsigned char)entry->d_name[0])) {
            continue;
        }
        char commPath[64];
        char comm[32] = {0};
        if (snprintf_s(commPath, sizeof(commPath), sizeof(commPath) - 1,
            "/proc/self/task/%s/comm", entry->d_name) > 0) {
            (void)GateReadFile(commPath, comm, sizeof(comm));
            comm[strcspn(comm, "\n")] = '\0';
        }
        int n = snprintf_s(buf + used, bufLen - used, bufLen - used - 1, "%s(%s) ", entry->d_name, comm);
        if (n <= 0) {
            break;
        }
        used += (uint32_t)n;
    }
    closedir(dir);
}

/**
 * @brief Charge self-check: pids.current must be 1 before a steady close window.
 *
 * Any mismatch is a detectable constraint bypass (e.g. a runtime-dlopen library
 * created threads during an open window); report with thread-name snapshot,
 * still perform the write-back (bounded window first, migrate-then-tighten has
 * its only explicit exemption here).
 */
static void GateCloseSelfCheck(void)
{
    char path[96];
    char buf[GATE_PROC_LINE_LEN] = {0};
    int ret = snprintf_s(path, sizeof(path), sizeof(path) - 1, "%s%s", g_gate.mainGroup, GATE_PIDS_CUR_FILE);
    APPSPAWN_CHECK(ret > 0, return, "snprintf failed");
    if (GateReadFile(path, buf, sizeof(buf)) <= 0) {
        return;
    }
    int diff = atoi(buf) - 1;
    if (diff <= 0) {
        g_gate.chargeLeakReported = 0;  // recovered: re-arm first-report
        return;
    }
    // Report control: first occurrence of a leak + every change of the diff
    // value. A steady leak must not spam one ERROR per close window (one per
    // spawn); the 60s periodic re-report lands with the P3 sampling timer.
    if (g_gate.chargeLeakReported && diff == g_gate.lastChargeDiff) {
        return;
    }
    g_gate.chargeLeakReported = 1;
    g_gate.lastChargeDiff = diff;
    char names[512] = {0};
    GateDumpThreadNames(names, sizeof(names));
    APPSPAWN_LOGE("gate close self-check: pids.current=%{public}s != 1 (dry-run or leak), threads: %{public}s",
        buf, names);
    GateReportEvent(GATE_EVENT_CHARGE_LEAK);
}

/**
 * @brief Create the periodic retry timer idempotently (D3-b close retry + leftover migrate).
 */
static void GateEnsureRetryTimer(void)
{
    if (g_gate.retryTimer != NULL) {
        return;
    }
    LE_STATUS status = LE_CreateTimer(LE_GetDefaultLoop(), &g_gate.retryTimer, GateRetryTimerCb, NULL);
    if (status != LE_SUCCESS) {
        g_gate.retryTimer = NULL;
        APPSPAWN_LOGE("create retry timer failed %{public}d", status);
        return;
    }
    status = LE_StartTimer(LE_GetDefaultLoop(), g_gate.retryTimer,
        (uint64_t)g_gate.retrySec * 1000, INT64_MAX);  // periodic, never rearmed
    if (status != LE_SUCCESS) {
        LE_StopTimer(LE_GetDefaultLoop(), g_gate.retryTimer);
        g_gate.retryTimer = NULL;
        APPSPAWN_LOGE("start retry timer failed %{public}d", status);
    }
}

/**
 * @brief Physically close the gate: self-check then write pids.max back with verify.
 *
 * Monitor revocation binds to a *successful* physical close write (single live
 * monitor invariant): on failure the monitor keeps watching the degraded
 * window and the retry timer takes over; the next Enter destroys the stale
 * handle before creating a new one.
 *
 * @return 0 on success; -1 when the close write failed (D3-b path armed).
 */
static int GateCloseWindow(void)
{
    GateCloseSelfCheck();
    if (GateWritePidsMaxVerify(g_gate.mainGroup, g_gate.limitValue) != 0) {
        g_gate.failCnt++;
        g_gate.closeRetryPending = 1;
        GateEnsureRetryTimer();
        APPSPAWN_LOGE("close gate failed, retry every %{public}us, failCnt %{public}u",
            g_gate.retrySec, g_gate.failCnt);
        GateCheckFailLimit();
        return -1;
    }
    g_gate.failCnt = 0;
    g_gate.gateOpen = 0;
    g_gate.closeRetryPending = 0;
    if (g_gate.monitorTimer != NULL) {
        // Safe here: node waits in timerList (not PROCESSING), CancelTimer unlinks + frees.
        LE_StopTimer(LE_GetDefaultLoop(), g_gate.monitorTimer);
        g_gate.monitorTimer = NULL;
    }
    return 0;
}

/**
 * @brief D2 watchdog callback: force-collect the in-flight table and close the window.
 *
 * One-shot timer: the handle is already freed by loop_event after firing, so
 * only NULL it here (handle invalidation red line).
 */
static void GateMonitorTimerCb(const TimerHandle taskHandle, void *context)
{
    (void)taskHandle;
    (void)context;
    g_gate.monitorTimer = NULL;  // fired one-shot, loop freed it already
    APPSPAWN_LOGW("gate D2 timeout, force collect %{public}d entries", g_gate.tableCnt);
    while (g_gate.tableCnt > 0) {
        GateEntry entry = g_gate.table[0];
        // Consume by index 0: DropTableByIndex moves the last entry into the hole.
        GateDropTableByIndex(0);
        if (GateMigrateToSpawned(entry.pid) != 0 && errno != ESRCH) {
            GateAddRemain(entry.pid);
            GateEnsureRetryTimer();
        }
    }
    g_gate.syncSection = 0;
    (void)GateCloseWindow();
}

/**
 * @brief Periodic callback: retry the close write (only when table empty) and re-migrate leftovers.
 */
static void GateRetryTimerCb(const TimerHandle taskHandle, void *context)
{
    (void)context;
    if (g_gate.closeRetryPending) {
        if (g_gate.tableCnt == 0 && !g_gate.syncSection) {
            // Table non-empty (new spawns in flight during the degraded window):
            // skip this round to avoid migrating children before their setcon point.
            if (GateWritePidsMaxVerify(g_gate.mainGroup, g_gate.limitValue) == 0) {
                g_gate.failCnt = 0;
                g_gate.gateOpen = 0;
                g_gate.closeRetryPending = 0;
                APPSPAWN_LOGI("gate close retry succeeded");
            } else {
                g_gate.failCnt++;
                GateCheckFailLimit();
            }
        }
    }
    for (int i = g_gate.remainCnt - 1; i >= 0; i--) {
        if (GateMigrateToSpawned(g_gate.remainList[i]) == 0 || errno == ESRCH) {
            // Success or already dead: drop from the leftover list.
            for (int j = i; j < g_gate.remainCnt - 1; j++) {
                g_gate.remainList[j] = g_gate.remainList[j + 1];
            }
            g_gate.remainCnt--;
        }
    }
    if (!g_gate.closeRetryPending && g_gate.remainCnt == 0 && !g_gate.gateOpen) {
        // Everything settled and the gate is physically closed: self-revoke.
        // Safe in PROCESSING state: CancelTimer only marks CANCELED, loop frees after return.
        LE_StopTimer(LE_GetDefaultLoop(), taskHandle);
        g_gate.retryTimer = NULL;
    }
}

/**
 * @brief fail.limit terminal check: permanent degrade with observability.
 */
static void GateCheckFailLimit(void)
{
    if (g_gate.state != GATE_STATE_ARMED || g_gate.failCnt < g_gate.failLimit) {
        return;
    }
    g_gate.state = GATE_STATE_DEGRADED;
    APPSPAWN_LOGE("gate degraded permanently: %{public}u consecutive cgroup write failures", g_gate.failCnt);
    GateReportEvent(GATE_EVENT_DEGRADED);
}

static void GateDropTableByIndex(int index)
{
    if (index < 0 || index >= g_gate.tableCnt) {
        return;
    }
    g_gate.table[index] = g_gate.table[g_gate.tableCnt - 1];
    g_gate.tableCnt--;
}

/**
 * @brief Open the gate and (re)create the one-shot monitor respecting the single-live invariant.
 */
static int GateOpenWindow(void)
{
    // Single live monitor invariant: destroy a stale handle (D3-b window where
    // the close write failed) before creating a fresh one; this runs on the
    // main loop thread, never inside a timer callback.
    if (g_gate.monitorTimer != NULL) {
        LE_StopTimer(LE_GetDefaultLoop(), g_gate.monitorTimer);
        g_gate.monitorTimer = NULL;
    }
    // D3-a: hot-path open write, retry once on failure.
    if (GateWritePidsMaxHot(g_gate.mainGroup, g_gate.gateMax) != 0 &&
        GateWritePidsMaxHot(g_gate.mainGroup, g_gate.gateMax) != 0) {
        // Spec D3-a: the Enter failure path revokes the sync-section flag by
        // itself and never falls back to a close write (the gate never opened,
        // a third write to the same just-failed file would fail anyway).
        g_gate.syncSection = 0;
        g_gate.failCnt++;
        GateCheckFailLimit();
        APPSPAWN_LOGE("gate open failed (fork would EAGAIN), failCnt %{public}u", g_gate.failCnt);
        return -1;
    }
    g_gate.failCnt = 0;
    g_gate.gateOpen = 1;
    LE_STATUS status = LE_CreateTimer(LE_GetDefaultLoop(), &g_gate.monitorTimer, GateMonitorTimerCb, NULL);
    if (status != LE_SUCCESS) {
        // Monitor missing degrades to D1 SIGCHLD + retry-timer convergence only.
        g_gate.monitorTimer = NULL;
        APPSPAWN_LOGE("create monitor timer failed %{public}d, D2 watchdog missing", status);
    } else {
        status = LE_StartTimer(LE_GetDefaultLoop(), g_gate.monitorTimer,
            (uint64_t)g_gate.timeoutSec * 1000, 0);  // one-shot: deadline fixed at open edge
        if (status != LE_SUCCESS) {
            LE_StopTimer(LE_GetDefaultLoop(), g_gate.monitorTimer);
            g_gate.monitorTimer = NULL;
            APPSPAWN_LOGE("start monitor timer failed %{public}d", status);
        }
    }
    return 0;
}

/**
 * @brief Pipe watcher callback: drain all frames, match by appId, ignore EOF/HUP/ERR.
 */
static void GatePipeEvent(const WatcherHandle taskHandle, int fd, uint32_t *events, const void *context)
{
    (void)taskHandle;
    (void)context;
    (void)events;  // EOF/HUP/EPOLLERR are explicitly ignored: parent holds the write end
    char buf[GATE_FRAME_SIZE * 16];
    ssize_t rlen;
    while ((rlen = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t off = 0;
        while (off + GATE_FRAME_SIZE <= rlen) {
            GateFrame frame;
            errno_t err = memcpy_s(&frame, sizeof(frame), buf + off, GATE_FRAME_SIZE);
            if (err != EOK) {
                break;
            }
            off += GATE_FRAME_SIZE;
            g_gate.frameTotal++;
            if (frame.magic != GATE_PIPE_MAGIC) {
                // Bad frame: drop exactly one frame slot, count separately
                // (never mixed into the miss-ratio metric).
                g_gate.frameBad++;
                continue;
            }
            int hit = -1;
            for (int i = 0; i < g_gate.tableCnt; i++) {
                if (g_gate.table[i].framePairable && g_gate.table[i].appId == frame.appId) {
                    hit = i;
                    break;
                }
            }
            if (hit < 0) {
                // Expected ~100% in P-C1 dry-run (no RegisterPid consumer yet);
                // after P-C2 this metric flags pidns/protocol mismatch when it jumps.
                g_gate.frameMiss++;
                if (g_gate.missLogCounter++ % 1000 == 0) {
                    APPSPAWN_LOGI("gate frame miss ratio sample: total %{public}llu miss %{public}llu bad %{public}llu",
                        (unsigned long long)g_gate.frameTotal,
                        (unsigned long long)g_gate.frameMiss,
                        (unsigned long long)g_gate.frameBad);
                }
                continue;
            }
            GateEntry entry = g_gate.table[hit];
            GateDropTableByIndex(hit);
            // Same consume semantics as SpawnGateLeave: migrate out, ESRCH means
            // the child already died (kernel auto-uncharge), other failures go
            // to the leftover list for periodic re-migration.
            if (GateMigrateToSpawned(entry.pid) != 0 && errno != ESRCH) {
                GateAddRemain(entry.pid);
                GateEnsureRetryTimer();
            }
            if (g_gate.tableCnt == 0 && !g_gate.syncSection) {
                (void)GateCloseWindow();
            }
        }
        // trailing partial frame (< 6 bytes) is defensively kept for the next callback
    }
}

int SpawnGateArm(AppSpawnMgr *content)
{
    // Arm-point invariant guard: AppSpawnRun calls us after STAGE_SERVER_PRELOAD
    // (preloader forks / library init threads would be rejected otherwise) and
    // before LE_RunLoop; a usable default loop at this point is the side proof.
    APPSPAWN_CHECK(content != NULL && LE_GetDefaultLoop() != NULL, return -1,
        "gate arm skipped: preload-not-finished or no loop");

    const char *spawnName = NULL;
    for (size_t i = 0; i < sizeof(GATE_SPAWN_NAME_MAP) / sizeof(GATE_SPAWN_NAME_MAP[0]); i++) {
        if (GATE_SPAWN_NAME_MAP[i].mode == content->content.mode) {
            spawnName = GATE_SPAWN_NAME_MAP[i].name;
            break;
        }
    }
    APPSPAWN_CHECK(spawnName != NULL, return -1, "gate arm skipped: unknown spawn mode %{public}d",
        content->content.mode);

    int ret = snprintf_s(g_gate.mainGroup, sizeof(g_gate.mainGroup), sizeof(g_gate.mainGroup) - 1,
        "%s%s%s", PIDS_ROOT, spawnName, GATE_GROUP_SUFFIX_MAIN);
    APPSPAWN_CHECK(ret > 0, return -1, "snprintf main group failed");
    ret = snprintf_s(g_gate.spawnedGroup, sizeof(g_gate.spawnedGroup), sizeof(g_gate.spawnedGroup) - 1,
        "%s%s%s", PIDS_ROOT, spawnName, GATE_GROUP_SUFFIX_SPAWNED);
    APPSPAWN_CHECK(ret > 0, return -1, "snprintf spawned group failed");

    // Directory existence check prevents cross-spawner group mismatch (e.g.
    // hybridspawn writing appspawn_main succeeds at DAC level but corrupts both).
    struct stat st = {0};
    if (stat(g_gate.mainGroup, &st) != 0 || stat(g_gate.spawnedGroup, &st) != 0) {
        APPSPAWN_LOGE("gate group dir missing, degrade to rule-only constraint: %{public}s",
            g_gate.mainGroup);
        GateReportEvent(GATE_EVENT_ARM_FAIL);
        return -1;
    }

    g_gate.limitValue = GateReadParamUint(PARAM_THREAD_LIMIT, GATE_LIMIT_DRYRUN_DEFAULT);
    g_gate.gateMax = GateReadParamUint(PARAM_GATE_MAX, GATE_MAX_DEFAULT);
    g_gate.timeoutSec = GateReadParamUint(PARAM_GATE_TIMEOUT, GATE_TIMEOUT_DEFAULT);
    g_gate.retrySec = GateReadParamUint(PARAM_GATE_RETRY, GATE_RETRY_DEFAULT);
    g_gate.failLimit = GateReadParamUint(PARAM_GATE_FAIL_LIMIT, GATE_FAIL_LIMIT_DEFAULT);
    g_gate.dryrun = (g_gate.limitValue > 1) ? 1 : 0;  // staged default 10000 = dry-run

    // Step 1: self-migrate first (order matters: migrating after setting the
    // limit could self-lock the migration itself).
    char procPath[96];
    ret = snprintf_s(procPath, sizeof(procPath), sizeof(procPath) - 1,
        "%s%s", g_gate.mainGroup, GATE_PROC_FILE);
    APPSPAWN_CHECK(ret > 0, return -1, "snprintf proc path failed");
    if (GateWriteProcFile(procPath, (long)getpid()) != 0) {
        APPSPAWN_LOGE("gate self-migrate failed errno %{public}d, degrade (first boot: group never limited)",
            errno);
        GateReportEvent(GATE_EVENT_ARM_FAIL);
        return -1;
    }

    // Step 1': scan leftovers in the group - drain immediately once, anything
    // that fails goes to the periodic re-migrate list (cross-generation residue
    // from a previous service instance that died mid-convergence).
    char buf[2048];
    int rlen = GateReadFile(procPath, buf, sizeof(buf));
    if (rlen > 0) {
        char *savePtr = NULL;
        char *line = strtok_r(buf, "\n", &savePtr);
        while (line != NULL) {
            pid_t pid = atoi(line);
            if (pid > 0 && pid != getpid()) {
                if (GateMigrateToSpawned(pid) != 0 && errno != ESRCH) {
                    GateAddRemain(pid);
                }
            }
            line = strtok_r(NULL, "\n", &savePtr);
        }
    }

    // Step 2: self-check - pids.current>1 means threads exist (or dry-run residue);
    // ERROR with thread names, never blocks arming or service start.
    char curPath[96];
    char curBuf[GATE_PROC_LINE_LEN] = {0};
    ret = snprintf_s(curPath, sizeof(curPath), sizeof(curPath) - 1,
        "%s%s", g_gate.mainGroup, GATE_PIDS_CUR_FILE);
    if (ret > 0 && GateReadFile(curPath, curBuf, sizeof(curBuf)) > 0 && atoi(curBuf) > 1) {
        char names[512] = {0};
        GateDumpThreadNames(names, sizeof(names));
        APPSPAWN_LOGE("gate arm self-check: pids.current=%{public}s > 1, threads: %{public}s", curBuf, names);
        GateReportEvent(GATE_EVENT_CHARGE_LEAK);
    }

    // Step 3: set the limit (cold path: write + read-back verify).
    if (GateWritePidsMaxVerify(g_gate.mainGroup, g_gate.limitValue) != 0) {
        // Step 4: partial failure rollback - migrate ourselves out to the
        // transitional group (same permission domain, pids.max explicitly "max"
        // in cfg) so that "degraded, unlimited" classification matches reality.
        char spawnedProc[96];
        int r2 = snprintf_s(spawnedProc, sizeof(spawnedProc), sizeof(spawnedProc) - 1,
            "%s%s", g_gate.spawnedGroup, GATE_PROC_FILE);
        if (r2 > 0 && GateWriteProcFile(spawnedProc, (long)getpid()) == 0) {
            APPSPAWN_LOGE("gate set-limit failed, rolled back to spawned group, degrade");
        } else {
            // Stuck state: inside the limited group with both write paths dead.
            // One-shot self-restart re-evaluation is wired at P-C2 with M10'
            // SIGCHLD wiring; observable immediately here.
            g_gate.state = GATE_STATE_DEGRADED;
            APPSPAWN_LOGE("gate stuck state: rollback failed too, in limited group with dead writes");
            GateReportEvent(GATE_EVENT_DEGRADED);
        }
        GateReportEvent(GATE_EVENT_ARM_FAIL);
        return -1;
    }

    // Arming succeeded: create the gate pipe and the resident drain watcher.
    // O_CLOEXEC on both ends: never leak to spawned apps; O_NONBLOCK write end:
    // a full pipe drops the frame (D2 is the safety net), never blocks setcon.
    if (pipe2(g_gate.pipeFd, O_NONBLOCK | O_CLOEXEC) != 0) {
        APPSPAWN_LOGE("gate pipe2 failed errno %{public}d, notify channel dead", errno);
        close(g_gate.pipeFd[0]);
        close(g_gate.pipeFd[1]);
        g_gate.pipeFd[0] = -1;
        g_gate.pipeFd[1] = -1;
    } else {
        LE_WatchInfo watchInfo = {0};
        watchInfo.fd = g_gate.pipeFd[0];
        watchInfo.flags = 0;  // resident watcher (not WATCHER_ONCE)
        watchInfo.events = EVENT_READ;
        watchInfo.processEvent = GatePipeEvent;
        LE_STATUS status = LE_StartWatcher(LE_GetDefaultLoop(), &g_gate.watcher, &watchInfo, NULL);
        if (status != LE_SUCCESS) {
            APPSPAWN_LOGE("gate watcher failed %{public}d", status);
            g_gate.watcher = NULL;
        }
    }

    if (g_gate.remainCnt > 0) {
        GateEnsureRetryTimer();
    }
    g_gate.state = GATE_STATE_ARMED;
    g_gate.failCnt = 0;
    APPSPAWN_LOGI("gate armed: group %{public}s limit %{public}u%s gateMax %{public}u timeout %{public}us "
        "retry %{public}us failLimit %{public}u leftover %{public}d",
        g_gate.mainGroup, g_gate.limitValue, g_gate.dryrun ? " (dry-run)" : "",
        g_gate.gateMax, g_gate.timeoutSec, g_gate.retrySec, g_gate.failLimit, g_gate.remainCnt);
    return 0;
}

int SpawnGateEnter(void)
{
    if (g_gate.state != GATE_STATE_ARMED) {
        return 0;  // unarmed/degraded: gate ops are no-op, spawns never blocked by us
    }
    g_gate.syncSection = 1;
    if (g_gate.gateOpen) {
        return 0;  // already open: entering never re-arms the open-edge timer
    }
    return GateOpenWindow();
}

void SpawnGateRegisterPid(pid_t pid, uint32_t appId)
{
    if (g_gate.state != GATE_STATE_ARMED || pid <= 0) {
        return;
    }
    if (g_gate.tableCnt >= GATE_TABLE_SIZE) {
        // Revoke the sync flag before bailing out: the entry stays unregistered
        // (its frame misses and D2 converges - the documented degradation), but
        // a stale flag would block every other close window until D2 fires.
        g_gate.syncSection = 0;
        APPSPAWN_LOGE("gate table full (%{public}d), frame will miss and fall to D2", g_gate.tableCnt);
        return;
    }
    GateEntry *entry = &g_gate.table[g_gate.tableCnt++];
    entry->pid = pid;
    entry->appId = appId;
    entry->framePairable = (appId != GATE_APPID_NONE) ? 1 : 0;
    g_gate.syncSection = 0;
}

void SpawnGateEnterFail(void)
{
    if (g_gate.state != GATE_STATE_ARMED) {
        return;
    }
    g_gate.syncSection = 0;
    if (g_gate.tableCnt == 0 && !g_gate.gateOpen) {
        return;
    }
    if (g_gate.tableCnt == 0) {
        (void)GateCloseWindow();  // synchronous rollback: no 2s window leaked
    }
}

void SpawnGateLeave(pid_t pid)
{
    if (g_gate.state != GATE_STATE_ARMED || pid <= 0) {
        return;
    }
    int hit = -1;
    for (int i = 0; i < g_gate.tableCnt; i++) {
        if (g_gate.table[i].pid == pid) {
            hit = i;
            break;
        }
    }
    if (hit < 0) {
        return;  // unmatched: no-op (late frames / repeated SIGCHLD)
    }
    GateEntry entry = g_gate.table[hit];
    GateDropTableByIndex(hit);
    // ESRCH write means the child already died: entry consumed, kernel auto-uncharge.
    if (GateMigrateToSpawned(entry.pid) != 0 && errno != ESRCH) {
        GateAddRemain(entry.pid);
        GateEnsureRetryTimer();
    }
    if (g_gate.tableCnt == 0 && !g_gate.syncSection) {
        (void)GateCloseWindow();
    }
}

void SpawnGateNotify(AppSpawnClient *client)
{
    // First-line no-op guard: cold-run children and unarmed services stop here.
    // The child inherits the parent's armed state through fork, so armed means
    // the parent armed before forking this child.
    if (g_gate.state != GATE_STATE_ARMED || client == NULL || g_gate.pipeFd[1] < 0) {
        return;
    }
    GateFrame frame;
    frame.magic = GATE_PIPE_MAGIC;
    frame.appId = client->id;  // never getpid(): pidns children never match
    frame.taskCnt = 0;
    DIR *dir = opendir("/proc/self/task");
    if (dir != NULL) {
        uint8_t cnt = 0;
        struct dirent *entry = NULL;
        while ((entry = readdir(dir)) != NULL && cnt < 255) {
            if (isdigit((unsigned char)entry->d_name[0])) {
                cnt++;
            }
        }
        closedir(dir);
        frame.taskCnt = cnt;
    }
    ssize_t wlen = write(g_gate.pipeFd[1], &frame, sizeof(frame));
    if (wlen != (ssize_t)sizeof(frame)) {
        // EAGAIN (pipe full) / EPIPE: drop the frame, D2 timeout is the net.
        // Rate-limit: a persistently full pipe must not log per frame.
        if (g_gate.dropLogCounter++ % 1000 == 0) {
            APPSPAWN_LOGW("gate notify dropped (count %{public}llu), errno %{public}d",
                (unsigned long long)g_gate.dropLogCounter, errno);
        }
    }
}
