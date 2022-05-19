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
#include "RtpCbHandler.h"
#include "RtpStreamListener.h"

namespace multicast {
class RtpDemuxer : public Demuxer {
public:
    explicit RtpDemuxer(uint32_t id) : Demuxer(id) {
        mCb = std::make_shared<RtpCbHandler>();
    }

    void init() override;

    int connect() override;

    int start(int identifier) override;

    void informFirstFrameReceived() override;

    int setDemuxerParameters(const std::string &params) override;

    int disconnect(int connectionId) override;

    int open(const std::string &uri, const std::string &hwInterface) override;

    std::string getGlobalStats() override;

    std::string getChannelStats(bool resetCounters) override;

    std::shared_ptr<DemuxerCallbackHandler> getCallbackHandler() override;

private:
    std::shared_ptr<RtpCbHandler> mCb;
    RtpStreamListener mRtpStreamListener;
};

}
