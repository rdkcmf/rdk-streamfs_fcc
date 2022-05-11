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
#include "confighandler/SeekRequestHandler.h"

#define SECONDS_TO_MILLISECONDS 1000

static MapConfigToFSPathType ConfigMAP_SeekRequestHandler = {
        {kFlush0, CONFIG_F_STREAM_INFO_FLUSH}
};

int fcc::SeekRequestHandler::writeConfig(const std::string &fileName, const std::string &buf, size_t size) {
    UNUSED(fileName);

    std::lock_guard<std::mutex> lockGuard(mStateMtx);
    try {
        auto seekValue = std::stoull(buf);
        LOG(INFO) << "SeekRequestHandler::writeConfig : seekValue (Time): " << seekValue;
        if (mTsbStreamParser->setSeekTime(seekValue * SECONDS_TO_MILLISECONDS)) {
            std::string sCh = "seek change";
            *mFlush = ByteVectorType(sCh.begin(), sCh.end());
            return size;
        } else {
            LOG(WARNING) << "Invalid seek value: " << seekValue;
            return -1;
        }
    } catch (const std::invalid_argument &ia) {
        LOG(ERROR) << "Invalid argument: " << ia.what();
        return -1;
    }

}

std::string fcc::SeekRequestHandler::readConfig(const std::string &fileName) {
    UNUSED(fileName);

    return getConfig();
}

fcc::SeekRequestHandler::SeekRequestHandler(std::shared_ptr<StreamParser::TimeShiftBufferConsumer> tsbSp,
                                            streamfs::PluginCallbackInterface *cb) :
        ConfigHandlerMVarCb<ByteVectorType>(cb), mTsbStreamParser(std::move(tsbSp)) {

    mCbFunc = MVar<ByteVectorType>::getWatcher(this, ConfigMAP_SeekRequestHandler);
    mFlush = &MVar<ByteVectorType>::getVariable(kFlush0);
}

uint64_t fcc::SeekRequestHandler::getSize(const std::string &fileName) {
    UNUSED(fileName);
    return getConfig().size();
}

std::string fcc::SeekRequestHandler::getConfig() {
    std::lock_guard<std::mutex> lockGuard(mStateMtx);
    return std::to_string(mTsbStreamParser->getSeekTime() / SECONDS_TO_MILLISECONDS) + CONFIG_ITEMS_SEPARATOR +
           std::to_string(mTsbStreamParser->getMaxSeekTime() / SECONDS_TO_MILLISECONDS) + CONFIG_ITEMS_SEPARATOR +
           std::to_string(mTsbStreamParser->getSeekOffset()) + CONFIG_ITEMS_SEPARATOR +
           std::to_string(mTsbStreamParser->getActualBufferByteSize()) + CONFIG_ITEMS_SEPARATOR +
           std::to_string((mTsbStreamParser->getBufferCapacityByteSize()));
}

