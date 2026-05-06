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

#ifndef CHECKPOINT_TEST_HELPER_H
#define CHECKPOINT_TEST_HELPER_H

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <unistd.h>
#include <vector>

namespace OHOS {

using AddTlvFunction = std::function<int(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount)>;

/**
 * @brief Checkpoint消息参数结构体
 */
struct CheckpointMsgParams {
    uint32_t msgType;
    const char *processName;
    pid_t imagePid;
    uint64_t checkPointId;
    const char *checkPointImgName;  // checkpoint镜像进程名，NULL则使用bundleName
};

/**
 * @brief Checkpoint测试辅助类
 * 参考AppMgrTestHelper的实现方式，用于创建带有checkpoint信息的测试消息
 */
class CheckpointTestHelper {
public:
    CheckpointTestHelper() : processName_("com.example.checkpoint.test") {}
    ~CheckpointTestHelper() = default;

    /**
     * @brief 创建Checkpoint消息（简化接口）
     * @param buffer 消息缓冲区
     * @param msgLen 输出消息长度
     * @param params 消息参数结构体
     * @return 0成功，其他失败
     */
    int CreateCheckpointMsg(std::vector<uint8_t> &buffer, uint32_t &msgLen, const CheckpointMsgParams &params);

    /**
     * @brief 创建并发送消息，参考AppMgrTestHelper::AppMgrTestCreateSendMsg
     * @param buffer 消息缓冲区
     * @param msgType 消息类型
     * @param msgLen 输出消息长度
     * @param addTlvFuncs TLV添加函数列表
     * @return 0成功，其他失败
     */
    int CheckpointTestCreateSendMsg(std::vector<uint8_t> &buffer, uint32_t msgType, uint32_t &msgLen,
        const std::vector<AddTlvFunction> &addTlvFuncs);

    /**
     * @brief 添加基础TLV信息（参考AppMgrTestAddBaseTlv）
     * @param buffer TLV缓冲区
     * @param bufferLen 缓冲区长度
     * @param realLen 实际写入长度
     * @param tlvCount TLV计数
     * @return 0成功，其他失败
     */
    int CheckpointTestAddBaseTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount);

    /**
     * @brief 添加Checkpoint信息TLV
     * @param buffer TLV缓冲区
     * @param bufferLen 缓冲区长度
     * @param realLen 实际写入长度
     * @param tlvCount TLV计数
     * @param params Checkpoint参数结构体
     * @return 0成功，其他失败
     */
    int CheckpointTestAddCheckpointTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount,
        const CheckpointMsgParams &params);

    /**
     * @brief 添加Checkpoint信息TLV
     * @param buffer TLV缓冲区
     * @param bufferLen 缓冲区长度
     * @param realLen 实际写入长度
     * @param tlvCount TLV计数
     * @param params Checkpoint参数结构体
     * @return 0成功，其他失败
     */
    int CheckpointTestAddBundleTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount,
        const CheckpointMsgParams &params);

    /**
     * @brief 添加自定义长度的bundle info TLV（支持超长名称）
     * @param buffer TLV缓冲区
     * @param bufferLen 缓冲区长度
     * @param realLen 实际写入长度
     * @param tlvCount TLV计数
     * @param bundleName bundle名称字符串
     * @return 0成功，其他失败
     */
    int CheckpointTestAddLongBundleTlv(uint8_t *buffer, uint32_t bufferLen, uint32_t &realLen, uint32_t &tlvCount,
        const char *bundleName);

private:
    std::string processName_;
};

}  // namespace OHOS
#endif  // CHECKPOINT_TEST_HELPER_H
