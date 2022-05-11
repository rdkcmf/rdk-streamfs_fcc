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

#include <utility>

#include "StreamParser/Buffer.h"
#include "StreamProcessor.h"

namespace StreamParser {

class StreamSource {

public:
    StreamSource( std::shared_ptr<StreamProcessor>  streamProcessor) : mSPr(std::move(streamProcessor)) {};
    /**
     * Post buffer to stream processor
     * @param buf
     */
     virtual void post(const StreamParser::Buffer& buf) final;

     virtual void onEndOfStream(const char* channelId) final;

    virtual void onOpen(const char* channelId) final;

    virtual ~StreamSource();

private:
    std::shared_ptr<StreamProcessor> mSPr;
};

}
