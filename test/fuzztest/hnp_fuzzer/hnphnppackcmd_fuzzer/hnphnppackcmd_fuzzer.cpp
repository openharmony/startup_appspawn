/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hnp_fuzzer.h"
#include <string>
#include "appspawn_utils.h"
#include "parameter.h"
#include "hnp_base.h"
#include "hnp_pack.h"
#include "hnp_installer.h"
#include "hnp_api.h"
#include "securec.h"

namespace OHOS {
    int FuzzHnpPackCmd(const uint8_t *data, size_t size)
    {
        if (data == nullptr) {
            return 0;
        }
        (void)mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        (void)mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        FILE *fp = fopen("./hnp_sample/hnp.json", "w");
        (void)fclose(fp);
        
        char arg1[] = "hnp";
        char arg2[] = "pack";
        char arg3[] = const_cast<char *>(data);
        arg4[] = const_cast<char *>(data);
        arg5[] = const_cast<char *>(data);
        arg6[] = const_cast<char *>(data);
        char *argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);

        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":{}}";
        fp = fopen("./hnp_sample/hnp.json", "w");
        (void)fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp);
        (void)fclose(fp);

        (void)HnpCmdPack(argc, argv);
        (void)remove("./hnp_out/sample.hnp");

        (void)remove("./hnp_sample/hnp.json");
        (void)rmdir("hnp_sample");
        (void)rmdir("hnp_out");

        return 0;
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzHnpPackCmd(data, size);

    return 0;
}
