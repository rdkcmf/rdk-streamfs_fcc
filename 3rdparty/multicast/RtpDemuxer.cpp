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
#include "RtpDemuxer.h"

std::shared_ptr<Demuxer> new_demuxer_implementation(int id) {
    UNUSED(id);
    auto res = std::make_shared<multicast::RtpDemuxer>(id);
    return  std::dynamic_pointer_cast<Demuxer>(res);
}

namespace multicast {

void RtpDemuxer::init() {

}

int RtpDemuxer::connect() {
    return 0;
}

int RtpDemuxer::start(int identifier) {
    UNUSED(identifier);
    return 0;
}

void RtpDemuxer::informFirstFrameReceived() {

}

int RtpDemuxer::setDemuxerParameters(const std::string &params) {
    UNUSED(params);
    return 0;
}

int RtpDemuxer::disconnect(int connectionId) {
    UNUSED(connectionId);
    return 0;
}

int RtpDemuxer::open(const std::string &uri, const std::string &hwInterface) {
    UNUSED(uri);
    UNUSED(hwInterface);
    ChannelConfig channelConfig(uri);

    mRtpSListener.setup(channelConfig.destIPDotDecimal.c_str(), channelConfig.port, &mSrcHandler);

    return 0;
}

std::string RtpDemuxer::getGlobalStats() {
    return {};
}

std::string RtpDemuxer::getChannelStats(bool resetCounters) {
    UNUSED(resetCounters);
    return {};
}

std::shared_ptr<DemuxerCallbackHandler> RtpDemuxer::getCallbackHandler() {
    return std::dynamic_pointer_cast<DemuxerCallbackHandler>(mCb);
}
}
