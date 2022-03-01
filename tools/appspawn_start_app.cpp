#include <cstring>

#include "appspawn_server.h"
#include "client_socket.h"
#include "hilog/log.h"
#include "securec.h"

static const int DECIMAL = 10;

int main(int argc, char *const argv[])
{
    if (argc <= 11) { // 11 min argc
        printf("appspawntools xxxx \n");
        return 0;
    }

    // calculate child process long name size
    uintptr_t start = reinterpret_cast<uintptr_t>(argv[0]);
    uintptr_t end = reinterpret_cast<uintptr_t>(strchr(argv[argc - 1], 0));
    uintptr_t argvSize = end - start;
    if (end == 0) {
        return 0;
    }

    auto appProperty = std::make_unique<OHOS::AppSpawn::ClientSocket::AppProperty>();
    if (appProperty == nullptr) {
        return -1;
    }
    int index = 1;
    int fd = strtoul(argv[index++], nullptr, DECIMAL);
    appProperty->uid = strtoul(argv[index++], nullptr, DECIMAL);
    appProperty->gid = strtoul(argv[index++], nullptr, DECIMAL);
    (void)strcpy_s(appProperty->processName, sizeof(appProperty->processName), argv[index++]);
    (void)strcpy_s(appProperty->bundleName, sizeof(appProperty->bundleName), argv[index++]);
    (void)strcpy_s(appProperty->soPath, sizeof(appProperty->soPath), argv[index++]);
    appProperty->accessTokenId = strtoul(argv[index++], nullptr, DECIMAL);
    (void)strcpy_s(appProperty->apl, sizeof(appProperty->apl), argv[index++]);
    (void)strcpy_s(appProperty->renderCmd, sizeof(appProperty->renderCmd), argv[index++]);
    appProperty->flags = strtoul(argv[index++], nullptr, DECIMAL);

    appProperty->gidCount = strtoul(argv[index++], nullptr, DECIMAL);
    uint32_t i = 0;
    while (i < appProperty->gidCount && i < sizeof(appProperty->gidTable) / sizeof(sizeof(appProperty->gidTable[0]))) {
        if (argv[index] == nullptr) {
            break;
        }
        appProperty->gidTable[i] = strtoul(argv[index++], nullptr, DECIMAL);
    }
    auto appspawnServer = std::make_shared<OHOS::AppSpawn::AppSpawnServer>("AppSpawn");
    if (appspawnServer != nullptr) {
        int ret = appspawnServer->AppColdStart(argv[0], argvSize, appProperty.get(), fd);
        if (ret != 0) {
            printf("Cold start %s fail \n", appProperty->bundleName);
        }
    }
    return 0;
}
