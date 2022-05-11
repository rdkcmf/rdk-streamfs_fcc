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


#include "TimeShiftBufferConsumer.h"
#include "StreamParser/PSIParser.h"
#include "version.h"
#include "MediaSourceHandler.h"
#include <boost/filesystem/path.hpp>
#include <config_options.h>
#include <memory>
#include "fcc/FCCConfigHandlers.h"


void streamfs::FCCConfigHandlers::initConfigHandlers(
        const std::shared_ptr<StreamParser::TimeShiftBufferConsumer> &tsbConsumer,
        MediaSourceHandler *mediaSourceHandler, streamfs::PluginCallbackInterface *cb) {

    /**
     * Init configuration handlers
     */
    mPlayerStateRequestHandler = std::make_shared<fcc::PlayerStateRequestHandler>(
            std::dynamic_pointer_cast<StreamParser::TimeShiftBufferConsumer>(tsbConsumer),
            cb);
    mSeekConfigurationHandler = std::make_shared<fcc::SeekRequestHandler>(
            std::dynamic_pointer_cast<StreamParser::TimeShiftBufferConsumer>(tsbConsumer),
            cb);

    mTrickPlayRequestHandler = std::make_shared<fcc::TrickPlayRequestHandler>(
            std::dynamic_pointer_cast<StreamParser::TimeShiftBufferConsumer>(tsbConsumer),
            cb);

    mStatsRequestHandler = std::make_shared<fcc::StatsRequestHandler>(mediaSourceHandler, cb);
    mStreamInfoRequestHandler = std::make_shared<fcc::StreamInfoRequestHandler>(mediaSourceHandler, cb);
    mProtectionInfoRequestHandler = std::make_shared<fcc::ProtectionInfoRequestHandler>(cb);
    mCdmStatusHandler = std::make_shared<fcc::CdmStatusHandler>(cb);
    mChannelSelector = std::make_shared<fcc::ChannelSelector>(
            std::dynamic_pointer_cast<StreamParser::TimeShiftBufferConsumer>(tsbConsumer),
            mediaSourceHandler,
            cb);

    //
    // Setup configuration handlers
    //
    for (auto &i : config_handlers_g) {
        switch (i.type) {
            case CHANNEL_SELECT:
                mConfigHandlers.emplace(i.type, mChannelSelector.get());
                break;
            case PLAYER_STATE:
                mConfigHandlers.emplace(i.type, mPlayerStateRequestHandler.get());
                break;
            case SEEK_CONTROL:
                mConfigHandlers.emplace(i.type, mSeekConfigurationHandler.get());
                break;
            case TRICK_PLAY:
                mConfigHandlers.emplace(i.type, mTrickPlayRequestHandler.get());
                break;
            case STATS_CONTROL:
                mConfigHandlers.emplace(i.type, mStatsRequestHandler.get());
                break;
            case STREAM_INFO:
                mConfigHandlers.emplace(i.type, mStreamInfoRequestHandler.get());
                break;
            case PROTECTION_INFO:
                mConfigHandlers.emplace(i.type, mProtectionInfoRequestHandler.get());
                break;
            case CDM_STATUS:
                mConfigHandlers.emplace(i.type, mCdmStatusHandler.get());
                break;
            case UNDEFINED:
                break;
        }
    }

}

fcc::ConfigHandler *streamfs::FCCConfigHandlers::targetToConfigHandler(const std::string &b) {
    auto conf = file_name_to_ConfigType(b.c_str());

    auto confHandler = mConfigHandlers.find(conf);
    if (confHandler == mConfigHandlers.end()) {
        return nullptr;
    }
    return confHandler->second;
}

std::string streamfs::FCCConfigHandlers::getConfigValue(uint64_t handle, const std::string &path, bool useCached) {

    UNUSED(handle);
    UNUSED(useCached);
    auto confHandler = targetToConfigHandler(path);

    if (confHandler == nullptr)
        return std::string();

    return confHandler->readConfig(path);
#if 0
    if (!useCached || !mConfigCache.hasKey(handle)) {
        const std::string c = confHandler->readConfig(path);
        mConfigCache.insert(handle, c);
    }

    auto result =  mConfigCache.getValue(handle);
    return result;
#endif
}

uint64_t streamfs::FCCConfigHandlers::getConfigSize(const std::string &path) {
    auto confHandler = targetToConfigHandler(path);

    // Support only config handlers. All other files are infinity
    if (confHandler == nullptr) {
        return INT64_MAX;
    }

    return confHandler->getSize(path);
}
