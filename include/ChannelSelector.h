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

#ifndef NOKIA_FCC_LIB_CHANNELSELECTOR_H
#define NOKIA_FCC_LIB_CHANNELSELECTOR_H

#include <cstddef>
#include <memory>
#include <string>
#include <mutex>
#include <utility>
#include <utility>
#include <streamfs/ByteBufferPool.h>
#include "confighandler/ConfigHandlerMVarCb.h"
#include "config_fcc.h"
#include "TimeShiftBufferConsumer.h"
#include <time.h>

class MediaSourceHandler;

namespace fcc {
class ChannelSelector : public ConfigHandlerMVarCb<ByteVectorType> {

public:
    ChannelSelector() = delete;

    ChannelSelector(std::shared_ptr<StreamParser::TimeShiftBufferConsumer> tsbStreamParser, MediaSourceHandler *mMediaSource,
                    streamfs::PluginCallbackInterface *cb) : ConfigHandlerMVarCb<ByteVectorType>(cb),  mTsb(tsbStreamParser),
                    mMSrcHandler(mMediaSource)
                    {};

    int writeConfig(const std::string &fileName, const std::string &buf, size_t size) override;

    std::string readConfig(const std::string &fileName) override;

    uint64_t getSize(const std::string &fileName) override;

private:
    std::string getConfig(const std::string & fileName);
    std::shared_ptr<StreamParser::TimeShiftBufferConsumer> mTsb;
    MediaSourceHandler *mMSrcHandler;
    struct timespec mLastRequestTime;
    std::mutex mStateMutex;
};

}

#endif //NOKIA_FCC_LIB_CHANNELSELECTOR_H
