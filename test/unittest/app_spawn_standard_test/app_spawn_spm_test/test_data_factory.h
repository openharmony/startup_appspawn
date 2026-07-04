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

#ifndef TEST_DATA_FACTORY_H
#define TEST_DATA_FACTORY_H

#include <cstring>
#include <cstdlib>
// Include actual type definitions FIRST, before mock headers
#include "appspawn_manager.h"
#include "appspawn_msg.h"
#include "appspawn_mount_permission.h"
#include "appspawn_hook.h"  // For RunMode
#include "appspawn_server.h"  // For RunMode enum
#include "modules/spm/spm.h"  // For APL_SYSTEM_CORE enum
#include "spm_func_mock.h"  // For SpmData, SPAWN_ID_APP, etc. (must come AFTER appspawn_manager.h)

/**
 * @brief SPM 测试数据配置结构体
 */
struct SpmDataConfig {
    uint32_t tokenid = 12345678;
    uint32_t tokenidAttr = 0x12345678;
    uint32_t uid = 1000;
    uint32_t apl = APL_SYSTEM_CORE;
    uint64_t ownerid = 999999;
    uint32_t idType = 0;
    uint32_t distributionType = 1;
    const char* bundleName = "com.example.app";
};

/**
 * @brief 测试数据工厂类
 *
 * 提供各种测试数据结构的构建函数
 */
class TestDataFactory {
public:
    /**
     * @brief 创建测试用 SPM 数据
     * @param config SPM 数据配置，使用默认值或自定义字段
     * @return SpmData指针，调用方负责释放
     */
    static SpmData* CreateMockSpmData(const SpmDataConfig &config = SpmDataConfig());

    /**
     * @brief 创建测试用消息节点
     * @return AppSpawnMsgNode指针，调用方负责释放
     */
    static AppSpawnMsgNode* CreateMockMessageNode();

    /**
     * @brief 创建测试用 AppSpawnMgr
     * @return AppSpawnMgr指针，调用方负责释放
     */
    static AppSpawnMgr* CreateMockAppSpawnMgr(uint32_t runMode = MODE_FOR_APP_SPAWN);

    /**
     * @brief 创建测试用 AppSpawningCtx
     * @return AppSpawningCtx指针，调用方负责释放
     */
    static AppSpawningCtx* CreateMockSpawningCtx();

    /**
     * @brief 创建测试用权限队列
     * @return SandboxQueue指针，调用方负责释放
     */
    static SandboxQueue* CreateMockPermissionQueue();

    /**
     * @brief 释放 SPM 数据
     */
    static void FreeSpmData(SpmData *data);

    /**
     * @brief 释放消息节点
     */
    static void FreeMessageNode(AppSpawnMsgNode *msg);

    /**
     * @brief 释放 AppSpawnMgr
     */
    static void FreeAppSpawnMgr(AppSpawnMgr *mgr);

    /**
     * @brief 释放 AppSpawningCtx
     */
    static void FreeSpawningCtx(AppSpawningCtx *ctx);

    /**
     * @brief 释放权限队列
     */
    static void FreePermissionQueue(SandboxQueue *queue);

    // 预定义的测试数据常量
    static constexpr uint32_t TEST_MOCK_PID = 12345;
    static constexpr uint32_t TEST_TOKENID = 12345678;
    static constexpr uint32_t TEST_TOKENID_ATTR = 0x12345678;
    static constexpr uint32_t TEST_UID = 1000;
    static constexpr uint32_t TEST_GID = 1000;
    static constexpr uint32_t TEST_SPAWN_ID = SPAWN_ID_APP;
    static constexpr char testBundleName[] = "com.example.app";
    static constexpr char testProcessName[] = "com.example.app.TestProcess";
};

#endif  // TEST_DATA_FACTORY_H
