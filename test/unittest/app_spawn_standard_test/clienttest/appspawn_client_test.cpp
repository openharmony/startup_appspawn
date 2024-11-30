/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_manager.h"
#include "appspawn_modulemgr.h"
#include "appspawn_permission.h"
#include "appspawn_sandbox.h"
#include "appspawn_utils.h"
#include "appspawn_server.h"
#include "securec.h"

#include "app_spawn_stub.h"
#include "app_spawn_test_helper.h"

// 假设的全局配置
using namespace testing;
using namespace testing::ext;

namespace OHOS {
    class GetDefaultTimeoutTest : public testing::Test {
    protected:
        void SetUp() override
        {
            // 每个测试前初始化
            global_value = nullptr; // 确保全局值清空
        }

        void TearDown() override
        {
            // 每个测试后进行清理
            global_value = nullptr; // 清理全局设置
        }
    };

    // 测试默认值
    TEST_F(GetDefaultTimeoutTest, TestDefaultValue)
    {
        uint32_t def = 60;
        uint32_t timeout = GetDefaultTimeout(def);
        EXPECT_EQ(timeout, def); // 应该返回默认值 60
    }

    // 测试 SetParameter 设置的有效超时值
    TEST_F(GetDefaultTimeoutTest, TestValidTimeout)
    {
        SetParameter("persist.appspawn.reqMgr.timeout", "30"); // 设置超时时间为 30

        uint32_t def = 60;
        uint32_t timeout = GetDefaultTimeout(def);
        EXPECT_EQ(timeout, 30); // 应该返回 30
    }

    // 测试 SetParameter 设置的无效超时值（非数字）
    TEST_F(GetDefaultTimeoutTest, TestInvalidTimeout)
    {
        SetParameter("persist.appspawn.reqMgr.timeout", "invalid"); // 设置超时时间为无效值

        uint32_t def = 60;
        uint32_t timeout = GetDefaultTimeout(def);
        EXPECT_EQ(timeout, def); // 应该返回默认值 60
    }

    // 测试 SetParameter 设置 "0" 时的行为
    TEST_F(GetDefaultTimeoutTest, TestZeroTimeout)
    {
        SetParameter("persist.appspawn.reqMgr.timeout", "0"); // 设置超时时间为 "0"

        uint32_t def = 60;
        uint32_t timeout = GetDefaultTimeout(def);
        EXPECT_EQ(timeout, def); // 应该返回默认值 60
    }

    // 测试返回空字符串的情况
    TEST_F(GetDefaultTimeoutTest, TestEmptyTimeout)
    {
        SetParameter("persist.appspawn.reqMgr.timeout", ""); // 设置超时时间为空

        uint32_t def = 60;
        uint32_t timeout = GetDefaultTimeout(def);
        EXPECT_EQ(timeout, def); // 应该返回默认值 60
    }

    // 测试默认行为，没有调用 SetParameter 时
    TEST_F(GetDefaultTimeoutTest, TestNoTimeoutSet)
    {
        uint32_t def = 60;
        uint32_t timeout = GetDefaultTimeout(def);
        EXPECT_EQ(timeout, def); // 应该返回默认值 60，因为没有设置超时
    }

}