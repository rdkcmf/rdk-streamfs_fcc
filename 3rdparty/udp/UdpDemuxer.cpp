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


#include <Demuxer.h>
#include "UdpDemuxer.h"

std::shared_ptr<Demuxer> new_demuxer_implementation(int id) {
    UNUSED(id);
    auto res = std::make_shared<multicast::UdpDemuxer>(id);
    return  std::dynamic_pointer_cast<Demuxer>(res);
}

namespace multicast {

void UdpDemuxer::init() {

}

int UdpDemuxer::connect() {
    return 0;
}

int UdpDemuxer::start(int identifier) {
    UNUSED(identifier);
    return 0;
}

void UdpDemuxer::informFirstFrameReceived() {

}

int UdpDemuxer::setDemuxerParameters(const std::string &params) {
    UNUSED(params);
    return 0;
}

int UdpDemuxer::disconnect(int connectionId) {
    UNUSED(connectionId);
    return 0;
}

int UdpDemuxer::open(const std::string &uri, const std::string &hwInterface) {
    UNUSED(uri);
    UNUSED(hwInterface);
    ChannelConfig channelConfig(uri);

    mUdpStreamListener.setup(channelConfig.destIPDotDecimal.c_str(), channelConfig.port, &mSrcHandler);

    return 0;
}

std::string UdpDemuxer::getGlobalStats() {
    return {};
}

std::string UdpDemuxer::getChannelStats(bool resetCounters) {
    UNUSED(resetCounters);
    return {};
}

std::shared_ptr<DemuxerCallbackHandler> UdpDemuxer::getCallbackHandler() {
    return std::dynamic_pointer_cast<DemuxerCallbackHandler>(mCb);
}
}
