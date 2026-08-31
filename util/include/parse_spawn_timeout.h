/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef STARTUP_PARSE_SPAWN_TIMEOUT_H
#define STARTUP_PARSE_SPAWN_TIMEOUT_H

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Parse a full decimal uint32 string. Reject empty, signs, spaces, junk, overflow. */
static inline bool ParseSpawnTimeoutU32(const char *text, uint32_t *out)
{
    char *end = NULL;
    unsigned long value;

    if (text == NULL || out == NULL || *text == '\0') {
        return false;
    }
    /* strtoul accepts leading whitespace and optional sign; reject those. */
    if (*text < '0' || *text > '9') {
        return false;
    }
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno == ERANGE || end == text || *end != '\0' || value > UINT32_MAX) {
        return false;
    }
    *out = (uint32_t)value;
    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* STARTUP_PARSE_SPAWN_TIMEOUT_H */
