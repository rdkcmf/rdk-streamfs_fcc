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
#include "StreamParser/Buffer.h"
#include "StreamParser/StreamConsumer.h"
#include <memory>
#include <vector>

namespace StreamParser {

/**
 * StreamProcessor is the central class for handling
 * received stream data.
 * The class is responsible to passing buffers to
 * the different StreamConsumers
 */
class StreamProcessor {
    CLASS_NO_COPY_OR_ASSIGN(StreamProcessor);

public:
    StreamProcessor(std::initializer_list<std::shared_ptr<StreamConsumer>> consumers) {
        for (auto it : consumers) {
                mSCons.emplace_back(it);
        }
    };
    void onEndOfStream(const char* channelId);

    void onOpen(const char* channelId);

    void post(const StreamParser::Buffer& buf) const;

private:
    std::vector<std::shared_ptr<StreamConsumer>> mSCons;
};

} //namespace StreamParser
