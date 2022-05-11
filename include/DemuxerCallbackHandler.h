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
#include "DemuxerStatusCallback.h"

class DemuxerCallbackHandler {

public:
    /**
     * Set callback function for a demuxer
     * @param cb  - callback object
     * @param demuxerId  - demuxer id
     */
    virtual void setCallbackHandler(DemuxerStatusCallback *cb, int demuxerId);

    /**
     * Notify stream switched.
     *
     * Socket messages from Nokia VBO have no identifiers. The only way we know
     * that it is a PAT/PMT or ECM message is that if the message is received right
     * after channel switch or it is the second message after channel switch.
     * To be able to identify the message type, we need to notify the callback handler
     * about the stream switched.
     *
     * This method should be called
     *
     * @param channelIp  - new channel IP (and port). E.g. "234.80.160.1:5900"
     */
    virtual void notifyStreamSwitched(const std::string &channelIp) = 0;

    int getDemuxerId() const {
        return mDemuxId;
    }

protected:
    DemuxerStatusCallback *mCb = nullptr;
    int mDemuxId = 0;
};
