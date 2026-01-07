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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <dlfcn.h>

#include "policycoreutils.h"
#ifdef CODE_SIGNATURE_ENABLE
#include "code_sign_utils_in_c.h"
#endif
#include "hnp_installer.h"

#if defined(__aarch64__) || defined(__x86_64__)
#define BIN_SEC_PATH "/system/lib64/libsps_binary_security_sdk.z.so"
#else
#define BIN_SEC_PATH "/system/lib/libsps_binary_security_sdk.z.so"
#endif
#define HNP_BASE_DEC (10)
#define HNP_BASE_UID (100)
#ifdef __cplusplus
extern "C" {
#endif

static int HnpInstallerUidGet(const char *uidIn, int *uidOut)
{
    int index;

    for (index = 0; uidIn[index] != '\0'; index++) {
        if (!isdigit(uidIn[index])) {
            return HNP_ERRNO_INSTALLER_ARGV_UID_INVALID;
        }
    }

    *uidOut = atoi(uidIn); // 转化为10进制
    HNP_ERROR_CHECK(*uidOut >= HNP_BASE_UID, return HNP_ERRNO_PARAM_INVALID, "invalid uid");
    return 0;
}

static int HnpGenerateSoftLinkAllByJson(const char *installPath, const char *dstPath, HnpCfgInfo *hnpCfg,
    bool canRecovery, bool isPublic)
{
    char srcFile[MAX_FILE_PATH_LEN];
    char dstFile[MAX_FILE_PATH_LEN];
    NativeBinLink *currentLink = hnpCfg->links;
    char *fileNameTmp;

    if (access(dstPath, F_OK) != 0) {
        int ret = mkdir(dstPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            HNP_LOGE("mkdir [%{public}s] unsuccess, ret=%{public}d, errno:%{public}d", dstPath, ret, errno);
            return HNP_ERRNO_BASE_MKDIR_PATH_FAILED;
        }
    }

    for (unsigned int i = 0; i < hnpCfg->linkNum; i++) {
        if (strstr(currentLink->source, "..") || strstr(currentLink->target, "..")) {
            HNP_LOGE("hnp json link source[%{public}s],target[%{public}s],does not allow the use of ..",
                currentLink->source, currentLink->target);
            return HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED;
        }
        int ret = sprintf_s(srcFile, MAX_FILE_PATH_LEN, "%s/%s", installPath, currentLink->source);
        char *fileName;
        if (ret < 0) {
            HNP_LOGE("sprintf install bin src file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        /* 如果target为空则使用源二进制名称 */
        if (strcmp(currentLink->target, "") == 0) {
            fileNameTmp = currentLink->source;
        } else {
            fileNameTmp = currentLink->target;
        }
        fileName = strrchr(fileNameTmp, DIR_SPLIT_SYMBOL);
        if (fileName == NULL) {
            fileName = fileNameTmp;
        } else {
            fileName++;
        }
        ret = sprintf_s(dstFile, MAX_FILE_PATH_LEN, "%s/%s", dstPath, fileName);
        HNP_ERROR_CHECK(ret > 0, return HNP_ERRNO_BASE_SPRINTF_FAILED,
            "sprintf install bin dst file unsuccess.");

        /* 生成软链接 */
        ret = HnpSymlink(srcFile, dstFile, hnpCfg, canRecovery, isPublic);
        HNP_ERROR_CHECK(ret == 0, return ret, "hnpSymlink failed");

        currentLink++;
    }

    return 0;
}

static int HnpGenerateSoftLinkAll(const char *installPath, const char *dstPath, HnpCfgInfo *hnpCfg,
    bool canRecovery, bool isPublic)
{
    char srcPath[MAX_FILE_PATH_LEN];
    char srcFile[MAX_FILE_PATH_LEN];
    char dstFile[MAX_FILE_PATH_LEN];
    int ret;
    DIR *dir;
    struct dirent *entry;

    ret = sprintf_s(srcPath, MAX_FILE_PATH_LEN, "%s/bin", installPath);
    HNP_ERROR_CHECK(ret > 0, return HNP_ERRNO_BASE_SPRINTF_FAILED,
        "sprintf install bin path unsuccess.");

    if ((dir = opendir(srcPath)) == NULL) {
        HNP_LOGI("soft link bin file:%{public}s not exist", srcPath);
        return 0;
    }

    if (access(dstPath, F_OK) != 0) {
        ret = mkdir(dstPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            closedir(dir);
            HNP_LOGE("mkdir [%{public}s] unsuccess, ret=%{public}d, errno:%{public}d", dstPath, ret, errno);
            return HNP_ERRNO_BASE_MKDIR_PATH_FAILED;
        }
    }

    while (((entry = readdir(dir)) != NULL)) {
        /* 非二进制文件跳过 */
        if (entry->d_type != DT_REG) {
            continue;
        }
        ret = sprintf_s(srcFile, MAX_FILE_PATH_LEN, "%s/%s", srcPath, entry->d_name);
        if (ret < 0) {
            closedir(dir);
            HNP_LOGE("sprintf install bin src file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        ret = sprintf_s(dstFile, MAX_FILE_PATH_LEN, "%s/%s", dstPath, entry->d_name);
        if (ret < 0) {
            closedir(dir);
            HNP_LOGE("sprintf install bin dst file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        /* 生成软链接 */
        ret = HnpSymlink(srcFile, dstFile, hnpCfg, canRecovery, isPublic);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
    }

    closedir(dir);
    return 0;
}

static int HnpGenerateSoftLink(HnpInstallInfo *hnpInfo, HnpCfgInfo *hnpCfg)
{
    HNP_ERROR_CHECK(hnpInfo != NULL, return HNP_ERRNO_BASE_PARAMS_INVALID,
        "invalid hnpInfp");
    bool canRecovery = CanRecovery(hnpInfo->hapInstallInfo->hapPackageName, hnpCfg);
    int ret = 0;
    char binPath[MAX_FILE_PATH_LEN];

    ret = sprintf_s(binPath, MAX_FILE_PATH_LEN, "%s/bin", hnpInfo->hnpBasePath);
    HNP_ERROR_CHECK(ret > 0, return HNP_ERRNO_BASE_SPRINTF_FAILED,
        "sprintf install bin path unsuccess.");

    if (hnpCfg->linkNum == 0) {
        ret = HnpGenerateSoftLinkAll(hnpInfo->hnpVersionPath, binPath, hnpCfg,
            canRecovery, hnpInfo->isPublic);
    } else {
        ret = HnpGenerateSoftLinkAllByJson(hnpInfo->hnpVersionPath, binPath, hnpCfg,
            canRecovery, hnpInfo->isPublic);
    }

    return ret;
}

/*
* 判断当前hnp是否在main函数入参installInfo->independentSignHnpPaths中
* 如果hnpFile与installInfo->independentSignHnpPaths中某个文件的realpath一致
* 则认为该hnp为独立签名hnp 此时返回true
*/
static bool CheckIsSignHnp(const char *hnpFile, HapInstallInfo *installInfo)
{
    HNP_ONLY_EXPER(hnpFile == NULL || installInfo == NULL || installInfo->signHnpPaths == NULL ||
        installInfo->signHnpSize <= 0, return false);

    bool isSign = false;
    for (int i = 0; i < installInfo->signHnpSize; i++) {
        HNP_ONLY_EXPER(installInfo->signHnpPaths[i] == NULL, continue);

        char hnpReal[PATH_MAX] = {0};
        HNP_INFO_CHECK(realpath(hnpFile, hnpReal) != NULL, continue,
            "file %{public}s not exist", hnpFile);
        HNP_ONLY_EXPER(strcmp(installInfo->signHnpPaths[i], hnpReal) == 0, isSign = true;
            break);
    }
    HNP_LOGI("hnp %{public}s isSign %{public}d", hnpFile, isSign);
    return isSign;
}

static int HnpInstall(const char *hnpFile, HnpInstallInfo *hnpInfo, HnpCfgInfo *hnpCfg,
    HnpSignMapInfo *hnpSignMapInfos, int *count)
{
    int ret;
    int currentIndex = *count;
    /* 解压hnp文件 */
    ret = HnpUnZip(hnpFile, hnpInfo->hnpVersionPath, hnpInfo->hnpSignKeyPrefix, hnpSignMapInfos, count);
    if (ret != 0) {
        return ret; /* 内部已打印日志 */
    }
    // 判断当前hnp是否独立签名并填充本次解压产生的二进制文件数据
    bool isSign = CheckIsSignHnp(hnpFile, hnpInfo->hapInstallInfo);
    for (int i = currentIndex; i < *count; i++) {
        hnpSignMapInfos[i].independentSign = isSign;
        hnpSignMapInfos[i].hnpType = hnpInfo->isPublic;
    }

    /* 生成软链 */
    return HnpGenerateSoftLink(hnpInfo, hnpCfg);
}

/**
 * 删除 ../hnppublic/bin目录下所有失效的hnp
 */
APPSPAWN_STATIC void ClearSoftLink(int uid)
{
    char path[MAX_FILE_PATH_LEN] = {0};
    ssize_t bytes = snprintf_s(path, MAX_FILE_PATH_LEN, MAX_FILE_PATH_LEN - 1,
        HNP_DEFAULT_INSTALL_ROOT_PATH"/%d/hnppublic/bin", uid);
    HNP_ERROR_CHECK(bytes > 0, return,
        "Build bin path failed %{public}d %{public}zd", uid, bytes);

    DIR *dir = opendir(path);
    HNP_ERROR_CHECK(dir != NULL, return,
        "open bin path failed %{public}s %{public}d", path, errno);

    struct dirent *entry;
    int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        count++;
        HNP_INFO_CHECK(entry->d_type == DT_LNK, continue,
            "not lnk file %s skip to next", entry->d_name);

        char sourcePath[MAX_FILE_PATH_LEN] = {0};
        bytes = snprintf_s(sourcePath, MAX_FILE_PATH_LEN, MAX_FILE_PATH_LEN - 1,
            "%s/%s", path, entry->d_name);
        HNP_ERROR_CHECK(bytes > 0, continue,
            "build soft link path failed %{public}s", entry->d_name);

        char targetPath[MAX_FILE_PATH_LEN] = {0};
        bytes = readlink(sourcePath, targetPath, MAX_FILE_PATH_LEN);
        HNP_ERROR_CHECK(bytes > 0, continue, "readlink failed %{public}s", sourcePath);

        char linkPath[MAX_FILE_PATH_LEN] = {0};
        bytes = snprintf_s(linkPath, MAX_FILE_PATH_LEN, MAX_FILE_PATH_LEN - 1,
            "%s/%s", path, targetPath);
        HNP_ERROR_CHECK(bytes > 0, continue, "build link file source path failed %{public}s", targetPath);

        if (access(linkPath, F_OK) != 0) {
            int ret = unlink(sourcePath);
            HNP_LOGI("unlink file %{public}s %{public}d", sourcePath, ret);
        }
    }
    HNP_LOGI("file count is %d", count);
    closedir(dir);
}

/**
 * 卸载公共hnp.
 *
 * @param packageName hap名称.
 * @param name  hnp名称
 * @param version 版本号.
 * @param uid 用户id.
 * @param isInstallVersion 是否卸载安装版本.
 *
 * @return 0:success;other means failure.
 */
static int HnpUnInstallPublicHnp(const char* packageName, const char *name, const char *version, int uid,
    bool isInstallVersion)
{
    int ret;
    char hnpNamePath[MAX_FILE_PATH_LEN];
    char hnpVersionPath[MAX_FILE_PATH_LEN];
    char sandboxPath[MAX_FILE_PATH_LEN];

    if (sprintf_s(hnpNamePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d/hnppublic/%s.org", uid, name) < 0) {
        HNP_LOGE("hnp uninstall name path sprintf unsuccess,uid:%{public}d,name:%{public}s", uid, name);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if (sprintf_s(hnpVersionPath, MAX_FILE_PATH_LEN, "%s/%s_%s", hnpNamePath, name, version) < 0) {
        HNP_LOGE("hnp uninstall sprintf version path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if (sprintf_s(sandboxPath, MAX_FILE_PATH_LEN, HNP_SANDBOX_BASE_PATH"/%s.org", name) < 0) {
        HNP_LOGE("sprintf unstall base path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    ret = HnpProcessRunCheck(sandboxPath);
    if (ret != 0) {
        return ret;
    }

    if (!isInstallVersion) {
        ret = HnpPackageInfoHnpDelete(packageName, name, version, uid);
        if (ret != 0) {
            return ret;
        }
    }

    ret = HnpDeleteFolder(hnpVersionPath);
    if (ret != 0) {
        return ret;
    }

    if (HnpPathFileCount(hnpNamePath) == 0) {
        ClearSoftLink(uid);
        return HnpDeleteFolder(hnpNamePath);
    }

    return 0;
}

static int HnpNativeUnInstall(HnpPackageInfo *packageInfo, int uid, const char *packageName)
{
    int ret = 0;

    HNP_LOGI("hnp uninstall start now! name=%{public}s,version=[%{public}s,%{public}s],uid=%{public}d,"
        "package=%{public}s", packageInfo->name, packageInfo->currentVersion, packageInfo->installVersion, uid,
        packageName);

    if (!packageInfo->hnpExist) {
        ret = HnpUnInstallPublicHnp(packageName, packageInfo->name, packageInfo->currentVersion, uid, false);
        if (ret != 0) {
            return ret;
        }
    }

    if (strcmp(packageInfo->installVersion, "none") != 0 &&
        strcmp(packageInfo->currentVersion, packageInfo->installVersion) != 0) {
        ret = HnpUnInstallPublicHnp(packageName, packageInfo->name, packageInfo->installVersion, uid, true);
        if (ret != 0) {
            return ret;
        }
    }
    HNP_LOGI("hnp uninstall end! ret=%{public}d", ret);

    return 0;
}

/**
* 卸载bss信息，so不存在时返回0，其他情况按照执行结果返回
*/
static int BssUninstall(int uid, const char *packageName)
{
#ifndef APPSPAWN_TEST
    HNP_INFO_CHECK(access(BIN_SEC_PATH, F_OK) == 0, return 0,
        "bin file not exist %{public}d ignore", errno);

    void *handle = dlopen(BIN_SEC_PATH, RTLD_NOW | RTLD_LOCAL);
    HNP_ERROR_CHECK(handle != NULL, return HNP_ERRNO_DL_FAILED,
        "Failed to dlopen errno:%{public}s", dlerror());
    
    ProcessHnpUninstall bssUninstall = (ProcessHnpUninstall)dlsym(handle, "ProcessHnpUninstall");
    HNP_ERROR_CHECK(bssUninstall != NULL, dlclose(handle);
        return HNP_ERRNO_BSS_ERROR, "ProcessHnpUninstall not found errno:%{public}s", dlerror());

    BssString bundleName = {
        .str = (char*)packageName,
        .len = strlen(packageName)
    };
    int32_t ret = bssUninstall(bundleName, uid);
    
    dlclose(handle);
    HNP_ERROR_CHECK(ret == 0, return HNP_ERRNO_BSS_ERROR,
        "exec bss uninstall failed %{public}d", ret);

    return ret;
#else
    return 0;
#endif
}

APPSPAWN_STATIC int RebuildHnpInfoCfg(int uid)
{
    char newCfg[PATH_MAX] = {0};
    int ret = snprintf_s(newCfg, PATH_MAX, PATH_MAX - 1,
        HNP_PACKAGE_INFO_JSON_FILE_PATH, uid);
    HNP_ERROR_CHECK(ret > 0, return HNP_ERRNO_BASE_SPRINTF_FAILED,
        "build cfg Path failed");
    HNP_INFO_CHECK(access(newCfg, F_OK) != 0, return 0,
        "already exist cfg ignore");
    
    HNP_INFO_CHECK(access(HNP_OLD_CFG_PATH, F_OK) == 0, return 0,
        "old cfg not exist can't rebuild cfg");
    return DoRebuildHnpInfoCfg(uid);
}

static int HnpUnInstall(int uid, const char *packageName)
{
    int ret = RebuildHnpInfoCfg(uid);
    HNP_LOGI("rebuild cfg with ret %{public}d", ret);
    HnpPackageInfo *packageInfo = NULL;
    int count = 0;
    char privatePath[MAX_FILE_PATH_LEN];
    char dstPath[MAX_FILE_PATH_LEN];

    /* 拼接卸载路径 */
    if (sprintf_s(dstPath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d", uid) < 0) {
        HNP_LOGE("hnp install sprintf unsuccess, uid:%{public}d", uid);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 验证卸载路径是否存在 */
    if (access(dstPath, F_OK) != 0) {
        HNP_LOGE("hnp uninstall uid path[%{public}s] is not exist", dstPath);
        return HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST;
    }

    ret = HnpPackageInfoGet(packageName, &packageInfo, &count, uid);
    if (ret != 0) {
        return ret;
    }

    /* 卸载公有native */
    for (int i = 0; i < count; i++) {
        ret = HnpNativeUnInstall(&packageInfo[i], uid, packageName);
        if (ret != 0) {
            free(packageInfo);
            return ret;
        }
    }
    free(packageInfo);

    ret = HnpPackageInfoDelete(packageName, uid);
    if (ret != 0) {
        return ret;
    }

    if (sprintf_s(privatePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d/hnp/%s", uid, packageName) < 0) {
        HNP_LOGE("hnp uninstall private path sprintf unsuccess, uid:%{public}d,package name[%{public}s]", uid,
            packageName);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    (void)HnpDeleteFolder(privatePath);

    return BssUninstall(uid, packageName);
}

static int HnpInstallForceCheck(HnpCfgInfo *hnpCfgInfo, HnpInstallInfo *hnpInfo)
{
    int ret = 0;

    /* 判断安装目录是否存在，存在判断是否是强制安装，如果是则走卸载流程，否则返回错误 */
    if (access(hnpInfo->hnpSoftwarePath, F_OK) == 0) {
        if (hnpInfo->hapInstallInfo->isForce == false) {
            HNP_LOGE("hnp install path[%{public}s] exist, but force is false", hnpInfo->hnpSoftwarePath);
            return HNP_ERRNO_INSTALLER_PATH_IS_EXIST;
        }
        if (hnpInfo->isPublic == false) {
            if (HnpDeleteFolder(hnpInfo->hnpSoftwarePath) != 0) {
                return ret;
            }
        }
    }

    ret = HnpCreateFolder(hnpInfo->hnpVersionPath);
    if (ret != 0) {
        return HnpDeleteFolder(hnpInfo->hnpVersionPath);
    }
    return ret;
}

static int HnpInstallPathGet(HnpCfgInfo *hnpCfgInfo, HnpInstallInfo *hnpInfo)
{
    int ret;

    /* 拼接安装路径 */
    ret = sprintf_s(hnpInfo->hnpSoftwarePath, MAX_FILE_PATH_LEN, "%s/%s.org", hnpInfo->hnpBasePath,
        hnpCfgInfo->name);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf hnp base path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 拼接安装路径 */
    ret = sprintf_s(hnpInfo->hnpVersionPath, MAX_FILE_PATH_LEN, "%s/%s_%s", hnpInfo->hnpSoftwarePath,
        hnpCfgInfo->name, hnpCfgInfo->version);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf install path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    if (strstr(hnpInfo->hnpVersionPath, "..")) {
        HNP_LOGE("hnp version path[%{public}s], does not allow the use of ..", hnpInfo->hnpVersionPath);
        return HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED;
    }

    return 0;
}

static int HnpPublicDealAfterInstall(HnpInstallInfo *hnpInfo, HnpCfgInfo *hnpCfg)
{
    char *version = HnpCurrentVersionUninstallCheck(hnpCfg->name, hnpInfo->hapInstallInfo->uid);
    if (version == NULL) {
        version = HnpCurrentVersionGet(hnpCfg->name, hnpInfo->hapInstallInfo->uid);
        if (version != NULL) {
            HnpUnInstallPublicHnp(hnpInfo->hapInstallInfo->hapPackageName, hnpCfg->name, version,
                hnpInfo->hapInstallInfo->uid, true);
        }
    }
    if (version != NULL) {
        free(version);
        version = NULL;
    }
    hnpCfg->isInstall = true;
    return HnpInstallInfoJsonWrite(hnpInfo->hapInstallInfo->hapPackageName, hnpCfg);
}

static int HnpReadAndInstall(char *srcFile, HnpInstallInfo *hnpInfo, HnpSignMapInfo *hnpSignMapInfos, int *count)
{
    int ret;
    HnpCfgInfo hnpCfg = {0};
    hnpCfg.uid = hnpInfo->hapInstallInfo->uid;
    HNP_LOGI("hnp install start now! src file=%{public}s, dst path=%{public}s", srcFile, hnpInfo->hnpBasePath);
    /* 从hnp zip获取cfg信息 */
    ret = HnpCfgGetFromZip(srcFile, &hnpCfg);
    if (ret != 0) {
        return ret;
    }

    ret = HnpInstallPathGet(&hnpCfg, hnpInfo);
    if (ret != 0) {
        // 释放软链接占用的内存
        if (hnpCfg.links != NULL) {
            free(hnpCfg.links);
        }
        return ret;
    }

    /* 存在对应版本的公有hnp包跳过安装 */
    if (access(hnpInfo->hnpVersionPath, F_OK) == 0 && hnpInfo->isPublic) {
        /* 刷新软链 */
        ret = HnpGenerateSoftLink(hnpInfo, &hnpCfg);

        // 释放软链接占用的内存
        HNP_ONLY_EXPER(hnpCfg.links != NULL, free(hnpCfg.links));
        HNP_ONLY_EXPER(ret != 0, return ret);

        return HnpPublicDealAfterInstall(hnpInfo, &hnpCfg);
    }

    ret = HnpInstallForceCheck(&hnpCfg, hnpInfo);
    if (ret != 0) {
        // 释放软链接占用的内存
        if (hnpCfg.links != NULL) {
            free(hnpCfg.links);
        }
        return ret;
    }

    /* hnp安装 */
    ret = HnpInstall(srcFile, hnpInfo, &hnpCfg, hnpSignMapInfos, count);
    // 释放软链接占用的内存
    if (hnpCfg.links != NULL) {
        free(hnpCfg.links);
    }
    if (ret != 0) {
        HnpUnInstallPublicHnp(hnpInfo->hapInstallInfo->hapPackageName, hnpCfg.name, hnpCfg.version,
            hnpInfo->hapInstallInfo->uid, false);
        return ret;
    }

    if (hnpInfo->isPublic) {
        ret = HnpPublicDealAfterInstall(hnpInfo, &hnpCfg);
        if (ret != 0) {
            HnpUnInstallPublicHnp(hnpInfo->hapInstallInfo->hapPackageName, hnpCfg.name, hnpCfg.version,
                hnpInfo->hapInstallInfo->uid, false);
        }
    }

    return ret;
}

static bool HnpFileCheck(const char *file)
{
    const char suffix[] = ".hnp";
    int len = strlen(file);
    int suffixLen = strlen(suffix);
    if ((len >= suffixLen) && (strcmp(file + len - suffixLen, suffix) == 0)) {
        return true;
    }

    return false;
}

static int HnpPackageGetAndInstall(const char *dirPath, HnpInstallInfo *hnpInfo, char *sunDir,
    HnpSignMapInfo *hnpSignMapInfos, int *count)
{
    DIR *dir;
    struct dirent *entry;
    char path[MAX_FILE_PATH_LEN];
    char sunDirNew[MAX_FILE_PATH_LEN];

    if ((dir = opendir(dirPath)) == NULL) {
        HNP_LOGE("hnp install opendir:%{public}s unsuccess, errno=%{public}d", dirPath, errno);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        if (sprintf_s(path, MAX_FILE_PATH_LEN, "%s/%s", dirPath, entry->d_name) < 0) {
            HNP_LOGE("hnp install sprintf unsuccess, dir[%{public}s], path[%{public}s]", dirPath, entry->d_name);
            closedir(dir);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        if (entry->d_type == DT_DIR) {
            if (sprintf_s(sunDirNew, MAX_FILE_PATH_LEN, "%s%s/", sunDir, entry->d_name) < 0) {
                HNP_LOGE("hnp install sprintf sub dir unsuccess");
                closedir(dir);
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
            int ret = HnpPackageGetAndInstall(path, hnpInfo, sunDirNew, hnpSignMapInfos, count);
            if (ret != 0) {
                closedir(dir);
                return ret;
            }
        } else {
            if (HnpFileCheck(path) == false) {
                continue;
            }
            if (sprintf_s(hnpInfo->hnpSignKeyPrefix, MAX_FILE_PATH_LEN, "hnp/%s/%s%s", hnpInfo->hapInstallInfo->abi,
                sunDir, entry->d_name) < 0) {
                HNP_LOGE("hnp install sprintf unsuccess,sub[%{public}s],path[%{public}s]", sunDir, entry->d_name);
                closedir(dir);
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
            int ret = HnpReadAndInstall(path, hnpInfo, hnpSignMapInfos, count);
            HNP_LOGI("hnp install end, ret=%{public}d", ret);
            if (ret != 0) {
                closedir(dir);
                return ret;
            }
        }
    }
    closedir(dir);
    return 0;
}

static int HapReadAndInstall(const char *dstPath, HapInstallInfo *installInfo, HnpSignMapInfo *hnpSignMapInfos,
    int *count)
{
    struct dirent *entry;
    char hnpPath[MAX_FILE_PATH_LEN];
    HnpInstallInfo hnpInfo = {0};
    int ret;

    DIR *dir = opendir(installInfo->hnpRootPath);
    if (dir == NULL) {
        HNP_LOGE("hnp install opendir:%{public}s unsuccess, errno=%{public}d", installInfo->hnpRootPath, errno);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    hnpInfo.hapInstallInfo = installInfo;
    /* 遍历src目录 */
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, "public") == 0) {
            hnpInfo.isPublic = true;
            if ((sprintf_s(hnpInfo.hnpBasePath, MAX_FILE_PATH_LEN, "%s/hnppublic", dstPath) < 0) ||
                (sprintf_s(hnpPath, MAX_FILE_PATH_LEN, "%s/public", installInfo->hnpRootPath) < 0)) {
                HNP_LOGE("hnp install public base path sprintf unsuccess.");
                closedir(dir);
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
        } else if (strcmp(entry->d_name, "private") == 0) {
            hnpInfo.isPublic = false;
            if ((sprintf_s(hnpInfo.hnpBasePath, MAX_FILE_PATH_LEN, "%s/hnp/%s", dstPath,
                installInfo->hapPackageName) < 0) || (sprintf_s(hnpPath, MAX_FILE_PATH_LEN, "%s/private",
                installInfo->hnpRootPath) < 0)) {
                HNP_LOGE("hnp install private base path sprintf unsuccess.");
                closedir(dir);
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
        } else {
            continue;
        }

        ret = HnpPackageGetAndInstall(hnpPath, &hnpInfo, "", hnpSignMapInfos, count);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
    }

    closedir(dir);
    return 0;
}

static int HnpInstallHnpFileCountGet(char *hnpPath, int *count)
{
    DIR *dir;
    struct dirent *entry;
    char path[MAX_FILE_PATH_LEN];
    int ret;

    if ((dir = opendir(hnpPath)) == NULL) {
        HNP_LOGE("hnp install count get opendir:%{public}s unsuccess, errno=%{public}d", hnpPath, errno);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }

        if (sprintf_s(path, MAX_FILE_PATH_LEN, "%s/%s", hnpPath, entry->d_name) < 0) {
            HNP_LOGE("hnp install count get sprintf unsuccess, dir[%{public}s], path[%{public}s]", hnpPath,
                entry->d_name);
            closedir(dir);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        if (entry->d_type == DT_DIR) {
            if (sprintf_s(path, MAX_FILE_PATH_LEN, "%s/%s", hnpPath, entry->d_name) < 0) {
                HNP_LOGE("hnp install sprintf sub dir unsuccess");
                closedir(dir);
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
            ret = HnpInstallHnpFileCountGet(path, count);
            if (ret != 0) {
                closedir(dir);
                return ret;
            }
        } else {
            if (HnpFileCheck(path) == false) {
                continue;
            }
            ret = HnpFileCountGet(path, count);
            if (ret != 0) {
                closedir(dir);
                return ret;
            }
        }
    }

    closedir(dir);
    return 0;
}

static int HnpInstallHapFileCountGet(const char *root, int *count)
{
    struct dirent *entry;
    char hnpPath[MAX_FILE_PATH_LEN];
    char realPath[MAX_FILE_PATH_LEN] = {0};

    if ((realpath(root, realPath) == NULL) || (strnlen(realPath, MAX_FILE_PATH_LEN) >= MAX_FILE_PATH_LEN)) {
        HNP_LOGE("hnp root path:%{public}s invalid", root);
        return HNP_ERRNO_BASE_PARAMS_INVALID;
    }

    DIR *dir = opendir(realPath);
    if (dir == NULL) {
        HNP_LOGE("hnp install opendir:%{public}s unsuccess, errno=%{public}d", realPath, errno);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(entry->d_name, "public") != 0) && (strcmp(entry->d_name, "private") != 0)) {
            continue;
        }
        if (sprintf_s(hnpPath, MAX_FILE_PATH_LEN, "%s/%s", realPath, entry->d_name) < 0) {
            HNP_LOGE("hnp install private base path sprintf unsuccess.");
            closedir(dir);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        int ret = HnpInstallHnpFileCountGet(hnpPath, count);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
    }

    closedir(dir);
    return 0;
}

static int SetHnpRestorecon(char *path)
{
    int ret;
    char publicPath[MAX_FILE_PATH_LEN] = {0};
    if (sprintf_s(publicPath, MAX_FILE_PATH_LEN, "%s/hnppublic", path) < 0) {
        HNP_LOGE("sprintf fail, get hnp restorecon path fail");
        return HNP_ERRNO_INSTALLER_RESTORECON_HNP_PATH_FAIL;
    }

    if (access(publicPath, F_OK) != 0) {
        ret = mkdir(publicPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            HNP_LOGE("mkdir public path fail");
            return HNP_ERRNO_BASE_MKDIR_PATH_FAILED;
        }
    }

    if (RestoreconRecurse(publicPath)) {
        HNP_LOGE("restorecon hnp path fail");
        return HNP_ERRNO_INSTALLER_RESTORECON_HNP_PATH_FAIL;
    }

    // 重置hnp目录权限标签
    char privatePath[MAX_FILE_PATH_LEN] = {0};
    ret = sprintf_s(privatePath, MAX_FILE_PATH_LEN, "%s/hnp", path);
    HNP_ERROR_CHECK(ret > 0, return HNP_ERRNO_INSTALLER_RESTORECON_HNP_PATH_FAIL,
        "sprintf fail, get hnp restorecon path fail %{public}d", ret);

    ret = access(privatePath, F_OK);
    HNP_ONLY_EXPER(ret != 0, ret = mkdir(privatePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));

    HNP_ONLY_EXPER((ret != 0) && (errno != EEXIST), HNP_LOGE("mkdir private path fail");
        return HNP_ERRNO_BASE_MKDIR_PATH_FAILED);

    ret = RestoreconRecurse(privatePath);
    HNP_ERROR_CHECK(ret == 0, return HNP_ERRNO_INSTALLER_RESTORECON_HNP_PATH_FAIL,
        "restorecon private hnp path fail %{public}d %{public}d", errno, ret);

    return 0;
}

static int CheckInstallPath(char *dstPath, HapInstallInfo *installInfo)
{
    /* 拼接安装路径 */
    if (sprintf_s(dstPath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d", installInfo->uid) < 0) {
        HNP_LOGE("hnp install sprintf unsuccess, uid:%{public}d", installInfo->uid);
        return HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED;
    }

    /* 验证安装路径是否存在 */
    if (access(dstPath, F_OK) != 0) {
        HNP_LOGE("hnp install uid path[%{public}s] is not exist", dstPath);
        return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
    }

    /* restorecon hnp 安装目录 */
    return SetHnpRestorecon(dstPath);
}

APPSPAWN_STATIC void HapInstallInfoDestory(HapInstallInfo **installInfo)
{
    HNP_ERROR_CHECK(installInfo != NULL && *installInfo != NULL, return,
        "free installinfo with invaliddata");

    HapInstallInfo *info = *installInfo;
    if (info->signHnpPaths != NULL) {
        for (int i = 0; i < info->signHnpSize; i++) {
            free(info->signHnpPaths[i]);
            info->signHnpPaths[i] = NULL;
        }
        free(info->signHnpPaths);
        info->signHnpPaths = NULL;
    }
    free(info);
    *installInfo = NULL;
}

#ifdef CODE_SIGNATURE_ENABLE
static int BssInstall(HnpSignMapInfo *hnpSignMapInfos, int count, HapInstallInfo *installInfo, int execFilesCount)
{
    HNP_INFO_CHECK(access(BIN_SEC_PATH, F_OK) == 0, return 0,
        "bin file not exist ignore install bss %{public}d", errno);

    void *handle = dlopen(BIN_SEC_PATH, RTLD_NOW | RTLD_LOCAL);
    HNP_ERROR_CHECK(handle != NULL, return HNP_ERRNO_DL_FAILED,
        "Failed to dlopen errno:%{public}s", dlerror());
    
    ProcessHnpInstall bssInstall = (ProcessHnpInstall)dlsym(handle, "ProcessHnpInstall");
    HNP_ERROR_CHECK(bssInstall != NULL, dlclose(handle);
        return HNP_ERRNO_BSS_ERROR, "ProcessHnpInstall not found errno:%{public}s", dlerror());

    HnpFileInfo *hnpFiles = (HnpFileInfo *)calloc(1, execFilesCount * sizeof(HnpFileInfo));
    HNP_ERROR_CHECK(hnpFiles != NULL, dlclose(handle);
        return HNP_ERRNO_ALLOC_FAILED, "Failed to get hnpfile mem:%{public}d", errno);
    int ret = 0;
    do {
        int currentHnpIndex = 0;
        // 获取isExec的signInfo, 组装参数
        for (int i = 0; i < count; i++) {
            HNP_ONLY_EXPER(!hnpSignMapInfos[i].isExec, continue);
            HNP_ERROR_CHECK(currentHnpIndex < execFilesCount, ret = HNP_ERRNO_BSS_ERROR;
                break, "TooMany bss file %{public}d %{public}d", currentHnpIndex, execFilesCount);
            hnpFiles[currentHnpIndex].rootPath.str = hnpSignMapInfos[i].value;
            hnpFiles[currentHnpIndex].rootPath.len = strlen(hnpSignMapInfos[i].value);
            hnpFiles[currentHnpIndex].independentSign = hnpSignMapInfos[i].independentSign;
            hnpFiles[currentHnpIndex].hnpType = hnpSignMapInfos[i].hnpType;
            HNP_LOGI("add bss info %{public}s %{public}d %{public}d", hnpFiles[currentHnpIndex].rootPath.str,
                hnpFiles[currentHnpIndex].independentSign, hnpFiles[currentHnpIndex].hnpType);
            currentHnpIndex++;
        }
        HNP_ONLY_EXPER(ret != 0, break);
        HnpFiles files = {
            .files = hnpFiles,
            .len = execFilesCount
        };
        BssString bundleName = {
            .str = installInfo->hapPackageName,
            .len= strlen(installInfo->hapPackageName)
        };
        BssString appIdentifier = {
            .str = installInfo->appIdentifier,
            .len= strlen(installInfo->appIdentifier)
        };
        //调用bss接口
        ret = bssInstall(bundleName, appIdentifier, installInfo->uid, files);
        HNP_LOGI("bss intall finish %{public}d", ret);
    } while (0);
    dlclose(handle);
    free(hnpFiles);
    return ret;
}
#endif

static int HnpInstallPre(HapInstallInfo *installInfo)
{
    char dstPath[MAX_FILE_PATH_LEN];
    int count = 0;
    HnpSignMapInfo *hnpSignMapInfos = NULL;
#ifdef CODE_SIGNATURE_ENABLE
    struct EntryMapEntryData data = {0};
    int i;
#endif
    int ret;
    HNP_ONLY_EXPER((ret = CheckInstallPath(dstPath, installInfo)) != 0 ||
        (ret = HnpInstallHapFileCountGet(installInfo->hnpRootPath, &count)) != 0, return ret);
    if (count > 0) {
        hnpSignMapInfos = (HnpSignMapInfo *)malloc(sizeof(HnpSignMapInfo) * count);
        HNP_ONLY_EXPER(hnpSignMapInfos == NULL, return HNP_ERRNO_NOMEM);
    }
    count = 0;
    ret = HapReadAndInstall(dstPath, installInfo, hnpSignMapInfos, &count);
    HNP_LOGI("sign start [%{public}s],[%{public}s],%{public}d", installInfo->hapPath, installInfo->abi, count);
#ifdef CODE_SIGNATURE_ENABLE
    if ((ret == 0) && (count > 0)) {
        data.entries = malloc(sizeof(struct EntryMapEntry) * count);
        if (data.entries == NULL) {
            free(hnpSignMapInfos);
            return HNP_ERRNO_NOMEM;
        }
        int exeFileCount = 0;
        for (i = 0; i < count; i++) {
            data.entries[i].key = hnpSignMapInfos[i].key;
            data.entries[i].value = hnpSignMapInfos[i].value;
            exeFileCount += (hnpSignMapInfos[i].isExec ? 1 : 0);
        }
        data.count = count;
        ret = EnforceCodeSignForApp(installInfo->hapPath, &data, FILE_ENTRY_ONLY);
        HNP_LOGI("sign end ret=%{public}d,last key[%{public}s],value[%{public}s]", ret, data.entries[i - 1].key,
            data.entries[i - 1].value);
        free(data.entries);
        if (ret != 0) {
            HnpUnInstall(installInfo->uid, installInfo->hapPackageName);
            ret = HNP_ERRNO_INSTALLER_CODE_SIGN_APP_FAILED;
        }
        HNP_ONLY_EXPER(ret == 0 && installInfo->appIdentifier != NULL && exeFileCount > 0,
            ret = BssInstall(hnpSignMapInfos, count, installInfo, exeFileCount));
        HNP_ERROR_CHECK(ret == 0 || ret == HNP_ERRNO_INSTALLER_CODE_SIGN_APP_FAILED,
            HnpUnInstall(installInfo->uid, installInfo->hapPackageName);
            ret = HNP_ERRNO_BSS_ERROR, "bssinstall failed %{public}d", ret);
    }
#endif
    free(hnpSignMapInfos);
    return ret;
}

/**
* 获取-S参数，解析获取realpath并设置到installInfo->signHnps中
*/
static int HandleSignParameter(HapInstallInfo *installInfo, const char *signHnp)
{
    HNP_ERROR_CHECK(signHnp != NULL && strstr(signHnp, "..") == NULL,
        return HNP_ERRNO_PARAM_INVALID, "invalid signHnp param %{public}s", signHnp);
    HNP_ERROR_CHECK(installInfo != NULL && installInfo->hnpRootPath,
        return HNP_ERRNO_PARAM_INVALID, "installinfo or rootpath invalid");

    char **temp = (char **)malloc(sizeof(char *) * (installInfo->signHnpSize + 1));
    HNP_ERROR_CHECK(temp != NULL, return HNP_ERRNO_ALLOC_FAILED,
        "get for signHnp failed %{public}d", errno);
    int ret = 0;
    HNP_ONLY_EXPER(installInfo->signHnpPaths != NULL,
        ret = memcpy_s(temp, sizeof(char *) * (installInfo->signHnpSize + 1),
            installInfo->signHnpPaths, sizeof(char *) * (installInfo->signHnpSize)));
    HNP_ERROR_CHECK(ret == 0, free(temp);
        return HNP_ERRNO_BASE_COPY_FAILED, "copy sign hnps failed");
    HNP_ONLY_EXPER(installInfo->signHnpPaths != NULL, free(installInfo->signHnpPaths));
    installInfo->signHnpPaths = temp;
    installInfo->signHnpPaths[installInfo->signHnpSize] = NULL;
    
    char hnpPath[MAX_FILE_PATH_LEN] = {0};
    int bytes = snprintf_s(hnpPath, MAX_FILE_PATH_LEN, MAX_FILE_PATH_LEN - 1,
        "%s/%s", installInfo->hnpRootPath, signHnp);
    HNP_ERROR_CHECK(bytes > 0, return HNP_ERRNO_BASE_SPRINTF_FAILED,
        "build hnp path filed");
    char realHnpPath[PATH_MAX] = {0};
    char *real = realpath(hnpPath, realHnpPath);
    HNP_ERROR_CHECK(real != NULL, return HNP_ERRNO_PARAM_INVALID,
        "hnp path invalid %{public}s", signHnp);
    char *dupHnpRealPath = strdup(realHnpPath);
    HNP_ERROR_CHECK(dupHnpRealPath != NULL, return HNP_ERRNO_ALLOC_FAILED,
        "copy hnpreal path failed %{public}d", errno);

    HNP_LOGI("get real path success %{public}s", dupHnpRealPath);
    installInfo->signHnpPaths[installInfo->signHnpSize++] = dupHnpRealPath;
    return 0;
}

static int ParseInstallArgs(int argc, char *argv[], HapInstallInfo *installInfo)
{
    int ret;
    int ch;

    optind = 1; // 从头开始遍历参数
    while ((ch = getopt_long(argc, argv, "hu:p:i:s:a:S:I:f", NULL, NULL)) != -1) {
        switch (ch) {
            case 'h' :
                return HNP_ERRNO_OPERATOR_ARGV_MISS;
            case 'u': // 用户id
                ret = HnpInstallerUidGet(optarg, &installInfo->uid);
                if (ret != 0) {
                    HNP_LOGE("hnp install argv uid[%{public}s] invalid", optarg);
                    return ret;
                }
                break;
            case 'p': // app名称
                installInfo->hapPackageName = (char *)optarg;
                break;
            case 'i': // hnp安装目录
                installInfo->hnpRootPath = (char *)optarg;
                break;
            case 's': // hap目录
                installInfo->hapPath = (char *)optarg;
                break;
            case 'a': // 系统abi路径
                installInfo->abi = (char *)optarg;
                break;
            case 'f': // is force
                installInfo->isForce = true;
                break;
            case 'S':
                ret = HandleSignParameter(installInfo, optarg);
                HNP_ERROR_CHECK(ret == 0, return ret, "handle sign param failed %{public}s", optarg);
                break;
            case 'I':
                installInfo->appIdentifier = (char*)optarg;
                break;
            default:
                break;
        }
    }

    if ((installInfo->uid == -1) || (installInfo->hnpRootPath == NULL) || (installInfo->hapPath == NULL) ||
        (installInfo->abi == NULL) || (installInfo->hapPackageName == NULL)) {
        HNP_LOGE("argv params is missing.");
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }

    return 0;
}

/*
* 获取环境变量中的pipe，用于回写执行结果
*/
static int GetRespFd()
{
    char *fdEnv = getenv(HNP_INFO_RET_FD_ENV);
    HNP_ERROR_CHECK(fdEnv != NULL, return -1, "no fd env for resp");

    for (int i = 0; fdEnv[i] != '\0'; i++) {
        HNP_ERROR_CHECK(isdigit(fdEnv[i]), return -1,
            "invalid fdevc info %{public}s", fdEnv);
    }
    long fd = strtol(fdEnv, NULL, HNP_BASE_DEC);
    HNP_ERROR_CHECK(fd >= 0 && fd < INT_MAX, return -1, "fd out of range");
    return (int)fd;
}

int HnpCmdInstall(int argc, char *argv[])
{
    HapInstallInfo *installInfo = (HapInstallInfo *)calloc(1, sizeof(HapInstallInfo));
    HNP_ERROR_CHECK(installInfo != NULL, return HNP_ERRNO_ALLOC_FAILED,
        "get mem for install info failed");

    installInfo->uid = -1;
    installInfo->signHnpPaths = NULL;
    installInfo->signHnpSize = 0;
    // 解析参数并生成安装信息
    int ret = ParseInstallArgs(argc, argv, installInfo);
    if (ret != 0) {
        HapInstallInfoDestory(&installInfo);
        return ret;
    }
    ret = RebuildHnpInfoCfg(installInfo->uid);
    HNP_LOGI("rebuild cfg with ret %{public}d", ret);
    ret = HnpInstallPre(installInfo);
    HapInstallInfoDestory(&installInfo);

    int respFd = GetRespFd();
    HNP_ONLY_EXPER(respFd <= 0, return ret);
    HnpApiResult result = {
        .result = ret,
    };
    HNP_ERROR_CHECK(write(respFd, &result, sizeof(HnpApiResult)) == sizeof(HnpApiResult),
        return HNP_ERRNO_WRITE_ERROR, "write result failed %{public}d", errno);
    return ret;
}

int HnpCmdUnInstall(int argc, char *argv[])
{
    int uid;
    char *uidArg = NULL;
    char *packageName = NULL;
    int ret;
    int ch;

    optind = 1; // 从头开始遍历参数
    while ((ch = getopt_long(argc, argv, "hu:p:", NULL, NULL)) != -1) {
        switch (ch) {
            case 'h' :
                return HNP_ERRNO_OPERATOR_ARGV_MISS;
            case 'u': // uid
                uidArg = optarg;
                ret = HnpInstallerUidGet(uidArg, &uid);
                if (ret != 0) {
                    HNP_LOGE("hnp install arg uid[%{public}s] invalid", uidArg);
                    return ret;
                }
                break;
            case 'p': // hnp package name
                packageName = (char *)optarg;
                break;
            default:
                break;
            }
    }

    if ((uidArg == NULL) || (packageName == NULL)) {
        HNP_LOGE("hnp uninstall params invalid uid[%{public}s], package name[%{public}s]", uidArg, packageName);
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }
    ret = HnpUnInstall(uid, packageName);
    int respFd = GetRespFd();
    HNP_ONLY_EXPER(respFd <= 0, return ret);
    HnpApiResult result = {
        .result = ret,
    };
    HNP_ERROR_CHECK(write(respFd, &result, sizeof(HnpApiResult)) == sizeof(HnpApiResult),
        return HNP_ERRNO_WRITE_ERROR, "write result failed");
    return ret;
}

#ifdef __cplusplus
}
#endif