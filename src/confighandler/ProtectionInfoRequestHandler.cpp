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

#include <MediaSourceHandler.h>
#include <config_options.h>
#include <map>
#include "confighandler/ProtectionInfoRequestHandler.h"
#include "StreamProtectionConfigExport.h"

/**
 * Map internal variable ID to StreamFS paths
 */
static std::map<ConfigVariableId, const char *> ConfigMAP_ProtectionInfoToPath = {
        {kDrm0, CONFIG_F_PROTECTION_INFO}
};

int fcc::ProtectionInfoRequestHandler::writeConfig(const std::string &fileName, const std::string &buf, size_t size) {
    UNUSED(size);
    UNUSED(buf);
    LOG(ERROR) << "Write not implemented for:" << fileName;
    return -1;
}

std::string fcc::ProtectionInfoRequestHandler::readConfig(const std::string &fileName) {
    UNUSED(fileName);
    std::lock_guard<std::mutex> lockGuard(mMtxConfig);
    StreamProtectionConfigExport config(mCurrentConfig);
    return config.getJsonAsString();
}

void fcc::ProtectionInfoRequestHandler::notifyConfigurationChanged(const std::string &var,
                                                                   const StreamProtectionConfig &value) {
    std::lock_guard<std::mutex> lockGuard(mMtxConfig);

    StreamProtectionConfig::ConfidenceTypes newConfidence = value.confidence();

    if (newConfidence == StreamProtectionConfig::ConfidenceTypes::RESET ||
        newConfidence >= mCurrentConfig.confidence()) {
        mCurrentConfig = value;
        StreamProtectionConfigExport config(mCurrentConfig);
        mSize = config.getJsonAsString().size();
        fcc::ConfigHandlerMVarCb<StreamProtectionConfig>::notifyConfigurationChanged(var, value);
    }
}

fcc::ProtectionInfoRequestHandler::ProtectionInfoRequestHandler(streamfs::PluginCallbackInterface *cb) :
        ConfigHandlerMVarCb<StreamProtectionConfig>(cb),
        mConfidence(0) {

    StreamProtectionConfigExport config(mCurrentConfig);
    mSize = config.getJsonAsString().size();

    // Register watcher for variables
    mCbFunc = MVar<StreamProtectionConfig>::getWatcher(this, ConfigMAP_ProtectionInfoToPath);
}

uint64_t fcc::ProtectionInfoRequestHandler::getSize(const std::string &fileName) {
    UNUSED(fileName);
    return mSize;
}

