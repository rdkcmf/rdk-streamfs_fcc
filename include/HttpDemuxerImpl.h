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

#include "Demuxer.h"
#include <memory>
#include <curl/curl.h>
#include <streamfs/BufferPool.h>
#include <streamfs/ByteBufferPool.h>

#include "externals.h"

class HttpCBHandler : public DemuxerCallbackHandler {
public :
    HttpCBHandler() {}

    void notifyStreamSwitched(const std::string &channelIp) { UNUSED(channelIp); }
};

class HttpDemuxerImpl : public Demuxer {

public:
    explicit HttpDemuxerImpl(uint32_t id);

    ~HttpDemuxerImpl();

    int start(int id) override;

    void init() override;

    int disconnect(int connect_id) override;

    int connect() override;

    int open(const std::string &uri, const std::string &hwInterface = "") override;

    void attachMediaSourceHandler(MediaSourceHandler *handler) override;

    int setDemuxerParameters(const std::string &params) override;

    void informFirstFrameReceived() override;

    std::string getGlobalStats() override;

    std::string getChannelStats(bool resetCounters) override;

    size_t write(void *ptr, size_t size, size_t nmemb);

    std::shared_ptr<DemuxerCallbackHandler> getCallbackHandler() override;

    bool mExitRequested;

private:
    void loop();

    std::shared_ptr<std::thread> mHttpThread;
    CURL *curl_handle;
    std::string mUri;
    size_t mCurrentOffset;
    uint32_t mHttpBufCount;
    struct bq_buffer *httpBuffer[FEIP_DEFAULT_BUFFER_COUNT];
    std::shared_ptr<HttpCBHandler> mCb;
};
