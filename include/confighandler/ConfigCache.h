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

#include <string>
#include <map>
#include <mutex>

/**
 * Config Cache
 *
 * Config cache ensures that if an application accesses a
 * streamfs config file in chunks, the application will
 * read the chunks of the same config until it closes the
 * file descriptor.
 *
 * This ensures that reader will not get fragments of different
 * config values if the file is read in chunks.
 */
class ConfigCache {
private:
    std::map<uint64_t , std::string> configMap;
    std::mutex mConfigMtx;
public:
    /**
     * Insert new configuration value
     * @param handle - file handle obtained from FUSE
     * @param value  - value
     */
    virtual void insert(uint64_t handle, std::string value);

    /**
     * Check if config cache has a handle
     * @param handle
     * @return
     */
    virtual bool hasKey(uint64_t handle) {
        std::lock_guard<std::mutex> lockGuard(mConfigMtx);
        return configMap.find(handle) != configMap.end();
    }

    /**
     * Get config value
     * @param handle
     * @return - current config value
     */
    virtual std::string getValue(uint64_t handle) {
        std::lock_guard<std::mutex> lockGuard(mConfigMtx);
        auto it = configMap.find(handle);

        if (it == configMap.end())
            return std::string();

        return it->second;
    }

    /**
     * Remove a handle.
     * Cache user is required to remote items from the cache
     * @param handle
     */
    virtual void remove(uint64_t handle) {
        std::lock_guard<std::mutex> lockGuard(mConfigMtx);
        configMap.erase(handle);
    }
};
