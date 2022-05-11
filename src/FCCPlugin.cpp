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

#include <algorithm>
#include <memory>
#include <config_options.h>
#include <boost/filesystem/path.hpp>
#include "FCCPlugin.h"
#include "MediaSourceHandler.h"
#include "version.h"
#include "StreamParser/PSIParser.h"
#include "StreamParser/EcmCache.h"
#include "TimeShiftBufferConsumer.h"
#include "fcc/FCCConfigHandlers.h"

debug_options_t* ref_debug_options = nullptr;

void streamfs::FCCPlugin::updateConfiguration(const PluginConfig &config) {
    UNUSED(config);
}

void streamfs::FCCPlugin::stopPlayback() {
}

int streamfs::FCCPlugin::open(std::string path) {

    TRACE_EVENT(TR_FCC_SWITCH, __FUNCTION__);
    std::lock_guard<std::mutex> lockGuard(mFdHandlerMtx);

    if (mFiles.find(path) != mFiles.end()) {
        auto it = mStreamOpenCounters.find(path);

        if (it == mStreamOpenCounters.end()) {
            mStreamOpenCounters[path] = 1;
        } else {
            it->second +=1;
        }

        // TODO: support multiple clients opening stream0.ts
        if (path == STREAM_SRC_FILE && mStreamOpenCounters.find(path)->second > 1 ) {
            LOG(ERROR) << " Opening stream0.ts multiple times will cause continuity errors!";
        }

        return 0;
    } else {
        return -1;
    }
}

int streamfs::FCCPlugin::release(uint64_t handle, std::string path) {
    std::lock_guard<std::mutex> lockGuard(mFdHandlerMtx);

    auto it = mStreamOpenCounters.find(path);

    if (it != mStreamOpenCounters.end()) {
        it->second --;
    }  else {
        LOG(WARNING) << "BUG: can't remove handle of path that was never opened.";
        LOG(WARNING) << "BUG: Handle " << handle << " path: " << path;
    }

    // If the path is not a stream file
    if (mActiveUris.find(path) == mActiveUris.end()) {
        // Release request handle
        releaseConfigValue(handle);
    } else {
        // Release stream file handle
        mTsbConsumer->release(handle);
    }

    return 0;
}

int streamfs::FCCPlugin::read(uint64_t handle, std::string path, char *buf, size_t size, uint64_t offset) {

    TRACE_EVENT(TR_FCC_SWITCH, "FCCPlugin::read", "size", size, "offset", offset);

    int res = 0;
    uint64_t size64 = size;

    if (mActiveUris.find(path) != mActiveUris.end()) {
#ifdef CHANNEL_SWITCH_BLOCKS_READ
        while (!mIsCdmSetupDone->getValue()) {
            // Wait for CDM setup complete
            mIsCdmSetupDone->waitForValue(true);
       }
#endif
        if (mMediaSource->isChannelActive()) {
            res = mTsbConsumer->readData(handle, buf, size);
        } else {
            LOG(WARNING) << "Channel not active, return 0";
            return 0;
        }
    } else {

        std::string conf = getConfigValue(handle, path, offset != 0);

        if (conf.empty()) {
            return 0;
        }

        if (offset > conf.size()) {
            return -EOVERFLOW;
        }

        auto readByteNum = std::min(conf.size() - offset, size64);
        auto result = conf.substr(offset, offset + readByteNum);
        memcpy(buf, result.c_str(), readByteNum);
        return readByteNum;
    }
#ifdef ARCH_X86_64
    if (res == 0 && (offset > INT64_MAX - TS_FILE_FOOTER_BYTES) ) {
        memset(buf, 0, size);
        res = size;
    }
#endif

    return res;
}

streamfs::FCCPlugin::FCCPlugin(streamfs::PluginCallbackInterface *cb, Demuxer *demuxer,
                                         std::shared_ptr<DemuxerCallbackHandler> cbHandler, debug_options_t* debugOptions)
        : PluginInterface(cb, debugOptions), mIsReadBlocked(false) {


    InitTracing();
    TRACE_EVENT("fccswith", "FCCPlugin plugin load");
    mIsCdmSetupDone = &MVar<bool>::getVariable(kCdm0);

    mDefferalHandler = std::make_shared<ConstDelayDefHandler>(milliseconds(CHANNEL_READ_TIMEOUT_MS));

    mTsbConsumer = std::make_shared<StreamParser::TimeShiftBufferConsumer>(&debugOptions->tsDumpEnable);

    auto p = new StreamParser::StreamProcessor(
            {
                    std::dynamic_pointer_cast<StreamParser::StreamConsumer>(mTsbConsumer),
                    std::shared_ptr<StreamParser::StreamConsumer>(new StreamParser::EcmCache()),
                    std::shared_ptr<StreamParser::StreamConsumer>(new StreamParser::PSIParser())
            }
    );

    mStreamProcessor.reset(p);

    mMediaSource = std::shared_ptr<MediaSourceHandler>(
            new MediaSourceHandler(demuxer, cbHandler,
                                   mDefferalHandler.get(), mStreamProcessor, &debugOptions->tsDumpEnable));

    initConfigHandlers(mTsbConsumer, mMediaSource.get(), cb);

    LOG(INFO) << "Loading Nokia FCC plugin. Version: v" <<
              PROJECT_MAJOR_VERSION << "."
              << PROJECT_MINOR_VERSION << "-" << PROJECT_REVISION;

    for (auto &i : available_channels_g) {
        mFiles.insert(i);
    }

    for (const auto &it : available_channels_g) {
        mActiveUris.insert(it);
    }

    for (auto &i : config_handlers_g) {
        mFiles.insert(i.config_file);
    }

    std::vector<std::string> allowedFiles(mFiles.begin(), mFiles.end());

    setAvailableStreams(allowedFiles);
}

int streamfs::FCCPlugin::write(std::string node, const char *buf, size_t size, uint64_t offset) {
    TRACE_EVENT("fccfuse", "Writing config file", "node", node.c_str());
    if (offset != 0) {
        LOG(ERROR) << "Offsite write not supported. Short config writes without seek are allowed only";
        return size;
    }

    auto confHandler = targetToConfigHandler(node);

    if (confHandler == nullptr) {
        return size;
    }
    std::string configValue = std::string(buf, size);
    return confHandler->writeConfig(node, configValue, size);
}

uint64_t streamfs::FCCPlugin::getSize(std::string path) {
    if (mActiveUris.find(path) != mActiveUris.end()) {
        return INT64_MAX;
    } else {
        return getConfigSize(path);
    }
}

streamfs::FCCPlugin::~FCCPlugin() = default;

template<>
void BufferProducer<buffer_chunk>::stop() {}
