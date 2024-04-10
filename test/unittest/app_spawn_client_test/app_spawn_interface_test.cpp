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

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <iostream>

#include "appspawn.h"
#include "appspawn_hook.h"
#include "appspawn_mount_permission.h"
#include "appspawn_service.h"
#include "appspawn_utils.h"
#include "securec.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS {
class AppSpawnInterfaceTest : public testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

/**
 * @brief 测试接口：AppSpawnClientInit
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Init_001, TestSize.Level0)
{
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, nullptr);
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Init_002, TestSize.Level0)
{
    int ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, nullptr);
    EXPECT_EQ(APPSPAWN_ARG_INVALID, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Init_003, TestSize.Level0)
{
    int ret = AppSpawnClientInit("test", nullptr);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Init_004, TestSize.Level0)
{
    int ret = AppSpawnClientInit(nullptr, nullptr);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Init_005, TestSize.Level0)
{
    AppSpawnClientHandle handle;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &handle);
    EXPECT_EQ(0, ret);
}

/**
 * @brief 测试接口：AppSpawnClientDestroy
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Destroy_001, TestSize.Level0)
{
    AppSpawnClientHandle handle;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &handle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnClientDestroy(handle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Destroy_002, TestSize.Level0)
{
    AppSpawnClientHandle handle;
    int ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &handle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnClientDestroy(handle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Destroy_003, TestSize.Level0)
{
    int ret = AppSpawnClientDestroy(nullptr);
    EXPECT_EQ(APPSPAWN_SYSTEM_ERROR, ret);
}

/**
 * @brief 测试接口：AppSpawnClientSendMsg
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Send_Msg_001, TestSize.Level0)
{
    int ret = 0;
    AppSpawnReqMsgHandle reqHandle = nullptr;
    ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    AppSpawnResult result = {};
    ret = AppSpawnClientSendMsg(nullptr, reqHandle, &result);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Send_Msg_002, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    EXPECT_EQ(0, ret);
    AppSpawnResult result = {};
    ret = AppSpawnClientSendMsg(clientHandle, nullptr, &result);
    EXPECT_NE(0, ret);
    AppSpawnClientDestroy(clientHandle);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Send_Msg_003, TestSize.Level0)
{
    int ret = 0;
    AppSpawnClientHandle clientHandle = nullptr;
    ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgHandle reqHandle = nullptr;
    ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    ret = AppSpawnClientSendMsg(clientHandle, reqHandle, nullptr);
    EXPECT_NE(0, ret);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试接口：AppSpawnReqMsgCreate
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_002, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_GET_RENDER_TERMINATION_STATUS, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_003, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_SPAWN_NATIVE_PROCESS, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_004, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_DUMP, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_005, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MAX_TYPE_INVALID, "com.ohos.myapplication", &reqHandle);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_006, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication::@#$%^&*()test", &reqHandle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_008, TestSize.Level0)
{
    char buffer[255];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, buffer, &reqHandle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_009, TestSize.Level0)
{
    char buffer[257];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, buffer, &reqHandle);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_010, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, nullptr, &reqHandle);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_011, TestSize.Level0)
{
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", nullptr);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_012, TestSize.Level0)
{
    const char *testMsg = "testMsg";
    char *p = const_cast<char *>(testMsg);
    AppSpawnReqMsgHandle handle = reinterpret_cast<AppSpawnReqMsgHandle>(p);
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &handle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_013, TestSize.Level0)
{
    char buffer[128];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, buffer, &reqHandle);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_014, TestSize.Level0)
{
    char buffer[129];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, buffer, &reqHandle);
    EXPECT_EQ(0, ret);
}
/**
 * @brief 测试接口：AppSpawnReqMsgFree
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Free_001, TestSize.Level0)
{
    AppSpawnReqMsgFree(nullptr);
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief 测试接口：AppSpawnReqMsgSetBundleInfo
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_ReqMsgSetBundleInfo_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, "com.ohos.myapplication");
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgFree(reqHandle);
}


HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_ReqMsgSetBundleInfo_002, TestSize.Level0)
{
    int ret = AppSpawnReqMsgSetBundleInfo(nullptr, 0, "com.ohos.myapplication");
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_ReqMsgSetBundleInfo_005, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 1, "com.ohos.myapplication");
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgFree(reqHandle);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_ReqMsgSetBundleInfo_006, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, -1, "com.ohos.myapplication");
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgFree(reqHandle);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_ReqMsgSetBundleInfo_007, TestSize.Level0)
{
    char buffer[255];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, buffer);
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgFree(reqHandle);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_ReqMsgSetBundleInfo_008, TestSize.Level0)
{
    char buffer[257];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, buffer);
    EXPECT_NE(0, ret);
    AppSpawnReqMsgFree(reqHandle);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_ReqMsgSetBundleInfo_009, TestSize.Level0)
{
    char buffer[128];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, buffer);
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgFree(reqHandle);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_ReqMsgSetBundleInfo_010, TestSize.Level0)
{
    char buffer[129];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetBundleInfo(reqHandle, 0, buffer);
    EXPECT_EQ(0, ret);
    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief 测试接口：AppSpawnReqMsgSetAppDacInfo
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_App_Dac_001, TestSize.Level0)
{
    AppDacInfo dacInfo;
    dacInfo.uid = 20010029;              // 20010029 test data
    dacInfo.gid = 20010029;              // 20010029 test data
    dacInfo.gidCount = 2;                // 2 count
    dacInfo.gidTable[0] = 20010029;      // 20010029 test data
    dacInfo.gidTable[1] = 20010029 + 1;  // 20010029 test data
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetAppDacInfo(reqHandle, &dacInfo);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_App_Dac_002, TestSize.Level0)
{
    AppDacInfo dacInfo;
    dacInfo.uid = 20010029;              // 20010029 test data
    dacInfo.gid = 20010029;              // 20010029 test data
    dacInfo.gidCount = 2;                // 2 count
    dacInfo.gidTable[0] = 20010029;      // 20010029 test data
    dacInfo.gidTable[1] = 20010029 + 1;  // 20010029 test data
    int ret = AppSpawnReqMsgSetAppDacInfo(nullptr, &dacInfo);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_App_Dac_004, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetAppDacInfo(reqHandle, nullptr);
    EXPECT_NE(0, ret);
}

/**
 * @brief 测试接口：AppSpawnReqMsgSetAppDomainInfo
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Domain_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetAppDomainInfo(reqHandle, 1, "system_core");
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Domain_002, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgSetAppDomainInfo(reqHandle, 1, "system_core");
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Domain_004, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetAppDomainInfo(reqHandle, 1, nullptr);
    EXPECT_NE(0, ret);
}

/**
 * @brief 测试接口：AppSpawnReqMsgSetAppInternetPermissionInfo
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Permission_Info_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetAppInternetPermissionInfo(reqHandle, 102, 102);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Permission_Info_002, TestSize.Level0)
{
    int ret = AppSpawnReqMsgSetAppInternetPermissionInfo(nullptr, 102, 102);
    EXPECT_NE(0, ret);
}

/**
 * @brief 测试接口：AppSpawnReqMsgSetAppAccessToken
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Access_Token_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetAppAccessToken(reqHandle, 12345678);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Access_Token_002, TestSize.Level0)
{
    int ret = AppSpawnReqMsgSetAppAccessToken(nullptr, 12345678);
    EXPECT_NE(0, ret);
}

/**
 * @brief 测试接口：AppSpawnReqMsgSetAppOwnerId
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Owner_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetAppOwnerId(reqHandle, "ohos.permission.FILE_ACCESS_MANAGER");
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Owner_002, TestSize.Level0)
{
    int ret = AppSpawnReqMsgSetAppOwnerId(nullptr, "ohos.permission.FILE_ACCESS_MANAGER");
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Owner_004, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgSetAppOwnerId(reqHandle, nullptr);
    EXPECT_NE(0, ret);
}

/**
 * @brief 测试接口：AppSpawnReqMsgAddPermission
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_Permission_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgAddPermission(reqHandle, "ohos.permission.READ_IMAGEVIDEO");
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_Permission_002, TestSize.Level0)
{
    int ret = AppSpawnReqMsgAddPermission(nullptr, "ohos.permission.READ_IMAGEVIDEO");
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_Permission_004, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgAddPermission(reqHandle, nullptr);
    EXPECT_NE(0, ret);
}

/**
 * @brief 测试接口：AppSpawnReqMsgAddExtInfo
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_Ext_Info_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    const char *testData = "ssssssssssssss sssssssss ssssssss";
    ret = AppSpawnReqMsgAddExtInfo(reqHandle, "tlv-name-1",
        reinterpret_cast<uint8_t *>(const_cast<char *>(testData)), strlen(testData));
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_Ext_Info_002, TestSize.Level0)
{
    const char *testData = "ssssssssssssss sssssssss ssssssss";
    int ret = AppSpawnReqMsgAddExtInfo(nullptr, "tlv-name-1",
        reinterpret_cast<uint8_t *>(const_cast<char *>(testData)), strlen(testData));
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_Ext_Info_004, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    const char *testData = "ssssssssssssss sssssssss ssssssss";
    ret = AppSpawnReqMsgAddExtInfo(reqHandle, nullptr,
        reinterpret_cast<uint8_t *>(const_cast<char *>(testData)), strlen(testData));
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_Ext_Info_005, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    const char *testData = "ssssssssssssss sssssssss ssssssss";
    ret = AppSpawnReqMsgAddExtInfo(reqHandle, "tlv-name-1", nullptr, strlen(testData));
    EXPECT_NE(0, ret);
}

/**
 * @brief 测试接口：AppSpawnReqMsgAddStringInfo
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_String_Info_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    const char *testData = "ssssssssssssss sssssssss ssssssss";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, "test", testData);
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_String_Info_002, TestSize.Level0)
{
    const char *testData = "ssssssssssssss sssssssss ssssssss";
    int ret = AppSpawnReqMsgAddStringInfo(nullptr, "test", testData);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_String_Info_004, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    const char *testData = "ssssssssssssss sssssssss ssssssss";
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, nullptr, testData);
    EXPECT_NE(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Add_String_Info_005, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnReqMsgAddStringInfo(reqHandle, "test", nullptr);
    EXPECT_NE(0, ret);
}

/**
 * @brief 测试接口：GetPermissionByIndex
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_GetPermissionByIndex_005, TestSize.Level0)
{
    int32_t maxIndex = GetMaxPermissionIndex();
    for (int i = 0; i < maxIndex; i++) {
        EXPECT_NE(GetPermissionByIndex(i), nullptr);
    }

    EXPECT_EQ(GetPermissionByIndex(maxIndex), nullptr);
    EXPECT_EQ(GetPermissionByIndex(-1), nullptr);

    int32_t index = GetPermissionIndex("ohos.permission.ACCESS_BUNDLE_DIR");
    int ret = index >= 0 && index < maxIndex ? 0 : -1;
    EXPECT_EQ(0, ret);
}
}
