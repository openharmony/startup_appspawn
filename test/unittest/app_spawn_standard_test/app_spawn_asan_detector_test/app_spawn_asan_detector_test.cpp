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

#include "appspawn_modulemgr.h"
#include "appspawn_server.h"
#include "appspawn_manager.h"
#include "appspawn.h"

#ifdef __cplusplus
extern "C" {
#endif

APPSPAWN_STATIC bool RegisterFfrtStackFunc();

#ifdef __cplusplus
}
#endif

using namespace testing;
using namespace testing::ext;
using namespace OHOS;

namespace {
using FfrtCoroutineStackFn = bool (*)(void **stackAddr, size_t *size);
using RegisterFfrtStackFuncFn = void (*)(FfrtCoroutineStackFn fn);

constexpr const char *REGISTER_FUNC_SYMBOL = "libc_gwp_asan_register_ffrt_stack_func";
constexpr const char *FFRT_STACK_FUNC_SYMBOL = "ffrt_get_current_coroutine_stack";

void *g_mockRegisterFuncSymbol = nullptr;
void *g_mockFfrtFuncSymbol = nullptr;

int g_dlsymRegisterFuncCount = 0;
int g_dlsymFfrtFuncCount = 0;
int g_registerCalledCount = 0;

FfrtCoroutineStackFn g_registeredFfrtFunc = nullptr;

void ResetRegisterFfrtStackFuncMock()
{
    g_mockRegisterFuncSymbol = nullptr;
    g_mockFfrtFuncSymbol = nullptr;

    g_dlsymRegisterFuncCount = 0;
    g_dlsymFfrtFuncCount = 0;
    g_registerCalledCount = 0;

    g_registeredFfrtFunc = nullptr;
}

bool MockFfrtGetCurrentCoroutineStack(void **stackAddr, size_t *size)
{
    if (stackAddr != nullptr) {
        *stackAddr = nullptr;
    }

    if (size != nullptr) {
        *size = 0;
    }

    return true;
}

void MockLibcGwpAsanRegisterFfrtStackFunc(FfrtCoroutineStackFn fn)
{
    g_registerCalledCount++;
    g_registeredFfrtFunc = fn;
}
} // namespace

extern "C" void *__wrap_dlsym(void *handle, const char *symbol)
{
    (void)handle;

    if (symbol == nullptr) {
        return nullptr;
    }

    if (strcmp(symbol, REGISTER_FUNC_SYMBOL) == 0) {
        g_dlsymRegisterFuncCount++;
        return g_mockRegisterFuncSymbol;
    }

    if (strcmp(symbol, FFRT_STACK_FUNC_SYMBOL) == 0) {
        g_dlsymFfrtFuncCount++;
        return g_mockFfrtFuncSymbol;
    }

    return nullptr;
}

namespace OHOS {
class AppSpawnAsanDetectorTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp()
    {
        ResetRegisterFfrtStackFuncMock();
    }
    void TearDown()
    {
        ResetRegisterFfrtStackFuncMock();
    }
};

/**
 * @tc.name: RegisterFfrtStackFunc01
 * @tc.desc: libc_gwp_asan_register_ffrt_stack_func symbol not found
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnAsanDetectorTest, RegisterFfrtStackFunc01, TestSize.Level1)
{
    g_mockRegisterFuncSymbol = nullptr;
    g_mockFfrtFuncSymbol = reinterpret_cast<void *>(MockFfrtGetCurrentCoroutineStack);

    bool ret = RegisterFfrtStackFunc();

    EXPECT_FALSE(ret);
    EXPECT_EQ(g_dlsymRegisterFuncCount, 1);
    EXPECT_EQ(g_dlsymFfrtFuncCount, 0);
    EXPECT_EQ(g_registerCalledCount, 0);
    EXPECT_EQ(g_registeredFfrtFunc, nullptr);
}

/**
 * @tc.name: RegisterFfrtStackFunc02
 * @tc.desc: ffrt_get_current_coroutine_stack symbol not found
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnAsanDetectorTest, RegisterFfrtStackFunc02, TestSize.Level1)
{
    g_mockRegisterFuncSymbol = reinterpret_cast<void *>(MockLibcGwpAsanRegisterFfrtStackFunc);
    g_mockFfrtFuncSymbol = nullptr;

    bool ret = RegisterFfrtStackFunc();

    EXPECT_FALSE(ret);
    EXPECT_EQ(g_dlsymRegisterFuncCount, 1);
    EXPECT_EQ(g_dlsymFfrtFuncCount, 1);
    EXPECT_EQ(g_registerCalledCount, 0);
    EXPECT_EQ(g_registeredFfrtFunc, nullptr);
}

/**
 * @tc.name: RegisterFfrtStackFunc03
 * @tc.desc: register ffrt_get_current_coroutine_stack successfully
 * @tc.type: FUNC
 */
HWTEST_F(AppSpawnAsanDetectorTest, RegisterFfrtStackFunc03, TestSize.Level1)
{
    g_mockRegisterFuncSymbol = reinterpret_cast<void *>(MockLibcGwpAsanRegisterFfrtStackFunc);
    g_mockFfrtFuncSymbol = reinterpret_cast<void *>(MockFfrtGetCurrentCoroutineStack);

    bool ret = RegisterFfrtStackFunc();

    EXPECT_TRUE(ret);
    EXPECT_EQ(g_dlsymRegisterFuncCount, 1);
    EXPECT_EQ(g_dlsymFfrtFuncCount, 1);
    EXPECT_EQ(g_registerCalledCount, 1);
    EXPECT_EQ(g_registeredFfrtFunc, MockFfrtGetCurrentCoroutineStack);
}
}   // namespace OHOS
