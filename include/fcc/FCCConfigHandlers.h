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

#include <exception>
#include <memory>
#include <string>
#include <utility>
#include <set>
#include <streamfs/PluginInterface.h>
#include <streamfs/BufferConsumer.h>
#include <streamfs/ByteBufferPool.h>
#include <streamfs/PluginCallbackInterface.h>
#include <confighandler/CdmStatusHandler.h>
#include <fcc/FCCConfigHandlers.h>
#include "config_options.h"
#include "Demuxer.h"
#include "confighandler/ConfigHandler.h"
#include "confighandler/ChannelSelector.h"
#include "confighandler/PlayerStateRequestHandler.h"
#include "confighandler/SeekRequestHandler.h"
#include "confighandler/StatsRequestHandler.h"
#include "confighandler/StreamInfoRequestHandler.h"
#include "confighandler/ProtectionInfoRequestHandler.h"
#include "confighandler/TrickPlayRequestHandler.h"
#include "utils/ConstDelayDefHandler.h"
#include "HttpDemuxerImpl.h"

#include "../confighandler/ConfigCache.h"

/**
 * Convert file name to config type
 * @param filename
 * @return
 */
inline ConfigType file_name_to_ConfigType(const char *filename) {
    for (auto &i : config_handlers_g) {
        if (strncmp(filename, i.config_file, strlen(i.config_file)) == 0) {
            return i.type;
        }
    }
    return UNDEFINED;
}

namespace streamfs {

class   FCCConfigHandlers {

protected:
    std::shared_ptr<fcc::CdmStatusHandler> mCdmStatusHandler;

    std::shared_ptr<fcc::ChannelSelector> mChannelSelector;

    // Configuration handlers
    std::map<ConfigType, fcc::ConfigHandler *> mConfigHandlers;
    std::shared_ptr<fcc::ProtectionInfoRequestHandler> mProtectionInfoRequestHandler;
    std::shared_ptr<fcc::PlayerStateRequestHandler> mPlayerStateRequestHandler;
    std::shared_ptr<fcc::SeekRequestHandler> mSeekConfigurationHandler;
    std::shared_ptr<fcc::StatsRequestHandler> mStatsRequestHandler;
    std::shared_ptr<fcc::StreamInfoRequestHandler> mStreamInfoRequestHandler;
    std::shared_ptr<fcc::TrickPlayRequestHandler> mTrickPlayRequestHandler;

private:
    ConfigCache mConfigCache;
protected:
    /**
     * Init configuration handlers
     * @param tsbConsumer  - time shift consumer
     * @param mediaSourceHandler - media source handler
     * @param cb  - callback to StreamFS interface
     */
    void initConfigHandlers(
            const std::shared_ptr<StreamParser::TimeShiftBufferConsumer>& tsbConsumer,
            MediaSourceHandler* mediaSourceHandler,
            streamfs::PluginCallbackInterface *cb
    );

    fcc::ConfigHandler *targetToConfigHandler(const std::string &b);

    std::string getConfigValue(uint64_t handle, const std::string &path, bool useCached);

    uint64_t getConfigSize(const std::string &path);

    void releaseConfigValue(uint64_t handle) {
        mConfigCache.remove(handle);
    }
};
}
