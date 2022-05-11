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
#include "confighandler/CdmStatusHandler.h"

/**
 * Map internal variable ID to StreamFS paths
 */
static std::map<ConfigVariableId, const char *> ConfigMAP_CdmInfoToPath = {
        {kCdm0, CONFIG_F_CDM_STATUS}
};

int fcc::CdmStatusHandler::writeConfig(const std::string &fileName, const std::string &buf, size_t size) {
    UNUSED(size);
    UNUSED(fileName);

    try {
        if (stoi(buf) == 1) {
            *mIsCdmSetupDone = true;
            mCdmConfigComplete = true;
            return size;
        }
    } catch (std::invalid_argument &e) {
        UNUSED(e);
        LOG(INFO) << "Invalid argument:" << buf;
    }
    return -1;
}

std::string fcc::CdmStatusHandler::readConfig(const std::string &fileName) {
    return getConfig(fileName);
}

void fcc::CdmStatusHandler::notifyConfigurationChanged(const std::string &var, const bool &value) {
    UNUSED(var);
    std::lock_guard<std::mutex> lockGuard(mMtxConfig);
    mCdmConfigComplete = value;
}

fcc::CdmStatusHandler::CdmStatusHandler(streamfs::PluginCallbackInterface *cb) :
        ConfigHandlerMVarCb<bool>(cb),
        mConfidence(0) {

    // Register watcher for variables
    mCbFunc = MVar<bool>::getWatcher(this, ConfigMAP_CdmInfoToPath);
    mCdmConfigComplete = true;
    mIsCdmSetupDone = &MVar<bool>::getVariable(kCdm0);
}

uint64_t fcc::CdmStatusHandler::getSize(const std::string &fileName) {
    return getConfig(fileName).size();
}

std::string fcc::CdmStatusHandler::getConfig(const std::string &fileName) {
    UNUSED(fileName);
    std::lock_guard<std::mutex> lockGuard(mMtxConfig);
    return mIsCdmSetupDone->getValue() ? "1" : "0";
}


