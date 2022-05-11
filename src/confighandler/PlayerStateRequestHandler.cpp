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
#include "confighandler/PlayerStateRequestHandler.h"

int fcc::PlayerStateRequestHandler::writeConfig(const std::string &fileName, const std::string &buf, size_t size) {
    UNUSED(fileName);

    std::lock_guard<std::mutex> lockGuard(mStateMtx);
    try {
        auto playerState = PlayerStateEnum(buf);
        if (playerState.isValid()) {
            LOG(INFO) << "PlayerStateRequestHandler::writeConfig : playerState=" << playerState.stringValue() << " size=" << size;
            mTsbStreamParser->setPlayerState(playerState.enumValue());
            return size;
        } else {
            LOG(WARNING) << "Invalid playerState=" << buf;
            return -1;
        }
    } catch (const std::invalid_argument &ia) {
        LOG(ERROR) << "Invalid argument: " << ia.what();
        return -1;
    }
}

std::string fcc::PlayerStateRequestHandler::readConfig(const std::string &fileName) {
    UNUSED(fileName);
    return getPlayerState();
}

fcc::PlayerStateRequestHandler::PlayerStateRequestHandler(std::shared_ptr<StreamParser::TimeShiftBufferConsumer> tsbSp,
                                            streamfs::PluginCallbackInterface *cb) :
        ConfigHandlerMVarCb<ByteVectorType>(cb), mTsbStreamParser(std::move(tsbSp)) {
}

uint64_t fcc::PlayerStateRequestHandler::getSize(const std::string &fileName) {
    UNUSED(fileName);
    return getPlayerState().size();
}

std::string fcc::PlayerStateRequestHandler::getPlayerState() {
    std::lock_guard<std::mutex> lockGuard(mStateMtx);
    auto playerState = PlayerStateEnum(mTsbStreamParser->getPlayerState());
    return playerState.stringValue();
}

