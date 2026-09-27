/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef SANDBOX_CLONE_APP_H
#define SANDBOX_CLONE_APP_H

#include "sandbox_def.h"
#include "appspawn_manager.h"

namespace OHOS {
namespace AppSpawn {

typedef struct CloneAppGrantUri {
    struct ListNode node; //链表节点
    uint32_t grantUri; // 目录名使用的包名类型: 非0值分身包名 0原应用包名
    uint32_t indexMin; // 规则生效的bundleIndex下界(含)
    uint32_t indexMax; // 规则生效的bundleIndex上界(含)
} CloneAppGrantUri;

typedef struct CloneAppGrantExtData {
    AppSpawnExtData extData;                 // 扩展数据节点, dataId为EXT_DATA_CLONE_APP_GRANT
    bool configLoaded;                       // 配置文件是否加载成功
    uint32_t grantCount;                     // 有效规则数
    struct ListNode grantQueue;              // 授权规则链表, 按配置顺序匹配
} CloneAppGrantExtData;

} // namespace AppSpawn
} // namespace OHOS

#endif // SANDBOX_CLONE_APP_H
