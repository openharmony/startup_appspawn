/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#ifndef NWEBSPAWNLANCHER_CPP
#define NWEBSPAWNLANCHER_CPP
#include "selinux/selinux.h"
#include <sys/capability.h>
#include <sys/resource.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sched.h>
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif
pid_t NwebSpawnLanch();
#ifdef __cplusplus
}
#endif

#endif