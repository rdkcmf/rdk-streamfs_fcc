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

#include <mutex>
#include <streamfs/BufferPool.h>
#include <streamfs/ByteBufferPool.h>
#include "StreamParser/StreamProcessor.h"
#include "StreamParser/BufferIndexer.h"
#include "HandleContext.h"
#include "utils/TimeoutWatchdog.h"
#include "utils/TimeIntervalMonitor.h"
#include "PlayerStateEnum.h"
#include "utils/CyclicEventTimer.h"
#include "utils/MonitoredVariable.h"
#ifdef TS_PACKAGE_DUMP
#include "SocketServer.h"
#endif

#define PAUSE_POST_READ_RATE_TIMEOUT_MS 1000
#define TRICK_PLAY_RATE_MS 350

//TODO: move to includes

namespace StreamParser {

class TimeShiftBufferConsumer : public StreamConsumer,
        public std::enable_shared_from_this<TimeShiftBufferConsumer>
{
    typedef BufferProducer<buffer_chunk> BufProducerType;

public:

    class TsBufferProducer : public BufferProducer<buffer_chunk>
    {

    };

    TimeShiftBufferConsumer(std::atomic<bool> *tsDumpEnable)
        : StreamConsumer("TimeShift", false)
        , mTsDumpEnable(tsDumpEnable)
        , mSeekByteOffset(0)
        , mIsStreaming(false)
        , mIsPaused(false)
        , mPlayerState(PlayerStateEnum::StateType::UNDEF)
        , mTrickPlaySpeed(1) {
        allocateBufferQueue();
        mBufIndexer = std::make_shared<BufferIndexer>(BUFFER_POOL_SIZE, BUFFER_POOL_TAIL_SIZE, BUFFER_SAMPLING_RATIO);
        mBufferReadWatchdog = std::make_shared<TimeoutWatchdog>(PAUSE_POST_READ_RATE_TIMEOUT_MS, [this](bool expired){
            // If this callback is being invoked, it means that the watchdog timer has expired or been stopped
            // The value og expired will indicate if the watchdog has expired/time-out (true) or if it has
            // been stopped. Set the state of mIsPaused accordingly.
            mIsPaused = expired;
        });

        mTrickPlayTimer = std::make_shared<CyclicEventTimer>([this](){
            // The CyclicEventTimer instance will ensure that updateTrickPlayPosition
            // is called periodically with the duration defined by TRICK_PLAY_RATE_MS.
            //
            // updateTrickPlayPosition will emulate trick play by changing seek
            // position relative to the current seek position, speed and direction.
            // When the TSB boundary is reached, the return value
            // from updateTrickPlayPosition will be true, which tells the
            // CyclicEventTimer instance to stop period invocation of this
            // closure.
            return updateTrickPlayPosition();
        });

        mTrickPlayTimer->setPeriodInMilliseconds(TRICK_PLAY_RATE_MS);

        mFlush = &MVar<ByteVectorType>::getVariable(kFlush0);
        mTrickPlayFile = &MVar<ByteVectorType>::getVariable(kTrickPlay0);
    }

    ~TimeShiftBufferConsumer() override {
        mRingBufferPool->abortAllOperations();
    }

    void interruptReads() {
        mRingBufferPool->abortAllOperations();
    }

    void post(const StreamParser::Buffer& buf) override;

    /**
     * Set trick play speed and direction.
     *
     * Direction depends upon the sign of the speed value,
     * where negative values means rewind and positive
     * values means forward.
     *
     * @param speed (seconds)
     * @return - trick play started (true) or failure (false)
     */
    bool setTrickPlaySpeed(int16_t speed);

    /**
     * Get trick play speed and direction
     *
     * @return trick play speed and direction
     */
    int16_t getTrickPlaySpeed();

    /**
     * Set seek time for current stream in milliseconds
     *
     * @param seekTime (milliseconds)
     * @return - seek success (true) or failure (false)
     */
    bool setSeekTime(time_t seekTime);

    /**
     * Get current seek time in milliseconds
     * @return current seek time in seconds
     */
    time_t getSeekTime();

    /**
     * Set the player state.
     * @param state - StateType enum indicating the state.
     */
    void setPlayerState(PlayerStateEnum::StateType state);

    /**
     * Set the play state
     * @param state - StateType enum indicating the state.
     */
    PlayerStateEnum::StateType getPlayerState() const;

    /**
     * Get current maximum available time duration in the TSB buffer
     * @return available time duration in the TSB buffer in milliseconds
     */
    time_t getMaxSeekTime();

    /**
     * Read the data from the TSB with a buffer size equal to the
     * size parameter. The read will be done relative to the end
     * of the last read position for the particular handle.
     *
     * @param handle - file handle
     * @param data   - reference to buffer where to store the data
     * @param size   - number of bytes to read
     * @return - the number of bytes read.
     */
    size_t readData(uint64_t handle, char *data, size_t size);

    void onEndOfStream(const char *channelId) override;

    void onOpen(const char *channelId) override;

    /**
     * Release / delete file handle context.
     * @param handle - the file handle to release
     */
    void release(uint64_t handle);

    /**
     * Get seek offset in bytes
     *
     * @return the seek offset in bytes
     */
    int64_t getSeekOffset();

    /**
     * Get actual buffer size in bytes
     *
     * @return actual buffer size in bytes
     */
    uint64_t getActualBufferByteSize();

    /**
     * Get maximum buffer size in bytes
     *
     * @return maximum buffer size in bytes
     */
    uint64_t getBufferCapacityByteSize();

private:
    std::mutex mSeekMtx;
    std::mutex mParamMtx;
    std::mutex mTrickPlaySpeedMtx;
    std::atomic<bool> *mTsDumpEnable;
    std::atomic<uint64_t> mSeekByteOffset;
    std::atomic<bool> mIsStreaming;
    std::atomic<bool> mIsPaused;
    std::atomic<PlayerStateEnum::StateType> mPlayerState;
    int16_t mTrickPlaySpeed;

    std::shared_ptr<TsBufferProducer> mTsBufProd;
    std::shared_ptr<ByteBufferPool> mRingBufferPool;
    std::shared_ptr<BufferIndexer> mBufIndexer;
    std::shared_ptr<TimeoutWatchdog> mBufferReadWatchdog;
    std::shared_ptr<CyclicEventTimer> mTrickPlayTimer;

    std::map<uint64_t, HandleContext> mHandles;

    TimeIntervalMonitor mPauseTimeMonitor;

    MVar<ByteVectorType> *mFlush;
    MVar<ByteVectorType> *mTrickPlayFile;

#ifdef TS_PACKAGE_DUMP
    SocketServer mSocketServer;
#endif

private:

    void allocateBufferQueue();

    /**
     * Get total virtual buffer size in bytes
     *
     * @return total virtual buffer size in bytes
     */
    uint64_t getTotalBufferByteCount();

    /**
     * Get the EPOC timestamp for the current buffer pool read position.
     *
     * @return delta time in us
     */
    uint64_t getTimestampUsForCurrentByteReadIndex();

    /**
     * Reset seek states.
     */
    void reset();

    /**
     * Updates current seek position based on the trick play speed.
     * Returns false if trick play shall continue or true if trick play
     * shall stop. This will happen when the seek value reaches TSB
     * boundary. In conjunction with this the seek value will be
     * truncated accordingly.
     *
     * @return boolean indicating if trick play shall stop (true)
     * or not (false).
     */
    bool updateTrickPlayPosition();

    /**
     * Updates the trick play file fs-node via poll.
     */
    void updateTrickPlayFile();
};

}//namespace StreamParser
