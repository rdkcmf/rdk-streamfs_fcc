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

#include "StreamParser/BufferIndexer.h"
#include <streamfs/ByteBufferPool.h>
#include "TimeShiftBufferConsumer.h"
#include <glog/logging.h>

void ByteBufferPool::pushBuffer(const buffer_chunk &buffer, bool lastBuffer, size_t lastBufferSize) {
    BufferPool::pushBuffer(buffer, lastBuffer, lastBufferSize);
}

namespace StreamParser {
class SampleConsumer : public BufferConsumer<buffer_chunk> {

public:
    void newBufferAvailable(buffer_chunk &buffer) override {
        //TODO: implement this
        UNUSED(buffer);
    }

    explicit SampleConsumer() = default;
};

void TimeShiftBufferConsumer::post(const StreamParser::Buffer &buf) {
    StreamConsumer::post(buf);
    /**
     * TODO: need to do more testing and validation if we will not receive
     * any buffers from the previous stream;
     */
    if (!mIsStreaming) {
        LOG(WARNING) << " Stream OFF. Ignore buffer for: " << buf.channelInfo;
        return;
    }

    mTsBufProd->queueBuffer(*buf.chunk, false, 0);
    auto bufIndexerRetValue = mBufIndexer->registerBufferCount(getTotalBufferByteCount());

    if (mPlayerState == PlayerStateEnum::StateType::PAUSED) {
        if (bufIndexerRetValue.first && bufIndexerRetValue.second == 1) {
            // Start the watch dog if player state was changed to PAUSED before
            // buffers became available in the TSB. In this case the watch dog
            // timer cannot be started in setPlayerState, since there are no
            // buffers in the buffer pool. This condition is met if
            // the buffer size was registered in the bufferIndexer
            // (bufIndexerRetValue.first == true) AND the circular buffer size
            // is 1 (bufIndexerRetValue.second == 1) AND the player is PAUSED.
            mBufferReadWatchdog->start();
        }

        if (mIsPaused) {
            // If we are here, it means that player is paused and has stopped
            // reading buffers from the TSB. mIsPaused is set to true when
            // the watchdog callback is invoked and the timer is expired.
            //
            // To ensure that mSeekByteOffset maintains the correct TSB read
            // offset from the live point, the delta bytes between the live
            // point and current read position is calculated and the seek
            // offset and current read position is incremented accordingly.
            std::lock_guard <std::mutex> paramLock(mParamMtx);

            uint64_t livePos = getTotalBufferByteCount();
            uint64_t actualPos = mHandles.begin()->second.getCurrentReadOffset();
            uint64_t deltaBytes = 0;

            if (livePos > actualPos)  {
                deltaBytes = livePos - actualPos;
                mPauseTimeMonitor.stopTimeInterval();
            } else {
                mPauseTimeMonitor.updateTimeInterval();
            }

            mHandles.begin()->second.incReadOffset(deltaBytes);

            mSeekByteOffset += deltaBytes;

            uint64_t maxSize = mBufIndexer->getIndexSizeInBytes();

            if (maxSize < mSeekByteOffset) {
                mSeekByteOffset = maxSize;
            }
        } else {
            mPauseTimeMonitor.updateTimeInterval();
        }
    }
}

bool TimeShiftBufferConsumer::setTrickPlaySpeed(int16_t speed) {
    std::lock_guard<std::mutex> paramGuard(mTrickPlaySpeedMtx);
    if (speed && speed != mTrickPlaySpeed) {
        mTrickPlaySpeed = speed;
        mTrickPlayTimer->start();
        updateTrickPlayFile();
        return true;
    }
    return false;
}

int16_t TimeShiftBufferConsumer::getTrickPlaySpeed() {
    std::lock_guard<std::mutex> paramGuard(mTrickPlaySpeedMtx);
    return mTrickPlaySpeed;
}

bool TimeShiftBufferConsumer::updateTrickPlayPosition() {
    std::lock_guard<std::mutex> paramGuard(mTrickPlaySpeedMtx);
    bool stopTrickPlay = true;

    time_t currentSeek = getSeekTime();
    time_t newSeek = 0;

    if (mTrickPlaySpeed == 1) {
        newSeek = currentSeek;
    } else {
        time_t deltaSeek = TRICK_PLAY_RATE_MS * mTrickPlaySpeed;
        if (deltaSeek < 0) {
            // Rewind - calculate new seek position:
            // Subtract the delta-seek position (a positive value) from the
            // current seek position and add TRICK_PLAY_RATE_MS to compensate
            // for the additional live data added to the TSB.
            newSeek = currentSeek - deltaSeek + TRICK_PLAY_RATE_MS;
            if (newSeek < getMaxSeekTime()) {
                stopTrickPlay = false;
            } else {
                mTrickPlaySpeed = 1;
            }
        } else {
            // Fast forward  - calculate new seek position:
            // Subtract the delta-seek position (a negative value) from the
            // current seek position and subtract TRICK_PLAY_RATE_MS to compensate
            // for the additional live data added to the TSB.
            newSeek = currentSeek - deltaSeek - TRICK_PLAY_RATE_MS;
            if (newSeek > 0) {
                stopTrickPlay = false;
            } else {
                newSeek = 0;
                mTrickPlaySpeed = 1;
            }
        }
    }

    mRingBufferPool->enableReadThrottling(stopTrickPlay);
    setSeekTime(newSeek);
    // Flush gstreamer pipeline
    std::string sCh = "seek change";
    *mFlush = ByteVectorType(sCh.begin(), sCh.end());

    if (mTrickPlaySpeed == 1) {
        updateTrickPlayFile();
    }

    return stopTrickPlay;
}

void TimeShiftBufferConsumer::updateTrickPlayFile() {
    std::string trickPlaySpeedString = std::to_string(mTrickPlaySpeed);
    *mTrickPlayFile = ByteVectorType(trickPlaySpeedString.begin(), trickPlaySpeedString.end());
}

bool TimeShiftBufferConsumer::setSeekTime(time_t seekTime) {
    std::lock_guard<std::mutex> seekGuard(mSeekMtx);

    uint64_t maxSeekTime = mBufIndexer->getIndexSizeInTimeUs() / 1e3;

    if ((uint64_t) seekTime > maxSeekTime) {
        LOG(WARNING) << __FUNCTION__ << ": seekTime=" << seekTime << " is out of range!"
                     << " Will be truncated to max.seekTime=" << maxSeekTime;
        seekTime = maxSeekTime;
    }

    uint64_t byteOffset;
    switch (mBufIndexer->getByteOffsetFromTimeUs(seekTime * 1e3, byteOffset)) {
        case BUF_OK:
            break;
        case BUF_OUT_OF_RANGE:
            LOG(WARNING) << __FUNCTION__ << " Invalid seek time";
            return false;
        case BUF_EMPTY:
            LOG(WARNING) << __FUNCTION__ << " Buffer is empty";
            return false;
    }

    // Set member variables holding the current seek time and byte offset
    mSeekByteOffset = byteOffset;

    // Reset the watchdog and pause monitor timer
    mBufferReadWatchdog->clear();
    mPauseTimeMonitor.reset();

    // Initialize the accumulated player read offset to point at
    // live position, which is at the byte offset position equal
    // to the total buffer pool size in bytes.
    uint64_t bufferSize = getTotalBufferByteCount();
    for (auto& kv : mHandles) {
        kv.second.initReadOffset(bufferSize);
    }

    // Restart buffer read watchdog if seek is initiated during
    // pause, since pipeline flushing will introduce buffer reading
    // even though the player is in pause state.
    if (mPlayerState == PlayerStateEnum::StateType::PAUSED) {
        mBufferReadWatchdog->start();
    }

    LOG(INFO) << __FUNCTION__ << ": set seek time=" << seekTime
              << " --> seekByteOffset=" << mSeekByteOffset
              << " max.seek time=" << maxSeekTime;

    return true;
}

uint64_t TimeShiftBufferConsumer::getTimestampUsForCurrentByteReadIndex() {
    uint64_t actualPos = mHandles.begin()->second.getCurrentReadOffset() - mSeekByteOffset;
    uint64_t time = 0;
    mBufIndexer->getTimestampUsForByteIndex(actualPos,time);
    return time;
}

time_t TimeShiftBufferConsumer::getSeekTime() {
    std::lock_guard<std::mutex> paramGuard(mParamMtx);
    uint64_t seekTime = 0;

    // Get the corresponding seek time based on current seek byte offset.
    mBufIndexer->getTimeUsFromByteOffset(mSeekByteOffset,seekTime);
    seekTime += mPauseTimeMonitor.getAccumulatedTimeInMicroSeconds();

    return (seekTime / 1e3);
}

void TimeShiftBufferConsumer::setPlayerState(PlayerStateEnum::StateType state) {
    switch (state) {
        case PlayerStateEnum::StateType::PLAYING:
            // If the player changes state to PLAYING, stop the pause post read
            // watchdog timer.
            mBufferReadWatchdog->stop();
            mPauseTimeMonitor.stopTimeInterval();
            mIsPaused = false;
            break;
        case PlayerStateEnum::StateType::PAUSED:
            // If the player is paused and there is data in the TSB, store the
            // timestamp associated with the current TSB read position and start
            // the pause post read watchdog timer. When the timer expires
            // the total post read time (mPausePostBufferTimeUs) will be calculated
            // in the watchdog callback.
            if (mBufIndexer->getIndexSizeInBytes()) {
                mBufferReadWatchdog->start();
            }
            break;
        default:
            return;
    }
    mPlayerState = state;
}

PlayerStateEnum::StateType TimeShiftBufferConsumer::getPlayerState() const {
    return mPlayerState;
}

time_t TimeShiftBufferConsumer::getMaxSeekTime() {
    return mBufIndexer->getIndexSizeInTimeUs() / 1e3;
}

size_t TimeShiftBufferConsumer::readData(uint64_t handle, char *data, size_t size) {
    std::lock_guard<std::mutex> seekGuard(mSeekMtx);
    auto ctx = mHandles.find(handle);

    if (ctx == mHandles.end()) {
        HandleContext tCtx;
        mHandles[handle] = tCtx;
        ctx = mHandles.find(handle);
        ctx->second.initReadOffset(getTotalBufferByteCount());
        LOG(INFO) << "Added new context for file handle: " << handle << " readOffset = "
        << ctx->second.getCurrentReadOffset() << " seekOffset = " << mSeekByteOffset;
    }

    auto& hCtx = ctx->second;

    uint64_t bufferPoolOffset = hCtx.getCurrentReadOffset() - mSeekByteOffset;
    size_t readSize = mRingBufferPool->readRandomAccess(data, size, bufferPoolOffset);

    if (readSize > 0) {
        hCtx.incReadOffset(readSize);
    }

    if (mPlayerState == PlayerStateEnum::StateType::PAUSED) {
        mBufferReadWatchdog->restart();
    }

#ifdef TS_PACKAGE_DUMP
    mSocketServer.send(data, size);
#endif

    return readSize;
}

void TimeShiftBufferConsumer::onEndOfStream(const char *channelId) {
    StreamConsumer::onEndOfStream(channelId);
    mIsStreaming = false;
    mRingBufferPool->clearToLastRead();
    reset();
}

void TimeShiftBufferConsumer::onOpen(const char *channelId) {
    StreamConsumer::onOpen(channelId);
    mIsStreaming = true;
#ifdef TS_PACKAGE_DUMP
    if (mTsDumpEnable != nullptr) {
        if (*mTsDumpEnable) {
            mSocketServer.start(PLAYER_READ_TS_DUMP_SOCKET_PORT);
        } else {
            mSocketServer.stop();
        }
    }
#endif
}

void TimeShiftBufferConsumer::release(uint64_t handle) {
    mHandles.erase(handle);
}

void TimeShiftBufferConsumer::allocateBufferQueue() {

    std::shared_ptr<SampleConsumer> scons = std::make_shared<SampleConsumer>();

    ByteBufferPool::shared_consumer_type consumer = std::dynamic_pointer_cast<BufferConsumer<buffer_chunk>>(scons);

    mTsBufProd = std::make_shared<TsBufferProducer>();

    ByteBufferPool::shared_producer_type producer = std::dynamic_pointer_cast<BufferProducer<buffer_chunk>>(mTsBufProd);

    mRingBufferPool = std::make_shared<ByteBufferPool>(producer, consumer, BUFFER_POOL_SIZE);

}

int64_t TimeShiftBufferConsumer::getSeekOffset() {
    return mSeekByteOffset;
}

uint64_t TimeShiftBufferConsumer::getTotalBufferByteCount() {
    return mTsBufProd->getTotalBufferCount() * BUFFER_CHUNK_SIZE;
}

uint64_t TimeShiftBufferConsumer::getActualBufferByteSize() {
    return mBufIndexer->getIndexSizeInBytes();
}

uint64_t TimeShiftBufferConsumer::getBufferCapacityByteSize() {
    return (mBufIndexer->getBufferCapacity() - 1) * BUFFER_CHUNK_SIZE * BUFFER_SAMPLING_RATIO;
}

void TimeShiftBufferConsumer::reset() {
    TRACE_EVENT(TR_FCC_SWITCH, "reset seek");
    std::lock_guard<std::mutex> paramLock(mParamMtx);
    std::lock_guard<std::mutex> paramGuard(mTrickPlaySpeedMtx);

    if (mSeekByteOffset) {
        uint64_t bufferSize = getTotalBufferByteCount();
        for (auto& kv : mHandles) {
            kv.second.initReadOffset(bufferSize);
        }
    }

    mSeekByteOffset = 0;
    mTrickPlaySpeed = 1;
    updateTrickPlayFile();
    mRingBufferPool->enableReadThrottling(true);
    mTrickPlayTimer->stop();
    mBufIndexer->clear();
    mBufferReadWatchdog->clear();
    mPauseTimeMonitor.reset();
    mIsPaused = false;
}

}
