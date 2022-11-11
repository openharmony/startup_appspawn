/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include <pthread.h>
#include <cstdarg>
#include <cstdbool>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <csignal>
#include <ctime>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/capability.h>

#include "appspawn_server.h"
#include "beget_ext.h"
#include "securec.h"

HapContextStub::HapContextStub() {}
HapContextStub::~HapContextStub() {}
static int g_testHapDomainSetcontext = 0;
int HapContextStub::HapDomainSetcontext(const std::string &apl, const std::string &packageName)
{
    if (g_testHapDomainSetcontext == 0) {
        return 0;
    } else if (g_testHapDomainSetcontext == 1) {
        sleep(2); // 2 is sleep wait time
    }
    return g_testHapDomainSetcontext;
}
#ifdef __cplusplus
    extern "C" {
#endif
void SetHapDomainSetcontextResult(int result)
{
    g_testHapDomainSetcontext = result;
}

void *DlopenStub(const char *pathname, int mode)
{
    UNUSED(pathname);
    UNUSED(mode);
    static size_t index = 0;
    return &index;
}

bool InitEnvironmentParamStub(const char *name)
{
    UNUSED(name);
    return true;
}

void *DlsymStub(void *handle, const char *symbol)
{
    UNUSED(handle);
    if (strcmp(symbol, "InitEnvironmentParam") == 0) {
        return (void *)InitEnvironmentParamStub;
    }
    return nullptr;
}

int DlcloseStub(void *handle)
{
    UNUSED(handle);
    return 0;
}

pid_t WaitpidStub(pid_t *pid, int *status, int opt)
{
    UNUSED(pid);
    UNUSED(opt);
    static int count = 0;
    static int statusCount = 0;
    *status = (statusCount % 2 == 0) ? 0x0e007f : 0; // 2 is to judge whether it is even
    count++;
    printf("waitpid stub %d\n", GetTestPid());
    if ((count % 2) == 1) { // 2 is to judge whether it is odd
        statusCount++;
        return GetTestPid();
    }
    return -1;
}

void DisallowInternet(void)
{
}

int BindStub(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    UNUSED(sockfd);
    UNUSED(addr);
    UNUSED(addrlen);
    return 0;
}

int ListenStub(int fd, int backlog)
{
    UNUSED(fd);
    UNUSED(backlog);
    return 0;
}

int LchownStub(const char *pathname, uid_t owner, gid_t group)
{
    UNUSED(pathname);
    UNUSED(owner);
    UNUSED(group);
    return 0;
}

int LchmodStub(const char *pathname, mode_t mode)
{
    UNUSED(pathname);
    UNUSED(mode);
    return 0;
}

int GetsockoptStub(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    UNUSED(sockfd);
    UNUSED(level);
    UNUSED(optlen);
    if (optval == nullptr) {
        return -1;
    }
    if (optname == SO_PEERCRED) {
        struct ucred *cred = reinterpret_cast<struct ucred *>(optval);
        cred->uid = 0;
    }
    return 0;
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

struct ForkArgs {
    int (*childFunc)(void *arg);
    void *args;
};

static void *ThreadFunc(void *arg)
{
    struct ForkArgs *forkArg = reinterpret_cast<struct ForkArgs *>(arg);
    forkArg->childFunc(forkArg->args);
    free(forkArg);
    return nullptr;
}

static pid_t g_pid = 1000;
pid_t AppSpawnFork(int (*childFunc)(void *arg), void *args)
{
    static pthread_t thread = 0;
    struct ForkArgs *forkArg = reinterpret_cast<struct ForkArgs *>(malloc(sizeof(struct ForkArgs)));
    if (forkArg == nullptr) {
        return -1;
    }
    printf("ThreadFunc TestFork args %p forkArg %p\n", args, forkArg);
    forkArg->childFunc = childFunc;
    forkArg->args = args;
    int ret = pthread_create(&thread, nullptr, ThreadFunc, forkArg);
    if (ret != 0) {
        printf("Failed to create thread %d \n", errno);
        return -1;
    }
    g_pid++;
    return g_pid;
}

pid_t GetTestPid(void)
{
    return g_pid;
}

int CloneStub(int (*fn)(void *), void *stack, int flags, void *arg, ...)
{
    static int testResult = 0;
    testResult++;
    return testResult == 1 ? AppSpawnFork(fn, arg) : -1;
}

void StartupLog_stub(InitLogLevel logLevel, uint32_t domain, const char *tag, const char *fmt, ...)
{
    char tmpFmt[1024] = {0};
    va_list vargs;
    va_start(vargs, fmt);
    if (vsnprintf_s(tmpFmt, sizeof(tmpFmt), sizeof(tmpFmt) - 1, fmt, vargs) == -1) {
        tmpFmt[sizeof(tmpFmt) - 2] = '\n'; // 2 add \n to tail
        tmpFmt[sizeof(tmpFmt) - 1] = '\0';
    }
    va_end(vargs);

    struct timespec curr;
    (void)clock_gettime(CLOCK_REALTIME, &curr);
    struct tm t;
    char dateTime[80] = {"00-00-00 00:00:00"}; // 80 data time
    if (localtime_r(&curr.tv_sec, &t) != nullptr) {
        strftime(dateTime, sizeof(dateTime), "%Y-%m-%d %H:%M:%S", &t);
    }
    (void)fprintf(stdout, "[%s.%ld][pid=%d %d][%s]%s \n", dateTime, curr.tv_nsec, getpid(), gettid(), tag, tmpFmt);
    (void)fflush(stdout);
}

bool SetSeccompPolicyWithName(const char *filterName)
{
    static int result = 0;
    result++;
    return (result % 3) == 0; // 3 is test data
}

#ifdef __cplusplus
    }
#endif
