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

#ifndef TEST_WRAPPER_H
#define TEST_WRAPPER_H
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <errno.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
// for wrapper execve;
int __real_execve(const char *pathname, char *const argv[], char *const envp[]);
typedef int (*ExecveFunc)(const char *pathname, char *const argv[], char *const envp[]);
void UpdateExecveFunc(ExecveFunc func);

// for wrapper read;
ssize_t __real_read(int fd, void *buf, size_t count);
typedef ssize_t (*ReadFunc)(int fd, void *buf, size_t count);
void UpdateReadFunc(ReadFunc func);

// for wrapper fread;
size_t __real_fread(void *ptr, size_t size, size_t count, FILE *stream);
typedef size_t (*FreadFunc)(void *ptr, size_t size, size_t count, FILE *stream);
void UpdateFreadFunc(FreadFunc func);

// for wrapper memcmp;
int __real_memcmp(const void *ptr1, const void *ptr2, size_t num);
typedef int (*MemcmpFunc)(const void *ptr1, const void *ptr2, size_t num);
void UpdateMemcmpFunc(MemcmpFunc func);

// for wrapper fseek;
int __real_fseek(FILE *stream, long offset, int whence);
typedef int (*FseekFunc)(FILE *stream, long offset, int whence);
void UpdateFseekFunc(FseekFunc func);

// for wrapper fopen;
FILE* __real_fopen(const char *filename, const char *mode);
typedef FILE* (*FopenFunc)(const char *filename, const char *mode);
void UpdateFopenFunc(FopenFunc func);

// for wrapper ftell;
long __real_ftell(FILE *stream);
typedef long (*FtellFunc)(FILE *stream);
void UpdateFtellFunc(FtellFunc func);

// for wrapper strdup;
char* __real_strdup(const char* string);
typedef char* (*StrdupFunc)(const char* string);
void UpdateStrdupFunc(StrdupFunc func);

// for wrapper malloc;
void* __real_malloc(size_t size);
typedef void* (*MallocFunc)(size_t size);
void UpdateMallocFunc(MallocFunc func);

// for wrapper strncat_s;
int __real_strncat_s(char *strDest, size_t destMax, const char *strSrc, size_t count);
typedef int (*StrncatSFunc)(char *strDest, size_t destMax, const char *strSrc, size_t count);
void UpdateStrncatSFunc(StrncatSFunc func);

// for wrapper mkdir;
int __real_mkdir(const char *path, mode_t mode);
typedef int (*MkdirFunc)(const char *path, mode_t mode);
void UpdateMkdirFunc(MkdirFunc func);

// for wrapper mount;
int __real_mount(const char *source, const char *target, const char *fsType, unsigned long flags, const void *data);
typedef int (*MountFunc)(const char *source, const char *target,
    const char *fsType, unsigned long flags, const void *data);
void UpdateMountFunc(MountFunc func);

// for wrapper ioctl;
int __real_ioctl(int fd, int req, ...);
typedef int (*IoctlFunc)(int fd, int req, va_list args);
void UpdateIoctlFunc(IoctlFunc func);

// for wrapper calloc;
void* __real_calloc(size_t m, size_t n);
typedef void* (*CallocFunc)(size_t m, size_t n);
void UpdateCallocFunc(CallocFunc func);

// for wrapper minor;
int __real_minor(dev_t dev);
typedef int (*MinorFunc)(dev_t dev);
void UpdateMinorFunc(MinorFunc func);

// for wrapper memset_s;
int __real_memset_s(void *dest, size_t destMax, int c, size_t count);
typedef int (*MemsetSFunc)(void *dest, size_t destMax, int c, size_t count);
void UpdateMemsetSFunc(MemsetSFunc func);

// for wrapper memcpy_s;
int __real_memcpy_s(void *dest, size_t destMax, const void *src, size_t count);
typedef int (*MemcpySFunc)(void *dest, size_t destMax, const void *src, size_t count);
void UpdateMemcpySFunc(MemcpySFunc func);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif // GENERATED_WRAPPER_H
