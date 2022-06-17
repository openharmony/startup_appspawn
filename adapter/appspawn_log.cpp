/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
 
#include "appspawn_server.h"

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

#include <cerrno>
#include <cstdarg>
#include <ctime>

#include "hilog/log.h"
#include "securec.h"

namespace {
    AppspawnLogLevel g_logLevel = AppspawnLogLevel::INFO;
    constexpr int MAX_LOG_SIZE = 1024;
    constexpr int BASE_YEAR = 1900;
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, 0, APPSPAWN_LABEL};
    const char *APPSPAWN_LOG_PATH = "/data/init_agent/appspawn.log";
}

void AppspawnLogPrint(AppspawnLogLevel logLevel, const char *file, int line, const char *fmt, ...)
{
    if (logLevel < g_logLevel) {
        return;
    }

    va_list list;
    va_start(list, fmt);
    char tmpFmt[MAX_LOG_SIZE];
    if (vsnprintf_s(tmpFmt, MAX_LOG_SIZE, MAX_LOG_SIZE - 1, fmt, list) == -1) {
        va_end(list);
        return;
    }
    va_end(list);

    switch (logLevel) {
        case AppspawnLogLevel::DEBUG:
            OHOS::HiviewDFX::HiLog::Debug(LABEL, "[%{public}s:%{public}d]%{public}s", file, line, tmpFmt);
            break;
        case AppspawnLogLevel::INFO:
            OHOS::HiviewDFX::HiLog::Info(LABEL, "[%{public}s:%{public}d]%{public}s", file, line, tmpFmt);
            break;
        case AppspawnLogLevel::WARN:
            OHOS::HiviewDFX::HiLog::Warn(LABEL, "[%{public}s:%{public}d]%{public}s", file, line, tmpFmt);
            break;
        case AppspawnLogLevel::ERROR:
            OHOS::HiviewDFX::HiLog::Error(LABEL, "[%{public}s:%{public}d]%{public}s", file, line, tmpFmt);
            break;
        case AppspawnLogLevel::FATAL:
            OHOS::HiviewDFX::HiLog::Fatal(LABEL, "[%{public}s:%{public}d]%{public}s", file, line, tmpFmt);
            break;
        default:
            break;
    }

    time_t second = time(0);
    if (second <= 0) {
        return;
    }
    struct tm *t = localtime(&second);
    FILE *outfile = fopen(APPSPAWN_LOG_PATH, "a+");
    if (t == nullptr || outfile == nullptr) {
        return;
    }
    (void)fprintf(outfile, "[%d-%d-%d %d:%d:%d][pid=%d][%s:%d]%s \n",
        (t->tm_year + BASE_YEAR), (t->tm_mon + 1), t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec,
        getpid(), file, line, tmpFmt);
    (void)fflush(outfile);
    (void)fclose(outfile);
    return;
}
