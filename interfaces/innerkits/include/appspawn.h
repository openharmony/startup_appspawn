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

#ifndef APPSPAWN_H
#define APPSPAWN_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief appspawn请求消息构造句柄，不支持多线程消息构建
 *
 * 根据业务使用AppSpawnReqMsgCreate/AppSpawnTerminateMsgCreate 构建消息
 * 如果调用AppSpawnClientSendMsg后，消息句柄不需要处理
 * 否则需要调用 AppSpawnReqMsgFree 释放句柄
 *
 * 所有字符串输入的接口，只能接受合法的字符串，输入null、""、和大于合法长度的字符串都返回错误
 *
 */
typedef void *AppSpawnReqMsgHandle;

/**
 * @brief 支持多线程获取句柄，这个是线程安全的。使用时，全局创建一个句柄，支持多线程发送对应线程的消息请求
 *
 */
typedef void *AppSpawnClientHandle;

#define INVALID_PERMISSION_INDEX (-1)
#define INVALID_REQ_HANDLE NULL
#define NWEBSPAWN_SERVER_NAME "nwebspawn"
#define APPSPAWN_SERVER_NAME "appspawn"
#define CJAPPSPAWN_SERVER_NAME "cjappspawn"
#define NWEBSPAWN_RESTART "nwebRestart"
#define NATIVESPAWN_SERVER_NAME "nativespawn"

#pragma pack(4)
#define APP_MAX_GIDS 64
#define APP_USER_NAME 64
#define APP_MAX_FD_COUNT 16
#define APP_FDENV_PREFIX "APPSPAWN_FD_"
#define APP_FDNAME_MAXLEN 20
typedef struct {
    uint32_t uid;       // the UNIX uid that the child process setuid() to after fork()
    uint32_t gid;       // the UNIX gid that the child process setgid() to after fork()
    uint32_t gidCount;  // the size of gidTable
    uint32_t gidTable[APP_MAX_GIDS];
    char userName[APP_USER_NAME];
} AppDacInfo;

typedef struct {
    int result;
    pid_t pid;
} AppSpawnResult;
#pragma pack()

/**
 * @brief init spawn client, eg: nwebspawn、appspawn
 *
 * @param serviceName service name, eg: nwebspawn、appspawn
 * @param handle handle for client
 * @return if succeed return 0,else return other value
 */
int AppSpawnClientInit(const char *serviceName, AppSpawnClientHandle *handle);
/**
 * @brief destroy client
 *
 * @param handle handle for client
 * @return if succeed return 0,else return other value
 */
int AppSpawnClientDestroy(AppSpawnClientHandle handle);

/**
 * @brief send client request
 *
 * @param handle handle for client
 * @param reqHandle handle for request
 * @param result result from appspawn service
 * @return if succeed return 0,else return other value
 */
int AppSpawnClientSendMsg(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle, AppSpawnResult *result);

/**
 * @brief send client user lock status request
 *
 * @param userId user id
 * @param isLocked lock status
 * @return if succeed return 0,else return other value
 */
int AppSpawnClientSendUserLockStatus(uint32_t userId, bool isLocked);

typedef enum {
    MSG_APP_SPAWN = 0,
    MSG_GET_RENDER_TERMINATION_STATUS,
    MSG_SPAWN_NATIVE_PROCESS,
    MSG_DUMP,
    MSG_BEGET_CMD,
    MSG_BEGET_SPAWNTIME,
    MSG_UPDATE_MOUNT_POINTS,
    MSG_RESTART_SPAWNER,
    MSG_DEVICE_DEBUG,
    MSG_UNINSTALL_DEBUG_HAP,
    MSG_LOCK_STATUS,
    MSG_OBSERVE_PROCESS_SIGNAL_STATUS,
    MAX_TYPE_INVALID
} AppSpawnMsgType;

/**
 * @brief create spawn request
 *
 * @param msgType msg type. eg: MSG_APP_SPAWN,MSG_SPAWN_NATIVE_PROCESS
 * @param processName process name, max length is 255
 * @param reqHandle handle for request message
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgCreate(AppSpawnMsgType msgType, const char *processName, AppSpawnReqMsgHandle *reqHandle);

/**
 * @brief create request
 *
 * @param pid process pid
 * @param reqHandle handle for request message
 * @return if succeed return 0,else return other value
 */
int AppSpawnTerminateMsgCreate(pid_t pid, AppSpawnReqMsgHandle *reqHandle);

/**
 * @brief destroy request
 *
 * @param reqHandle handle for request
 */
void AppSpawnReqMsgFree(AppSpawnReqMsgHandle reqHandle);

/**
 * @brief set bundle info
 *
 * @param reqHandle handle for request message
 * @param bundleIndex bundle index
 * @param bundleName bundle name, max length is 255
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgSetBundleInfo(AppSpawnReqMsgHandle reqHandle, uint32_t bundleIndex, const char *bundleName);

/**
 * @brief set app flags info
 *
 * @param reqHandle handle for request message
 * @param flagIndex flags index from AppFlagsIndex
 * @return if succeed return 0,else return other value
 */
typedef enum {
    APP_FLAGS_COLD_BOOT = 0,
    APP_FLAGS_BACKUP_EXTENSION = 1,
    APP_FLAGS_DLP_MANAGER = 2,
    APP_FLAGS_DEBUGGABLE = 3,
    APP_FLAGS_ASANENABLED = 4,
    APP_FLAGS_ACCESS_BUNDLE_DIR = 5,
    APP_FLAGS_NATIVEDEBUG = 6,
    APP_FLAGS_NO_SANDBOX = 7,
    APP_FLAGS_OVERLAY = 8,
    APP_FLAGS_BUNDLE_RESOURCES = 9,
    APP_FLAGS_GWP_ENABLED_FORCE,   // APP_GWP_ENABLED_FORCE 0x400
    APP_FLAGS_GWP_ENABLED_NORMAL,  // APP_GWP_ENABLED_NORMAL 0x800
    APP_FLAGS_TSAN_ENABLED,  // APP_TSANENABLED 0x1000
    APP_FLAGS_IGNORE_SANDBOX = 13,  // ignore sandbox result
    APP_FLAGS_ISOLATED_SANDBOX,
    APP_FLAGS_EXTENSION_SANDBOX,
    APP_FLAGS_CLONE_ENABLE,
    APP_FLAGS_DEVELOPER_MODE = 17,
    APP_FLAGS_BEGETCTL_BOOT, // Start an app from begetctl.
    APP_FLAGS_ATOMIC_SERVICE,
    APP_FLAGS_CHILDPROCESS,
    APP_FLAGS_HWASAN_ENABLED = 21,
    APP_FLAGS_UBSAN_ENABLED = 22,
    APP_FLAGS_ISOLATED_SANDBOX_TYPE,
    APP_FLAGS_ISOLATED_SELINUX_LABEL,
    APP_FLAGS_ISOLATED_SECCOMP_TYPE,
    APP_FLAGS_ISOLATED_NETWORK,
    APP_FLAGS_ISOLATED_DATAGROUP,
    APP_FLAGS_TEMP_JIT = 28,
    APP_FLAGS_PRE_INSTALLED_HAP = 29,
    APP_FLAGS_GET_ALL_PROCESSES = 30,
    MAX_FLAGS_INDEX = 63,
} AppFlagsIndex;

int AppSpawnReqMsgSetAppFlag(AppSpawnReqMsgHandle reqHandle, AppFlagsIndex flagIndex);

/**
 * @brief set dac info
 *
 * @param reqHandle handle for request message
 * @param dacInfo dac info from AppDacInfo
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgSetAppDacInfo(AppSpawnReqMsgHandle reqHandle, const AppDacInfo *dacInfo);

/**
 * @brief set domain info
 *
 * @param reqHandle handle for request message
 * @param hapFlags hap of flags
 * @param apl apl value, max length is 31
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgSetAppDomainInfo(AppSpawnReqMsgHandle reqHandle, uint32_t hapFlags, const char *apl);

/**
 * @brief set internet permission info
 *
 * @param reqHandle handle for request message
 * @param allowInternet
 * @param setAllowInternet
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgSetAppInternetPermissionInfo(AppSpawnReqMsgHandle reqHandle, uint8_t allow, uint8_t setAllow);

/**
 * @brief set access token info
 *
 * @param reqHandle handle for request message
 * @param accessTokenIdEx access tokenId
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgSetAppAccessToken(AppSpawnReqMsgHandle reqHandle, uint64_t accessTokenIdEx);

/**
 * @brief set owner info
 *
 * @param reqHandle handle for request message
 * @param ownerId owner id, max length is 63
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgSetAppOwnerId(AppSpawnReqMsgHandle reqHandle, const char *ownerId);

/**
 * @brief add permission to message
 *
 * @param reqHandle handle for request message
 * @param permission permission name
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgAddPermission(AppSpawnReqMsgHandle reqHandle, const char *permission);

/**
 * @brief add permission to message
 *
 * @param handle handle for client
 * @param reqHandle handle for request message
 * @param permission permission name
 * @return if succeed return 0,else return other value
 */
int AppSpawnClientAddPermission(AppSpawnClientHandle handle, AppSpawnReqMsgHandle reqHandle, const char *permission);

/**
 * @brief add extend info to message
 *
 * @param reqHandle handle for request message
 * @param name extend name, max length is 31
 * @param value extend value, max length is 32768
 * @param valueLen extend value length
 * @return if succeed return 0,else return other value
 */
#define MSG_EXT_NAME_RENDER_CMD "render-cmd"
#define MSG_EXT_NAME_HSP_LIST "HspList"
#define MSG_EXT_NAME_OVERLAY "Overlay"
#define MSG_EXT_NAME_DATA_GROUP "DataGroup"
#define MSG_EXT_NAME_APP_ENV "AppEnv"
#define MSG_EXT_NAME_APP_EXTENSION "AppExtension"
#define MSG_EXT_NAME_BEGET_PID "AppPid"
#define MSG_EXT_NAME_BEGET_PTY_NAME "ptyName"
#define MSG_EXT_NAME_ACCOUNT_ID "AccountId"
#define MSG_EXT_NAME_PROVISION_TYPE "ProvisionType"
#define MSG_EXT_NAME_PROCESS_TYPE "ProcessType"
#define MSG_EXT_NAME_MAX_CHILD_PROCCESS_MAX "MaxChildProcess"
#define MSG_EXT_NAME_APP_FD "AppFd"
#define MSG_EXT_NAME_JIT_PERMISSIONS "JITPermissions"
#define MSG_EXT_NAME_USERID "uid"
#define MSG_EXT_NAME_EXTENSION_TYPE "ExtensionType"

int AppSpawnReqMsgAddExtInfo(AppSpawnReqMsgHandle reqHandle, const char *name, const uint8_t *value, uint32_t valueLen);

/**
 * @brief add extend info to message
 *
 * @param reqHandle handle for request message
 * @param name extend name, max length is 31
 * @param value extend value, max length is 32767
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgAddStringInfo(AppSpawnReqMsgHandle reqHandle, const char *name, const char *value);

/**
 * @brief add fd info to message
 *
 * @param reqHandle handle for request message
 * @param name fd name
 * @param value fd value
 * @return if succeed return 0,else return other value
 */
int AppSpawnReqMsgAddFd(AppSpawnReqMsgHandle reqHandle, const char* fdName, int fd);

/**
 * @brief Get the permission index by permission name
 *
 * @param handle handle for client
 * @param permission permission name
 * @return int32_t permission index, if not exit, return INVALID_PERMISSION_INDEX
 */
int32_t GetPermissionIndex(AppSpawnClientHandle handle, const char *permission);

/**
 * @brief Get the max permission Index
 *
 * @param handle handle for client
 * @return int32_t max permission Index
 */
int32_t GetMaxPermissionIndex(AppSpawnClientHandle handle);

/**
 * @brief Get the permission name by index
 *
 * @param handle handle for client
 * @param index permission index
 * @return const char* permission name
 */
const char *GetPermissionByIndex(AppSpawnClientHandle handle, int32_t index);

/**
 * @brief set up a pipe fd to capture the exit reason of a child process
 *
 * @param fd fd for write signal info
 * @return if succeed return 0,else return other value
 */
int SpawnListenFdSet(int fd);


/**
 * @brief close the listener for child process exit
 *
 * @return if succeed return 0,else return other value
 */
int SpawnListenCloseSet(void);

#ifdef __cplusplus
}
#endif

#endif
