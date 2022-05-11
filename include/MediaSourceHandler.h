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

#include <cstdint>
#include <cstdio>
#include <thread>
#include <ctime>
#include <atomic>

#include <boost/circular_buffer.hpp>

#include <streamfs/BufferPool.h>
#include <streamfs/ByteBufferPool.h>
#include "BufferQueue.h"
#include "externals.h"
#include "Demuxer.h"
#include "DemuxerStatusCallback.h"
#include "DemuxerCallbackHandler.h"
#include "ReadDefferHandler.h"
#include "network/NetworkRouteObserver.h"
#ifdef TS_PACKAGE_DUMP
#include "SocketServer.h"
#endif

#include "utils/MonitoredVariable.h"
#include "StreamParser/StreamSource.h"
#include "StreamParser/StreamProtectionConfig.h"
#include "ChannelConfig.h"

#include <chrono>
using namespace std::chrono;


#define FEIP_CONNECTION_UNINITIALIZED -0xFF

class Demuxer;

class FeipSession {
public:
    FeipSession(Demuxer *dMux) :
            mDemuxer(dMux),
            connectionId(0),
            firstBufferDisplayed(false) {}

    ~FeipSession() {
    }

public:
    Demuxer *mDemuxer;
    std::weak_ptr<DemuxerCallbackHandler> mDmxCb;
    int connectionId;
    bool firstBufferDisplayed;
private:
    FeipSession(const FeipSession &) = delete;
};

class MediaSourceHandler :
        public DemuxerStatusCallback,
        public StreamParser::StreamSource
        {
    typedef std::shared_ptr<FeipSession> session_ptr_t;

public:

    virtual ~MediaSourceHandler() {
        mExitRequested = true;

        if (mConsumerThread != nullptr && mConsumerThread->joinable()) {
            mConsumerThread->join();
        }

        if (mMessageThread != nullptr && mMessageThread->joinable()) {
            mMessageThread->join();
        }

        if (mInputStreamDumpFile != nullptr) {
            fclose(mInputStreamDumpFile);
        }

        if (mMonitorThread != nullptr) {
            mMonitorThread->join();
        }

        mFccBufferQueue.clear();

        for (int i = 0; i < FEIP_DEFAULT_BUFFER_COUNT; i++) {
            free_bq_buffer(bufferRefs[i]);
        }
    }

    /**
     * Change channel
     * @param uri  - URI
     * @param demuxer  - demuxer (default is 0)
     * @param timeout  - timeout for channel change in ms. Setting 0 means no timeout
     * @return
     */
    int open(std::string uri, uint32_t demuxer, uint32_t timeout = 0);

    std::string getGlobalStats(int demuxerId);

    std::string getChannelStats(int demuxerId);


    /**
     * Inform that a valid TB buffer is queued.
     * Buffers queued using pushBuffertoBQ may be valid TS buffers
     * or null TS buffers in case of connectivity drop.
     * Calling reportTSBufferQueued allows the Monitor thread
     * to detect if there is a connectivity issue.
     */
    void reportTSBufferQueued();

public:

    MediaSourceHandler(Demuxer *dMux, std::shared_ptr<DemuxerCallbackHandler> cbHandler,
                       ReadDefferHandler *defHandler,
                       std::shared_ptr< StreamParser::StreamProcessor> sProc,
                       std::atomic<bool> *tsDumpEnable);

    MediaSourceHandler(MediaSourceHandler const &) = delete;

    void operator=(MediaSourceHandler const &) = delete;

    /**
     * Queue a single buffer
     * @param pBuffer
     */
    void pushBuffertoBQ(bq_buffer *pBuffer, int demuxId);

    bq_buffer *acquireBuffer(int demuxId);

    /**
     * Release unused buffer. Buffer will be not queued for display
     */
    void returnBufferToBQ(bq_buffer *pBuffer);

    void notifyStatusChanged(demuxer_status status, int32_t feipId, uint32_t demuxSessionId) override;


    /**
     * Get current active URI
     * @return active channel
     */
    std::string getCurrentUri() {
        std::lock_guard<std::mutex> lockGuard(mParamMtx);
        return mCurrentUri;
    }

    /**
     * Check if we have an active channel set
     * @return
     */
    bool isChannelActive() {
        std::lock_guard<std::mutex> lockGuard(mParamMtx);
        return ((mCurrentChannelConfig.destinationIp != 0)
             || (0 == mCurrentUri.compare(0, 4, "http")));
    }

private:
    /**
     * Connect FEIP.
     * @param demuxer
     * @return > 0 Feip connection ID on success. negative value on failure.
     */
    int connect(session_ptr_t &sess);

    /**
     * Disconnect feip session
     * @param sess
     */
    void disconnect(session_ptr_t &sess);

    /**
     * Main thread for filling the Time Shift Buffer
     */
    void consumerLoop();

    /**
     * Thread for consuming network change related messages
     */
    void messageLoop();

private:
    BufferQueue<bq_buffer, FEIP_DEFAULT_BUFFER_COUNT> mFccBufferQueue;
    std::map<uint32_t, session_ptr_t> mSessions;
    bool mExitRequested;
    std::shared_ptr<std::thread> mConsumerThread;
    bool mIsFeipConnected;
    std::mutex mFeipConfMutex;

private:
    bool mDumpInputStreamEnabled;
    std::string mInputStreamDumpFileName;
    FILE *mInputStreamDumpFile;

    std::string mCurrentUri;
    std::shared_ptr<DemuxerCallbackHandler> mDmxCb;
    bq_buffer *bufferRefs[FEIP_DEFAULT_BUFFER_COUNT];
    std::mutex mParamMtx;

    std::shared_ptr<NetworkRouteMonitor::NetworkRouteCallback> mNetworkObs;
    std::shared_ptr<std::thread> mMessageThread;
    std::shared_ptr<std::thread> mMonitorThread;

    std::shared_ptr<msg_queue_t> mMessageHandler;

    //Read defferal handler
    ReadDefferHandler *mDefHandler;
    std::atomic<bool> *mTsDumpEnable;

    MVar<StreamProtectionConfig> *mDrm;

    // Time of the last buffer received
    std::atomic<milliseconds> mLastValidBufferTimeMs;

    // Set to true if we are not receiving valid buffers
    // for certain period of time
    MVar<ByteVectorType> *mBufferSrcLostMVar;

    void dataMonitorLoop();

    // Inject fixed number of null TS buffers
    void injectNullBuffers();

    ChannelConfig mCurrentChannelConfig;

    std::atomic<bool> mBufferSourceLost = {false};

    // Times we lost the source due to unknown error
    unsigned int mSourceLostCounter = {0};

    /**
     * Convert source state to mutable variable.
     * Output format is <bool - status>,<unsigned int - error count>
     * @param state
     */
    ByteVectorType srcStateToMVar(bool state);

#ifdef TS_PACKAGE_DUMP
    SocketServer mSocketServer;
#endif
};

