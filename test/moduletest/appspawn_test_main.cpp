/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <termios.h>
#include <unistd.h>

#include <sys/stat.h>

#include "appspawn.h"
#include "appspawn_msg.h"
#include "appspawn_utils.h"
#include "appspawn_test_cmder.h"
#include "command_lexer.h"
#include "json_utils.h"
#include "cJSON.h"
#include "securec.h"
#include "thread_manager.h"

int main(int argc, char *const argv[])
{
    if (argc <= 0) {
        return 0;
    }

    OHOS::AppSpawnModuleTest::AppSpawnTestCommander commander;
    int ret = commander.ProcessArgs(argc, argv);
    if (ret == 0) {
        commander.Run();
    }
    return 0;
}