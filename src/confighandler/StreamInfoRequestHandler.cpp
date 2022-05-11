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
#include "confighandler/StreamInfoRequestHandler.h"
#include <map>

/**
 * Map internal variable ID to StreamFS paths
 */
static std::map<ConfigVariableId, const char *> ConfigMAP_StreamInfoToPath = {
        {kEcm0, CONFIG_F_FCC_STREAM_INFO_ECM},
        {kPmt0, CONFIG_F_FCC_STREAM_INFO_PMT},
        {kPat0, CONFIG_F_FCC_STREAM_INFO_PAT}
};

int fcc::StreamInfoRequestHandler::writeConfig(const std::string &fileName, const std::string &buf, size_t size) {
    UNUSED(size);
    UNUSED(buf);
    LOG(WARNING) << "TODO. Write not implemented for:" << fileName;
    return -1;
}

std::string fcc::StreamInfoRequestHandler::readConfig(const std::string &fileName) {
    return getConfig(fileName);
}

fcc::StreamInfoRequestHandler::StreamInfoRequestHandler(MediaSourceHandler *mediaSourceHandler,
                                                        streamfs::PluginCallbackInterface *cb) :
        ConfigHandlerMVarCb<ByteVectorType>(cb),
        mMSrcHandler(mediaSourceHandler) {

    // Register watcher for variables
    mCbFunc = MVar<ByteVectorType>::getWatcher(this, ConfigMAP_StreamInfoToPath);
}

uint64_t fcc::StreamInfoRequestHandler::getSize(const std::string &fileName) {
    return getConfig(fileName).size();
}

std::string fcc::StreamInfoRequestHandler::getConfig(const std::string &fileName) {
    ByteVectorType tmp;

    switch (hashStr(fileName.c_str())) {
        case hashStr(CONFIG_F_FCC_STREAM_INFO_PMT):
            tmp = MVar<ByteVectorType>::getVariable(kPmt0).getValue();
            goto success;

        case hashStr(CONFIG_F_FCC_STREAM_INFO_PAT):
            tmp = MVar<ByteVectorType>::getVariable(kPat0).getValue();
            goto success;

        case hashStr(CONFIG_F_FCC_STREAM_INFO_ECM):
            tmp = MVar<ByteVectorType>::getVariable(kEcm0).getValue();
            goto success;
    }

    return "Not supported";
    success:
    return std::string(tmp.begin(), tmp.end());
}

