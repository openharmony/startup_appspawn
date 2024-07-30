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

#include "hnpnativeinstallhnp_fuzzer.h"

#include <string>

#include "appspawn_utils.h"
#include "parameter.h"
#include "hnp_base.h"
#include "hnp_pack.h"
#include "hnp_installer.h"
#include "hnp_api.h"
#include "securec.h"

namespace OHOS {
    void HnpPackWithoutBin(char *name, bool isPublic, bool isFirst)
    {
        char arg6[MAX_FILE_PATH_LEN];

        if (isPublic) {
            (void)strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/public");
        } else {
            (void)strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/private");
        }

        if (isFirst) {
            (void)mkdir("./hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("./hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_out/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_out/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }

        char arg1[] = "hnpcli";
        char arg2[] = "pack";
        char arg3[] = "-i";
        char arg4[] = "./hnp_sample";
        char arg5[] = "-o";
        char arg7[] = "-n";
        char arg9[] = "-v";
        char arg10[] = "1.1";
        char *argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, name, arg9, arg10};
        int argc = sizeof(argv) / sizeof(argv[0]);

        (void)HnpCmdPack(argc, argv);
    }

    void HnpPackWithoutBinDelete(void)
    {
        (void)HnpDeleteFolder("hnp_sample");
        (void)HnpDeleteFolder("hnp_out");
    }

    void HnpPackWithBin(char *name, bool isPublic, bool isFirst, mode_t mode)
    {
        char arg6[MAX_FILE_PATH_LEN];

        if (isPublic) {
            (void)strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/public");
        } else {
            (void)strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/private");
        }

        if (isFirst) {
            (void)mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_sample/bin", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            FILE *fp = fopen("hnp_sample/bin/out", "wb");
            (void)fclose(fp);
            (void)chmod("hnp_sample/bin/out", mode);
            (void)mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_out/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_out/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }

        char arg1[] = "hnpcli";
        char arg2[] = "pack";
        char arg3[] = "-i";
        char arg4[] = "./hnp_sample";
        char arg5[] = "-o";
        char arg7[] = "-n";
        char arg9[] = "-v";
        char arg10[] = "1.1";
        char *argv[] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, name, arg9, arg10};
        int argc = sizeof(argv) / sizeof(argv[0]);

        (void)HnpCmdPack(argc, argv);
    }

    void HnpPackWithBinDelete(void)
    {
        (void)HnpDeleteFolder("hnp_sample");
        (void)HnpDeleteFolder("hnp_out");
    }

    void HnpPackWithCfg(bool isPublic, bool isFirst)
    {
        FILE *fp;
        int whitelen;
        char arg6[MAX_FILE_PATH_LEN];

        if (isPublic) {
            (void)strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/public");
        } else {
            (void)strcpy_s(arg6, MAX_FILE_PATH_LEN, "./hnp_out/private");
        }

        if (isFirst) {
            (void)mkdir("hnp_sample", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_sample/bin", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_out", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_out/public", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            (void)mkdir("hnp_out/private", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            fp = fopen("hnp_sample/bin/out", "wb");
            (void)fclose(fp);
            fp = fopen("hnp_sample/bin/out2", "wb");
            (void)fclose(fp);
            fp = fopen("hnp_sample/hnp.json", "w");
            (void)fclose(fp);
        }

        char arg1[] = "hnp";
        char arg2[] = "pack";
        char arg3[] = "-i";
        char arg4[] = "./hnp_sample";
        char arg5[] = "-o";
        char *argv[] = {arg1, arg2, arg3, arg4, arg5, arg6};
        int argc = sizeof(argv) / sizeof(argv[0]);
        char cfg[] = "{\"type\":\"hnp-config\",\"name\":\"sample\",\"version\":\"1.1\",\"install\":"
            "{\"links\":[{\"source\":\"bin/out\",\"target\":\"outt\"},{\"source\":\"bin/out2\","
            "\"target\":\"out2\"}]}}";
        fp = fopen("hnp_sample/hnp.json", "w");
        whitelen = fwrite(cfg, sizeof(char), strlen(cfg) + 1, fp);
        (void)fclose(fp);
        (void)whitelen, strlen(cfg) + 1);

        (void)HnpCmdPack(argc, argv);
    }

    void HnpPackWithCfgDelete(void)
    {
        (void)HnpDeleteFolder("hnp_sample");
        (void)HnpDeleteFolder("hnp_out");
    }


    int FuzzNativeInstallHnp(const uint8_t *data, size_t size)
    {
        if (data == nullptr) {
            return 0;
        }

        char *userId = const_cast<char *>(data);
        int installOptions = *(reinterpret_cast<int *>(data));
        const char *hnpRootPath = "./hnp_out";
        HapInfo hapInfo;
        hapInfo.packageName = const_cast<char *>(data);
        hapInfo.hapPath = const_cast<char *>(data);
        hapInfo.abi = const_cast<char *>(data);

        // clear resource before test
        HnpDeleteFolder("hnp_sample");
        HnpDeleteFolder("hnp_out");
        HnpDeleteFolder(HNP_BASE_PATH);
        remove(HNP_PACKAGE_INFO_JSON_FILE_PATH);
   
        (void)mkdir(HNP_BASE_PATH, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        HnpPackWithBin(const_cast<char *>("sample_public"), true, true, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
        HnpPackWithBin(const_cast<char *>("sample_private"), false, false, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);

        (void)NativeInstallHnp(userId, hnpRootPath, hapInfo, installOptions);
        return 0;
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzNativeInstallHnp(data, size);
    return 0;
}
