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

// 工作线程上下文，传递任务列表和计数器
struct MountWorkerContext {
    std::vector<LockBundleInfo> *bundles;
    std::atomic<int> *successCount;
    std::atomic<int> *failCount;
    std::atomic<int> *skipCount;
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

// 无锁工作线程函数：每个线程处理 index, index+mod, index+2*mod, ... 的挂载点配置
// 对每个挂载点，处理所有包（保证耗时挂载点在最后）
APPSPAWN_STATIC void MountWorkerThread(unsigned int index, unsigned int mod, const MountWorkerContext &ctx)
{
    size_t configSize = 0;
    const UnlockMountEntry* mountConfig = GetUnlockMountEntry(&configSize);

    // Process mount point configs in parallel (index, index+mod, index+2*mod, ...)
    for (size_t i = index; i < configSize; i += mod) {
        // For this mount point, process all bundles
        for (const auto &bundle : *ctx.bundles) {
            auto r = MountSingleConfigEntry(mountConfig[i], bundle);
            *ctx.successCount += r.success;
            *ctx.failCount += r.fail;
            *ctx.skipCount += r.skip;
        }
        APPSPAWN_LOGV("MountWorkerThread[%{public}u]: mount point %{public}zu done",
                      index, i);
    }
}

int DoSharedMountForUser(int uid)
{
    APPSPAWN_LOGI("DoSharedMountForUser start, uid=%{public}d, g_lockBundleMap.size=%{public}zu",
                  uid, GetLockBundleMapSize());

    // Get mount bundle list from g_lockBundleMap (replaces opendir + _preunlock suffix scan)
    std::vector<LockBundleInfo> bundles = GetAllLockBundles();

    // Filter bundles by userId and other conditions
    std::vector<LockBundleInfo> filteredBundles;
    for (const auto &bundle : bundles) {
        // Filter by userId
        APPSPAWN_ONLY_EXPER(bundle.uid / UID_BASE != static_cast<uint32_t>(uid), continue);
        filteredBundles.push_back(bundle);
    }

    if (filteredBundles.empty()) {
        APPSPAWN_LOGI("DoSharedMountForUser: no apps to mount, uid=%{public}d", uid);
#ifdef APPSPAWN_HISYSEVENT
        ReportKeyEvent(UNLOCK_MOUNT_NO_APPS);
#endif
        return 0;
    }

#ifdef APPSPAWN_HISYSEVENT
    ReportKeyEvent(UNLOCK_MOUNT_SCAN_DONE);
#endif

    // 第二阶段：多线程并行挂载（无锁分片模式）
    auto mountStart = std::chrono::steady_clock::now();
    const unsigned int threadCount = std::min(
        static_cast<unsigned int>(filteredBundles.size()),
        std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 4
    );

    APPSPAWN_LOGI("DoSharedMountForUser: mounting %{public}zu apps with %{public}u threads (lock-free)",
                  filteredBundles.size(), threadCount);

    // 使用局部变量，避免全局状态重入风险
    std::atomic<int> mountSuccessCount{0};
    std::atomic<int> mountFailCount{0};
    std::atomic<int> mountSkipCount{0};
    MountWorkerContext ctx = { &filteredBundles, &mountSuccessCount, &mountFailCount, &mountSkipCount };

    // 创建工作线程，每个线程传入 (index, mod, ctx) 进行分片处理
    std::vector<std::thread> workers;
    for (unsigned int i = 0; i < threadCount; i++) {
        workers.emplace_back(MountWorkerThread, i, threadCount, std::cref(ctx));
    }

    // 等待所有线程结束
    for (auto &worker : workers) {
        APPSPAWN_ONLY_EXPER(worker.joinable(), worker.join());
    }

    auto mountEnd = std::chrono::steady_clock::now();
    int64_t duration = std::chrono::duration_cast<std::chrono::milliseconds>(mountEnd - mountStart).count();

    APPSPAWN_LOGI("DoSharedMountForUser done, %{public}d, %{public}d, %{public}d, %{public}d, %{public}" PRId64 " ms",
                  uid, mountSuccessCount.load(), mountFailCount.load(), mountSkipCount.load(), duration);
#ifdef APPSPAWN_HISYSEVENT
    ReportUnlockMountResult(uid, static_cast<int32_t>(filteredBundles.size()),
        mountSuccessCount.load(), mountFailCount.load(), duration);
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

#endif // APPSPAWN_SANDBOX_NEW
