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

#include "hnp_installer.h"

#ifdef __cplusplus
extern "C" {
#endif

static int HnpInstallerUidGet(const char *uidIn, unsigned long *uidOut)
{
    int index;
    char *endptr;

    for (index = 0; uidIn[index] != '\0'; index++) {
        if (!isdigit(uidIn[index])) {
            return HNP_ERRNO_INSTALLER_ARGV_UID_INVALID;
        }
    }

    *uidOut = strtoul(uidIn, &endptr, 10); // 转化为10进制
    return 0;
}

static int HnpGenerateSoftLinkAllByJson(const char *installPath, const char *dstPath, NativeHnpHead *hnpHead)
{
    char srcFile[MAX_FILE_PATH_LEN];
    char dstFile[MAX_FILE_PATH_LEN];
    char uninstallFile[MAX_FILE_PATH_LEN];
    NativeBinLink *currentLink = hnpHead->links;
    char *fileName;
    char *fileNameTmp;

    if (access(dstPath, F_OK) != 0) {
        int ret = mkdir(dstPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if ((ret != 0) && (errno != EEXIST)) {
            HNP_LOGE("mkdir [%s] unsuccess, ret=%d, errno:%d", dstPath, ret, errno);
            return HNP_ERRNO_BASE_MKDIR_PATH_FAILED;
        }
    }

    for (unsigned int i = 0; i < hnpHead->linkNum; i++) {
        int ret = sprintf_s(srcFile, MAX_FILE_PATH_LEN, "%s%s", installPath, currentLink->source);
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
        ret = sprintf_s(dstFile, MAX_FILE_PATH_LEN, "%s%s", dstPath, fileName);
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

    int ret = sprintf_s(uninstallFile, MAX_FILE_PATH_LEN, "%s"HNP_UNSTALL_INFO_FILE, installPath);
    if (ret < 0) {
        HNP_LOGE("sprintf install info file unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    /* 向生成的hnp_unstall.txt文件头写入配置信息便于卸载 */
    ret = HnpWriteInfoToFile(uninstallFile, (char*)hnpHead, hnpHead->headLen);

    return ret;
}

static int HnpGenerateSoftLinkAll(const char *installPath, const char *dstPath)
{
    char srcPath[MAX_FILE_PATH_LEN];
    char srcFile[MAX_FILE_PATH_LEN];
    char dstFile[MAX_FILE_PATH_LEN];
    int ret;
    DIR *dir;
    struct dirent *entry;

    ret = sprintf_s(srcPath, MAX_FILE_PATH_LEN, "%sbin/", installPath);
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
            HNP_LOGE("mkdir [%s] unsuccess, ret=%d, errno:%d", dstPath, ret, errno);
            return HNP_ERRNO_BASE_MKDIR_PATH_FAILED;
        }
    }

    while (((entry = readdir(dir)) != NULL)) {
        /* 非二进制文件跳过 */
        if (entry->d_type != DT_REG) {
            continue;
        }
        ret = sprintf_s(srcFile, MAX_FILE_PATH_LEN, "%s%s", srcPath, entry->d_name);
        if (ret < 0) {
            closedir(dir);
            HNP_LOGE("sprintf install bin src file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }

        ret = sprintf_s(dstFile, MAX_FILE_PATH_LEN, "%s%s", dstPath, entry->d_name);
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

static int HnpGenerateSoftLink(const char *installPath, const char *hnpBasePath, NativeHnpHead *hnpHead)
{
    int ret = 0;
    char binPath[MAX_FILE_PATH_LEN];

    ret = sprintf_s(binPath, MAX_FILE_PATH_LEN, "%sbin/", hnpBasePath);
    if (ret < 0) {
        HNP_LOGE("sprintf install bin path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    if (hnpHead->linkNum == 0) {
        ret = HnpGenerateSoftLinkAll(installPath, binPath);
    } else {
        ret = HnpGenerateSoftLinkAllByJson(installPath, binPath, hnpHead);
    }

    return ret;
}

static int HnpInstall(const char *hnpFile, NativeHnpPath *hnpDstPath, NativeHnpHead *hnpHead)
{
    int ret;

    /* 解压hnp文件 */
    ret = HnpUnZip(hnpFile, hnpDstPath->hnpVersionPath);
    if (ret != 0) {
        return ret; /* 内部已打印日志 */
    }

    /* 生成软链 */
    return HnpGenerateSoftLink(hnpDstPath->hnpVersionPath, hnpDstPath->hnpBasePath, hnpHead);
}

static int HnpProcessRunCheckWithFile(const char *file, NativeHnpPath *hnpDstPath, const char *versionPath)
{
    int ret;
    NativeHnpHead *hnpHead;
    NativeBinLink *currentLink;
    char *fileName;

    ret = HnpReadFromZipHead(file, &hnpHead);
    if (ret != 0) {
        return ret; /* 内部已打印日志 */
    }

    currentLink = hnpHead->links;
    for (unsigned int i = 0; i < hnpHead->linkNum; i++) {
        /* 如果target为空则使用源二进制名称 */
        if (strcmp(currentLink->target, "") == 0) {
            fileName = strrchr(currentLink->source, DIR_SPLIT_SYMBOL);
            if (fileName == NULL) {
                fileName = currentLink->source;
            } else {
                fileName++;
            }
            ret = HnpProcessRunCheck(fileName, versionPath);
        } else {
            ret = HnpProcessRunCheck(currentLink->target, versionPath);
        }
        if (ret != 0) {
            free(hnpHead);
            return ret;
        }
        currentLink++;
    }

    free(hnpHead);
    return 0;
}

static int HnpProcessRunCheckWithPath(const char *path, NativeHnpPath *hnpDstPath, const char *versionPath)
{
    DIR *dir;
    struct dirent *entry;
    int ret;

    if ((dir = opendir(path)) == NULL) {
        HNP_LOGE("uninstall run check opendir:%s unsuccess", path);
        return 0; /* 无bin文件继续删除目录 */
    }
    while (((entry = readdir(dir)) != NULL)) {
        /* 非二进制文件跳过 */
        if (entry->d_type != DT_REG) {
            continue;
        }
        /* 查询软件是否正在运行 */
        ret = HnpProcessRunCheck(entry->d_name, versionPath);
        if (ret != 0) {
            closedir(dir);
            return ret;
        }
    }

    closedir(dir);
    return 0;
}

static int HnpUnInstall(NativeHnpPath *hnpDstPath, const char *versionPath, bool runCheck)
{
    int ret;
    char uninstallFile[MAX_FILE_PATH_LEN];
    char binPath[MAX_FILE_PATH_LEN];

    HNP_LOGI("hnp uninstall start now! path=%s program name[%s], run check=%d", hnpDstPath->hnpSoftwarePath,
        hnpDstPath->hnpSoftwareName, runCheck);

    if (runCheck == false) {
        ret = HnpDeleteFolder(hnpDstPath->hnpSoftwarePath);
        HNP_LOGI("hnp uninstall end! ret=%d", ret);
        return ret;
    }

    ret = sprintf_s(uninstallFile, MAX_FILE_PATH_LEN, "%s/"HNP_UNSTALL_INFO_FILE, versionPath);
    if (ret < 0) {
        HNP_LOGE("sprintf uninstall info file unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    if (access(uninstallFile, F_OK) == 0) {
        ret = HnpProcessRunCheckWithFile(uninstallFile, hnpDstPath, versionPath);
    } else {
        ret = sprintf_s(binPath, MAX_FILE_PATH_LEN, "%s/bin", versionPath);
        if (ret < 0) {
            HNP_LOGE("sprintf uninstall info file unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        ret = HnpProcessRunCheckWithPath(binPath, hnpDstPath, versionPath);
    }

    if (ret != 0) {
        return ret;
    }

    ret = HnpDeleteFolder(hnpDstPath->hnpSoftwarePath);
    HNP_LOGI("hnp uninstall end! ret=%d", ret);
    return ret;
}

static int HnpGetVersionPathInProgramPath(const char *programPath, char *versionPath)
{
    DIR *dir;
    struct dirent *entry;
    int ret;

    if ((dir = opendir(programPath)) == NULL) {
        HNP_LOGE("get version file opendir:%s unsuccess", programPath);
        return HNP_ERRNO_BASE_FILE_OPEN_FAILED;
    }
    while (((entry = readdir(dir)) != NULL)) {
        if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0)) {
            continue;
        }
        ret = sprintf_s(versionPath, MAX_FILE_PATH_LEN, "%s/%s", programPath, entry->d_name);
        if (ret < 0) {
            closedir(dir);
            HNP_LOGE("get version file sprintf_s unsuccess.");
            return HNP_ERRNO_BASE_SPRINTF_FAILED;
        }
        HNP_LOGI("get version file:%s success", versionPath);
        closedir(dir);
        return 0;
    }

    closedir(dir);
    return HNP_ERRNO_INSTALLER_VERSION_FILE_GET_FAILED;
}

static int HnpInstallPathGet(const char *fileName, bool isForce, char* hnpVersion, NativeHnpPath *hnpDstPath)
{
    int ret;
    char *hnpNameTmp;
    char versionOldPath[MAX_FILE_PATH_LEN];

    /* 裁剪获取文件名使用 */
    ret = strcpy_s(hnpDstPath->hnpSoftwareName, MAX_FILE_PATH_LEN, fileName);
    if (ret != EOK) {
        HNP_LOGE("hnp install program path strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }
    hnpNameTmp = strrchr(hnpDstPath->hnpSoftwareName, '.');
    if (hnpNameTmp && !strcmp(hnpNameTmp, ".hnp")) {
        *hnpNameTmp = '\0';
    } else {
        return HNP_ERRNO_INSTALLER_GET_HNP_NAME_FAILED;
    }

    /* 拼接安装路径 */
    ret = sprintf_s(hnpDstPath->hnpSoftwarePath, MAX_FILE_PATH_LEN, "%s%s.org/", hnpDstPath->hnpBasePath,
        hnpDstPath->hnpSoftwareName);
    if (ret < 0) {
        HNP_LOGE("hnp install sprintf hnp base path unsuccess.");
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 拼接安装路径 */
    ret = sprintf_s(hnpDstPath->hnpVersionPath, MAX_FILE_PATH_LEN, "%s%s_%s/", hnpDstPath->hnpSoftwarePath,
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
            ret = HnpGetVersionPathInProgramPath(hnpDstPath->hnpSoftwarePath, versionOldPath);
            if (ret != 0) {
                return ret;
            }
            ret = HnpUnInstall(hnpDstPath, versionOldPath, true);
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
    NativeHnpHead *hnpHead;
    int ret;
    char *fileName;

    /* 从hnp头文件获取头文件信息，由于大小不固定，内存在方法内申请，失败需要手动释放 */
    ret = HnpReadFromZipHead(srcFile, &hnpHead);
    if (ret != 0) {
        return ret; /* 内部已打印日志 */
    }

    /* 获取安装路径 */
    fileName = strrchr(srcFile, DIR_SPLIT_SYMBOL);
    if (fileName == NULL) {
        fileName = srcFile;
    } else {
        fileName++;
    }
    ret = HnpInstallPathGet(fileName, isForce, hnpHead->hnpVersion, hnpDstPath);
    if (ret != 0) {
        free(hnpHead);
        return ret; /* 内部已打印日志 */
    }

    /* hnp安装 */
    ret = HnpInstall(srcFile, hnpDstPath, hnpHead);
    free(hnpHead);
    if (ret != 0) {
        /* 安装失败卸载当前包 */
        HnpUnInstall(hnpDstPath, hnpDstPath->hnpVersionPath, false);
        return ret;
    }
    hnpHead = NULL;

    return 0;
}

static int HnpInsatllPre(unsigned long uid, char *softwarePath[], int count, const char *installPath, bool isForce)
{
    char srcFile[count][MAX_FILE_PATH_LEN];
    char dstPath[MAX_FILE_PATH_LEN];
    NativeHnpPath hnpDstPath = {0};
    int ret;
    int i;

    /* 拼接安装路径 */
    if (sprintf_s(dstPath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"%lu/", uid) < 0) {
        HNP_LOGE("hnp install sprintf unsuccess, uid:%lu", uid);
        return HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED;
    }

    /* 验证安装路径是否存在 */
    if (access(dstPath, F_OK) != 0) {
        HNP_LOGE("hnp install uid path[%s] is not exist", dstPath);
        return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
    }

    /* 验证native软件包路径是否存在 */
    for (i = 0; i < count; i++) {
        if (GetRealPath(softwarePath[i], srcFile[i]) != 0) {
            HNP_LOGE("hnp install path invalid! src path=%s", softwarePath[i]);
            return HNP_ERRNO_INSTALLER_GET_REALPATH_FAILED;
        }
    }

    ret = sprintf_s(hnpDstPath.hnpBasePath, MAX_FILE_PATH_LEN, "%shnp/%s/", dstPath, installPath);
    if (ret < 0) {
        HNP_LOGE("hnp install public base path sprintf unsuccess.");
        return HNP_ERRNO_INSTALLER_GET_HNP_PATH_FAILED;
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
    char *installPath = "hnppublic";
    char *softwarePath[MAX_SOFTWARE_NUM] = {0};
    int count = 0;
    unsigned long uid = 0;
    char *uidArg = NULL;
    bool isForce = false;
    int ret;
    int ch;

    optind = 1;  // 从头开始遍历参数
    while ((ch = getopt(argc, argv, "hu:p:if")) != -1) {
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
            case 'p': //hnp software path
                if ((count + 1) >= MAX_SOFTWARE_NUM) {
                    HNP_LOGE("hnp install software must less than %d", MAX_SOFTWARE_NUM);
                    return HNP_ERRNO_INSTALLER_SOFTWARE_NUM_OVERSIZE;
                }
                softwarePath[count++] = (char *)optarg;
                break;
            case 'i': //private package name
                if (optarg == NULL) {
                    break;
                }
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

static int HnpUnInstallPre(unsigned int uid, const char *hnpName, const char *version, const char *packagePath)
{
    NativeHnpPath hnpDstPath = {0};

    if (sprintf_s(hnpDstPath.hnpVersionPath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"%lu/%s/%s.org/%s_%s/",
        uid, packagePath, hnpName, hnpName, version) < 0) {
        HNP_LOGE("hnp uninstall  path sprintf unsuccess, uid:%lu， program name[%s], version[%s]", uid, hnpName,
            version);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }

    /* 校验目标目录是否存在判断是否安装 */
    if (access(hnpDstPath.hnpVersionPath, F_OK) != 0) {
        HNP_LOGE("hnp uninstall path:%s is not exist", hnpDstPath.hnpVersionPath);
        return HNP_ERRNO_UNINSTALLER_HNP_PATH_NOT_EXIST;
    }

    /* 拼接基本路径 */
    if (sprintf_s(hnpDstPath.hnpSoftwarePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"%lu/%s/%s.org/", uid,
        packagePath, hnpName) < 0) {
        HNP_LOGE("hnp uninstall pro path sprintf unsuccess, uid:%lu, program name[%s]", uid, hnpName);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    if (sprintf_s(hnpDstPath.hnpBasePath, MAX_FILE_PATH_LEN, HNP_DEFAULT_INSTALL_ROOT_PATH"%lu/%s/", uid,
        packagePath) < 0) {
        HNP_LOGE("hnp uninstall base path sprintf unsuccess, uid:%lu, program name[%s]", uid, hnpName);
        return HNP_ERRNO_BASE_SPRINTF_FAILED;
    }
    if (strcpy_s(hnpDstPath.hnpSoftwareName, MAX_FILE_PATH_LEN, hnpName) != EOK) {
        HNP_LOGE("hnp uninstall strcpy unsuccess.");
        return HNP_ERRNO_BASE_COPY_FAILED;
    }

    return HnpUnInstall(&hnpDstPath, hnpDstPath.hnpVersionPath, true);
}

int HnpCmdUnInstall(int argc, char *argv[])
{
    char *installPath = "hnppublic";
    unsigned long uid;
    char *uidArg = NULL;
    char *hnpName = NULL;
    char *version = NULL;
    int ret;
    int ch;

    while ((ch = getopt(argc, argv, "hu:n:v:i")) != -1) {
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
                if (optarg == NULL) {
                    break;
                }
                installPath = (char *)optarg;
                break;

            default:
                break;
            }
    }

    if ((uidArg == NULL) || (hnpName == NULL) || (version == NULL)) {
        HNP_LOGE("hnp uninstall params invalid uid[%s], hnp software[%s], version[%s]", uidArg, hnpName, version);
        return HNP_ERRNO_OPERATOR_ARGV_MISS;
    }

    return HnpUnInstallPre(uid, hnpName, version, installPath);
}

#ifdef __cplusplus
}
#endif