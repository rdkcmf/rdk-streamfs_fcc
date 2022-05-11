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

#include <tracing.h>

#include <memory>
#include <MediaSourceHandler.h>
#include "HttpDemuxerImpl.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

extern "C" {
static MediaSourceHandler *media_handler_http_g;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
    HttpDemuxerImpl *httpDemuxer = (HttpDemuxerImpl *) stream;
    if (httpDemuxer->mExitRequested) {
        return -1;
    } else
        return httpDemuxer->write(ptr, size, nmemb);
}

HttpDemuxerImpl::HttpDemuxerImpl(uint32_t id) : Demuxer(id) {
    mExitRequested = false;
    mCurrentOffset = 0;
    mHttpThread = nullptr;
    mCb = std::make_shared<HttpCBHandler>();
}

HttpDemuxerImpl::~HttpDemuxerImpl() {
    mExitRequested = true;
    if (mHttpThread) {
        mHttpThread->join();
    }
}

void HttpDemuxerImpl::init() {
    mHttpBufCount = 0;
}

int HttpDemuxerImpl::start(int id) {
    UNUSED(id);
    return 0;
}

int HttpDemuxerImpl::disconnect(int connect_id) {
    UNUSED(connect_id);
    return 0;
}

int HttpDemuxerImpl::connect() {
    return 0;
}

int HttpDemuxerImpl::open(const std::string &uri, const std::string &hwInterface) {
    UNUSED(hwInterface);
    LOG(INFO) << "HttpDemuxerImpl::open entry with uri : " << uri;
    if (mHttpThread != nullptr) {
        mExitRequested = true;
        mHttpThread->join();
    }
    mUri = uri;
    mExitRequested = false;

    mHttpThread = std::shared_ptr<std::thread>(new std::thread(&HttpDemuxerImpl::loop, this));

    return 0;
}

void HttpDemuxerImpl::informFirstFrameReceived() {
}

int HttpDemuxerImpl::setDemuxerParameters(const std::string &params) {
    UNUSED(params);
    return 0;
}

void HttpDemuxerImpl::attachMediaSourceHandler(MediaSourceHandler *handler) {
    Demuxer::attachMediaSourceHandler(handler);
    media_handler_http_g = handler;
}

std::string HttpDemuxerImpl::getGlobalStats() {
    return std::string();
}

std::string HttpDemuxerImpl::getChannelStats(bool resetCounters) {
    UNUSED(resetCounters);
    return std::string();
}

size_t HttpDemuxerImpl::write(void *ptr, size_t size, size_t nmemb) {
    auto total_size = nmemb * size;
    auto remaining_data = total_size;

    while (remaining_data > 0) {
        auto freeBytesInTempBuf = FEIP_BUFFER_SIZE - mCurrentOffset;

        if (freeBytesInTempBuf == FEIP_BUFFER_SIZE) {
            httpBuffer[mHttpBufCount] = media_handler_http_g->acquireBuffer(0);
        }

        auto writeLength = std::min(freeBytesInTempBuf, remaining_data);

        memcpy(&httpBuffer[mHttpBufCount]->buffer[mCurrentOffset], &((char *) ptr)[total_size - remaining_data],
               writeLength);
        mCurrentOffset += writeLength;

        if (mCurrentOffset == FEIP_BUFFER_SIZE) {
            httpBuffer[mHttpBufCount]->size = mCurrentOffset;
            httpBuffer[mHttpBufCount]->capacity = mCurrentOffset;
            if(httpBuffer[mHttpBufCount]->channelInfo != nullptr)
            {
                free(httpBuffer[mHttpBufCount]->channelInfo);
            }
            httpBuffer[mHttpBufCount]->channelInfo = strdup(mUri.c_str());
            media_handler_http_g->pushBuffertoBQ(httpBuffer[mHttpBufCount], 0);
            mCurrentOffset = 0;
            mHttpBufCount++;
            if (0 == (mHttpBufCount % FEIP_DEFAULT_BUFFER_COUNT))
                mHttpBufCount = 0;
        }

        remaining_data -= writeLength;
    }
    return size * nmemb;
}

void HttpDemuxerImpl::loop() {
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    LOG(INFO) << " HttpDemuxerImpl::loop entry, mUri : " << mUri;

    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, FEIP_BUFFER_SIZE);
    curl_easy_setopt(curl_handle, CURLOPT_URL, mUri.c_str());

    int result = curl_easy_perform(curl_handle);

    LOG(INFO) << " HttpDemuxerImpl::loop Exit, mUri : " << mUri << " result : " << result;

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}

std::shared_ptr<DemuxerCallbackHandler> HttpDemuxerImpl::getCallbackHandler() {
    return std::dynamic_pointer_cast<DemuxerCallbackHandler>(mCb);
}
