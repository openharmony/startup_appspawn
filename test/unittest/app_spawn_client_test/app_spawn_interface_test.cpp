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
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "appspawn.h"
#include "appspawn_client.h"
#include "appspawn_hook.h"
#include "appspawn_mount_permission.h"
#include "appspawn_service.h"
#include "appspawn_utils.h"
#include "securec.h"

#include "app_spawn_stub.h"

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
    AppSpawnClientHandle handle;
    const char *serviceName[] = {APPSPAWN_SERVER_NAME, APPSPAWN_SOCKET_NAME,
        NWEBSPAWN_SERVER_NAME, NWEBSPAWN_SOCKET_NAME,
        "test", "", nullptr};
    AppSpawnClientHandle *inputhandle[] = {&handle, nullptr};

    for (size_t i = 0; i < ARRAY_LENGTH(serviceName); i++) {
        for (size_t j = 0; j < ARRAY_LENGTH(inputhandle); j++) {
            printf("App_Spawn_Interface_Init_001 %zu %zu \n", i, j);
            int ret = AppSpawnClientInit(serviceName[i], inputhandle[j]);
            EXPECT_EQ(((i <= 5) && j == 0 && ret == 0) || (ret != 0), 1); // 3 valid
            if (ret == 0) {
                AppSpawnClientDestroy(handle);
            }
        }
    }
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
    AppSpawnClientHandle clientHandle = nullptr;
    ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    EXPECT_EQ(0, ret);
    AppSpawnClientHandle clientHandle1 = nullptr;
    ret = AppSpawnClientInit(NWEBSPAWN_SERVER_NAME, &clientHandle1);
    EXPECT_EQ(0, ret);

    AppSpawnResult result1 = {};
    AppSpawnClientHandle handle[] = {clientHandle, clientHandle1, nullptr};
    AppSpawnResult *result[] = {&result1, nullptr};

    for (size_t i = 0; i < ARRAY_LENGTH(handle); i++) {
        for (size_t k = 0; k < ARRAY_LENGTH(result); k++) {
            AppSpawnReqMsgHandle reqHandle1 = nullptr;
            ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle1);
            EXPECT_EQ(0, ret);

            printf("App_Spawn_Interface_Send_Msg_001 %zu %zu %d \n", i, k, ret);
            ret = AppSpawnClientSendMsg(handle[i], reqHandle1, result[k]);
            EXPECT_EQ((i <= 1 && k == 0 && ret == 0) || (ret != 0), 1);
        }
    }
    for (size_t i = 0; i < ARRAY_LENGTH(handle); i++) {
        for (size_t k = 0; k < ARRAY_LENGTH(result); k++) {
            ret = AppSpawnClientSendMsg(handle[i], nullptr, result[k]);
            EXPECT_EQ(ret != 0, 1);
        }
    }
    ret = AppSpawnClientDestroy(clientHandle);
    EXPECT_EQ(0, ret);
    ret = AppSpawnClientDestroy(clientHandle1);
    EXPECT_EQ(0, ret);
}

/**
 * @brief 测试接口：AppSpawnReqMsgCreate
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Msg_Create_001, TestSize.Level0)
{
    std::vector<char> name1(APP_LEN_PROC_NAME - 1, 'a');
    name1.push_back('\0');
    std::vector<char> name2(APP_LEN_PROC_NAME, 'b');
    name2.push_back('\0');
    std::vector<char> name3(APP_LEN_PROC_NAME + 1, 'c');
    name3.push_back('\0');

    AppSpawnMsgType msgType[] = {MSG_APP_SPAWN, MSG_GET_RENDER_TERMINATION_STATUS,
        MSG_SPAWN_NATIVE_PROCESS, MSG_DUMP, MAX_TYPE_INVALID};
    const char *processName[] = {"com.ohos.myapplication",
        "com.ohos.myapplication::@#$%^&*()test",
        name1.data(), name2.data(), name3.data(), nullptr, ""};

    AppSpawnReqMsgHandle reqHandle = nullptr;

    for (size_t i = 0; i < ARRAY_LENGTH(msgType); i++) {
        for (size_t j = 0; j < ARRAY_LENGTH(processName); j++) {
            int ret = AppSpawnReqMsgCreate(msgType[i], processName[j], &reqHandle);
            printf("App_Spawn_Interface_Msg_Create_001 %zu %zu \n", i, j);
            EXPECT_EQ(((i != 4) && (j < 3) && ret == 0) || (ret != 0), 1); // 4 3 valid index
            if (ret == 0) {
                AppSpawnReqMsgFree(reqHandle);
            }
        }
    }
    for (size_t i = 0; i < ARRAY_LENGTH(msgType); i++) {
        for (size_t j = 0; j < ARRAY_LENGTH(processName); j++) {
            int ret = AppSpawnReqMsgCreate(msgType[i], processName[j], nullptr);
            EXPECT_EQ(ret != 0, 1);
        }
    }
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

    std::vector<char> name1(APP_LEN_BUNDLE_NAME - 1, 'a');
    name1.push_back('\0');
    std::vector<char> name2(APP_LEN_BUNDLE_NAME, 'b');
    name2.push_back('\0');
    std::vector<char> name3(APP_LEN_BUNDLE_NAME + 1, 'c');
    name3.push_back('\0');

    const AppSpawnReqMsgHandle inputHandle[] = {reqHandle, nullptr};
    const uint32_t bundleIndex[] = {0, 1};
    const char *bundleName[] = {"com.ohos.myapplication", name1.data(), name2.data(), name3.data(), nullptr};
    for (size_t i = 0; i < ARRAY_LENGTH(inputHandle); i++) {
        for (size_t j = 0; j < ARRAY_LENGTH(bundleIndex); j++) {
            for (size_t k = 0; k < ARRAY_LENGTH(bundleName); k++) {
                printf("App_Spawn_Interface_ReqMsgSetBundleInfo_001 %zu %zu %zu %d \n", i, j, k, ret);
                ret = AppSpawnReqMsgSetBundleInfo(inputHandle[i], bundleIndex[j], bundleName[k]);
                EXPECT_EQ((i == 0 && (k <= 2) && ret == 0) || (ret != 0), 1); // 2 valid index
            }
        }
    }
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
    const AppSpawnReqMsgHandle inputHandle[] = {reqHandle, nullptr};
    const AppDacInfo *dacInfo1[] = {&dacInfo, nullptr};

    for (size_t i = 0; i < ARRAY_LENGTH(inputHandle); i++) {
        for (size_t j = 0; j < ARRAY_LENGTH(dacInfo1); j++) {
            printf("App_Spawn_Interface_Msg_App_Dac_001 %zu %zu %d \n", i, j, ret);
            ret = AppSpawnReqMsgSetAppDacInfo(inputHandle[i], dacInfo1[j]);
            EXPECT_EQ((i == 0 && j == 0 && ret == 0) || (ret != 0), 1);
        }
    }
    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief 测试接口：AppSpawnReqMsgSetAppDomainInfo
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_Set_App_Domain_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = nullptr;
    char buffer[33];
    (void)memset_s(buffer, sizeof(buffer), 'A', sizeof(buffer));
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.ohos.myapplication", &reqHandle);
    EXPECT_EQ(0, ret);

    std::vector<char> name1(APP_APL_MAX_LEN - 1, 'a');
    name1.push_back('\0');
    std::vector<char> name2(APP_APL_MAX_LEN, 'b');
    name2.push_back('\0');
    std::vector<char> name3(APP_APL_MAX_LEN + 1, 'c');
    name3.push_back('\0');

    const AppSpawnReqMsgHandle inputHandle[] = {reqHandle, nullptr};
    const uint32_t hapFlags[] = {1};
    const char *apl[] = {"system_core", name1.data(), name2.data(), name3.data(), "", nullptr};

    for (size_t i = 0; i < ARRAY_LENGTH(inputHandle); i++) {
        for (size_t j = 0; j < ARRAY_LENGTH(hapFlags); j++) {
            for (size_t k = 0; k < ARRAY_LENGTH(apl); k++) {
                printf("App_Spawn_Interface_Set_App_Domain_001 %zu %zu %zu %d \n", i, j, k, ret);
                ret = AppSpawnReqMsgSetAppDomainInfo(inputHandle[i], hapFlags[j], apl[k]);
                EXPECT_EQ((i == 0 && k <= 2 && ret == 0) || (ret != 0), 1); // 2 valid index
            }
        }
    }
    AppSpawnReqMsgFree(reqHandle);
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
    AppSpawnReqMsgFree(reqHandle);
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
    AppSpawnReqMsgFree(reqHandle);
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

    std::vector<char> name1(APP_OWNER_ID_LEN - 1, 'a');
    name1.push_back('\0');
    std::vector<char> name2(APP_OWNER_ID_LEN, 'b');
    name2.push_back('\0');
    std::vector<char> name3(APP_OWNER_ID_LEN + 1, 'c');
    name3.push_back('\0');

    const AppSpawnReqMsgHandle inputHandle[] = {reqHandle, nullptr};
    const char *ownerId[] = {"FILE_ACCESS_MANAGER", name1.data(), name2.data(), name3.data(), "", nullptr};
    for (size_t i = 0; i < ARRAY_LENGTH(inputHandle); i++) {
        for (size_t j = 0; j < ARRAY_LENGTH(ownerId); j++) {
            printf("App_Spawn_Interface_Set_App_Owner_001 %zu %zu %d \n", i, j, ret);
            ret = AppSpawnReqMsgSetAppOwnerId(inputHandle[i], ownerId[j]);
            EXPECT_EQ((i == 0 && j <= 1 && ret == 0) || (ret != 0), 1);
        }
    }
    AppSpawnReqMsgFree(reqHandle);
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

    const AppSpawnReqMsgHandle inputHandle[] = {reqHandle, nullptr};
    const char *permission[] = {"ohos.permission.READ_IMAGEVIDEO", "", nullptr};
    for (size_t i = 0; i < ARRAY_LENGTH(inputHandle); i++) {
        for (size_t j = 0; j < ARRAY_LENGTH(permission); j++) {
            printf("App_Spawn_Interface_Add_Permission_001 %zu %zu %d \n", i, j, ret);
            ret = AppSpawnReqMsgAddPermission(inputHandle[i], permission[j]);
            EXPECT_EQ((i == 0 && j == 0 && ret == 0) || (ret != 0), 1);
        }
    }
    AppSpawnReqMsgFree(reqHandle);
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
    const char *inputName[] = {"tlv-name-1",
        "234567890123456789012345678901",
        "2345678901234567890123456789012",
        "23456789012345678901234567890123",
        "", nullptr};
    std::vector<uint8_t> data(EXTRAINFO_TOTAL_LENGTH_MAX + 1, static_cast<uint8_t>('c'));

    const uint8_t *inputData[] = {data.data(), data.data(), data.data(), data.data(), nullptr};
    const size_t inputvalueLen[] = {23,
        EXTRAINFO_TOTAL_LENGTH_MAX - 1,
        EXTRAINFO_TOTAL_LENGTH_MAX,
        EXTRAINFO_TOTAL_LENGTH_MAX + 1,
        0};

    for (size_t j = 0; j < ARRAY_LENGTH(inputName); j++) {
        for (size_t k = 0; k < ARRAY_LENGTH(inputData); k++) {
            for (size_t l = 0; l < ARRAY_LENGTH(inputvalueLen); l++) {
                ret = AppSpawnReqMsgAddExtInfo(reqHandle,
                    inputName[j], inputData[k], static_cast<uint32_t>(inputvalueLen[l]));
                printf("App_Spawn_Interface_Add_Ext_Info_001 %zu %zu %zu %d \n", j, k, l, ret);
                EXPECT_EQ((j <= 2 && k <= 4 && l <= 2 && ret == 0) || (ret != 0), 1); // 2 4 valid index
            }
        }
    }
    for (size_t j = 0; j < ARRAY_LENGTH(inputName); j++) {
        for (size_t k = 0; k < ARRAY_LENGTH(inputData); k++) {
            for (size_t l = 0; l < ARRAY_LENGTH(inputvalueLen); l++) {
                ret = AppSpawnReqMsgAddExtInfo(nullptr,
                    inputName[j], inputData[k], static_cast<uint32_t>(inputvalueLen[l]));
                EXPECT_EQ(ret != 0, 1);
            }
        }
    }
    AppSpawnReqMsgFree(reqHandle);
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

    std::vector<char> name1(23, 'a'); // 23 test data
    name1.push_back('\0');
    std::vector<char> name2(EXTRAINFO_TOTAL_LENGTH_MAX - 1, 'b');
    name2.push_back('\0');
    std::vector<char> name3(EXTRAINFO_TOTAL_LENGTH_MAX, 'c');
    name3.push_back('\0');
    std::vector<char> name4(EXTRAINFO_TOTAL_LENGTH_MAX + 1, 'd');
    name4.push_back('\0');

    const char *inputName[] = {"tlv-name-1",
        "234567890123456789012345678901",
        "2345678901234567890123456789012",
        "23456789012345678901234567890123",
        "", nullptr};
    const char *inputData[] = {name1.data(), name2.data(), name3.data(), name4.data(), nullptr};

    for (size_t j = 0; j < ARRAY_LENGTH(inputName); j++) {
        for (size_t k = 0; k < ARRAY_LENGTH(inputData); k++) {
            printf("App_Spawn_Interface_Add_String_Info_001 %zu %zu %d \n", j, k, ret);
            ret = AppSpawnReqMsgAddStringInfo(reqHandle, inputName[j], inputData[k]);
            EXPECT_EQ((j <= 2 && k <= 1 && ret == 0) || (ret != 0), 1);
        }
    }

    for (size_t j = 0; j < ARRAY_LENGTH(inputName); j++) {
        for (size_t k = 0; k < ARRAY_LENGTH(inputData); k++) {
            ret = AppSpawnReqMsgAddStringInfo(nullptr, inputName[j], inputData[k]);
            EXPECT_EQ(ret != 0, 1);
        }
    }
    AppSpawnReqMsgFree(reqHandle);
}

/**
 * @brief 测试flags set
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Set_Msg_Flags_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = 0;
    int ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.example.myapplication", &reqHandle);
    EXPECT_EQ(ret, 0);

    for (int32_t i = 0; i <= MAX_FLAGS_INDEX; i++) {
        ret = AppSpawnReqMsgSetAppFlag(reqHandle, static_cast<AppFlagsIndex>(i));
        printf(" App_Spawn_Set_Msg_Flags_001 %d %d \n", i, ret);
        EXPECT_EQ((i != MAX_FLAGS_INDEX && ret == 0) || (ret != 0), 1);
    }
    AppSpawnReqMsgFree(reqHandle);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_TerminateMsg_001, TestSize.Level0)
{
    AppSpawnReqMsgHandle reqHandle = 0;
    int ret = AppSpawnTerminateMsgCreate(0, &reqHandle);
    EXPECT_EQ(ret, 0);
    AppSpawnReqMsgFree(reqHandle);

    ret = AppSpawnTerminateMsgCreate(0, nullptr);
    EXPECT_EQ(ret, APPSPAWN_ARG_INVALID);
}

/**
 * @brief 测试add permission
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Permission_001, TestSize.Level0)
{
    AppSpawnClientHandle clientHandle;
    AppSpawnReqMsgHandle reqHandle = 0;
    int ret = AppSpawnClientInit(APPSPAWN_SERVER_NAME, &clientHandle);
    EXPECT_EQ(ret, 0);

    ret = AppSpawnReqMsgCreate(MSG_APP_SPAWN, "com.example.myapplication", &reqHandle);
    EXPECT_EQ(ret, 0);

    const int32_t inputCount = 2;
    const char *permissions[] = {"ohos.permission.ACCESS_BUNDLE_DIR", "", "3333333333", nullptr};
    const AppSpawnClientHandle inputClientHandle[inputCount] = {clientHandle, nullptr};
    const AppSpawnReqMsgHandle inputReqHandle[inputCount] = {reqHandle, nullptr};
    for (int32_t i = 0; i < inputCount; i++) {
        for (int32_t j = 0; j < inputCount; j++) {
            for (int32_t k = 0; k < 4; k++) { // 4 sizeof permissions
                ret = AppSpawnClientAddPermission(inputClientHandle[i], inputReqHandle[j], permissions[k]);
                printf(" AppSpawnClientAddPermission %d %d %d %d \n", i, j, k, ret);
                EXPECT_EQ(i == 0 && j == 0 && k == 0 && ret == 0 || ret != 0, 1);
            }
        }
    }
    AppSpawnReqMsgFree(reqHandle);
    AppSpawnClientDestroy(clientHandle);
}

/**
 * @brief 测试接口：GetPermissionByIndex
 *
 */
HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_GetPermissionByIndex_001, TestSize.Level0)
{
    int32_t maxIndex = GetMaxPermissionIndex(nullptr);
    for (int i = 0; i < maxIndex; i++) {
        EXPECT_NE(GetPermissionByIndex(nullptr, i), nullptr);
    }

    EXPECT_EQ(GetPermissionByIndex(nullptr, maxIndex), nullptr);
    EXPECT_EQ(GetPermissionByIndex(nullptr, -1), nullptr);

    int32_t index = GetPermissionIndex(nullptr, "ohos.permission.ACCESS_BUNDLE_DIR");
    int ret = index >= 0 && index < maxIndex ? 0 : -1;
    EXPECT_EQ(0, ret);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_ClientSocket_001, TestSize.Level0)
{
    int socketId = CreateClientSocket(CLIENT_FOR_APPSPAWN, 2);
    CloseClientSocket(socketId);
    socketId = CreateClientSocket(CLIENT_FOR_NWEBSPAWN, 2);
    CloseClientSocket(socketId);
    socketId = CreateClientSocket(CLIENT_MAX, 2);
    CloseClientSocket(socketId);
    CloseClientSocket(-1);
}

HWTEST(AppSpawnInterfaceTest, App_Spawn_Interface_GetSpawnTimeout_001, TestSize.Level0)
{
    int timeout = GetSpawnTimeout(6);  // 6 test
    EXPECT_EQ(timeout >= 6, 1);        // 6 test
    timeout = GetSpawnTimeout(6);      // 6 test
    EXPECT_EQ(timeout >= 6, 1);        // 6 test
    timeout = GetSpawnTimeout(6);      // 6 test
    EXPECT_EQ(timeout >= 6, 1);        // 6 test
    timeout = GetSpawnTimeout(2);      // 2 test
    EXPECT_EQ(timeout >= 2, 1);        // 2 test
    timeout = GetSpawnTimeout(2);      // 2 test
    EXPECT_EQ(timeout >= 2, 1);        // 2 test
    timeout = GetSpawnTimeout(2);      // 2 test
    EXPECT_EQ(timeout >= 2, 1);        // 2 test
}
}  // namespace OHOS
