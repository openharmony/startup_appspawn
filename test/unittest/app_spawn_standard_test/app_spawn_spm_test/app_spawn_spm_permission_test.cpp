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

#include <gtest/gtest.h>
#include "appspawn_utils.h"
#include "appspawn_mount_permission.h"
#include "spm_permission.h"
#include "securec.h"
#include "list.h"

using namespace testing;
using namespace testing::ext;

/**
 * @brief SPM 权限模块测试类
 */
class AppSpawnSpmPermissionTest : public testing::Test {
protected:
    void SetUp() override
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";
        APPSPAWN_LOGI("%{public}s.%{public}s start", info->test_suite_name(), info->name());

        // 初始化测试队列
        OH_ListInit(&testQueue.front);
        testQueue.type = 0;
    }

    void TearDown() override
    {
        const TestInfo *info = UnitTest::GetInstance()->current_test_info();
        GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";
        APPSPAWN_LOGI("%{public}s.%{public}s end", info->test_suite_name(), info->name());

        // 清理测试队列中的节点
        ListNode *node = testQueue.front.next;
        while (node != &testQueue.front) {
            ListNode *next = node->next;
            SandboxPermissionNode *permNode = (SandboxPermissionNode *)ListEntry(node, SandboxMountNode, node);
            // name 字段是灵活数组成员，包含在 calloc 分配的内存中，无需单独释放
            free(permNode);
            node = next;
        }
        OH_ListInit(&testQueue.front);
    }

    /**
     * @brief 创建测试用的权限节点
     * @param name 权限名称
     * @param permissionIndex 权限索引
     * @param opcode 权限 opcode
     * @return 创建的权限节点指针
     *
     * 注意：SPM 模块使用 APPSPAWN_CLIENT 定义，结构为：
     * typedef struct TagPermissionNode {
     *     SandboxMountNode sandboxNode;
     *     uint32_t permissionIndex;
     *     uint32_t opcode;
     *     char name[0];
     * } SandboxPermissionNode;
     */
    SandboxPermissionNode* CreateTestPermissionNode(const char *name, uint32_t permissionIndex, uint32_t opcode)
    {
        // 计算需要的内存大小（结构体 + name 字符串）
        size_t nameLen = name != nullptr ? strlen(name) + 1 : 0;
        size_t totalSize = sizeof(SandboxPermissionNode) + nameLen;

        // 分配内存
        SandboxPermissionNode *node = (SandboxPermissionNode *)calloc(1, totalSize);
        if (node == nullptr) {
            return nullptr;
        }

        // 设置权限信息
        node->sandboxNode.node.prev = nullptr;
        node->sandboxNode.node.next = nullptr;
        node->permissionIndex = permissionIndex;
        node->opcode = opcode;

        // 复制 name 字符串（使用灵活数组成员）
        if (name != nullptr && nameLen > 0) {
            int ret = memcpy_s(node->name, nameLen, name, nameLen - 1);
            if (ret != 0) {
                free(node);
                return nullptr;
            }
            node->name[nameLen - 1] = '\0';
        }

        return node;
    }

    /**
     * @brief 将权限节点添加到测试队列
     * @param node 权限节点
     */
    void AddPermissionToQueue(SandboxPermissionNode *node)
    {
        OH_ListAddTail(&testQueue.front, &node->sandboxNode.node);
    }

    // 测试队列
    SandboxQueue testQueue;
};

// ============================================================================
// GetPermissionNodeByOpcode 测试用例
// ============================================================================

/**
 * @brief 正常查找权限节点
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetPermissionNodeByOpcode_001, TestSize.Level1)
{
    // 准备测试数据：创建三个权限节点
    SandboxPermissionNode *node1 = CreateTestPermissionNode("permission1", 0, 100);
    SandboxPermissionNode *node2 = CreateTestPermissionNode("permission2", 1, 200);
    SandboxPermissionNode *node3 = CreateTestPermissionNode("permission3", 2, 300);

    ASSERT_NE(node1, nullptr);
    ASSERT_NE(node2, nullptr);
    ASSERT_NE(node3, nullptr);

    // 添加到队列
    AddPermissionToQueue(node1);
    AddPermissionToQueue(node2);
    AddPermissionToQueue(node3);

    // 测试查找 opcode=200 的节点
    const SandboxPermissionNode *result = GetPermissionNodeByOpcode(&testQueue, 200);

    // 验证结果
    EXPECT_NE(result, nullptr);
    if (result != nullptr) {
        EXPECT_EQ(result->permissionIndex, 1);
        EXPECT_STREQ(result->name, "permission2");
    }
}

/**
 * @brief 查找 opcode 为 0 的节点
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetPermissionNodeByOpcode_002, TestSize.Level2)
{
    // 准备测试数据：创建一个 opcode=0 的节点
    SandboxPermissionNode *node1 = CreateTestPermissionNode("permission_zero", 0, 0);
    SandboxPermissionNode *node2 = CreateTestPermissionNode("permission_normal", 1, 100);

    ASSERT_NE(node1, nullptr);
    ASSERT_NE(node2, nullptr);

    // 添加到队列
    AddPermissionToQueue(node1);
    AddPermissionToQueue(node2);

    // 测试查找 opcode=0 的节点
    const SandboxPermissionNode *result = GetPermissionNodeByOpcode(&testQueue, 0);

    // 验证结果：应该找到 opcode=0 的节点
    EXPECT_NE(result, nullptr);
    if (result != nullptr) {
        EXPECT_EQ(result->permissionIndex, 0);
        EXPECT_STREQ(result->name, "permission_zero");
    }
}

/**
 * @brief 查找不存在的 opcode
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetPermissionNodeByOpcode_003, TestSize.Level2)
{
    // 准备测试数据：创建两个节点
    SandboxPermissionNode *node1 = CreateTestPermissionNode("permission1", 0, 100);
    SandboxPermissionNode *node2 = CreateTestPermissionNode("permission2", 1, 200);

    ASSERT_NE(node1, nullptr);
    ASSERT_NE(node2, nullptr);

    // 添加到队列
    AddPermissionToQueue(node1);
    AddPermissionToQueue(node2);

    // 测试查找不存在的 opcode=999
    const SandboxPermissionNode *result = GetPermissionNodeByOpcode(&testQueue, 999);

    // 验证结果：应该返回 NULL
    EXPECT_EQ(result, nullptr);
}

/**
 * @brief 空队列中查找节点
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetPermissionNodeByOpcode_004, TestSize.Level2)
{
    // 测试在空队列中查找
    const SandboxPermissionNode *result = GetPermissionNodeByOpcode(&testQueue, 100);

    // 验证结果：应该返回 NULL
    EXPECT_EQ(result, nullptr);
}

// ============================================================================
// GetPermissionNameByOpcode 测试用例
// ============================================================================

/**
 * @brief 正常获取权限名称
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetPermissionNameByOpcode_001, TestSize.Level1)
{
    // 准备测试数据
    SandboxPermissionNode *node = CreateTestPermissionNode("test_permission", 5, 150);
    ASSERT_NE(node, nullptr);
    AddPermissionToQueue(node);

    // 测试获取权限名称
    const char *name = GetPermissionNameByOpcode(&testQueue, 150);

    // 验证结果
    EXPECT_NE(name, nullptr);
    if (name != nullptr) {
        EXPECT_STREQ(name, "test_permission");
    }
}

/**
 * @brief 获取不存在 opcode 的权限名称
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetPermissionNameByOpcode_002, TestSize.Level2)
{
    // 准备测试数据
    SandboxPermissionNode *node = CreateTestPermissionNode("test_permission", 5, 150);
    ASSERT_NE(node, nullptr);
    AddPermissionToQueue(node);

    // 测试获取不存在的 opcode
    const char *name = GetPermissionNameByOpcode(&testQueue, 999);

    // 验证结果：应该返回 NULL
    EXPECT_EQ(name, nullptr);
}

// ============================================================================
// GetSpawnFlagIndexesFromPermissionBitmap 测试用例
// ============================================================================

/**
 * @brief 正常转换权限位图
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetSpawnFlagIndexesFromPermissionBitmap_001, TestSize.Level1)
{
    // 准备测试数据：创建权限节点
    // 假设有 3 个权限：
    // - permissionIndex=0, opcode=10
    // - permissionIndex=5, opcode=20
    // - permissionIndex=10, opcode=30
    SandboxPermissionNode *node1 = CreateTestPermissionNode("perm0", 0, 10);
    SandboxPermissionNode *node2 = CreateTestPermissionNode("perm5", 5, 20);
    SandboxPermissionNode *node3 = CreateTestPermissionNode("perm10", 10, 30);

    ASSERT_NE(node1, nullptr);
    ASSERT_NE(node2, nullptr);
    ASSERT_NE(node3, nullptr);

    AddPermissionToQueue(node1);
    AddPermissionToQueue(node2);
    AddPermissionToQueue(node3);

    // 准备权限位图：授权 opcode=10 和 opcode=30，不授权 opcode=20
    uint32_t perms[MAX_PERM_BIT_MAP_SIZE] = {0};
    perms[0] = (1U << 10) | (1U << 30);  // bitmapIndex 0: bit 10 和 bit 30

    // 准备输出数组（1个 word 足够覆盖 permissionIndex 0-10）
    uint32_t flagIndexes[1] = {0};

    // 调用函数
    int32_t ret = GetSpawnFlagIndexesFromPermissionBitmap(&testQueue, perms, flagIndexes, 1);

    // 验证返回值
    EXPECT_EQ(ret, 0);

    // 验证结果：应该设置 permissionIndex=0 和 permissionIndex=10 的位
    // permissionIndex=0 -> wordIdx=0, bitIdx=0
    // permissionIndex=10 -> wordIdx=0, bitIdx=10
    EXPECT_EQ(flagIndexes[0] & (1U << 0), (1U << 0));  // permissionIndex=0 已授权
    EXPECT_EQ(flagIndexes[0] & (1U << 10), (1U << 10));  // permissionIndex=10 已授权
    EXPECT_EQ(flagIndexes[0] & (1U << 5), 0);  // permissionIndex=5 未授权
}

/**
 * @brief 权限位图为空
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetSpawnFlagIndexesFromPermissionBitmap_002, TestSize.Level2)
{
    // 准备测试数据：创建权限节点
    SandboxPermissionNode *node = CreateTestPermissionNode("perm0", 0, 10);
    ASSERT_NE(node, nullptr);
    AddPermissionToQueue(node);

    // 准备空的权限位图
    uint32_t perms[MAX_PERM_BIT_MAP_SIZE] = {0};

    // 准备输出数组
    uint32_t flagIndexes[1] = {0};

    // 调用函数
    int32_t ret = GetSpawnFlagIndexesFromPermissionBitmap(&testQueue, perms, flagIndexes, 1);

    // 验证返回值
    EXPECT_EQ(ret, 0);

    // 验证结果：所有权限都应该未授权
    EXPECT_EQ(flagIndexes[0], 0);
}

/**
 * @brief 参数校验：queue 为 NULL
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetSpawnFlagIndexesFromPermissionBitmap_003, TestSize.Level2)
{
    // 准备权限位图
    uint32_t perms[MAX_PERM_BIT_MAP_SIZE] = {0x1};
    uint32_t flagIndexes[1] = {0};

    // 调用函数，queue 为 NULL
    int32_t ret = GetSpawnFlagIndexesFromPermissionBitmap(nullptr, perms, flagIndexes, 1);

    // 验证返回值：应该返回 -1
    EXPECT_EQ(ret, -1);
}

/**
 * @brief 参数校验：flagCount 为 0
 */
HWTEST_F(AppSpawnSpmPermissionTest, App_Spawn_Spm_GetSpawnFlagIndexesFromPermissionBitmap_004, TestSize.Level2)
{
    // 准备权限节点
    SandboxPermissionNode *node = CreateTestPermissionNode("perm0", 0, 10);
    ASSERT_NE(node, nullptr);
    AddPermissionToQueue(node);

    // 准备权限位图
    uint32_t perms[MAX_PERM_BIT_MAP_SIZE] = {0x1};
    uint32_t flagIndexes[1] = {0};

    // 调用函数，flagCount 为 0
    int32_t ret = GetSpawnFlagIndexesFromPermissionBitmap(&testQueue, perms, flagIndexes, 0);

    // 验证返回值：应该返回 -1
    EXPECT_EQ(ret, -1);
}
