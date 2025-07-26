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

#ifndef APPSPAWN_LIB_FUNC_DEFINE_H
#define APPSPAWN_LIB_FUNC_DEFINE_H

#define fseeko Fseeko
#define ftello Ftello
#define access Access
#define mkdir Mkdir
#define fread Fread
#define fwrite Fwrite
#define realpath Realpath
#define fopen Fopen
#define fclose Fclose
#define chmod Chmod
#define utime Utime
#define stat(pathname, statbuf) Stat(pathname, statbuf)
#define ferror Ferror
#define fflush Fflush
#define remove Remove
#define getpwuid Getpwuid
#define getgrgid Getgrgid
#define open Open
#define read Read
#define write Write
#define close Close
#define umount2 Umount2
#define rmdir Rmdir

#endif // APPSPAWN_LIB_FUNC_DEFINE_H