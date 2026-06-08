/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MOCK_HISYSEVENT_H
#define MOCK_HISYSEVENT_H

/*
 * Mock hisysevent.h for unit testing.
 *
 * Replaces the real hisysevent.h to intercept HiSysEventWrite calls
 * without depending on the HiSysEvent daemon (libhisyseventmanager).
 *
 * The real HiSysEventWrite is a macro that expands to call
 * HiSysEvent::Write<domain>(), which communicates with the HiSysEvent
 * service via IPC. In unit test environments, this IPC fails because
 * the service is not running, making the return value unpredictable.
 *
 * This mock provides a controllable HiSysEventWrite macro that:
 * - Tracks call count and last domain/eventName/type for verification
 * - Returns a configurable return value (0 = success, non-zero = failure)
 * - Eliminates the dependency on libhisysevent / libhisyseventmanager
 */

#include <cstdint>
#include <cstring>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

// Minimal HiSysEventParam types needed by hisysevent_adapter.cpp
#define MAX_LENGTH_OF_PARAM_NAME 49

enum HiSysEventParamType {
    HISYSEVENT_INVALID = 0,
    HISYSEVENT_BOOL = 1,
    HISYSEVENT_INT8 = 2,
    HISYSEVENT_UINT8 = 3,
    HISYSEVENT_INT16 = 4,
    HISYSEVENT_UINT16 = 5,
    HISYSEVENT_INT32 = 6,
    HISYSEVENT_UINT32 = 7,
    HISYSEVENT_INT64 = 8,
    HISYSEVENT_UINT64 = 9,
    HISYSEVENT_FLOAT = 10,
    HISYSEVENT_DOUBLE = 11,
    HISYSEVENT_STRING = 12,
    HISYSEVENT_BOOL_ARRAY = 13,
    HISYSEVENT_INT8_ARRAY = 14,
    HISYSEVENT_UINT8_ARRAY = 15,
    HISYSEVENT_INT16_ARRAY = 16,
    HISYSEVENT_UINT16_ARRAY = 17,
    HISYSEVENT_INT32_ARRAY = 18,
    HISYSEVENT_UINT32_ARRAY = 19,
    HISYSEVENT_INT64_ARRAY = 20,
    HISYSEVENT_UINT64_ARRAY = 21,
    HISYSEVENT_FLOAT_ARRAY = 22,
    HISYSEVENT_DOUBLE_ARRAY = 23,
    HISYSEVENT_STRING_ARRAY = 24
};
typedef enum HiSysEventParamType HiSysEventParamType;

union HiSysEventParamValue {
    bool b;
    int8_t i8;
    uint8_t ui8;
    int16_t i16;
    uint16_t ui16;
    int32_t i32;
    uint32_t ui32;
    int64_t i64;
    uint64_t ui64;
    float f;
    double d;
    char *s;
    void *array;
};
typedef union HiSysEventParamValue HiSysEventParamValue;

struct HiSysEventParam {
    char name[MAX_LENGTH_OF_PARAM_NAME];
    HiSysEventParamType t;
    HiSysEventParamValue v;
    size_t arraySize;
};
typedef struct HiSysEventParam HiSysEventParam;

// C API stub
#define OH_HiSysEvent_Write(domain, name, type, params, size) (0)
inline int HiSysEvent_Write(const char* func, int64_t line, const char* domain,
    const char* name, int type, const HiSysEventParam params[], size_t size)
{
    return 0;
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <string>

namespace OHOS {
namespace HiviewDFX {

// Global mock state for HiSysEventWrite verification
struct MockHiSysEventState {
    int callCount = 0;
    int returnValue = 0;  // Configurable: 0 = success, non-zero = failure
    std::string lastDomain;
    std::string lastEventName;
    int lastEventType = 0;
};

// Singleton mock state, accessible from both test and adapter code
inline MockHiSysEventState& GetMockHiSysEventState()
{
    static MockHiSysEventState state;
    return state;
}

class HiSysEvent {
public:
    class Domain {
    public:
        static constexpr char APPSPAWN[] = "APPSPAWN";
    };

    enum EventType {
        FAULT     = 1,
        STATISTIC = 2,
        SECURITY  = 3,
        BEHAVIOR  = 4
    };

    // Mock Write: records call and returns configurable value
    template<const char* domain, typename... Types>
    static int Write(const char* func, int64_t line, const std::string& eventName,
        EventType type, Types... keyValues)
    {
        MockHiSysEventState& state = GetMockHiSysEventState();
        state.callCount++;
        state.lastDomain = domain;
        state.lastEventName = eventName;
        state.lastEventType = static_cast<int>(type);
        return state.returnValue;
    }
};

}  // namespace HiviewDFX
}  // namespace OHOS

// Mock HiSysEventWrite macro: mirrors the real macro structure but uses the mock Write
#define HiSysEventWrite(domain, eventName, type, ...) \
({ \
    int hiSysEventWriteRet2023___ = OHOS::HiviewDFX::HiSysEvent::Write<domain>( \
        __FUNCTION__, __LINE__, eventName, type, ##__VA_ARGS__); \
    hiSysEventWriteRet2023___; \
})

#endif // __cplusplus
#endif // MOCK_HISYSEVENT_H
