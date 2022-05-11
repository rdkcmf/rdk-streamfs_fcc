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
#include <utility>
#include "confighandler/TrickPlayRequestHandler.h"

static MapConfigToFSPathType ConfigMAP_TrickPlayRequestHandler = {
        {kTrickPlay0, CONFIG_F_TRICK_PLAY}
};

int fcc::TrickPlayRequestHandler::writeConfig(const std::string &fileName, const std::string &buf, size_t size) {
    UNUSED(fileName);

    std::lock_guard<std::mutex> lockGuard(mStateMtx);
    try {
        int16_t speedValue = std::stoi(buf);
        LOG(INFO) << "TrickPlayRequestHandler::writeConfig : speed : " << speedValue;
        if (mTsbStreamParser->setTrickPlaySpeed(speedValue)) {
            return size;
        } else {
            LOG(WARNING) << "Invalid speed value: " << speedValue;
            return -1;
        }
    } catch (const std::invalid_argument &ia) {
        LOG(ERROR) << "Invalid argument: " << ia.what();
        return -1;
    }

}

std::string fcc::TrickPlayRequestHandler::readConfig(const std::string &fileName) {
    UNUSED(fileName);

    return getConfig();
}

fcc::TrickPlayRequestHandler::TrickPlayRequestHandler(std::shared_ptr<StreamParser::TimeShiftBufferConsumer> tsbSp,
                                            streamfs::PluginCallbackInterface *cb) :
        ConfigHandlerMVarCb<ByteVectorType>(cb), mTsbStreamParser(std::move(tsbSp)) {

    mCbFunc = MVar<ByteVectorType>::getWatcher(this, ConfigMAP_TrickPlayRequestHandler);
}

uint64_t fcc::TrickPlayRequestHandler::getSize(const std::string &fileName) {
    UNUSED(fileName);
    return getConfig().size();
}

std::string fcc::TrickPlayRequestHandler::getConfig() {
    std::lock_guard<std::mutex> lockGuard(mStateMtx);
    return std::to_string(mTsbStreamParser->getTrickPlaySpeed());
}

