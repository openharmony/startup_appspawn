/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef APPSPAWN_CLIENT_TEST_H
#define APPSPAWN_CLIENT_TEST_H

#include <sstream>
#include <fstream>

#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <vector>

#include "client_socket.h"
#include "appspawn_server.h"
#include "parameter.h"

namespace OHOS {
namespace AppSpawn {

class AppSpawnTestClient {
public:
    AppSpawnTestClient() = default;
    ~AppSpawnTestClient() = default;

    int ClientCreateSocket(std::string path = "/dev/unix/socket/NWebSpawn")
    {
        // close old socket and create new
        if (clientSocket_ != nullptr) {
            clientSocket_->CloseClient();
            clientSocket_ = nullptr;
        }
        clientSocket_ = std::make_shared<ClientSocket>(path);
        APPSPAWN_CHECK(clientSocket_ != nullptr, return -1, "Failed to create client for path %s", path.c_str());
        int ret = clientSocket_->CreateClient();
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to create socket for path %ss", path.c_str());
        ret = clientSocket_->ConnectSocket();
        APPSPAWN_CHECK(ret == 0, return -1, "Failed to connect to server %ss", path.c_str());
        return 0;
    }

    void ClientClose()
    {
        if (clientSocket_ != nullptr) {
            clientSocket_->CloseClient();
            clientSocket_ = nullptr;
        }
    }
    int ClientSendMsg(const uint8_t *buf, int len)
    {
        int curr = 0;
        int real = 0;
        uint8_t *buffer = const_cast<uint8_t *>(buf);
        const int maxLen = 5 * 1024;
        while (curr < len) {
            if ((len - curr) > maxLen) {
                real = maxLen;
            } else {
                real = len - curr;
            }
            if (clientSocket_->WriteSocketMessage(static_cast<const void *>(buffer + curr), real) != real) {
                return -1;
            }
            curr += real;
        }
        return 0;
    }

    int ClientRecvMsg(pid_t &pid)
    {
        pid = -1;
        std::vector<uint8_t> data(sizeof(pid_t)); // 4 pid size
        if (clientSocket_->ReadSocketMessage(data.data(), data.size()) == sizeof(pid_t)) {
            int ret = *(reinterpret_cast<int *>(data.data()));
            if (ret < 0) {
                return ret;
            }
            pid = static_cast<pid_t>(ret);
            return 0;
        }
        return -1;
    }

    void ClientFillMsg(AppParameter *request, const std::string &processName, const std::string &cmd)
    {
        request->code = DEFAULT;
        request->flags = 0;
        request->uid = 20010033; // 20010033 test uid
        request->gid = 20010033; // 20010033 test gid
        request->gidCount = 0;
        request->accessTokenId = 0x200a509d; // 0x200a509d test token id
        request->accessTokenIdEx = 0x4832514205; // 0x4832514205 test token id
        request->allowInternet = 1;
        request->hspList.totalLength = 0;
        request->hspList.savedLength = 0;
        request->hspList.data = nullptr;
        int ret = strcpy_s(request->apl, sizeof(request->processName), processName.c_str());
        ret += strcpy_s(request->processName, sizeof(request->processName), processName.c_str());
        ret += strcpy_s(request->bundleName, sizeof(request->bundleName), processName.c_str());
        ret += strcpy_s(request->renderCmd, sizeof(request->renderCmd), cmd.c_str());
        if (ret != 0) {
            printf("Failed to copy bundle name \n");
        }
    }
private:
    std::shared_ptr<ClientSocket> clientSocket_ {};
};

}  // namespace AppSpawn
}  // namespace OHOS
#endif // APPSPAWN_CLIENT_TEST_H