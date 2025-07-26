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

#include "lib_func_mock.h"

using namespace OHOS::AppSpawn;
int Fseeko(FILE *stream, off_t offset, int whence)
{
    return LibraryFunc::libraryFunc_->fseeko(stream, offset, whence);
}

off_t Ftello(FILE *stream)
{
    return LibraryFunc::libraryFunc_->ftello(stream);
}

int Access(const char *pathname, int mode)
{
    return LibraryFunc::libraryFunc_->access(pathname, mode);
}

int Mkdir(const char *pathname, mode_t mode)
{
    return LibraryFunc::libraryFunc_->mkdir(pathname, mode);
}

size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return LibraryFunc::libraryFunc_->fread(ptr, size, nmemb, stream);
}

size_t Fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    return LibraryFunc::libraryFunc_->fwrite(ptr, size, nmemb, stream);
}

char *Realpath(const char *path, char *resolved_path)
{
    return LibraryFunc::libraryFunc_->realpath(path, resolved_path);
}

FILE *Fopen(const char *pathname, const char *mode)
{
    return LibraryFunc::libraryFunc_->fopen(pathname, mode);
}

int Fclose(FILE *stream)
{
    return LibraryFunc::libraryFunc_->fclose(stream);
}

int Chmod(const char *pathname, mode_t mode)
{
    return LibraryFunc::libraryFunc_->chmod(pathname, mode);
}

int Stat(const char *pathname, struct stat *statbuf)
{
    return LibraryFunc::libraryFunc_->stat(pathname, statbuf);
}

int Utime(const char *filename, const struct utimbuf *times)
{
    return LibraryFunc::libraryFunc_->utime(filename, times);
}

int Ferror(FILE *f)
{
    return LibraryFunc::libraryFunc_->ferror(f);
}

int Fflush(FILE *f)
{
    return LibraryFunc::libraryFunc_->fflush(f);
}

int Remove(const char *path)
{
    return LibraryFunc::libraryFunc_->remove(path);
}

struct passwd *Getpwuid(uid_t uid)
{
    return LibraryFunc::libraryFunc_->getpwuid(uid);
}

struct group *Getgrgid(gid_t gid)
{
    return LibraryFunc::libraryFunc_->getgrgid(gid);
}

int Open(const char *filename, int flags, ...)
{
    return LibraryFunc::libraryFunc_->open(filename, flags);
}

ssize_t Read(int fd, void *buf, size_t count)
{
    return LibraryFunc::libraryFunc_->read(fd, buf, count);
}

ssize_t Write(int fd, const void *buf, size_t count)
{
    return LibraryFunc::libraryFunc_->write(fd, buf, count);
}

int Close(int fd)
{
    return LibraryFunc::libraryFunc_->close(fd);
}

int Umount2(const char *file, int flag)
{
    return LibraryFunc::libraryFunc_->umount2(file, flag);
}

int Rmdir(const char *file)
{
    return LibraryFunc::libraryFunc_->rmdir(file);
}