/*
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright (c) 2022 Nuuday.
 *
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

#pragma once

#include <cstddef>
#include <string>
#include <config_fcc.h>
#define CONFIG_ITEMS_SEPARATOR ","

namespace fcc {

class ConfigHandler {
public:
    explicit ConfigHandler() {}

    virtual ~ConfigHandler() = default;

    /**
     * Write configuration
     * @return 0 on success
     */
    virtual int writeConfig(const std::string &fileName, const std::string &buf, size_t size) {
        UNUSED(fileName);
        UNUSED(buf);
        UNUSED(size);
        return -1;
    }

    /**
     * Read config
     *
     * @param fileName
     * @param lazy - if set to true the config is not refreshed. This is used when the reader wants to access
     *               the same config value but in different chunks.
     * @return
     */
    virtual std::string readConfig(
            const std::string &fileName) {
        UNUSED(fileName);
        return "";
    };

    virtual uint64_t getSize(const std::string& fileName) {
        UNUSED(fileName);
        return 0;
    };

};

}
