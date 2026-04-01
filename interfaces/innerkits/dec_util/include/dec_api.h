/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef DEC_API_H
#define DEC_API_H

#include <map>
#include <string>
#include <vector>

/**
 * @brief Get decPaths map from sandbox json config file, Only allowed to be called by sandbox_manager_services,
 *        used to clean up decPaths when the application package changes
 *
 * @param void
 *
 * @return A map where the key is a permission string and the value is a vector of corresponding decPaths
 */
std::map<std::string, std::vector<std::string>> GetDecPathMap(void);

/**
 * @brief Get ignore case directory configuration list
 *
 * @return A vector of pairs where first is path string,
 *         second is mode (DEC_MODE_IGNORE_CASE or DEC_MODE_NOT_IGNORE_CASE)
 */
std::vector<std::pair<std::string, int>> GetIgnoreCaseDirs(void);

#endif
