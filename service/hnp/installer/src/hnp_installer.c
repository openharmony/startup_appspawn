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

#include "hnp_installer.h"

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
    return 0;
}

static int HnpGenerateSoftLinkAllByJson(const char *installPath, const char *dstPath, HnpCfgInfo *hnpCfg)
{
    char srcFile[MAX_FILE_PATH_LEN];
    char dstFile[MAX_FILE_PATH_LEN];
    NativeBinLink *currentLink = hnpCfg->links;
    char *fileNameTmp;

    if (access(dstPath, F_OK) != 0) {
        int ret = mkdir(dstPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            HNP_LOGE("mkdir [%s] unsuccess, ret=%d, errno:%d", dstPath, ret, errno);
            return HNP_ERRNO_BASE_MKDIR_PATH_FAILED;
        }
    }

    for (unsigned int i = 0; i < hnpCfg->linkNum; i++) {
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
        if (ret < 0) {
            HNP_LOGE("sprintf install bin dst file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        /* 生成软链接 */
        ret = HnpSymlink(srcFile, dstFile);
        if (ret != 0) {
            return ret;
        }

        currentLink++;
    }

    return 0;
}

static int HnpGenerateSoftLinkAll(const char *installPath, const char *dstPath)
{
    char srcPath[MAX_FILE_PATH_LEN];
    char srcFile[MAX_FILE_PATH_LEN];
    char dstFile[MAX_FILE_PATH_LEN];
    int ret;
    DIR *dir;
    struct dirent *entry;

    ret = sprintf_s(srcPath, MAX_FILE_PATH_LEN, "%s/bin", installPath);
    if (ret < 0) {
        HNP_LOGE("sprintf install bin path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if ((dir = opendir(srcPath)) == NULL) {
        HNP_LOGI("soft link bin file:%s not exist", srcPath);
        return 0;
    }

    if (access(dstPath, F_OK) != 0) {
        ret = mkdir(dstPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            closedir(dir);
            HNP_LOGE("mkdir [%s] unsuccess, ret=%d, errno:%d", dstPath, ret, errno);
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
        ret = HnpSymlink(srcFile, dstFile);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
    }

    closedir(dir);
    return 0;
}

static int HnpGenerateSoftLink(const char *installPath, const char *hnpBasePath, HnpCfgInfo *hnpCfg)
{
    int ret = 0;
    char binPath[MAX_FILE_PATH_LEN];

    ret = sprintf_s(binPath, MAX_FILE_PATH_LEN, "%s/bin", hnpBasePath);
    if (ret < 0) {
        HNP_LOGE("sprintf install bin path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if (hnpCfg->linkNum == 0) {
        ret = HnpGenerateSoftLinkAll(installPath, binPath);
    } else {
        ret = HnpGenerateSoftLinkAllByJson(installPath, binPath, hnpCfg);
    }

    return ret;
}

static int HnpInstall(const char *hnpFile, NativeHnpPath *hnpDstPath, HnpCfgInfo *hnpCfg)
{
    int ret;

    /* 解压hnp文件 */
    ret = HnpUnZip(hnpFile, hnpDstPath->hnpVersionPath);
    if (ret != 0) {
        return ret; /* 内部已打印日志 */
    }

    /* 生成软链 */
    return HnpGenerateSoftLink(hnpDstPath->hnpVersionPath, hnpDstPath->hnpBasePath, hnpCfg);
}

static int HnpSingleUnInstall(const char *name, const char *version, int uid)
{
    int ret;
    char hnpNamePath[MAX_FILE_PATH_LEN];
    char sandboxPath[MAX_FILE_PATH_LEN];

    if (sprintf_s(hnpNamePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d/hnppublic/%s.org", uid, name) < 0) {
        HNP_LOGE("hnp uninstall name path sprintf unsuccess,uid:%d,name:%s", uid, name);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if (sprintf_s(sandboxPath, MAX_FILE_PATH_LEN, HNP_SANDBOX_BASE_PATH"/%s.org/%s_%s", name, name, version) < 0) {
        HNP_LOGE("sprintf unstall base path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    ret = HnpProcessRunCheck(sandboxPath);
    if (ret != 0) {
        return ret;
    }

    return HnpDeleteFolder(hnpNamePath);
}

static int HnpUnInstall(int uid, const char *packageName)
{
    HnpPackageInfo *packageInfo = NULL;
    int count = 0;
    char privatePath[MAX_FILE_PATH_LEN];

    int ret = HnpPackageInfoGet(packageName, &packageInfo, &count);
    if (ret != 0) {
        return ret;
    }

    /* 卸载公有native */
    for (int i = 0; i < count; i++) {
        HNP_LOGI("hnp uninstall start now! name=%s,version=%s,uid=%d,package=%s", packageInfo[i].name,
            packageInfo[i].version, uid, packageName);
        ret = HnpSingleUnInstall(packageInfo[i].name, packageInfo[i].version, uid);
        HNP_LOGI("hnp uninstall end! ret=%d", ret);
        if (ret != 0) {
            free(packageInfo);
            return ret;
        }
        ret = HnpPackageInfoHnpDelete(packageName, packageInfo[i].name, packageInfo[i].version);
        if (ret != 0) {
            free(packageInfo);
            return ret;
        }
    }
    free(packageInfo);

    ret = HnpPackageInfoDelete(packageName);
    if (ret != 0) {
        return ret;
    }

    if (sprintf_s(privatePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d/hnp/%s", uid, packageName) < 0) {
        HNP_LOGE("hnp uninstall private path sprintf unsuccess, uid:%s,package name[%s]", uid, packageName);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    return HnpDeleteFolder(privatePath);
}

static int HnpInstallPathGet(HnpCfgInfo *hnpCfgInfo, NativeHnpPath *hnpDstPath, bool isPublic, bool isForce)
{
    int ret;

    /* 拼接安装路径 */
    ret = sprintf_s(hnpDstPath->hnpSoftwarePath, MAX_FILE_PATH_LEN, "%s/%s.org", hnpDstPath->hnpBasePath,
        hnpCfgInfo->name);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf hnp base path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 拼接安装路径 */
    ret = sprintf_s(hnpDstPath->hnpVersionPath, MAX_FILE_PATH_LEN, "%s/%s_%s", hnpDstPath->hnpSoftwarePath,
        hnpCfgInfo->name, hnpCfgInfo->version);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf install path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 判断安装目录是否存在，存在判断是否是强制安装，如果是则走卸载流程，否则返回错误 */
    if (access(hnpDstPath->hnpSoftwarePath, F_OK) == 0) {
        if (isForce && isPublic) {
            ret = HnpSingleUnInstall(hnpCfgInfo->name, hnpCfgInfo->version, hnpDstPath->uid);
            if (ret != 0) {
                return ret;
            }
        }
        if (isForce == false) {
            HNP_LOGE("hnp install path[%s] exist, but force is false", hnpDstPath->hnpSoftwarePath);
            return HNP_ERRNO_INSTALLER_PATH_IS_EXIST;
        }
    }

    ret = HnpCreateFolder(hnpDstPath->hnpVersionPath);
    if (ret != 0) {
        return HnpDeleteFolder(hnpDstPath->hnpVersionPath);
    }
    return ret;
}

static int HnpReadAndInstall(char *srcFile, NativeHnpPath *hnpDstPath, bool isPublic, bool isForce)
{
    int ret;
    HnpCfgInfo hnpCfg = {0};

    /* 从hnp zip获取cfg信息 */
    ret = HnpCfgGetFromZip(srcFile, &hnpCfg);
    if (ret != 0) {
        return ret; /* 内部已打印日志 */
    }

    ret = HnpInstallPathGet(&hnpCfg, hnpDstPath, isPublic, isForce);
    if (ret != 0) {
        // 释放软链接占用的内存
        if (hnpCfg.links != NULL) {
            free(hnpCfg.links);
            hnpCfg.links = NULL;
        }
        return ret; /* 内部已打印日志 */
    }

    /* hnp安装 */
    ret = HnpInstall(srcFile, hnpDstPath, &hnpCfg);
    // 释放软链接占用的内存
    if (hnpCfg.links != NULL) {
        free(hnpCfg.links);
        hnpCfg.links = NULL;
    }
    if (ret != 0) {
        return ret;
    }
    if (isPublic) {
        ret = HnpInstallInfoJsonWrite(hnpDstPath, &hnpCfg);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

static bool HnpFileCheck(const char *file)
{
    char suffix[] = ".hnp";
    int len = strlen(file);
    int suffixLen = strlen(suffix);

    if ((len >= suffixLen) && (strcmp(file + len - suffixLen, suffix) == 0)) {
        return true;
    }

    return false;
}

static int HnpPackageGetAndInstall(const char *dirPath, NativeHnpPath *hnpDstPath, bool isPublic, bool isForce)
{
    DIR *dir;
    struct dirent *entry;
    char path[MAX_FILE_PATH_LEN];
    int ret;

    if ((dir = opendir(dirPath)) == NULL) {
        HNP_LOGE("hnp install opendir:%s unsuccess, errno=%d", dirPath, errno);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    while ((entry = readdir(dir)) != NULL) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        if (sprintf_s(path, MAX_FILE_PATH_LEN, "%s/%s", dirPath, entry->d_name) < 0) {
            HNP_LOGE("hnp install sprintf unsuccess, dir[%s], path[%s]", dirPath, entry->d_name);
            closedir(dir);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        if (entry->d_type == DT_DIR) {
            ret = HnpPackageGetAndInstall(path, hnpDstPath, isPublic, isForce);
            if (ret != 0) {
                closedir(dir);
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
        } else {
            if (HnpFileCheck(path) == false) {
                continue;
            }
            HNP_LOGI("hnp install start now! src file=%s, dst path=%s", path, hnpDstPath->hnpBasePath);
            ret = HnpReadAndInstall(path, hnpDstPath, isPublic, isForce);
            HNP_LOGI("hnp install end, ret=%d", ret);
            if (ret != 0) {
                /* 失败卸载当前包 */
                HnpUnInstall(hnpDstPath->uid, hnpDstPath->hnpPackageName);
                closedir(dir);
                return ret;
            }
        }
    }
    closedir(dir);
    return 0;
}

static int HnpInsatllPre(int uid, char *srcPath, char *packageName, bool isForce)
{
    char dstPath[MAX_FILE_PATH_LEN];
    NativeHnpPath hnpDstPath = {packageName, {0}, {0}, {0}, uid};
    struct dirent *entry;
    char hnpPath[MAX_FILE_PATH_LEN];
    int ret;
    bool isPublic = true;

    /* 拼接安装路径 */
    if (sprintf_s(dstPath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d", uid) < 0) {
        HNP_LOGE("hnp install sprintf unsuccess, uid:%d", uid);
        return HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED;
    }

    /* 验证安装路径是否存在 */
    if (access(dstPath, F_OK) != 0) {
        HNP_LOGE("hnp install uid path[%s] is not exist", dstPath);
        return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
    }

    DIR *dir = opendir(srcPath);
    if (dir == NULL) {
        HNP_LOGE("hnp install opendir:%s unsuccess, errno=%d", srcPath, errno);
        return HNP_ERRNO_BASE_DIR_OPEN_FAILED;
    }

    if (isForce) {
        (void)HnpUnInstall(uid, packageName);
    }

    /* 遍历src目录 */
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, "public") == 0) {
            isPublic = true;
            if ((sprintf_s(hnpDstPath.hnpBasePath, MAX_FILE_PATH_LEN, "%s/hnppublic", dstPath) < 0) ||
            (sprintf_s(hnpPath, MAX_FILE_PATH_LEN, "%s/public", srcPath) < 0)) {
                closedir(dir);
                HNP_LOGE("hnp install public base path sprintf unsuccess.");
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
        } else if (strcmp(entry->d_name, "private") == 0) {
            isPublic = false;
            if ((sprintf_s(hnpDstPath.hnpBasePath, MAX_FILE_PATH_LEN, "%s/hnp/%s", dstPath, packageName) < 0) ||
            (sprintf_s(hnpPath, MAX_FILE_PATH_LEN, "%s/private", srcPath) < 0)) {
                closedir(dir);
                HNP_LOGE("hnp install private base path sprintf unsuccess.");
                return HNP_ERRNO_BASE_SPRINTF_FAILED;
            }
        } else {
            continue;
        }

        ret = HnpPackageGetAndInstall(hnpPath, &hnpDstPath, isPublic, isForce);
        if (ret < 0) {
            closedir(dir);
            return ret;
        }

    }
    closedir(dir);
    return 0;
}

int HnpCmdInstall(int argc, char *argv[])
{
    char *srcPath;
    int uid = 0;
    char *uidArg = NULL;
    bool isForce = false;
    char *packageName;
    int ch;

    optind = 1;  // 从头开始遍历参数
    while ((ch = getopt_long(argc, argv, "hu:p:i:f", NULL, NULL)) != -1) {
        switch (ch) {
            case 'h' :
                return HNP_ERRNO_OPERATOR_ARGV_MISS;
            case 'u': //uid
                uidArg = optarg;
                int ret = HnpInstallerUidGet(uidArg, &uid);
                if (ret != 0) {
                    HNP_LOGE("hnp install arg uid[%s] invalid", uidArg);
                    return ret;
                }
                break;
            case 'p': //hnp package name
                packageName = (char *)optarg;
                break;
            case 'i': //hnp source path
                srcPath = (char *)optarg;
                break;
            case 'f': //is force
                isForce = true;
                break;
            default:
                break;
            }
    }

    if ((uidArg == NULL) || (srcPath == NULL) || (packageName == NULL)) {
        HNP_LOGE("hnp install params invalid,uid[%s],hnp src path[%s], package name[%s]", uidArg, srcPath, packageName);
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }

    return HnpInsatllPre(uid, srcPath, packageName, isForce);
}

int HnpCmdUnInstall(int argc, char *argv[])
{
    int uid;
    char *uidArg = NULL;
    char *packageName = NULL;
    int ret;
    int ch;

    optind = 1;  // 从头开始遍历参数
    while ((ch = getopt_long(argc, argv, "hu:p:", NULL, NULL)) != -1) {
        switch (ch) {
            case 'h' :
                return HNP_ERRNO_OPERATOR_ARGV_MISS;
            case 'u': //uid
                uidArg = optarg;
                ret = HnpInstallerUidGet(uidArg, &uid);
                if (ret != 0) {
                    HNP_LOGE("hnp install arg uid[%s] invalid", uidArg);
                    return ret;
                }
                break;
            case 'p': //hnp package name
                packageName = (char *)optarg;
                break;
            default:
                break;
            }
    }

    if ((uidArg == NULL) || (packageName == NULL)) {
        HNP_LOGE("hnp uninstall params invalid uid[%s], package name[%s]", uidArg, packageName);
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }

    return HnpUnInstall(uid, packageName);
}

#ifdef __cplusplus
}
#endif