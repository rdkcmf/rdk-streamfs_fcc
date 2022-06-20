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
#include "config_options.h"
#include <streamfs/PluginInterface.h>
#include <streamfs/BufferConsumer.h>
#include <streamfs/ByteBufferPool.h>
#include <streamfs/PluginCallbackInterface.h>
#include <confighandler/CdmStatusHandler.h>
#include <fcc/FCCConfigHandlers.h>
#include "Demuxer.h"
#include "confighandler/ConfigHandler.h"
#include "confighandler/ChannelSelector.h"
#include "confighandler/SeekRequestHandler.h"
#include "confighandler/StatsRequestHandler.h"
#include "confighandler/StreamInfoRequestHandler.h"
#include "confighandler/ProtectionInfoRequestHandler.h"
#include "utils/ConstDelayDefHandler.h"
#include "HttpDemuxerImpl.h"
#include "Logging.h"
#include "HandleContext.h"
#include "demux_impl.h"

std::shared_ptr<Demuxer> g_fcc_demuxer = nullptr;

namespace streamfs {

class FCCPlugin : public PluginInterface, public FCCConfigHandlers {
public:
    explicit FCCPlugin(streamfs::PluginCallbackInterface *cb, Demuxer *demuxer,
                            std::shared_ptr<DemuxerCallbackHandler> cbHandler,
                            debug_options_t *debugOptions);

    void updateConfiguration(const PluginConfig &config) override;

    std::string getId() override { return CONFIG_FCC_PLUGIN_ID; }

    void stopPlayback() override;

    int open(std::string path) override;

    int read(uint64_t handle, std::string path, char *buf, size_t size, uint64_t offset) override;

    int write(std::string node, const char *buf, size_t size, uint64_t offset) override;

    virtual ~FCCPlugin();

    virtual MediaSourceHandler *getSourceHandler() const noexcept {
        return mMediaSource.get();
    }

    FCCPlugin(FCCPlugin const &) = delete;

    void operator=(FCCPlugin const &) = delete;

    int release(uint64_t handle, std::string path) override;

    uint64_t getSize(std::string path) override;

private:

    void allocateBufferQueue();

private:
    // Read-blocked ensures that the reader thread can not read
    // any new data until the encryption is reconfigured by the
    // CDM
    bool mIsReadBlocked;
    MVar<bool> *mIsCdmSetupDone;

private:
    std::set<std::string> mActiveUris;

    std::set<std::string> mFiles;
    std::shared_ptr<MediaSourceHandler> mMediaSource;

    std::shared_ptr<ConstDelayDefHandler> mDefferalHandler;
    std::shared_ptr<StreamParser::StreamProcessor> mStreamProcessor;
    std::shared_ptr<StreamParser::TimeShiftBufferConsumer> mTsbConsumer;

    std::map<uint64_t, HandleContext> mHandles;

    std::map<std::string, uint32_t>  mStreamOpenCounters {};
    std::mutex mFdHandlerMtx;
};
}

extern "C" {
    streamfs::PluginInterface *INIT_STREAMFS_PLUGIN(streamfs::PluginCallbackInterface *cb, debug_options_t *debugOptions) {
    char *streamType = getenv("STREAM_TYPE");

    ref_debug_options = debugOptions;

    if (NULL != streamType) {
        HttpDemuxerImpl *mHttpDemuxer;
        std::shared_ptr<HttpCBHandler> mHttpCBHandler;
        if (0 == strcmp(streamType, "http")) {
            mHttpDemuxer = new HttpDemuxerImpl(0);
            LOG(INFO) << "PluginInterface : creating new instance of FCCPlugin for HTTP";
            return new streamfs::FCCPlugin(cb, mHttpDemuxer, mHttpDemuxer->getCallbackHandler(), debugOptions);
        } else {
            LOG(ERROR) << "PluginInterface : invalid environment variable STREAM_TYPE";
            return nullptr;
        }
    } else {
        if (g_fcc_demuxer != nullptr) {
            LOG(ERROR) << "Plugin interface already initialized";
            return nullptr;
        }

        g_fcc_demuxer = new_demuxer_implementation(0);

        return new streamfs::FCCPlugin(cb, g_fcc_demuxer.get(), g_fcc_demuxer->getCallbackHandler(), debugOptions);
    }
}

const char *GET_STREAMFS_PLUGIN_ID() {
    return CONFIG_FCC_PLUGIN_ID;
}

EXPORT void UNLOAD_STREAMFS_PLUGIN(streamfs::PluginInterface *mFcc) {
    if (mFcc != nullptr) {
        mFcc->stopPlayback();
    } else
        LOG(INFO) << " FCCPlugin:: mFcc is nullptr";
}

}

