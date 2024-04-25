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

static int HnpUnInstall(NativeHnpPath *hnpDstPath, const char *uninstallPath, bool runCheck)
{
    int ret;

    HNP_LOGI("hnp uninstall start now! path=%s soft name[%s], run check=%d", hnpDstPath->hnpSoftwarePath,
        hnpDstPath->hnpSoftwareName, runCheck);

    if (runCheck == false) {
        ret = HnpDeleteFolder(hnpDstPath->hnpSoftwarePath);
        HNP_LOGI("hnp uninstall end! ret=%d", ret);
        return ret;
    }

    ret = HnpProcessRunCheck(uninstallPath);
    if (ret != 0) {
        return ret;
    }

    ret = HnpDeleteFolder(hnpDstPath->hnpSoftwarePath);
    HNP_LOGI("hnp uninstall end! ret=%d", ret);
    return ret;
}

static int HnpGetUninstallPath(const char *softwarePath, char *uninstallPath)
{
    DIR *dir;
    struct dirent *entry;
    int ret;

    if ((dir = opendir(softwarePath)) == NULL) {
        HNP_LOGE("get version file opendir:%s unsuccess", softwarePath);
        return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
    }
    while (((entry = readdir(dir)) != NULL)) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        ret = sprintf_s(uninstallPath, MAX_FILE_PATH_LEN, "%s/%s", softwarePath, entry->d_name);
        if (ret < 0) {
            closedir(dir);
            HNP_LOGE("get version file sprintf_s unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        closedir(dir);
        return 0;
    }

    HNP_LOGE("get uninstall path in[%s] unsuccess", softwarePath);
    closedir(dir);
    return HNP_ERRNO_INSTALLER_VERSION_FILE_GET_FAILED;
}

static int HnpInstallPathGet(const char *fileName, bool isForce, char* hnpVersion, NativeHnpPath *hnpDstPath)
{
    int ret;
    char *hnpNameTmp;
    char uninstallPath[MAX_FILE_PATH_LEN];

    /* 裁剪获取文件名使用 */
    ret = strcpy_s(hnpDstPath->hnpSoftwareName, MAX_FILE_PATH_LEN, fileName);
    if (ret != EOK) {
        HNP_LOGE("hnp install program path strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }
    hnpNameTmp = strrchr(hnpDstPath->hnpSoftwareName, '.');
    if (hnpNameTmp) {
        *hnpNameTmp = '\0';
    } else {
        return HNP_ERRNO_INSTALLER_GET_HNP_NAME_FAILED;
    }

    /* 拼接安装路径 */
    ret = sprintf_s(hnpDstPath->hnpSoftwarePath, MAX_FILE_PATH_LEN, "%s/%s.org", hnpDstPath->hnpBasePath,
        hnpDstPath->hnpSoftwareName);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf hnp base path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 拼接安装路径 */
    ret = sprintf_s(hnpDstPath->hnpVersionPath, MAX_FILE_PATH_LEN, "%s/%s_%s", hnpDstPath->hnpSoftwarePath,
        hnpDstPath->hnpSoftwareName, hnpVersion);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf install path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 判断安装目录是否存在，存在判断是否是强制安装，如果是则走卸载流程，否则返回错误 */
    if (access(hnpDstPath->hnpSoftwarePath, F_OK) == 0) {
        if (isForce == false) {
            HNP_LOGE("hnp install path[%s] exist, but force is false", hnpDstPath->hnpSoftwarePath);
            return HNP_ERRNO_INSTALLER_PATH_IS_EXIST;
        } else {
            ret = HnpGetUninstallPath(hnpDstPath->hnpSoftwarePath, uninstallPath);
            if (ret != 0) {
                return ret;
            }
            ret = HnpUnInstall(hnpDstPath, uninstallPath, true);
            if (ret != 0) {
                return ret;
            }
        }
    }

    ret = HnpCreateFolder(hnpDstPath->hnpVersionPath);
    if (ret != 0) {
        return HnpDeleteFolder(hnpDstPath->hnpVersionPath);
    }
    return ret;
}

static int HnpReadAndInstall(char *srcFile, NativeHnpPath *hnpDstPath, bool isForce)
{
    int ret;
    char *fileName;
    HnpCfgInfo hnpCfg = {0};

    /* 从hnp zip获取cfg信息 */
    ret = HnpCfgGetFromZip(srcFile, &hnpCfg);
    if (ret != 0) {
        return ret; /* 内部已打印日志 */
    }

    /* 获取文件名称 */
    fileName = strrchr(srcFile, DIR_SPLIT_SYMBOL);
    if (fileName == NULL) {
        fileName = srcFile;
    } else {
        fileName++;
    }
    ret = HnpInstallPathGet(fileName, isForce, hnpCfg.version, hnpDstPath);
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
        /* 安装失败卸载当前包 */
        HnpUnInstall(hnpDstPath, hnpDstPath->hnpVersionPath, false);
        return ret;
    }

    return 0;
}

static int HnpInsatllPre(int uid, char *softwarePath[], int count, char *installPath, bool isForce)
{
    char srcFile[count][MAX_FILE_PATH_LEN];
    char dstPath[MAX_FILE_PATH_LEN];
    NativeHnpPath hnpDstPath = {0};
    int ret;
    int i;

    /* 验证native软件包路径是否存在 */
    for (i = 0; i < count; i++) {
        if (GetRealPath(softwarePath[i], srcFile[i]) != 0) {
            HNP_LOGE("hnp install path invalid! src path=%s", softwarePath[i]);
            return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
        }
    }

    if (installPath == NULL) {
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

        ret = sprintf_s(hnpDstPath.hnpBasePath, MAX_FILE_PATH_LEN, "%s/hnp/hnppublic", dstPath);
        if (ret < 0) {
            HNP_LOGE("hnp install public base path sprintf unsuccess.");
            return HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED;
        }
    } else {
        if (GetRealPath(installPath, hnpDstPath.hnpBasePath) != 0) {
            HNP_LOGE("hnp install path[%s] is not exist", installPath);
            return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
        }
    }

    for (i = 0; i < count; i++) {
        HNP_LOGI("hnp install start now! src file=%s, dst path=%s, is force=%d", srcFile[i],
            hnpDstPath.hnpBasePath, isForce);
        ret = HnpReadAndInstall(srcFile[i], &hnpDstPath, isForce);
        HNP_LOGI("hnp install src file=%s end, ret=%d", srcFile[i], ret);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

int HnpCmdInstall(int argc, char *argv[])
{
    char *installPath = NULL;
    char *softwarePath[MAX_SOFTWARE_NUM] = {0};
    int count = 0;
    int uid = 0;
    char *uidArg = NULL;
    bool isForce = false;
    int ch;
    char *ptr;

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
            case 'p': //hnp software path
                ptr = strstr(optarg, ".hnp");
                if ((ptr == NULL) || (strcmp(ptr, ".hnp") != 0)) {
                    HNP_LOGE("package name[%s] invalid", optarg);
                    return HNP_ERRNO_INSTALLER_GET_HNP_NAME_FAILED;
                }
                if ((count + 1) >= MAX_SOFTWARE_NUM) {
                    HNP_LOGE("hnp install software must less than %d", MAX_SOFTWARE_NUM);
                    return HNP_ERRNO_INSTALLER_SOFTWARE_NUM_OVERSIZE;
                }
                softwarePath[count++] = (char *)optarg;
                break;
            case 'i': //private package name
                installPath = (char *)optarg;
                break;
            case 'f': //is force
                isForce = true;
                break;
            default:
                break;
            }
    }

    if ((uidArg == NULL) || (count == 0)) {
        HNP_LOGE("hnp install params invalid, uid[%s] hnp software num[%d]", uidArg, count);
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }

    return HnpInsatllPre(uid, softwarePath, count, installPath, isForce);
}

static int HnpUnInstallPre(int uid, const char *hnpName, const char *version, char *uninstallPath)
{
    NativeHnpPath hnpDstPath = {0};

    if (uninstallPath == NULL) {
        /* 拼接基本路径 */
        if (sprintf_s(hnpDstPath.hnpBasePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"/%d/hnp/hnppublic",
            uid) < 0) {
            HNP_LOGE("hnp uninstall base path sprintf unsuccess,uid:%d, process name[%s]", uid, hnpName);
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
    } else {
        if (GetRealPath(uninstallPath, hnpDstPath.hnpBasePath) != 0) {
            HNP_LOGE("hnp uninstall path[%s] is not exist", uninstallPath);
            return HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST;
        }
    }

    if (sprintf_s(hnpDstPath.hnpSoftwarePath, MAX_FILE_PATH_LEN, "%s/%s.org", hnpDstPath.hnpBasePath, hnpName) < 0) {
        HNP_LOGE("hnp uninstall path sprintf unsuccess,base path:%s,process name[%s]", hnpDstPath.hnpBasePath, hnpName);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if (sprintf_s(hnpDstPath.hnpVersionPath, MAX_FILE_PATH_LEN, "%s/%s_%s", hnpDstPath.hnpSoftwarePath, hnpName,
        version) < 0) {
        HNP_LOGE("hnp uninstall  path sprintf unsuccess,software path:%s， process name[%s],version[%s]",
            hnpDstPath.hnpSoftwarePath, hnpName, version);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 校验目标目录是否存在判断是否安装 */
    if (access(hnpDstPath.hnpVersionPath, F_OK) != 0) {
        HNP_LOGE("hnp uninstall path:%s is not exist", hnpDstPath.hnpVersionPath);
        return HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST;
    }

    if (strcpy_s(hnpDstPath.hnpSoftwareName, MAX_FILE_PATH_LEN, hnpName) != EOK) {
        HNP_LOGE("hnp uninstall strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }

    return HnpUnInstall(&hnpDstPath, hnpDstPath.hnpVersionPath, true);
}

int HnpCmdUnInstall(int argc, char *argv[])
{
    char *uninstallPath = NULL;
    int uid;
    char *uidArg = NULL;
    char *hnpName = NULL;
    char *version = NULL;
    int ret;
    int ch;

    optind = 1;  // 从头开始遍历参数
    while ((ch = getopt_long(argc, argv, "hu:n:v:i:", NULL, NULL)) != -1) {
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
            case 'n': //hnp software name
                hnpName = optarg;
                break;
            case 'v': //version
                version = optarg;
                break;
            case 'i': //package name
                uninstallPath = (char *)optarg;
                break;

            default:
                break;
            }
    }

    if ((uidArg == NULL) || (hnpName == NULL) || (version == NULL)) {
        HNP_LOGE("hnp uninstall params invalid uid[%s], hnp software[%s], version[%s]", uidArg, hnpName, version);
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }

    return HnpUnInstallPre(uid, hnpName, version, uninstallPath);
}

#ifdef __cplusplus
}
#endif