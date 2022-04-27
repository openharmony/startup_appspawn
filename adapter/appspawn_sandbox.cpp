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

#include <cerrno>
#include <fcntl.h>
#include <map>
#include <sched.h>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "sandbox.h"
#include "sandbox_namespace.h"

bool g_isPrivAppSandboxCreated = false;
bool g_isAppSandboxCreated = false;

constexpr std::string_view APL_SYSTEM_CORE("system_core");
constexpr std::string_view APL_SYSTEM_BASIC("system_basic");
constexpr static int UID_BASE = 200000;
constexpr static mode_t FILE_MODE = 0711;
constexpr static mode_t NWEB_FILE_MODE = 0511;

static void RegisterSandbox(AppSpawnContentExt *appSpawnContent, const char *sandbox)
{
    if (sandbox == nullptr) {
        APPSPAWN_LOGE("AppSpawnServer::invalid parameters");
        return;
    }
    APPSPAWN_LOGE("RegisterSandbox %s", sandbox);
    InitDefaultNamespace();
    if (!InitSandboxWithName(sandbox)) {
        CloseDefaultNamespace();
        APPSPAWN_LOGE("AppSpawnServer::Failed to init sandbox with name %s", sandbox);
        return;
    }

    DumpSandboxByName(sandbox);
    if (PrepareSandbox(sandbox) != 0) {
        APPSPAWN_LOGE("AppSpawnServer::Failed to prepare sandbox %s", sandbox);
        DestroySandbox(sandbox);
        CloseDefaultNamespace();
        return;
    }
    if (EnterDefaultNamespace() < 0) {
        APPSPAWN_LOGE("AppSpawnServer::Failed to set default namespace");
        DestroySandbox(sandbox);
        CloseDefaultNamespace();
        return;
    }
    CloseDefaultNamespace();
    if (strcmp(sandbox, "app") == 0) {
        appSpawnContent->flags |= FLAGS_SANDBOX_APP;
    } else if (strcmp(sandbox, "priv-app") == 0) {
        appSpawnContent->flags |= FLAGS_SANDBOX_PRIVATE;
    }
}

void RegisterAppSandbox(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    AppSpawnContentExt *appSpawnContent = (AppSpawnContentExt *)content;
    APPSPAWN_CHECK(appSpawnContent != NULL, return, "Invalid appspawn content");

    if ((appSpawnContent->flags & FLAGS_SANDBOX_PRIVATE) != FLAGS_SANDBOX_PRIVATE) {
        if (strcmp("system_basic", appProperty->property.apl) == 0) {
            RegisterSandbox(appSpawnContent, "priv-app");
        }
    }
    if ((appSpawnContent->flags & FLAGS_SANDBOX_APP) != FLAGS_SANDBOX_APP) {
        if (strcmp("normal", appProperty->property.apl) == 0) {
            RegisterSandbox(appSpawnContent, "app");
        }
    }
}

static int32_t DoAppSandboxMountOnce(const std::string originPath, const std::string destinationPath)
{
    int rc = 0;

    rc = mount(originPath.c_str(), destinationPath.c_str(), NULL, MS_BIND | MS_REC, NULL);
    if (rc) {
        APPSPAWN_LOGE("bind mount %s to %s failed %d", originPath.c_str(),
            destinationPath.c_str(), errno);
        return rc;
    }

    rc = mount(NULL, destinationPath.c_str(), NULL, MS_PRIVATE, NULL);
    if (rc) {
        APPSPAWN_LOGE("private mount to %s failed %d", destinationPath.c_str(), errno);
        return rc;
    }

    return 0;
}

static int32_t DoAppSandboxMount(const AppParameter &appProperty, std::string rootPath)
{
    std::string currentUserId = std::to_string(appProperty.uid / UID_BASE);
    std::string oriInstallPath = "/data/app/el1/bundle/public/";
    std::string oriel1DataPath = "/data/app/el1/" + currentUserId + "/base/";
    std::string oriel2DataPath = "/data/app/el2/" + currentUserId + "/base/";
    std::string oriDatabasePath = "/data/app/el2/" + currentUserId + "/database/";
    std::string destDatabasePath = rootPath + "/data/storage/el2/database";
    std::string destInstallPath = rootPath + "/data/storage/el1/bundle";
    std::string destel1DataPath = rootPath + "/data/storage/el1/base";
    std::string destel2DataPath = rootPath + "/data/storage/el2/base";

    int rc = 0;

    std::string bundleName = appProperty.bundleName;
    oriInstallPath += bundleName;
    oriel1DataPath += bundleName;
    oriel2DataPath += bundleName;
    oriDatabasePath += bundleName;

    std::map<std::string, std::string> mountMap;
    mountMap[destDatabasePath] = oriDatabasePath;
    mountMap[destInstallPath] = oriInstallPath;
    mountMap[destel1DataPath] = oriel1DataPath;
    mountMap[destel2DataPath] = oriel2DataPath;

    std::map<std::string, std::string>::iterator iter;
    for (iter = mountMap.begin(); iter != mountMap.end(); ++iter) {
        rc = DoAppSandboxMountOnce(iter->second.c_str(), iter->first.c_str());
        if (rc) {
            return rc;
        }
    }

    // to create some useful dir when mount point created
    std::vector<std::string> mkdirInfo;
    std::string dirPath;
    mkdirInfo.push_back("/data/storage/el1/bundle/nweb");
    mkdirInfo.push_back("/data/storage/el1/bundle/ohos.global.systemres");

    for (size_t i = 0; i < mkdirInfo.size(); i++) {
        dirPath = rootPath + mkdirInfo[i];
        mkdir(dirPath.c_str(), FILE_MODE);
    }

    return 0;
}

static int32_t DoAppSandboxMountCustomized(const AppParameter &appProperty, const std::string &rootPath)
{
    std::string bundleName = appProperty.bundleName;
    std::string currentUserId = std::to_string(appProperty.uid / UID_BASE);
    std::string destInstallPath = rootPath + "/data/storage/el1/bundle";
    bool AuthFlag = false;
    const std::vector<std::string> AuthAppList = {"com.ohos.launcher", "com.ohos.permissionmanager"};
    if (std::find(AuthAppList.begin(), AuthAppList.end(), bundleName) != AuthAppList.end()) {
        AuthFlag = true;
    }

    if (strcmp(appProperty.apl, APL_SYSTEM_BASIC.data()) == 0 ||
        strcmp(appProperty.apl, APL_SYSTEM_CORE.data()) == 0 || AuthFlag) {
        // account_0/applications/ dir can still access other packages' data now for compatibility purpose
        std::string oriapplicationsPath = "/data/app/el1/bundle/public/";
        std::string destapplicationsPath = rootPath + "/data/accounts/account_0/applications/";
        DoAppSandboxMountOnce(oriapplicationsPath.c_str(), destapplicationsPath.c_str());

        // need permission check for system app here
        std::string destbundlesPath = rootPath + "/data/bundles/";
        DoAppSandboxMountOnce(oriapplicationsPath.c_str(), destbundlesPath.c_str());
    }

    std::string orimntHmdfsPath = "/mnt/hmdfs/";
    std::string destmntHmdfsPath = rootPath + orimntHmdfsPath;
    DoAppSandboxMountOnce(orimntHmdfsPath.c_str(), destmntHmdfsPath.c_str());

    // Add distributedfile module support, later reconstruct it
    std::string oriDistributedPath = "/mnt/hmdfs/" +  currentUserId + "/account/merge_view/data/" + bundleName;
    std::string destDistributedPath = rootPath + "/data/storage/el2/distributedfiles";
    DoAppSandboxMountOnce(oriDistributedPath.c_str(), destDistributedPath.c_str());

    std::string oriDistributedGroupPath = "/mnt/hmdfs/" +  currentUserId + "/non_account/merge_view/data/" + bundleName;
    std::string destDistributedGroupPath = rootPath + "/data/storage/el2/auth_groups";
    DoAppSandboxMountOnce(oriDistributedGroupPath.c_str(), destDistributedGroupPath.c_str());

    // do nweb adaption
    std::string orinwebPath = "/data/app/el1/bundle/public/com.ohos.nweb";
    std::string destnwebPath = destInstallPath + "/nweb";
    chmod(destnwebPath.c_str(), NWEB_FILE_MODE);
    DoAppSandboxMountOnce(orinwebPath.c_str(), destnwebPath.c_str());

    // do systemres adaption
    std::string oriSysresPath = "/data/app/el1/bundle/public/ohos.global.systemres";
    std::string destSysresPath = destInstallPath + "/ohos.global.systemres";
    chmod(destSysresPath.c_str(), NWEB_FILE_MODE);
    DoAppSandboxMountOnce(oriSysresPath.c_str(), destSysresPath.c_str());

    if (bundleName.find("medialibrary") != std::string::npos) {
        std::string oriMediaPath = "/storage/media/" +  currentUserId;
        std::string destMediaPath = rootPath + "/storage/media";
        DoAppSandboxMountOnce(oriMediaPath.c_str(), destMediaPath.c_str());
    }

    return 0;
}

static void DoAppSandboxMkdir(const std::string &sandboxPackagePath, const AppParameter &appProperty)
{
    std::vector<std::string> mkdirInfo;
    std::string dirPath;

    mkdirInfo.push_back("/mnt/");
    mkdirInfo.push_back("/mnt/hmdfs/");
    mkdirInfo.push_back("/data/");
    mkdirInfo.push_back("/storage/");
    mkdirInfo.push_back("/storage/media");
    mkdirInfo.push_back("/data/storage");
    // to create /mnt/sandbox/<packagename>/data/storage/el1 related path, later should delete this code.
    mkdirInfo.push_back("/data/storage/el1");
    mkdirInfo.push_back("/data/storage/el1/bundle");
    mkdirInfo.push_back("/data/storage/el1/base");
    mkdirInfo.push_back("/data/storage/el1/database");
    mkdirInfo.push_back("/data/storage/el2");
    mkdirInfo.push_back("/data/storage/el2/base");
    mkdirInfo.push_back("/data/storage/el2/database");
    mkdirInfo.push_back("/data/storage/el2/distributedfiles");
    mkdirInfo.push_back("/data/storage/el2/auth_groups");
    // create applications folder for compatibility purpose
    mkdirInfo.push_back("/data/accounts");
    mkdirInfo.push_back("/data/accounts/account_0");
    mkdirInfo.push_back("/data/accounts/account_0/applications/");
    mkdirInfo.push_back("/data/bundles/");

    for (size_t i = 0; i < mkdirInfo.size(); i++) {
        dirPath = sandboxPackagePath + mkdirInfo[i];
        mkdir(dirPath.c_str(), FILE_MODE);
    }
}

static int32_t DoSandboxRootFolderCreateAdapt(const std::string &sandboxPackagePath)
{
    int rc = mount(nullptr, "/", nullptr, MS_REC | MS_SLAVE, nullptr);
    if (rc) {
        APPSPAWN_LOGE("set propagation slave failed");
        return rc;
    }

    // bind mount "/" to /mnt/sandbox/<packageName> path
    // rootfs: to do more resources bind mount here to get more strict resources constraints
    rc = mount("/", sandboxPackagePath.c_str(), nullptr, MS_BIND | MS_REC, nullptr);
    if (rc) {
        APPSPAWN_LOGE("mount bind / failed");
        return rc;
    }

    return 0;
}

static int32_t DoSandboxRootFolderCreate(const std::string &sandboxPackagePath)
{
    int rc = mount(nullptr, "/", nullptr, MS_REC | MS_SLAVE, nullptr);
    if (rc) {
        return rc;
    }

    // bind mount sandboxPackagePath to make it a mount point for pivot_root syscall
    DoAppSandboxMountOnce(sandboxPackagePath.c_str(), sandboxPackagePath.c_str());

    // do /mnt/sandbox/<packageName> path mkdir
    std::map<std::string, std::string> mountMap;
    std::vector<std::string> vecInfo;
    std::string tmpDir = "";

    vecInfo.push_back("/config");
    vecInfo.push_back("/dev");
    vecInfo.push_back("/proc");
    vecInfo.push_back("/sys");
    vecInfo.push_back("/sys_prod");
    vecInfo.push_back("/system");

    for (size_t i = 0; i < vecInfo.size(); i++) {
        tmpDir = sandboxPackagePath + vecInfo[i];
        mkdir(tmpDir.c_str(), FILE_MODE);
        mountMap[vecInfo[i]] = tmpDir;
    }

    // bind mount root folder to /mnt/sandbox/<packageName> path
    std::map<std::string, std::string>::iterator iter;
    for (iter = mountMap.begin(); iter != mountMap.end(); ++iter) {
        rc = DoAppSandboxMountOnce(iter->first.c_str(), iter->second.c_str());
        if (rc) {
            APPSPAWN_LOGE("move root folder failed, %s", sandboxPackagePath.c_str());
        }
    }

    // to create symlink at /mnt/sandbox/<packageName> path
    // bin -> /system/bin
    // d -> /sys/kernel/debug
    // etc -> /system/etc
    // init -> /system/bin/init
    // lib -> /system/lib
    // sdcard -> /storage/self/primary
    std::map<std::string, std::string> symlinkMap;
    symlinkMap["/system/bin"] = sandboxPackagePath + "/bin";
    symlinkMap["/sys/kernel/debug"] = sandboxPackagePath + "/d";
    symlinkMap["/system/etc"] = sandboxPackagePath + "/etc";
    symlinkMap["/system/bin/init"] = sandboxPackagePath + "/init";
#ifdef __aarch64__
    symlinkMap["/system/lib64"] = sandboxPackagePath + "/lib64";
#else
    symlinkMap["/system/lib"] = sandboxPackagePath + "/lib";
#endif

    for (iter = symlinkMap.begin(); iter != symlinkMap.end(); ++iter) {
        symlink(iter->first.c_str(), iter->second.c_str());
    }
    return 0;
}

static void MatchSandbox(AppSpawnClientExt *appProperty)
{
    if (appProperty == nullptr) {
        return;
    }
    if (strcmp("system_basic", appProperty->property.apl) == 0) {
        EnterSandbox("priv-app");
    } else if (strcmp("normal", appProperty->property.apl) == 0) {
        EnterSandbox("app");
    } else if (strcmp("system_core ", appProperty->property.apl) == 0) {
        EnterSandbox("app");
    } else {
        APPSPAWN_LOGE("AppSpawnServer::Failed to match appspawn sandbox %s", appProperty->property.apl);
        EnterSandbox("app");
    }
}

int32_t SetAppSandboxProperty(struct AppSpawnContent_ *content, AppSpawnClient *client)
{
    int rc = 0;
    APPSPAWN_CHECK(client != NULL, return -1, "Invalid appspwn client");
    AppSpawnClientExt *appProperty = (AppSpawnClientExt *)client;
    MatchSandbox(appProperty);
    // create /mnt/sandbox/<packagename> path�?later put it to rootfs module
    std::string sandboxPackagePath = "/";
    sandboxPackagePath += appProperty->property.bundleName;
    mkdir(sandboxPackagePath.c_str(), FILE_MODE);

    // add pid to a new mnt namespace
    rc = unshare(CLONE_NEWNS);
    if (rc) {
        APPSPAWN_LOGE("unshare failed, packagename is %s", appProperty->property.processName);
        return rc;
    }

    // to make wargnar work
    if (access("/3rdmodem", F_OK) == 0) {
        rc = DoSandboxRootFolderCreateAdapt(sandboxPackagePath);
    } else {
        rc = DoSandboxRootFolderCreate(sandboxPackagePath);
    }
    if (rc) {
        APPSPAWN_LOGE("DoSandboxRootFolderCreate failed, %s", appProperty->property.processName);
        return rc;
    }

    // to create /mnt/sandbox/<packagename>/data/storage related path
    DoAppSandboxMkdir(sandboxPackagePath, appProperty->property);

    rc = DoAppSandboxMount(appProperty->property, sandboxPackagePath);
    if (rc) {
        APPSPAWN_LOGE("DoAppSandboxMount failed, packagename is %s", appProperty->property.processName);
        return rc;
    }

    rc = DoAppSandboxMountCustomized(appProperty->property, sandboxPackagePath);
    if (rc) {
        APPSPAWN_LOGE("DoAppSandboxMountCustomized failed, packagename is %s", appProperty->property.processName);
        return rc;
    }

    rc = chdir(sandboxPackagePath.c_str());
    if (rc) {
        APPSPAWN_LOGE("chdir failed, packagename is %s, path is %s", \
            appProperty->property.processName, sandboxPackagePath.c_str());
        return rc;
    }

    rc = syscall(SYS_pivot_root, sandboxPackagePath.c_str(), sandboxPackagePath.c_str());
    if (rc) {
        APPSPAWN_LOGE("pivot root failed, packagename is %s, errno is %d", \
            appProperty->property.processName, errno);
        return rc;
    }

    rc = umount2(".", MNT_DETACH);
    if (rc) {
        APPSPAWN_LOGE("MNT_DETACH failed, packagename is %s", appProperty->property.processName);
        return rc;
    }
    return 0;
}
