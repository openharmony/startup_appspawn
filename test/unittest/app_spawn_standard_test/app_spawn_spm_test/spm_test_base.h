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

#ifndef SPM_TEST_BASE_H
#define SPM_TEST_BASE_H

#include <gtest/gtest.h>

// Include appspawn headers FIRST to get type definitions
#include "appspawn_manager.h"
#include "appspawn_utils.h"

/**
 * @brief SPM 测试基类
 *
 * 提供通用的测试环境设置和清理
 */
class SpmTestBase : public testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    // 辅助方法
    virtual void SetupMockEnvironment();
    virtual void CleanupMockEnvironment();
    virtual void InitDefaultExpectations();
};

#endif  // SPM_TEST_BASE_H
