/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#ifndef APPSPAWN_ENCAPS_CPP
#define APPSPAWN_ENCAPS_CPP

#ifdef __cplusplus
extern "C" {
#endif

#define OH_ENCAPS_KEY_MAX_LEN 64

typedef enum {
    ENCAPS_PROC_TYPE_MODE,        // enable the encaps attribute of a process
    ENCAPS_PERMISSION_TYPE_MODE,  // set the encaps permission of a process
    ENCAPS_MAX_TYPE_MODE
} AppSpawnEncapsBaseType;

typedef enum {
    ENCAPS_BOOL,
    ENCAPS_INT,
    ENCAPS_AS_ARRAY = 10,
    ENCAPS_BOOL_ARRAY,
    ENCAPS_INT_ARRAY,
    ENCAPS_CHAR_ARRAY
} EncapsType;

typedef struct {
    char key[OH_ENCAPS_KEY_MAX_LEN];
    EncapsType type;
    union {
        uint64_t intValue;
        void *ptrValue;
    } value;
    uint32_t valueLen;  // valueLen < 512
} UserEncap;

typedef struct {
    UserEncap *encap;
    uint32_t encapsCount;
} UserEncaps;

#ifdef __cplusplus
}
#endif
#endif
