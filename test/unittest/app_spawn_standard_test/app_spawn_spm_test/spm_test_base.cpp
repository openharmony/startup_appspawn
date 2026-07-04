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

#include "spm_test_base.h"
#include "appspawn_utils.h"
#include "spm_func_mock.h"

void SpmTestBase::SetUp()
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();
    APPSPAWN_LOGI("%s.%s start", info->test_suite_name(), info->name());
    GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " start";

    SetupMockEnvironment();
    InitDefaultExpectations();
}

void SpmTestBase::TearDown()
{
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();
    APPSPAWN_LOGI("%s.%s end", info->test_suite_name(), info->name());
    GTEST_LOG_(INFO) << info->test_suite_name() << "." << info->name() << " end";

    CleanupMockEnvironment();
}

void SpmTestBase::SetupMockEnvironment()
{
    // 基类默认实现，子类可以重写
}

void SpmTestBase::CleanupMockEnvironment()
{
    // 基类默认实现，子类可以重写
    // 清理 Mock 接口：将静态指针重置为 nullptr
    OHOS::AppSpawn::SpmFuncMock::spmFuncMock_.reset();
}

void SpmTestBase::InitDefaultExpectations()
{
    // 基类默认实现，子类可以重写以设置默认的 Mock 期望
}
