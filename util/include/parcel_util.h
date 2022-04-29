/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef PACEL_UTIL_H
#define PACEL_UTIL_H

#include <cstdint>
#include <list>
#include "parcel.h"

namespace OHOS {
namespace AppSpawn {
template<class T>
std::vector<T> TranslateListToVector(const std::list<T> &originList)
{
    std::size_t len = originList.size();
    std::vector<T> destVector(len);
    std::copy(originList.begin(), originList.end(), destVector.begin());
    return destVector;
}

template<class T>
std::list<T> TranslateVectorToList(const std::vector<T> &originVector)
{
    int len = originVector.length();
    std::list<T> destList(len);
    std::copy(originVector.begin(), originVector.end(), destList.begin());
    return destList;
}
} // namespace AppSpawn
} // namespace OHOS

#endif // PACEL_UTIL_H