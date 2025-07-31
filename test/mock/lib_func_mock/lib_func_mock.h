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

#ifndef APPSPAWN_LIB_FUNC_MOCK_H
#define APPSPAWN_LIB_FUNC_MOCK_H

#include <gmock/gmock.h>

#include <cstdio>
#include <fcntl.h>
#include <grp.h>
#include <memory>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "securec.h"

int Fseeko(FILE*, off_t, int);
off_t Ftello(FILE*);
int Access(const char*, int);
int Mkdir(const char*, mode_t);
size_t Fread(void*, size_t, size_t, FILE*);
size_t Fwrite(const void*, size_t, size_t, FILE*);
char* Realpath(const char*, char*);
FILE* Fopen(const char*, const char*);
int Fclose(FILE*);
int Chmod(const char*, mode_t);
int Stat(const char*, struct stat*);
int Utime(const char*, const struct utimbuf*);
int Ferror(FILE*);
int Fflush(FILE*);
int Remove(const char*);
struct passwd* Getpwuid(uid_t);
struct group* Getgrgid(gid_t);
int Open(const char*, int, ...);
ssize_t Read(int, void*, size_t);
ssize_t Write(int, const void*, size_t);
int Close(int);
int Umount2(const char*, int);
int Rmdir(const char*);


namespace OHOS {
namespace AppSpawn {
class LibraryFunc {
public:
    virtual ~LibraryFunc() = default;
    virtual int fseeko(FILE*, off_t, int) = 0;
    virtual off_t ftello(FILE*) = 0;
    virtual int access(const char*, int) = 0;
    virtual int mkdir(const char*, mode_t) = 0;
    virtual size_t fread(void*, size_t, size_t, FILE*) = 0;
    virtual size_t fwrite(const void*, size_t, size_t, FILE*) = 0;
    virtual char* realpath(const char*, char*) = 0;
    virtual FILE* fopen(const char*, const char*) = 0;
    virtual int fclose(FILE*) = 0;
    virtual int chmod(const char*, mode_t) = 0;
    virtual int stat(const char*, struct stat*) = 0;
    virtual int utime(const char*, const struct utimbuf*) = 0;
    virtual int ferror(FILE*) = 0;
    virtual int fflush(FILE*) = 0;
    virtual int remove(const char*) = 0;
    virtual struct passwd* getpwuid(uid_t) = 0;
    virtual struct group* getgrgid(gid_t) = 0;
    virtual int open(const char *, int) = 0;
    virtual ssize_t read(int, void*, size_t) = 0;
    virtual ssize_t write(int, const void*, size_t) = 0;
    virtual int close(int) = 0;
    virtual int umount2(const char*, int) = 0;
    virtual int rmdir(const char*) = 0;
public:
    static inline std::shared_ptr<LibraryFunc> libraryFunc_ = nullptr;
};

class LibraryFuncMock : public LibraryFunc {
public:
    MOCK_METHOD(int, fseeko, (FILE*, off_t, int));
    MOCK_METHOD(off_t, ftello, (FILE*));
    MOCK_METHOD(int, access, (const char*, int));
    MOCK_METHOD(int, mkdir, (const char*, mode_t));
    MOCK_METHOD(size_t, fread, (void*, size_t, size_t, FILE*));
    MOCK_METHOD(size_t, fwrite, (const void*, size_t, size_t, FILE*));
    MOCK_METHOD(char*, realpath, (const char*, char*));
    MOCK_METHOD(FILE*, fopen, (const char*, const char*));
    MOCK_METHOD(int, fclose, (FILE*));
    MOCK_METHOD(int, chmod, (const char*, mode_t));
    MOCK_METHOD(int, stat, (const char*, struct stat*));
    MOCK_METHOD(int, utime, (const char*, const struct utimbuf*));
    MOCK_METHOD(int, ferror, (FILE*));
    MOCK_METHOD(int, fflush, (FILE*));
    MOCK_METHOD(int, remove, (const char*));
    MOCK_METHOD(passwd*, getpwuid, (uid_t));
    MOCK_METHOD(group*, getgrgid, (gid_t));
    MOCK_METHOD(int, open, (const char*, int));
    MOCK_METHOD(ssize_t, read, (int, void*, size_t));
    MOCK_METHOD(ssize_t, write, (int, const void*, size_t));
    MOCK_METHOD(int, close, (int));
    MOCK_METHOD(int, umount2, (const char*, int));
    MOCK_METHOD(int, rmdir, (const char*));
};
} // namespace AppSpawn
} // namespace OHOS

#endif // APPSPAWN_LIB_FUNC_MOCK_H