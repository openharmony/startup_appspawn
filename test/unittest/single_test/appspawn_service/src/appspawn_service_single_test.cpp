/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "appspawn_service_single_test.h"
#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/mman.h>
#include <cerrno>
#include "securec.h"
#include "appspawn_utils.h"

#define MEMSIZE 4096

using namespace testing;
using namespace testing::ext;

const static int CLIENT_ID = 10000;

#ifdef __cplusplus
extern "C" {
#endif


static void CreateTestFile(const char *fileName, const char *data)
{
    FILE *tmpFile = fopen(fileName, "wr");
    if (tmpFile != nullptr) {
        fprintf(tmpFile, "%s", data);
        (void)fflush(tmpFile);
        fclose(tmpFile);
    }
}


#ifdef __cplusplus
}
#endif

namespace OHOS {
class AppspawnServiceSingleTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
};

void AppspawnServiceSingleTest::SetUpTestCase()
{
    GTEST_LOG_(INFO) << "AppspawnServiceSingleTest SetUpTestCase";
}

void AppspawnServiceSingleTest::TearDownTestCase()
{
    GTEST_LOG_(INFO) << "AppspawnServiceSingleTest TearDownTestCase";
}

void AppspawnServiceSingleTest::SetUp()
{
    GTEST_LOG_(INFO) << "AppspawnServiceSingleTest SetUp";
}

void AppspawnServiceSingleTest::TearDown()
{
    GTEST_LOG_(INFO) << "AppspawnServiceSingleTest TearDown";
}


HWTEST_F(AppspawnServiceSingleTest, ClearMmap_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ClearMmap_001 start";
    char perforkFile[PATH_MAX] = {0};
    size_t bytes = snprintf_s(perforkFile, PATH_MAX, PATH_MAX - 1, APPSPAWN_MSG_DIR"appspawn/prefork_%d", CLIENT_ID);
    EXPECT_NE(bytes, 0);

    AppSpawnMgr *mgr = CreateAppSpawnMgr(0);
    EXPECT_TRUE(mgr != nullptr);
    MakeDirRec(APPSPAWN_MSG_DIR"appspawn", 0711, 1);

    CreateTestFile(perforkFile, "");
    int fd = open(perforkFile, O_RDONLY, S_IRWXU);

    EXPECT_TRUE(fd > 0);
    void *areaAddr = mmap(nullptr, MEMSIZE, PROT_READ, MAP_SHARED, fd, 0);
    EXPECT_TRUE(areaAddr != nullptr);
    mgr->content.propertyBuffer = (char *)areaAddr;
    ClearMMAP(CLIENT_ID, MEMSIZE);
    EXPECT_TRUE(mgr->content.propertyBuffer == nullptr);
    EXPECT_TRUE(access(perforkFile, F_OK) != 0);
    DeleteAppSpawnMgr(mgr);
    GTEST_LOG_(INFO) << "ClearMmap_001 end";
}

HWTEST_F(AppspawnServiceSingleTest, ClearMmap_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ClearMmap_002 start";
    char perforkFile[PATH_MAX] = {0};
    size_t bytes = snprintf_s(perforkFile, PATH_MAX, PATH_MAX - 1, APPSPAWN_MSG_DIR"appspawn/prefork_%d", CLIENT_ID);
    EXPECT_NE(bytes, 0);

    ClearMMAP(CLIENT_ID, MEMSIZE);
    EXPECT_NE(access(perforkFile, F_OK), 0);
    GTEST_LOG_(INFO) << "ClearMmap_002 end";
}

HWTEST_F(AppspawnServiceSingleTest, ClearPreforkInfo_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ClearPreforkInfo_001 start";
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    EXPECT_TRUE(ctx != nullptr);
    char perforkFile[PATH_MAX] = {0};
    size_t bytes = snprintf_s(perforkFile, PATH_MAX, PATH_MAX - 1, APPSPAWN_MSG_DIR"appspawn/prefork_%d", CLIENT_ID);
    EXPECT_NE(bytes, 0);
    MakeDirRec(APPSPAWN_MSG_DIR"appspawn", 0711, 1);

    CreateTestFile(perforkFile, "");
    int fd = open(perforkFile, O_RDONLY, S_IRWXU);
    EXPECT_TRUE(fd > 0);
    void *areaAddr = mmap(nullptr, MEMSIZE, PROT_READ, MAP_SHARED, fd, 0);
    EXPECT_TRUE(areaAddr != nullptr);
    ctx->forkCtx.childMsg = (char *)areaAddr;

    ctx->client.id = CLIENT_ID;
    ClearPreforkInfo(ctx);
    EXPECT_TRUE(ctx->forkCtx.childMsg == nullptr);
    DeleteAppSpawningCtx(ctx);
    GTEST_LOG_(INFO) << "ClearPreforkInfo_001 end";
}

HWTEST_F(AppspawnServiceSingleTest, ClearPreforkInfo_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << "ClearPreforkInfo_002 start";
    AppSpawningCtx *ctx = CreateAppSpawningCtx();
    EXPECT_TRUE(ctx != nullptr);
    ClearPreforkInfo(ctx);
    DeleteAppSpawningCtx(ctx);
    GTEST_LOG_(INFO) << "ClearPreforkInfo_002 end";
}
}