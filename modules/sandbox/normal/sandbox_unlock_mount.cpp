/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <cerrno>
#include <cinttypes>
#include <sys/mount.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <cstring>
#include <set>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <queue>
#include <mutex>
#include <functional>

#include "sandbox_unlock_mount.h"
#include "sandbox_shared_mount.h"
#include "sandbox_common.h"
#include "appspawn_utils.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "parameter.h"
#ifdef APPSPAWN_HISYSEVENT
#include "hisysevent_adapter.h"
#endif

#ifndef APPSPAWN_SANDBOX_NEW

constexpr const char *USER_ID_PLACEHOLDER = "<userId>";
constexpr const char *BUNDLE_NAME_PLACEHOLDER = "<bundleName>";

#define DIR_MODE 0711

/**
 * @brief Replace placeholders in path template
 * @param templateStr Path template containing <userId> and <bundleName>
 * @param userId User ID string
 * @param bundleName Bundle name
 * @return Path with placeholders replaced
 */
APPSPAWN_STATIC std::string ReplacePlaceholders(const std::string &templateStr, const std::string &userId,
    const std::string &bundleName)
{
    std::string result = templateStr;

    // Replace <userId> placeholder
    size_t pos = 0;
    while ((pos = result.find(USER_ID_PLACEHOLDER, pos)) != std::string::npos) {
        result.replace(pos, strlen(USER_ID_PLACEHOLDER), userId);
        pos += userId.length();
    }

    // Replace <bundleName> placeholder
    pos = 0;
    while ((pos = result.find(BUNDLE_NAME_PLACEHOLDER, pos)) != std::string::npos) {
        result.replace(pos, strlen(BUNDLE_NAME_PLACEHOLDER), bundleName);
        pos += bundleName.length();
    }

    return result;
}

// 单个挂载任务
struct MountTask {
    size_t configIndex;    // 挂载点配置索引
    size_t bundleIndex;    // 应用索引
};

// 任务队列上下文
struct MountQueueContext {
    std::queue<MountTask> taskQueue;         // 任务队列
    std::mutex queueMutex;                    // 互斥锁
    const UnlockMountEntry *mountConfig;    // 配置指针
    std::vector<LockBundleInfo> bundles;    // 应用列表（直接存储，不是指针）
    std::atomic<int> successCount{0};       // 成功计数
    std::atomic<int> failCount{0};          // 失败计数
    std::atomic<int> skipCount{0};           // 跳过计数
    unsigned int threadCount;                 // 线程数
};

// Returns mount result for one config entry: {success, fail, skip}
APPSPAWN_STATIC MountEntryResult MountSingleConfigEntry(const UnlockMountEntry &config, const LockBundleInfo &bundle)
{
    MountEntryResult result = {0};
    // Calculate userId from bundle.uid
    std::string userIdStr = std::to_string(bundle.uid / UID_BASE);
    std::string srcPath = ReplacePlaceholders(config.srcPath, userIdStr, bundle.bundleName);
    std::string destPath = ReplacePlaceholders(config.destPath, userIdStr, bundle.bundleName);

    // lockPath is the sandbox root path, use it directly
    std::string fullDestPath = bundle.lockPath + destPath;
    APPSPAWN_ONLY_EXPER(!fullDestPath.empty() && fullDestPath.back() == '/',
        fullDestPath.pop_back());

    // Check if dest path exists, fail if not (app hasn't created this directory yet)
    APPSPAWN_ONLY_EXPER(access(fullDestPath.c_str(), F_OK) != 0,
        APPSPAWN_LOGV("Dest path %{public}s not exist, skip", fullDestPath.c_str());
        result.skip++;
        return result);

    // Try mount
    SharedMountArgs arg = {
        .srcPath = srcPath.c_str(),
        .destPath = fullDestPath.c_str(),
        .fsType = nullptr,
        .mountFlags = MS_BIND | MS_REC,
        .options = nullptr,
        .mountSharedFlag = MS_SLAVE
    };
    int ret = DoSharedMount(&arg);
    if (ret != 0) {
        // If ENOENT, check if source path exists
        APPSPAWN_ONLY_EXPER(errno == ENOENT, OHOS::AppSpawn::SandboxCommon::VerifyDirRecursive(srcPath));
        // Other errors are real failures
        APPSPAWN_LOGW("mount %{public}s to %{public}s failed, ret=%{public}d, errno=%{public}d",
            srcPath.c_str(), fullDestPath.c_str(), ret, errno);
        APPSPAWN_KLOGE("UnlockMount: mount %{public}s to %{public}s failed", srcPath.c_str(), fullDestPath.c_str());
        APPSPAWN_KLOGE("uid=%{public}d, ret=%{public}d, errno=%{public}d", bundle.uid, ret, errno);
        result.fail++;
#ifdef APPSPAWN_HISYSEVENT
        ReportUnlockMountAppFail(bundle.uid, bundle.bundleName.c_str(),
            srcPath.c_str(), fullDestPath.c_str(), ret);
#endif
        return result;
    }
    APPSPAWN_LOGV("Mounted %{public}s to %{public}s successfully", srcPath.c_str(), fullDestPath.c_str());
    result.success++;
    return result;
}

// 工作队列核心逻辑：从共享队列中获取任务并执行
APPSPAWN_STATIC void MountQueueWorkerThread(MountQueueContext &ctx, unsigned int workerId)
{
    size_t processedTasks = 0;
    MountTask task;

    while (true) {
        // 从队列中获取任务（加锁保护）
        {
            std::lock_guard<std::mutex> lock(ctx.queueMutex);
            if (ctx.taskQueue.empty()) {
                // 队列为空，退出
                break;
            }
            task = ctx.taskQueue.front();
            ctx.taskQueue.pop();
        }

        // 执行挂载任务（无需锁，避免阻塞其他线程）
        const LockBundleInfo &bundle = ctx.bundles[task.bundleIndex];
        auto r = MountSingleConfigEntry(ctx.mountConfig[task.configIndex], bundle);
        ctx.successCount.fetch_add(r.success, std::memory_order_relaxed);
        ctx.failCount.fetch_add(r.fail, std::memory_order_relaxed);
        ctx.skipCount.fetch_add(r.skip, std::memory_order_relaxed);

        processedTasks++;
    }

    APPSPAWN_LOGV("MountQueueWorkerThread[%{public}u]: processed %{public}zu tasks",
                  workerId, processedTasks);
}

/**
 * @brief Create and initialize mount queue context with all resources
 * @return nullptr if no apps to mount, otherwise the context
 */
APPSPAWN_STATIC std::unique_ptr<MountQueueContext> CreateMountContext(int uid)
{
    // Get mount bundle list
    std::vector<LockBundleInfo> bundles = GetAllLockBundles();

    // Filter bundles by userId
    std::vector<LockBundleInfo> filteredBundles;
    for (const auto &bundle : bundles) {
        APPSPAWN_ONLY_EXPER(bundle.uid / UID_BASE != static_cast<uint32_t>(uid), continue);
        filteredBundles.push_back(bundle);
    }

    if (filteredBundles.empty()) {
        APPSPAWN_LOGI("CreateMountContext: no apps to mount, uid=%{public}d", uid);
        return nullptr;
    }

#ifdef APPSPAWN_HISYSEVENT
    ReportKeyEvent(UNLOCK_MOUNT_SCAN_DONE);
#endif

    // Calculate thread count
    unsigned int threadCount = std::min(
        static_cast<unsigned int>(filteredBundles.size()),
        std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 4
    );

    // Get mount configuration
    size_t configSize = 0;
    const UnlockMountEntry* mountConfig = GetUnlockMountEntry(&configSize);

    // Build task queue: all (configIndex, bundleIndex) combinations
    std::queue<MountTask> taskQueue;
    for (size_t i = 0; i < configSize; i++) {
        for (size_t j = 0; j < filteredBundles.size(); j++) {
            taskQueue.push({i, j});
        }
    }

    // Create context
    auto ctx = std::make_unique<MountQueueContext>();
    ctx->taskQueue = std::move(taskQueue);
    ctx->bundles = std::move(filteredBundles);
    ctx->mountConfig = mountConfig;
    ctx->threadCount = threadCount;

    return ctx;
}

int DoSharedMountForUser(int uid)
{
    APPSPAWN_LOGI("DoSharedMountForUser start, uid=%{public}d, g_lockBundleMap.size=%{public}zu",
                  uid, GetLockBundleMapSize());

    auto mountStart = std::chrono::steady_clock::now();

    // Create mount context (includes getting bundles, filtering, building task queue)
    auto ctx = CreateMountContext(uid);
    if (ctx == nullptr) {
#ifdef APPSPAWN_HISYSEVENT
        ReportKeyEvent(UNLOCK_MOUNT_NO_APPS);
#endif
        return 0;
    }

    // Create worker threads
    std::vector<std::thread> workers;
    for (unsigned int i = 0; i < ctx->threadCount; i++) {
        workers.emplace_back(MountQueueWorkerThread, std::ref(*ctx), i);
    }

    // Wait for all threads to complete
    for (auto &worker : workers) {
        APPSPAWN_ONLY_EXPER(worker.joinable(), worker.join());
    }

    auto mountEnd = std::chrono::steady_clock::now();
    int64_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(mountEnd - mountStart).count();

    APPSPAWN_LOGI("DoSharedMountForUser done, %{public}d, %{public}d, %{public}d, %{public}d, %{public}" PRId64 " ms",
                  uid, ctx->successCount.load(), ctx->failCount.load(),
                  ctx->skipCount.load(), duration);
#ifdef APPSPAWN_HISYSEVENT
    ReportUnlockMountResult(uid, static_cast<int32_t>(ctx->bundles.size()),
        ctx->successCount.load(), ctx->failCount.load(), duration);
#endif
    return 0;
}

/**
 * @brief 处理用户解锁挂载的钩子函数
 * @param mgr AppSpawnMgr 实例
 * @return 成功返回 0，失败返回负数
 */
int HandleUnlockMountForUser(AppSpawnMgr *mgr)
{
    if (mgr == nullptr) {
        return APPSPAWN_ARG_INVALID;
    }

    // 从 AppSpawnContent 中获取当前需要处理的 UID
    AppSpawnContent *content = &mgr->content;
    int uid = content->currentUnlockUid;

    if (uid <= 0) {
        APPSPAWN_LOGE("Invalid uid for unlock mount");
        return APPSPAWN_ARG_INVALID;
    }

    return DoSharedMountForUser(uid);
}

MODULE_CONSTRUCTOR(void)
{
    (void)AddServerStageHook(STAGE_SERVER_LOCK, HOOK_PRIO_COMMON, HandleUnlockMountForUser);
    APPSPAWN_LOGI("RegisterUnlockMountHook: unlock mount hook registered");
}

#endif // APPSPAWN_SANDBOX_NEW
