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

#include "appspawn_test_client.h"

#include <signal.h>
#include <unistd.h>

OHOS::AppSpawn::AppSpawnTestClient g_testClient;

static int DoRun(const char *server, const AppParameter *request)
{
    int ret = g_testClient.ClientCreateSocket(server);
    if (ret != 0) {
        printf("Failed to connect server \n");
        return 1;
    }
    ret = g_testClient.ClientSendMsg(reinterpret_cast<const uint8_t *>(request), sizeof(AppParameter));
    if (ret != 0) {
        printf("Failed to send msg to server \n");
        g_testClient.ClientClose();
        return 1;
    }
    pid_t pid = 0;
    ret = g_testClient.ClientRecvMsg(pid);
    if (ret != 0) {
        printf("Failed spawn new app result %d \n", ret);
        g_testClient.ClientClose();
        quick_exit(0);
        return 1;
    }
    if (pid > 0) {
        printf("Success spawn new app pid %d \n", pid);
    }
    int index = 0;
    while (index < 5) { // wait 5s
        sleep(1);
        index++;
    }
    if (pid > 0) {
        APPSPAWN_LOGI("Success spawn new app pid %{public}d \n", pid);
        kill(pid, SIGKILL);
    }
    // close client
    g_testClient.ClientClose();
    quick_exit(0);
    return 0;
}

int main(int argc, char *const argv[])
{
    int coldStart = 0;
    int withSandbox = 0;
    const char *cmd = "ls -l /data > /data/test.log";
    const char *bundleName = "ohos.samples.test";
    const char *server = "/dev/unix/socket/AppSpawn";
    for (int32_t i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            coldStart = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            withSandbox = 1;
        } else if (strcmp(argv[i], "--nwebspawn") == 0) {
            server = "/dev/unix/socket/NWebSpawn";
        } else if (strcmp(argv[i], "-b") == 0 && ((i + 1) < argc)) {
            i++;
            bundleName = argv[i];
        } else if (strcmp(argv[i], "-C") == 0 && ((i + 1) < argc)) {
            i++;
            cmd = argv[i];
        }
    }

    if (coldStart) {
        SetParameter("startup.appspawn.cold.boot", "1");
        SetParameter("persist.appspawn.client.timeout", "10");
    }

    AppParameter request = {};
    memset_s((void *)(&request), sizeof(request), 0, sizeof(request));
    g_testClient.ClientFillMsg(&request, bundleName, cmd);

    request.flags = coldStart ? APP_COLD_BOOT : 0;
    request.flags |= !withSandbox ? APP_NO_SANDBOX : 0;
    request.code = SPAWN_NATIVE_PROCESS;
    return DoRun(server, &request);
}
