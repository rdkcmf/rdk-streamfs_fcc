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

#include "ConfigHandlerMVarCb.h"
#include "TimeShiftBufferConsumer.h"
#include <MediaSourceHandler.h>

class MediaSourceHandler;

namespace fcc {
class TrickPlayRequestHandler : public ConfigHandlerMVarCb<ByteVectorType> {
public:
    explicit TrickPlayRequestHandler(std::shared_ptr<StreamParser::TimeShiftBufferConsumer> tsbStreamParser,
                                streamfs::PluginCallbackInterface *cb);

public:
    int writeConfig(const std::string &fileName, const std::string &buf, size_t size) override;

    std::string readConfig(const std::string &fileName) override;

    TrickPlayRequestHandler() = delete;
    uint64_t getSize(const std::string &fileName) override;
private:
    std::string getConfig();
    std::shared_ptr<StreamParser::TimeShiftBufferConsumer>  mTsbStreamParser;
    std::shared_ptr<MVar<ByteVectorType>::watcher_function> mCbFunc;
    std::mutex mStateMtx;
};

}
