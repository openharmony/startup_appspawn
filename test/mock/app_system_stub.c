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

#include "app_spawn_stub.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/capability.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "appspawn_hook.h"
#include "appspawn_server.h"
#include "appspawn_sandbox.h"
#include "hilog/log.h"
#include "securec.h"

#ifdef __cplusplus
extern "C" {
#endif

StubNode g_stubNodes[] = {
    {STUB_MOUNT, 0, 0, NULL},
    {STUB_EXECV, 0, 0, NULL},
};

StubNode *GetStubNode(int type)
{
    if (type >= (int)(sizeof(g_stubNodes) / sizeof(g_stubNodes[0]))) {
        return NULL;
    }

    return &g_stubNodes[type];
}

void *DlopenStub(const char *pathname, int mode)
{
    UNUSED(pathname);
    UNUSED(mode);
    static size_t index = 0;
    return &index;
}

static bool InitEnvironmentParamStub(const char *name)
{
    UNUSED(name);
    return true;
}

static bool SetRendererSecCompPolicyStub(void)
{
    return true;
}

static void NWebRenderMainStub(const char *cmd)
{
    printf("NWebRenderMainStub cmd %s \n", cmd);
}

uint32_t g_dlsymResultFlags = 0;
#define DLSYM_FAIL_SET_SEC_POLICY 0x01
#define DLSYM_FAIL_NWEB_MAIN 0x02
#define DLSYM_FAIL_INIT_ENV 0x04
void SetDlsymResult(uint32_t flags, bool success)
{
    if (success) {
        g_dlsymResultFlags &= ~flags;
    } else {
        g_dlsymResultFlags |= flags;
    }
}

void *DlsymStub(void *handle, const char *symbol)
{
    printf("DlsymStub %s \n", symbol);
    UNUSED(handle);
    if (strcmp(symbol, "InitEnvironmentParam") == 0) {
        return ((g_dlsymResultFlags & DLSYM_FAIL_INIT_ENV) == 0) ? (void *)(InitEnvironmentParamStub) : NULL;
    }
    if (strcmp(symbol, "SetRendererSeccompPolicy") == 0) {
        return ((g_dlsymResultFlags & DLSYM_FAIL_SET_SEC_POLICY) == 0) ? (void *)(SetRendererSecCompPolicyStub) : NULL;
    }
    if (strcmp(symbol, "NWebRenderMain") == 0) {
        return ((g_dlsymResultFlags & DLSYM_FAIL_NWEB_MAIN) == 0) ? (void *)(NWebRenderMainStub) : NULL;
    }
    return NULL;
}

int DlcloseStub(void *handle)
{
    UNUSED(handle);
    return 0;
}

void DisallowInternet(void)
{
}

bool may_init_gwp_asan(bool forceInit)
{
    return false;
}

int SetgroupsStub(size_t size, const gid_t *list)
{
    UNUSED(size);
    UNUSED(list);
    return 0;
}

int SetresuidStub(uid_t ruid, uid_t euid, uid_t suid)
{
    UNUSED(ruid);
    UNUSED(euid);
    UNUSED(suid);
    return 0;
}

int SetresgidStub(gid_t rgid, gid_t egid, gid_t sgid)
{
    UNUSED(rgid);
    UNUSED(egid);
    UNUSED(sgid);
    return 0;
}

int CapsetStub(cap_user_header_t hdrp, const cap_user_data_t datap)
{
    UNUSED(hdrp);
    UNUSED(datap);
    return 0;
}

int UnshareStub(int flags)
{
    printf("UnshareStub %x \n", flags);
    return 0;
}

int MountStub(const char *originPath, const char *destinationPath,
    const char *fsType, unsigned long mountFlags, const char *options, mode_t mountSharedFlag)
{
    StubNode *node = GetStubNode(STUB_MOUNT);
    if (node == NULL || node->arg == NULL || (node->flags & STUB_NEED_CHECK) != STUB_NEED_CHECK) {
        return 0;
    }
    MountArg *args = (MountArg *)node->arg;

    printf("args->originPath %s == %s \n", args->originPath, originPath);
    printf("args->destinationPath %s == %s \n", args->destinationPath, destinationPath);
    printf("args->fsType %s == %s \n", args->fsType, fsType);
    printf("args->options %s == %s \n", args->options, options);
    printf("mountFlags %lx args->mountFlags %lx \n", mountFlags, args->mountFlags);
    printf("mountSharedFlag 0x%x args->mountSharedFlag 0x%x \n", mountSharedFlag, args->mountSharedFlag);

    if (originPath != NULL && (strcmp(originPath, args->originPath) == 0)) {
        int result = (destinationPath != NULL && (strcmp(destinationPath, args->destinationPath) == 0) &&
            (mountFlags == args->mountFlags) &&
            (args->fsType == NULL || (fsType != NULL && strcmp(fsType, args->fsType) == 0)) &&
            (args->options == NULL || (options != NULL && strcmp(options, args->options) == 0)));
        errno = result ? 0 : -EINVAL;
        node->result = result ? 0 : -EINVAL;
        printf("MountStub result %d node->result %d \n", result, node->result);
        return errno;
    }
    return 0;
}

int SymlinkStub(const char *target, const char *linkName)
{
    return 0;
}

int ChdirStub(const char *path)
{
    return 0;
}

int ChrootStub(const char *path)
{
    return 0;
}

long int SyscallStub(long int type, ...)
{
    return 0;
}

int Umount2Stub(const char *path, int type)
{
    return 0;
}

int UmountStub(const char *path)
{
    return 0;
}

int mallopt(int param, int value)
{
    return 0;
}

int AccessStub(const char *pathName, int mode)
{
    printf(" AccessStub pathName %s \n", pathName);
    if (strstr(pathName, "/data/app/el2/50/base") != NULL) {
        return -1;
    }
    if (strstr(pathName, "/mnt/sandbox/50/com.example.myapplication/data/storage/el2") != NULL) {
        return -1;
    }
    if (strstr(pathName, "/data/app/el5/100/base/com.example.myapplication") != NULL) {
        return -1;
    }
    return 0;
}

int ExecvStub(const char *pathName, char *const argv[])
{
    printf("ExecvStub %s \n", pathName);
    StubNode *node = GetStubNode(STUB_EXECV);
    if (node == NULL || node->arg == NULL || (node->flags & STUB_NEED_CHECK) != STUB_NEED_CHECK) {
        return 0;
    }

    ExecvFunc func = (ExecvFunc)node->arg;
    func(pathName, argv);
    return 0;
}

int ExecvpStub(const char *pathName, char *const argv[])
{
    printf("ExecvpStub %s \n", pathName);
    return 0;
}

int GetprocpidStub()
{
    return 0;
}

int CloneStub(int (*fn)(void *), void *stack, int flags, void *arg, ...)
{
    printf("CloneStub 11 %d \n", getpid());
    pid_t pid = fork();
    if (pid == 0) {
        fn(arg);
        _exit(0x7f); // 0x7f user exit
    }
    return pid;
}

int SetuidStub(uid_t uid)
{
    return 0;
}

int SetgidStub(gid_t gid)
{
    return 0;
}

int IoctlStub(int fd, unsigned long request, ...)
{
    return 0;
}

#ifdef __cplusplus
}
#endif
