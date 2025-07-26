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

#ifndef APPSPAWN_LIB_FUNC_UNDEF_H
#define APPSPAWN_LIB_FUNC_UNDEF_H

#define Fseeko fseeko
#define Ftello ftello
#define Access access
#define Mkdir mkdir
#define Fread fread
#define Fwrite fwrite
#define Realpath realpath
#define Fopen fopen
#define Fclose fclose
#define Chmod chmod
#define Utime utime
#define Stat(pathname, statbuf) stat(pathname, statbuf)
#define Ferror ferror
#define Fflush fflush
#define Remove remove
#define Getpwuid getpwuid
#define Getgrgid getgrgid
#define Open open
#define Read read
#define Write write
#define Close close
#define Umount2 umount2
#define Rmdir rmdir

#endif // APPSPAWN_LIB_FUNC_UNDEF_H