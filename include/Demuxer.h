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
#include "MediaSourceHandler.h"
#include "DemuxerCallbackHandler.h"

class MediaSourceHandler;

class Demuxer {
public:
    Demuxer(uint32_t id) : mId(id) {};

    uint32_t getId() const { return mId; }

    /**
     * Attach media source handler to demuxer
     * @param handler
     */
    virtual void attachMediaSourceHandler(MediaSourceHandler *handler) {
        mSrcHandler = handler;
    };

    /**
     * Init demuxer
     */
    virtual void init() = 0;

    /**
     * Connect demuxer
     * @return
     */
    virtual int connect() = 0;

    /**
     * Start demuxer
     * @param identifier
     * @return
     */
    virtual int start(int identifier) = 0;

    virtual std::shared_ptr<DemuxerCallbackHandler> getCallbackHandler() = 0;

    /**
     * Inform the demuxer about the first frame displayed.
     * Used for statistics purposes
     */
    virtual void informFirstFrameReceived() = 0;

    /**
     * Set demuxer parameters
     * @param params
     * @return
     */
    virtual int setDemuxerParameters(const std::string &params) = 0;

    /**
     * Disconnecte demuxer
     * @param connectionId
     * @return
     */
    virtual int disconnect(int connectionId) = 0;

    /**
     * Open URI
     * @param uri  - format is demuxer specific
     * @param uri  - hardware interface to use as source. The format is demuxer specific
     * @return
     */
    virtual int open(const std::string &uri, const std::string &hwInterface = "") = 0;

    virtual ~Demuxer() {};

    /**
     * Get global statistics
     * @return - JSON formated string
     */
    virtual std::string getGlobalStats() = 0;

    /**
     * Get channel statistics
     * @return - JSON formated string
     */
    virtual std::string getChannelStats(bool resetCounters) = 0;

protected:
    MediaSourceHandler *mSrcHandler;
    uint32_t mId;
};
