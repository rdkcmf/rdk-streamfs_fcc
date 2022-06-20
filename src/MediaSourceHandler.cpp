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


#include <chrono>
#include <memory>
#include <utility>
#include <glog/logging.h>
#include <DemuxerCallbackHandler.h>

#include "MediaSourceHandler.h"
#include "streamfs/config.h"
#include "externals.h"
#include "DemuxerStatusCallback.h"
#include <streamfs/LogLevels.h>
#include "Logging.h"
#include "ChannelConfig.h"

std::mutex mBqAllocator;
char NOKIA_BUFFER_MAGIC[4] = {'n', 'k', 'i', 'a'};

#define NO_BUFFER_RECEIVED_THRESHOLD_MS 2000
#define NO_BUFFER_RECEIVED_RECONFIGURE_THRESHOLD_MS 5000
#define BUFFER_CHECK_PERIOD_MS 500

#define NO_CHANNEL_URL "0.0.0.0:5900"

MediaSourceHandler::MediaSourceHandler(Demuxer *dMux,
                                       std::shared_ptr<DemuxerCallbackHandler> cbHandler,
                                       ReadDefferHandler *defHandler,
                                       std::shared_ptr<StreamParser::StreamProcessor> sProc,
                                       std::atomic<bool> *tsDumpEnable)
        : StreamParser::StreamSource(sProc),
          mExitRequested(false),
          mDumpInputStreamEnabled(false),
          mInputStreamDumpFile(nullptr),
          mDmxCb(std::move(cbHandler)
          ) {
    TRACE_EVENT(TR_FCC_SWITCH, "Create MediaSourceHandler");

    mDefHandler = defHandler;
    mTsDumpEnable = tsDumpEnable;

    mDrm = &MVar<StreamProtectionConfig>::getVariable(kDrm0);
    mBufferSrcLostMVar = &MVar<ByteVectorType>::getVariable(kBufferSrcLost0);
    *mBufferSrcLostMVar = srcStateToMVar(mBufferSourceLost);

    for (int i = 0; i < FEIP_DEFAULT_BUFFER_COUNT; i++) {
        auto buf = alloc_bq_buffer(FEIP_BUFFER_SIZE, this);
        bufferRefs[i] = buf;
        mFccBufferQueue.release(buf);
    }

    dMux->attachMediaSourceHandler(this);
    dMux->init();

    std::shared_ptr<FeipSession> sess = std::make_shared<FeipSession>(dMux);

    sess->connectionId = FEIP_CONNECTION_UNINITIALIZED;

    auto ins = std::make_pair(dMux->getId(), sess);

    mSessions.insert(ins);

    mConsumerThread = std::shared_ptr<std::thread>(
            new std::thread(&MediaSourceHandler::consumerLoop, this));

    if (mDmxCb != nullptr) {
        mDmxCb->setCallbackHandler(this, 0);
    } else {
        LOG(ERROR) << "Callback handler is null. Playback will fail";
        exit(0);
    }

    mMessageHandler = std::make_shared<msg_queue_t>();

    mMessageThread = std::shared_ptr<std::thread>(
            new std::thread(&MediaSourceHandler::messageLoop, this));

    mMonitorThread = std::shared_ptr<std::thread>(
            new std::thread(&MediaSourceHandler::dataMonitorLoop, this));

    mNetworkObs = std::make_shared<NetworkRouteObserver>(mMessageHandler.get());
    NetworkRouteMonitor::getInstance().registerCallback(mNetworkObs);
}

void MediaSourceHandler::returnBufferToBQ(bq_buffer *pBuffer) {
    if (pBuffer != nullptr)
        mFccBufferQueue.release(pBuffer);
}

void MediaSourceHandler::reportTSBufferQueued() {
    mLastValidBufferTimeMs = std::chrono::duration_cast< milliseconds >
            (steady_clock::now().time_since_epoch());

    // There may be a race condition here with the
    // dataMonitorLoop that could cause inject one batch of
    // null TS packets. But it is a sacrifice
    // I'm willing to make to avoid mutex handling or complex
    // logic around mBufferSourceLost.

    if (mBufferSourceLost) {
        mBufferSourceLost = false;
        *mBufferSrcLostMVar = srcStateToMVar(mBufferSourceLost);
    }
}

void MediaSourceHandler::pushBuffertoBQ(bq_buffer *pBuffer, int demuxId) {
    UNUSED(demuxId);
    auto session = mSessions.find(demuxId);
    if (session == mSessions.end()) {
        LOG(ERROR) << "Could not find FEIP session for demuxer: " << demuxId;
    }

#ifdef TS_PACKAGE_DUMP
    mSocketServer.send(pBuffer->buffer, pBuffer->size);
#endif
    if (mDumpInputStreamEnabled) {
        fwrite(pBuffer->buffer, pBuffer->size, 1, mInputStreamDumpFile);
    }

    mFccBufferQueue.queue(pBuffer);
    mDefHandler->signal(ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED);

    if (!session->second->firstBufferDisplayed) {
        LOG(INFO) << "Informing first frame for session " << session->second->connectionId;
        session->second->mDemuxer->informFirstFrameReceived();
        session->second->firstBufferDisplayed = true;
    }

}

bq_buffer *MediaSourceHandler::acquireBuffer(int demuxId) {
    UNUSED(demuxId);
    bq_buffer *res = nullptr;
    mFccBufferQueue.acquire(&res);
    return res;
}

int MediaSourceHandler::open(std::string uri, uint32_t demuxer, uint32_t timeout) {
    UNUSED(timeout);
    TRACE_EVENT(TR_FCC_SWITCH, "Open channel", "URI", uri.c_str());
    std::lock_guard<std::mutex> lockGuard(mFeipConfMutex);
    auto session = mSessions.find(demuxer);

#ifdef TS_PACKAGE_DUMP
    if (mTsDumpEnable != nullptr) {
        if (*mTsDumpEnable) {
            mSocketServer.start(DEMUX_OUTPUT_TS_DUMP_SOCKET_PORT);
        } else {
            mSocketServer.stop();
        }
    }
#endif

    if (mCurrentUri == uri) {
        return -1;
    }

#ifdef DEBUG_INPUT_STREAM
    mDumpInputStreamEnabled = true;
    mInputStreamDumpFileName = INPUT_STREAM_DUMP_FILE + uri + ".ts";

    if (mInputStreamDumpFile != NULL) {
        fclose(mInputStreamDumpFile);
    }

    mInputStreamDumpFile = fopen(mInputStreamDumpFileName.c_str(), "w");

    if (mInputStreamDumpFile == nullptr) {
        LOG(ERROR) << "Failed to open dump file " << mInputStreamDumpFileName;
        throw;
    }
    LOG(INFO) << "Dumping input stream to file: " << mInputStreamDumpFileName;
#endif
    auto sess = mSessions.find(demuxer);

    if (sess == mSessions.end()) {
        LOG(ERROR) << "Could not find demuxer" << demuxer;
        return -1;
    }

    auto feip = sess->second;

    auto channelStats = feip->mDemuxer->getChannelStats(true);

    mCurrentChannelConfig = ChannelConfig(uri);
    mCurrentUri = uri;

    disconnect(feip);

    mDmxCb->notifyStreamSwitched(uri);

    if (uri.empty()) {
        goto finish;
    }

    if (!uri.empty()) {
        session->second->firstBufferDisplayed = false;
    }

    connect(feip);

    feip->mDemuxer->open(uri);

    StreamSource::onOpen(uri.c_str());
    mLastValidBufferTimeMs =
            std::chrono::duration_cast< milliseconds >(steady_clock::now().time_since_epoch());

finish:
    return 0;
}

void MediaSourceHandler::consumerLoop() {
    bq_buffer *tmpBuf;
    buffer_chunk chunk;
    size_t offset = 0;
    int count = 0;

    SLOG(INFO, LOG_DATA_SRC) << "Starting consumer thread";

    while (!mExitRequested) {

        if (!mFccBufferQueue.consume(&tmpBuf, std::chrono::seconds(1))) {
            continue;
        }
        count++;
        auto *buffStartPos = tmpBuf->buffer;
        auto remainingInputBytes = tmpBuf->size;
        while (remainingInputBytes > 0) {
            uint32_t remaining_bytes = chunk.size() - offset;
            uint32_t copy_bytes = std::min(remaining_bytes, remainingInputBytes);

            memcpy(&chunk.data()[offset], buffStartPos, copy_bytes);

            if (copy_bytes + offset == chunk.size()) {
                StreamParser::Buffer b = {tmpBuf->channelInfo, &chunk};
                post(b);
            }

            offset = (offset + copy_bytes) % chunk.size();
            buffStartPos += copy_bytes;
            remainingInputBytes -= copy_bytes;
        }

        mFccBufferQueue.release(tmpBuf);
    }

    if (mExitRequested) {
        return;
    }
}
ByteVectorType MediaSourceHandler::srcStateToMVar(bool state) {
    auto result = ( state ? TRUE_STR : FALSE_STR) + "," + std::to_string(mSourceLostCounter);
    return {ByteVectorType (result.begin(), result.end())};
}

void MediaSourceHandler::dataMonitorLoop() {
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(BUFFER_CHECK_PERIOD_MS) );

        auto currentTime =
                std::chrono::duration_cast< milliseconds >(steady_clock::now().time_since_epoch());

        // We are connected for longer time than NO_BUFFER_RECEIVED_THRESHOLD_MS and
        // we got no buffers. We try to re-join
        if (mCurrentChannelConfig.destinationIp != 0)
        {
            auto diff = currentTime.count() - mLastValidBufferTimeMs.load().count();
            if (diff > NO_BUFFER_RECEIVED_RECONFIGURE_THRESHOLD_MS)
            {
                LOG(WARNING) << "Time since last valid buffer : " << diff;
                mMessageHandler->pushMessage(NetworkMessage(NetworkMessage::NO_MULTICAST));
            }
            else if (diff > NO_BUFFER_RECEIVED_THRESHOLD_MS)
            {
                if (!mBufferSourceLost) {
                    mBufferSourceLost = true;
                    mSourceLostCounter ++;
                    *mBufferSrcLostMVar = srcStateToMVar(mBufferSourceLost);
                }
                injectNullBuffers();
            }
        }

    } while (!mExitRequested);
}
void MediaSourceHandler::messageLoop() {

    LOG(INFO) << "Starting network message thread";

    NetworkRouteMonitor::getInstance().start();

    do {
        NetworkMessage msg = mMessageHandler->waitForMessage();
        switch (msg.what) {
            case NetworkMessage::MessageType::NO_GATEWAY:
                LOG(INFO) << "No gateway available";
                for (const auto &kv : mSessions) {
                    kv.second->mDemuxer->open(NO_CHANNEL_URL, "lo");
                }
                break;
            case NetworkMessage::MessageType::NEW_GATEWAY:
            {
                // MediaSourceHandler::open(...) may also call open on the demuxer, so we need a lock here, since it's called from a different thread.
                std::lock_guard<std::mutex> lockGuard(mFeipConfMutex);

                // Give some time for the Nokia FCC to setup. Don't restart for some time.
                mLastValidBufferTimeMs = std::chrono::duration_cast< milliseconds >(steady_clock::now().time_since_epoch());
                LOG(INFO) << "New gateway : " << msg.interface;
                // Here it is assumed that all demuxers (if there are more than one) used the same interface
                for (const auto &kv : mSessions) {
                    kv.second->mDemuxer->open(mCurrentUri, msg.interface);
                }
                break;
            }
            case NetworkMessage::MessageType::NO_MULTICAST:
            {
                LOG(INFO) << "NO_MULTICAST, reconfigure demux : uri : " << mCurrentUri;
                // Give some time for the Nokia FCC to setup. Don't restart for some time.
                mLastValidBufferTimeMs = std::chrono::duration_cast< milliseconds >(steady_clock::now().time_since_epoch());
                // Here it is assumed that all demuxers (if there are more than one) used the same interface
                for (const auto &kv1 : mSessions) {
                    auto sess = kv1.second;
                    if (sess->connectionId != FEIP_CONNECTION_UNINITIALIZED) {
                        sess->mDemuxer->disconnect(sess->connectionId);
                        sess->connectionId = FEIP_CONNECTION_UNINITIALIZED;
                    }
                    auto connectionId = sess->mDemuxer->connect();

                    if (connectionId >= 0) {
                        sess->connectionId = connectionId;
                    }
                    sess->mDemuxer->open(mCurrentUri);
                }
                break;
            }
            default:
                LOG(WARNING) << "Got unhandled network observer message: " << msg.what;

        }
    } while (!mExitRequested);
}

void
MediaSourceHandler::notifyStatusChanged(demuxer_status status, int32_t feipId, uint32_t demuxSessionId) {
    TRACE_EVENT(TR_FCC_SWITCH, "MediaSourceHandler::notifyStatusChanged");
    std::lock_guard<std::mutex> lockGuard(mParamMtx);

    auto sess = mSessions.find(demuxSessionId);

    if (sess == mSessions.end()) {
        LOG(ERROR) << "Failed to find session for demuxer: " << demuxSessionId;
        return;
    }

    LOG(INFO) << "Got new message of type : " << status << " for feip " << feipId;

    if (status == DemuxerStatusCallback::PAT_PMT_UPDATED) {
        LOG(INFO) << "Calling feip_start_ts_inject for feip " << feipId;
        sess->second->mDemuxer->start(feipId);
    }
}

void MediaSourceHandler::disconnect(std::shared_ptr<FeipSession> &sess) {

    TRACE_EVENT(TR_FCC_SWITCH, "Feip disconnect");
    if (sess->connectionId == FEIP_CONNECTION_UNINITIALIZED) {
        LOG(INFO) << "Ignore disconnect request on a disconnected FEIP session";
        return;
    }

    sess->mDemuxer->disconnect(sess->connectionId);
    sess->connectionId = FEIP_CONNECTION_UNINITIALIZED;
    {//lock mutex
        std::lock_guard<std::mutex> lockGuard(mParamMtx);
        onEndOfStream(mCurrentUri.c_str());
    }
}


int MediaSourceHandler::connect(std::shared_ptr<FeipSession> &sess) {
    TRACE_EVENT(TR_FCC_SWITCH, "MediaSourceHandler::connect");

    auto connectionId = sess->mDemuxer->connect();

    if (connectionId >= 0) {
        sess->connectionId = connectionId;
    }

    return connectionId;
}

std::string MediaSourceHandler::getGlobalStats(int demuxerId) {
    std::lock_guard<std::mutex> lockGuard(mFeipConfMutex);

    auto sess = mSessions.find(demuxerId);

    if (sess == mSessions.end()) {
        LOG(ERROR) << "Failed to find session for demuxer: " << demuxerId;
        return "";
    }
    return sess->second->mDemuxer->getGlobalStats();

}

/**
 * Read channel statistics
 * @param demuxerId
 * @return
 */
std::string MediaSourceHandler::getChannelStats(int demuxerId) {
    std::lock_guard<std::mutex> lockGuard(mFeipConfMutex);

    auto sess = mSessions.find(demuxerId);

    if (sess == mSessions.end()) {
        LOG(ERROR) << "Failed to find session for demuxer: " << demuxerId;
        return "";
    }
    return sess->second->mDemuxer->getChannelStats(false);
}

void MediaSourceHandler::injectNullBuffers() {

    auto injCount = 1;

    for(int i = 0; i < injCount; i ++) {
       auto buf = acquireBuffer(0);
       auto* b = (uint8_t* ) buf->buffer;
       do {
           b[0] = 0x47;
           b[1] = 0x00 | 0x1f;
           b[2] = 0xff;
           b[3] = 0x10;
           memset(&b[4], 0x00, TS_PACKAGE_SIZE - 4);
           b += TS_PACKAGE_SIZE;
       }   while (b < (uint8_t *) (buf->buffer + buf->size));

       pushBuffertoBQ(buf, 0);
    }
}
