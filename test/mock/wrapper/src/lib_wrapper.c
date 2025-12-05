/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "lib_wrapper.h"
#include "securec.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// start wrap execve
static ExecveFunc g_execve = NULL;
void UpdateExecveFunc(ExecveFunc func)
{
    g_execve = func;
}
int __wrap_execve(const char *pathname, char *const argv[], char *const envp[])
{
    if (g_execve) {
        return g_execve(pathname, argv, envp);
    } else {
        return __real_execve(pathname, argv, envp);
    }
}

// start wrap read
static ReadFunc g_read = NULL;
void UpdateReadFunc(ReadFunc func)
{
    g_read = func;
}
ssize_t __wrap_read(int fd, void *buf, size_t count)
{
    if (g_read) {
        return g_read(fd, buf, count);
    } else {
        return __real_read(fd, buf, count);
    }
}

// start wrap fread
static FreadFunc g_fread = NULL;
void UpdateFreadFunc(FreadFunc func)
{
    g_fread = func;
}
size_t __wrap_fread(void *ptr, size_t size, size_t count, FILE *stream)
{
    if (g_fread) {
        return g_fread(ptr, size, count, stream);
    } else {
        return __real_fread(ptr, size, count, stream);
    }
}

// start wrap memcmp
static MemcmpFunc g_memcmp = NULL;
void UpdateMemcmpFunc(MemcmpFunc func)
{
    g_memcmp = func;
}
int __wrap_memcmp(const void *ptr1, const void *ptr2, size_t num)
{
    if (g_memcmp) {
        return g_memcmp(ptr1, ptr2, num);
    } else {
        return __real_memcmp(ptr1, ptr2, num);
    }
}

// start wrap fseek
static FseekFunc g_fseek = NULL;
void UpdateFseekFunc(FseekFunc func)
{
    g_fseek = func;
}
int __wrap_fseek(FILE *stream, long offset, int whence)
{
    if (g_fseek) {
        return g_fseek(stream, offset, whence);
    } else {
        return __real_fseek(stream, offset, whence);
    }
}

// start wrap fopen
static FopenFunc g_fopen = NULL;
void UpdateFopenFunc(FopenFunc func)
{
    g_fopen = func;
}
FILE* __wrap_fopen(const char *filename, const char *mode)
{
    if (g_fopen) {
        return g_fopen(filename, mode);
    } else {
        return __real_fopen(filename, mode);
    }
}

// start wrap ftell
static FtellFunc g_ftell = NULL;
void UpdateFtellFunc(FtellFunc func)
{
    g_ftell = func;
}
long __wrap_ftell(FILE *stream)
{
    if (g_ftell) {
        return g_ftell(stream);
    } else {
        return __real_ftell(stream);
    }
}

// start wrap strdup
static StrdupFunc g_strdup = NULL;
void UpdateStrdupFunc(StrdupFunc func)
{
    g_strdup = func;
}

char* __wrap_strdup(const char* string)
{
    if (g_strdup) {
        return g_strdup(string);
    } else {
        return __real_strdup(string);
    }
}

// start wrap malloc
static MallocFunc g_malloc = NULL;
void UpdateMallocFunc(MallocFunc func)
{
    g_malloc = func;
}

void* __wrap_malloc(size_t size)
{
    if (g_malloc) {
        return g_malloc(size);
    } else {
        return __real_malloc(size);
    }
}

// start wrap strncat_s
static StrncatSFunc g_strncat_s = NULL;
void UpdateStrncatSFunc(StrncatSFunc func)
{
    g_strncat_s = func;
}

int __wrap_strncat_s(char *strDest, size_t destMax, const char *strSrc, size_t count)
{
    if (g_strncat_s) {
        return g_strncat_s(strDest, destMax, strSrc, count);
    } else {
        return __real_strncat_s(strDest, destMax, strSrc, count);
    }
}

// start wrap mkdir
static MkdirFunc g_mkdir = NULL;
void UpdateMkdirFunc(MkdirFunc func)
{
    g_mkdir = func;
}

int __wrap_mkdir(const char *path, mode_t mode)
{
    if (g_mkdir) {
        return g_mkdir(path, mode);
    } else {
        return __real_mkdir(path, mode);
    }
}

// start wrap mount
static MountFunc g_mount = NULL;
void UpdateMountFunc(MountFunc func)
{
    g_mount = func;
}

int __wrap_mount(const char *source, const char *target,
    const char *fsType, unsigned long flags, const void *data)
{
    if (g_mount) {
        return g_mount(source, target, fsType, flags, data);
    } else {
        return __real_mount(source, target, fsType, flags, data);
    }
}


// start wrap ioctl
static IoctlFunc g_ioctl = NULL;
void UpdateIoctlFunc(IoctlFunc func)
{
    g_ioctl = func;
}

int __wrap_ioctl(int fd, int req, ...)
{
    va_list args;
    va_start(args, req);
    int rc;
    if (g_ioctl) {
        rc = g_ioctl(fd, req, args);
    } else {
        rc = __real_ioctl(fd, req, args);
    }
    va_end(args);
    return rc;
}

// start wrap calloc
static CallocFunc g_calloc = NULL;
void UpdateCallocFunc(CallocFunc func)
{
    g_calloc = func;
}

void* __wrap_calloc(size_t m, size_t n)
{
    if (g_calloc) {
        return g_calloc(m, n);
    } else {
        return __real_calloc(m, n);
    }
}

// start wrap minor
static MinorFunc g_minor = NULL;
void UpdateMinorFunc(MinorFunc func)
{
    g_minor = func;
}

int __wrap_minor(dev_t dev)
{
    if (g_minor) {
        return g_minor(dev);
    } else {
        return __real_minor(dev);
    }
}

// start wrap memset_s
static MemsetSFunc g_memset_s = NULL;
void UpdateMemsetSFunc(MemsetSFunc func)
{
    g_memset_s = func;
}

int __wrap_memset_s(void *dest, size_t destMax, int c, size_t count)
{
    if (g_memset_s) {
        return g_memset_s(dest, destMax, c, count);
    } else {
        return __real_memset_s(dest, destMax, c, count);
    }
}

// start wrap memcpy_s
static MemcpySFunc g_memcpy_s = NULL;
void UpdateMemcpySFunc(MemcpySFunc func)
{
    g_memcpy_s = func;
}

int __wrap_memcpy_s(void *dest, size_t destMax, const void *src, size_t count)
{
    if (g_memcpy_s) {
        return g_memcpy_s(dest, destMax, src, count);
    } else {
        return __real_memcpy_s(dest, destMax, src, count);
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
