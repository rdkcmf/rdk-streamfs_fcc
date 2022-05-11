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

#include <config_fcc.h>
#include "Buffer.h"

namespace StreamParser {

/**
 * Base class for consumers.
 */
class StreamConsumer {

public:
    /**
     * Constructor. Async StreamConsumers will start a
     * separate consumer thread that needs to be synchronized
     * once stream fragment is handled.
     * @param async - asynchronous stream handling. Default is false.
     */
    explicit StreamConsumer(const char* name, bool async = false);

    /**
     * Post data to consumer
     * @param buf
     */
    virtual void post(const StreamParser::Buffer &buf) {
        UNUSED(buf);
    };

    /**
     * Notification called on end of current stream
     */
    virtual void onEndOfStream(const char* channelId) {
        UNUSED(channelId);
    };

    /**
     * Notification called on end of current stream
     */
    virtual void onOpen(const char* channelId) {
        UNUSED(channelId);
    };

    /**
     * Complete buffer processing.
     * Implement this only for async consumers.
     */
    virtual void join() {};

    virtual ~StreamConsumer();

private:
    std::string mName;

};

}//namespace StreamParser
