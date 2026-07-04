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

#include "test_data_factory.h"
#include "securec.h"
#include "list.h"

SpmData* TestDataFactory::CreateMockSpmData(const SpmDataConfig &config)
{
    // 分配 SpmData 结构
    SpmData *data = (SpmData*)calloc(1, sizeof(SpmData));
    if (data == nullptr) {
        return nullptr;
    }

    // 分配 bundle name 缓冲区
    data->name.buf = (char*)calloc(1, MAX_BUNDLE_NAME_LEN);
    if (data->name.buf == nullptr) {
        free(data);
        return nullptr;
    }

    // 分配 extend permissions 缓冲区
    data->extendPerms.buf = (char*)calloc(1, MAX_EXTEND_PERM_SIZE);
    if (data->extendPerms.buf == nullptr) {
        free(data->name.buf);
        free(data);
        return nullptr;
    }

    // 填充数据
    data->tokenid = config.tokenid;
    data->tokenidAttr = config.tokenidAttr;
    data->uid = config.uid;
    data->apl = config.apl;
    data->ownerid = config.ownerid;
    data->idType = config.idType;
    data->distributionType = config.distributionType;
    data->index = 1;

    if (config.bundleName != nullptr) {
        data->name.bufSize = strlen(config.bundleName);
        if (data->name.bufSize >= MAX_BUNDLE_NAME_LEN) {
            data->name.bufSize = MAX_BUNDLE_NAME_LEN - 1;
        }
        memcpy_s(data->name.buf, MAX_BUNDLE_NAME_LEN, config.bundleName, data->name.bufSize);
    }

    return data;
}

AppSpawnMsgNode* TestDataFactory::CreateMockMessageNode()
{
    AppSpawnMsgNode *msg = (AppSpawnMsgNode*)calloc(1, sizeof(AppSpawnMsgNode));
    if (msg == nullptr) {
        return nullptr;
    }

    // 初始化消息头
    msg->msgHeader.msgId = 1;
    msg->msgHeader.msgLen = sizeof(AppSpawnMsg);
    strncpy_s(msg->msgHeader.processName, sizeof(msg->msgHeader.processName),
               testProcessName, strlen(testProcessName));

    // 初始化 TLV offset 数组
    msg->tlvOffset = (uint32_t*)calloc(TLV_MAX, sizeof(uint32_t));
    if (msg->tlvOffset == nullptr) {
        free(msg);
        return nullptr;
    }

    for (int i = 0; i < TLV_MAX; i++) {
        msg->tlvOffset[i] = INVALID_OFFSET;
    }

    return msg;
}

AppSpawnMgr* TestDataFactory::CreateMockAppSpawnMgr(uint32_t runMode)
{
    AppSpawnMgr *mgr = (AppSpawnMgr*)calloc(1, sizeof(AppSpawnMgr));
    if (mgr == nullptr) {
        return nullptr;
    }

    mgr->content.mode = static_cast<RunMode>(runMode);

    return mgr;
}

AppSpawningCtx* TestDataFactory::CreateMockSpawningCtx()
{
    AppSpawningCtx *ctx = (AppSpawningCtx*)calloc(1, sizeof(AppSpawningCtx));
    if (ctx == nullptr) {
        return nullptr;
    }

    ctx->pid = TEST_MOCK_PID;
    ctx->spmRefAdded = SPM_REF_NONE;

    return ctx;
}

SandboxQueue* TestDataFactory::CreateMockPermissionQueue()
{
    SandboxQueue *queue = (SandboxQueue*)calloc(1, sizeof(SandboxQueue));
    if (queue == nullptr) {
        return nullptr;
    }

    OH_ListInit(&queue->front);

    return queue;
}

void TestDataFactory::FreeSpmData(SpmData *data)
{
    if (data != nullptr) {
        if (data->name.buf != nullptr) {
            free(data->name.buf);
        }
        if (data->extendPerms.buf != nullptr) {
            free(data->extendPerms.buf);
        }
        free(data);
    }
}

void TestDataFactory::FreeMessageNode(AppSpawnMsgNode *msg)
{
    if (msg != nullptr) {
        if (msg->buffer != nullptr) {
            free(msg->buffer);
        }
        if (msg->tlvOffset != nullptr) {
            free(msg->tlvOffset);
        }
        free(msg);
    }
}

void TestDataFactory::FreeAppSpawnMgr(AppSpawnMgr *mgr)
{
    if (mgr != nullptr) {
        free(mgr);
    }
}

void TestDataFactory::FreeSpawningCtx(AppSpawningCtx *ctx)
{
    if (ctx != nullptr) {
        if (ctx->message != nullptr) {
            FreeMessageNode(ctx->message);
        }
        free(ctx);
    }
}

void TestDataFactory::FreePermissionQueue(SandboxQueue *queue)
{
    if (queue != nullptr) {
        // 注意：这里假设队列中的节点已经在外部释放
        free(queue);
    }
}
